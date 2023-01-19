/*
Dynamic Script Engine Plugin for Touch Portal
Copyright Maxim Paperno; all rights reserved.

This file may be used under the terms of the GNU
General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

A copy of the GNU General Public License is available at <http://www.gnu.org/licenses/>.

This project may also use 3rd-party Open Source software under the terms
of their respective licenses. The copyright notice above does not apply
to any 3rd-party components used within.
*/

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QMetaObject>
#include <QThread>

#include "Plugin.h"
#include "common.h"
#include "DSE.h"
#include "Logger.h"
#include "DynamicScript.h"
#include "ScriptEngine.h"
#include "ConnectorData.h"

enum ActionTokens : quint8 {
	AT_Unknown,

	AH_Script,
	AH_Plugin,

	SA_Eval,
	SA_Load,
	SA_Import,
	SA_Update,
	SA_SingleShot,  // deprecated, remove

	CA_Instance,
	CA_DelScript,
	CA_DelEngine,
	CA_ResetEngine,
	CA_SetStateValue,  // deprecated, remove
	CA_RepeatRate,

	AT_Script,
	AT_Engine,
	AT_Shared,
	AT_Private,
	AT_Default,
	AT_All,
	AT_Rate,
	AT_Delay,
	AT_RateDelay,
	AT_Set,
	AT_Increment,
	AT_Decrement,
};

Plugin *Plugin::instance = nullptr;

static std::atomic_uint32_t g_errorCount = 0;
static bool g_startupComplete = false;

int tokenFromName(const QByteArray &name, int deflt = AT_Unknown)
{
	static const QHash<QByteArray, int> hash = {
		{ "script",  AH_Script },
	  { "plugin",  AH_Plugin },

		{ "eval",    SA_Eval },
	  { "load",    SA_Load },
	  { "import",  SA_Import },
	  { "update",  SA_Update },
	  { "oneshot", SA_SingleShot },

	  { "instance",                 CA_Instance },
	  { "Delete Script Instance",   CA_DelScript },
	  { "Delete Instance",          CA_DelScript },  // BC with v < 1.1.0.2
	  { "Delete Engine Instance",   CA_DelEngine },
	  { "Reset Engine Environment", CA_ResetEngine },
	  { "Set State Value",          CA_SetStateValue },   // BC with v < 1.1.0.2

	  { "repRate",      CA_RepeatRate },

	  { "Script",       AT_Script },
	  { "Engine",       AT_Engine },
	  { "Shared",       AT_Shared },
	  { "Private",      AT_Private },
	  { "Default",      AT_Default },
	  { "All",          AT_All },
	  { "Rate",         AT_Rate },
	  { "Delay",        AT_Delay },
	  { "Rate & Delay", AT_RateDelay },
	  { "Set",          AT_Set },
	  { "Increment",    AT_Increment },
	  { "Decrement",    AT_Decrement },
	};
	return hash.value(name, deflt);
}

static QByteArray tokenToName(int token, const QByteArray &deflt = QByteArray())
{
	static const QHash<int, QByteArray> hash = {
		{ SA_Eval,       "Eval" },
		{ SA_Load,       "Load" },
		{ SA_Import,     "Import" },
		{ SA_Update,     "Update" },

	  { CA_Instance,       "instance" },
	  { CA_DelScript,      "Delete Script Instance" },
	  { CA_DelScript,      "Delete Engine Instance" },
	  { CA_ResetEngine,    "Reset Engine Environment" },
	  { CA_SetStateValue,  "Set State Value" },
	  { CA_RepeatRate,     "RepeatRate" },

	  { AT_Script,    "Script" },
	  { AT_Engine,    "Engine" },
	  { AT_Shared,    "Shared" },
	  { AT_Private,   "Private" },
	  { AT_Default,   "Default" },
	  { AT_All,       "All" },
	  { AT_Rate,      "Rate" },
	  { AT_Delay,     "Delay" },
	  { AT_RateDelay, "Rate & Delay" },
	  { AT_Set,       "Set" },
	  { AT_Increment, "Increment" },
	  { AT_Decrement, "Decrement" },
	};
	return hash.value(token, deflt);
}

static DSE::EngineInstanceType stringToScope(QStringView str, bool unknownIsPrivate = false)
{
	return str == QStringLiteral("Shared") ? DSE::SharedInstance : (unknownIsPrivate || str == QStringLiteral("Private") ? DSE::PrivateInstance : DSE::UnknownInstanceType);
}

// legacy, remove
static DSE::ScriptDefaultType stringToDefaultType(QStringView str)
{
	return str.isEmpty() || str.at(0) == 'N' ? DSE::NoSavedDefault :
	                                           str.at(0) == 'F' ? DSE::FixedValueDefault :
	                                                              str.at(0) == 'C' ? DSE::CustomExprDefault :
	                                                                                 DSE::MainExprDefault;
}

static DSE::ScriptDefaultType stringToStateType(QStringView str, bool *createState, bool *singleShot)
{
	*createState = !str.isEmpty() && str.at(0) == 'Y';
	*singleShot = !*createState && str.indexOf('&') > -1;
	if (!*createState || str == QLatin1String("Yes"))
		return DSE::NoSavedDefault;
	int lbi = str.indexOf('\n') + 1;
	if (lbi < 4)
		return DSE::NoSavedDefault;
	return str.at(lbi) == 'A' ? DSE::MainExprDefault : str.at(lbi) == 'C' ? DSE::CustomExprDefault : DSE::FixedValueDefault;
}

