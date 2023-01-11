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

#include <QJSManagedValue>
#include <QObject>

//#include "common.h"
//#include "utils.h"
#include "Plugin.h"
#include "ScriptEngine.h"
#include "ConnectorData.h"
#include "DSE.h"
#include "DynamicScript.h"

#ifndef DOXYGEN
namespace ScriptLib {
#endif

class TPAPI : public QObject
{
		Q_OBJECT

	private:
		ScriptEngine *se = nullptr;
		ConnectorData *connData = nullptr;
		QHash<QByteArray, QJSValue> m_notificationCallbacks;

	public:
		explicit TPAPI(ScriptEngine *se = nullptr, QObject *p = nullptr) :
		  QObject(p), se(se)
		{
			setObjectName("DSE.TPAPI");
			connect(ConnectorData::instance(), &ConnectorData::connectorsUpdated, this, &TPAPI::connectorIdsChanged);
		}

		~TPAPI()
		{
			if (connData)
				connData->deleteLater();
		}

		void connectSignals(const Plugin *plugin, Qt::ConnectionType ctype = Qt::AutoConnection)
		{
			// Global from script engine which needs name lookup because the state name is not fully qualified.
			connect(this, &TPAPI::stateValueUpdateByName, plugin, &Plugin::onStateUpdateByName, ctype);
			// Direct(ish) connection to socket where state ID is already fully qualified;
			connect(this, &TPAPI::stateValueUpdateById, plugin, &Plugin::tpStateUpdate, ctype);
			// Other direct connections from eponymous script functions.
			connect(this, &TPAPI::stateCreate, plugin, &Plugin::tpStateCreate, ctype);
			connect(this, &TPAPI::stateRemove, plugin, &Plugin::tpStateRemove, ctype);
			connect(this, &TPAPI::choiceUpdate, plugin, &Plugin::tpChoiceUpdateStrList, ctype);
			connect(this, &TPAPI::connectorUpdateByLongId, plugin, &Plugin::tpConnectorUpdate, ctype);
			connect(this, &TPAPI::connectorUpdate, plugin, &Plugin::tpConnectorUpdateShort, ctype);
			connect(this, &TPAPI::tpNotification, plugin, &Plugin::tpNotification, ctype);
		}

		void connectInstance(const DynamicScript *ds)
		{
			// Global from script engine which needs name lookup because the state name is not fully qualified.
			connect(this, &TPAPI::stateValueUpdate, ds, &DynamicScript::onEngineValueUpdate);
		}

		void connectSlots(const Plugin *plugin, Qt::ConnectionType ctype = Qt::AutoConnection)
		{
			// Connect to notifications about TP events so they can be re-broadcast to scripts in this instance.
			connect(plugin, &Plugin::tpNotificationClicked, this, &TPAPI::onNotificationClicked, ctype);
			connect(plugin, &Plugin::tpBroadcast, this, &TPAPI::broadcastEvent, ctype);
		}

		Q_INVOKABLE ConnectorRecord getConnectorByShortId(QJSValue shortId)
		{
			if (!shortId.isString() || shortId.toString().isEmpty()) {
				se->throwError(QJSValue::TypeError, tr("Parameter must be a non-empty connector shortId or search pattern string."));
				return ConnectorRecord();
			}
			ConnectorData *cdata = connectorData();
			QString errMsg;
			const ConnectorRecord ret = cdata->getByShortId(shortId.toString().toUtf8(), &errMsg);
			if (!errMsg.isEmpty())
				se->throwError(QJSValue::TypeError, errMsg);
			return ret;
		}

		Q_INVOKABLE QStringList getConnectorShortIds(QJSValue query = QJSValue())
		{
			ConnectorData *cdata = nullptr;
			const QVariantMap q = initConnectorQuery(query, &cdata);
			if (!cdata)
				return QStringList();

			QString errMsg;
			const QStringList ret = cdata->getShortIds(q, &errMsg);
			if (!errMsg.isEmpty())
				se->throwError(QJSValue::TypeError, errMsg);
			return ret;
		}

