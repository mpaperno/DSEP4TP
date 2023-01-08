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

#pragma once

#include <QCoreApplication>
#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QObject>
#include <QReadWriteLock>
#include <QThread>
#include <QTimer>
#include <QWaitCondition>

#include "common.h"
#include "Plugin.h"
#include "ScriptEngine.h"
#include "utils.h"

class DynamicScript : public QObject
{
	private:
		Q_OBJECT

		constexpr static uint32_t SAVED_PROPERTIES_VERSION = 2;
		constexpr static int MUTEX_LOCK_TIMEOUT_MS = 250;

	public:
		enum class InputType : quint8 {
			Unknown,
			Expression,
			Script,
			Module,
		};
		Q_ENUM(InputType)

		enum class Scope : quint8 {
			Unknown, Shared, Private
		};
		Q_ENUM(Scope)

		enum class DefaultType : quint8 {
			NoDefault, FixedValue, CustomExpression, MainExpression
		};
		Q_ENUM(DefaultType)

		enum State : quint8 {
			NoErrorState       = 0,
			UninitializedState = 0x01,
			PropertyErrorState = 0x02,
			FileLoadErrorState = 0x04,
			ScriptErrorState   = 0x10,

			CriticalErrorState = UninitializedState | PropertyErrorState | FileLoadErrorState
		};
		//Q_ENUM(State)
		Q_DECLARE_FLAGS(States, State)

	private:
		States m_state = State::UninitializedState;
		InputType m_inputType = InputType::Unknown;
		std::atomic<Scope> m_scope = Scope::Unknown;
		QString m_expr;
		QString m_file;
		QString m_originalFile;
		QString m_moduleAlias;
		QDateTime m_scriptLastMod;
		QReadWriteLock m_mutex;
		ScriptEngine * m_engine = nullptr;
		Plugin *m_plugin = nullptr;
		QThread *m_thread = nullptr;

	public:
		const QByteArray name;
		const QByteArray tpStateName;
		QString lastError;
		QByteArray defaultValue;
		std::atomic_bool singleShot = false;
		std::atomic_bool stateCreated = false;
		DefaultType defaultType = DefaultType::NoDefault;

		explicit DynamicScript(const QByteArray &name, Plugin *p) :
		  QObject(/*p*/),
		  m_plugin(p),
		  name{name},
		  tpStateName(DYNAMIC_VALUE_STATE_PRFX + name)
		{
			// These connections must be established for all instance types. Others may be made later when the Scope is set.
			connect(this, &DynamicScript::scriptError, p, &Plugin::onDsScriptError, Qt::QueuedConnection);
			// Direct connection to socket where state ID is already fully qualified;
			connect(this, &DynamicScript::dataReady, p->tpClient(), qOverload<const QByteArray&, const QByteArray&>(&TPClientQt::stateUpdate), Qt::QueuedConnection);
			//connect(this, &DynamicScript::dataReady, p, &Plugin::tpStateUpdate);  // alternate
			//qCDebug(lcPlugin) << name << "Created";
		}

		~DynamicScript() {
			if (m_scope == DynamicScript::Scope::Private && m_engine)
				delete m_engine; //->deleteLater();
			moveToMainThread();
			//qCDebug(lcPlugin) << name << "Destroyed";
		}

		InputType inputType() const { return m_inputType; }
		Scope scope() const { return m_scope.load(); }

		bool setCommonProperties(InputType type, Scope scope, DefaultType save = DefaultType::NoDefault, const QByteArray &def = QByteArray())
		{
			QWriteLocker lock(&m_mutex);
			bool ok = setTypeScope(type, scope);
			if (ok)
				setSaveAndDefault(save, def);
			return !(m_state.setFlag(State::PropertyErrorState, !ok) & State::CriticalErrorState);
		}

		bool setExpressionProperties(Scope scope, const QString &expr, DefaultType save = DefaultType::NoDefault, const QByteArray &def = QByteArray())
		{
			if (!setCommonProperties(InputType::Expression, scope, save, def))
				return false;
			QWriteLocker lock(&m_mutex);
			bool ok = setExpr(expr);
			if (ok)
				setSaveAndDefault(save, def);
			return !(m_state.setFlag(State::PropertyErrorState, !ok) & State::CriticalErrorState);
		}

