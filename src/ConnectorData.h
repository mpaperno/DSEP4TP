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
#include <QMetaEnum>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#include "common.h"
#include "DynamicScript.h"

#define CONNECTOR_DATA_PRIMARY_DB_CONN_NAME   QStringLiteral("Shared")

#ifdef DOXYGEN
#define QByteArray String
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

\sa ConnectorData, [Touch Portal Connectors API](https://www.touch-portal.com/api/index.php?section=connectors) for reference.
*/
class ConnectorRecord
{
		Q_GADGET
		//! The State Name of the connector. This may be "ANONYMOUS" for sliders using the "Anonymous (One-Time) Script" connector/action.
		Q_PROPERTY(QByteArray instanceName MEMBER instanceName CONSTANT)
		//! The type of connector action. Enumeration type, one of: "Eval", "Load", "Import", "Update", "OneTime"
		//! __Note:__ When using this field in search criteria, the search value must be exactly one of the choices listed above, including case. No wildcards are allowed.
		Q_PROPERTY(QByteArray actionType MEMBER actionType CONSTANT)
		//! The type of scripting action. Enumeration type, one of: "Expression", "Script", "Module", "Unknown" (for Update type actions)
		//! __Note:__ When using this field in search criteria, the search value must be exactly one of the choices listed above, including case. No wildcards are allowed.
		Q_PROPERTY(QByteArray inputType READ inputType CONSTANT)
		//! The full expression string of the connector. This is exactly as entered in the slider setup, no variables or code is evaluated.
		Q_PROPERTY(QByteArray expression MEMBER expression CONSTANT)
		//! The script/module file as specified in the connector, if any. Only Script and Module connector types will have a file.
		Q_PROPERTY(QByteArray file MEMBER file CONSTANT)
		//! The module import alias as specified in the connector. Only Module type connects will have an alias.
		Q_PROPERTY(QByteArray alias MEMBER alias CONSTANT)
		//! This is the Engine Instance type specified in the connector. One of: "Shared" or "Private"
		Q_PROPERTY(QByteArray instanceType READ instanceType CONSTANT)
		//! The Create State at Startup type specified in the connector. Enumeration type, one of: "NoDefault", "FixedValue", "CustomExpression", "MainExpression"
		//! __Note:__ When using this field in search criteria, the search value must be exactly one of the choices listed above, including case. No wildcards are allowed.
		Q_PROPERTY(QByteArray defaultType READ defaultType CONSTANT)
		//! The Default Value/Expression specified in the connector setup.
		Q_PROPERTY(QByteArray defaultValue MEMBER defaultValue CONSTANT)
		//! The full connectorId which Touch Portal uses to identify this type of connector action type.
		Q_PROPERTY(QByteArray connectorId MEMBER connectorId CONSTANT)
		//! The "short connector ID" assigned by Touch Portal to this partulcar instance of a slider.
		//! This is the ID to be used for sending connectorUpdate() messages back to Touch Portal.
		Q_PROPERTY(QByteArray shortId MEMBER shortId CONSTANT)
		//! A timestamp of when the notifaction about this connector was received from Touch Portal. The format is the common "milliseconds since epoch".
		//! The latest notification will typically mean that slider actually exists in the user's page, whereas previous versions
		//! may or may not still exist. Touch Portal does not notify when a connector was removed.
		//! __Note:__ This property cannot currently be used in search queries.
		Q_PROPERTY(qint64 timestamp MEMBER timestamp CONSTANT)
		//! `true` if this record holds no value (eg. the result of a failed database lookup); `false` if this record is valid.
		//! __Note:__ This property cannot be used in search queries.
		Q_PROPERTY(bool isNull READ isNull CONSTANT)

	public:
		ConnectorRecord() {}

		DynamicScript::InputType eInputType;
		DynamicScript::Scope eInstanceType;
		DynamicScript::DefaultType eDefaultType;
		qint64 timestamp;
		QByteArray actionType;
		QByteArray instanceName;
		QByteArray connectorId;
		QByteArray shortId;
		QByteArray expression;
		QByteArray file;
		QByteArray alias;
		QByteArray defaultValue;

		QByteArray inputType() const { return inputTypeMeta().key((int)eInputType); }
		QByteArray instanceType() const { return instanceTypeMeta().key((int)eInstanceType); }
		QByteArray defaultType() const { return defaultTypeMeta().key((int)eDefaultType); }
		bool isNull() const { return !timestamp; }

		static const QStringList &columnNames() {
			static const QStringList n {
				"instanceName",  // 0
				"actionType",    // 1
				"inputType",     // 2
				"expression",    // 3
				"file",          // 4
				"alias",         // 5
				"instanceType",  // 6
				"defaultType",   // 7
				"defaultValue",  // 8
				"connectorId",   // 9
				"shortId",       // 10
				"timestamp"      // 11
			};
			return n;
		}

		explicit ConnectorRecord(QSqlQuery *qry)
		{
			instanceName  = qry->value(0).toByteArray();
			actionType    = qry->value(1).toByteArray();
			eInputType    = DynamicScript::InputType(qry->value(2).toUInt());
			expression    = qry->value(3).toByteArray();
			file          = qry->value(4).toByteArray();
			alias         = qry->value(5).toByteArray();
			eInstanceType = DynamicScript::Scope(qry->value(6).toUInt());
			eDefaultType  = DynamicScript::DefaultType(qry->value(7).toUInt());
			defaultValue  = qry->value(8).toByteArray();
			connectorId   = qry->value(9).toByteArray();
			shortId       = qry->value(10).toByteArray();
			timestamp     = qry->value(11).toLongLong();
		}

		void bindAll(QSqlQuery *qry) const
		{
			qry->bindValue(0,  qPrintable(instanceName));
			qry->bindValue(1,  qPrintable(actionType));
			qry->bindValue(2,  uint(eInputType));
			qry->bindValue(3,  qPrintable(expression));
			qry->bindValue(4,  qPrintable(file));
			qry->bindValue(5,  qPrintable(alias));
			qry->bindValue(6,  uint(eInstanceType));
			qry->bindValue(7,  uint(eDefaultType));
			qry->bindValue(8,  qPrintable(defaultValue));
			qry->bindValue(9,  qPrintable(connectorId));
			qry->bindValue(10, qPrintable(shortId));
			qry->bindValue(11, QDateTime::currentMSecsSinceEpoch());
		}

		static const QMetaEnum inputTypeMeta() { static const QMetaEnum m = QMetaEnum::fromType<DynamicScript::InputType>(); return m; }
		static const QMetaEnum instanceTypeMeta() { static const QMetaEnum m = QMetaEnum::fromType<DynamicScript::Scope>(); return m; }
		static const QMetaEnum defaultTypeMeta() { static const QMetaEnum m = QMetaEnum::fromType<DynamicScript::DefaultType>(); return m; }
};

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

		QStringList getShortIds(const QVariantMap &query, QString *error = nullptr)
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

		QVector<ConnectorRecord> records(const QVariantMap &query, QString *error = nullptr)
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

		QString buildQuery(const QVariantMap &query, QStringView select) const
		{
			QStringList filter;

			auto addEnumFltr = [&](const QString &key, const QMetaEnum &meta) {
				const QVariant &p = query.value(key);
				if (p.canConvert<QString>()) {
					bool ok;
					const int e = meta.keyToValue(qPrintable(p.toString()), &ok);
					if (ok)
						filter << key + '=' + QString::number(e);
				}
			};
			addEnumFltr(QStringLiteral("inputType"), ConnectorRecord::inputTypeMeta());
			addEnumFltr(QStringLiteral("instanceType"), ConnectorRecord::instanceTypeMeta());
			addEnumFltr(QStringLiteral("defaultType"), ConnectorRecord::defaultTypeMeta());

			auto addStringFltr = [&](const QString &key) {
				const QVariant &p = query.value(key);
				if (p.canConvert<QString>())
					filter << key + " GLOB '" + p.toString() + '\'';
			};
			addStringFltr(QStringLiteral("instanceName"));
			addStringFltr(QStringLiteral("actionType"));
			addStringFltr(QStringLiteral("expression"));
			addStringFltr(QStringLiteral("file"));
			addStringFltr(QStringLiteral("alias"));
			addStringFltr(QStringLiteral("defaultValue"));
			addStringFltr(QStringLiteral("connectorId"));
			addStringFltr(QStringLiteral("shortId"));

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
				"  instanceName varchar(100) NOT NULL,"
				"  actionType   varchar(10)  NOT NULL,"
				"  inputType    INTEGER      NOT NULL DEFAULT 0,"
				"  expression   TEXT         NOT NULL DEFAULT '',"
				"  file         varchar(255) NOT NULL DEFAULT '',"
				"  alias        varchar(30)  NOT NULL DEFAULT '',"
				"  instanceType INTEGER      NOT NULL DEFAULT 0,"
				"  defaultType  INTEGER      NOT NULL DEFAULT 0,"
				"  defaultValue TEXT         NOT NULL DEFAULT '',"
				"  connectorId  varchar(200) NOT NULL DEFAULT '',"
				"  shortId      varchar(20)  NOT NULL UNIQUE,"
				"  timestamp    INTEGER      NOT NULL,"
				"  PRIMARY KEY(instanceName, actionType, inputType, expression, file, alias, instanceType, defaultType, defaultValue)"
				") WITHOUT ROWID;"
			));
			if (!execQry(sql)) {
				m_db.rollback();
				qCCritical(lcPlugin) << "Failed to created database definitions for" << m_db.connectionName() << m_db.databaseName();
				return;
			}
			qCDebug(lcPlugin) << "Created database definitions for" << m_db.connectionName() << m_db.databaseName();

			execQry(QStringLiteral("CREATE INDEX IDX_instanceName ON ConnectorData (instanceName);"));
			execQry(QStringLiteral("CREATE INDEX IDX_actionType   ON ConnectorData (actionType);"));
			execQry(QStringLiteral("CREATE INDEX IDX_inputType    ON ConnectorData (inputType);"));
			execQry(QStringLiteral("CREATE INDEX IDX_expression   ON ConnectorData (expression);"));
			execQry(QStringLiteral("CREATE INDEX IDX_file         ON ConnectorData (file);"));
			execQry(QStringLiteral("CREATE INDEX IDX_alias        ON ConnectorData (alias);"));
			execQry(QStringLiteral("CREATE INDEX IDX_instanceType ON ConnectorData (instanceType);"));
			execQry(QStringLiteral("CREATE INDEX IDX_defaultType  ON ConnectorData (defaultType);"));
			execQry(QStringLiteral("CREATE INDEX IDX_defaultValue ON ConnectorData (defaultValue);"));
			execQry(QStringLiteral("CREATE INDEX IDX_shortId      ON ConnectorData (shortId);"));
			execQry(QStringLiteral("CREATE INDEX IDX_connectorId  ON ConnectorData (connectorId);"));
			execQry(QStringLiteral("CREATE INDEX IDX_timestamp    ON ConnectorData (timestamp);"));
			m_db.commit();
		}

		QSqlDatabase m_db;
		bool m_primary = false;
};

Q_DECLARE_METATYPE(ConnectorRecord)
