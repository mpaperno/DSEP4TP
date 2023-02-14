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

#include "common.h"
#include "Plugin.h"
#include "DSE.h"
#include "Logger.h"
#include "DynamicScript.h"
#include "ScriptEngine.h"
#include "ConnectorData.h"

#define SETTINGS_GROUP_PLUGIN    "Plugin"
#define SETTINGS_GROUP_SCRIPTS   "DynamicStates"

using namespace Strings;

enum TimerEventType : quint8 {
	TE_None,
	TE_RemoveScriptInstance,
};

struct TimerData {
	TimerEventType type = TE_None;
	QVariant data;
	TimerData() {}
	TimerData(TimerEventType t, const QVariant &d) : type(t), data(d) {}
};

using TimerDatahash = QHash<int, TimerData>;
Q_GLOBAL_STATIC(TimerDatahash, g_timersData)
Q_GLOBAL_STATIC(QReadWriteLock, g_timersDataMutex)

using ScriptTimerHash = QHash<QByteArray, int>;
Q_GLOBAL_STATIC(ScriptTimerHash, g_instanceDeleteTimers)
Q_GLOBAL_STATIC(QReadWriteLock, g_instanceTimersMutex)

bool g_startupComplete = false;
std::atomic_bool g_ignoreNextSettings = true;
std::atomic_bool g_shuttingDown = false;
std::atomic_uint32_t g_errorCount = 0;

static DSE::EngineInstanceType stringToScope(const QByteArray &str, bool unknownIsPrivate = false)
{
	return str == tokenToName(AT_Shared) ? DSE::SharedInstance : (unknownIsPrivate || str == tokenToName(AT_Private) ? DSE::PrivateInstance : DSE::UnknownInstanceType);
}

// legacy for v < 1.2
static DSE::StateDefaultType stringToDefaultType(QStringView str)
{
	return str.isEmpty() || str.at(0) == 'N' ? DSE::NoStateUsed :
	                                           str.at(0) == 'F' ? DSE::FixedValueDefault :
	                                                              str.at(0) == 'C' ? DSE::CustomExprDefault :
	                                                                                 DSE::MainExprDefault;
}

static DSE::StateDefaultType stringToStateType(QStringView str)
{
	//	"No",
	//  "Yes, default type:\nFixed Value",
	//  "Yes, default type:\nCustom Expression",
	//  "Yes, default type:\nAction's Expression"
	if (str.length() < 20 /*|| str.at(0) != 'Y'*/)
		return DSE::NoStateUsed;

	const QChar typ = str.at(19);
	return typ == 'A' ? DSE::MainExprDefault : typ == 'C' ? DSE::CustomExprDefault : DSE::FixedValueDefault;
}

static DSE::PersistenceType stringToPersistenceType(QStringView str)
{
	// "Session", "Saved", "Temporary"
	if (str.isEmpty() || (str.at(0) == 'S' && str.at(1) == 'e'))
		return DSE::PersistenceType::PersistSession;
	return str.at(0) == 'T' ? DSE::PersistenceType::PersistTemporary : DSE::PersistenceType::PersistSave;
}

static DSE::ActivationBehaviors stringToActivationType(QStringView str)
{
	//	"On Press",
	//  "On Press &\nRelease",
	//  "On Press\nthen Repeat",
	//  "Repeat\nafter Delay",
	//  "On Release"
	if (str.length() < 4 || str.at(3) == 'R')
		return DSE::ActivationBehavior::OnRelease;
	if (str.at(0) == 'R')
		return DSE::ActivationBehavior::RepeatOnHold;
	if (str.length() < 10)
		return DSE::ActivationBehavior::OnPress;
	if (str.at(8) == '\n')
		return DSE::ActivationBehavior::OnPress | DSE::ActivationBehavior::RepeatOnHold;
	return DSE::ActivationBehavior::OnPress | DSE::ActivationBehavior::OnRelease;
}


// -----------------------------------
// Plugin
// -----------------------------------

Plugin *Plugin::instance = nullptr;