// -----------------------------------
// Plugin
// -----------------------------------

Plugin::Plugin(const QString &tpHost, uint16_t tpPort, QObject *parent) :
  QObject(parent),
  client(new TPClientQt(PLUGIN_ID/*, this*/)),
  clientThread(new QThread())
{
	instance = this;

	qRegisterMetaType<JSError>("JSError");

	client->setHostProperties(tpHost, tpPort);

	connect(qApp, &QCoreApplication::aboutToQuit, this, &Plugin::quit);
	connect(this, &Plugin::loggerRotateLogs, Logger::instance(), &Logger::rotateLogs);
	connect(client, &TPClientQt::connected, this, &Plugin::onTpConnected);
	connect(client, &TPClientQt::disconnected, this, &Plugin::exit);
	connect(client, &TPClientQt::error, this, &Plugin::exit);
	connect(client, &TPClientQt::message, this, &Plugin::onTpMessage, Qt::QueuedConnection);
	connect(this, &Plugin::tpConnect, client, qOverload<>(&TPClientQt::connect), Qt::QueuedConnection);
	connect(this, &Plugin::tpDisconnect, client, &TPClientQt::disconnect, Qt::DirectConnection);
	connect(this, &Plugin::tpStateUpdate, client, qOverload<const QByteArray &, const QByteArray &>(&TPClientQt::stateUpdate), Qt::QueuedConnection);
	connect(this, &Plugin::tpStateCreate, client, qOverload<const QByteArray &, const QByteArray &, const QByteArray &, const QByteArray &>(&TPClientQt::createState), Qt::QueuedConnection);
	connect(this, &Plugin::tpStateRemove, client, qOverload<const QByteArray &>(&TPClientQt::removeState), Qt::QueuedConnection);
	connect(this, &Plugin::tpChoiceUpdate, client, qOverload<const QByteArray &, const QByteArrayList &>(&TPClientQt::choiceUpdate), Qt::QueuedConnection);
	connect(this, &Plugin::tpChoiceUpdateInstance, client, qOverload<const QByteArray &, const QByteArray &, const QByteArrayList &>(&TPClientQt::choiceUpdate), Qt::QueuedConnection);
	connect(this, &Plugin::tpConnectorUpdateShort, client, qOverload<const QByteArray&, uint8_t>(&TPClientQt::connectorUpdate), Qt::QueuedConnection);
	// These are just for scripting engine user functions, not used by plugin directly. Emitted by ScriptEngine.
	connect(this, &Plugin::tpChoiceUpdateStrList, client, qOverload<const QByteArray &, const QStringList &>(&TPClientQt::choiceUpdate), Qt::QueuedConnection);
	connect(this, &Plugin::tpConnectorUpdate, client, qOverload<const QByteArray&, uint8_t, bool>(&TPClientQt::connectorUpdate), Qt::QueuedConnection);
	connect(this, &Plugin::tpNotification, client, qOverload<const QByteArray&, const QByteArray&, const QByteArray&, const QVariantList&>(&TPClientQt::showNotification), Qt::QueuedConnection);

	client->moveToThread(clientThread);
	clientThread->start();

	m_loadSettingsTmr.setSingleShot(true);
	m_loadSettingsTmr.setInterval(750);
	connect(&m_loadSettingsTmr, &QTimer::timeout, this, &Plugin::loadSettings);

	Q_EMIT tpConnect();
	//QMetaObject::invokeMethod(this, "start", Qt::QueuedConnection);
}

Plugin::~Plugin()
{
	QWriteLocker il(DSE::instances_mutex());
	qDeleteAll(*DSE::instances());
	DSE::instances()->clear();
	il.unlock();

	QWriteLocker el(DSE::engines_mutex());
	qDeleteAll(*DSE::engines());
	DSE::engines()->clear();
	el.unlock();

	delete ScriptEngine::sharedInstance;
	ScriptEngine::sharedInstance = nullptr;

	if (clientThread) {
		clientThread->quit();
		clientThread->wait();
		clientThread->deleteLater();
		clientThread = nullptr;
	}
	delete client;
	client = nullptr;
	qCInfo(lcPlugin) << PLUGIN_SHORT_NAME " exiting.";
}

void Plugin::start()
{
	Q_EMIT tpConnect();
}

void Plugin::exit()
{
	qApp->exit(0);
}

void Plugin::quit()
{
	if (client) {
		if (client->thread() != qApp->thread())
			Utils::runOnThreadSync(client->thread(), [=]() { client->moveToThread(qApp->thread()); });
		if (client->isConnected()) {
			client->stateUpdate(PLUGIN_ID ".state.pluginState", "Stopped");
			client->stateUpdate(PLUGIN_ID ".state.createdStatesList", "");
			client->disconnect();
		}
	}
	saveSettings();
}

void Plugin::initEngine()
{
	new ScriptEngine(QByteArrayLiteral("Shared"));
	connect(ScriptEngine::instance(), &ScriptEngine::engineError, this, &Plugin::onEngineError, Qt::QueuedConnection);
	connect(DSE::sharedInstance, &DSE::defaultActionRepeatRateChanged, this, &Plugin::onActionRepeatRateChanged, Qt::QueuedConnection);
	connect(DSE::sharedInstance, &DSE::defaultActionRepeatDelayChanged, this, &Plugin::onActionRepeatDelayChanged, Qt::QueuedConnection);
}

