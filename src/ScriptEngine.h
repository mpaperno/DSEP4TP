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

#define SCRIPT_ENGINE_USE_QML     0

#if SCRIPT_ENGINE_USE_QML
	#define SCRIPT_ENGINE_BASE_TYPE   QQmlEngine
	#include <QQmlEngine>
	#include <QQmlComponent>
#else
	#define SCRIPT_ENGINE_BASE_TYPE   QJSEngine
	#include <QJSEngine>
#endif

#include <QFile>
#include <QJsonDocument>
#include <QJSValue>
#include <QJSValueIterator>
#include <QMetaObject>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QObject>
#include <QThread>

#include "common.h"
#include "DSE_NS.h"
#include "JSError.h"
#include "ScriptLibNS.h"

class DSE;
class DynamicScript;
class ScriptEngine;

namespace ScriptLib {
	class TPAPI;
	class Util;
	struct TimerData;
}

ScriptEngine *scriptEngine(const QJSEngine *jse);
ScriptEngine *scriptEngine(const QObject *o);

class ScriptEngine : public QObject
{
	Q_OBJECT

	public:
		static ScriptEngine *sharedInstance;
		static ScriptEngine *instance() { return sharedInstance; }

		explicit ScriptEngine(const QByteArray &instanceName = QByteArray(), QObject *p = nullptr);
		~ScriptEngine();

		inline QJSEngine *engine() const { return se; }
		inline QJSValue globalObject() const { return se ? se->globalObject() : QJSValue(); }
		inline QJSValue registeredModules() const { return globalObject().property("registeredModules"); }
		inline DSE *dseObject() const { return dse; }
		inline bool isSharedInstance() const { return m_isShared; }
		inline DseNS::EngineInstanceType instanceType() const { return m_isShared ? DseNS::EngineInstanceType::SharedInstance : DseNS::EngineInstanceType::PrivateInstance; }
		inline QByteArray name() const { return m_name; }
		inline ScriptLib::TPAPI *tpApiObject() const { return tpapi; }
		QByteArray currentInstanceName() const;
		// called by custom XmlHttpRequest implementation
		inline QNetworkAccessManager *networkAccessManager()
		{
			if (!m_nam)
				m_nam = new QNetworkAccessManager();
			return m_nam;
		}

		static QString stackTrace(QJSEngine *se);
		inline QString stackTrace() const { return stackTrace(engine()); }

	Q_SIGNALS:
		void raiseError(QJSValue err) const;
		void engineError(const JSError &err) const;
		void engineAboutToReset();
		void engineInitComplete();
		// void resultReady(const QJSValue &val);

	public Q_SLOTS:
		void reset() {
			Q_EMIT engineAboutToReset();
			initScriptEngine();
			qCInfo(lcPlugin) << (m_isShared ? "Shared" : "Private") << "Scripting Engine reset completed for" << name();
		}
		//void connectScriptInstance(DynamicScript *ds);
		void connectNamedScriptInstance(DynamicScript *ds);
		void disconnectNamedScriptInstance(DynamicScript *ds);
		void clearInstanceData(DynamicScript *ds);
		void checkErrors() const;
		void checkErrorsLater() const;

		void throwError(const QJSValue &err) const;
		void throwError(QJSValue err, const QByteArray &instName) const;
		void throwError(QJSValue::ErrorType type, const QString &msg, const QJSValue &cause, const QByteArray &instName = QByteArray()) const;
		void throwError(QJSValue::ErrorType type, const QString &msg, const QByteArray &instName) const;
		void throwError(QJSValue::ErrorType type, const QString &msg) const;

		void throwDomError(int code, const QString &msg, const QString &name = QString()) const;
		QJSValue newDomError(int code, const QString &msg, const QString &name = QString()) const;

		//! Calls GC with a mutex lock -- do not use synchronously from inside scripts.
		void collectGarbage();

		QJSValue expressionValue(const QString &fromValue, const QByteArray &instName = QByteArray());
		QJSValue scriptValue(const QString &fileName, const QString &expr, const QByteArray &instName = QByteArray());
		QJSValue moduleValue(const QString &fileName, const QString &alias, const QString &expr, const QByteArray &instName = QByteArray());
		bool timerExpression(const ScriptLib::TimerData *timData);
		void include(const QString &file) const;
		QJSValue require(const QString &file) const;

		// public static

