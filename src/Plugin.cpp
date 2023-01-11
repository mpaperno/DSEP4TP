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
//#include <QThread>

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
	SA_SingleShot,

	CA_Instance,
	CA_DelInstance,
	CA_SetStateValue,
	CA_ResetEngine,
};

Plugin *Plugin::instance = nullptr;

static std::atomic_uint32_t g_errorCount = 0;
static std::atomic_uint32_t g_singleShotCount = 0;
static bool g_startupComplete = false;

using TokenMapHash = QHash<QByteArray, int>;
static const TokenMapHash &tokenMap()
{
	static const TokenMapHash hash = {
		{ "script",  AH_Script },
	  { "plugin",  AH_Plugin },

		{ "eval",    SA_Eval },
	  { "load",    SA_Load },
	  { "import",  SA_Import },
	  { "update",  SA_Update },
	  { "oneshot", SA_SingleShot },

	  { "instance", CA_Instance },
	  { "Delete Instance", CA_DelInstance },
	  { "Set State Value", CA_SetStateValue },
	  { "Reset Engine Environment", CA_ResetEngine },
	};
	return hash;
}

using EnumNameHash = QHash<int, QByteArray>;
static const EnumNameHash &tokenToName()
{
	static const EnumNameHash hash = {
		{ SA_Eval,       "Eval" },
		{ SA_Load,       "Load" },
		{ SA_Import,     "Import" },
		{ SA_Update,     "Update" },
		{ SA_SingleShot, "OneTime" },
	};
	return hash;
}

static DSE::EngineInstanceType stringToScope(QStringView str)
{
	return str.at(0) == 'S' ? DSE::EngineInstanceType::Shared : DSE::EngineInstanceType::Private;
}

static DSE::ScriptInputType stringToInputType(QStringView ityp)
{
	return ityp.isEmpty() || ityp.at(0) == 'E' ? DSE::ScriptInputType::Expression : ityp.at(0) == 'S' ? DSE::ScriptInputType::Script : DSE::ScriptInputType::Module;
}

static DSE::ScriptDefaultType stringToDefaultType(QStringView str)
{
	return str.isEmpty() || str.at(0) == 'N' ? DSE::ScriptDefaultType::NoDefault :
	                                           str.at(0) == 'F' ? DSE::ScriptDefaultType::FixedValue :
	                                                              str.at(0) == 'C' ? DSE::ScriptDefaultType::CustomExpression :
	                                                                                 DSE::ScriptDefaultType::MainExpression;
}

// -----------------------------------
// Plugin
// -----------------------------------

Plugin::Plugin(const QString &tpHost, uint16_t tpPort, QObject *parent) :
  QObject(parent),
  client(new TPClientQt(PLUGIN_ID, this))
{
	instance = this;
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
	// These are just for scripting engine user functions, not used by plugin directly. Emitted by ScriptEngine.
	connect(this, &Plugin::tpChoiceUpdateStrList, client, qOverload<const QByteArray &, const QStringList &>(&TPClientQt::choiceUpdate), Qt::QueuedConnection);
	connect(this, &Plugin::tpConnectorUpdate, client, qOverload<const QByteArray&, uint8_t, bool>(&TPClientQt::connectorUpdate), Qt::QueuedConnection);
	connect(this, &Plugin::tpConnectorUpdateShort, client, qOverload<const QByteArray&, uint8_t>(&TPClientQt::connectorUpdate), Qt::QueuedConnection);
	connect(this, &Plugin::tpNotification, client, qOverload<const QByteArray&, const QByteArray&, const QByteArray&, const QVariantList&>(&TPClientQt::showNotification), Qt::QueuedConnection);

	m_loadSettingsTmr.setSingleShot(true);
	m_loadSettingsTmr.setInterval(750);
	connect(&m_loadSettingsTmr, &QTimer::timeout, this, &Plugin::loadSettings);

	Q_EMIT tpConnect();
	//QMetaObject::invokeMethod(this, "start", Qt::QueuedConnection);
}

Plugin::~Plugin()
{
	for (DynamicScript *ds : DSE::instances_const()) {
		delete ds;
	}

	if (client) {
		client->disconnect();
		delete client;
		client = nullptr;
	}
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
	if (client && client->isConnected())
		clearStateLists();
	Q_EMIT tpDisconnect();
	saveSettings();
}

void Plugin::initEngine()
{
	connect(ScriptEngine::instance(), &ScriptEngine::raiseError, this, &Plugin::onScriptEngineError, Qt::QueuedConnection);
}

