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
#include <QtConcurrent>
#include <QJsonArray>
#include <QJsonObject>
#include <QMetaObject>
//#include <QThread>

#include "Plugin.h"
#include "common.h"
#include "Logger.h"
#include "DynamicScript.h"
#include "ScriptEngine.h"

enum ActionHandler {
	AH_Script,
	AH_Plugin,
	AH_Unknown,
};

enum ScriptAction {
	SA_Eval,
	SA_Load,
	SA_Import,
	SA_Update,
	SA_SingleShot,
	SA_Unknown
};

enum ControlAction {
	CA_Instance,
	CA_DelInstance,
	CA_SetStateValue,
	CA_ResetEngine,
	CA_Unknown
};

using ScriptState = QHash<QByteArray, DynamicScript *>;
Q_GLOBAL_STATIC(ScriptState, g_instances)

static std::atomic_uint32_t g_errorCount = 0;
static std::atomic_uint32_t g_singleShotCount = 0;
QString g_scriptsBaseDir;


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


// -----------------------------------
// Plugin
// -----------------------------------

Plugin::Plugin(const QString &tpHost, uint16_t tpPort, QObject *parent) :
  QObject(parent),
  client(new TPClientQt(PLUGIN_ID, this))
{
	client->setHostProperties(tpHost, tpPort);

	connect(qApp, &QCoreApplication::aboutToQuit, this, &Plugin::quit);
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
	connect(this, &Plugin::tpConnectorUpdate, client, qOverload<const QByteArray&, uint8_t>(&TPClientQt::connectorUpdate), Qt::QueuedConnection);
	connect(this, &Plugin::tpNotification, client, qOverload<const QByteArray&, const QByteArray&, const QByteArray&, const QVariantList&>(&TPClientQt::showNotification), Qt::QueuedConnection);

	Q_EMIT tpConnect();
	//QMetaObject::invokeMethod(this, "start", Qt::QueuedConnection);
}

