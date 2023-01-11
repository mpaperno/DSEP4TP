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

DynamicScript::DynamicScript(const QByteArray &name, QObject *p) :
  QObject(p),
  name{name},
  tpStateName(QByteArrayLiteral(PLUGIN_STATE_ID_PREFIX) + name)
{
	setObjectName("DSE.DynamicScript");
	QJSEngine::setObjectOwnership(this, QJSEngine::CppOwnership);
	// These connections must be established for all instance types. Others may be made later when the DSE::EngineInstanceType is set.
	connect(this, &DynamicScript::scriptError, Plugin::instance, &Plugin::onDsScriptError, Qt::QueuedConnection);
	// Direct connection to socket where state ID is already fully qualified;
	connect(this, &DynamicScript::dataReady, Plugin::instance->tpClient(), qOverload<const QByteArray&, const QByteArray&>(&TPClientQt::stateUpdate), Qt::QueuedConnection);
	//connect(this, &DynamicScript::dataReady, p, &Plugin::tpStateUpdate);  // alternate
	//qCDebug(lcPlugin) << name << "Created";
}

DynamicScript::~DynamicScript() {
	if (m_scope == DSE::EngineInstanceType::Private && m_engine)
		delete m_engine; //->deleteLater();
	moveToMainThread();
	//qCDebug(lcPlugin) << name << "Destroyed";
}

bool DynamicScript::setCommonProperties(DSE::ScriptInputType type, DSE::EngineInstanceType scope, DSE::ScriptDefaultType save, const QByteArray &def)
{
	QWriteLocker lock(&m_mutex);
	bool ok = setTypeScope(type, scope);
	if (ok)
		setSaveAndDefault(save, def);
	return !(m_state.setFlag(State::PropertyErrorState, !ok) & State::CriticalErrorState);
}

bool DynamicScript::setExpressionProperties(DSE::EngineInstanceType scope, const QString &expr, DSE::ScriptDefaultType save, const QByteArray &def)
{
	if (!setCommonProperties(DSE::ScriptInputType::Expression, scope, save, def))
		return false;
	QWriteLocker lock(&m_mutex);
	bool ok = setExpr(expr);
	if (ok)
		setSaveAndDefault(save, def);
	return !(m_state.setFlag(State::PropertyErrorState, !ok) & State::CriticalErrorState);
}

bool DynamicScript::setScriptProperties(DSE::EngineInstanceType scope, const QString &file, const QString &expr, DSE::ScriptDefaultType save, const QByteArray &def)
{
	if (!setCommonProperties(DSE::ScriptInputType::Script, scope, save, def))
		return false;
	QWriteLocker lock(&m_mutex);
	bool ok = setFile(file);
	if (ok) {
		setExpr(expr);  // expression is not required
		setSaveAndDefault(save, def);
	}
	m_state.setFlag(State::PropertyErrorState, !ok);
	return !(m_state.setFlag(State::PropertyErrorState, !ok) & State::CriticalErrorState);
}

bool DynamicScript::setModuleProperties(DSE::EngineInstanceType scope, const QString &file, const QString &alias, const QString &expr, DSE::ScriptDefaultType save, const QByteArray &def)
{
	if (!setCommonProperties(DSE::ScriptInputType::Module, scope, save, def))
		return false;
	QWriteLocker lock(&m_mutex);
	bool ok = setFile(file);
	if (ok) {
		m_moduleAlias = alias.isEmpty() ? QStringLiteral("M") : alias;
		setExpr(expr);  // expression is not required
		setSaveAndDefault(save, def);
	}
	return !(m_state.setFlag(State::PropertyErrorState, !ok) & State::CriticalErrorState);
}