void Plugin::saveSettings() const
{
	if (!g_startupComplete)
		return;
	int count = 0;
	QSettings s;
	s.beginGroup("DynamicStates");
	s.remove("");
	for (DynamicScript * const ds : DSE::instances_const()) {
		if (ds->defaultType() != DSE::NoSavedDefault) {
			s.setValue(ds->name, ds->serialize());
			++count;
		}
	}
	s.endGroup();
	qCInfo(lcPlugin) << "Saved" << count << "instance(s) to settings.";
}

void Plugin::savePluginSettings() const
{
	QSettings s;
	s.beginGroup("Plugin");
	s.setValue("actRepeatRate", DSE::defaultRepeatRate.load());
	s.setValue("actRepeatDelay", DSE::defaultRepeatDelay.load());
	s.endGroup();
}

void Plugin::loadSettings()
{
	int count = 0;
	QSettings s;

	s.beginGroup("Plugin");
	DSE::sharedInstance->setDefaultActionRepeatRate(s.value("actRepeatRate", 350).toInt());
	DSE::sharedInstance->setDefaultActionRepeatDelay(s.value("actRepeatDelay", 350).toInt());
	s.endGroup();

	s.beginGroup("DynamicStates");
	const QStringList &childs = s.childKeys();
	for (const QString &dvName : childs) {
		// FIXME: TP doesn't fire state change events based on the default value.
		// Best we can do now is send a blank default value and then the actual default (if it's not also blank).
		DynamicScript *ds = getOrCreateInstance(dvName.toUtf8());
		ds->deserialize(s.value(dvName).toByteArray());
		if (ds->engineName().isEmpty()) {
			qCCritical(lcPlugin) << "Engine name for script instance" << dvName << "is empty.";
			continue;
		}
		ds->setEngine(getOrCreateEngine(ds->engineName(), ds->instanceType() == DSE::PrivateInstance));
		++count;
		QMetaObject::invokeMethod(ds, "evaluateDefault", Qt::QueuedConnection);
	}
	s.endGroup();
	if (count)
		qCInfo(lcPlugin) << "Loaded" << count << "saved instance(s) from settings.";

	sendInstanceLists();
	g_startupComplete = true;
	Q_EMIT tpStateUpdate(QByteArrayLiteral(PLUGIN_ID ".state.pluginState"), QByteArrayLiteral("Started"));
}

ScriptEngine *Plugin::getOrCreateEngine(const QByteArray &name, bool privateType, bool failIfMissing)
{
	if (!privateType)
		return ScriptEngine::instance();
	ScriptEngine *se = DSE::engine(name);
	if (!se && !failIfMissing) {
		se = DSE::insert(name, new ScriptEngine(name));
		// Instance-specific errors from background tasks.
		connect(se, &ScriptEngine::engineError, this, &Plugin::onEngineError, Qt::QueuedConnection);
		sendEngineLists();
	}
	return se;
}

DynamicScript *Plugin::getOrCreateInstance(const QByteArray &name, bool failIfMissing)
{
	DynamicScript *ds = DSE::instance(name);
	if (!ds && !failIfMissing) {
		//qCDebug(lcPlugin) << dvName << "Creating";
		ds = DSE::insert(name, new DynamicScript(name));
		connect(ds, &DynamicScript::scriptError, Plugin::instance, &Plugin::onScriptError, Qt::QueuedConnection);
		connect(ds, &DynamicScript::dataReady, client, qOverload<const QByteArray&, const QByteArray&>(&TPClientQt::stateUpdate), Qt::QueuedConnection);
		sendInstanceLists();
	}
	return ds;
}

void Plugin::removeInstance(DynamicScript *ds, bool removeFromGlobal, bool removeUnusedEngine) const
{
	if (!ds)
		return;
	ScriptEngine *se = ds->engine();
	ScriptEngine::instance()->clearInstanceData(ds->name);
	ds->setCreateState(false);  // removes State if it was created
	if (removeFromGlobal) {
		DSE::removeInstance(ds->name);
		sendInstanceLists();
	}
	disconnect(ds, nullptr, this, nullptr);
	qCInfo(lcPlugin) << "Deleted Script instance" << ds->name;
	delete ds;

	// Check if any other instances are still using this engine.
	if (removeUnusedEngine && se && !se->isSharedInstance()) {
		for (DynamicScript *ds : DSE::instances_const())
			if (ds->engine() == se)
				return;
		// remove engine if no script instances are using it
		removeEngine(se, true, false);
	}
}

void Plugin::removeEngine(ScriptEngine *se, bool removeFromGlobal, bool removeScripts) const
{
	if (!se || se->isSharedInstance())
		return;

	bool scriptsRemoved = false;
	if (removeScripts) {
		QWriteLocker l(DSE::instances_mutex());
		DSE::ScriptState::iterator it = DSE::instances()->begin();
		while (it != DSE::instances()->end()) {
			if (it.value()->engine() == se) {
				removeInstance(it.value(), false, false);
				scriptsRemoved = true;
				it = DSE::instances()->erase(it);
				continue;
			}
			++it;
		}
	}

	disconnect(se, nullptr, this, nullptr);
	if (removeFromGlobal) {
		DSE::removeEngine(se->name());
		sendEngineLists();
	}
	if (removeFromGlobal || scriptsRemoved)
		sendInstanceLists();
	qCInfo(lcPlugin) << "Deleted Engine instance" << se->name();
	delete se;
}

