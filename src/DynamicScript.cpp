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

constexpr static uint32_t SAVED_PROPERTIES_VERSION = 3;
constexpr static int MUTEX_LOCK_TIMEOUT_MS = 250;

DynamicScript::DynamicScript(const QByteArray &name, QObject *p) :
  QObject(p),
  name{name},
  tpStateId(QByteArrayLiteral(PLUGIN_STATE_ID_PREFIX) + name),
  createState(false)
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

void DynamicScript::moveToMainThread()
{
	if (this->thread() == qApp->thread())
		return;
	Utils::runOnThreadSync(this->thread(), [&]() {
		moveToThread(qApp->thread());
	});
}

bool DynamicScript::setExpressionProperties(const QString &expr)
{
	QWriteLocker lock(&m_mutex);
	m_inputType = DSE::ScriptInputType::ExpressionInput;
	bool ok = setExpr(expr);
	return !(m_state.setFlag(State::PropertyErrorState, !ok) & State::CriticalErrorState);
}

bool DynamicScript::setScriptProperties(const QString &file, const QString &expr)
{
	QWriteLocker lock(&m_mutex);
	m_inputType = DSE::ScriptInputType::ScriptInput;
	bool ok = setFile(file);
	if (ok)
		setExpr(expr);  // expression is not required
	m_state.setFlag(State::PropertyErrorState, !ok);
	return !(m_state.setFlag(State::PropertyErrorState, !ok) & State::CriticalErrorState);
}

bool DynamicScript::setModuleProperties(const QString &file, const QString &alias, const QString &expr)
{
	QWriteLocker lock(&m_mutex);
	m_inputType = DSE::ScriptInputType::ModuleInput;
	bool ok = setFile(file);
	if (ok) {
		m_moduleAlias = alias.isEmpty() ? QStringLiteral("M") : alias;
		setExpr(expr);  // expression is not required
	}
	return !(m_state.setFlag(State::PropertyErrorState, !ok) & State::CriticalErrorState);
}

bool DynamicScript::setProperties(DSE::ScriptInputType type, const QString &expr, const QString &file, const QString &alias, bool ignoreErrors)
{
	switch(type) {
		case DSE::ScriptInputType::ExpressionInput:
			return setExpressionProperties(expr) || ignoreErrors;
		case DSE::ScriptInputType::ScriptInput:
			return setScriptProperties(file, expr) || ignoreErrors;
		case DSE::ScriptInputType::ModuleInput:
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
		if (!m_engine->isSharedInstance())
			m_engine->disconnectNamedScriptInstance(this);
	}
	m_engine = se;

	if (se) {
		if (this->thread() != se->thread()) {
			if (this->thread() == QThread::currentThread())
				moveToThread(se->thread());
			else
				Utils::runOnThread(this->thread(), [this]() { this->moveToThread(m_engine->thread()); });
		}
		if (createState && !se->isSharedInstance()) {
			// An unqualified stateUpdate command for this particular instance must add our actual state ID before sending.
			m_engine->connectNamedScriptInstance(this);
		}
		m_scope = se->instanceType();
		m_engineName = se->name();
	}
	else {
		m_scope = DSE::EngineInstanceType::UnknownInstanceType;
		m_engineName.clear();
	}
	return !(m_state.setFlag(State::UninitializedState, !se) & State::CriticalErrorState);
}

void DynamicScript::setCreateState(bool create)
{
	if (createState != create) {
		createState = create;
		if (!create) {
			removeTpState();
		}
		else if (m_engine && !m_engine->isSharedInstance()) {
			// An unqualified stateUpdate command for this particular instance must add our actual state ID before sending.
			m_engine->connectNamedScriptInstance(this);
		}
	}
}

void DynamicScript::createTpState()
{
	m_state.setFlag(TpStateCreatedFlag, true);
	// FIXME: TP doesn't fire state change events based on the default value.
	Q_EMIT Plugin::instance->tpStateCreate(tpStateId, stateCategory(), stateName(), QByteArray() /*defaultValue*/);
	qCDebug(lcPlugin) << "Created instance State" << tpStateId << "in" << stateCategory();
}

void DynamicScript::removeTpState()
{
	if (m_state.testFlags(TpStateCreatedFlag)) {
		m_state.setFlag(TpStateCreatedFlag, false);
		Q_EMIT Plugin::instance->tpStateRemove(tpStateId);
		qCDebug(lcPlugin) << "Removed instance State" << tpStateId;
	}
}

void DynamicScript::setDefaultTypeValue(DSE::ScriptDefaultType defType, const QByteArray &def)
{
	QWriteLocker lock(&m_mutex);
	m_defaultType = defType;
	m_defaultValue = def;
}

void DynamicScript::setSingleShot(bool ss)
{
	if (ss == singleShot)
		return;
	singleShot = ss;
	if (ss)
		connect(this, &DynamicScript::finished, Plugin::instance, &Plugin::onDsFinished);
	else
		disconnect(this, &DynamicScript::finished, Plugin::instance, &Plugin::onDsFinished);
}

