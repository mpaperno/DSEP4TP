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

#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaEnum>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#include "common.h"
#include "DSE.h"

#define CONNECTOR_DATA_PRIMARY_DB_CONN_NAME   QStringLiteral("Shared")

#ifdef DOXYGEN
#define QByteArray String
#define QJsonObject Object
#endif

/*!
\class ConnectorRecord
\ingroup TPAPI
The ConnectorRecord object holds information about a Touch Portal Connector (Slider) instance. These instances are
reported by Touch Portal in the plugin API `shortConnectorIdNotification` message. These messages are parsed by the
plugin and stored for future reference as objects in a database. The `ConnectorRecord` represents such a stored object.

Instances of `ConnectorRecrord` are obtained by calls to `TP` class functions like `TP.getConnectorRecords()`.
Objects of this type cannot be created directly.

All properties are read-only.

\sa ConnectorData, [Touch Portal Connectors API](https://www.touch-portal.com/api/index.php?section=connectors) for reference
\since v1.1.0.
*/
class ConnectorRecord
{
		Q_GADGET
		//! The "short connector ID" assigned by Touch Portal to this partulcar instance of a slider.
		//! This is the ID to be used for sending connectorUpdate() messages back to Touch Portal.
		Q_PROPERTY(QString shortId MEMBER shortId CONSTANT)
		//! The full connectorId which Touch Portal uses to identify this type of connector action. This is the ID specified in the `entry.tp` plugin configuration JSON. \n
		//! By convention this plugin uses IDs with "parts" separated by periods. For example `us.paperno.max.tpp.dse.conn.script.eval` \n
		//! The IDs always start with the full plugin ID, followed by potentially other designators, and always ending with the actual "thing" name,
		//! in this case the name of an action a connector should perform. The last part of an ID should always be unique.
		Q_PROPERTY(QString connectorId MEMBER connectorId CONSTANT)
		//! The type of connector action. May be either of:
		//! - Enumeration type, one of: "Eval", "Load", "Import", "Update", "OneTime"
		//! - Another string with an arbitrary action name, which is taken from the last part (when split on periods `.`) of the @ref connectorId property (see below).
		Q_PROPERTY(QString actionType MEMBER actionType CONSTANT)
		//! The State Name of the connector. This may be "ANONYMOUS" for "OneTime" @ref actionType connectors, or an empty string for non-enumerated `actionType` types.
		Q_PROPERTY(QString instanceName MEMBER instanceName CONSTANT)
		//! The type of scripting action. Enumeration type, one of: `DSE.ExpressionInput`, `DSE.ScriptInput`, `DSE.ModuleInput`, `DSE.UnknownInputType` \n
		//! When used for search, an equivalent string may also be used, eg. "Expression", "Script", "Module", or "Unknown". Wildcards are not allowed.
		Q_PROPERTY(DSE::ScriptInputType inputType MEMBER inputType CONSTANT)
		//! The full expression string of the connector. This is exactly as entered in the slider setup, no variables or code is evaluated.
		Q_PROPERTY(QString expression MEMBER expression CONSTANT)
		//! The script/module file as specified in the connector, if any. Only Script and Module connector types will have a file.
		Q_PROPERTY(QString file MEMBER file CONSTANT)
		//! The module import alias as specified in the connector. Only Module type connects will have an alias.
		Q_PROPERTY(QString alias MEMBER alias CONSTANT)
		//! This is the Engine Instance type specified in the connector. One of: `DSE.SharedInstance` or `DSE.PrivateInstance` \n
		//! When used for search, an equivalent string may also be used: "Shared" or "Private". Wildcards are not allowed.
		Q_PROPERTY(DSE::EngineInstanceType instanceType MEMBER instanceType CONSTANT)
		/*! This property holds an object with any other Connector data members which were found but do not fit into any of the other data member property types.
			__Note:__ This is for advanced use with custom connector definitions in a custom entry.tp file. By convention this plugin uses action/connector data IDs with
			"parts" separated by periods. For example `us.paperno.max.tpp.dse.act.script.eval.expr` \n
			The property is __returned__ as an `Object` type, possibly empty, with the last portion of the data ID (after the last period (`.`) separator, "expr" in the example above)
			as key(s) with the corresponding value(s) from the user's entry. The value is always a String type. \n
			The property is __searched__ (and stored) as compact serialized JSON text with both keys and values quoted. Eg. `{"rangeMin":"100","rangeMax":"1000"}` \n
			The quotes and colons can be effectively used in search terms to limit the scope of the search. For example ```{ otherData: '*"range*":"100?"*' }``` \n
			Note the wildcards outside of the quoted object property and value -- this is to ensure other properties properties in the JSON string are ignored. \n
			This property can be specified multiple times in a search pattern when used with search array notation. For example
			```js
			TP.getConnectorRecords([ { otherData: '*"rangeMin":"1000"*' }, { otherData: '*"rangeMax":"100"*' } ]);
			```
		*/
		Q_PROPERTY(QJsonObject otherData MEMBER otherData CONSTANT)
		//! A timestamp of when the notifaction about this connector was received from Touch Portal. The format is the common "milliseconds since epoch".
		//! The latest notification will typically mean that slider actually exists in the user's page, whereas previous versions
		//! may or may not still exist. Touch Portal does not notify when a connector was removed.
		//! __Note:__ This property cannot currently be used in search queries.
		Q_PROPERTY(qint64 timestamp MEMBER timestamp CONSTANT)
		//! `true` if this record holds no value (eg. the result of a failed database lookup); `false` if this record is valid.
		//! __Note:__ This property cannot be used in search queries.
		Q_PROPERTY(bool isNull READ isNull CONSTANT)


