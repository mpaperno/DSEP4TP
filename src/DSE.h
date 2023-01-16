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

#define PLUGIN_STATE_ID_PREFIX        "dsep."
#define PLUGIN_DYNAMIC_STATES_PARENT  "Dynamic Values"

#ifdef DOXYGEN
#define QByteArray String
#define QVariantList Array<String>
#endif

class DynamicScript;
class ScriptEngine;

//! \class DSE
//! \ingroup PluginAPI
//! The DSE object contains constants and functions related to the plugin environment.
//! It can be used to get or set properties of any existing script instance.
class DSE : public QObject
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
		Q_PROPERTY(QString VALUE_STATE_PREFIX READ valueStatePrefix CONSTANT)
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
		//! The repeat rate takes effect _after_ the initial repeat delay. Minimum interval is 50ms. \sa defaultActionRepeatDelay, DynamicScript.repeatRate
		//! \since v1.1
		Q_PROPERTY(int defaultActionRepeatRate READ defaultActionRepeatRate WRITE setDefaultActionRepeatRate NOTIFY defaultActionRepeatRateChanged)
		//! The global default action repeat delay, in milliseconds. The delay is the amount of time before a held action starts repeating, after the initial activation.
		//! After this initial delay, the action will be repeated at the current repeat rate. Minimum delay is 50ms. \sa defaultActionRepeatRate, DynamicScript.repeatDelay
		//! \since v1.1
		Q_PROPERTY(int defaultActionRepeatDelay READ defaultActionRepeatDelay WRITE setDefaultActionRepeatDelay NOTIFY defaultActionRepeatDelayChanged)

		//! Returns the engine instance type associated with the current script instance, one of DSE.EngineInstanceType enum values: `DSE.PrivateInstance` or `DSE.SharedInstance`. \sa DynamicScript.engineType
		//! \since v1.1
		Q_PROPERTY(DSE::EngineInstanceType engineInstanceType READ instanceType CONSTANT)
		//! Returns the name of the engine instance associated with the current script instance. For the global shared engine this is always "Shared".
		//! For private engine instances, this may or may not be the same as the script's Instance Name,
		//! for example if a script action was set to use a specific private named engine instance instead of just "Private".
		//! Unlike \ref currentInstanceName property, this value will always be constant, even inside asyncronous methods.  \sa currentInstanceName, DynamicScript.engineName
		//! \since v1.1
		Q_PROPERTY(QString engineInstanceName READ engineInstanceName CONSTANT)
		//! The Instance Name of the script currently being evaluated, as specified in the corresponding Touch Portal action which invoked the script/expression.
		//! This is essentially the same as calling `DSE.currentInstance()?.name`.
		//! \note In some cases this property may return an empty, or even incorrect, result, such as when working inside asyncronous methods like with a `Promise`.
		//! In such cases it is best to save the intance name to a local constant variable before entering the asyncronous method.  \sa engineInstanceName, currentInstance(), DynamicScript.name
		//! \since v1.1
		Q_PROPERTY(QString currentInstanceName READ currentInstanceName CONSTANT)
		//! Alias for \ref engineInstanceType. \sa DynamicScript.engineType  \since v1.1
		Q_PROPERTY(DSE::EngineInstanceType currentInstanceType READ instanceType CONSTANT)

		//! The scope of the current script's engine, either "Shared" or "Private".
		//! \deprecated{v1.1}
		//! This property is deprecated and may be removed in a future version; Use `DSE.engineInstanceType` or `DSE.currentInstace()?.engineType` instead which return enumeration values instead of strings.
		Q_PROPERTY(QString INSTANCE_TYPE READ instanceTypeStr CONSTANT)
		//! The the current script's Instance Name, as specified in the corresponding Touch Portal action which is running the script/expression.
		//! \deprecated{v1.1}
		//! This property is deprecated and may be removed in a future version; Use `DSE.currentInstanceName` or `DSE.currentInstace()?.name` instead.
		Q_PROPERTY(QString INSTANCE_NAME MEMBER instanceName CONSTANT)
		//! This is the default value as specified in the action which created this script instance, which may be empty/blank (but not null/undefined). This property is empty in Shared instance types.
		//! \deprecated{v1.1}
		//! This property is deprecated and may be removed in a future version; Use `DSE.currentInstance()?.defaultValue` (for example) instead, which is more reliable and works in shared engine instances.
		Q_PROPERTY(QString INSTANCE_DEFAULT_VALUE READ instanceDefault CONSTANT)

	public:
		using ScriptState = QHash<QByteArray, DynamicScript *>;
		using EngineState = QHash<QByteArray, ScriptEngine *>;


		// The enum properties is a workaround for enum types not being available in jsengine.

		//! Type of script engine instances.
		enum EngineInstanceType : quint8 {
			UnknownInstanceType,  //!< Unknown engine instance type.
			SharedInstance,       //!< Shared engine instance type.
			PrivateInstance,      //!< Private engine instance type.
		};
		Q_ENUM(EngineInstanceType)
		static EngineInstanceType EngineInstanceType_UnknownInstanceType() { return UnknownInstanceType; }
		static EngineInstanceType EngineInstanceType_SharedInstance()      { return SharedInstance; }
		static EngineInstanceType EngineInstanceType_PrivateInstance()     { return PrivateInstance; }
