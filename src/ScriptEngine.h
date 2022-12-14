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
#include <QJSValue>
#include <QJSValueIterator>
#include <QObject>
#include <QMutex>
#include <QNetworkAccessManager>

#include "common.h"

#define SCRIPT_ENGINE_CHECK_ERRORS(JSE) \
	if (ScriptEngine *_scriptEngine = JSE->property("ScriptEngine").value<ScriptEngine *>()) { \
		_scriptEngine->checkErrors(); }


#define SCRIPT_ENGINE_FORMAT_ERR_MSG(JSVAL, ...) [&]() {          \
	const ScriptEngine::JSError err = ScriptEngine::jsError(JSVAL); \
	QString errorMessage = err.name + ": " + err.message;           \
	if (!err.fileName.isEmpty()) {                                  \
		errorMessage += " (in file '" + err.fileName + '\'';          \
		if (!err.lineNumber.isEmpty())                                \
			errorMessage += " at line " + err.lineNumber;               \
		errorMessage += __VA_ARGS__ ")";                              \
	}                                                               \
  return errorMessage;                                            \
}()


#if SCRIPT_ENGINE_USE_QML
// Used with QQmlEngine for XMLHttpRequest
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

namespace ScriptLib {
	class Util;
	struct TimerData;
}

class ConnectorData;
class ConnectorRecord;

class ScriptEngine : public QObject
{
	Q_OBJECT
	public:
		struct JSError
		{
			QJSValue::ErrorType type = QJSValue::NoError;
			QString name;
			QString message;
			QString fileName;
			QString lineNumber;
			QString stack;
			QJSValue cause;
			explicit JSError(const QJSValue &err) {
				QJSValue tmp;
				type = err.isError() ? err.errorType() : QJSValue::GenericError;
				if (!(tmp = err.property(QStringLiteral("name"))).isUndefined())
					name = tmp.toString();
				else
					name = QStringLiteral("Error");
				if (!(tmp = err.property(QStringLiteral("message"))).isUndefined())
					message = tmp.toString();
				else
					message = err.toString();
				if (!(tmp = err.property(QStringLiteral("fileName"))).isUndefined())
					fileName = tmp.toString();
				if (!(tmp = err.property(QStringLiteral("lineNumber"))).isUndefined())
					lineNumber = tmp.toString();
				if (!(tmp = err.property(QStringLiteral("stack"))).isUndefined())
					stack = tmp.toString();
				if (err.hasOwnProperty(QStringLiteral("cause")))
					cause = err.property(QStringLiteral("cause"));
			}
		};

		static ScriptEngine *instance()
		{
			static ScriptEngine e(true);
			return &e;
		}

		explicit ScriptEngine(const QByteArray &instanceName = QByteArray(), QObject *p = nullptr) :
		  ScriptEngine(false, instanceName, p)
		{ }
		~ScriptEngine();

		inline QJSEngine *engine() const { return se; }
		inline QJSValue globalObject() const { return se ? se->globalObject() : QJSValue(); }
		inline QJSValue dseObject() const { return globalObject().property("DSE"); }
		inline bool isSharedInstance() const { return m_isShared; }
		inline QByteArray currentInstanceName() const { return dseObject().property(QStringLiteral("INSTANCE_NAME")).toString().toUtf8(); }

		Q_INVOKABLE ConnectorRecord getConnectorByShortId(QJSValue shortId);
		Q_INVOKABLE QStringList getConnectorShortIds(QJSValue query = QJSValue());
		Q_INVOKABLE QVector<ConnectorRecord> getConnectorRecords(QJSValue query = QJSValue());
		Q_INVOKABLE static QString getTpCurrentPageName();

		static inline JSError jsError(const QJSValue &err) { return JSError(err); }

	Q_SIGNALS:
		void raiseError(const QJSValue &err) const;
		// void resultReady(const QJSValue &val);

		void stateValueUpdate(const QByteArray &);
		void stateValueUpdateByName(const QByteArray &, const QByteArray &);
		void stateValueUpdateById(const QByteArray &, const QByteArray &);
		void stateCreate(const QByteArray &, const QByteArray &, const QByteArray &, const QByteArray &);
		void stateRemove(const QByteArray &);
		void choiceUpdate(const QByteArray &, const QStringList &);
		void connectorUpdate(const QByteArray &, uint8_t, bool = false);
		void connectorUpdateShort(const QByteArray &, uint8_t);

