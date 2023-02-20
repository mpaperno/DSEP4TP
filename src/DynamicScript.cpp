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

#include "DynamicScript.h"

#include "common.h"
#include "Plugin.h"
#include "ScriptEngine.h"
#include "utils.h"

using namespace DseNS;

constexpr static uint32_t SAVED_PROPERTIES_VERSION = 3;
constexpr static int MUTEX_LOCK_TIMEOUT_MS = 250;

DynamicScript::DynamicScript(const QByteArray &name, QObject *p) :
  QObject(p),
  name{name},
  tpStateId(DSE::valueStatePrefix + name)
{
	setObjectName("DynamicScript: " + name);
	QJSEngine::setObjectOwnership(this, QJSEngine::CppOwnership);
	//qCDebug(lcPlugin) << name << "Created";
}

DynamicScript::~DynamicScript() {
	//moveToMainThread();
	QWriteLocker lock(&m_mutex);
	setupRepeatTimer(false);
	//qCDebug(lcPlugin) << name << "Destroyed";
}

//void DynamicScript::moveToMainThread()
//{
//	if (this->thread() == qApp->thread())
//		return;
//	Utils::runOnThreadSync(this->thread(), [&]() {
//		moveToThread(qApp->thread());
//	});
//}

bool DynamicScript::setExpressionProperties(const QString &expr)
{
	QWriteLocker lock(&m_mutex);
	m_inputType = ScriptInputType::ExpressionInput;
	bool ok = setExpr(expr);
	return !(m_state.setFlag(State::PropertyErrorState, !ok) & State::CriticalErrorState);
}

bool DynamicScript::setScriptProperties(const QString &file, const QString &expr)
{
	QWriteLocker lock(&m_mutex);
	m_inputType = ScriptInputType::ScriptInput;
	bool ok = setFile(file);
	if (ok)
		setExpr(expr);  // expression is not required
	return !(m_state.setFlag(State::PropertyErrorState, !ok) & State::CriticalErrorState);
}

bool DynamicScript::setModuleProperties(const QString &file, const QString &alias, const QString &expr)
{
	QWriteLocker lock(&m_mutex);
	m_inputType = ScriptInputType::ModuleInput;
	bool ok = setFile(file);
	if (ok) {
		m_moduleAlias = alias.isEmpty() ? QStringLiteral("M") : alias;
		setExpr(expr);  // expression is not required
	}
	return !(m_state.setFlag(State::PropertyErrorState, !ok) & State::CriticalErrorState);
}

bool DynamicScript::setProperties(ScriptInputType type, const QString &expr, const QString &file, const QString &alias, bool ignoreErrors)
{
	switch(type) {
		case ScriptInputType::ExpressionInput:
			return setExpressionProperties(expr) || ignoreErrors;
		case ScriptInputType::ScriptInput:
			return setScriptProperties(file, expr) || ignoreErrors;
		case ScriptInputType::ModuleInput:
			return setModuleProperties(file, alias, expr) || ignoreErrors;
		default:
			return false;
	}
}

bool DynamicScript::setExpression(const QString &expr)
{
	QWriteLocker lock(&m_mutex);
	bool ok = setExpr(expr);
	return !(m_state.setFlag(State::PropertyErrorState, !ok) & State::CriticalErrorState);
}

bool DynamicScript::setEngine(ScriptEngine *se)
{
	QWriteLocker lock(&m_mutex);
	//qCDebug(lcPlugin) << m_engine << se << (m_engine == se) << m_state << m_state.testAnyFlags(State::CriticalErrorState);
	if (m_engine == se)
		return !m_state.testAnyFlags(State::CriticalErrorState);

	if (m_engine) {
		qCWarning(lcPlugin) << "Switching engine instances could lead to unexpected results and plugin instability.";
		m_engine->clearInstanceData(this);
		disconnect(m_engine, nullptr, this, nullptr);
		serializeStoredData();
	}
	m_engine = se;

	if (se) {
		if (this->thread() != se->thread()) {
			if (this->thread() == QThread::currentThread())
				moveToThread(se->thread());
			else
				Utils::runOnThread(this->thread(), [this]() { this->moveToThread(m_engine->thread()); });
		}
		if (createState() && !se->isSharedInstance()) {
			// An unqualified stateUpdate command for this particular instance must add our actual state ID before sending.
			m_engine->connectNamedScriptInstance(this);
		}
		m_scope = se->instanceType();
		m_engineName = se->name();

		connect(m_engine, &ScriptEngine::engineAboutToReset, this, &DynamicScript::serializeStoredData);
	}
	else {
		m_scope = EngineInstanceType::UnknownInstanceType;
		m_engineName.clear();
	}
	return !(m_state.setFlag(State::UninitializedState, !se) & State::CriticalErrorState);
}