void Plugin::saveSettings()
{
	if (!g_startupComplete)
		return;
	int count = 0;
	QSettings s;
	s.beginGroup("DynamicStates");
	s.remove("");
	for (DynamicScript * const ds : DSE::instances_const()) {
		if (ds->defaultType != DSE::ScriptDefaultType::NoDefault) {
			s.setValue(ds->name, ds->serialize());
			++count;
		}
	}
	s.endGroup();
	qCInfo(lcPlugin) << "Saved" << count << "instance(s) to settings.";
}

void Plugin::loadSettings()
{
	int count = 0;
	QSettings s;
	s.beginGroup("DynamicStates");
	const QStringList &childs = s.childKeys();
	for (const QString &dvName : childs) {
		// FIXME: TP doesn't fire state change events based on the default value.
		// Best we can do now is send a blank default value and then the actual default (if it's not also blank).
		DynamicScript *ds = getOrCreateInstance(dvName.toUtf8(), false);
		ds->deserialize(s.value(dvName).toByteArray());
		++count;
		switch (ds->defaultType) {
			case DSE::ScriptDefaultType::FixedValue:
				if (!ds->defaultValue.isEmpty())
					sendScriptState(ds, ds->defaultValue);
				break;
			default:
				QMetaObject::invokeMethod(ds, "evaluateDefault", Qt::QueuedConnection);
				break;
		}
	}
	s.endGroup();
	if (count)
		qCInfo(lcPlugin) << "Loaded" << count << "saved instance(s) from settings.";
	sendStateLists();
	g_startupComplete = true;
}

DynamicScript *Plugin::getOrCreateInstance(const QByteArray &name, bool deferState, bool failIfMissing)
{
	DynamicScript *ds = DSE::instance(name);
	if (!ds && !failIfMissing) {
		//qCDebug(lcPlugin) << dvName << "Creating";
		ds = DSE::instances()->insert(name, new DynamicScript(name)).value();
		if (!deferState)
			createScriptState(ds);
	}
	return ds;
}

void Plugin::removeInstance(DynamicScript *ds)
{
	if (ds) {
		if (ds->instanceType() == DSE::EngineInstanceType::Shared)
			ScriptEngine::instance()->clearInstanceData(ds->name);
		DSE::instances()->remove(ds->name);
		removeScriptState(ds);
		delete ds;
	}
}

void Plugin::sendStateLists() const
{
	const QByteArrayList &nameArry = DSE::instanceKeys();
  Q_EMIT tpStateUpdate(QByteArrayLiteral(PLUGIN_ID ".state.createdStatesList"), nameArry.join(','));
	if (nameArry.size())
		Q_EMIT tpChoiceUpdate(QByteArrayLiteral(PLUGIN_ID ".act.plugin.instance.name"),
	                        QByteArrayList({ "All Instances", "All Shared Engine Instances", "All Private Engine Instances" }) + nameArry);
	else
		Q_EMIT tpChoiceUpdate(QByteArrayLiteral(PLUGIN_ID ".act.plugin.instance.name"), { QByteArrayLiteral("[ no instances created ]") });
}

void Plugin::clearStateLists() const
{
	// This happens on exit, so use a direct call here.
	//	Q_EMIT tpStateUpdate(QByteArrayLiteral(PLUGIN_ID ".state.createdStatesList"), QByteArrayLiteral(""));
	client->stateUpdate(PLUGIN_ID ".state.createdStatesList", "");
}

void Plugin::sendScriptState(DynamicScript *ds, const QByteArray &value) const
{
	Q_EMIT tpStateUpdate(ds->tpStateName, value);
}

void Plugin::createScriptState(DynamicScript *ds) const
{
	if (!ds->stateCreated) {
		ds->stateCreated = true;
		// FIXME: TP doesn't fire state change events based on the default value.
		Q_EMIT tpStateCreate(ds->tpStateName, QByteArrayLiteral("Dynamic Values"), ds->name, QByteArray() /*ds->defaultValue*/);
		sendStateLists();
	}
}

void Plugin::removeScriptState(DynamicScript *ds, bool delayListUpdate) const
{
	if (ds->stateCreated) {
		Q_EMIT tpStateRemove(ds->tpStateName);
		if (!delayListUpdate)
			sendStateLists();
		qCDebug(lcPlugin) << "Removed instance State" << ds->name;
		ds->stateCreated = false;
	}
}