		void tpNotification(const QByteArray &, const QByteArray &, const QByteArray &, const QVariantList & = QVariantList());
		void tpBroadcast(const QString &, const QVariantMap &);

		void connectorIdsChanged(const QByteArray &instanceName, const QByteArray &shortId);

	public Q_SLOTS:
		inline void reset() { initScriptEngine(); }
		void clearInstanceData(const QByteArray &name);
		void checkErrors() const;
		void throwError(const QJSValue &err) const;
		void throwError(QJSValue err, const QByteArray &instName) const;
		void throwError(QJSValue::ErrorType type, const QString &msg, const QJSValue &cause, const QByteArray &instName = QByteArray()) const;
		void throwError(QJSValue::ErrorType type, const QString &msg, const QByteArray &instName) const;
		void throwError(QJSValue::ErrorType type, const QString &msg) const;
		//void onScriptResultReady(const QVariant &vres) { if (se) emit resultReady(se->toScriptValue(vres)); }

		void stateUpdate(const QByteArray &value) { emit stateValueUpdate(value); }
		void stateUpdate(const QByteArray &name, const QByteArray &value) { emit stateValueUpdateByName(name, value); }
		void stateUpdateById(const QByteArray &id, const QByteArray &value) { emit stateValueUpdateById(id, value); }

		void showNotification(const QByteArray &id, const QByteArray &title, const QByteArray &msg, QVariantList options = QVariantList(), QJSValue callback = QJSValue());
		void onNotificationClicked(const QString &notifyId, const QString &optionId);

		QJSValue expressionValue(const QString &fromValue, const QByteArray &instName = QByteArray());
		QJSValue scriptValue(const QString &fileName, const QString &expr, const QByteArray &instName = QByteArray());
		QJSValue moduleValue(const QString &fileName, const QString &alias, const QString &expr, const QByteArray &instName = QByteArray());
		bool timerExpression(const ScriptLib::TimerData *timData);
		void include(const QString &file) const;

		static void checkErrors(QJSEngine *e) {
			if (!e)
				return;
			if (ScriptEngine *se = e->property("ScriptEngine").value<ScriptEngine *>()) {
				se->checkErrors(); }
		}

		static void checkErrors(ScriptEngine *se) { if (se) se->checkErrors(); }

	private:
		SCRIPT_ENGINE_BASE_TYPE *se = nullptr;
		ScriptLib::Util *ulib = nullptr;
		ConnectorData *connData = nullptr;
		QByteArray m_currInstanceName;
		bool m_isShared = false;
		QMutex m_mutex;
		QHash<QByteArray, QJSValue> m_notificationCallbacks;
#if SCRIPT_ENGINE_USE_QML
		NetworkAccessManagerFactory m_factory;
#endif

		ScriptEngine(bool isStatic, const QByteArray &instanceName = QByteArray(), QObject *p = nullptr);
		void initScriptEngine();
		QVariantMap initConnectorQuery(QJSValue query, ConnectorData **cdata);
		ConnectorData *connectorData();

		inline void setInstanceProperties(const QByteArray &instName)
		{
			if (m_isShared) {
				dseObject().setProperty(QStringLiteral("INSTANCE_NAME"), QLatin1String(instName));
			}
		}
		inline void resetInstanceProperties()
		{
			if (m_isShared) {
				dseObject().setProperty(QStringLiteral("INSTANCE_NAME"), QLatin1String(""));
			}
		}

		void evalScript(const QString &fn) const
		{
			bool ok;
			const QString script = readFile(fn, &ok);
			if (!ok) {
				qCWarning(lcPlugin) << script;
				return;
			}
			const QJSValue res = se->evaluate(script, fn);
			if (res.isError())
				qCCritical(lcPlugin) << "Exception in script" << fn << "at line" << res.property("lineNumber").toInt() << ":" << res.toString();
		}

		inline QString readFile(const QString &fn, bool *ok = nullptr) const
		{
			QFile scriptFile(fn);
			if (!scriptFile.open(QIODevice::ReadOnly)) {
				if (ok)
					*ok = false;
				return "Error opening file '" + fn + "': " + scriptFile.errorString();
			}
			QString ret = scriptFile.readAll();
			scriptFile.close();
			if (ok)
				*ok = true;
			return ret;
		}

};