#ifndef DOXYGEN
		Q_PROPERTY(EngineInstanceType UnknownInstanceType READ EngineInstanceType_UnknownInstanceType CONSTANT)
		Q_PROPERTY(EngineInstanceType SharedInstance      READ EngineInstanceType_SharedInstance      CONSTANT)
		Q_PROPERTY(EngineInstanceType PrivateInstance     READ EngineInstanceType_PrivateInstance     CONSTANT)
#endif

		//! Input types for script actions.
		enum ScriptInputType : quint8 {
			UnknownInputType,      //!< Unknown script input type.
			ExpressionInput,   //!< Expression input type.
			ScriptInput,       //!< Script file input type.
			ModuleInput,       //!< Module file script input type.
		};
		Q_ENUM(ScriptInputType)
		static ScriptInputType ScriptInputType_UnknownInputType() { return UnknownInputType; }
		static ScriptInputType ScriptInputType_ExpressionInput()  { return ExpressionInput; }
		static ScriptInputType ScriptInputType_ScriptInput()      { return ScriptInput; }
		static ScriptInputType ScriptInputType_ModuleInput()      { return ModuleInput; }
#ifndef DOXYGEN
		Q_PROPERTY(ScriptInputType UnknownInputType  READ ScriptInputType_UnknownInputType CONSTANT)
		Q_PROPERTY(ScriptInputType ExpressionInput   READ ScriptInputType_ExpressionInput  CONSTANT)
		Q_PROPERTY(ScriptInputType ScriptInput       READ ScriptInputType_ScriptInput      CONSTANT)
		Q_PROPERTY(ScriptInputType ModuleInput       READ ScriptInputType_ModuleInput      CONSTANT)
#endif

		//! Named instance default value types.
		enum ScriptDefaultType : quint8 {
			NoSavedDefault,      //!< No default, instance not saved or restored.
			FixedValueDefault,   //!< Instance is saved, and restored with a fixed default value (specified in `DynamicScript.defaultValue`)
			CustomExprDefault,   //!< Instance is saved, and restored by evaluating a custom expression (specified in `DynamicScript.defaultValue`)
			MainExprDefault,     //!< Instance is saved, and restored by evaluating the same expression as specified for the last action to invoke this instance.
		};
		Q_ENUM(ScriptDefaultType)
		static ScriptDefaultType ScriptDefaultType_NoSavedDefault()    { return NoSavedDefault; }
		static ScriptDefaultType ScriptDefaultType_FixedValueDefault() { return FixedValueDefault; }
		static ScriptDefaultType ScriptDefaultType_CustomExprDefault() { return CustomExprDefault; }
		static ScriptDefaultType ScriptDefaultType_MainExprDefault()   { return MainExprDefault; }
#ifndef DOXYGEN
		Q_PROPERTY(ScriptDefaultType NoSavedDefault     READ ScriptDefaultType_NoSavedDefault    CONSTANT)
		Q_PROPERTY(ScriptDefaultType FixedValueDefault  READ ScriptDefaultType_FixedValueDefault CONSTANT)
		Q_PROPERTY(ScriptDefaultType CustomExprDefault  READ ScriptDefaultType_CustomExprDefault CONSTANT)
		Q_PROPERTY(ScriptDefaultType MainExprDefault    READ ScriptDefaultType_MainExprDefault   CONSTANT)
#endif


		//! Action repeat property type.
		enum RepeatProperty : quint8 {
			RepeatRateProperty = 0x01,      //!< Rate, or pause interval between repetitions, in milliseconds.
			RepeatDelayProperty = 0x02,     //!< Initial delay before the the first repetition is activated, in milliseconds.
			AllRepeatProperties = RepeatRateProperty | RepeatDelayProperty   //!< OR combination of Rate and Delay properties.
		};
		Q_ENUM(RepeatProperty)
		static RepeatProperty RepeatProperty_RepeatRateProperty()   { return RepeatRateProperty; }
		static RepeatProperty RepeatProperty_RepeatDelayProperty()  { return RepeatDelayProperty; }
		static RepeatProperty RepeatProperty_AllRepeatProperties()  { return AllRepeatProperties; }
