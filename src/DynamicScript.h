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

#include "DSE.h"

#ifdef DOXYGEN
#define QByteArray String
#endif

class ScriptEngine;
class Plugin;
namespace ScriptLib {
class TPAPI;
}

/*!
\class DynamicScript
\ingroup PluginAPI
DynamicScript represents an instance of an expression, script file, or module being evaluated using a scripting engine.
In other words, this is the type of object created by the plugin when a user invokes one of the scripting type actions this plugin provides.

It has properties representing some of the action parameters which can be set in an action/connector, such as the State/instance Name, expression, file, and so on.
Most properties are read-only, but a few can be set at runtime such as the default type/value and held action repeating delay/rate.

A DynamicScript object instance can be retrieved with the `DSE` object functions such as `DSE.instace()`, `DSE.instace(String name)`, or `DSE.instanceList()`.
*/
class DynamicScript : public QObject
{
	private:
		Q_OBJECT
		//! The State Name of this instance. This could be "ANON_n" for anonymous/one-time scripts, where "n" is an incrementing number.
		//! \n This property is read-only.
		Q_PROPERTY(QString name MEMBER name CONSTANT)
		//! The State ID of this instance as used with Touch Portal to uniquely identify this State. This is typically `DSE.VALUE_STATE_PREFIX + name`.
		//! \n This property is read-only.
		Q_PROPERTY(QString stateId MEMBER tpStateName CONSTANT)
		//! The type of scripting action. Enumeration value, one of: `DSE.Expression`, `DSE.Script`, `DSE.Module`, `DSE.Unknown`
		//! \n This property is read-only.
		Q_PROPERTY(DSE::ScriptInputType inputType READ inputType CONSTANT)
		//! The full expression string. This is as received from Touch Portal, possibly with any TP macros already evaluated.
		//! \n This property is read-only.
		Q_PROPERTY(QString expression MEMBER m_expr CONSTANT)
		//! The script/module file, if any. Only Script and Module instance types will have a file.
		//! \n This property is read-only.
		Q_PROPERTY(QString file MEMBER m_file CONSTANT)
		//! The module import alias. Only Module type instances will have an alias.
		//! \n This property is read-only.
		Q_PROPERTY(QString alias MEMBER m_moduleAlias CONSTANT)
		//! This is the Engine Instance type. Enumeration value, one of: `DSE.Shared` or `DSE.Private`
		//! \n This property is read-only.
		Q_PROPERTY(DSE::EngineInstanceType instanceType READ instanceType CONSTANT)
#ifdef DOXYGEN
		//! The Create State at Startup selection. Enumeration value, one of: `DSE.NoDefault`, `DSE.FixedValue`, `DSE.CustomExpression`, `DSE.MainExpression`
		Q_PROPERTY(DSE::ScriptDefaultType defaultType READ defaultType WRITE defaultType)
		//! The default State value specified for saved instance, if any.
		Q_PROPERTY(QByteArray defaultValue READ defaultValue WRITE defaultValue)
#else
		Q_PROPERTY(DSE::ScriptDefaultType defaultType MEMBER defaultType)
		Q_PROPERTY(QByteArray defaultValue MEMBER defaultValue)
#endif

