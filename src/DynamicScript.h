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
#include "JSError.h"

#ifdef DOXYGEN
#define QByteArray String
#endif

class ScriptEngine;
class Plugin;

/*!
\class DynamicScript
\ingroup PluginAPI
DynamicScript represents an instance of an expression, script file, or module being evaluated using a scripting engine.
In other words, this is the type of object created by the plugin when a user invokes one of the scripting type actions this plugin provides.

It has properties representing some of the action parameters which can be set in an action/connector, such as the State/instance Name, expression, file, and so on.
Most properties are read-only, but a few can be set at runtime such as the default type/value and held action repeating delay/rate.

A DynamicScript object instance can be retrieved with the `DSE` object functions such as `DSE.currentInstance()`, `DSE.instance()`, or `DSE.instanceList()`.

\since v1.1
*/
class DynamicScript : public QObject
{
	private:
		Q_OBJECT
		//! The Instance Name of this script instance as specified in the corresponding Action which created it.
		//! \n This property is read-only.
		Q_PROPERTY(QString name MEMBER name CONSTANT)
		//! \name Touch Portal State properties.
		//! \{

		//! This property value is `true` when when this script instance was created with "Create State" set to "Yes" by the corresponding action, and `false` otherwise.
		//! Changing this property from `false` to `true` will create a new Touch Portal State, and toggling from `true` to `false` will remove the State from Touch Portal.
		//! State creation happens "laziliy," meaning one will be created before a State update is to be sent (if ever). State removal, on the other hand, is immediate.  \sa stateCreated
		Q_PROPERTY(bool createState MEMBER createState WRITE setCreateState)
		//! The State ID of this instance as used with Touch Portal to uniquely identify this State, if any.
		//! This value will be empty when \ref createState property is `false`. If not empty, this is typically in the form of `DSE.VALUE_STATE_PREFIX + name`.
		//! \n This property is read-only.
		Q_PROPERTY(QString stateId READ stateId CONSTANT)
		//! The "friendly name" of the created State (if any), as it will appear in Touch Portal selector lists. By default this is the same as the \ref name property.
		//! \n If this property is set by a script or expression in the initial evaluation (when the Action creating it is first used), then the State will be created with the new name.
		//! \n Changing this property _after_ the State has already been created will have no effect. Either set it before changing the \ref createState property to `true`,
		//! or toggle \ref createState to `false` and then `true` again to re-create the State under the new parent category.    \sa createState, stateCreated
		Q_PROPERTY(QString stateName READ stateName WRITE stateName)
		//! This property holds the name of the Touch Portal parent cateogry into which the created State (if any) will be placed.
		//! By default the property is not explicitly set, and reading it will return the global default value (`DSE.VALUE_STATE_PARENT_CATEOGRY`).
		//! \n If this property is set by a script or expression in the initial evaluation (when the Action creating it is first used), then the State will be created with the new name.
		//! \n Changing this property _after_ the State has already been created will have no effect. Either set it before changing the \ref createState property to `true`,
		//! or toggle \ref createState to `false` and then `true` again to re-create the State under the new parent category.   \sa createState, stateCreated
		Q_PROPERTY(QString stateParentCategory READ stateCategory WRITE stateCategory)
		//! This read-only property value is `true` if a Touch Portal State has already been created for this script instance, `false` otherwise. This should always be `false` if \ref createState is `false`.
		//! \n This property is read-only.
		Q_PROPERTY(bool stateCreated READ stateCreated CONSTANT)
		//! \}

		//! \name Type, Input and Engine properties.
		//! \{