Plugin::Plugin(const QString &tpHost, uint16_t tpPort, const QByteArray &pluginId, QObject *parent) :
  QObject(parent),
  m_pluginId(!pluginId.isEmpty() ? pluginId : QByteArrayLiteral(PLUGIN_ID)),
  client(new TPClientQt(m_pluginId /*, this*/)),
  clientThread(new QThread())
{
	instance = this;

	qRegisterMetaType<JSError>("JSError");

	loadPluginSettings();

	if (m_pluginId != PLUGIN_ID)
		DSE::valueStatePrefix = m_pluginId + '.';

	client->setHostProperties(tpHost, tpPort);
	// Set up constant IDs of things we send to TP like states and choice list updates.
	{
		auto const &tokens = tokenStrings();
		for (int i = 0; i < SID_ENUM_MAX; ++i)
			m_stateIds[i] =  m_pluginId + QByteArrayLiteral(".state.") + tokens[i];
	}
	{
		auto const &tokens = choiceListTokenStrings();
		for (int i = 0; i < CLID_ENUM_MAX; ++i)
			m_choiceListIds[i] =  m_pluginId + QByteArrayLiteral(".act.") + tokens[i];
	}

	connect(qApp, &QCoreApplication::aboutToQuit, this, &Plugin::quit);
	connect(this, &Plugin::loggerRotateLogs, Logger::instance(), &Logger::rotateLogs);
	connect(client, &TPClientQt::connected, this, &Plugin::onTpConnected);
	connect(client, &TPClientQt::disconnected, this, &Plugin::onClientDisconnect);
	connect(client, &TPClientQt::error, this, &Plugin::onClientError);
	connect(client, &TPClientQt::message, this, &Plugin::onTpMessage, Qt::QueuedConnection);
	connect(this, &Plugin::tpConnect, client, qOverload<>(&TPClientQt::connect), Qt::QueuedConnection);
	//connect(this, &Plugin::tpDisconnect, client, &TPClientQt::disconnect, Qt::DirectConnection);
	connect(this, &Plugin::tpStateUpdate, client, qOverload<const QByteArray &, const QByteArray &>(&TPClientQt::stateUpdate), Qt::QueuedConnection);
	connect(this, &Plugin::tpStateCreate, client, qOverload<const QByteArray &, const QByteArray &, const QByteArray &, const QByteArray &>(&TPClientQt::createState), Qt::QueuedConnection);
	connect(this, &Plugin::tpStateRemove, client, qOverload<const QByteArray &>(&TPClientQt::removeState), Qt::QueuedConnection);
	connect(this, &Plugin::tpChoiceUpdate, client, qOverload<const QByteArray &, const QByteArrayList &>(&TPClientQt::choiceUpdate), Qt::QueuedConnection);
	connect(this, &Plugin::tpChoiceUpdateInstance, client, qOverload<const QByteArray &, const QByteArray &, const QByteArrayList &>(&TPClientQt::choiceUpdate), Qt::QueuedConnection);
	connect(this, &Plugin::tpConnectorUpdateShort, client, qOverload<const QByteArray&, uint8_t>(&TPClientQt::connectorUpdate), Qt::QueuedConnection);
	connect(this, &Plugin::tpSettingUpdate, client, qOverload<const QByteArray&, const QByteArray &>(&TPClientQt::settingUpdate), Qt::QueuedConnection);
	// These are just for scripting engine user functions, not used by plugin directly. Emitted by ScriptEngine.
	connect(this, &Plugin::tpChoiceUpdateStrList, client, qOverload<const QByteArray &, const QStringList &>(&TPClientQt::choiceUpdate), Qt::QueuedConnection);
	connect(this, &Plugin::tpChoiceUpdateInstanceStrList, client, qOverload<const QByteArray &, const QByteArray &, const QStringList &>(&TPClientQt::choiceUpdate), Qt::QueuedConnection);
	connect(this, &Plugin::tpConnectorUpdate, client, qOverload<const QByteArray&, uint8_t, bool>(&TPClientQt::connectorUpdate), Qt::QueuedConnection);
	connect(this, &Plugin::tpNotification, client, qOverload<const QByteArray&, const QByteArray&, const QByteArray&, const QVariantList&>(&TPClientQt::showNotification), Qt::QueuedConnection);

	client->moveToThread(clientThread);
	clientThread->start();

	m_loadSettingsTmr.setSingleShot(true);
	m_loadSettingsTmr.setInterval(750);
	connect(&m_loadSettingsTmr, &QTimer::timeout, this, &Plugin::loadStartupSettings);

	Q_EMIT tpConnect();
	//QMetaObject::invokeMethod(this, "start", Qt::QueuedConnection);
}

Plugin::~Plugin()
{
	if (!g_shuttingDown)
		quit();

	delete DSE::defaultScriptInstance;
	DSE::defaultScriptInstance = nullptr;
	delete ScriptEngine::sharedInstance;
	ScriptEngine::sharedInstance = nullptr;

	delete client;
	client = nullptr;
	delete clientThread;
	clientThread = nullptr;
	qCInfo(lcPlugin) << PLUGIN_SHORT_NAME " exiting.";
}

//void Plugin::start()
//{
//	Q_EMIT tpConnect();
//}

void Plugin::exit()
{
	if (!g_shuttingDown)
		qApp->quit();
}