void Plugin::raiseScriptError(const QByteArray &dsName, const QString &msg, const QString &type) const
{
	++g_errorCount;
	Q_EMIT tpStateUpdate(QByteArrayLiteral(PLUGIN_ID ".state.errorCount"), QByteArray::number(g_errorCount));
	QByteArray v;
	if (dsName.isEmpty()) {
		v = QStringLiteral("%1 [%2] %3").arg(g_errorCount, 3, 10, QLatin1Char('0')).arg(QTime::currentTime().toString("HH:mm:ss.zzz"), msg).toUtf8();
		qCWarning(lcDse).noquote().nospace() << type << " [" << g_errorCount << "] " << msg;
	}
	else{
		v = QStringLiteral("%1 [%2] %3 %4").arg(g_errorCount, 3, 10, QLatin1Char('0')).arg(QTime::currentTime().toString("HH:mm:ss.zzz"), dsName, msg).toUtf8();
		qCWarning(lcDse).noquote().nospace() << type << " [" << g_errorCount << "] for instance '" << dsName << "': " << msg;
	}
	Q_EMIT tpStateUpdate(QByteArrayLiteral(PLUGIN_ID ".state.lastError"), v);
}

void Plugin::clearScriptErrors()
{
	g_errorCount = 0;
	Q_EMIT tpStateUpdate(QByteArrayLiteral(PLUGIN_ID ".state.errorCount"), QByteArray::number(g_errorCount));
}

void Plugin::onStateUpdateByName(const QByteArray &name, const QByteArray &value) const
{
	//qCDebug(lcPlugin) << "Sending state update" << PLUGIN_STATE_ID_PREFIX + name;
	Q_EMIT tpStateUpdate(QByteArrayLiteral(PLUGIN_STATE_ID_PREFIX) + name, value);
}

void Plugin::onDsScriptError(const QJSValue &e) const
{
	if (DynamicScript *ds = qobject_cast<DynamicScript *>(sender()))
		raiseScriptError(ds->name, e.property(QStringLiteral("message")).toString(), tr("SCRIPT EXCEPTION"));
}

void Plugin::onScriptEngineError(const QJSValue &e) const
{
	raiseScriptError(e.property(QStringLiteral("instanceName")).toString().toUtf8(), e.property(QStringLiteral("message")).toString(), tr("SCRIPT EXCEPTION"));
}

void Plugin::onDsFinished()
{
	if (DynamicScript *ds = qobject_cast<DynamicScript *>(sender())) {
		if (ds->singleShot)
			removeInstance(ds);
	}
}

void Plugin::onTpConnected(const TPClientQt::TPInfo &info, const QJsonObject &settings)
{
	qCInfo(lcPlugin).nospace().noquote()
		<< PLUGIN_SHORT_NAME " Connected to Touch Portal v" << info.tpVersionString
		<< " (" << info.tpVersionCode << "; SDK v" << info.sdkVersion
		<< ") with entry.tp v" << info.pluginVersion << ", running v" << APP_VERSION_STR;
	DSE::tpVersion = info.tpVersionCode;
	DSE::tpVersionStr = info.tpVersionString;
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
		case TPClientQt::MessageType::connectorChange:
			dispatchAction(type, msg);
			return;

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
	const QString actId = msg.value(type == TPClientQt::MessageType::action ? QLatin1String("actionId") : QLatin1String("connectorId")).toString();
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	const QVector<QStringRef> actIdArry = actId.splitRef('.');
#else
	const QVector<QStringView> actIdArry = QStringView(actId).split('.');
#endif
	if (actIdArry.length() < 8) {
		qCWarning(lcPlugin) << "Action ID is malformed for action:" << actId;
		return;
	}
	const int handler = tokenMap().value(actIdArry.at(6).toUtf8(), AT_Unknown);
	if (handler == AT_Unknown) {
		qCWarning(lcPlugin) << "Unknown action handler for this plugin:" << actId;
		return;
	}

	const QByteArray action(actIdArry.at(7).toUtf8());
	const int act = tokenMap().value(action, AT_Unknown);
	if (act == AT_Unknown) {
		qCWarning(lcPlugin) << "Unknown action for this plugin:" << action;
		return;
	}

	const QJsonArray data = msg.value(QLatin1String("data")).toArray();
	if (!data.size()) {
		qCWarning(lcPlugin) << "Action data missing for action:" << actId;
		return;  // we have no actions w/out data members
	}

	const QMap<QString, QString> dataMap = TPClientQt::actionDataToMap(data, '.');
	qint32 connVal = type == TPClientQt::MessageType::connectorChange ? msg.value(QLatin1String("value")).toInt(0) : -1;

	switch(handler) {
		case AH_Script:
			scriptAction(type, act, dataMap, connVal);
			break;

		case AH_Plugin:
			pluginAction(type, act, dataMap);
			break;

		default:
			return;
	}
}