Plugin::~Plugin()
{
	for (DynamicScript *ds : qAsConst(*g_instances)) {
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
//	if (client)
//		client->disconnect();
	if (client && client->isConnected())
		clearStateLists();
	Q_EMIT tpDisconnect();
	saveSettings();
}

void Plugin::initEngine()
{
	connect(ScriptEngine::instance(), &ScriptEngine::raiseError, this, &Plugin::onScriptEngineError, Qt::QueuedConnection);
	connect(ScriptEngine::instance(), &ScriptEngine::stateValueUpdateByName, this, &Plugin::onStateUpdateByName, Qt::QueuedConnection);
	connect(ScriptEngine::instance(), &ScriptEngine::stateValueUpdateById, this, &Plugin::tpStateUpdate, Qt::QueuedConnection);
	connect(ScriptEngine::instance(), &ScriptEngine::stateCreate, this, &Plugin::tpStateCreate, Qt::QueuedConnection);
	connect(ScriptEngine::instance(), &ScriptEngine::stateRemove, this, &Plugin::tpStateRemove, Qt::QueuedConnection);
	connect(ScriptEngine::instance(), &ScriptEngine::choiceUpdate, this, &Plugin::tpChoiceUpdateStrList, Qt::QueuedConnection);
	connect(ScriptEngine::instance(), &ScriptEngine::connectorUpdate, this, &Plugin::tpConnectorUpdate, Qt::QueuedConnection);
	connect(ScriptEngine::instance(), &ScriptEngine::tpNotification, this, &Plugin::tpNotification, Qt::QueuedConnection);
	connect(this, &Plugin::tpNotificationClicked, ScriptEngine::instance(), &ScriptEngine::onNotificationClicked, Qt::QueuedConnection);
	//	connect(ScriptEngine::instance(), &ScriptEngine::stateValueUpdateById, client, qOverload<const QByteArray &, const QByteArray &>(&TPClientQt::stateUpdate), Qt::QueuedConnection);
	//	connect(ScriptEngine::instance(), &ScriptEngine::stateCreate, client, qOverload<const QByteArray&, const QByteArray&, const QByteArray& , const QByteArray&>(&TPClientQt::createState), Qt::QueuedConnection);
	//	connect(ScriptEngine::instance(), &ScriptEngine::stateRemove, client, qOverload<const QByteArray&>(&TPClientQt::removeState), Qt::QueuedConnection);
	//	connect(ScriptEngine::instance(), &ScriptEngine::choiceUpdate, client, qOverload<const QByteArray&, const QStringList&>(&TPClientQt::choiceUpdate), Qt::QueuedConnection);
	//	connect(ScriptEngine::instance(), &ScriptEngine::connectorUpdate, client, qOverload<const QByteArray&, uint8_t>(&TPClientQt::connectorUpdate), Qt::QueuedConnection);
	//	connect(ScriptEngine::instance(), &ScriptEngine::showNotification, client, qOverload<const QByteArray&, const QByteArray&, const QByteArray&, const QVariantList&>(&TPClientQt::showNotification), Qt::QueuedConnection);

	connect(this, &Plugin::loggerRotateLogs, Logger::instance(), &Logger::rotateLogs);
}

void Plugin::saveSettings()
{
	QSettings s;
	s.beginGroup("DynamicStates");
	s.remove("");
	for (DynamicScript * const ds : qAsConst(*g_instances)) {
		if (ds->defaultType != DynamicScript::DefaultType::NoDefault)
			s.setValue(ds->name, ds->serialize());
	}
	s.endGroup();
	qCInfo(lcPlugin) << "Saved plugin settings.";
}

void Plugin::loadSettings()
{
	QSettings s;
	s.beginGroup("DynamicStates");
	const QStringList &childs = s.childKeys();
	for (const QString &dvName : childs) {
		// FIXME: TP doesn't fire state change events based on the default value.
		// Best we can do now is send a blank default value and then the actual default (if it's not also blank).
		DynamicScript *ds = getOrCreateInstance(dvName.toUtf8(), false);
		ds->deserialize(s.value(dvName).toByteArray());
		if (ds->defaultValue.isEmpty())
			continue;
		switch (ds->defaultType) {
			case DynamicScript::DefaultType::FixedValue:
				sendScriptState(ds, ds->defaultValue);
				break;
			default:
				QMetaObject::invokeMethod(ds, "evaluateDefault", Qt::QueuedConnection);
				//if (ds->scope == DynamicScript::Scope::Shared)
				//	ds->evaluateDefault();
				//else
				//	auto _ = QtConcurrent::run(&DynamicScript::evaluateDefault, ds);
				break;
		}
	}
	s.endGroup();
}

DynamicScript *Plugin::getOrCreateInstance(const QByteArray &name, bool deferState, bool failIfMissing)
{
	DynamicScript *ds = g_instances->value(name, nullptr);
	if (!ds && !failIfMissing) {
		//qCDebug(lcPlugin) << dvName << "Creating";
		ds = g_instances->insert(name, new DynamicScript(name, this)).value();
		//connect(ds, &DynamicScript::scriptError, this, &Plugin::onDvScriptError, Qt::QueuedConnection);
		//connect(dv, &DynamicScript::dataReady, this, &Plugin::onDvDataReady);
		// Direct connection to socket where state ID is already fully qualified; also emits for ScriptEngine::stateValueUpdateById() signals.
		//connect(ds, &DynamicScript::dataReady, client, qOverload<const QByteArray&, const QByteArray&>(&TPClientQt::stateUpdate), Qt::QueuedConnection);
		// Global from script engine which needs name lookup because the state name is not fully qualified.
		//connect(ds, &DynamicScript::stateValueUpdateByName, this, &Plugin::onStateUpdateByName, Qt::QueuedConnection);
		if (!deferState)
			createScriptState(ds);
	}
	return ds;
}

void Plugin::removeInstance(DynamicScript *ds)
{
	if (ds) {
		removeScriptState(ds);
		if (ds->scope == DynamicScript::Scope::Shared)
			ScriptEngine::instance()->clearInstanceData(ds->name);
		g_instances->remove(ds->name);
		delete ds;
//		ds->deleteLater();
	}
}

void Plugin::sendStateLists() const
{
	QByteArrayList nameArry = g_instances->keys();
  Q_EMIT tpStateUpdate(QByteArrayLiteral(PLUGIN_ID ".state.createdStatesList"), nameArry.join(','));
	if (nameArry.size())
		nameArry = QByteArrayList({ "All Instances", "All Shared Engine Instances", "All Private Engine Instances" }) + nameArry;
	else
		nameArry = QByteArrayList({ QByteArrayLiteral("[ no instances created ]") });
	Q_EMIT tpChoiceUpdate(QByteArrayLiteral(PLUGIN_ID ".act.plugin.instance.name"), nameArry);
}

void Plugin::clearStateLists() const
{
//	Q_EMIT tpStateUpdate(QByteArrayLiteral(PLUGIN_ID ".state.createdStatesList"), QByteArrayLiteral(""));
	client->stateUpdate(PLUGIN_ID ".state.createdStatesList", "");
	qCInfo(lcPlugin) << "Cleared States list.";
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

//void Plugin::evaluateDV(DynamicScript *dv) const
//{
//	if (ds->scope == DynamicScript::Scope::Shared)
//		ds->evaluate();
//	else
//		QtConcurrent::run(ds, &DynamicScript::evaluate);
//}

void Plugin::onStateUpdateByName(const QByteArray &name, const QByteArray &value) const
{
	qCDebug(lcPlugin) << "Sending state update" << DYNAMIC_VALUE_STATE_PRFX + name;
	Q_EMIT tpStateUpdate(DYNAMIC_VALUE_STATE_PRFX + name, value);
	//if (DynamicScript *ds = g_instances->value(name))
	//	sendScriptState(ds, value);
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
		<< ") with entry.tp v" << info.pluginVersion << ", running v" << PLUGIN_VERSION_STR;
	handleSettings(settings);
	initEngine();
	loadSettings();
	clearScriptErrors();
	sendStateLists();
}

void Plugin::onTpMessage(TPClientQt::MessageType type, const QJsonObject &msg)
{
	//qCDebug(lcPlugin) << msg;
	switch (type) {
		case TPClientQt::MessageType::action:
			dispatchAction(msg);
			return;

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

void Plugin::dispatchAction(const QJsonObject &msg)
{
	const QString actId = msg.value(QLatin1String("actionId")).toString();
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	const QVector<QStringRef> actIdArry = actId.splitRef('.');
#else
	const QVector<QStringView> actIdArry = QStringView(actId).split('.');
#endif
	if (actIdArry.length() < 8) {
		qCWarning(lcPlugin) << "Action ID is malformed for action:" << actId;
		return;
	}
	const int handler = tokenMap().value(actIdArry.at(6).toUtf8(), AH_Unknown);
	if (handler == AH_Unknown) {
		qCWarning(lcPlugin) << "Unknown action for this plugin:" << actId;
		return;
	}
	const QJsonArray data = msg.value(QLatin1String("data")).toArray();
	if (!data.size()) {
		qCWarning(lcPlugin) << "Action data missing for action:" << actId;
		return;  // we have no actions w/out data members
	}

	const QByteArray action(actIdArry.at(7).toUtf8());
	switch(handler) {
		case AH_Script:
			scriptAction(action, data);
			break;

		case AH_Plugin:
			pluginAction(action, data);
			break;

		default:
			return;
	}
}

void Plugin::scriptAction(const QByteArray &actId, const QJsonArray &data)
{
	//qCDebug(lcPlugin) << actId << data;
	const ScriptAction act = (ScriptAction)tokenMap().value(actId, SA_Unknown);
	if (act == SA_Unknown) {
		qCWarning(lcPlugin) << "Unknown Script Handler action:" << actId;
		return;
	}

	const QMap<QString, QString> dataMap = TPClientQt::actionDataToMap(data, '.');

	QByteArray dvName;
	if (act == SA_SingleShot) {
		dvName = "ANON_" + QByteArray::number(++g_singleShotCount);
	}
	else {
		dvName = dataMap.value("name").simplified().replace(' ', '_').toUtf8();
		if (dvName.isEmpty()) {
			qCWarning(lcPlugin) << "Script state name missing for action" << actId;
			return;
		}
	}

	DynamicScript *ds = getOrCreateInstance(dvName, act == SA_SingleShot, act == SA_Update);
	if (!ds) {
		raiseScriptError(dvName, tr("ValidationError: Instance not found for state name %1").arg(dvName.constData()), tr("VALIDATION ERROR"));
		return;
	}
	DynamicScript::Scope scope = dataMap.value("scope", QStringLiteral("Shared")).at(0) == 'S' ? DynamicScript::Scope::Shared : DynamicScript::Scope::Private;
	const QString &dtyp = act == SA_SingleShot ? QString() : dataMap.value("save", QStringLiteral("No"));
	DynamicScript::DefaultType defType =
	    dtyp.isEmpty() || dtyp.at(0) == 'N' ? DynamicScript::DefaultType::NoDefault
	                                        : dtyp.at(0) == 'F' ? DynamicScript::DefaultType::FixedValue
	                                                            : dtyp.at(0) == 'C' ? DynamicScript::DefaultType::CustomExpression
	                                                                                : DynamicScript::DefaultType::MainExpression;
	const QByteArray defVal = defType != DynamicScript::DefaultType::NoDefault ? dataMap.value("default").toUtf8() : QByteArray();
	bool ok = false;
	switch (act)
	{
		case SA_Eval:
			ok = ds->setExpressionProperties(scope, dataMap.value("expr"), defType, defVal);
			break;

		case SA_Load:
			ok = ds->setScriptProperties(scope, dataMap.value("file").trimmed(), dataMap.value("expr"), defType, defVal);
			break;

		case SA_Import:
			ok = ds->setModuleProperties(scope, dataMap.value("file").trimmed(), dataMap.value("alias").trimmed(), dataMap.value("expr"), defType, defVal);
			break;

		case SA_Update:
			ok = ds->setExpression(dataMap.value("expr"));
			break;

		case SA_SingleShot: {
			const QString &ityp = dataMap.value("type", QStringLiteral("Expression"));
			DynamicScript::InputType iType = ityp.isEmpty() || ityp.at(0) == 'E' ? DynamicScript::InputType::Expression : ityp.at(0) == 'S' ? DynamicScript::InputType::Script : DynamicScript::InputType::Module;
			ds->singleShot = true;
			ok = ds->setProperties(iType, scope, dataMap.value("expr"), dataMap.value("file").trimmed(), dataMap.value("alias").trimmed(), defType);
			break;
		}

		default:
			return;
	}
	if (!ok) {
		raiseScriptError(ds->name, QStringLiteral("ValidationError:") + ds->lastError, tr("VALIDATION ERROR"));
		if (ds->singleShot)
			removeInstance(ds);
		return;
	}
	QMetaObject::invokeMethod(ds, "evaluate", Qt::QueuedConnection);
	//evaluateDV(dv);
}

void Plugin::pluginAction(const QByteArray &actId, const QJsonArray &data)
{
	ControlAction act = (ControlAction)tokenMap().value(actId, CA_Unknown);
	if (act == CA_Unknown) {
		qCWarning(lcPlugin) << "Unknown Control Handler action:" << actId;
		return;
	}
	const QMap<QString, QString> dataMap = TPClientQt::actionDataToMap(data, '.');
	act = (ControlAction)tokenMap().value(dataMap.value("action").toUtf8(), CA_Unknown);
	if (act == CA_Unknown) {
		qCWarning(lcPlugin) << "Unknown Command action:" << dataMap.value("action");
		return;
	}
	const QByteArray dvName = dataMap.value("name", "All").toUtf8();
	quint8 type = 0;  // named instance
	if (dvName.startsWith("All "))
		type = (dvName.at(4) == 'I' ? 255 : dvName.at(4) == 'S' ? (quint8)DynamicScript::Scope::Shared : dvName.at(4) == 'P' ? (quint8)DynamicScript::Scope::Private : 0);
	//qCDebug(lcPlugin) << "Command" << TPClientQt::getIndexedActionDataValue(0, data) << act << dvName;
	switch (act)
	{
		case CA_DelInstance: {
			if (type) {
				QMutableHashIterator<QByteArray, DynamicScript *> it(*g_instances);
				while (it.hasNext()) {
					it.next();
					if (type == 255 || type == (quint8)it.value()->scope.load()) {
						removeScriptState(it.value(), true);
						delete it.value(); //->deleteLater();
						it.remove();
					}
				}
				if (type == 255 || type == (quint8)DynamicScript::Scope::Shared){
					ScriptEngine::instance()->reset();
					qCInfo(lcPlugin) << "Shared Scripting Engine reset completed.";
				}
				sendStateLists();
			}
			else if (DynamicScript *ds = g_instances->value(dvName)) {
				removeInstance(ds);
			}
			else {
				qCWarning(lcPlugin) << "Script not found for name:" << dvName;
				sendStateLists();  // update the states list anyway in case there's a stale name in tere
				return;
			}
			return;
		}

		case CA_SetStateValue: {
			const QByteArray stateValue = dataMap.value("value").toUtf8();
			if (type) {
				for (DynamicScript * const ds : qAsConst(*g_instances)) {
					if (type == 255 || type == (quint8)ds->scope.load())
						sendScriptState(ds, stateValue);
				}
			}
			else if (DynamicScript *ds = g_instances->value(dvName)) {
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
				if (DynamicScript *ds = g_instances->value(dvName))
					ds->resetEngine();
				else
					qCWarning(lcPlugin) << "Script not found for name:" << dvName;
				return;
			}
			if (type == 255 || type == (quint8)DynamicScript::Scope::Private) {
				for (DynamicScript * const ds : qAsConst(*g_instances)) {
					if (ds->scope == DynamicScript::Scope::Private)
						ds->resetEngine();
				}
			}
			if (type == 255 || type == (quint8)DynamicScript::Scope::Shared) {
				ScriptEngine::instance()->reset();
				qCInfo(lcPlugin) << "Shared Scripting Engine reset completed.";
			}
			return;
		}

		default:
			return;
	}
}


void Plugin::handleSettings(const QJsonObject &settings)
{
	//qCDebug(lcPlugin) << "Got settings object:" << settings;
	QJsonObject::const_iterator next = settings.begin(), last = settings.end();
	for (; next != last; ++next) {
		if (next.key().startsWith(QStringLiteral("Script Files"))) {
			g_scriptsBaseDir = QDir::fromNativeSeparators(next.value().toString());
			if (!g_scriptsBaseDir.endsWith('/'))
				g_scriptsBaseDir += '/';
			//qCDebug(lcPlugin) << g_scriptsBaseDir << settings;
		}
	}
}