bool DynamicScript::setProperties(DSE::ScriptInputType type, DSE::EngineInstanceType scope, const QString &expr, const QString &file, const QString &alias, DSE::ScriptDefaultType save, const QByteArray &def)
{
	switch(type) {
		case DSE::ScriptInputType::Expression:
			return setExpressionProperties(scope, expr, save, def);
		case DSE::ScriptInputType::Script:
			return setScriptProperties(scope, file, expr, save, def);
		case DSE::ScriptInputType::Module:
			return setModuleProperties(scope, file, alias, expr, save, def);
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

void DynamicScript::resetEngine()
{
	QWriteLocker lock(&m_mutex);
	if (m_engine) {
		m_engine->reset();
		qCInfo(lcPlugin) << "Private Scripting Engine reset completed for" << name;
	}
}

QByteArray DynamicScript::serialize() const
{
	QByteArray ba;
	QDataStream ds(&ba, QIODevice::WriteOnly);
	ds << SAVED_PROPERTIES_VERSION << (int)m_scope << (int)m_inputType << m_expr << m_file << m_moduleAlias << defaultValue << (int)defaultType;
	return ba;
}

void DynamicScript::deserialize(const QByteArray &data)
{
	uint32_t version;
	QDataStream ds(data);
	ds >> version;
	if (!version || version > SAVED_PROPERTIES_VERSION) {
		qCWarning(lcPlugin) << "Cannot restore settings for" << name << "because settings version" << version << "is invalid or is newer than current version" << SAVED_PROPERTIES_VERSION;
		return;
	}

	int scope, type, defType;
	QString expr, file, alias;
	QByteArray deflt;
	ds >> scope >> type >> expr >> file >> alias >> deflt >> defType;

	// DSE::ScriptInputType enum values changed in v2.
	if (version == 1)
		++type;

	switch ((DSE::ScriptInputType)type) {
		case DSE::ScriptInputType::Expression:
			setExpressionProperties((DSE::EngineInstanceType)scope, expr, (DSE::ScriptDefaultType)defType, deflt);
			break;
		case DSE::ScriptInputType::Script:
			setScriptProperties((DSE::EngineInstanceType)scope, file, expr, (DSE::ScriptDefaultType)defType, deflt);
			break;
		case DSE::ScriptInputType::Module:
			setModuleProperties((DSE::EngineInstanceType)scope, file, alias, expr, (DSE::ScriptDefaultType)defType, deflt);
			break;
		default:
			qCWarning(lcPlugin) << "Cannot restore settings for" << name << "because the saved input type:" << type << "is unknown";
			return;
	}
}

void DynamicScript::moveToMainThread()
{
	if (!m_thread)
		return;
	QMutex m;
	QWaitCondition wc;
	Utils::runOnThread(m_thread, [&]() {
		moveToThread(qApp->thread());
		wc.notify_all();
	});
	m.lock();
	wc.wait(&m);
	m.unlock();
	m_thread->quit();
	m_thread->wait(1000);
	delete m_thread;
	m_thread = nullptr;
}

bool DynamicScript::setScope(DSE::EngineInstanceType newScope)
{
	if (newScope == DSE::EngineInstanceType::Unknown) {
		lastError = tr("Engine Instance Type is Unknown.");
		return false;
	}
	if (newScope == m_scope)
		return true;

	if (m_engine && m_scope == DSE::EngineInstanceType::Private)
		m_engine->deleteLater();

	m_scope = newScope;
	moveToMainThread();

	if (m_scope == DSE::EngineInstanceType::Private) {
		m_thread = new QThread();
		m_thread->setObjectName(name);
		moveToThread(m_thread);
		m_thread->start();
		m_engine = new ScriptEngine(name /*, this*/);
		m_engine->moveToThread(m_thread);
		// Connect to signals from the engine which are emitted by user scripts. For shared scope this is already done in parent's code.
		// Some of these don't apply to "one time" scripts at all since they don't have a state name and can't run background tasks.
		if (!singleShot) {
			// An unqualified stateUpdate command for this particular instance, must add our actual state ID before sending.
			// Connect to notifications about TP events so they can be re-broadcast to scripts in this instance.
			m_engine->connectNamedScriptInstance(this);
			// Instance-specific errors from background tasks.
			connect(m_engine, &ScriptEngine::raiseError, this, &DynamicScript::scriptError);
		}
	}
	else {
		m_engine = ScriptEngine::instance();
	}
	m_state &= ~State::UninitializedState;
	return true;
}

bool DynamicScript::setTypeScope(DSE::ScriptInputType type, DSE::EngineInstanceType scope)
{
	if (type == DSE::ScriptInputType::Unknown) {
		lastError = tr("Input Type is Unknown.");
		return false;
	}
	if (!setScope(scope))
		return false;
	m_inputType = type;
	return true;
}

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

void DynamicScript::setSaveAndDefault(DSE::ScriptDefaultType defType, const QByteArray &def)
{
	defaultType = defType;
	defaultValue = def;
}

void DynamicScript::evaluate()
{
	if (m_state.testFlag(State::CriticalErrorState))
		return;

	if (!m_mutex.tryLockForRead(MUTEX_LOCK_TIMEOUT_MS)) {
		qCDebug(lcPlugin) << "Mutex lock timeout for" << name;
		return;
	}

	m_state.setFlag(State::EvaluatingNowState, true);
	QJSValue res;
	switch (m_inputType) {
		case DSE::ScriptInputType::Expression:
			res = m_engine->expressionValue(m_expr, name);
			break;

		case DSE::ScriptInputType::Script:
			res = m_engine->scriptValue(m_file, m_expr, name);
			break;

		case DSE::ScriptInputType::Module:
			res = m_engine->moduleValue(m_file, m_moduleAlias, m_expr, name);
			break;

		default:
			m_mutex.unlock();
			return;
	}
	m_state.setFlag(State::EvaluatingNowState, false);

	m_state.setFlag(State::ScriptErrorState, res.isError());
	if (m_state.testFlag(State::ScriptErrorState)) {
		Q_EMIT scriptError(res);
	}
	else if (!singleShot && !res.isUndefined() && !res.isNull()) {
		//qCDebug(lcPlugin) << "DynamicScript instance" << name << "sending result:" << res.toString();
		Q_EMIT dataReady(tpStateName, res.toString().toUtf8());
	}

	m_mutex.unlock();

	if (!m_state.testFlag(State::ScriptErrorState) && repeating) {
		const int delay = m_repeatCount > 0 ? effectiveRepeatRate() : effectiveRepeatDelay();
		if (delay >= 50) {
			++m_repeatCount;
			QTimer::singleShot(delay, this, &DynamicScript::repeatEvaluate);
			return;
		}
	}

	Q_EMIT finished();
}

void DynamicScript::evaluateDefault()
{
	//qCDebug(lcPlugin) << "DynamicScript instance" << name << "default type:" << defaultType << m_expr << defaultValue;
	switch (defaultType) {
		case DSE::ScriptDefaultType::MainExpression:
			evaluate();
			return;

		case DSE::ScriptDefaultType::CustomExpression:
			if (!defaultValue.isEmpty()) {
				const QString saveExpr = m_expr;
				m_expr = defaultValue;
				evaluate();
				m_expr = saveExpr;
			}
			return;

		case DSE::ScriptDefaultType::FixedValue:
			Q_EMIT dataReady(tpStateName, defaultValue);
			return;

		default:
			return;
	}
}

#include "moc_DynamicScript.cpp"