void Plugin::scriptAction(TPClientQt::MessageType /*type*/, int act, const QMap<QString, QString> &dataMap, qint32 connectorValue)
{
	QByteArray dvName;
	if (act == SA_SingleShot) {
		dvName = "ANON_" + QByteArray::number(++g_singleShotCount);
	}
	else {
		dvName = dataMap.value("name").trimmed().toUtf8();
		if (dvName.isEmpty()) {
			qCWarning(lcPlugin) << "Script state name missing for action" << tokenToName().value(act);
			return;
		}
	}

	DynamicScript *ds = getOrCreateInstance(dvName, act == SA_SingleShot, act == SA_Update);
	if (!ds) {
		raiseScriptError(dvName, tr("ValidationError: Instance not found for state name %1").arg(dvName.constData()), tr("VALIDATION ERROR"));
		return;
	}
	DSE::EngineInstanceType scope = stringToScope(dataMap.value("scope", QStringLiteral("Shared")));
	const QString &dtyp = act == SA_SingleShot ? QString() : dataMap.value("save", QStringLiteral("No"));
	DSE::ScriptDefaultType defType = stringToDefaultType(dtyp);
	const QByteArray defVal = defType != DSE::ScriptDefaultType::NoDefault ? dataMap.value("default").toUtf8() : QByteArray();
	QString expression = dataMap.value("expr");
	if (connectorValue > -1) {
		expression.replace(QLatin1String("${connector_value}"), QString::number(connectorValue), Qt::CaseInsensitive);
	}
	bool ok = false;
	switch (act)
	{
		case SA_Eval:
			ok = ds->setExpressionProperties(scope, expression, defType, defVal);
			break;

		case SA_Load:
			ok = ds->setScriptProperties(scope, dataMap.value("file").trimmed(), expression, defType, defVal);
			break;

		case SA_Import:
			ok = ds->setModuleProperties(scope, dataMap.value("file").trimmed(), dataMap.value("alias").trimmed(), expression, defType, defVal);
			break;

		case SA_Update:
			ok = ds->setExpression(expression);
			break;

		case SA_SingleShot: {
			ds->setSingleShot();
			const QString &ityp = dataMap.value("type", QStringLiteral("Expression"));
			ok = ds->setProperties(stringToInputType(ityp), scope, expression, dataMap.value("file").trimmed(), dataMap.value("alias").trimmed(), defType);
			break;
		}
	}
	if (!ok) {
		raiseScriptError(ds->name, QStringLiteral("ValidationError:") + ds->lastError, tr("VALIDATION ERROR"));
		if (ds->singleShot)
			removeInstance(ds);
		return;
	}
	QMetaObject::invokeMethod(ds, "evaluate", Qt::QueuedConnection);
}

void Plugin::pluginAction(TPClientQt::MessageType /*type*/, int act, const QMap<QString, QString> &dataMap)
{
	const int subAct = tokenMap().value(dataMap.value("action").toUtf8(), AT_Unknown);
	if (subAct == AT_Unknown) {
		qCWarning(lcPlugin) << "Unknown Command action:" << dataMap.value("action");
		return;
	}

	switch (act) {
		case CA_Instance:
			instanceControlAction(subAct, dataMap);
			break;
	}
}