		//! The default action repeat rate for this particular instance, in milliseconds. If `-1` (default) then the global default rate is used.  \sa activeRepeatRate
		Q_PROPERTY(int repeatRate READ repeatRate WRITE setRepeatRate)
		//! The default action repeat delay for this particular instance, in milliseconds. If `-1` (default) then the global default rate is used.  \sa activeRepeatDelay
		Q_PROPERTY(int repeatDelay READ repeatDelay WRITE setRepeatDelay)
		//! The action repeat rate for the _currently repeating_ script action, in milliseconds. Changes to this value are only relevant while an action is actively repeating (`isRepeating` == `true`).
		//!  If `-1` (default) then `repeatRate` or the global default rate is used.
		Q_PROPERTY(int activeRepeatRate READ activeRepeatRate WRITE setActiveRepeatRate)
		//! The action repeat delay time for the _currently repeating_ script action, in milliseconds. Changes to this value are only relevant while an action is actively repeating (`isRepeating` == `true`).
		//!  If `-1` (default) then `repeatDelay` or the global default delay is used.
		Q_PROPERTY(int activeRepeatDelay READ activeRepeatDelay WRITE setActiveRepeatDelay)
		//! The currently effective action repeat rate which is either the global default rate, or this instance's `repeatRate` if set, or `activeRepeatRate` if it was set and `isRepeating` is `true`.
		//! \n This property is read-only.
		Q_PROPERTY(int effectiveRepeatRate READ effectiveRepeatRate CONSTANT)
		//! The currently effective action repeat delay which is either the global default delay, or this instance's `repeatDelay` if set, or `activeRepeatDelay` if it was set and `isRepeating` is `true`.
		//! \n This property is read-only.
		Q_PROPERTY(int effectiveRepeatDelay READ effectiveRepeatDelay CONSTANT)
		//! `true` if an Action using this instance is currently repeating, `false` otherwise.
		//! \n This property is read-only.
		Q_PROPERTY(bool isRepeating MEMBER repeating CONSTANT)
		//! The number of times the current, or last, repeating action of this instance has repeated. The property is reset to zero when the action if first invoked.
		//! \n This property is read-only.
		Q_PROPERTY(int repeatCount MEMBER m_repeatCount CONSTANT)

		constexpr static uint32_t SAVED_PROPERTIES_VERSION = 2;
		constexpr static int MUTEX_LOCK_TIMEOUT_MS = 250;

		enum State : quint8 {
			NoErrorState       = 0,
			UninitializedState = 0x01,
			PropertyErrorState = 0x02,
			FileLoadErrorState = 0x04,
			ScriptErrorState   = 0x10,

			EvaluatingNowState = 0x80,

			CriticalErrorState = UninitializedState | PropertyErrorState | FileLoadErrorState
		};
		//Q_ENUM(State)
		Q_DECLARE_FLAGS(States, State)

		bool m_evaluating = false;
		States m_state = State::UninitializedState;
		DSE::ScriptInputType m_inputType = DSE::ScriptInputType::Unknown;
		DSE::EngineInstanceType m_scope = DSE::EngineInstanceType::Unknown;
		std::atomic_int m_repeatRate = -1;
		std::atomic_int m_repeatDelay = -1;
		std::atomic_int m_activeRepeatRate = -1;
		std::atomic_int m_activeRepeatDelay = -1;
		std::atomic_int m_repeatCount = 0;
		QString m_expr;
		QString m_file;
		QString m_originalFile;
		QString m_moduleAlias;
		QDateTime m_scriptLastMod;
		QReadWriteLock m_mutex;
		ScriptEngine * m_engine = nullptr;
		QThread *m_thread = nullptr;

	public:
		const QByteArray name;
		const QByteArray tpStateName;
		QString lastError;
		QByteArray defaultValue;
		std::atomic_bool singleShot = false;
		std::atomic_bool stateCreated = false;
		std::atomic_bool repeating = false;
		DSE::ScriptDefaultType defaultType = DSE::ScriptDefaultType::NoDefault;

		explicit DynamicScript(const QByteArray &name, QObject *p = nullptr);
		~DynamicScript();

		DSE::ScriptInputType inputType() const { return m_inputType; }
		DSE::EngineInstanceType instanceType() const { return m_scope; }

//		QString inputTypeStr() const { return DSE::inputTypeMeta().key((int)m_inputType); }
//		QString instanceTypeStr() const { return DSE::instanceTypeMeta().key((int)m_scope); }
//		QString defaultTypeStr() const { return DSE::defaultTypeMeta().key((int)defaultType); }