	public:
		enum Columns {
			COL_ACTTYPE,
			COL_NAME,
			COL_EXPR,
			COL_FILE,
			COL_ALIAS,
			COL_CONNID,
			COL_SHORTID,
			COL_OTHER,
			COL_INPTYPE,
			COL_INSTYPE,
			COL_TS,
		};

		static const QStringList &columnNames() {
			static const QStringList n {
				"actionType",    // COL_ACTTYPE,
				"instanceName",  // COL_NAME,
				"expression",    // COL_EXPR,
				"file",          // COL_FILE,
				"alias",         // COL_ALIAS,
				"connectorId",   // COL_CONNID,
				"shortId",       // COL_SHORTID,
				"otherData",     // COL_OTHER,
				"inputType",     // COL_INPTYPE,
				"instanceType",  // COL_INSTYPE,
				"timestamp",     // COL_TS,
			};
			return n;
		}

		static const QStringList &textPropertyNames() {
			static const QStringList l(columnNames().sliced(COL_ACTTYPE, COL_OTHER + 1));
			return l;
		}
		static const QStringList &enumPropertyNames() {
			static const QStringList l(columnNames().sliced(COL_INPTYPE, COL_TS - COL_INPTYPE));
			return l;
		}
		static const QMap<QString, QMetaEnum> &enumProperties()
		{
			static const QMap<QString, QMetaEnum> map {
				{ columnNames().at(COL_INPTYPE), DSE::inputTypeMeta() },
				{ columnNames().at(COL_INSTYPE), DSE::instanceTypeMeta() }
			};
			return map;
		}

		ConnectorRecord() {}

		DSE::ScriptInputType inputType = DSE::ScriptInputType::UnknownInputType;
		DSE::EngineInstanceType instanceType = DSE::EngineInstanceType::UnknownInstanceType;
		qint64 timestamp;
		QByteArray actionType;
		QByteArray instanceName;
		QByteArray connectorId;
		QByteArray shortId;
		QByteArray expression;
		QByteArray file;
		QByteArray alias;
		QJsonObject otherData;

		QString inputTypeStr() const { return DSE::inputTypeMeta().key((int)inputType); }
		QString instanceTypeStr() const { return DSE::instanceTypeMeta().key((int)instanceType); }
		bool isNull() const { return !timestamp; }

		explicit ConnectorRecord(QSqlQuery *qry)
		{
			instanceName  = qry->value(COL_NAME).toByteArray();
			actionType    = qry->value(COL_ACTTYPE).toByteArray();
			expression    = qry->value(COL_EXPR).toByteArray();
			file          = qry->value(COL_FILE).toByteArray();
			alias         = qry->value(COL_ALIAS).toByteArray();
			connectorId   = qry->value(COL_CONNID).toByteArray();
			shortId       = qry->value(COL_SHORTID).toByteArray();
			timestamp     = qry->value(COL_TS).toLongLong();
			inputType    = DSE::ScriptInputType(qry->value(COL_INPTYPE).toUInt());
			instanceType = DSE::EngineInstanceType(qry->value(COL_INSTYPE).toUInt());
			otherData     = QJsonDocument::fromJson(qry->value(COL_OTHER).toByteArray()).object();
		}