void Plugin::sendInstanceLists() const
{
	QByteArrayList nameArry = DSE::instanceKeys();
	std::sort(nameArry.begin(), nameArry.end());
	Q_EMIT tpStateUpdate(QByteArrayLiteral(PLUGIN_ID ".state.createdStatesList"), nameArry.join(',') + ',');
	Q_EMIT tpChoiceUpdate(QByteArrayLiteral(PLUGIN_ID ".act.script.update.name"), nameArry);
	nameArry.prepend(tokenToName(AT_Default));
	Q_EMIT tpChoiceUpdate(QByteArrayLiteral(PLUGIN_ID ".act.plugin.repRate.name"), nameArry);
}

void Plugin::sendEngineLists() const
{
	QByteArrayList nameArry = DSE::engineKeys();
	std::sort(nameArry.begin(), nameArry.end());
	nameArry.prepend(tokenToName(AT_Private));
	nameArry.prepend(tokenToName(AT_Shared));
	Q_EMIT tpChoiceUpdate(QByteArrayLiteral(PLUGIN_ID ".act.script.d.scope"), nameArry);
	//Q_EMIT tpChoiceUpdate(QByteArrayLiteral(PLUGIN_ID ".act.script.eval.scope"), nameArry);
	//Q_EMIT tpChoiceUpdate(QByteArrayLiteral(PLUGIN_ID ".act.script.load.scope"), nameArry);
	//Q_EMIT tpChoiceUpdate(QByteArrayLiteral(PLUGIN_ID ".act.script.import.scope"), nameArry);
}

void Plugin::updateInstanceChoices(int token, const QByteArray &instId) const
{
	QByteArrayList nameArry = token == AT_Engine ? DSE::engineKeys() : DSE::instanceKeys();
	std::sort(nameArry.begin(), nameArry.end());
	if (nameArry.isEmpty()) {
		nameArry.append(QByteArrayLiteral("[ no instances created ]"));
	}
	else {
		if (token == CA_DelEngine) {
			nameArry.prepend(QByteArrayLiteral("All Private Engine Instances"));
		}
		else {
			int what = token == CA_DelEngine || token == CA_ResetEngine ? AT_Engine : AT_Script;
			const QByteArray &typeName = tokenToName(what);
			nameArry.prepend(QLatin1String("All Private %1 Instances").arg(typeName).toUtf8());
			nameArry.prepend(QLatin1String("All Shared %1 Instances").arg(typeName).toUtf8());
			nameArry.prepend(QByteArrayLiteral("All Instances"));
		}
	}
	if (instId.isEmpty())
		Q_EMIT tpChoiceUpdate(QByteArrayLiteral(PLUGIN_ID ".act.plugin.instance.name"), nameArry);
	else
		Q_EMIT tpChoiceUpdateInstance(QByteArrayLiteral(PLUGIN_ID ".act.plugin.instance.name"), instId, nameArry);
}

// Only used by deprecated CA_SetStateValue action.
void Plugin::sendScriptState(DynamicScript *ds, const QByteArray &value) const
{
	if (ds)
		ds->stateUpdate(value);
}

void Plugin::updateConnectors(const QMultiMap<QString, QVariant> &qry, int value, float rangeMin, float rangeMax) const
{
	const auto connectors = ConnectorData::instance()->records(qry);
	for (const auto &conn : connectors) {
		float rmin = (float)conn.otherData.value(QLatin1String("rangeMin")).toString("0").toDouble();
		float rmax = (float)conn.otherData.value(QLatin1String("rangeMax")).toString("0").toDouble();
		if (rmin == 0.0f || rmax == 0.0f)
			continue;
		int connVal = qRound(Utils::rangeValueToPercent(value, qBound(rangeMin, rmin, rangeMax), qBound(rangeMin, rmax, rangeMax)));
		Q_EMIT tpConnectorUpdateShort(conn.shortId, connVal);
	}
}

void Plugin::updateActionRepeatProperties(int ms, int param) const
{
	savePluginSettings();
	const QByteArray &paramName = tokenToName(param);
	Q_EMIT tpStateUpdate(QByteArrayLiteral(PLUGIN_ID ".state.actRepeat") + paramName, QByteArray::number(ms));
	updateConnectors({
		{"actionType", tokenToName(CA_RepeatRate)},
		{"instanceName", tokenToName(AT_Default)},
		{"otherData",  QStringLiteral("*\"param\":\"*%1*\"*").arg(paramName)},
		{"otherData",  QStringLiteral("*\"action\":\"%1\"*").arg(tokenToName(AT_Set))},
	}, ms, 50.0f, 60000.0f);
}