		static inline void checkErrors(const QObject *o) { checkErrors(qjsEngine(o)); }
		static inline void checkErrors(QJSEngine *jse) {
			if (ScriptEngine *se = scriptEngine(jse))
				se->checkErrors();
		}

		static inline void checkErrorsLater(const QObject *o) { checkErrors(qjsEngine(o)); }
		static inline void checkErrorsLater(QJSEngine *jse) {
			if (ScriptEngine *se = scriptEngine(jse))
				se->checkErrorsLater();
		}

		static inline bool throwError(const QObject *o, const QJSValue &err)          { return throwError(qjsEngine(o), err); }
		static inline bool throwError(const QObject *o, int type, const QString &msg) { return throwError(qjsEngine(o), newErrorObject(o, type, msg)); }
		static inline bool throwError(QJSEngine *jse, int type, const QString &msg)   { return throwError(jse, newErrorObject(jse, type, msg)); }
		static bool throwError(QJSEngine *jse, const QJSValue &err);

		static inline QJSValue newErrorObject(const QObject *o, int type, const QString &msg) { return newErrorObject(qjsEngine(o), type, msg); }
		static QJSValue newErrorObject(QJSEngine *jse, int type, const QString &msg)
		{
			if (ScriptLib::isDomError(type))
				return newDomError(jse, ScriptLib::domFromCustomError(type), msg);
			if (jse)
				return jse->newErrorObject(QJSValue::ErrorType(type), msg);
			QJSValue err;
			err.setProperty("name", ScriptLib::errorTypeName(type));
			err.setProperty("message", msg);
			return err;
		}

		static inline QJSValue newDomError(const QObject *o, int code, const QString &msg, const QString &name = QString()) { return newDomError(qjsEngine(o), code, msg, name); }
		static QJSValue newDomError(QJSEngine *jse, int code, const QString &msg, const QString &name = QString());

	private:
		SCRIPT_ENGINE_BASE_TYPE *se = nullptr;
		DSE *dse = nullptr;
		ScriptLib::TPAPI *tpapi = nullptr;
		ScriptLib::Util *ulib = nullptr;
		QThread *m_thread = nullptr;
		QByteArray m_name;
		bool m_isShared = false;
		QMutex m_mutex;
		QNetworkAccessManager *m_nam = nullptr;

		void initScriptEngine();
		bool resolveFilePath(const QString &fileName, QString &resolvedFile) const;

		void evalScript(const QString &fn) const
		{
			bool ok;
			const QString script = QString::fromUtf8(readFile(fn, &ok));
			if (!ok) {
				qCWarning(lcPlugin) << script;
				return;
			}
			const QJSValue res = se->evaluate(script, fn);
			if (res.isError())
				qCCritical(lcPlugin) << "Exception in script" << fn << "at line" << res.property("lineNumber").toInt() << ":" << res.toString();
		}

		QJSValue loadModule(const QString &fn) const
		{
			const QJSValue res = se->importModule(fn);
			if (!res.isError())
				return res;
			qCCritical(lcPlugin) << "Exception in module" << fn << "at line" << res.property("lineNumber").toInt() << ":" << res.toString();
			return QJSValue();
		}

		void registerModule(const QString &name, QJSValue module)
		{
			QJSValue modules = registeredModules();
			modules.setProperty(name, std::move(module));
			se->registerModule(name, modules.property(name));
		}

		inline QByteArray readFile(const QString &fn, bool *ok = nullptr) const
		{
			QFile scriptFile(fn);
			if (!scriptFile.open(QIODevice::ReadOnly)) {
				if (ok)
					*ok = false;
				return ("Error opening file '" + fn + "': " + scriptFile.errorString()).toUtf8();
			}
			QByteArray ret = scriptFile.readAll();
			scriptFile.close();
			if (ok)
				*ok = true;
			return ret;
		}

		QJsonDocument loadJson(const QString &fileName, QString *error) const
		{
			bool ok;
			const QByteArray jsData = readFile(fileName, &ok);
			if (jsData.isEmpty()) {
				*error = jsData;
				return QJsonDocument();
			}

			QJsonParseError jspe;
			const QJsonDocument jsDoc = QJsonDocument::fromJson(jsData, &jspe);
			if (jsDoc.isNull())
				*error = "Error loading JSON: " + jspe.errorString() + ' ' + fileName + " @ " + QString::number(jspe.offset);
			return jsDoc;
		}

		Q_DISABLE_COPY(ScriptEngine)

};
