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
#include <QMutex>
#include <QObject>
#include <QThread>

#include "common.h"
#include "DSE.h"
#include "JSError.h"

#define SCRIPT_ENGINE_CHECK_ERRORS(JSE) \
	if (ScriptEngine *_scriptEngine = JSE->property("ScriptEngine").value<ScriptEngine *>()) { \
		_scriptEngine->checkErrors(); }


#if SCRIPT_ENGINE_USE_QML
// Used with QQmlEngine for XMLHttpRequest
#include <QNetworkAccessManager>
#include <QQmlNetworkAccessManagerFactory>
class NetworkAccessManagerFactory : public QQmlNetworkAccessManagerFactory
{
	private:
		QMutex m_mutex;
		QVector<QNetworkAccessManager *> m_managers {};

	public:
		QNetworkAccessManager *create(QObject * = nullptr) override
		{
			QMutexLocker lock(&m_mutex);
			QNetworkAccessManager *nam = new QNetworkAccessManager();
#if false && QT_CONFIG(networkproxy)
			if (!proxyHost.isEmpty()) {
				QNetworkProxy proxy(QNetworkProxy::HttpCachingProxy, proxyHost, proxyPort);
				nam->setProxy(proxy);
			}
#endif // networkproxy
			m_managers.append(nam);
			return nam;
		}

		~NetworkAccessManagerFactory() {
			QMutexLocker lock(&m_mutex);
			for (auto nam : qAsConst(m_managers))
				if (nam)
					nam->deleteLater();
		}
};
#endif

class DynamicScript;

namespace ScriptLib {
	class TPAPI;
	class Util;
	struct TimerData;
}

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
		inline DSE *dseObject() const { return dse; }
		inline bool isSharedInstance() const { return m_isShared; }
		inline DSE::EngineInstanceType instanceType() const { return m_isShared ? DSE::EngineInstanceType::SharedInstance : DSE::EngineInstanceType::PrivateInstance; }
		inline QByteArray name() const { return m_name; }
		inline QByteArray currentInstanceName() const { return dse->instanceName; }
		inline ScriptLib::TPAPI *tpApiObject() const { return tpapi; }

	Q_SIGNALS:
		void raiseError(QJSValue err) const;
		void engineError(const JSError &err) const;
		void engineAboutToReset();
		void engineInitComplete();
		// void resultReady(const QJSValue &val);

	public Q_SLOTS:
		inline void reset() {
			Q_EMIT engineAboutToReset();
			initScriptEngine();
			qCInfo(lcPlugin) << (m_isShared ? "Shared" : "Private") << "Scripting Engine reset completed for" << name();
		}
		//void connectScriptInstance(DynamicScript *ds);
		void connectNamedScriptInstance(DynamicScript *ds);
		void disconnectNamedScriptInstance(DynamicScript *ds);
		void clearInstanceData(const QByteArray &name);
		void checkErrors() const;
		void throwError(const QJSValue &err) const;
		void throwError(QJSValue err, const QByteArray &instName) const;
		void throwError(QJSValue::ErrorType type, const QString &msg, const QJSValue &cause, const QByteArray &instName = QByteArray()) const;
		void throwError(QJSValue::ErrorType type, const QString &msg, const QByteArray &instName) const;
		void throwError(QJSValue::ErrorType type, const QString &msg) const;
		//void onScriptResultReady(const QVariant &vres) { if (se) emit resultReady(se->toScriptValue(vres)); }

		QJSValue expressionValue(const QString &fromValue, const QByteArray &instName = QByteArray());
		QJSValue scriptValue(const QString &fileName, const QString &expr, const QByteArray &instName = QByteArray());
		QJSValue moduleValue(const QString &fileName, const QString &alias, const QString &expr, const QByteArray &instName = QByteArray());
		bool timerExpression(const ScriptLib::TimerData *timData);
		void include(const QString &file) const;
		QJSValue require(const QString &file) const;

		static void checkErrors(QJSEngine *e) {
			if (!e)
				return;
			if (ScriptEngine *se = e->property("ScriptEngine").value<ScriptEngine *>()) {
				se->checkErrors(); }
		}

		static void checkErrors(ScriptEngine *se) { if (se) se->checkErrors(); }

	private:
		SCRIPT_ENGINE_BASE_TYPE *se = nullptr;
		DSE *dse = nullptr;
		ScriptLib::TPAPI *tpapi = nullptr;
		ScriptLib::Util *ulib = nullptr;
		QThread *m_thread = nullptr;
		QByteArray m_name;
		bool m_isShared = false;
		QMutex m_mutex;
#if SCRIPT_ENGINE_USE_QML
		NetworkAccessManagerFactory m_factory;
#endif
		void initScriptEngine();
		void moveToMainThread();
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