void Plugin::raiseScriptError(const QByteArray &dsName, const QString &msg, const QString &type, const QString &stack) const
{
	const uint32_t count = ++g_errorCount;
	Q_EMIT tpStateUpdate(QByteArrayLiteral(PLUGIN_ID ".state.errorCount"), QByteArray::number(count));
	QByteArray v;
	if (dsName.isEmpty()) {
		v = QStringLiteral("%1 [%2] %3").arg(count, 3, 10, QLatin1Char('0')).arg(QTime::currentTime().toString("HH:mm:ss.zzz"), msg).toUtf8();
		qCWarning(lcDse).noquote().nospace() << type << " [" << count << "] " << msg;
	}
	else {
		v = QStringLiteral("%1 [%2] %3 %4").arg(count, 3, 10, QLatin1Char('0')).arg(QTime::currentTime().toString("HH:mm:ss.zzz"), dsName, msg).toUtf8();
		qCWarning(lcDse).noquote().nospace() << type << " [" << count << "] for script instance '" << dsName << "': " << msg;
	}
	if (!stack.isEmpty())
		qCInfo(lcDse).noquote().nospace() << "Stack trace [" << count << "]:\n" << stack.toUtf8();
	Q_EMIT tpStateUpdate(QByteArrayLiteral(PLUGIN_ID ".state.lastError"), v);
}

void Plugin::clearScriptErrors()
{
	g_errorCount = 0;
	Q_EMIT tpStateUpdate(QByteArrayLiteral(PLUGIN_ID ".state.errorCount"), QByteArrayLiteral("0"));
}

void Plugin::onScriptError(const JSError &e) const
{
	//qDebug() << sender() << e.stack;
	if (DynamicScript *ds = qobject_cast<DynamicScript *>(sender()))
		raiseScriptError(ds->name, e.message, tr("SCRIPT EXCEPTION"), e.stack);
}

void Plugin::onEngineError(const JSError &e) const
{
	//qDebug() << e.instanceName << e.stack;
	raiseScriptError(e.instanceName.toUtf8(), e.toString(), tr("ENGINE EXCEPTION"), e.stack);
}

void Plugin::onDsFinished() const
{
	if (DynamicScript *ds = qobject_cast<DynamicScript *>(sender())) {
		if (ds->singleShot)
			removeInstance(ds);
	}
}

void Plugin::onStateUpdateByName(const QByteArray &name, const QByteArray &value) const
{
	//qCDebug(lcPlugin) << "Sending state update" << PLUGIN_STATE_ID_PREFIX + name;
	Q_EMIT tpStateUpdate(QByteArrayLiteral(PLUGIN_STATE_ID_PREFIX) + name, value);
}

void Plugin::onActionRepeatRateChanged(int ms) const { updateActionRepeatProperties(ms, AT_Rate); }
void Plugin::onActionRepeatDelayChanged(int ms) const { updateActionRepeatProperties(ms, AT_Delay); }

void Plugin::onTpConnected(const TPClientQt::TPInfo &info, const QJsonObject &settings)
{
	qCInfo(lcPlugin).nospace().noquote()
		<< PLUGIN_SHORT_NAME " Connected to Touch Portal v" << info.tpVersionString
		<< " (" << info.tpVersionCode << "; SDK v" << info.sdkVersion
		<< ") with entry.tp v" << info.pluginVersion << ", running v" << APP_VERSION_STR;
	DSE::tpVersion = info.tpVersionCode;
	DSE::tpVersionStr = info.tpVersionString;
	Q_EMIT tpStateUpdate(QByteArrayLiteral(PLUGIN_ID ".state.pluginState"), QByteArrayLiteral("Starting"));
	handleSettings(settings);
	Q_EMIT tpStateUpdate(QByteArrayLiteral(PLUGIN_ID ".state.tpDataPath"), Utils::tpDataPath());
	initEngine();
	clearScriptErrors();
	m_loadSettingsTmr.start();
}

void Plugin::onTpMessage(TPClientQt::MessageType type, const QJsonObject &msg)
{
	//qCDebug(lcPlugin) << msg;
	switch (type) {
		case TPClientQt::MessageType::action:
		case TPClientQt::MessageType::down:
		case TPClientQt::MessageType::up:
		case TPClientQt::MessageType::connectorChange:
			dispatchAction(type, msg);
			return;

		case TPClientQt::MessageType::listChange: {
			if (!msg.value("actionId").toString().endsWith(tokenToName(CA_Instance)))
				return;
			if (!msg.value("listId").toString().endsWith(QLatin1String(".action")))
				return;
			int token = tokenFromName(msg.value("value").toString().toUtf8());
			if (token != AT_Unknown) {
				updateInstanceChoices(token, msg.value("instanceId").toString().toUtf8());
			}
			return;
		}

		case TPClientQt::MessageType::broadcast: {
			QVariantMap data;
			const QString event = msg.value(QLatin1String("event")).toString();
			if (!event.compare(QLatin1String("pageChange"))) {
				const QByteArray pgName = msg.value(QLatin1String("pageName")).toString().toUtf8().sliced(1).replace(".tml", QByteArray()).replace('\\', '/');
				if (pgName.isEmpty())
					return;
				DSE::tpCurrentPage = pgName;
				data.insert(QLatin1String("pageName"), pgName);
				Q_EMIT tpStateUpdate(QByteArrayLiteral(PLUGIN_ID ".state.currentPage"), pgName);
			}
			Q_EMIT tpBroadcast(event, data);
			break;
		}

		case TPClientQt::MessageType::shortConnectorIdNotification:
			// delay initial loading of saved script instances until all notifications have been recieved
			if (m_loadSettingsTmr.isActive())
				m_loadSettingsTmr.start();
			parseConnectorNotification(msg);
			break;

		case TPClientQt::MessageType::settings:
			handleSettings(msg);
			break;

		case TPClientQt::MessageType::closePlugin:
			qCInfo(lcPlugin()) << "Got plugin close message from TP, exiting.";
			exit();
			return;

		case TPClientQt::MessageType::notificationOptionClicked:
			// passthrough to any scripts which may be listening on a callback.
			Q_EMIT tpNotificationClicked(msg.value(QLatin1String("notificationId")).toString(), msg.value(QLatin1String("optionId")).toString());
			return;

		default:
			return;
	}
}