void Plugin::quit()
{
	if (g_shuttingDown)
		return;
	g_shuttingDown = true;

	QWriteLocker tl(g_timersDataMutex);
	for (int timId : g_timersData->keys())
		killTimer(timId);
	tl.unlock();

	if (client) {
		disconnect(client, nullptr, this, nullptr);
		disconnect(this, nullptr, client, nullptr);
		if (client->thread() != qApp->thread())
			Utils::runOnThreadSync(client->thread(), [=]() { client->moveToThread(qApp->thread()); });
		if (client->isConnected()) {
			client->stateUpdate(m_stateIds[SID_PluginState], tokenToName(AT_Stopped));
			client->stateUpdate(m_stateIds[SID_CreatedInstanceList], QByteArray());
			client->disconnect();
		}
	}

	savePluginSettings();
	saveAllInstances();

	QWriteLocker il(DSE::instances_mutex());
	qDeleteAll(*DSE::instances());
	DSE::instances()->clear();
	il.unlock();

	QWriteLocker el(DSE::engines_mutex());
	qDeleteAll(*DSE::engines());
	DSE::engines()->clear();
	el.unlock();

	if (clientThread) {
		clientThread->quit();
		clientThread->wait();
	}

}

void Plugin::initEngine()
{
	new ScriptEngine(QByteArrayLiteral("Shared"));
	connect(ScriptEngine::instance(), &ScriptEngine::engineError, this, &Plugin::onEngineError, Qt::QueuedConnection);
	connect(this, &Plugin::setActionRepeatProperty, DSE::sharedInstance, &DSE::setActionRepeatProperty, Qt::QueuedConnection);
	connect(DSE::sharedInstance, &DSE::defaultActionRepeatRateChanged, this, &Plugin::onActionRepeatRateChanged, Qt::QueuedConnection);
	connect(DSE::sharedInstance, &DSE::defaultActionRepeatDelayChanged, this, &Plugin::onActionRepeatDelayChanged, Qt::QueuedConnection);

	// Default "anonymous" shared worker instance.
	DynamicScript *ds = DSE::defaultScriptInstance = new DynamicScript(QByteArrayLiteral("Default Shared"));
	ds->setEngine(ScriptEngine::instance());
	ds->setExpressionProperties(QString());
	connect(ds, &DynamicScript::scriptError, this, &Plugin::onScriptError, Qt::QueuedConnection);
	// currently doesn't create or update any State... reserved for future use?
	//connect(ds, &DynamicScript::dataReady, client, qOverload<const QByteArray&, const QByteArray&>(&TPClientQt::stateUpdate), Qt::QueuedConnection);
}

void Plugin::savePluginSettings() const
{
	QSettings s;
	s.beginGroup(SETTINGS_GROUP_PLUGIN);
	s.setValue("SettingsVersion", APP_VERSION);
	s.setValue("ScriptsBaseDir", DSE::scriptsBaseDir);
	s.endGroup();
}

void Plugin::loadPluginSettings()
{
	QSettings s;
	DSE::scriptsBaseDir = s.value(SETTINGS_GROUP_PLUGIN "/ScriptsBaseDir", QString()).toString();
}

void Plugin::loadStartupSettings()
{
	QSettings s;
	s.beginGroup(SETTINGS_GROUP_PLUGIN);
	DSE::sharedInstance->setDefaultActionRepeatRate(s.value("actRepeatRate", 350).toInt());
	DSE::sharedInstance->setDefaultActionRepeatDelay(s.value("actRepeatDelay", 350).toInt());
	s.endGroup();

	loadAllInstances();

	g_startupComplete = true;
	g_ignoreNextSettings = true;
	Q_EMIT tpStateUpdate(m_stateIds[SID_PluginState], tokenToName(AT_Started));
	Q_EMIT tpSettingUpdate(tokenToName(ST_SettingsVersion), QByteArray::number(APP_VERSION));
}

void Plugin::saveAllInstances() const
{
	if (!g_startupComplete)
		return;
	int count = 0;
	QSettings s;
	s.beginGroup(SETTINGS_GROUP_SCRIPTS);
	s.remove("");
	for (DynamicScript * const ds : DSE::instances_const()) {
		if (ds->persistence() == DSE::PersistSave) {
			s.setValue(ds->name, ds->serialize());
			++count;
		}
	}
	s.endGroup();
	qCInfo(lcPlugin) << "Saved" << count << "instance(s) to settings.";
}

bool Plugin::saveScriptInstance(const QByteArray &name) const
{
	DynamicScript *ds = DSE::instance(name);
	if (!ds /*|| ds->persistence() != DSE::PersistSave*/)
		return false;
	QSettings s;
	const QString key(QByteArrayLiteral(SETTINGS_GROUP_SCRIPTS "/") + ds->name);
	s.setValue(key, ds->serialize());
	return true;
}