#ifndef DOXYGEN
		Q_PROPERTY(RepeatProperty RepeatRateProperty  READ RepeatProperty_RepeatRateProperty  CONSTANT)
		Q_PROPERTY(RepeatProperty RepeatDelayProperty READ RepeatProperty_RepeatDelayProperty CONSTANT)
		Q_PROPERTY(RepeatProperty AllRepeatProperties READ RepeatProperty_AllRepeatProperties CONSTANT)
#endif

		//! How to "adjust" or set a value, eg. in an absolute or relative fashion.
		enum AdjustmentType : quint8 {
			SetAbsolute,   //!< Set something to a specific given value.
			SetRelative,   //!< Set something relative to another value; eg. add a positive or negative amount to a current value.
			Increment,     //!< Set something relative to another value by increasing it by a given value. More specific than `SetRelative`.
			Decrement      //!< Set something relative to another value by decreasing it by a given value. More specific than `SetRelative`.
		};
		Q_ENUM(AdjustmentType)
		static AdjustmentType AdjustmentType_SetAbsolute() { return SetAbsolute; }
		static AdjustmentType AdjustmentType_SetRelative() { return SetRelative; }
		static AdjustmentType AdjustmentType_Increment()   { return Increment; }
		static AdjustmentType AdjustmentType_Decrement()   { return Decrement; }
#ifndef DOXYGEN
		Q_PROPERTY(AdjustmentType SetAbsolute READ AdjustmentType_SetAbsolute CONSTANT)
		Q_PROPERTY(AdjustmentType SetRelative READ AdjustmentType_SetRelative CONSTANT)
		Q_PROPERTY(AdjustmentType Increment   READ AdjustmentType_Increment   CONSTANT)
		Q_PROPERTY(AdjustmentType Decrement   READ AdjustmentType_Decrement   CONSTANT)