void Plugin::dispatchAction(TPClientQt::MessageType type, const QJsonObject &msg)
{
	const QString actId = msg.value(type == TPClientQt::MessageType::connectorChange ? QLatin1String("connectorId") : QLatin1String("actionId")).toString();
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	const QVector<QStringRef> actIdArry = actId.splitRef('.');
#else
	const QVector<QStringView> actIdArry = QStringView(actId).split('.');
#endif
	if (actIdArry.length() < 8) {
		qCCritical(lcPlugin) << "Action ID is malformed for action:" << actId;
		return;
	}
	const int handler = tokenFromName(actIdArry.at(6).toUtf8());
	if (handler == AT_Unknown) {
		qCCritical(lcPlugin) << "Unknown action handler for this plugin:" << actId;
		return;
	}

	const QByteArray action(actIdArry.at(7).toUtf8());
	const int act = tokenFromName(action);
	if (act == AT_Unknown) {
		qCCritical(lcPlugin) << "Unknown action for this plugin:" << action;
		return;
	}

	const QJsonArray data = msg.value(QLatin1String("data")).toArray();
	if (!data.size()) {
		qCCritical(lcPlugin) << "Action data missing for action:" << actId;
		return;  // we have no actions w/out data members
	}

	const QMap<QString, QString> dataMap = TPClientQt::actionDataToMap(data, '.');
	qint32 connVal = type == TPClientQt::MessageType::connectorChange ? msg.value(QLatin1String("value")).toInt(0) : -1;

	switch(handler) {
		case AH_Script:
			scriptAction(type, act, dataMap, connVal);
			break;

		case AH_Plugin:
			pluginAction(type, act, dataMap, connVal);
			break;

		default:
			return;
	}
}

void Plugin::scriptAction(TPClientQt::MessageType type, int act, const QMap<QString, QString> &dataMap, qint32 connectorValue)
{
	if (act == SA_SingleShot)  {
		qCCritical(lcPlugin) << "Anyonymous script instances are no longer supported. Please use another type with 'Create State' set to 'No & Delete'.";
		return;
	}

	const QByteArray dvName = dataMap.value("name").trimmed().toUtf8();
	if (dvName.isEmpty()) {
		qCCritical(lcPlugin) << "Script Instance Name missing for action" << tokenToName(act);
		return;
	}

	DynamicScript *ds = getOrCreateInstance(dvName, act == SA_Update);
	if (!ds) {
		raiseScriptError(dvName, tr("ValidationError: Could not find script instance '%1' for Update action.").arg(dvName.constData()), tr("VALIDATION ERROR"));
		return;
	}

	ds->setRepeating(false);
	if (type == TPClientQt::MessageType::up)
		return;

	if (act != SA_Update) {
		QByteArray engName;
		const QString &strScope = dataMap.value("scope", QStringLiteral("Shared"));
		DSE::EngineInstanceType scope = stringToScope(strScope);
		// If a scope/instance type returns Unknown this means it should be a specific named engine instance.
		if (scope == DSE::UnknownInstanceType) {
			// Make sure it's not an empty name.
			if (strScope.isEmpty()) {
				raiseScriptError(dvName, tr("ValidationError: Engine name/type is empty for script instance '%1'.").arg(dvName.constData()), tr("VALIDATION ERROR"));
				return;
			}
			engName = strScope.toUtf8();
			scope = DSE::PrivateInstance;
		}
		// Otherwise if the scope is explicitly "Private" then use the script instance name as the engine name.
		else if (scope == DSE::PrivateInstance) {
			engName = dvName;
		}
		ds->setEngine(scope == DSE::PrivateInstance ? getOrCreateEngine(engName) : ScriptEngine::instance());

		bool createState = true;
		bool singleShot = false;
		const QString &stateParam = dataMap.value("state");
		// Connectors do not have a default type parameter, only create state yes/no; Do not modify any previosly-set defaults.
		if (type == TPClientQt::MessageType::connectorChange) {
			if (!stateParam.isEmpty())
				stringToStateType(stateParam, &createState, &singleShot);
		}
		else {
			DSE::ScriptDefaultType defType;
			// Preserve BC with v < 1.1.0.2
			if (stateParam.isEmpty())
				defType = stringToDefaultType(dataMap.value("save", QStringLiteral("No")));
			else
				defType = stringToStateType(stateParam, &createState, &singleShot);
			ds->setDefaultTypeValue(defType, dataMap.value("default").toUtf8());
		}
		ds->setSingleShot(singleShot);
		ds->setCreateState(createState);
	}

	QString expression = dataMap.value("expr");
	if (connectorValue > -1) {
		expression.replace(QLatin1String("${connector_value}"), QString::number(connectorValue), Qt::CaseInsensitive);
	}
	bool ok;
	switch (act)
	{
		case SA_Eval:
			ok = ds->setExpressionProperties(expression);
			break;
		case SA_Load:
			ok = ds->setScriptProperties(dataMap.value("file").trimmed(), expression);
			break;
		case SA_Import:
			ok = ds->setModuleProperties(dataMap.value("file").trimmed(), dataMap.value("alias").trimmed(), expression);
			break;
		case SA_Update:
			ok = ds->setExpression(expression);
			break;
	}
	if (!ok) {
		raiseScriptError(ds->name, tr("ValidationError: %1").arg(ds->lastError), tr("VALIDATION ERROR"));
		return;
	}
	if (type == TPClientQt::MessageType::down)
		ds->setRepeating(true);
	QMetaObject::invokeMethod(ds, "evaluate", Qt::QueuedConnection);
}