void DynamicScript::setDefaultType(SavedDefaultType type)
{
	if (m_defaultType == type)
		return;
	QWriteLocker lock(&m_mutex);
	m_defaultType = type;
}

void DynamicScript::setActivation(ActivationBehaviors behavior)
{
	if (m_activation == behavior)
		return;

	QWriteLocker lock(&m_mutex);
	m_activation = behavior;
}

void DynamicScript::setPressedState(bool isPressed)
{
	QWriteLocker lock(&m_mutex);
	setPressed(isPressed);
}

void DynamicScript::setPersistence(PersistenceType newPersist)
{
	if (m_persist == newPersist)
		return;

	QWriteLocker lock(&m_mutex);
	m_persist = newPersist;
	if (newPersist == PersistenceType::PersistTemporary)
		connect(this, &DynamicScript::finished, Plugin::instance, &Plugin::onDsFinished, Qt::UniqueConnection);
	else
		disconnect(this, &DynamicScript::finished, Plugin::instance, &Plugin::onDsFinished);
}

QJSValue &DynamicScript::dataStorage()
{
	if (!m_storedData.isObject()) {
		if (m_engine)
			m_storedData = m_engine->engine()->toScriptValue(m_storedDataVar);
		else
			m_storedData = qvariant_cast<QJSValue>(m_storedDataVar.toVariantMap());
	}
	return m_storedData;
}

QByteArray DynamicScript::serialize() const
{
	QByteArray ba;
	QDataStream ds(&ba, QIODevice::WriteOnly);
	ds << SAVED_PROPERTIES_VERSION << int(m_engine ? m_engine->instanceType() : m_scope) << (int)m_inputType << m_expr << m_file << m_moduleAlias
	   << m_defaultValue << (int)m_defaultType << m_createState
	   << m_repeatDelay << m_repeatRate << (m_engine ? m_engine->name() : m_engineName) << tpStateCategory << tpStateName
	   << (int)m_persist << (int)m_activation;
	if (m_storedData.isObject()) {
		if (m_engine)
			ds << m_engine->engine()->fromScriptValue<QJsonObject>(m_storedData);
		else
			ds << QJsonObject::fromVariantMap(m_storedData.toVariant().toMap());
	}
	else {
		ds << m_storedDataVar;
	}
	return ba;
}

bool DynamicScript::deserialize(const QByteArray &data)
{
	uint32_t version;
	QDataStream ds(data);
	ds >> version;
	if (!version || version > SAVED_PROPERTIES_VERSION) {
		qCCritical(lcPlugin) << "Cannot restore settings for" << name << "because settings version" << version << "is invalid or is newer than current version" << SAVED_PROPERTIES_VERSION;
		return false;
	}

	int scope, inpType, defType, repDelay, repRate,
	    persist = PersistenceType::PersistSave,
	    act = ActivationBehavior::OnRelease;
	QString expr, file, alias;
	QByteArray deflt;
	ds >> scope >> inpType >> expr >> file >> alias >> m_defaultValue >> defType;

	m_scope = (EngineInstanceType)scope;
	m_defaultType = (SavedDefaultType)defType;

	bool createState = true;
	// ScriptInputType enum values changed in v2.
	if (version == 1) {
		++inpType;
	}
	if (version > 2) {
		ds >> createState >> repDelay >> repRate >> m_engineName >> tpStateCategory >> tpStateName >> persist >> act >> m_storedDataVar;
		m_repeatDelay = repDelay;
		m_repeatRate = repRate;
	}
	else if (m_scope == EngineInstanceType::PrivateInstance && m_engineName.isEmpty()) {
		m_engineName = name;
	}

	setPersistence((PersistenceType)persist);
	setActivation((ActivationBehaviors)act);
	setCreateState(createState);

	//qCDebug(lcPlugin) << name << (ScriptInputType)type << instType << file << deflt << (ScriptDefaultType)defType;
	if (!setProperties((ScriptInputType)inpType, expr, file, alias, true)) {
		qCCritical(lcPlugin) << "Cannot restore settings for" << name << "because the saved input type:" << inpType << "is unknown";
		return false;
	}
	return !m_state.testAnyFlags(State::ConfigErrorState);
}