void Plugin::loadAllInstances() const
{
	int count = 0;
	QSettings s;
	s.beginGroup(SETTINGS_GROUP_SCRIPTS);
	const QStringList &childs = s.childKeys();
	s.endGroup();
	for (const QString &dvName : childs) {
		if (DynamicScript *ds = loadScriptInstance(dvName.toUtf8())) {
			++count;
			QMetaObject::invokeMethod(ds, "evaluateDefault", Qt::QueuedConnection);
		}
	}
	qCInfo(lcPlugin) << "Loaded" << count << "saved instance(s) from settings.";

	sendInstanceLists();
}

DynamicScript *Plugin::loadScriptInstance(const QByteArray &name) const
{
	DynamicScript *ds = getOrCreateInstance(name);
	if (!loadScriptSettings(ds)) {
		removeInstance(ds);
		return nullptr;
	}
	if (ds->instanceType() == DSE::PrivateInstance) {
		if (Q_UNLIKELY(ds->engineName().isEmpty())) {
			qCWarning(lcPlugin) << "Engine name for script instance" << name << "is empty.";
			return ds;
		}
		ds->setEngine(getOrCreateEngine(ds->engineName()));
	}
	else {
		ds->setEngine(ScriptEngine::instance());
	}
	return ds;
}

bool Plugin::loadScriptSettings(DynamicScript *ds) const
{
	QSettings s;
	const QString key(QByteArrayLiteral(SETTINGS_GROUP_SCRIPTS "/") + ds->name);
	return s.contains(key) ? ds->deserialize(s.value(key).toByteArray()) : false;
}

ScriptEngine *Plugin::getOrCreateEngine(const QByteArray &name, bool failIfMissing) const
{
	ScriptEngine *se = DSE::engine(name);
	if (!se && !failIfMissing) {
		se = DSE::insert(name, new ScriptEngine(name));
		// Instance-specific errors from background tasks.
		connect(se, &ScriptEngine::engineError, this, &Plugin::onEngineError, Qt::QueuedConnection);
		sendEngineLists();
	}
	return se;
}