QByteArray DynamicScript::serialize() const
{
	QByteArray ba;
	QDataStream ds(&ba, QIODevice::WriteOnly);
	ds << SAVED_PROPERTIES_VERSION << int(m_engine ? m_engine->instanceType() : m_scope) << (int)m_inputType << m_expr << m_file << m_moduleAlias
	   << m_defaultValue << (int)m_defaultType << m_repeatDelay << m_repeatRate
	   << createState << (m_engine ? m_engine->name() : m_engineName)
	   << tpStateCategory << tpStateName;
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

	int scope, inpType, defType, repDelay, repRate;
	QString expr, file, alias;
	QByteArray deflt;
	ds >> scope >> inpType >> expr >> file >> alias >> deflt >> defType;

	m_scope = (DSE::EngineInstanceType)scope;

	bool cs = true;
	// DSE::ScriptInputType enum values changed in v2.
	if (version == 1)
		++inpType;
	else if (version > 2) {
		ds >> repDelay >> repRate >> cs >> m_engineName >> tpStateCategory >> tpStateName;
		m_repeatDelay = repDelay;
		m_repeatRate = repRate;
	}
	else if (m_scope == DSE::EngineInstanceType::PrivateInstance && m_engineName.isEmpty()) {
		m_engineName = name;
	}

	setDefaultTypeValue((DSE::ScriptDefaultType)defType, deflt);
	setCreateState(cs);

	//qCDebug(lcPlugin) << name << (DSE::ScriptInputType)type << instType << file << deflt << (DSE::ScriptDefaultType)defType;
	if (!setProperties((DSE::ScriptInputType)inpType, expr, file, alias, true)) {
		qCCritical(lcPlugin) << "Cannot restore settings for" << name << "because the saved input type:" << inpType << "is unknown";
		return false;
	}
	return !m_state.testAnyFlags(State::CriticalErrorState);
}



// private setters, no mutex

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

void DynamicScript::setupRepeatTimer(bool create)
{
	if (create && !m_repeatTim) {
		m_repeatTim = new QTimer();
		m_repeatTim->setSingleShot(true);
		connect(m_repeatTim, &QTimer::timeout, this, &DynamicScript::repeatEvaluate);
	}
	else if (!create && m_repeatTim) {
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
}

void DynamicScript::setRepeating(bool repeat)
{
	if (m_repeating == repeat || (repeat && m_state.testAnyFlags(State::CriticalErrorState)))
		return;
	m_repeating = repeat;
	if (repeat) {
		m_repeatCount = 0;
		m_activeRepeatRate = -1;
	}
	else {
		setupRepeatTimer(false);
		Q_EMIT finished();
	}
}

void DynamicScript::repeatEvaluate()
{
	m_activeRepeatRate = -1;
	if (m_repeating)
		evaluate();
}

void DynamicScript::evaluate()
{
	if (m_state.testAnyFlags(State::CriticalErrorState))
		return;

	if (!m_mutex.tryLockForRead(MUTEX_LOCK_TIMEOUT_MS)) {
		qCDebug(lcPlugin) << "Mutex lock timeout for" << name;
		return;
	}

	m_state.setFlag(State::EvaluatingNowState, true);
	QJSValue res;
	switch (m_inputType) {
		case DSE::ScriptInputType::ExpressionInput:
			res = m_engine->expressionValue(m_expr, name);
			break;

		case DSE::ScriptInputType::ScriptInput:
			res = m_engine->scriptValue(m_file, m_expr, name);
			break;

		case DSE::ScriptInputType::ModuleInput:
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
		setRepeating(false);
	}
	else if (!res.isUndefined() && !res.isNull()) {
		stateUpdate(res.toString().toUtf8());
	}

	m_mutex.unlock();

	if (m_repeating) {
		const int delay = m_repeatCount > 0 ? effectiveRepeatRate() : effectiveRepeatDelay();
		if (delay >= 50) {
			++m_repeatCount;
			setupRepeatTimer();
			m_repeatTim->start(delay);
			return;
		}
	}

	Q_EMIT finished();
}

void DynamicScript::evaluateDefault()
{
	//qCDebug(lcPlugin) << "DynamicScript instance" << name << "default type:" << m_defaultType << m_expr << m_defaultValue;
	switch (m_defaultType) {
		case DSE::ScriptDefaultType::MainExprDefault:
			evaluate();
			return;

		case DSE::ScriptDefaultType::CustomExprDefault:
			if (!m_defaultValue.isEmpty()) {
				const QString saveExpr = m_expr;
				m_expr = m_defaultValue;
				evaluate();
				m_expr = saveExpr;
			}
			return;

		case DSE::ScriptDefaultType::FixedValueDefault:
			stateUpdate(m_defaultValue);
			return;

		default:
			return;
	}
}

#include "moc_DynamicScript.cpp"