		//! The type of scripting action. `DSE.ScriptInputType` enumeration value, one of: `DSE.ExpressionInput`, `DSE.ScriptInput`, `DSE.ModuleInput`, `DSE.UnknownInputType`
		//! \n This property is read-only.
		Q_PROPERTY(DSE::ScriptInputType inputType READ inputType CONSTANT)
		//! The full expression string. This is as received from Touch Portal, possibly with any TP macros already evaluated.
		//! \n This property is read-only.
		Q_PROPERTY(QString expression MEMBER m_expr CONSTANT)
		//! The script/module file, if any. Only `DSE.ScriptInput` and `DSE.ModuleInput` \ref inputType instances will have a file, the value will be empty otherwise.
		//! \n This property is read-only.
		Q_PROPERTY(QString file MEMBER m_file CONSTANT)
		//! The module import alias. Only `Module` type instances will have an alias.
		//! \n This property is read-only.
		Q_PROPERTY(QString alias MEMBER m_moduleAlias CONSTANT)
		//! This is the Engine Instance type which this script is using for its execution environment. `DSE.EngineInstanceType` enumeration value, one of: `DSE.SharedInstance` or `DSE.PrivateInstance`
		//! \n This property is read-only.
		Q_PROPERTY(DSE::EngineInstanceType engineType READ instanceType CONSTANT)
		//! The name of the engine instance this script instance is using for its execution environment. The engine instance is specified in the Action which created this script instance.
		//! For `Private` type script instances this may nor may not be the same as the `name` property. For `Shared` instance types this is always "Shared".
		//! \n This property is read-only.
		Q_PROPERTY(QString engineName MEMBER m_engineName CONSTANT)
		//! \}
		//! \name Persistence and Default value.
		//! \{

		//! "Single shot" or "one-time" script instances are set to be automatically deleted once they are finished being evaluated (after \ref autoDeleteDelay milliseconds).
		//! These instances are created with the "Single-Shot" action. This property is `true` if the instance was created using such an action, or `false` otherwise.
		//! Note that if the instance was set to create a Touch Portal State, the State will also be removed when the instance is deleted.
		//! \n This property is read-only.  \sa autoDeleteDelay
		Q_PROPERTY(bool singleShot MEMBER singleShot CONSTANT)
		//! For "Single-Shot" type instances (\ref singleShot property is `true`), this property determines the delay time before the instance is automatically deleted. The value is in milliseconds.
		//! The default delay time is 10 seconds (10,000 ms). If the instance created a Touch Portal State, the State is also removed after this delay time.  \sa singleShot
		Q_PROPERTY(int autoDeleteDelay MEMBER autoDeleteDelay WRITE setAutoDeleteDelay)
		//! The "Load at Startup" selection. `DSE.ScriptDefaultType` enumeration value, one of: `DSE.NoSavedDefault`, `DSE.FixedValueDefault`, `DSE.CustomExprDefault`, `DSE.MainExprDefault`
		//! \sa defaultValue
		Q_PROPERTY(DSE::ScriptDefaultType defaultType READ defaultType WRITE defaultType)
		//! The default State value specified for saved instance, if any. Depending on the value of \ref defaultType,
		//! this could be an empty string, a fixed default string value, or an expression to be evaluated.  \sa defaultType
		Q_PROPERTY(QByteArray defaultValue READ defaultValue WRITE defaultValue)
		//! \}

		//! \name Repeating (held) action properties.
		//! \{

		//! The default action repeat rate for this particular instance, in milliseconds. If `-1` (default) then the global default rate is used.  \sa activeRepeatRate, DSE.defaultActionRepeatRate
		Q_PROPERTY(int repeatRate READ repeatRate WRITE setRepeatRate NOTIFY repeatRateChanged)
		//! The default action repeat delay for this particular instance, in milliseconds. If `-1` (default) then the global default rate is used.  \sa activeRepeatDelay, DSE.defaultActionRepeatDelay
		Q_PROPERTY(int repeatDelay READ repeatDelay WRITE setRepeatDelay NOTIFY repeatDelayChanged)
		//! The action repeat rate for the _currently repeating_ script action, in milliseconds. Changes to this value are only relevant while an action is actively repeating (\ref isRepeating == `true`).
		//!  If `-1` (default) then \ref repeatRate or the global default rate is used.
		Q_PROPERTY(int activeRepeatRate READ activeRepeatRate WRITE setActiveRepeatRate NOTIFY activeRepeatRateChanged)
		//! The action repeat delay time for the _currently repeating_ script action, in milliseconds. Changes to this value are only relevant while an action is actively repeating (\ref isRepeating == `true`).
		//!  If `-1` (default) then \ref repeatDelay or the global default delay is used.
		Q_PROPERTY(int activeRepeatDelay READ activeRepeatDelay WRITE setActiveRepeatDelay NOTIFY activeRepeatDelayChanged)
		//! The currently effective action repeat rate which is either the global default rate, or this instance's \ref repeatRate if set, or \ref activeRepeatRate if it was set and \ref isRepeating is `true`.
		//! \n This property is read-only.
		Q_PROPERTY(int effectiveRepeatRate READ effectiveRepeatRate CONSTANT)
		//! The currently effective action repeat delay which is either the global default delay, or this instance's \ref repeatDelay if set, or  \ref activeRepeatDelay if it was set and \ref isRepeating is `true`.
		//! \n This property is read-only.
		Q_PROPERTY(int effectiveRepeatDelay READ effectiveRepeatDelay CONSTANT)
		//! `true` if an Action using this instance is currently repeating, `false` otherwise.
		//! \n This property is read-only.
		Q_PROPERTY(bool isRepeating READ isRepeating CONSTANT)
		//! The number of times the current, or last, repeating action of this instance has repeated. The property is reset to zero when the action if first invoked.
		//! \n This property is read-only.
		Q_PROPERTY(int repeatCount READ repeatCount CONSTANT)
		//! \}