DynamicScript *Plugin::getOrCreateInstance(const QByteArray &name, bool forUpdateAction, bool loadSettings) const
{
	DynamicScript *ds = DSE::instance(name);
	if (!ds) {
		if (forUpdateAction)
			return (DSE::defaultScriptInstance->name == name ? DSE::defaultScriptInstance : nullptr);

		//qCDebug(lcPlugin) << dvName << "Creating";
		ds = DSE::insert(name, new DynamicScript(name));
		if (loadSettings)
			loadScriptSettings(ds);
		connect(ds, &DynamicScript::scriptError, this, &Plugin::onScriptError, Qt::QueuedConnection);
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
	ScriptEngine::instance()->clearInstanceData(ds);
	ds->removeTpState();
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

void Plugin::stopDeletionTimer(const QByteArray &name)
{
	QReadLocker rdlock(g_instanceTimersMutex);
	if (int timId = g_instanceDeleteTimers->value(name, 0)) {
		killTimer(timId);
		rdlock.unlock();
		QWriteLocker wrlock(g_instanceTimersMutex);
		(*g_instanceDeleteTimers)[name] = 0;
		QWriteLocker tl(g_timersDataMutex);
		g_timersData->remove(timId);
	}
}

void Plugin::removeInstanceLater(DynamicScript *ds)
{
	if (!ds)
		return;

	stopDeletionTimer(ds->name);
	const int timId = startTimer(ds->autoDeleteDelay(), Qt::CoarseTimer);
	if (timId) {
		QWriteLocker wrlock(g_instanceTimersMutex);
		(*g_instanceDeleteTimers)[ds->name] = timId;
		QWriteLocker tl(g_timersDataMutex);
		g_timersData->emplace(timId, TE_RemoveScriptInstance, QVariant::fromValue(ds->name));
		return;
	}
	qCWarning(lcPlugin) << "Could not start timer to remove instance" << ds->name;
}

void Plugin::sendInstanceLists() const
{
	QByteArrayList nameArry = DSE::instanceKeys();
	std::sort(nameArry.begin(), nameArry.end());
	Q_EMIT tpStateUpdate(m_stateIds[SID_CreatedInstanceList], nameArry.join(',') + ',');
	nameArry.prepend(DSE::defaultScriptInstance->name);
	Q_EMIT tpChoiceUpdate(m_choiceListIds[CLID_ScriptUpdateInstanceName], nameArry);
	nameArry[0] = tokenToName(AT_Default);
	Q_EMIT tpChoiceUpdate(m_choiceListIds[CLID_RepeatPropertyScriptName], nameArry);
}

void Plugin::sendEngineLists() const
{
	QByteArrayList nameArry = DSE::engineKeys();
	std::sort(nameArry.begin(), nameArry.end());
	nameArry.prepend(tokenToName(AT_Private));
	nameArry.prepend(tokenToName(AT_Shared));
	Q_EMIT tpChoiceUpdate(m_choiceListIds[CLID_ScriptActionEngineScope], nameArry);
}

void Plugin::updateInstanceChoices(int token, const QByteArray &instId) const
{
	ActionTokens listType = token == CA_DelEngine || token == CA_ResetEngine ? AT_Engine : AT_Script;
	QByteArrayList nameArry;
	if (listType == AT_Engine) {
		nameArry = DSE::engineKeys();
	}
	else if (token != CA_LoadInstance && token != CA_DelSavedInstance) {
		nameArry = DSE::instanceKeys();
	}
	else {
		QSettings s;
		s.beginGroup(SETTINGS_GROUP_SCRIPTS);
		const QStringList keys = s.childKeys();
		s.endGroup();
		if (keys.size()) {
			nameArry.resize(keys.size());
			std::transform(keys.cbegin(), keys.cend(), nameArry.begin(), [](const QString &s) { return s.toUtf8(); });
		}
	}
	if (nameArry.isEmpty()) {
		nameArry.append(QByteArrayLiteral("[ no instances created ]"));
	}
	else {
		std::sort(nameArry.begin(), nameArry.end());
		if (token == CA_DelEngine) {
			nameArry.prepend(QByteArrayLiteral("All Private Engine Instances"));
		}
		else if (token != CA_SaveInstance && token != CA_LoadInstance && token != CA_DelSavedInstance) {
			const QByteArray &typeName = tokenToName(listType);
			nameArry.prepend(QLatin1String("All Private %1 Instances").arg(typeName).toUtf8());
			nameArry.prepend(QLatin1String("All Shared %1 Instances").arg(typeName).toUtf8());
			nameArry.prepend(QByteArrayLiteral("All Instances"));
		}
		else {
			nameArry.prepend(QByteArrayLiteral("All Persistent Script Instances"));
		}
	}
	if (instId.isEmpty()) {
		Q_EMIT tpChoiceUpdate(m_choiceListIds[CLID_PluginControlInstanceName], nameArry);
	}
	else {
		Q_EMIT tpChoiceUpdateInstance(m_choiceListIds[CLID_PluginControlInstanceName], instId, nameArry);
	}
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
	Q_EMIT tpStateUpdate(m_stateIds[param == AT_Rate ? SID_ActRepeatDelay : SID_ActRepeatDelay], QByteArray::number(ms));
	const QByteArray &paramName = tokenToName(param);
	updateConnectors({
		{"actionType", tokenToName(AID_RepeatRate)},
		{"instanceName", tokenToName(AT_Default)},
		{"otherData",  QStringLiteral("*\"param\":\"*%1*\"*").arg(paramName)},
		{"otherData",  QStringLiteral("*\"action\":\"%1\"*").arg(tokenToName(AT_Set))},
	}, ms, 50.0f, 60000.0f);

	QSettings().setValue(SETTINGS_GROUP_PLUGIN "/actRepeat" + paramName, ms);
}

void Plugin::raiseScriptError(const QByteArray &dsName, const QString &msg, const QString &type, const QString &stack) const
{
	const uint32_t count = ++g_errorCount;
	Q_EMIT tpStateUpdate(m_stateIds[SID_ErrorCount], QByteArray::number(count));
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
	Q_EMIT tpStateUpdate(m_stateIds[SID_LastError], v);
}

void Plugin::clearScriptErrors()
{
	g_errorCount = 0;
	Q_EMIT tpStateUpdate(m_stateIds[SID_ErrorCount], QByteArrayLiteral("0"));
}


void Plugin::timerEvent(QTimerEvent *ev)
{
	if (ev->timerId())
		killTimer(ev->timerId());
	else
		return;

	g_timersDataMutex->lockForWrite();
	const TimerData timData = g_timersData->take(ev->timerId());
	g_timersDataMutex->unlock();
	if (timData.type == TE_RemoveScriptInstance) {
		const QByteArray instName = timData.data.toByteArray();
		g_instanceTimersMutex->lockForWrite();
		(*g_instanceDeleteTimers)[instName] = 0;
		g_instanceTimersMutex->unlock();
		if (DynamicScript *ds = DSE::instance(instName))
			removeInstance(ds);
	}
}

void Plugin::onStateUpdateByName(const QByteArray &name, const QByteArray &value) const
{
	//qCDebug(lcPlugin) << "Sending state update" << PLUGIN_STATE_ID_PREFIX + name;
	Q_EMIT tpStateUpdate(DSE::valueStatePrefix + name, value);
}

void Plugin::onClientDisconnect()
{
	if (!g_shuttingDown) {
		if (!g_startupComplete)
			qCCritical(lcPlugin()) << "Unable to connect to Touch Portal, shutting down now.";
		else
			qCCritical(lcPlugin()) << "Unexpectedly disconnected from Touch Portal, shutting down now.";
		exit();
	}
}

void Plugin::onClientError(QAbstractSocket::SocketError /*e*/)
{
	if (g_startupComplete)
		qCCritical(lcPlugin()) << "Lost connection to Touch Portal, shutting down now.";
	else
		qCCritical(lcPlugin()) << "Unable to connect to Touch Portal, shutting down now.";
	exit();
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

void Plugin::onDsFinished()
{
	DynamicScript *ds = qobject_cast<DynamicScript *>(sender());
	if (ds && ds->isTemporary())
		removeInstanceLater(ds);
		//removeInstance(ds);
}

void Plugin::onActionRepeatRateChanged(int ms) const { updateActionRepeatProperties(ms, AT_Rate); }
void Plugin::onActionRepeatDelayChanged(int ms) const { updateActionRepeatProperties(ms, AT_Delay); }

void Plugin::onTpConnected(const TPClientQt::TPInfo &info, const QJsonObject &settings)
{
	qCInfo(lcPlugin).nospace().noquote()
		<< PLUGIN_SHORT_NAME " v" APP_VERSION_STR " Connected to Touch Portal v" << info.tpVersionString
		<< " (" << info.tpVersionCode << "; SDK v" << info.sdkVersion
		<< ") for plugin ID " << m_pluginId << " with entry.tp v" << info.pluginVersion;
	DSE::tpVersion = info.tpVersionCode;
	DSE::tpVersionStr = info.tpVersionString;
	Q_EMIT tpStateUpdate(m_stateIds[SID_PluginState], tokenToName(AT_Starting));
	handleSettings(settings);
	Q_EMIT tpStateUpdate(m_stateIds[SID_TpDataPath], Utils::tpDataPath());
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
			break;

		case TPClientQt::MessageType::listChange: {
			if (!msg.value("actionId").toString().endsWith(tokenStrings()[AID_InstanceControl]))
				break;
			if (!msg.value("listId").toString().endsWith(QLatin1String(".action")))
				break;
			int token = tokenFromName(msg.value("value").toString().toUtf8());
			if (token != AT_Unknown) {
				updateInstanceChoices(token, msg.value("instanceId").toString().toUtf8());
			}
			break;
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
				Q_EMIT tpStateUpdate(m_stateIds[SID_TpCurrentPage], pgName);
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
			if (!g_ignoreNextSettings)
				handleSettings(msg);
			g_ignoreNextSettings = false;
			break;

		case TPClientQt::MessageType::closePlugin:
			qCInfo(lcPlugin()) << "Got plugin close message from TP, exiting.";
			exit();
			return;

		case TPClientQt::MessageType::notificationOptionClicked:
			// passthrough to any scripts which may be listening on a callback.
			Q_EMIT tpNotificationClicked(msg.value(QLatin1String("notificationId")).toString(), msg.value(QLatin1String("optionId")).toString());
			break;

		default:
			break;
	}
	Q_EMIT tpMessageEvent(msg);
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
	const QByteArray dvName = dataMap.value("name").trimmed().toUtf8();
	if (dvName.isEmpty()) {
		if (act == AID_SingleShot)
			qCCritical(lcPlugin) << "Anyonymous script instances are no longer supported. Please use another type with 'Persistence' set to 'Temporary'.";
		else
			qCCritical(lcPlugin) << "Script Instance Name missing for action" << tokenToName(act);
		return;
	}

	DynamicScript *ds = getOrCreateInstance(dvName, act == AID_Update, true);
	if (!ds) {
		raiseScriptError(dvName, tr("ValidationError: Could not find script instance '%1' for Update action.").arg(dvName.constData()), tr("VALIDATION ERROR"));
		return;
	}

	// Stop possible deletion timer for temporary instance.
	if (ds->persistence() == DSE::PersistenceType::PersistTemporary)
		stopDeletionTimer(ds->name);

	// Always unset the pressed state first because we cannot have the same action running concurrently.
	ds->setPressedState(false);
	// If used "On-Hold" and this is a release event, invoke eval method and exit now.
	// The script should handle whatever it needs to do with the data it already has.
	if (type == TPClientQt::MessageType::up) {
		QMetaObject::invokeMethod(ds, "evaluate", Qt::QueuedConnection);
		return;
	}

	// When used in "On-Press" the action is actually fired by TP on button release.
	if (type == TPClientQt::MessageType::action)
		ds->setActivation(DSE::ActivationBehavior::OnRelease);
	// If action is used in On-Hold then it may have separate on-press/hold/release behaviors.
	else
		ds->setActivation(stringToActivationType(dataMap.value("activation")));

	if (act != AID_Update) {
		ScriptEngine *se;
		const QByteArray &strScope = dataMap.value("scope", QStringLiteral("Shared")).toUtf8();
		DSE::EngineInstanceType scope = stringToScope(strScope);
		// If a scope/instance type returns Unknown this means it should be a specific named engine instance.
		if (scope == DSE::UnknownInstanceType) {
			// Make sure it's not an empty name.
			if (strScope.isEmpty()) {
				raiseScriptError(dvName, tr("ValidationError: Engine name/type is empty for script instance '%1'.").arg(dvName.constData()), tr("VALIDATION ERROR"));
				return;
			}
			se = getOrCreateEngine(strScope);
		}
		// Otherwise if the scope is explicitly "Private" then use the script instance name as the engine name.
		else if (scope == DSE::PrivateInstance) {
			se = getOrCreateEngine(dvName);
		}
		// Otherwise use the Shared instance.
		else {
			se = ScriptEngine::instance();
		}
		ds->setEngine(se);

		const QString &stateParam = dataMap.value("state");
		// preserve BC with < v1.2 actions which all create a state (except SS types but thsoe are excluded, above).
		if (stateParam.isEmpty()) {
			if (type == TPClientQt::MessageType::connectorChange) {
				// Connectors do not have a "save" option; Do not modify any previosly-set defaults but make sure a State is created.
				if (ds->defaultType() == DSE::StateDefaultType::NoStateUsed)
					ds->setDefaultType(DSE::StateDefaultType::FixedValueDefault);
			}
			else {
				// The "save" option only dictated if the instance was saved to settings; Interpret that into persistence and saved default properties.
				DSE::StateDefaultType defType = stringToDefaultType(dataMap.value("save"));
				ds->setPersistence(defType > DSE::StateDefaultType::NoStateUsed ? DSE::PersistenceType::PersistSave : DSE::PersistenceType::PersistSession);
				if (defType == DSE::StateDefaultType::NoStateUsed)
					defType = DSE::StateDefaultType::FixedValueDefault;
				ds->setDefaultTypeValue(defType, dataMap.value("default").toUtf8());
			}
		}
		// handle new action types for v1.2+
		else {
			ds->setPersistence(stringToPersistenceType(dataMap.value("save")));

			if (type == TPClientQt::MessageType::connectorChange) {
				// Connectors do not have a default type parameter, only State yes/no; Do not modify any previosly-set defaults.
				DSE::StateDefaultType defType = stateParam.at(0) == 'N' ? DSE::StateDefaultType::NoStateUsed : DSE::StateDefaultType::FixedValueDefault;
				if (defType == DSE::StateDefaultType::FixedValueDefault && ds->defaultType() == DSE::StateDefaultType::NoStateUsed)
					ds->setDefaultType(DSE::StateDefaultType::FixedValueDefault);
				else if (defType == DSE::StateDefaultType::NoStateUsed && ds->defaultType() > DSE::StateDefaultType::NoStateUsed)
					ds->setDefaultType(DSE::StateDefaultType::NoStateUsed);
			}
			else {
				ds->setDefaultTypeValue(stringToStateType(stateParam), dataMap.value("default").toUtf8());
			}
		}
	}  // act != AID_Update

	QString expression = dataMap.value("expr");
	if (connectorValue > -1) {
		expression.replace(QLatin1String("${connector_value}"), QString::number(connectorValue), Qt::CaseInsensitive);
	}
	bool ok = false;
	switch (act)
	{
		case AID_Eval:
			ok = ds->setExpressionProperties(expression);
			break;
		case AID_Load:
			ok = ds->setScriptProperties(dataMap.value("file").trimmed(), expression);
			break;
		case AID_Import:
			ok = ds->setModuleProperties(dataMap.value("file").trimmed(), dataMap.value("alias").trimmed(), expression);
			break;
		case AID_Update:
			ok = ds->setExpression(expression);
			break;
	}
	if (!ok) {
		raiseScriptError(ds->name, tr("ValidationError: %1").arg(ds->lastError), tr("VALIDATION ERROR"));
		if (ds->isTemporary())
			removeInstanceLater(ds);
		return;
	}

	// Set pressed state if action is being used in "On-Hold" area.
	if (type == TPClientQt::MessageType::down)
		ds->setPressedState(true);

	QMetaObject::invokeMethod(ds, "evaluate", Qt::QueuedConnection);
}

void Plugin::pluginAction(TPClientQt::MessageType type, int act, const QMap<QString, QString> &dataMap, qint32 connectorValue)
{
	int subAct = 0;
	if (act != AID_Shutdown) {
		subAct = tokenFromName(dataMap.value("action").toUtf8());
		if (subAct == AT_Unknown) {
			qCCritical(lcPlugin) << "Unknown Command action:" << dataMap.value("action");
			return;
		}
	}

	switch (act) {
		case AID_InstanceControl:
			instanceControlAction(subAct, dataMap);
			break;
		case AID_RepeatRate:
			setActionRepeatRate(type, subAct, dataMap, connectorValue);
			break;
		case AID_Shutdown:
			qCInfo(lcPlugin()) << "Got shutdown command, exiting.";
			exit();
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

		case CA_SaveInstance:
			if (type)
				saveAllInstances();
			else if (saveScriptInstance(dvName))
				qCInfo(lcPlugin) << "Saved script instance" << dvName << "to persistent storage.";
			else
				qCCritical(lcPlugin) << "Script instance not found for name:" << dvName;
			return;

		case CA_LoadInstance:
			if (type)
				loadAllInstances();
			else if (loadScriptInstance(dvName))
				qCInfo(lcPlugin) << "Loaded script instance" << dvName << "from persistent storage.";
			else
				qCCritical(lcPlugin) << "Script instance not found for name:" << dvName;
			return;

		case CA_DelSavedInstance: {
			if (type) {
				QSettings().remove(SETTINGS_GROUP_SCRIPTS);
				qCInfo(lcPlugin) << "Removed all saved script instances!";
			}
			else if (QSettings().contains(SETTINGS_GROUP_SCRIPTS "/" + dvName)) {
				QSettings().remove(SETTINGS_GROUP_SCRIPTS "/" + dvName);
				qCInfo(lcPlugin) << "Removed saved data for script instance" << dvName << ".";
			}
			else {
				qCWarning(lcPlugin) << "No saved data found for instance:" << dvName;
			}
			return;
		}

		// Deprecated in v1.2; remove.
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
	QByteArray instName = dataMap.value("name").toUtf8();
	if ((param != AT_Rate && param != AT_Delay && param != AT_RateDelay) || instName.isEmpty()) {
		qCCritical(lcPlugin) << "Invalid properties in action" << tokenToName(act) << "Repeat" << dataMap.value("param") << "for" << instName;
		return;
	}

	DSE::sharedInstance->cancelRepeatingAction(/*DSE::ACT_ADJ_REPEAT*/);
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

	if (instName == tokenToName(AT_Default))
		instName.clear();
	quint8 prop = param == AT_Rate ? DSE::RepeatRateProperty : (param == AT_Delay ? DSE::RepeatDelayProperty : DSE::AllRepeatProperties);
	quint8 repAct = act == AT_Increment ? DSE::Increment : (act == AT_Decrement ? DSE::Decrement : DSE::SetAbsolute);
	Q_EMIT setActionRepeatProperty(prop, repAct, value, instName, type == TPClientQt::MessageType::down);
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
			continue;
		}
		if (!g_startupComplete && next.key().startsWith(tokenToName(ST_SettingsVersion))) {
			if (next.value().toString().isEmpty())
				qCInfo(lcPlugin) << "No saved Plugin Settings version; first-time use of plugin.";
			else
				qCInfo(lcPlugin).noquote().nospace() << "Saved Touch Portal Plugin Settings v" << next.value().toString() << "; Current v" << APP_VERSION;
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
		actIdStr = act < STRING_TOKENS_COUNT ? QByteArray(tokenStrings()[act]) : tokenToName(act, actIdStr);

	ConnectorRecord cr;
	cr.actionType = actIdStr;
	cr.connectorId = propList.at(0).split('_').last().toUtf8();
	cr.shortId = msg.value(QLatin1String("shortId")).toString().toUtf8();

	QList<QStringView>::const_iterator it = propList.constBegin() + 1;
	for (; it != propList.constEnd(); ++it) {
		const auto dataPair = it->split('=');
		const QByteArray id = dataPair.first().split('.').last().toUtf8();
		const QStringView value = dataPair.last();
		if (cr.instanceName.isEmpty() && !id.compare(tokenStrings()[ADID_InstanceName]))
			cr.instanceName = value.toUtf8();
		else if (cr.instanceType == DSE::UnknownInstanceType && !id.compare(tokenStrings()[ADID_EngineScope]))
			cr.instanceType = stringToScope(value.toUtf8(), true);
		else if (cr.expression.isEmpty() && !id.compare(tokenStrings()[ADID_Expression]))
			cr.expression = value.toUtf8();
		else if (cr.file.isEmpty() && !id.compare(tokenStrings()[ADID_ScriptFile]))
			cr.file = value.toUtf8();
		else if (cr.alias.isEmpty() && !id.compare(tokenStrings()[ADID_ModuleAlias]))
			cr.alias = value.toUtf8();
		else
			cr.otherData.insert(id, value.toString());
	}

	switch (act) {
		case AID_Eval:
			cr.inputType = DSE::ExpressionInput;
			break;
		case AID_Load:
			cr.inputType = DSE::ScriptInput;
			break;
		case AID_Import:
			cr.inputType = DSE::ModuleInput;
			break;
		case AID_Update:
			if (DynamicScript *ds = DSE::instance(cr.instanceName))
				cr.inputType = ds->inputType();
			break;
	}

	ConnectorData::instance()->insert(cr);
}

#include "moc_Plugin.cpp"