// -----------------
// Private setters, no mutex

bool DynamicScript::setExpr(const QString &expr)
{
	if (expr.isEmpty()) {
		lastError = tr("Expression is empty.");
		return false;
	}
	m_expr = expr; // QString(expr).replace("\\", "\\\\");
	return true;
}

bool DynamicScript::setFile(const QString &file)
{
	if (file.isEmpty()) {
		lastError = tr("File path is empty.");
		return false;
	}
	if (m_state.testFlag(State::FileLoadErrorState) || m_originalFile != file) {
		QFileInfo fi(DSE::resolveFile(file));
		if (!fi.exists()) {
			//qCCritical(lcPlugin) << "File" << file << "not found for item" << name;
			lastError = QStringLiteral("File not found: '") + file + '\'';
			m_state.setFlag(State::FileLoadErrorState);
			return false;
		}
		m_file = fi.filePath();
		m_scriptLastMod = fi.lastModified();
		m_originalFile = file;
		m_state.setFlag(State::FileLoadErrorState, false);
	}
	return true;
}

void DynamicScript::setCreateState(bool create)
{
	if (m_createState == create)
		return;

	m_createState = create;
	if (!create) {
		removeTpState();
	}
	else if (m_engine && !m_engine->isSharedInstance()) {
		// An unqualified stateUpdate command for this particular instance must add our actual state ID before sending.
		m_engine->connectNamedScriptInstance(this);
	}
}

void DynamicScript::setPressed(bool isPressed)
{
	if (m_state.testFlags(State::PressedState) == isPressed)
		return;

	m_state.setFlag(State::PressedState, isPressed);
	if (!isPressed)
		setRepeating(false);

	Q_EMIT pressedStateChanged(isPressed);
}

void DynamicScript::serializeStoredData()
{
	if (m_engine)
		m_storedDataVar = m_engine->engine()->fromScriptValue<QJsonObject>(m_storedData);
	else if (m_storedData.isObject())
		m_storedDataVar = QJsonObject::fromVariantMap(m_storedData.toVariant(QJSValue::ConvertJSObjects).toMap());
	else if (!m_storedDataVar.isEmpty())
		m_storedDataVar = QJsonObject();
	m_storedData = QJSValue();
}

void DynamicScript::setupRepeatTimer(bool create)
{
	if (create == !!m_repeatTim)
		return;

	if (create) {
		m_repeatTim = new QTimer();
		m_repeatTim->setSingleShot(true);
		connect(m_repeatTim, &QTimer::timeout, this, &DynamicScript::repeatEvaluate);
		return;
	}

	disconnect(m_repeatTim, &QTimer::timeout, this, &DynamicScript::repeatEvaluate);
	if (m_repeatTim->thread() != QThread::currentThread()) {
		Utils::runOnThreadSync(m_repeatTim->thread(), [=]() {
			m_repeatTim->stop();
			m_repeatTim->deleteLater();
			m_repeatTim = nullptr;
		});
	}
	else {
		m_repeatTim->stop();
		delete m_repeatTim;
		m_repeatTim = nullptr;
	}
}

void DynamicScript::repeatEvaluate()
{
	m_activeRepeatRate = -1;
	if (isRepeating()) {
		//qCDebug(lcPlugin) << "Repeating" << name << m_repeatCount << m_maxRepeatCount;
		if (m_maxRepeatCount < 0 || m_repeatCount < m_maxRepeatCount) {
			++m_repeatCount;
			evaluate();
			Q_EMIT repeatCountChanged(m_repeatCount);
			return;
		}
		setRepeating(false);
	}
	setupRepeatTimer(false);
}