		void bindAll(QSqlQuery *qry) const
		{
			qry->bindValue(COL_NAME,  qPrintable(instanceName));
			qry->bindValue(COL_ACTTYPE,  qPrintable(actionType));
			qry->bindValue(COL_EXPR,  qPrintable(expression));
			qry->bindValue(COL_FILE, qPrintable(file));
			qry->bindValue(COL_ALIAS, qPrintable(alias));
			qry->bindValue(COL_CONNID, qPrintable(connectorId));
			qry->bindValue(COL_SHORTID, qPrintable(shortId));
			qry->bindValue(COL_INPTYPE,  uint(inputType));
			qry->bindValue(COL_INSTYPE, uint(instanceType));
			qry->bindValue(COL_OTHER, QJsonDocument(otherData).toJson(QJsonDocument::Compact));
			qry->bindValue(COL_TS, QDateTime::currentMSecsSinceEpoch());
		}
};


// ---------------------------------
// ConnectorData
// ---------------------------------

class ConnectorData : public QObject
{
		Q_OBJECT
	public:
		explicit ConnectorData(const QString &connName, QObject *p = nullptr)
		  : QObject(p), m_primary(connName == CONNECTOR_DATA_PRIMARY_DB_CONN_NAME)
		{
			if (QSqlDatabase::contains(connName)) {
				m_db = QSqlDatabase::database(connName, true);
				qCDebug(lcPlugin) << "Existing DB" << m_db.connectionName() << m_db.databaseName();
			}
			else
				setupDatabase(connName);
		}

		~ConnectorData()
		{
			const QString cname(m_db.connectionName());
			m_db.close();
			m_db = QSqlDatabase();
			QSqlDatabase::removeDatabase(cname);
		}

		static ConnectorData *instance() {
			static ConnectorData cd(CONNECTOR_DATA_PRIMARY_DB_CONN_NAME);
			return &cd;
		}

		void insert(const ConnectorRecord &cr)
		{
			if (!m_db.isOpen())
				return;

			const auto &cnames = ConnectorRecord::columnNames();
			auto placeholders = QStringLiteral("?,").repeated(ConnectorRecord::columnNames().size());
			placeholders.chop(1);

			QSqlQuery qry(m_db);
			qry.prepare(
			  QStringLiteral("REPLACE INTO ConnectorData (%1) VALUES (%2)").arg(cnames.join(','), placeholders)
			);
			cr.bindAll(&qry);
			if (!qry.exec())
				qCCritical(lcPlugin) << "Failed to insert record into" << m_db.connectionName() << m_db.databaseName() << ':' << qry.lastError().text() << '\n' << qry.executedQuery() << '\n' << qry.boundValues();
			else
				Q_EMIT connectorsUpdated(cr.instanceName, cr.shortId);
		}

		QStringList getShortIds(const QMultiMap<QString, QVariant> &query, QString *error = nullptr)
		{
			if (!m_db.isOpen())
				return QStringList();

			const QString sql = buildQuery(query, QStringLiteral("shortId"));

			QSqlQuery qry(sql, m_db);
			qry.setForwardOnly(true);
			if (!qry.exec())  {
				const QString err = tr("SQL query failed with error: %1").arg(qry.lastError().text());
				if (error)
					*error = err;
				else
					qCWarning(lcPlugin) << err;
				return QStringList();
			}
			QStringList ret;
			while (qry.next())
				ret << qry.value(0).toString();
			//qCDebug(lcPlugin) << ret;
			return ret;
		}

		ConnectorRecord getByShortId(const QByteArray &shortId, QString *error = nullptr)
		{
			if (!m_db.isOpen())
				return ConnectorRecord();

			const QString sql = QStringLiteral("SELECT %1 FROM ConnectorData WHERE shortId LIKE '%2' ORDER BY timestamp DESC LIMIT 1").arg(ConnectorRecord::columnNames().join(','), shortId);
			QSqlQuery qry(sql, m_db);
			qry.setForwardOnly(true);
			if (qry.exec())
				return qry.next() ? ConnectorRecord(&qry) : ConnectorRecord();

			const QString err = tr("SQL query failed with error: %1").arg(qry.lastError().text());
			if (error)
				*error = err;
			else
				qCWarning(lcPlugin) << err;
			return ConnectorRecord();
		}

		QVector<ConnectorRecord> records(const QMultiMap<QString, QVariant> &query, QString *error = nullptr)
		{
			if (!m_db.isOpen())
				return QVector<ConnectorRecord>();

			const QString sql = buildQuery(query, ConnectorRecord::columnNames().join(','));

			QSqlQuery qry(sql, m_db);
			qry.setForwardOnly(true);
			if (!qry.exec())  {
				const QString err = tr("SQL query failed with error: %1").arg(qry.lastError().text());
				if (error)
					*error = err;
				else
					qCWarning(lcPlugin) << err;
				return QVector<ConnectorRecord>();
			}

			QVector<ConnectorRecord> ret;
			while (qry.next())
				ret << ConnectorRecord(&qry);
			return ret;
		}