		bool setScriptProperties(Scope scope, const QString &file, const QString &expr, DefaultType save = DefaultType::NoDefault, const QByteArray &def = QByteArray())
		{
			if (!setCommonProperties(InputType::Script, scope, save, def))
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

		bool setModuleProperties(Scope scope, const QString &file, const QString &alias, const QString &expr, DefaultType save = DefaultType::NoDefault, const QByteArray &def = QByteArray())
		{
			if (!setCommonProperties(InputType::Module, scope, save, def))
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

		bool setProperties(InputType type, Scope scope, const QString &expr, const QString &file = QString(), const QString &alias = QString(), DefaultType save = DefaultType::NoDefault, const QByteArray &def = QByteArray())
		{
			switch(type) {
				case InputType::Expression:
					return setExpressionProperties(scope, expr, save, def);
				case InputType::Script:
					return setScriptProperties(scope, file, expr, save, def);
				case InputType::Module:
					return setModuleProperties(scope, file, alias, expr, save, def);
				default:
					return false;
			}
		}

		bool setExpression(const QString &expr)
		{
			QWriteLocker lock(&m_mutex);
			bool ok = setExpr(expr);
			return !(m_state.setFlag(State::PropertyErrorState, !ok) & State::CriticalErrorState);
		}

		void setSingleShot(bool ss = true)
		{
			if (ss == singleShot)
				return;
			singleShot = ss;
			if (ss)
				connect(this, &DynamicScript::finished, m_plugin, &Plugin::onDsFinished);
			else
				disconnect(this, &DynamicScript::finished, m_plugin, &Plugin::onDsFinished);
		}

		void resetEngine()
		{
			QWriteLocker lock(&m_mutex);
			if (m_engine) {
				m_engine->reset();
				qCInfo(lcPlugin) << "Private Scripting Engine reset completed for" << name;
			}
		}

		QByteArray serialize() const
		{
			QByteArray ba;
			QDataStream ds(&ba, QIODevice::WriteOnly);
			ds << SAVED_PROPERTIES_VERSION << (int)m_scope.load() << (int)m_inputType << m_expr << m_file << m_moduleAlias << defaultValue << (int)defaultType;
			return ba;
		}

		void deserialize(const QByteArray &data)
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

			// InputType enum values changed in v2.
			if (version == 1)
				++type;

			switch ((DynamicScript::InputType)type) {
				case DynamicScript::InputType::Expression:
					setExpressionProperties((DynamicScript::Scope)scope, expr, (DefaultType)defType, deflt);
					break;
				case DynamicScript::InputType::Script:
					setScriptProperties((DynamicScript::Scope)scope, file, expr, (DefaultType)defType, deflt);
					break;
				case DynamicScript::InputType::Module:
					setModuleProperties((DynamicScript::Scope)scope, file, alias, expr, (DefaultType)defType, deflt);
					break;
				default:
					qCWarning(lcPlugin) << "Cannot restore settings for" << name << "because the saved input type:" << type << "is unknown";
					return;
			}
		}

	private:
		void moveToMainThread()
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

		bool setScope(Scope newScope)
		{
			if (newScope == Scope::Unknown) {
				lastError = tr("Engine Instance Scope is Unknown.");
				return false;
			}
			if (newScope == m_scope)
				return true;

			m_scope = newScope;
			if (m_engine && m_engine->parent() == this)
				m_engine->deleteLater();
			moveToMainThread();

			if (m_scope == Scope::Private) {
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
					connect(m_engine, &ScriptEngine::stateValueUpdate, this, &DynamicScript::onEngineValueUpdate);
					// Instance-specific errors from background tasks.
					connect(m_engine, &ScriptEngine::raiseError, this, &DynamicScript::scriptError);
					// Connect to notifications about TP events so they can be re-broadcast to scripts in this instance.
					connect(m_plugin, &Plugin::tpNotificationClicked, m_engine, &ScriptEngine::onNotificationClicked);
					connect(m_plugin, &Plugin::tpBroadcast, m_engine, &ScriptEngine::tpBroadcast);
					// Save the default value to global constant.
					m_engine->dseObject().setProperty(QStringLiteral("INSTANCE_DEFAULT_VALUE"), QLatin1String(defaultValue));
				}
				// Global from script engine which needs name lookup because the state name is not fully qualified.
				connect(m_engine, &ScriptEngine::stateValueUpdateByName, m_plugin, &Plugin::onStateUpdateByName);
				// Direct(ish) connection to socket where state ID is already fully qualified;
				connect(m_engine, &ScriptEngine::stateValueUpdateById, m_plugin, &Plugin::tpStateUpdate);
				// Other direct connections from eponymous script functions.
				connect(m_engine, &ScriptEngine::stateCreate, m_plugin, &Plugin::tpStateCreate);
				connect(m_engine, &ScriptEngine::stateRemove, m_plugin, &Plugin::tpStateRemove);
				connect(m_engine, &ScriptEngine::choiceUpdate, m_plugin, &Plugin::tpChoiceUpdateStrList);
				connect(m_engine, &ScriptEngine::connectorUpdate, m_plugin, &Plugin::tpConnectorUpdate);
				connect(m_engine, &ScriptEngine::connectorUpdateShort, m_plugin, &Plugin::tpConnectorUpdateShort);
				connect(m_engine, &ScriptEngine::tpNotification, m_plugin, &Plugin::tpNotification);
			}
			else {
				m_engine = ScriptEngine::instance();
			}
			m_state &= ~State::UninitializedState;
			return true;
		}