bool DynamicScript::scheduleRepeatIfNeeded()
{
	if (m_activation.testFlags(ActivationBehavior::RepeatOnHold) && (m_maxRepeatCount < 0 || m_repeatCount < m_maxRepeatCount)) {
		const int delay = m_repeatCount > 0 ? effectiveRepeatRate() : effectiveRepeatDelay();
		if (delay >= 50) {
			setupRepeatTimer();
			m_repeatTim->start(delay);
			setRepeating(true);
			return true;
		}
	}
	return false;
}

void DynamicScript::evaluate()
{
	if (m_state.testAnyFlags(State::CriticalErrorState) || m_activation == ActivationBehavior::NoActivation)
		return;

	if (m_state.testFlags(State::HoldReleasedState)) {
		m_state.setFlag(State::HoldReleasedState, false);
		if (!m_activation.testFlags(ActivationBehavior::OnRelease)) {
			Q_EMIT finished();
			return;
		}
	}
	else if (isPressed() && !m_activation.testFlags(ActivationBehavior::OnPress) && !m_state.testFlags(State::RepeatingState)) {
		scheduleRepeatIfNeeded();
		return;
	}

	if (!m_mutex.tryLockForRead(MUTEX_LOCK_TIMEOUT_MS)) {
		qCDebug(lcPlugin) << "Mutex lock timeout for" << name;
		return;
	}

	m_state.setFlag(State::EvaluatingNowState, true);
	QJSValue res;
	switch (m_inputType) {
		case ScriptInputType::ExpressionInput:
			res = m_engine->expressionValue(m_expr, name);
			break;

		case ScriptInputType::ScriptInput:
			res = m_engine->scriptValue(m_file, m_expr, name);
			break;

		case ScriptInputType::ModuleInput:
			res = m_engine->moduleValue(m_file, m_moduleAlias, m_expr, name);
			break;

		default:
			m_mutex.unlock();
			return;
	}
	m_state.setFlag(State::EvaluatingNowState, false);

	m_state.setFlag(State::ScriptErrorState, res.isError());
	if (m_state.testFlag(State::ScriptErrorState)) {
		Q_EMIT scriptError(JSError(res));
		setPressed(false);
	}
	else if (!res.isUndefined() && !res.isNull()) {
		stateUpdate(res.toString().toUtf8());
	}

	m_mutex.unlock();

	if (isPressed() && scheduleRepeatIfNeeded())
		return;

	Q_EMIT finished();
}

void DynamicScript::evaluateDefault()
{
	// FIXME: TP v3.1 doesn't fire state change events based on the default value; v3.2 might.
	// As a workaround for now, the states are created with a blank default value and then the _actual_ default is sent
	// as a state update.
	stateUpdate(getDefaultValue());
}

QByteArray DynamicScript::getDefaultValue()
{
	if (m_state.testFlags(State::UninitializedState))
		return QByteArray();

	QReadLocker lock(&m_mutex);
	const QString expr = m_defaultType == SavedDefaultType::CustomExprDefault ? m_defaultValue : m_defaultType == SavedDefaultType::LastExprDefault ? m_expr : QByteArray();
	QJSValue res;
	switch (m_inputType) {
		case ScriptInputType::ExpressionInput:
			if (!expr.isEmpty())
				res = m_engine->expressionValue(expr, name);
			break;

		case ScriptInputType::ScriptInput:
			if (!m_state.testFlags(State::FileLoadErrorState))
				res = m_engine->scriptValue(m_file, expr, name);
			break;

		case ScriptInputType::ModuleInput:
			if (!m_state.testFlags(State::FileLoadErrorState))
				res = m_engine->moduleValue(m_file, m_moduleAlias, expr, name);
			break;

		default:
			break;
	}
	lock.unlock();

	if (res.isError()) {
		m_state.setFlag(State::ScriptErrorState, true);
		Q_EMIT scriptError(JSError(res));
	}

	if (m_defaultType == SavedDefaultType::FixedValueDefault)
		return m_defaultValue;

	if (!res.isUndefined() && !res.isNull() && !res.isError())
		return res.toString().toUtf8();

	return QByteArray();
}

#include "moc_DynamicScript.cpp"