		enum State : quint16 {
			NoErrorState       = 0,
			UninitializedState = 0x0001,
			PropertyErrorState = 0x0002,
			FileLoadErrorState = 0x0004,
			ScriptErrorState   = 0x0010,

			EvaluatingNowState = 0x0100,

			TpStateCreatedFlag = 0x1000,

			CriticalErrorState = UninitializedState | PropertyErrorState | FileLoadErrorState
		};
		//Q_ENUM(State)
		Q_DECLARE_FLAGS(States, State)

		States m_state = State::UninitializedState;
		DSE::ScriptInputType m_inputType = DSE::ScriptInputType::UnknownInputType;
		DSE::EngineInstanceType m_scope = DSE::EngineInstanceType::UnknownInstanceType;
		DSE::ScriptDefaultType m_defaultType = DSE::ScriptDefaultType::NoSavedDefault;
		std::atomic_bool m_repeating = false;
		std::atomic_int m_repeatRate = -1;
		std::atomic_int m_repeatDelay = -1;
		std::atomic_int m_activeRepeatRate = -1;
		std::atomic_int m_activeRepeatDelay = -1;
		std::atomic_int m_repeatCount = 0;
		QString m_expr;
		QString m_file;
		QString m_originalFile;
		QString m_moduleAlias;
		QByteArray m_defaultValue;
		QByteArray m_engineName;
		QDateTime m_scriptLastMod;
		QReadWriteLock m_mutex;
		ScriptEngine * m_engine = nullptr;
		QTimer *m_repeatTim = nullptr;

	public:
		const QByteArray name;
		const QByteArray tpStateId;
		QByteArray tpStateCategory;
		QByteArray tpStateName;
		QString lastError;
		bool createState = false;
		std::atomic_bool singleShot = false;
		std::atomic_int autoDeleteDelay = 10 * 1000;

		explicit DynamicScript(const QByteArray &name, QObject *p = nullptr);
		~DynamicScript();

		// returns empty if createState == false
		QByteArray stateId() const { return createState ? tpStateId : QByteArray(); }
		bool stateCreated() const { return (m_state & TpStateCreatedFlag); }

		QByteArray stateCategory() const { return tpStateCategory.isEmpty() ? QByteArrayLiteral(PLUGIN_DYNAMIC_STATES_PARENT) : tpStateCategory; }
		void stateCategory(const QString &value) { tpStateCategory = value.toUtf8(); }

		QByteArray stateName() const { return tpStateName.isEmpty() ? name : tpStateName; }
		void stateName(const QString &value) { tpStateName = value.toUtf8(); }

		DSE::ScriptInputType inputType() const { return m_inputType; }
		DSE::EngineInstanceType instanceType() const { return m_scope; }

		void setAutoDeleteDelay(int ms) { autoDeleteDelay = ms; }

		DSE::ScriptDefaultType defaultType() const { return m_defaultType; }
		void defaultType(DSE::ScriptDefaultType type) { m_defaultType = type; }

		QByteArray defaultValue() const { return m_defaultValue; }
		void defaultValue(const QByteArray &value) { m_defaultValue = value; }

		int repeatRate() const { return m_repeatRate; }
		void setRepeatRate(int ms) {
			if (ms < 50)
				ms = 50;
			if (m_repeatRate != ms) {
				m_repeatRate = ms;
				Q_EMIT repeatRateChanged(ms);
			}
		}

		int repeatDelay() const { return m_repeatDelay; }
		void setRepeatDelay(int ms) {
			if (ms < 50)
				ms = 50;
			if (m_repeatDelay != ms){
				m_repeatDelay = ms;
				Q_EMIT repeatDelayChanged(ms);
			}
		}

