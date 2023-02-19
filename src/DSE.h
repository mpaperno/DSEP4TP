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
#include <QMetaEnum>
#include <QObject>
#include <QQmlEngine>
#include <QReadWriteLock>

#include "utils.h"
#include "DSE_NS.h"

#define PLUGIN_STATE_ID_PREFIX        "dsep."
#define PLUGIN_DYNAMIC_STATES_PARENT  "Dynamic Values"

#ifdef DOXYGEN
#define QByteArray String
#define QVariantList Array<String>
#endif

class DynamicScript;
class Plugin;
class ScriptEngine;

//! \class DSE
//! \ingroup PluginAPI
//! The DSE object contains constants and functions related to the plugin environment.
//! It can be used to get or set properties of any existing script instance.
#ifndef DOXYGEN
class DSE : public QObject
#else
namespace DSE
#endif
{
		Q_OBJECT
		/*!
    Current DSE plugin version number in integer format.

    This is a "binary coded decimal" where<br />
    `((MAJOR << 24) | (MINOR << 16) | (PATCH << 8) | BUILD)` (each part is limited to values 0-99).<br />
    If formatted in hexadecimal this will match the version "number" that Touch Portal shows in the plugin's settings.<br />
    E.g. `"1.2.30.4" == 16920580 == 0x01023004` or "01023004" as shown in Touch Portal. <br />
    The possible "name" part of a version is ignored (the build number increments for each new pre-release alpha/beta version and again on the final release).
    */
		Q_PROPERTY(quint32 PLUGIN_VERSION_NUM MEMBER pluginVersion CONSTANT)
		/*!
      Current DSE plugin version number in `"MAJOR.MINOR.PATCH.BUILD[-name]"` string format. Eg. "1.0.0.2" or "1.2.0.1-beta1".
      At least one of the digits always increments for each new published version. The build number increments for any/each new pre-release (alpha/beta)
      version and again on the final (it may increment in irregular intervals).
    */
		Q_PROPERTY(QString PLUGIN_VERSION_STR MEMBER pluginVersionStr CONSTANT)
		//! The base directory path for script files. This is either the path specified in the plugin's "Script Files Base Directory" setting (in Touch Portal),
    //! or the plugin's current working directory (which is the default when nothing is specified in the setting).
		Q_PROPERTY(QString SCRIPTS_BASE_DIR READ getScriptsBaseDir CONSTANT)
		//! The prefix added by the plugin to an script instance's Name to form a unique State ID, before creating or updating it in Touch Portal.
		//! Touch Portal uses the unique ID to identify States. \sa `DynamicScript.stateId`
		Q_PROPERTY(QString VALUE_STATE_PREFIX MEMBER valueStatePrefix CONSTANT)
		//! This property contains the name of the Touch Portal parent cateogry into which newly created States will be placed by default.
		//! This is where the States will appear in Touch Portal selector lists (which sort States at all). The category will always appear as a child of
		//! the main "Dynamic Script Engine" category.  \sa DynamicScript.stateParentCategory
		Q_PROPERTY(QString VALUE_STATE_PARENT_CATEOGRY READ stateParentCategory CONSTANT)
		/*!
      This property contains the name of the operating system running the plugin.
      Possible values are:
      <pre>
        "linux" - Linux
        "osx" - macOS
        "unix" - Other Unix-based OS
        "windows" - Windows
        "android" - Android
        "ios" - iOS
        "winrt" - WinRT / UWP
        "wasm" - WebAssembly
      </pre>
    */
		Q_PROPERTY(QString PLATFORM_OS MEMBER platformOs CONSTANT)
		//! Contains the value of the current system user's Touch Portal settings directory path. This should be the same as shown in Touch Portal
    //! _Settings -> Info -> Data folder_ field. The value is determined at plugin startup based on the current user and operating system. \n
    //! Note that on Windows the path is separated with `/` (not backslashes). Backslashes are annoying in JavaScript and in just about every usage
    //! the forward slashes work just as well.
		//! \since v1.1
		Q_PROPERTY(QString TP_USER_DATA_PATH READ tpDataPath CONSTANT)
		//! Numeric version of currently connected Touch Portal desktop application. eg. `301007`
		//! \since v1.1
		Q_PROPERTY(quint32 TP_VERSION_CODE MEMBER tpVersion CONSTANT)
		//! String version of currently connected Touch Portal desktop application. eg. "3.1.7.0.0"
		//! \since v1.1
		Q_PROPERTY(QString TP_VERSION_STR MEMBER tpVersionStr CONSTANT)

		//! The global default action repeat rate (interval), in milliseconds. The rate is the amount of time to wait between repeat activations a held action.
		//! The repeat rate takes effect _after_ the initial repeat delay. Minimum interval is 50ms. \n
		//! This property has a change notification event: `defaultActionRepeatRateChanged(int ms)`. A callback can be attached to this method by using the `connect()` syntax, eg:
		//! ```js
		//! DSE.defaultActionRepeatRateChanged.connect( (ms) => { console.log("Repeat Rate changed to " + ms); } );
		//! ```
		//!  \sa defaultActionRepeatDelay, DynamicScript.repeatRate
		//! \since v1.2
		Q_PROPERTY(int defaultActionRepeatRate READ defaultActionRepeatRate WRITE setDefaultActionRepeatRate NOTIFY defaultActionRepeatRateChanged)
		//! The global default action repeat delay, in milliseconds. The delay is the amount of time before a held action starts repeating, after the initial activation.
		//! After this initial delay, the action will be repeated at the current repeat rate. Minimum delay is 50ms.
		//! This property has a change notification event: `defaultActionRepeatDelayChanged(int ms)`. A callback can be attached to this method by using the `connect()` syntax, eg:
		//! ```js
		//! DSE.defaultActionRepeatDelayChanged.connect( (ms) => { console.log("Repeat Delay changed to " + ms); } );
		//! ```
		//! \sa defaultActionRepeatRate, DynamicScript.repeatDelay
		//! \since v1.2
		Q_PROPERTY(int defaultActionRepeatDelay READ defaultActionRepeatDelay WRITE setDefaultActionRepeatDelay NOTIFY defaultActionRepeatDelayChanged)

		//! Returns the engine instance type associated with the current script instance, one of DSE.EngineInstanceType enum values: `DSE.PrivateInstance` or `DSE.SharedInstance`. \sa DynamicScript.engineType
		//! \since v1.2
		Q_PROPERTY(DseNS::EngineInstanceType engineInstanceType READ instanceType CONSTANT)
		//! Returns the name of the engine instance associated with the current script instance. For the global shared engine this is always "Shared".
		//! For private engine instances, this may or may not be the same as the script's Instance Name,
		//! for example if a script action was set to use a specific private named engine instance instead of just "Private".
		//! Unlike \ref currentInstanceName property, this value will always be constant, even inside asyncronous methods.  \sa currentInstanceName, DynamicScript.engineName
		//! \since v1.2
		Q_PROPERTY(QString engineInstanceName READ engineInstanceName CONSTANT)
		//! The Instance Name of the script currently being evaluated, as specified in the corresponding Touch Portal action which invoked the script/expression.
		//! This is essentially the same as calling `DSE.currentInstance()?.name`.
		//! \note In some cases this property may return an empty, or even incorrect, result, such as when working inside asyncronous methods like with a `Promise`.
		//! In such cases it is best to save the intance name to a local constant variable before entering the asyncronous method.  \sa engineInstanceName, currentInstance(), DynamicScript.name
		//! \since v1.2
		Q_PROPERTY(QString currentInstanceName READ currentInstanceName CONSTANT)
		//! Alias for \ref engineInstanceType. \sa DynamicScript.engineType  \since v1.2
		Q_PROPERTY(DseNS::EngineInstanceType currentInstanceType READ instanceType CONSTANT)

		//! The scope of the current script's engine, either "Shared" or "Private".
		//! \deprecated{v1.2}
		//! This property is deprecated and may be removed in a future version; Use `DSE.engineInstanceType` or `DSE.currentInstace()?.engineType` instead which return enumeration values instead of strings.
		Q_PROPERTY(QString INSTANCE_TYPE READ instanceTypeStr CONSTANT)
		//! The the current script's Instance Name, as specified in the corresponding Touch Portal action which is running the script/expression.
		//! \deprecated{v1.2}
		//! This property is deprecated and may be removed in a future version; Use `DSE.currentInstanceName` or `DSE.currentInstace()?.name` instead.
		Q_PROPERTY(QString INSTANCE_NAME MEMBER instanceName CONSTANT)
		//! This is the default value as specified in the action which created this script instance, which may be empty/blank (but not null/undefined). This property is empty in Shared instance types.
		//! \deprecated{v1.2}
		//! This property is deprecated and may be removed in a future version; Use `DSE.currentInstance()?.defaultValue` (for example) instead, which is more reliable and works in shared engine instances.
		Q_PROPERTY(QString INSTANCE_DEFAULT_VALUE READ instanceDefault CONSTANT)

		// The enum properties is a workaround for enum types not being available in QJSEngine.
#ifndef DOXYGEN
		static DseNS::EngineInstanceType EngineInstanceType_UnknownInstanceType() { return DseNS::UnknownInstanceType; }
		static DseNS::EngineInstanceType EngineInstanceType_SharedInstance()      { return DseNS::SharedInstance; }
		static DseNS::EngineInstanceType EngineInstanceType_PrivateInstance()     { return DseNS::PrivateInstance; }
		Q_PROPERTY(DseNS::EngineInstanceType UnknownInstanceType READ EngineInstanceType_UnknownInstanceType CONSTANT)
		Q_PROPERTY(DseNS::EngineInstanceType SharedInstance      READ EngineInstanceType_SharedInstance      CONSTANT)
		Q_PROPERTY(DseNS::EngineInstanceType PrivateInstance     READ EngineInstanceType_PrivateInstance     CONSTANT)

		static DseNS::ScriptInputType ScriptInputType_UnknownInputType() { return DseNS::UnknownInputType; }
		static DseNS::ScriptInputType ScriptInputType_ExpressionInput()  { return DseNS::ExpressionInput; }
		static DseNS::ScriptInputType ScriptInputType_ScriptInput()      { return DseNS::ScriptInput; }
		static DseNS::ScriptInputType ScriptInputType_ModuleInput()      { return DseNS::ModuleInput; }
		Q_PROPERTY(DseNS::ScriptInputType UnknownInputType  READ ScriptInputType_UnknownInputType CONSTANT)
		Q_PROPERTY(DseNS::ScriptInputType ExpressionInput   READ ScriptInputType_ExpressionInput  CONSTANT)
		Q_PROPERTY(DseNS::ScriptInputType ScriptInput       READ ScriptInputType_ScriptInput      CONSTANT)
		Q_PROPERTY(DseNS::ScriptInputType ModuleInput       READ ScriptInputType_ModuleInput      CONSTANT)

		static DseNS::PersistenceType PersistenceType_PersistSession()   { return DseNS::PersistSession; }
		static DseNS::PersistenceType PersistenceType_PersistTemporary() { return DseNS::PersistTemporary; }
		static DseNS::PersistenceType PersistenceType_PersistSave()      { return DseNS::PersistSave; }
		Q_PROPERTY(DseNS::PersistenceType PersistSession   READ PersistenceType_PersistSession   CONSTANT)
		Q_PROPERTY(DseNS::PersistenceType PersistTemporary READ PersistenceType_PersistTemporary CONSTANT)
		Q_PROPERTY(DseNS::PersistenceType PersistSave      READ PersistenceType_PersistSave      CONSTANT)

		static DseNS::SavedDefaultType SavedDefaultType_NoSavedDefault()    { return DseNS::NoSavedDefault; }
		static DseNS::SavedDefaultType SavedDefaultType_FixedValueDefault() { return DseNS::FixedValueDefault; }
		static DseNS::SavedDefaultType SavedDefaultType_CustomExprDefault() { return DseNS::CustomExprDefault; }
		static DseNS::SavedDefaultType SavedDefaultType_MainExprDefault()   { return DseNS::MainExprDefault; }
		Q_PROPERTY(DseNS::SavedDefaultType NoSavedDefault     READ SavedDefaultType_NoSavedDefault    CONSTANT)
		Q_PROPERTY(DseNS::SavedDefaultType FixedValueDefault  READ SavedDefaultType_FixedValueDefault CONSTANT)
		Q_PROPERTY(DseNS::SavedDefaultType CustomExprDefault  READ SavedDefaultType_CustomExprDefault CONSTANT)
		Q_PROPERTY(DseNS::SavedDefaultType MainExprDefault    READ SavedDefaultType_MainExprDefault   CONSTANT)

		static DseNS::ActivationBehavior ActivationBehavior_NoActivation() { return DseNS::NoActivation; }
		static DseNS::ActivationBehavior ActivationBehavior_OnPress()      { return DseNS::OnPress; }
		static DseNS::ActivationBehavior ActivationBehavior_OnRelease()    { return DseNS::OnRelease; }
		static DseNS::ActivationBehavior ActivationBehavior_RepeatOnHold() { return DseNS::RepeatOnHold; }
		Q_PROPERTY(DseNS::ActivationBehavior NoActivation READ ActivationBehavior_NoActivation CONSTANT)
		Q_PROPERTY(DseNS::ActivationBehavior OnPress      READ ActivationBehavior_OnPress      CONSTANT)
		Q_PROPERTY(DseNS::ActivationBehavior OnRelease    READ ActivationBehavior_OnRelease    CONSTANT)
		Q_PROPERTY(DseNS::ActivationBehavior RepeatOnHold READ ActivationBehavior_RepeatOnHold CONSTANT)

		static DseNS::RepeatProperty RepeatProperty_RepeatRateProperty()   { return DseNS::RepeatRateProperty; }
		static DseNS::RepeatProperty RepeatProperty_RepeatDelayProperty()  { return DseNS::RepeatDelayProperty; }
		static DseNS::RepeatProperty RepeatProperty_AllRepeatProperties()  { return DseNS::AllRepeatProperties; }
		Q_PROPERTY(DseNS::RepeatProperty RepeatRateProperty  READ RepeatProperty_RepeatRateProperty  CONSTANT)
		Q_PROPERTY(DseNS::RepeatProperty RepeatDelayProperty READ RepeatProperty_RepeatDelayProperty CONSTANT)
		Q_PROPERTY(DseNS::RepeatProperty AllRepeatProperties READ RepeatProperty_AllRepeatProperties CONSTANT)
/*
		static DseNS::AdjustmentType AdjustmentType_SetAbsolute() { return DseNS::SetAbsolute; }
		static DseNS::AdjustmentType AdjustmentType_SetRelative() { return DseNS::SetRelative; }
		static DseNS::AdjustmentType AdjustmentType_Increment()   { return DseNS::Increment; }
		static DseNS::AdjustmentType AdjustmentType_Decrement()   { return DseNS::Decrement; }
		Q_PROPERTY(DseNS::AdjustmentType SetAbsolute READ AdjustmentType_SetAbsolute CONSTANT)
		Q_PROPERTY(DseNS::AdjustmentType SetRelative READ AdjustmentType_SetRelative CONSTANT)
		Q_PROPERTY(DseNS::AdjustmentType Increment   READ AdjustmentType_Increment   CONSTANT)
		Q_PROPERTY(DseNS::AdjustmentType Decrement   READ AdjustmentType_Decrement   CONSTANT)
*/
#endif

	public:
		using ScriptState = QHash<QByteArray, DynamicScript *>;
		using EngineState = QHash<QByteArray, ScriptEngine *>;

		static const quint32 pluginVersion;
		static const QByteArray pluginVersionStr;
		static const QString platformOs;

		static QString scriptsBaseDir;
		static QByteArray valueStatePrefix;

		static quint32 tpVersion;
		static QString tpVersionStr;
		static QByteArray tpCurrentPage;

		static std::atomic_int defaultRepeatRate;
		static std::atomic_int defaultRepeatDelay;

		static DSE *sharedInstance;
		static DynamicScript *defaultScriptInstance;

		bool privateInstance { false };
		QByteArray instanceName;

		explicit DSE(ScriptEngine *se = nullptr, QObject *p = nullptr);

		static ScriptState *instances();
		static QReadWriteLock *instances_mutex();
		static const QList<DynamicScript *> instances_const();
		static DynamicScript *insert(const QByteArray &name, DynamicScript *ds);
		static bool removeInstance(const QByteArray &name);
		static QByteArrayList instanceKeys();

		static EngineState *engines();
		static const QList<ScriptEngine *> engines_const();
		static QReadWriteLock *engines_mutex();
		static ScriptEngine *engine(const QByteArray &name);
		static ScriptEngine *insert(const QByteArray &name, ScriptEngine *se);
		static bool removeEngine(const QByteArray &name);
		static QByteArrayList engineKeys();

		//! Returns all currently existing script instance names as an array of strings.
		//! \since v1.2
		Q_INVOKABLE static QVariantList instanceNames();

		//! \fn Array<DynamicScript> instanceList()
		//! \memberof DSE
		//! Returns all currently existing script instances as an iteratable array-like object (iterate with `of` eg. `for (const ds of DSE.instanceList()) ...`).
		//! \since v1.2
		Q_INVOKABLE static QList<DynamicScript *> instanceList();

		//! \fn DynamicScript instance(String name)
		//! \memberof DSE
		//! Returns the script instance with given `name`, if any, otherwise returns `null`.
		//! \since v1.2
		Q_INVOKABLE static DynamicScript *instance(const QByteArray &name);

		//! \fn DynamicScript currentInstance()
		//! \memberof DSE
		//! Returns the current script Instance. This is equivalent to calling `DSE.instance(DSE.currentInstanceName)`.
		//! \note In some cases this function may return a `null` result, such as when working inside asyncronous methods like with a `Promise`.
		//! If you need to access the current instance in such cases, you should save a reference to the instance before invoking any async methods
		//! and use the saved instace inside them. Or use the `instance(String name)` overload with a static string value for the instance name.
		//! \since v1.2
		Q_INVOKABLE DynamicScript *currentInstance() const { return instance(instanceName); }
		DseNS::EngineInstanceType instanceType() const { return privateInstance ? DseNS::PrivateInstance : DseNS::SharedInstance; };
		QByteArray currentInstanceName() const { return instanceName; }

		QByteArray engineInstanceName() const;

		static inline QString stateParentCategory() { return QStringLiteral(PLUGIN_DYNAMIC_STATES_PARENT); }
		static inline QString tpDataPath() { return QString::fromUtf8(Utils::tpDataPath()); }
		static inline QString getScriptsBaseDir() { return scriptsBaseDir.isEmpty() ? QDir::currentPath() : scriptsBaseDir; }
		static inline QString resolveFile(const QString &base)
		{
			if (scriptsBaseDir.isEmpty() || base.isEmpty())
				return base;
			const QString tbase = QDir::fromNativeSeparators(base);
			if (QDir::isAbsolutePath(tbase))
				return tbase;
			return QDir::cleanPath(scriptsBaseDir + tbase);
		}

		static int defaultActionRepeatRate() { return defaultRepeatRate; }
		static void setDefaultActionRepeatRate(int ms);
		static int defaultActionRepeatDelay() { return defaultRepeatDelay; }
		static void setDefaultActionRepeatDelay(int ms);

		static int defaultActionRepeatProperty(quint8 property) { return (property & DseNS::RepeatRateProperty) ? defaultRepeatRate : defaultRepeatDelay; }
		static void setDefaultActionRepeatProperty(quint8 property, int ms)
		{
			if (property & DseNS::RepeatRateProperty)
				setDefaultActionRepeatRate(ms);
			if (property & DseNS::RepeatDelayProperty)
				setDefaultActionRepeatDelay(ms);
		}

		// deprecated, remove
		inline QString instanceTypeStr() const { return DseNS::instanceTypeMeta().key((int)instanceType()); };
		// deprecated, remove
		QByteArray instanceDefault() const;

		//! \fn String instanceStateId()
		//! \memberof DSE
		//! Gets Touch Portal State ID of the current script's instance.
		//! This is what Touch Portal actually uses to uniquely identify the state (not just the name by itself).
		//! This is a convenience method that returns the same as `DSE.VALUE_STATE_PREFIX + DSE.INSTANCE_NAME`.
		//! \deprecated{v1.2}
		//! This function is deprecated and may be removed in a future version; `DSE.currentInstace()?.stateId` instead, for example.
		Q_INVOKABLE QString instanceStateId() { return QString(valueStatePrefix + instanceName); }

	public Q_SLOTS:
		//! \fn void setActionRepeat(DSE.RepeatProperty property, int ms, String forInstance = "")
		//! \memberof DSE
		//! Convenience method to set the action repeat rate and/or delay prooperties on either the global default or a specific named script instance.
		//! \param property should be one of `DSE.RepeatRateProperty`, `DSE.RepeatDelayProperty` or `DSE.AllRepeatProperties` to set both rate and delay at the same time.
		//! \param ms is the new property value, in milliseconds (minimum is 50ms).
		//! \param forInstance can be a script Instance Name, or empty (default) to set the global default repeat properties.
		//! \sa adjustActionRepeat() \since v1.2
		void setActionRepeat(quint8 property, int ms, const QByteArray &forInstance = QByteArray()) {
			setActionRepeatProperty(property, DseNS::SetAbsolute, ms, forInstance);
		}
		//! \fn void adjustActionRepeat(DSE.RepeatProperty property, int byMs, String forInstance = "")
		//! \memberof DSE
		//! Convenience method to adjust the action repeat rate and/or delay prooperties on either the global default or a specific named script instance.
		//! \param property should be one of `DSE.RepeatRateProperty`, `DSE.RepeatDelayProperty` or `DSE.AllRepeatProperties` to set both rate and delay at the same time.
		//! \param byMs is the adjustment amount, positive or negative, in milliseconds.
		//! \param forInstance can be a script Instance Name, or empty (default) to set the global default repeat properties.
		//! \sa setActionRepeat() \since v1.2
		void adjustActionRepeat(quint8 property, int byMs, const QByteArray &forInstance = QByteArray()) {
			setActionRepeatProperty(property, DseNS::SetRelative, byMs, forInstance);
		}

	Q_SIGNALS:
		void defaultActionRepeatRateChanged(int ms);
		void defaultActionRepeatDelayChanged(int ms);

	private Q_SLOTS:
		// Internal implementation, also invoked by Plugin via signal.
		void setActionRepeatProperty(quint8 property, quint8 action, int ms, const QByteArray &forInstance = QByteArray(), bool repeat = false);
		void cancelRepeatingAction(quint8 act = DSE::ACT_ADJ_REPEAT);
		void setActionRepeat_impl(quint8 property, quint8 action, int ms, const QByteArray &forInstance, uint repeaterId);

	private:
		ScriptEngine *se = nullptr;

		enum ActionID { ACT_ADJ_REPEAT, ACT_ENUM_LAST };
		struct ActionRecrod { std::atomic_uint repeaterId {0}; std::atomic_bool isRepeating {false}; };
		static ActionRecrod g_actionData[ACT_ENUM_LAST];
		static std::atomic_uint g_nextRepeaterId;

		friend class Plugin;
		Q_DISABLE_COPY_MOVE(DSE)
};

Q_DECLARE_METATYPE(DSE*);
