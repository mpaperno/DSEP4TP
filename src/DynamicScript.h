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
#include <QJsonObject>
#include <QJSValue>
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

		//! \name Persistence properties.
		//! \{

		//! Persistence essentially determines the lifespan of this script instance. "Session" persistence means it will exist until DSE exits. "Saved" means the instance data will be saved
		//! to a settings file when DSE exits, and restored from settings the next time DSE starts. "Temporary" instances will be automatically deleted after a time span specified in \ref autoDeleteDelay.
		Q_PROPERTY(DSE::PersistenceType persistence READ persistence WRITE setPersistence)
		/*!
			Persistent arbitrary data storage object which is attached to this script instance. This is a generic Object type on which properties can be created or deleted as necessary.
			The property value(s) can be any __serializable__ (to/from JSON) data type, incuding a objects or arrays.
			This data is stored with the script instance itself, not in the Engine the script is running in, so for example resetting the Engine instance will not affect this stored data, unlike
			with local variables. This data is also saved and restored at startup with the rest of the instance properties if its \ref persistence property is `DSE.PersistSave`.
			\par Note
			To persist between Engine resets or Dynamic Script Engine runs (in saved settings), this data is saved and restored by converting it to/from JSON.
			This means you _cannot_ restore a function-like Object (or class instance). Only plain data types, numbers and strings basically, will survive the JSON round-trip conversion. \n\n
			For example, this will not work properly:
			```js
			let color = new Color('blue');
			DSE.currentInstance().dataStore.color = color;
			// typeof dataStore.color == Color,  BUT only until the data storage needs to be saved/restored...
			// ... after JSON round-trip
			color = DSE.currentInstance().dataStore.color;
			// typeof color == Object;  color == { _a: 1, _b: 0, _format: 'name', _g: 128, .... }
			```
			Instead, save some way to restore a new instance of the object type to the same state as the saved one:
			```js
			DSE.currentInstance().dataStore.color = color.rgba()
			// dataStore.color == { r: 0, g: 128, b: 0, a: 1 }
			// ... after JSON round-trip, the Color type can accept the serialized 'rgba' object as input.
			color = new Color(DSE.currentInstance().dataStore.color);
			// color.isValid() == true; color.rgba() == { r: 0, g: 128, b: 0, a: 1 }
			```
			\warn Only use this object for data which actually needs to be saved and restored to/from persistent storage. For temporary/run-time variables it is far more
			efficient to just create and use them "on the stack" in your script (like you normally would) vs. accessing the `dataStore` object repeatedly.

			This property is read-only (meaning the actual storage object cannot be changed; properties of the object itself _can_ be fully manipulated).
		*/
		Q_PROPERTY(QJSValue dataStore READ dataStorage CONSTANT)
		//! For temporary instances, where \ref persistence property is `DSE.PersistTemporary`, this property determines the delay time before the instance is automatically deleted. The value is in milliseconds.
		//! The default delay time is 10 seconds (10,000 ms). If the instance created a Touch Portal State, the State is also removed after this delay time.  \sa persistence
		Q_PROPERTY(int autoDeleteDelay READ autoDeleteDelay WRITE setAutoDeleteDelay)
		//! \}

		//! \name Touch Portal State properties.
		//! \{

		//! This property value is `true` when when this script instance was created with "Create State" set to "Yes" by the corresponding action, and `false` otherwise.
		//! Changing this property from `false` to `true` will create a new Touch Portal State, and toggling from `true` to `false` will remove the State from Touch Portal.
		//! State creation happens "laziliy," meaning one will be created before a State update is to be sent (if ever). State removal, on the other hand, is immediate.  \sa stateCreated
		//Q_PROPERTY(bool createState MEMBER createState WRITE setCreateState)

		//! The "Create State" option selection. `DSE.StateDefaultType` enumeration value, one of: `DSE.NoStateUsed`, `DSE.FixedValueDefault`, `DSE.CustomExprDefault`, `DSE.MainExprDefault`
		//! \sa stateDefaultValue
		Q_PROPERTY(DSE::StateDefaultType stateDefaultType READ defaultType WRITE setDefaultType)
		//! The default State value specified for saved instance, if any. Depending on the value of \ref stateDefaultType,
		//! this could be an empty string, a fixed default string value, or an expression to be evaluated.  \sa stateDefaultType
		Q_PROPERTY(QByteArray stateDefaultValue READ defaultValue WRITE setDefaultValue)
		//! The State ID of this instance as used with Touch Portal to uniquely identify this State, if any.
		//! This value will be empty when \ref stateDefaultType property is `DSE.NoStateUsed`. If not empty, this is typically in the form of `DSE.VALUE_STATE_PREFIX + name`.
		//! \n This property is read-only.
		Q_PROPERTY(QString stateId READ stateId CONSTANT)
		//! The "friendly name" of the created State (if any), as it will appear in Touch Portal selector lists. By default this is the same as the \ref name property.
		//! \n If this property is set by a script or expression in the initial evaluation (when the Action creating it is first used), then the State will be created with the new name.
		//! \n Changing this property _after_ the State has already been created will have no effect. Either set it before changing the \ref stateDefaultType property,
		//! or toggle \ref stateDefaultType to `DSE.NoStateUsed` and then to another value to re-create the State under the new parent category. \sa stateDefaultType, stateCreated
		Q_PROPERTY(QString stateName READ stateName WRITE stateName)
		//! This property holds the name of the Touch Portal parent cateogry into which the created State (if any) will be placed.
		//! By default the property is not explicitly set, and reading it will return the global default value (`DSE.VALUE_STATE_PARENT_CATEOGRY`).
		//! \n If this property is set by a script or expression in the initial evaluation (when the Action creating it is first used), then the State will be created with the new name.
		//! \n Changing this property _after_ the State has already been created will have no effect. Either set it before changing the \ref stateDefaultType property,
		//! or toggle \ref stateDefaultType to `DSE.NoStateUsed` and then to another value to re-create the State under the new parent category.   \sa stateDefaultType, stateCreated
		Q_PROPERTY(QString stateParentCategory READ stateCategory WRITE stateCategory)
		//! This read-only property value is `true` if a Touch Portal State has already been created for this script instance, `false` otherwise. This should always be `false` if \ref stateDefaultType is `DSE.NoStateUsed`.
		//! \n This property is read-only.
		Q_PROPERTY(bool stateCreated READ stateCreated CONSTANT)
		//! \}

		//! \name Action behaviour properties -- how the instance reacts to various input types like button press/release/hold.
		//! \note This whole section is actually somewhat of a workaround for how Touch Portal allows "On Hold" button behaviors to be specified.
		//! All this configuration should really be on the control/button side, not in the action(s) the control is triggering.
		//! Eg. separate action for press vs. release, whether to repeat while held or not, repeat delay/rate, etc.
		//! \{

		//! The `activation` property determines how the instance will behave when a control (eg. button) using this instance is activated (eg. pressed, held, or released). \n
		//! This property is primarily relevant when an action is used in a Touch Portal "On Hold" button setup, and is usually set by the "On Hold" action options. \n
		//! The value can be any OR combination of `DSE.ActivationBehavior` enumeration flags. \n
		//! For example:
		//! - `DSE.OnPress` - Evaluates expression on initial button press only.
		//! - `DSE.OnPress | DSE.RepeatOnHold` - Evaluates expression on initial button press and repeatedly while it is held.
		//! - `DSE.OnPress | DSE.OnRelease` - Evaluates expression on initial button press and again when it is released.
		//! - `DSE.RepeatOnHold` - Ignores the initial button press and then starts repeating the evaluation after \ref effectiveRepeatDelay ms, until it is released.
		//! - `DSE.OnRelease` - Evaluates expression only when button is released. This is the default behavior when using an action in Touch Portal's "On Pressed" button setup (which actually triggers actions upon button release).
		Q_PROPERTY(DSE::ActivationBehaviors activation READ activation WRITE setActivation)
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
		//! Get or set the maximum number of times this action will repeat when held. A value of `-1` (default) means to repeat an unlimited number of times. Setting the value to `0` effectively disables repeating.
		Q_PROPERTY(int maxRepeatCount READ maxRepeatCount WRITE setMaxRepeatCount)
		//! The number of times the current, or last, repeating action of this instance has repeated. The property is reset to zero when the action if first invoked.
		//! \n This property is read-only.
		Q_PROPERTY(int repeatCount READ repeatCount CONSTANT)
		//! `true` if an Action using this instance is currently repeating, `false` otherwise.
		//! \n This property is read-only.
		Q_PROPERTY(bool isRepeating READ isRepeating CONSTANT)
		//! \}

		enum State : quint16 {
			NoErrorState       = 0,
			UninitializedState = 0x0001,
			PropertyErrorState = 0x0002,
			FileLoadErrorState = 0x0004,
			ScriptErrorState   = 0x0010,

			EvaluatingNowState = 0x0100,
			PressedState       = 0x0200,  // currently being held
			RepeatingState     = 0x0400,  // currently repeating (or waiting for timer)
			HoldReleasedState  = 0x0800,  // released after being pressed when m_activation != OnReleaseOnly

			TpStateCreatedFlag = 0x1000,

			ConfigErrorState   = PropertyErrorState | FileLoadErrorState,
			CriticalErrorState = UninitializedState | ConfigErrorState,
		};
		//Q_ENUM(State)
		Q_DECLARE_FLAGS(States, State)

		States m_state = State::UninitializedState;
		DSE::ScriptInputType m_inputType = DSE::ScriptInputType::UnknownInputType;
		DSE::ActivationBehaviors m_activation = DSE::ActivationBehavior::OnRelease; // | DSE::ActivationBehavior::RepeatOnHold;
		DSE::PersistenceType m_persist = DSE::PersistenceType::PersistSession;
		DSE::EngineInstanceType m_scope = DSE::EngineInstanceType::UnknownInstanceType;
		DSE::StateDefaultType m_defaultType = DSE::StateDefaultType::NoStateUsed;
		std::atomic_int m_autoDeleteDelay = 10 * 1000;
		std::atomic_int m_repeatRate = -1;
		std::atomic_int m_repeatDelay = -1;
		std::atomic_int m_activeRepeatRate = -1;
		std::atomic_int m_activeRepeatDelay = -1;
		std::atomic_int m_repeatCount = 0;
		std::atomic_int m_maxRepeatCount = -1;
		QString m_expr;
		QString m_file;
		QString m_originalFile;
		QString m_moduleAlias;
		QByteArray m_defaultValue;
		QByteArray m_engineName;
		QJSValue m_storedData;
		QJsonObject m_storedDataVar;
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

		explicit DynamicScript(const QByteArray &name, QObject *p = nullptr);
		~DynamicScript();

		inline bool createState() const { return m_defaultType != DSE::StateDefaultType::NoStateUsed; }

		// returns empty if createState == false
		QByteArray stateId() const { return createState() ? tpStateId : QByteArray(); }
		bool stateCreated() const { return (m_state & TpStateCreatedFlag); }

		QByteArray stateCategory() const { return tpStateCategory.isEmpty() ? QByteArrayLiteral(PLUGIN_DYNAMIC_STATES_PARENT) : tpStateCategory; }
		void stateCategory(const QString &value) { tpStateCategory = value.toUtf8(); }

		QByteArray stateName() const { return tpStateName.isEmpty() ? name : tpStateName; }
		void stateName(const QString &value) { tpStateName = value.toUtf8(); }

		DSE::ScriptInputType inputType() const { return m_inputType; }
		DSE::EngineInstanceType instanceType() const { return m_scope; }

		DSE::PersistenceType persistence() const { return m_persist; }
		void setPersistence(DSE::PersistenceType newPersist);
		inline bool isTemporary() const { return m_persist == DSE::PersistenceType::PersistTemporary; }

		QJSValue &dataStorage();

		int autoDeleteDelay() const { return m_autoDeleteDelay; }
		void setAutoDeleteDelay(int ms) { m_autoDeleteDelay = ms; }

		DSE::StateDefaultType defaultType() const { return m_defaultType; }
		void setDefaultType(DSE::StateDefaultType type);

		QByteArray defaultValue() const { return m_defaultValue; }
		void setDefaultValue(const QByteArray &value) { m_defaultValue = value; }

		void setDefaultTypeValue(DSE::StateDefaultType defType, const QByteArray &def)
		{
			setDefaultType(defType);
			setDefaultValue(def);
		}

		DSE::ActivationBehaviors activation() const { return m_activation; }
		void setActivation(DSE::ActivationBehaviors behavior);

		inline bool isPressed() const { return m_state.testFlags(State::PressedState); }
		inline bool isRepeating() const { return m_state.testFlags(State::RepeatingState); }
		int repeatCount() const { return m_repeatCount; }

		int maxRepeatCount() const { return m_maxRepeatCount; }
		void setMaxRepeatCount(int count) { m_maxRepeatCount = count; }

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
			if (isPressed() && m_state.testFlag(State::EvaluatingNowState) && m_activeRepeatRate != ms) {
				m_activeRepeatRate = ms;
				Q_EMIT activeRepeatRateChanged(ms);
			}
		}
		int activeRepeatDelay() const { return m_activeRepeatDelay; }
		void setActiveRepeatDelay(int ms)
		{
			if (ms < 50)
				ms = 50;
			if (isPressed() && m_state.testFlag(State::EvaluatingNowState) && m_activeRepeatDelay != ms) {
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

		bool setExpressionProperties(const QString &expr);
		bool setScriptProperties(const QString &file, const QString &expr);
		bool setModuleProperties(const QString &file, const QString &alias, const QString &expr);
		bool setProperties(DSE::ScriptInputType type, const QString &expr, const QString &file = QString(), const QString &alias = QString(), bool ignoreErrors = false);
		bool setExpression(const QString &expr);

		bool setEngine(ScriptEngine *se);
		ScriptEngine *engine() const { return m_engine; }
		QByteArray engineName() const { return m_engineName; }

		QByteArray serialize() const;
		bool deserialize(const QByteArray &data);

	public Q_SLOTS:
		//! Send a Touch Portal State value update using this instance's `stateId` as the State ID.
		//! If \ref stateDefaultType property is `DSE.NoStateUsed` then calling this method has no effect. \n
		//! Using this method is equivalent to (for example, assuming `ds` is an instance of DynamicScript): `TP.stateUpdateById(ds.stateId, value)`
		inline void stateUpdate(const QByteArray &value) {
			if (createState()) {
				// FIXME: TP v3.1 doesn't fire state change events based on the default value; v3.2 might.
				createTpState(/*m_defaultType != DSE::StateDefaultType::MainExprDefault*/);
				Q_EMIT dataReady(tpStateId, value);
				//qCDebug(lcPlugin) << "DynamicScript instance" << name << "sending result:" << value;
			}
		}

		void setPressedState(bool isPressed);

	Q_SIGNALS:
		void dataReady(const QByteArray &stateName, const QByteArray &result);
		void scriptError(const JSError &e);
		void finished();
		void repeatRateChanged(int ms);
		void repeatDelayChanged(int ms);
		void activeRepeatRateChanged(int ms);
		void activeRepeatDelayChanged(int ms);
		void pressedStateChanged(bool isHeld);

	private Q_SLOTS:
		// These are private to keep them hidden from scripting environment. `Plugin` is marked as friend to use these methods.
		void removeTpState();
		void evaluate();
		void evaluateDefault();

		// these really _are_ private
		void serializeStoredData();
		void createTpState(bool useActualDefault = false);
		void setCreateState();
		void setPressed(bool isPressed);
		void setupRepeatTimer(bool create = true);
		void repeatEvaluate();
		QByteArray getDefaultValue();

	private:
		void moveToMainThread();
		bool setExpr(const QString &expr);
		bool setFile(const QString &file);
		bool scheduleRepeatIfNeeded();

		friend class Plugin;
		Q_DISABLE_COPY(DynamicScript)
};

Q_DECLARE_METATYPE(DynamicScript*);
//Q_DECLARE_OPERATORS_FOR_FLAGS(DynamicScript::States);