void Plugin::instanceControlAction(quint8 act, const QMap<QString, QString> &dataMap)
{
	quint8 type = 0;  // named instance
	QByteArray dvName = dataMap.value("name", "All").toUtf8();
	if (dvName.startsWith("All "))
		type = (dvName.at(4) == 'I' ? 255 : dvName.at(4) == 'S' ? (quint8)DSE::EngineInstanceType::Shared : dvName.at(4) == 'P' ? (quint8)DSE::EngineInstanceType::Private : 0);
	//qCDebug(lcPlugin) << "Command" << TPClientQt::getIndexedActionDataValue(0, data) << act << dvName;
	switch (act)
	{
		case CA_DelInstance: {
			if (type) {
				QMutableHashIterator<QByteArray, DynamicScript *> it(*DSE::instances());
				while (it.hasNext()) {
					it.next();
					if (type == 255 || type == (quint8)it.value()->instanceType()) {
						removeScriptState(it.value(), true);
						delete it.value(); //->deleteLater();
						it.remove();
					}
				}
				if (type == 255 || type == (quint8)DSE::EngineInstanceType::Shared){
					ScriptEngine::instance()->reset();
					qCInfo(lcPlugin) << "Shared Scripting Engine reset completed.";
				}
				sendStateLists();
			}
			else if (DynamicScript *ds = DSE::instance(dvName)) {
				removeInstance(ds);
			}
			else {
				qCWarning(lcPlugin) << "Script not found for name:" << dvName;
				sendStateLists();  // update the states list anyway in case there's a stale name in there
				return;
			}
			return;
		}

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
				qCWarning(lcPlugin) << "Script not found for name:" << dvName;
				return;
			}
			return;
		}

		case CA_ResetEngine: {
			if (!type) {
				if (DynamicScript *ds = DSE::instance(dvName))
					ds->resetEngine();
				else
					qCWarning(lcPlugin) << "Script not found for name:" << dvName;
				return;
			}
			if (type == 255 || type == (quint8)DSE::EngineInstanceType::Private) {
				for (DynamicScript * const ds : DSE::instances_const()) {
					if (ds->instanceType() == DSE::EngineInstanceType::Private)
						ds->resetEngine();
				}
			}
			if (type == 255 || type == (quint8)DSE::EngineInstanceType::Shared) {
				ScriptEngine::instance()->reset();
				qCInfo(lcPlugin) << "Shared Scripting Engine reset completed.";
			}
			return;
		}
	}
}

			return;
	}
}


void Plugin::handleSettings(const QJsonObject &settings)
{
	//qCDebug(lcPlugin) << "Got settings object:" << settings;
	QJsonObject::const_iterator next = settings.begin(), last = settings.end();
	for (; next != last; ++next) {
		if (next.key().startsWith(QStringLiteral("Script Files"))) {
			DSE::scriptsBaseDir = QDir::fromNativeSeparators(next.value().toString());
			if (!DSE::scriptsBaseDir.endsWith('/'))
				DSE::scriptsBaseDir += '/';
		}
	}
}

void Plugin::parseConnectorNotification(const QJsonObject &msg)
{
	//qCDebug(lcPlugin) << msg;
	const QString longConnId(msg.value(QLatin1String("connectorId")).toString());
	const QList<QStringView> propList = QStringView(longConnId).split('|');
	if (propList.size() < 2)
		return;

	QByteArray actIdStr = propList.at(0).split('.').last().toUtf8();
	const int act = tokenMap().value(actIdStr, AT_Unknown);
	if (act != AT_Unknown)
		actIdStr = tokenToName().value(act, actIdStr);

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
		else if (cr.eInstanceType == DSE::EngineInstanceType::Unknown && !id.compare(QByteArrayLiteral("scope")))
			cr.eInstanceType = stringToScope(value);
		else if (cr.expression.isEmpty() && !id.compare(QByteArrayLiteral("expr")))
			cr.expression = value.toUtf8();
		else if (cr.file.isEmpty() && !id.compare(QByteArrayLiteral("file")))
			cr.file = value.toUtf8();
		else if (cr.alias.isEmpty() && !id.compare(QByteArrayLiteral("alias")))
			cr.alias = value.toUtf8();
		else if (act == SA_SingleShot && cr.eInputType == DSE::ScriptInputType::Unknown && !id.compare(QByteArrayLiteral("type")))
			cr.eInputType = stringToInputType(value);
		else
			cr.otherData.insert(id, value.toString());
	}
	//qDebug() << cr.actionType << cr.shortId << cr.otherData;

	switch (act) {
		case SA_Eval:
			cr.eInputType = DSE::ScriptInputType::Expression;
			break;
		case SA_Load:
			cr.eInputType = DSE::ScriptInputType::Script;
			break;
		case SA_Import:
			cr.eInputType = DSE::ScriptInputType::Module;
			break;
		case SA_Update:
			if (DynamicScript *ds = getOrCreateInstance(cr.instanceName, true, true))
				cr.eInputType = ds->inputType();
			break;
	}

	ConnectorData::instance()->insert(cr);
}

#include "moc_Plugin.cpp"
