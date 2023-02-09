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
#include <QTimer>

#include "TPClientQt.h"
#include "JSError.h"

QT_BEGIN_NAMESPACE
class QJsonObject;
class QThread;
QT_END_NAMESPACE
class DynamicScript;
class ScriptEngine;

class Plugin : public QObject
{
		Q_OBJECT
	public:
		explicit Plugin(const QString &tpHost, uint16_t tpPort, QObject *parent = nullptr);
		~Plugin();

		static Plugin *instance;

	Q_SIGNALS:
		void tpConnect();
		void tpDisconnect();
		void tpStateUpdate(const QByteArray &, const QByteArray &) const;
		void tpStateCreate(const QByteArray &, const QByteArray &, const QByteArray &, const QByteArray &) const;
		void tpStateRemove(const QByteArray &) const;
		void tpChoiceUpdate(const QByteArray &, const QByteArrayList &) const;
		void tpChoiceUpdateStrList(const QByteArray &, const QStringList &) const;
		void tpChoiceUpdateInstance(const QByteArray &, const QByteArray &, const QByteArrayList &) const;
		void tpConnectorUpdate(const QByteArray &, quint8, bool) const;
		void tpConnectorUpdateShort(const QByteArray &, quint8) const;
		void tpNotification(const QByteArray &, const QByteArray &, const QByteArray &, const QVariantList &) const;
		void tpSettingUpdate(const QByteArray &, const QByteArray &);

		void tpNotificationClicked(const QString &, const QString &) const;
		void tpBroadcast(const QString &, const QVariantMap &) const;

		void loggerRotateLogs() const;

	public Q_SLOTS:
		void onStateUpdateByName(const QByteArray &name, const QByteArray &value) const;

	protected Q_SLOTS:
		void timerEvent(QTimerEvent *ev) override;

	private Q_SLOTS:
		void start();
		void exit();
		void quit();
		void initEngine();
		void savePluginSettings() const;
		void saveAllInstances() const;
		void loadAllInstances() const;
		void loadPluginSettings();
		void loadStartupSettings();
		bool saveScriptInstance(const QByteArray &name) const;
		DynamicScript *loadScriptInstance(const QByteArray &name) const;
		bool loadScriptSettings(DynamicScript *ds) const;
		void removeInstance(DynamicScript *ds, bool removeFromGlobal = true, bool removeUnusedEngine = true) const;
		void removeEngine(ScriptEngine *se, bool removeFromGlobal = true, bool removeScripts = true) const;
		void stopDeletionTimer(const QByteArray &name);
		void removeInstanceLater(DynamicScript *ds);
		void sendInstanceLists() const;
		void sendEngineLists() const;
		void updateInstanceChoices(int token, const QByteArray &instId = QByteArray()) const;
		void sendScriptState(DynamicScript *ds, const QByteArray &value = QByteArray()) const;
		void updateConnectors(const QMultiMap<QString, QVariant> &qry, int value, float rangeMin, float rangeMax) const;
		void updateActionRepeatProperties(int ms, int param) const;
		void raiseScriptError(const QByteArray &dsName, const QString &msg, const QString &type, const QString &stack = QString()) const;
		void clearScriptErrors();
		void onScriptError(const JSError &e) const;
		void onEngineError(const JSError &e) const;
		void onDsFinished();
		void onActionRepeatRateChanged(int ms) const;
		void onActionRepeatDelayChanged(int ms) const;
		void onTpConnected(const TPClientQt::TPInfo &info, const QJsonObject &settings);
		void onTpMessage(TPClientQt::MessageType type, const QJsonObject &msg);
		void dispatchAction(TPClientQt::MessageType type, const QJsonObject &msg);
		void scriptAction(TPClientQt::MessageType type, int act, const QMap<QString, QString> &dataMap, qint32 connectorValue = -1);
		void pluginAction(TPClientQt::MessageType type, int act, const QMap<QString, QString> &dataMap, qint32 connectorValue);
		void instanceControlAction(quint8 act, const QMap<QString, QString> &dataMap);
		void setActionRepeatRate(TPClientQt::MessageType type, quint8 act, const QMap<QString, QString> &dataMap, qint32 connectorValue) const;
		void handleSettings(const QJsonObject &settings) const;
		void parseConnectorNotification(const QJsonObject &msg) const;

	private:
		ScriptEngine *getOrCreateEngine(const QByteArray &name, bool failIfMissing = false) const;
		DynamicScript *getOrCreateInstance(const QByteArray &name, bool forUpdateAction = false, bool loadSettings = false) const;
		inline TPClientQt *tpClient() const { return client; }

		TPClientQt *client = nullptr;
		QThread *clientThread = nullptr;
		QTimer m_loadSettingsTmr;

		friend class DynamicScript;
};