		Q_INVOKABLE QVector<ConnectorRecord> getConnectorRecords(QJSValue query = QJSValue())
		{
			ConnectorData *cdata = nullptr;
			const QVariantMap q = initConnectorQuery(query, &cdata);
			if (!cdata)
				return QVector<ConnectorRecord>();

			QString errMsg;
			const QVector<ConnectorRecord> ret = cdata->records(q, &errMsg);
			if (!errMsg.isEmpty())
				se->throwError(QJSValue::TypeError, errMsg);
			return ret;
		}

		Q_INVOKABLE static QString currentPageName() { return DSE::tpCurrentPage; }

	Q_SIGNALS:
		void stateValueUpdate(const QByteArray &);
		void stateValueUpdateByName(const QByteArray &, const QByteArray &);
		void stateValueUpdateById(const QByteArray &, const QByteArray &);
		void stateCreate(const QByteArray &, const QByteArray &, const QByteArray &, const QByteArray &);
		void stateRemove(const QByteArray &);
		void choiceUpdate(const QByteArray &, const QStringList &);
		void connectorUpdateByLongId(const QByteArray &, uint8_t, bool = false);
		void connectorUpdate(const QByteArray &, uint8_t);

		void broadcastEvent(const QString &, const QVariantMap &);
		void connectorIdsChanged(const QByteArray &instanceName, const QByteArray &shortId);

	public Q_SLOTS:
		inline void stateUpdate(const QByteArray &value) { Q_EMIT stateValueUpdate(value); }
		inline void stateUpdate(const QByteArray &name, const QByteArray &value) { Q_EMIT stateValueUpdateByName(name, value); }
		inline void stateUpdateById(const QByteArray &id, const QByteArray &value) { Q_EMIT stateValueUpdateById(id, value); }
		inline void connectorUpdateShort(const QByteArray &id, uint8_t val) { Q_EMIT connectorUpdate(id, val); }

		void showNotification(const QByteArray &id, const QByteArray &title, const QByteArray &msg, QVariantList options = QVariantList(), QJSValue callback = QJSValue())
		{
			if (options.isEmpty())
				options << QVariantMap({{ QStringLiteral("id"), QStringLiteral("option") }, { QStringLiteral("title"), QStringLiteral(" ") } });
			if (callback.isCallable() || callback.isString())
				m_notificationCallbacks.insert(id, callback);
			emit tpNotification(id, title, msg, options);
		}

		void onNotificationClicked(const QString &notifyId, const QString &optionId)
		{
			QJSValue cb = m_notificationCallbacks.value(notifyId.toUtf8());
			if (!cb.isCallable() && !cb.isString())
				return;

			QJSValue res;
			QJSValue thisObj;
			const QJSValueList args { optionId, notifyId };
			QJSManagedValue m;
			if (m.isArray()) {
				if (cb.property("length").toInt() > 1)
					thisObj = cb.property(1);
				m = QJSManagedValue(cb.property(0), se->engine());
			}
			else {
				m = QJSManagedValue(cb, se->engine());
			}
			if (m.isFunction()) {
				if (!thisObj.isUndefined() && !thisObj.isNull())
					res = m.callWithInstance(thisObj, args);
				else
					res = m.call(args);
			}
			else if (m.isObject())
				res = m.callAsConstructor(args);
			else if (m.isString())
				res = se->engine()->evaluate(m.toString());
			else
				return;

			se->checkErrors();
		}

	private:
		Q_SIGNAL void tpNotification(const QByteArray &, const QByteArray &, const QByteArray &, const QVariantList & = QVariantList());

		ConnectorData *connectorData()
		{
			if (connData)
				return connData;
			if (se->isSharedInstance())
				return ConnectorData::instance();
			connData = new ConnectorData(se->currentInstanceName() /*, this*/);
			return connData;
		}

		QVariantMap initConnectorQuery(QJSValue query, ConnectorData **cdata)
		{
			if (!query.isObject()) {
				if (!query.isUndefined() && !query.isNull()) {
					se->throwError(QJSValue::TypeError, tr("Parameter must be an object type"));
					return QVariantMap();
				}
				query = se->engine()->newObject();
			}

			*cdata = connectorData();
			return query.toVariant().value<QVariantMap>();
		}


};

#ifndef DOXYGEN
}  // ScriptLib
#endif