		int activeRepeatRate() const { return m_activeRepeatRate; }
		void setActiveRepeatRate(int ms)
		{
			if (ms < 50)
				ms = 50;
			if (m_repeating && m_state.testFlag(State::EvaluatingNowState) && m_activeRepeatRate != ms) {
				m_activeRepeatRate = ms;
				Q_EMIT activeRepeatRateChanged(ms);
			}
		}
		int activeRepeatDelay() const { return m_activeRepeatDelay; }
		void setActiveRepeatDelay(int ms)
		{
			if (ms < 50)
				ms = 50;
			if (m_repeating && m_state.testFlag(State::EvaluatingNowState) && m_activeRepeatDelay != ms) {
				m_activeRepeatDelay = ms;
				Q_EMIT activeRepeatDelayChanged(ms);
			}
		}

		int repeatProperty(quint8 property) { return (property & DSE::RepeatRateProperty) ? m_repeatRate : m_repeatDelay; }
		void setRepeatProperty(quint8 property, int ms)
		{
			if (property & DSE::RepeatRateProperty)
				setRepeatRate(ms);
			if (property & DSE::RepeatDelayProperty)
				setRepeatDelay(ms);
		}

		int effectiveRepeatRate() const { return m_activeRepeatRate > 0 ? m_activeRepeatRate.load() : (m_repeatRate > 0 ? m_repeatRate.load() : DSE::defaultRepeatRate.load()); }
		int effectiveRepeatDelay() const { return m_activeRepeatDelay > 0 ? m_activeRepeatDelay.load() : (m_repeatDelay > 0 ? m_repeatDelay.load() : DSE::defaultRepeatDelay.load()); }

		int repeatCount() const { return m_repeatCount; }
		bool isRepeating() const { return m_repeating; }

		bool setExpressionProperties(const QString &expr);
		bool setScriptProperties(const QString &file, const QString &expr);
		bool setModuleProperties(const QString &file, const QString &alias, const QString &expr);
		bool setProperties(DSE::ScriptInputType type, const QString &expr, const QString &file = QString(), const QString &alias = QString(), bool ignoreErrors = false);
		void setDefaultTypeValue(DSE::ScriptDefaultType defType, const QByteArray &def);
		bool setExpression(const QString &expr);

		bool setEngine(ScriptEngine *se);
		ScriptEngine *engine() const { return m_engine; }
		QByteArray engineName() const { return m_engineName; }

		QByteArray serialize() const;
		bool deserialize(const QByteArray &data);

	public Q_SLOTS:
		//! Send a Touch Portal State value update using this instance's `stateId` as the State ID.
		//! If \ref createState property is `false` then calling this method has no effect. \n
		//! Using this method is equivalent to (for example, assuming `ds` is an instance of DynamicScript): `TP.stateUpdateById(ds.stateId, value)`
		inline void stateUpdate(const QByteArray &value) {
			if (createState) {
				if (!(m_state & TpStateCreatedFlag))
					createTpState();
				Q_EMIT dataReady(tpStateId, value);
				//qCDebug(lcPlugin) << "DynamicScript instance" << name << "sending result:" << value;
			}
		}

		void setCreateState(bool create);

	Q_SIGNALS:
		void dataReady(const QByteArray &stateName, const QByteArray &result);
		void scriptError(const JSError &e);
		void finished();
		void repeatRateChanged(int ms);
		void repeatDelayChanged(int ms);
		void activeRepeatRateChanged(int ms);
		void activeRepeatDelayChanged(int ms);

	private Q_SLOTS:
		// These are private to keep them hidden from scripting environment. `Plugin` is marked as friend to use these methods.
		void createTpState();
		void removeTpState();
		void setSingleShot(bool ss = true);
		void setRepeating(bool repeat = true);
		void evaluate();
		void evaluateDefault();

		// these really _are_ private
		void setupRepeatTimer(bool create = true);
		void repeatEvaluate();

	private:
		void moveToMainThread();
		bool setExpr(const QString &expr);
		bool setFile(const QString &file);

		friend class Plugin;
		Q_DISABLE_COPY(DynamicScript)
};

Q_DECLARE_METATYPE(DynamicScript*);
//Q_DECLARE_OPERATORS_FOR_FLAGS(DynamicScript::States);