void Plugin::pluginAction(TPClientQt::MessageType type, int act, const QMap<QString, QString> &dataMap, qint32 connectorValue)
{
	const int subAct = tokenFromName(dataMap.value("action").toUtf8());
	if (subAct == AT_Unknown) {
		qCCritical(lcPlugin) << "Unknown Command action:" << dataMap.value("action");
		return;
	}

	switch (act) {
		case CA_Instance:
			instanceControlAction(subAct, dataMap);
			break;
		case CA_RepeatRate:
			setActionRepeatRate(type, subAct, dataMap, connectorValue);
			break;
	}
}

void Plugin::instanceControlAction(quint8 act, const QMap<QString, QString> &dataMap)
{
	quint8 type = 0;  // named instance
	QByteArray dvName = dataMap.value("name", "All").toUtf8();
	if (dvName.startsWith("All "))
		type = (dvName.at(4) == 'I' ? 255 : dvName.at(4) == 'S' ? (quint8)DSE::SharedInstance : dvName.at(4) == 'P' ? (quint8)DSE::PrivateInstance : 0);

	switch (act)
	{
		case CA_DelScript: {
			if (type) {
				QWriteLocker l(DSE::instances_mutex());
				DSE::ScriptState::iterator it = DSE::instances()->begin();
				while (it != DSE::instances()->end()) {
					if (type == 255 || type == (quint8)it.value()->instanceType()) {
						removeInstance(it.value(), false, true);
						it = DSE::instances()->erase(it);
					}
					else {
						++it;
					}
				}
				sendInstanceLists();
			}
			else if (DynamicScript *ds = DSE::instance(dvName)) {
				removeInstance(ds);
			}
			else {
				qCCritical(lcPlugin) << "Script instance not found for name:" << dvName;
				sendInstanceLists();  // update the states list anyway in case there's a stale name in there
			}
			return;
		}

		case CA_DelEngine: {
			if (type == (quint8)DSE::SharedInstance){
				qCCritical(lcPlugin) << "Cannot delete the shared engine instance.";
				return;
			}
			if (type) {
				QWriteLocker l(DSE::engines_mutex());
				DSE::EngineState::iterator it = DSE::engines()->begin();
				while (it != DSE::engines()->end()) {
					if ((type == 255 || type == (quint8)DSE::PrivateInstance) && !it.value()->isSharedInstance()) {
						removeEngine(it.value(), false);
						it = DSE::engines()->erase(it);
					}
					else {
						++it;
					}
				}
				sendEngineLists();
				//sendStateLists();
			}
			else if (ScriptEngine *se = DSE::engine(dvName)) {
				removeEngine(se);
			}
			else {
				qCCritical(lcPlugin) << "Engine instance not found for name:" << dvName;
				sendInstanceLists();  // update the states list anyway in case there's a stale name in there
			}
			return;
		}

		case CA_ResetEngine: {
			if (!type) {
				if (ScriptEngine *se = DSE::engine(dvName))
					se->reset();
				else
					qCCritical(lcPlugin) << "Engine instance not found for name:" << dvName;
				return;
			}
			if (type == 255 || type == (quint8)DSE::PrivateInstance) {
				for (ScriptEngine * const se : DSE::engines_const()) {
					if (!se->isSharedInstance())
						se->reset();
				}
			}
			if (type == 255 || type == (quint8)DSE::SharedInstance)
				ScriptEngine::instance()->reset();
			return;
		}

		// Deprecated in v1.1; remove.
		case CA_SetStateValue: {
			const QByteArray stateValue = dataMap.value("value").toUtf8();
			if (type) {
				for (DynamicScript * const ds : DSE::instances_const()) {
					if (type == 255 || type == (quint8)ds->instanceType())
						sendScriptState(ds, stateValue);
				}
			}
			else if (DynamicScript *ds = DSE::instance(dvName)) {
				sendScriptState(ds, stateValue);
			}
			else {
				qCCritical(lcPlugin) << "Script instance not found for name:" << dvName;
			}
			return;
		}
	}
}