		bool setTypeScope(InputType type, Scope scope)
		{
			if (type == InputType::Unknown) {
				lastError = tr("Input Type is Unknown.");
				return false;
			}
			if (!setScope(scope))
				return false;
			m_inputType = type;
			return true;
		}

		bool setExpr(const QString &expr)
		{
			if (expr.isEmpty()) {
				lastError = tr("Expression is empty.");
				return false;
			}
			m_expr = expr; // QString(expr).replace("\\", "\\\\");
			return true;
		}

		bool setFile(const QString &file)
		{
			if (file.isEmpty()) {
				lastError = tr("File path is empty.");
				return false;
			}
			if (m_state.testFlag(State::FileLoadErrorState) || m_originalFile != file) {
				QFileInfo fi(Utils::resolveFile(file));
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

		void setSaveAndDefault(DefaultType defType, const QByteArray &def)
		{
			defaultType = defType;
			defaultValue = def;
		}

	private Q_SLOTS:
		void onEngineValueUpdate(const QByteArray &v) {
			Q_EMIT dataReady(tpStateName, v);
		}
	public Q_SLOTS:
		void evaluate()
		{
			if (m_state.testFlag(State::CriticalErrorState))
				return;

			if (!m_mutex.tryLockForRead(MUTEX_LOCK_TIMEOUT_MS)) {
				qCDebug(lcPlugin) << "Mutex lock timeout for" << name;
				return;
			}

			QJSValue res;
			//ScriptEngine *e = m_engine ? m_engine : ScriptEngine::instance();
			switch (m_inputType) {
				case InputType::Expression:
					res = m_engine->expressionValue(m_expr, name);
					break;

				case InputType::Script:
					res = m_engine->scriptValue(m_file, m_expr, name);
					break;

				case InputType::Module:
					res = m_engine->moduleValue(m_file, m_moduleAlias, m_expr, name);
					break;

				default:
					m_mutex.unlock();
					return;
			}

			m_state.setFlag(State::ScriptErrorState, res.isError());
			if (m_state.testFlag(State::ScriptErrorState)) {
				Q_EMIT scriptError(res);
			}
			else if (!singleShot && !res.isUndefined() && !res.isNull()) {
				//qCDebug(lcPlugin) << "DynamicScript instance" << name << "sending result:" << res.toString();
				Q_EMIT dataReady(tpStateName, res.toString().toUtf8());
			}

			m_mutex.unlock();
			Q_EMIT finished();
		}

		void evaluateDefault()
		{
			//qCDebug(lcPlugin) << "DynamicScript instance" << name << "default type:" << defaultType << m_expr << defaultValue;
			switch (defaultType) {
				case DefaultType::MainExpression:
					evaluate();
					return;

				case DefaultType::CustomExpression:
					if (!defaultValue.isEmpty()) {
						const QString saveExpr = m_expr;
						m_expr = defaultValue;
						evaluate();
						m_expr = saveExpr;
					}
					return;

				case DefaultType::FixedValue:
					Q_EMIT dataReady(tpStateName, defaultValue);
					return;

				default:
					return;
			}
		}

	Q_SIGNALS:
		void dataReady(const QByteArray &stateName, const QByteArray &result);
		void scriptError(const QJSValue &e);
		void finished();

};

//Q_DECLARE_METATYPE(DynamicScript*);
Q_DECLARE_OPERATORS_FOR_FLAGS(DynamicScript::States);