		int repeatRate() const { return m_repeatRate; }
		int repeatDelay() const { return m_repeatDelay; }
		int activeRepeatRate() const { return m_activeRepeatRate; }
		int activeRepeatDelay() const { return m_activeRepeatDelay; }
		int effectiveRepeatRate() const { return m_activeRepeatRate > 0 ? m_activeRepeatRate.load() : (m_repeatRate > 0 ? m_repeatRate.load() : DSE::defaultRepeatRate.load());  }
		int effectiveRepeatDelay() const { return m_activeRepeatDelay > 0 ? m_activeRepeatDelay.load() : (m_repeatDelay > 0 ? m_repeatDelay.load() : DSE::defaultRepeatDelay.load());  }

	protected:
		bool setCommonProperties(DSE::ScriptInputType type, DSE::EngineInstanceType scope, DSE::ScriptDefaultType save = DSE::ScriptDefaultType::NoDefault, const QByteArray &def = QByteArray());
		bool setExpressionProperties(DSE::EngineInstanceType scope, const QString &expr, DSE::ScriptDefaultType save = DSE::ScriptDefaultType::NoDefault, const QByteArray &def = QByteArray());
		bool setScriptProperties(DSE::EngineInstanceType scope, const QString &file, const QString &expr, DSE::ScriptDefaultType save = DSE::ScriptDefaultType::NoDefault, const QByteArray &def = QByteArray());
		bool setModuleProperties(DSE::EngineInstanceType scope, const QString &file, const QString &alias, const QString &expr, DSE::ScriptDefaultType save = DSE::ScriptDefaultType::NoDefault, const QByteArray &def = QByteArray());
		bool setProperties(DSE::ScriptInputType type, DSE::EngineInstanceType scope, const QString &expr, const QString &file = QString(), const QString &alias = QString(), DSE::ScriptDefaultType save = DSE::ScriptDefaultType::NoDefault, const QByteArray &def = QByteArray());
		bool setExpression(const QString &expr);
		void setSingleShot(bool ss = true);
		void resetEngine();
		QByteArray serialize() const;
		void deserialize(const QByteArray &data);

	private:
		void moveToMainThread();
		bool setScope(DSE::EngineInstanceType newScope);
		bool setTypeScope(DSE::ScriptInputType type, DSE::EngineInstanceType scope);
		bool setExpr(const QString &expr);
		bool setFile(const QString &file);
		void setSaveAndDefault(DSE::ScriptDefaultType defType, const QByteArray &def);

	private Q_SLOTS:
		void repeatEvaluate()
		{
			m_activeRepeatRate = -1;
			if (repeating)
				evaluate();
			else
				Q_EMIT finished();
		}

		void setRepeating(bool repeat = true)
		{
			if (m_state.testFlag(State::CriticalErrorState) || repeating == repeat)
				return;
			repeating = repeat;
			if (repeat) {
				m_repeatCount = 0;
				m_activeRepeatRate = -1;
			}
		}

		void evaluate();
		void evaluateDefault();

		void onEngineValueUpdate(const QByteArray &v) {
			Q_EMIT dataReady(tpStateName, v);
		}


	public Q_SLOTS:
		void setRepeatRate(int intvl) { m_repeatRate = intvl; }
		void setRepeatDelay(int intvl) {  m_repeatDelay = intvl; }

		void setActiveRepeatRate(int intvl)
		{
			if (repeating && m_state.testFlag(State::EvaluatingNowState))
				m_activeRepeatRate = intvl;
		}

		void setActiveRepeatDelay(int intvl)
		{
			if (repeating && m_state.testFlag(State::EvaluatingNowState))
				m_activeRepeatDelay = intvl;
		}

	Q_SIGNALS:
		void dataReady(const QByteArray &stateName, const QByteArray &result);
		void scriptError(const QJSValue &e);
		void finished();

	private:
		friend class Plugin;
		friend class ScriptLib::TPAPI;

};

Q_DECLARE_METATYPE(DynamicScript*);
//Q_DECLARE_OPERATORS_FOR_FLAGS(DynamicScript::States);