void Plugin::setActionRepeatRate(TPClientQt::MessageType type, quint8 act, const QMap<QString, QString> &dataMap, qint32 connectorValue) const
{
	int param = tokenFromName(dataMap.value("param").toUtf8());
	const QByteArray instName = dataMap.value("name").toUtf8();
	if ((param != AT_Rate && param != AT_Delay && param != AT_RateDelay) || instName.isEmpty()) {
		qCCritical(lcPlugin) << "Invalid properties in action" << tokenToName(act) << "Repeat" << dataMap.value("param") << "for" << instName;
		return;
	}

	bool globalInst = (instName == tokenToName(AT_Default));
	DSE *dse = nullptr;
	if (globalInst) {
		dse = DSE::sharedInstance;
	}
	else {
		if (DynamicScript *ds = DSE::instance(instName))
			dse = ds->engine() ? ds->engine()->dseObject() : nullptr;
		if (!dse) {
			qCCritical(lcPlugin) << "Instance name" << instName << "not found or is invalid for action" << tokenToName(act) << "Repeat" << dataMap.value("param");
			return;
		}
	}

	dse->cancelRepeatingAction(/*DSE::ACT_ADJ_REPEAT*/);
	if (type == TPClientQt::MessageType::up)
		return;

	int value;
	bool ok;
	if (type == TPClientQt::MessageType::connectorChange) {
		float connValue = Utils::connectorValueToRange(connectorValue, 50.0f, 60000.0f, dataMap, &ok);
		if (!ok) {
			qCCritical(lcPlugin) << "Invalid slider range value(s) for connector Set" << dataMap.value("param") << "for" << dataMap.value("name");
			return;
		}
		value = qRound(connValue);
	}
	else {
		value = dataMap.value("value", "0").toInt(&ok);
	}

	if (!ok || value < 1) {
		qCCritical(lcPlugin) << "Value" << dataMap.value("value") << "is invalid in action" << tokenToName(act) << "Repeat" << dataMap.value("param") << "for" << instName;
		return;
	}

	quint8 prop = param == AT_Rate ? DSE::RepeatRateProperty : (param == AT_Delay ? DSE::RepeatDelayProperty : DSE::AllRepeatProperties);
	quint8 repAct = act == AT_Increment ? DSE::Increment : (act == AT_Decrement ? DSE::Decrement : DSE::SetAbsolute);
	QMetaObject::invokeMethod(
		dse,
		"setActionRepeat",
		Qt::QueuedConnection,
		Q_ARG(quint8, prop),
		Q_ARG(quint8, repAct),
		Q_ARG(int, value),
		Q_ARG(const QByteArray &, (globalInst ? QByteArray() : instName)),
		Q_ARG(bool, type == TPClientQt::MessageType::down)
	);
}

void Plugin::handleSettings(const QJsonObject &settings) const
{
	//qCDebug(lcPlugin) << "Got settings object:" << settings;
	QJsonObject::const_iterator next = settings.begin(), last = settings.end();
	for (; next != last; ++next) {
		if (next.key().startsWith(QStringLiteral("Script Files"))) {
			DSE::scriptsBaseDir = QDir::fromNativeSeparators(next.value().toString());
			if (!DSE::scriptsBaseDir.isEmpty() && !DSE::scriptsBaseDir.endsWith('/'))
				DSE::scriptsBaseDir += '/';
		}
	}
}

void Plugin::parseConnectorNotification(const QJsonObject &msg) const
{
	//qCDebug(lcPlugin) << msg;
	const QString longConnId(msg.value(QLatin1String("connectorId")).toString());
	const QList<QStringView> propList = QStringView(longConnId).split('|');
	if (propList.size() < 2)
		return;

	QByteArray actIdStr = propList.at(0).split('.').last().toUtf8();
	const int act = tokenFromName(actIdStr);
	if (act != AT_Unknown)
		actIdStr = tokenToName(act, actIdStr);

	ConnectorRecord cr;
	cr.actionType = actIdStr;
	cr.connectorId = propList.at(0).split('_').last().toUtf8();
	cr.shortId = msg.value(QLatin1String("shortId")).toString().toUtf8();

	QList<QStringView>::const_iterator it = propList.constBegin() + 1;
	for (; it != propList.constEnd(); ++it) {
		const auto dataPair = it->split('=');
		const QByteArray id = dataPair.first().split('.').last().toUtf8();
		const QStringView value = dataPair.last();
		if (cr.instanceName.isEmpty() && !id.compare(QByteArrayLiteral("name")))
			cr.instanceName = value.toUtf8();
		else if (cr.instanceType == DSE::UnknownInstanceType && !id.compare(QByteArrayLiteral("scope")))
			cr.instanceType = stringToScope(value, true);
		else if (cr.expression.isEmpty() && !id.compare(QByteArrayLiteral("expr")))
			cr.expression = value.toUtf8();
		else if (cr.file.isEmpty() && !id.compare(QByteArrayLiteral("file")))
			cr.file = value.toUtf8();
		else if (cr.alias.isEmpty() && !id.compare(QByteArrayLiteral("alias")))
			cr.alias = value.toUtf8();
		else
			cr.otherData.insert(id, value.toString());
	}

	switch (act) {
		case SA_Eval:
			cr.inputType = DSE::ExpressionInput;
			break;
		case SA_Load:
			cr.inputType = DSE::ScriptInput;
			break;
		case SA_Import:
			cr.inputType = DSE::ModuleInput;
			break;
		case SA_Update:
			if (DynamicScript *ds = DSE::instance(cr.instanceName))
				cr.inputType = ds->inputType();
			break;
	}

	ConnectorData::instance()->insert(cr);
}

#include "moc_Plugin.cpp"
