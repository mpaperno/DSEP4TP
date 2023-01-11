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

#include "utils.h"

#define PLUGIN_STATE_ID_PREFIX  "dsep."

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
		//! The scope of the current script's engine, either "Shared" or "Private".
		Q_PROPERTY(QString INSTANCE_TYPE READ instanceTypeStr CONSTANT)
		//! The name of the current script's instance (State Name), as specified in the corresponding
  	//! Touch Portal action which is running the script/expression.
		Q_PROPERTY(QString INSTANCE_NAME MEMBER instanceName CONSTANT)
		//! This is the default value as specified in the action which created this script instance, which may be empty/blank (but not null/undefined).
    //! \note This property is empty in Shared instance types. \sa `DSE.INSTANCE_TYPE`
		Q_PROPERTY(QString INSTANCE_DEFAULT_VALUE READ instanceDefault CONSTANT)
		//! The prefix added by the plugin to an instance's State Name before sending to Touch Portal to ensure uniqueness. Touch Portal uses the unique ID
    //! to identify States. \sa `DSE.instanceStateId()`
		Q_PROPERTY(QString VALUE_STATE_PREFIX READ valueStatePrefix CONSTANT)
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
		Q_PROPERTY(QString TP_USER_DATA_PATH READ tpDataPath CONSTANT)
		//! Numeric version of currently connected Touch Portal desktop application. eg. `301007`
		Q_PROPERTY(quint32 TP_VERSION_CODE MEMBER tpVersion CONSTANT)
		//! String version of currently connected Touch Portal desktop application. eg. "3.1.7.0.0"
		Q_PROPERTY(QString TP_VERSION_STR MEMBER tpVersionStr CONSTANT)

		Q_PROPERTY(DSE::EngineInstanceType instanceType READ instanceType CONSTANT)
		Q_PROPERTY(int defaultActionRepeatRate MEMBER defaultRepeatRate WRITE setDefaultActionRepeatRate NOTIFY defaultActionRepeatRateChanged)
		Q_PROPERTY(int defaultActionRepeatDelay MEMBER defaultRepeatDelay WRITE setDefaultActionRepeatDelay NOTIFY defaultActionRepeatDelayChanged)

		Q_PROPERTY(quint8 Unknown READ EngineInstanceType_Unknown CONSTANT)
		Q_PROPERTY(quint8 Shared  READ EngineInstanceType_Shared CONSTANT)
		Q_PROPERTY(quint8 Private READ EngineInstanceType_Private CONSTANT)
		Q_PROPERTY(quint8 Expression READ ScriptInputType_Expression CONSTANT)
		Q_PROPERTY(quint8 Script  READ ScriptInputType_Script CONSTANT)
		Q_PROPERTY(quint8 Module READ ScriptInputType_Module CONSTANT)
		Q_PROPERTY(quint8 NoDefault READ ScriptDefaultType_NoDefault CONSTANT)
		Q_PROPERTY(quint8 FixedValue  READ ScriptDefaultType_FixedValue CONSTANT)
		Q_PROPERTY(quint8 CustomExpression READ ScriptDefaultType_CustomExpression CONSTANT)
		Q_PROPERTY(quint8 MainExpression READ ScriptDefaultType_MainExpression CONSTANT)

		ScriptEngine *se = nullptr;

	public:
		using ScriptState = QHash<QByteArray, DynamicScript *>;

		//! Type of script engine instances.
		enum class EngineInstanceType : quint8 {
			Unknown,   //!< Unknown engine instance type.
			Shared,    //!< Shared engine instance type.
			Private,   //!< Private engine instance type.
		};
		Q_ENUM(EngineInstanceType)

		static quint8 EngineInstanceType_Unknown() { return (quint8)EngineInstanceType::Unknown; }
		static quint8 EngineInstanceType_Shared()  { return (quint8)EngineInstanceType::Shared; }
		static quint8 EngineInstanceType_Private() { return (quint8)EngineInstanceType::Private; }

		//! Input types for script actions.
		enum class ScriptInputType : quint8 {
			Unknown,      //!< Unknown script input type.
			Expression,   //!< Expression input type.
			Script,       //!< Script file input type.
			Module,       //!< Module file script input type.
		};
		Q_ENUM(ScriptInputType)
		static quint8 ScriptInputType_Unknown()    { return (quint8)ScriptInputType::Unknown; }
		static quint8 ScriptInputType_Expression() { return (quint8)ScriptInputType::Expression; }
		static quint8 ScriptInputType_Script()     { return (quint8)ScriptInputType::Script; }
		static quint8 ScriptInputType_Module()     { return (quint8)ScriptInputType::Module; }

		//! Named instance default value types.
		enum class ScriptDefaultType : quint8 {
			NoDefault,          //!< No default, instance not saved or restored.
			FixedValue,         //!< Instance is saved, and restored with a fixed default value (specified in `DynamicScript.defaultValue`)
			CustomExpression,   //!< Instance is saved, and restored by evaluating a custom expression (specified in `DynamicScript.defaultValue`)
			MainExpression,     //!< Instance is saved, and restored by evaluating the same expression as specified for the last action to invoke this instance.
		};
		Q_ENUM(ScriptDefaultType)
		static quint8 ScriptDefaultType_NoDefault()        { return (quint8)ScriptDefaultType::NoDefault; }
		static quint8 ScriptDefaultType_FixedValue()       { return (quint8)ScriptDefaultType::FixedValue; }
		static quint8 ScriptDefaultType_CustomExpression() { return (quint8)ScriptDefaultType::CustomExpression; }
		static quint8 ScriptDefaultType_MainExpression()   { return (quint8)ScriptDefaultType::MainExpression; }

		static const quint32 pluginVersion;
		static const QByteArray pluginVersionStr;
		static const QString platformOs;
		static quint32 tpVersion;
		static QString tpVersionStr;
		static QString scriptsBaseDir;
		static QByteArray tpCurrentPage;
		static std::atomic_int defaultRepeatRate;
		static std::atomic_int defaultRepeatDelay;

		bool privateInstance { false };
		QByteArray instanceName;

		explicit DSE(ScriptEngine *se = nullptr, QObject *p = nullptr);

		static ScriptState *instances();
		static const ScriptState &instances_const();
		static QByteArrayList instanceKeys();

		//! Returns all currently existing instance names as an array of strings.
		Q_INVOKABLE static QVariantList instanceNames();
		//! Returns all currently existing instances.
		Q_INVOKABLE static QList<DynamicScript *> instanceList();
		//! Returns the instances with given `name`, if any, otherwise returns `null`.
		Q_INVOKABLE static DynamicScript *instance(const QByteArray &name);

		static inline QString valueStatePrefix() { return QStringLiteral(PLUGIN_STATE_ID_PREFIX); }
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

		//! Returns the current instance. This is equivalent to calling `DSE.instance(DSE.INSTANCE_NAME)`.
		//! \note In some cases when a script is running in the `Shared` engine instance type,
		//! this function may return a `null` result, such as when working inside asyncronous methods like with a `Promise`.
		//! If you need to access the current instance in such cases, you should save a reference to the instance before invoking any async methods
		//! and use the saved instace inside them. Or use the `instance(String name)` overload with a static string value for the name.
		Q_INVOKABLE DynamicScript *instance() const { return instance(instanceName); }
		//! Returns the engine type associated with the current instance, either `DSE.Private` or `DSE.Shared`. This returns the same result as `DSE.INSTANCE_TYPE` but in numeric format.
		DSE::EngineInstanceType instanceType() const { return privateInstance ? EngineInstanceType::Private : EngineInstanceType::Shared; };
		inline QString instanceTypeStr() const { return instanceTypeMeta().key((int)instanceType()); };
		//! Gets Touch Portal State ID of the current script's instance.
    //! This is what Touch Portal actually uses to uniquely identify the state (not just the name by itself).
  	//! This is a convenience method that returns the same as `DSE.VALUE_STATE_PREFIX + DSE.INSTANCE_NAME`.
		Q_INVOKABLE QString instanceStateId() { return valueStatePrefix() + instanceName; }
		QByteArray instanceDefault() const;

	public Q_SLOTS:
		//! Set the global default action repeat rate (interval) to `ms` milliseconds. Minimum interval is 50ms.
		void setDefaultActionRepeatRate(int ms);
		//! Set the global default action repeat delay to `ms` milliseconds.
		//! This is how long to pause before starting to repeat an action the first time, after which the repeat reate (interval) is used. Minimum delay is 50ms.
		void setDefaultActionRepeatDelay(int ms);

	Q_SIGNALS:
		void defaultActionRepeatRateChanged(int ms);
		void defaultActionRepeatDelayChanged(int ms);

};

Q_DECLARE_METATYPE(DSE*);
Q_DECLARE_METATYPE(DSE::EngineInstanceType)
Q_DECLARE_METATYPE(DSE::ScriptInputType)
Q_DECLARE_METATYPE(DSE::ScriptDefaultType)