#endif


		static const quint32 pluginVersion;
		static const QByteArray pluginVersionStr;
		static const QString platformOs;
		static quint32 tpVersion;
		static QString tpVersionStr;
		static QString scriptsBaseDir;
		static QByteArray tpCurrentPage;
		static std::atomic_int defaultRepeatRate;
		static std::atomic_int defaultRepeatDelay;

		static DSE *sharedInstance;

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
		//! \since v1.1
		Q_INVOKABLE static QVariantList instanceNames();
		//! Returns all currently existing script instances as an iteratable array-like object (iterate with `of` eg. `for (const ds of DSE.instanceList()) ...`).
		//! \since v1.1
		Q_INVOKABLE static QList<DynamicScript *> instanceList();
		//! Returns the script instance with given `name`, if any, otherwise returns `null`.
		//! \since v1.1
		Q_INVOKABLE static DynamicScript *instance(const QByteArray &name);
		//! Returns the current script Instance. This is equivalent to calling `DSE.instance(DSE.currentInstanceName)`.
		//! \note In some cases this function may return a `null` result, such as when working inside asyncronous methods like with a `Promise`.
		//! If you need to access the current instance in such cases, you should save a reference to the instance before invoking any async methods
		//! and use the saved instace inside them. Or use the `instance(String name)` overload with a static string value for the instance name.
		//! \since v1.1
		Q_INVOKABLE DynamicScript *currentInstance() const { return instance(instanceName); }
		DSE::EngineInstanceType instanceType() const { return privateInstance ? PrivateInstance : SharedInstance; };
		QByteArray currentInstanceName() const { return instanceName; }

		QByteArray engineInstanceName() const;

		static inline QString valueStatePrefix() { return QStringLiteral(PLUGIN_STATE_ID_PREFIX); }
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

		static inline const QMetaEnum inputTypeMeta() { static const QMetaEnum m = QMetaEnum::fromType<DSE::ScriptInputType>(); return m; }
		static inline const QMetaEnum instanceTypeMeta() { static const QMetaEnum m = QMetaEnum::fromType<DSE::EngineInstanceType>(); return m; }
		static inline const QMetaEnum defaultTypeMeta() { static const QMetaEnum m = QMetaEnum::fromType<DSE::ScriptDefaultType>(); return m; }

		static int defaultActionRepeatRate() { return defaultRepeatRate; }
		static void setDefaultActionRepeatRate(int ms);
		static int defaultActionRepeatDelay() { return defaultRepeatDelay; }
		static void setDefaultActionRepeatDelay(int ms);

		static int defaultActionRepeatProperty(quint8 property) { return (property & DSE::RepeatRateProperty) ? defaultRepeatRate : defaultRepeatDelay; }
		static void setDefaultActionRepeatProperty(quint8 property, int ms)
		{
			if (property & DSE::RepeatRateProperty)
				setDefaultActionRepeatRate(ms);
			if (property & DSE::RepeatDelayProperty)
				setDefaultActionRepeatDelay(ms);
		}

		// deprecated, remove
		inline QString instanceTypeStr() const { return instanceTypeMeta().key((int)instanceType()); };
		Q_INVOKABLE QString instanceStateId() { return valueStatePrefix() + instanceName; }
		QByteArray instanceDefault() const;

	public Q_SLOTS:
		//! Convenience method to set the action repeat rate and/or delay prooperties on either the global default or a specific named script instance.
		//! \param property should be one of `DSE.RepeatRateProperty`, `DSE.RepeatDelayProperty` or `DSE.AllRepeatProperties` to set both rate and delay at the same time.
		//! \param ms is the new property value, in milliseconds (minimum is 50ms).
		//! \param forInstance can be a script Instance Name, or empty (default) to set the global default repeat properties.
		//! \sa adjustActionRepeat() \since v1.1
		void setActionRepeat(quint8 property, int ms, const QByteArray &forInstance = QByteArray()) {
			setActionRepeat(property, SetAbsolute, ms, forInstance);
		}
		//! Convenience method to adjust the action repeat rate and/or delay prooperties on either the global default or a specific named script instance.
		//! \param property should be one of `DSE.RepeatRateProperty`, `DSE.RepeatDelayProperty` or `DSE.AllRepeatProperties` to set both rate and delay at the same time.
		//! \param byMs is the adjustment amount, positive or negative, in milliseconds.
		//! \param forInstance can be a script Instance Name, or empty (default) to set the global default repeat properties.
		//! \sa setActionRepeat() \since v1.1
		void adjustActionRepeat(quint8 property, int byMs, const QByteArray &forInstance = QByteArray()) {
			setActionRepeat(property, SetRelative, byMs, forInstance);
		}

		//! Convenience method to set or adjust the action repeat rate and/or delay prooperties on either the global default or a specific named script instance.
		//! \n `property` should be one of `DSE.RepeatRateProperty`, `DSE.RepeatDelayProperty` or `DSE.AllRepeatProperties` to set both rate and delay at the same time.
		//! \n `action` can be one of `DSE.SetAbsolute`, `DSE.SetRelative`, `DSE.Increment` or `DSE.Decrement` (see linked documentation for details on each action type).
		//! \n `ms` is the set (absolute) or adjustment (relative) amount, positive or negative, in milliseconds.
		//! \n `forInstance` can be a script Instance Name, or empty (default) to set the global default repeat properties.
		//! \sa setActionRepeat(), adjustActionRepeat() \since v1.1
		void setActionRepeat(quint8 property, quint8 action, int ms, const QByteArray &forInstance = QByteArray(), bool repeat = false);

		// not public API
		void cancelRepeatingAction(quint8 act = DSE::ACT_ADJ_REPEAT);

	Q_SIGNALS:
		void defaultActionRepeatRateChanged(int ms);
		void defaultActionRepeatDelayChanged(int ms);

	private Q_SLOTS:
		void setActionRepeat_impl(quint8 property, quint8 action, int ms, const QByteArray &forInstance, uint repeaterId);

	private:
		ScriptEngine *se = nullptr;

		enum ActionID { ACT_ADJ_REPEAT, ACT_ENUM_LAST };
		struct ActionRecrod { std::atomic_uint repeaterId {0}; std::atomic_bool isRepeating {false}; };
		ActionRecrod m_actionData[ACT_ENUM_LAST] { ActionRecrod() };
		std::atomic_uint m_nextRepeaterId {0};

		Q_DISABLE_COPY_MOVE(DSE)
};

Q_DECLARE_METATYPE(DSE*);
Q_DECLARE_METATYPE(DSE::EngineInstanceType)
Q_DECLARE_METATYPE(DSE::ScriptInputType)
Q_DECLARE_METATYPE(DSE::ScriptDefaultType)
Q_DECLARE_METATYPE(DSE::RepeatProperty)
Q_DECLARE_METATYPE(DSE::AdjustmentType)