	Q_SIGNALS:
		void connectorsUpdated(const QByteArray &instanceName, const QByteArray &shortId);

	private:

		QString buildQuery(const QMultiMap<QString, QVariant> &query, QStringView select) const
		{
			QStringList filter;

			auto addEnumFltr = [&](const QString &key, const QVariant &p, const QMetaEnum &meta) {
				if (p.canConvert<QString>()) {
					bool ok;
					const int e = meta.keyToValue(qPrintable(p.toString()), &ok);
					if (ok)
						filter << key + '=' + QString::number(e);
				}
			};
			auto addStringFltr = [&](const QString &key, const QVariant &p) {
				if (p.canConvert<QString>())
					filter << key + " GLOB '" + p.toString() + '\'';
			};
			for (auto const &[k, v] : query.asKeyValueRange()) {
				if (ConnectorRecord::enumPropertyNames().contains(k)) {
					if (v.canConvert<int>())
						filter << k + '=' + QString::number(v.toInt());
					else
						addEnumFltr(k, v, ConnectorRecord::enumProperties().value(k));
				}
				else {
					addStringFltr(k, v);
				}
			}

			const QString orderBy = query.value(QStringLiteral("orderBy"), QStringLiteral("timestamp DESC")).toString();

			QString sql = QStringLiteral("SELECT %1 FROM ConnectorData").arg(select);
			if (filter.size())
				sql += " WHERE " + filter.join(QLatin1String(" AND "));
			sql += " ORDER BY " + orderBy;

			//qCDebug(lcPlugin) << sql;
			return sql;
		}

		static std::atomic_bool &dbCreated()
		{
			static std::atomic_bool created = false;
			return created;
		}

		void setupDatabase(const QString &name)
		{
			m_db = QSqlDatabase::addDatabase("QSQLITE", name);
			m_db.setDatabaseName("file:connectors?mode=memory&cache=shared");
			QString connOpts("QSQLITE_ENABLE_SHARED_CACHE;QSQLITE_OPEN_URI");  // ;QSQLITE_ENABLE_REGEXP
			if (!m_primary)
				connOpts += ";QSQLITE_OPEN_READONLY";
			m_db.setConnectOptions(connOpts);
			if (!m_db.open()) {
				m_db.setConnectOptions();
				qCCritical(lcPlugin) << "Unable to establish database connection for" << name << "with error:" << m_db.lastError();
				return;
			}

			qCDebug(lcPlugin) << "Opened DB" << m_db.connectionName() << m_db.databaseName() << "Created?" << dbCreated();
			if (dbCreated())
				return;
			dbCreated() = true;

			m_db.transaction();
			QSqlQuery q(m_db);
			auto execQry = [&](const QString &s) -> bool {
				if (q.exec(s))
					return true;
				qCDebug(lcPlugin) << "Query error for" << m_db.connectionName() << m_db.databaseName() << m_db.connectionNames() << '\n' << q.lastError() << '\n' << q.executedQuery() << '\n' << s;
				return false;
			};

			const QString sql(QStringLiteral(
				"CREATE TABLE ConnectorData ("
			  "  actionType   varchar(25)  NOT NULL,"
				"  instanceName varchar(100) NOT NULL DEFAULT '',"
				"  expression   TEXT         NOT NULL DEFAULT '',"
				"  file         varchar(255) NOT NULL DEFAULT '',"
				"  alias        varchar(30)  NOT NULL DEFAULT '',"
				"  connectorId  varchar(200) NOT NULL DEFAULT '',"
				"  shortId      varchar(20)  NOT NULL UNIQUE,"
			  "  otherData    TEXT         NOT NULL DEFAULT '{}',"
			  "  inputType    INTEGER      NOT NULL DEFAULT 0,"
			  "  instanceType INTEGER      NOT NULL DEFAULT 0,"
				"  timestamp    INTEGER      NOT NULL,"
				"  PRIMARY KEY(inputType, instanceType, actionType, instanceName, expression, file, alias, otherData)"
				") WITHOUT ROWID;"
			));
			if (!execQry(sql)) {
				m_db.rollback();
				qCCritical(lcPlugin) << "Failed to created database definitions for" << m_db.connectionName() << m_db.databaseName();
				return;
			}
			qCDebug(lcPlugin) << "Created database definitions for" << m_db.connectionName() << m_db.databaseName();

			for (const auto &prop : ConnectorRecord::columnNames())
				execQry(QStringLiteral("CREATE INDEX IDX_%1 ON ConnectorData (%1);").arg(prop));

			m_db.commit();
		}

		QSqlDatabase m_db;
		bool m_primary = false;
};

Q_DECLARE_METATYPE(ConnectorRecord)
