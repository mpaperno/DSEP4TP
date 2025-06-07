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

#include "ConnectorData.h"
#include "DSE.h"
#include "DynamicScript.h"
#include "Plugin.h"
#include "ScriptEngine.h"
#include "event_utils.h"

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
			setObjectName(QStringLiteral("TPAPI"));
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
			connect(this, &TPAPI::tpStateCreate, plugin, &Plugin::tpStateCreate, ctype);
			connect(this, &TPAPI::stateRemove, plugin, &Plugin::tpStateRemove, ctype);
			connect(this, &TPAPI::choiceUpdate, plugin, &Plugin::tpChoiceUpdateStrList, ctype);
			connect(this, &TPAPI::choiceUpdateInstance, plugin, &Plugin::tpChoiceUpdateInstanceStrList, ctype);
			connect(this, &TPAPI::connectorUpdateByLongId, plugin, &Plugin::tpConnectorUpdate, ctype);
			connect(this, &TPAPI::connectorUpdate, plugin, &Plugin::tpConnectorUpdateShort, ctype);
			connect(this, &TPAPI::settingUpdate, plugin, &Plugin::tpSettingUpdate, ctype);
			connect(this, &TPAPI::tpNotification, plugin, &Plugin::tpNotification, ctype);
			connect(this, &TPAPI::triggerEvent, plugin, &Plugin::tpTriggerEvent, ctype);
			connect(this, &TPAPI::triggerGenericScriptEvent, plugin, &Plugin::triggerGenericScriptEvent, ctype);
		}

		void connectInstance(const DynamicScript *ds)
		{
			// Global from script engine which needs name lookup because the state name is not fully qualified.
			connect(this, &TPAPI::stateValueUpdate, ds, &DynamicScript::stateUpdate, Qt::UniqueConnection);
		}

		void disconnectInstance(const DynamicScript *ds)
		{
			disconnect(this, &TPAPI::stateValueUpdate, ds, &DynamicScript::stateUpdate);
		}

		void connectSlots(const Plugin *plugin, Qt::ConnectionType ctype = Qt::AutoConnection)
		{
			// Connect to notifications about TP events so they can be re-broadcast to scripts in this instance.
			connect(plugin, &Plugin::tpNotificationClicked, this, &TPAPI::onNotificationClicked, ctype);
			connect(plugin, &Plugin::tpBroadcast, this, &TPAPI::broadcastEvent, ctype);
			connect(plugin, &Plugin::tpMessageEvent, this, &TPAPI::messageEvent, ctype);
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
			const QMultiMap<QString, QVariant> q = initConnectorQuery(query, &cdata);
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
			const QMultiMap<QString, QVariant> q = initConnectorQuery(query, &cdata);
			if (!cdata)
				return QVector<ConnectorRecord>();

			QString errMsg;
			const QVector<ConnectorRecord> ret = cdata->records(q, &errMsg);
			if (!errMsg.isEmpty())
				se->throwError(QJSValue::TypeError, errMsg);
			return ret;
		}

		Q_INVOKABLE static QString currentPageName() { return DSE::tpCurrentPage; }

		NAMED_EVENT_HANDLER();
		// legacy `on*()` event connection methods.
		ON_NAMED_EVENT(messageEvent);
		ON_NAMED_EVENT(broadcastEvent);
		ON_NAMED_EVENT(connectorIdsChanged);

	Q_SIGNALS:
		// Invokable by scripts
		void stateRemove(const QByteArray &);
		void choiceUpdate(const QByteArray &, const QStringList &);
		void choiceUpdateInstance(const QByteArray &, const QByteArray &, const QStringList &);
		void connectorUpdateByLongId(const QByteArray &, uint8_t, bool = false);
		void connectorUpdate(const QByteArray &, uint8_t);
		void settingUpdate(const QByteArray &name, const QByteArray &value);
		void triggerEvent(const QByteArray &, const QJsonObject & = QJsonObject()) const;

		// Connectable by scripts
		void messageEvent(const QJsonObject &message);
		void broadcastEvent(const QString &, const QVariantMap &);
		void connectorIdsChanged(const QByteArray &instanceName, const QByteArray &shortId);

	public Q_SLOTS:
		void stateCreate(const QByteArray &id, const QString &n, const QVariantMap &opts)
		{
			if (opts.isEmpty()) {
				stateCreate(id, n);
				return;
			}

			QString pg = opts.value("parentGroup", QString()).toString();
			if (pg.isEmpty()) {
				if (DynamicScript *ds = se->dseObject()->currentInstance())
					pg = QString::fromUtf8(ds->stateCategory());
			}

			stateCreate(id, pg, n,
				opts.value("defaultValue", QString()).toString(),
				opts.value("forceUpdate", false).toBool(),
				opts.value("delayMs", 0).toInt()
			);
		}

		void stateCreate(const QByteArray &id, const QString &n, const QString &d = QString(), int delayMs = 0)
		{
			QString pg;
			if (DynamicScript *ds = se->dseObject()->currentInstance())
				pg = QString::fromUtf8(ds->stateCategory());
			stateCreate(id, pg, n, d, false, delayMs);
		}

		void stateCreate(const QByteArray &id, const QString &p, const QString &n, const QString &d, int delayMs = 0) {
			stateCreate(id, p, n, d, false, delayMs);
		}

		void stateCreate(const QByteArray &id, const QString &p, const QString &n, const QString &d, bool force, int delayMs = 0)
		{
			Q_EMIT tpStateCreate(id, p.toUtf8(), n.toUtf8(), d.toUtf8(), force);
			if (delayMs > 0)
				QThread::msleep(delayMs);
		}

		inline void stateUpdate(const QByteArray &value) {
			if (DynamicScript *ds = se->dseObject()->currentInstance())
				ds->stateUpdate(value);
			else
				Q_EMIT stateValueUpdate(value);
		}
		inline void stateUpdate(const QByteArray &name, const QByteArray &value) { Q_EMIT stateValueUpdateByName(name, value); }
		inline void stateUpdateById(const QByteArray &id, const QByteArray &value) { Q_EMIT stateValueUpdateById(id, value); }
		inline void connectorUpdateShort(const QByteArray &id, uint8_t val) { Q_EMIT connectorUpdate(id, val); }

		void triggerGenericEvent(const QString &name, const QList<QJSPrimitiveValue> &values = QList<QJSPrimitiveValue>()) const
		{
			static const QString valueIdTemplate(QLatin1String("ScriptEvent.value.%1"));

			QJsonObject states;
			states[QLatin1String("ScriptEvent.name")] = name;

			for (int i=0; i < 10; ++i)
				states[valueIdTemplate.arg(i+1)] = values.value(i, QJSPrimitiveValue(QStringLiteral(""))).toString();

			// qCDebug(lcDse) << "Sending generic event trigger with" << states;
			Q_EMIT triggerGenericScriptEvent(states);
		}

		void triggerGenericEvent(
			const QString &name,   QJSPrimitiveValue v0,
			QJSPrimitiveValue v1 = QJSPrimitiveValue(QStringLiteral("")),
			QJSPrimitiveValue v2 = QJSPrimitiveValue(QStringLiteral("")),
			QJSPrimitiveValue v3 = QJSPrimitiveValue(QStringLiteral("")),
			QJSPrimitiveValue v4 = QJSPrimitiveValue(QStringLiteral("")),
			QJSPrimitiveValue v5 = QJSPrimitiveValue(QStringLiteral("")),
			QJSPrimitiveValue v6 = QJSPrimitiveValue(QStringLiteral("")),
			QJSPrimitiveValue v7 = QJSPrimitiveValue(QStringLiteral("")),
			QJSPrimitiveValue v8 = QJSPrimitiveValue(QStringLiteral("")),
			QJSPrimitiveValue v9 = QJSPrimitiveValue(QStringLiteral("")) ) const
		{
			triggerGenericEvent(name, {v0, v1, v2, v3, v4, v5, v6, v7, v8, v9});
		}

		void showNotification(const QByteArray &id, const QByteArray &title, const QByteArray &msg, QVariantList options = QVariantList(), QJSValue callback = QJSValue())
		{
			if (options.isEmpty())
				options << QVariantMap({{ QStringLiteral("id"), QStringLiteral("option") }, { QStringLiteral("title"), QStringLiteral(" ") } });
			if (callback.isCallable() || callback.isString())
				m_notificationCallbacks.insert(id, callback);
			Q_EMIT tpNotification(id, title, msg, options);
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
		Q_SIGNAL void stateValueUpdate(const QByteArray &);
		Q_SIGNAL void stateValueUpdateByName(const QByteArray &, const QByteArray &);
		Q_SIGNAL void stateValueUpdateById(const QByteArray &, const QByteArray &);
		Q_SIGNAL void tpNotification(const QByteArray &, const QByteArray &, const QByteArray &, const QVariantList & = QVariantList());
		Q_SIGNAL void tpStateCreate(const QByteArray &, const QByteArray &, const QByteArray &, const QByteArray &, bool force = false);
		Q_SIGNAL void triggerGenericScriptEvent(const QJsonObject &) const;

		ConnectorData *connectorData()
		{
			if (connData)
				return connData;
			if (se->isSharedInstance())
				return ConnectorData::instance();
			connData = new ConnectorData(se->currentInstanceName() /*, this*/);
			return connData;
		}

		QMultiMap<QString, QVariant> initConnectorQuery(QJSValue query, ConnectorData **cdata)
		{
			if (query.isArray()) {
				int len = query.property("length").toInt();
				QMultiMap<QString, QVariant> ret;
				for (int i=0; i < len; ++i) {
					const QJSValue &v = query.property(i);
					if (!v.isObject())
						continue;
					QJSValueIterator it(v);
					while (it.hasNext()) {
						it.next();
						ret.insert(it.name(), it.value().toVariant());
					}
				}
				*cdata = connectorData();
				return ret;
			}

			if (!query.isObject()) {
				if (!query.isUndefined() && !query.isNull()) {
					se->throwError(QJSValue::TypeError, tr("Parameter must be an object type"));
					return QMultiMap<QString, QVariant>();
				}
				query = se->engine()->newObject();
			}

			*cdata = connectorData();
			return QMultiMap<QString, QVariant>(query.toVariant().value<QVariantMap>());
		}

};

#ifndef DOXYGEN
}  // ScriptLib
#endif
