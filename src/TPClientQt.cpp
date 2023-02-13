/*
TPClientQt - Touch Poral Plugin API network client for C++/Qt-based plugins.
Copyright Maxim Paperno; all rights reserved.

Dual licensed under the terms of either the GNU General Public License (GPL)
or the GNU Lesser General Public License (LGPL), as published by the Free Software
Foundation, either version 3 of the Licenses, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

Copies of the GNU GPL and LGPL are available at <http://www.gnu.org/licenses/>.

This project may also use 3rd-party Open Source software under the terms
of their respective licenses. The copyright notice above does not apply
to any 3rd-party components used within.
*/

#include <QElapsedTimer>
#include <QMetaEnum>
#include <QTcpSocket>
#include <QThread>
#include <QDebug>
#if TP_CLIENT_ENABLE_SEND_QUEUE
#include <QCoreApplication>
#include <QQueue>
#endif

#include "TPClientQt.h"

#ifdef QT_DEBUG
Q_LOGGING_CATEGORY(lcTPC, "TPClientQt", QtDebugMsg);
#else
Q_LOGGING_CATEGORY(lcTPC, "TPClientQt", QtWarningMsg);
#endif


struct TPClientQt::Private
{
	Private(TPClientQt *q, const char *pluginId) :
	  q(q),
	  socket(new QTcpSocket(q)),
	  pluginId(pluginId)
	{
#if TP_CLIENT_ENABLE_SEND_QUEUE
		messageQ.reserve(100);
#endif
	}

	inline void onSockStateChanged(QAbstractSocket::SocketState s)
	{
		qCDebug(lcTPC) << "Socket state changed:" << s;
		switch (s) {
			case QAbstractSocket::ConnectedState:
				socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)) || !defined(Q_OS_WIN)
				// On POSIX this needs to be set after connection according to Qt5 docs.
				socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
#endif
				q->send({
					{"type", "pair"},
					{"id", pluginId.toUtf8().data()}
				});

				if (connTimeout > 0) {
					QThread *waitThread = QThread::create([=] { waitForPaired(); });
					QObject::connect(waitThread, &QThread::finished, q, [=]() { waitThread->wait(); waitThread->deleteLater(); }, Qt::QueuedConnection);
					waitThread->start();
				}
				break;

			case QAbstractSocket::UnconnectedState:
				if (tpInfo.paired) {
					tpInfo.paired = false;
					qCInfo(lcTPC) << "Closed Touch Portal Connection.";
				}
				break;

			default:
				break;
		}
	}

	inline void onSocketError(QAbstractSocket::SocketError e)
	{
		if (e == QAbstractSocket::TemporaryError || e == QAbstractSocket::UnknownSocketError)
			return;
		lastError = socket->errorString();
		Q_EMIT q->error(e);
		qCWarning(lcTPC) << "Permanent socket error:" << e << lastError;
	}

	void waitForPaired()
	{
		if (tpInfo.tpVersionCode)
			return;

		QElapsedTimer sw;
		sw.start();
		while (!tpInfo.tpVersionCode && socket->state() == QAbstractSocket::ConnectedState && !sw.hasExpired(connTimeout))
			QThread::yieldCurrentThread();

		if (!tpInfo.tpVersionCode) {
			qCCritical(lcTPC) << "Could not pair with Touch Portal! Disconnecting.";
			Q_EMIT q->error(QAbstractSocket::SocketTimeoutError);
			QMetaObject::invokeMethod(q, "disconnect", Qt::QueuedConnection);
		}
	}

	void onTpMessage(MessageType type, const QJsonObject &msg)
	{
		switch (type) {
			case MessageType::info: {
				tpInfo.status = msg.value(QLatin1String("status")).toString();
				tpInfo.paired = tpInfo.status.toLower() == "paired";
				tpInfo.sdkVersion = msg.value(QLatin1String("sdkVersion")).toInt(0);
				tpInfo.tpVersionCode = msg.value(QLatin1String("tpVersionCode")).toInt(0);
				tpInfo.pluginVersion = msg.value(QLatin1String("pluginVersion")).toInt(0);
				tpInfo.tpVersionString = msg.value(QLatin1String("tpVersionString")).toString("??");
				qCInfo(lcTPC).nospace().noquote()
				    << "Connection status '" << tpInfo.status << "' with Touch Portal v" << tpInfo.tpVersionString
				    << " (" << tpInfo.tpVersionCode << "; SDK v" << tpInfo.sdkVersion
				    << ") for Plugin " << pluginId << " v" << tpInfo.pluginVersion << " with TPClientQt v" << TP_CLIENT_VERSION_STR;

				if (Q_UNLIKELY(!tpInfo.paired)) {
					lastError = "Touch Portal responded with unknown 'status' of: " + tpInfo.status;
					qCCritical(lcTPC) << lastError;
					Q_EMIT q->error(QAbstractSocket::ConnectionRefusedError);
					q->disconnect();
					return;
				}

				const QJsonObject settings = arrayToObj(msg.value(QLatin1String("settings")));
				Q_EMIT q->connected(tpInfo, settings);
				Q_EMIT q->message(MessageType::info, msg);
				Q_EMIT q->message(MessageType::settings, settings);
				return;
			}

			case MessageType::settings:
				Q_EMIT q->message(MessageType::settings, arrayToObj(msg.value(QLatin1String("values"))));
				return;

			default:
				break;
		}
		Q_EMIT q->message(type, msg);
	}

	void write(const QByteArray &data) const
	{
		if (!socket || !socket->isWritable())
			return;
		const int len = data.length();
		qint64 bw = 0, sbw = 0;
		do {
			sbw = socket->write(data);
			bw += sbw;
		}
		while (bw != len && sbw > -1);
		if (sbw < 0) {
			qCCritical(lcTPC()) << "Socket write error: " << socket->errorString();
			q->disconnect();
			return;
		}
		socket->write("\n", 1);
	}

	QJsonObject arrayToObj(const QJsonValue &arry) const
	{
		QJsonObject ret;
		const QJsonArray a = arry.toArray();
		for (const QJsonValue &v : a) {
			if (v.isObject()) {
				const QJsonObject &vObj = v.toObject();
				QJsonObject::const_iterator next = vObj.begin(), last = vObj.end();
				for (; next != last; ++next)
					ret.insert(next.key(), next.value());
			}
		}
		return ret;
	}

	TPClientQt * const q;
	QTcpSocket * const socket;
	QString lastError;
	QString pluginId;
	QString tpHost = QStringLiteral("127.0.0.1");
	uint16_t tpPort = 12136;
	int connTimeout = 10000;  // ms
	TPClientQt::TPInfo tpInfo;
#if TP_CLIENT_ENABLE_SEND_QUEUE
	std::atomic_bool enableSendQueue = false;
	std::atomic_bool inQueue = false;
	QQueue<QByteArray> messageQ;
#endif

	friend class TPClientQt;
};

#define d_const  const_cast<const Private *>(d)

// ---------------------------------------
// TPClientQt
// ---------------------------------------

TPClientQt::TPClientQt(const char *pluginId, QObject *parent) :
  QObject(parent),
  d(new Private(this, pluginId))
{
	qRegisterMetaType<TPClientQt::MessageType>();
	qRegisterMetaType<TPClientQt::TPInfo>();
	qRegisterMetaType<QAbstractSocket::SocketState>();
	qRegisterMetaType<QAbstractSocket::SocketError>();

	QObject::connect(d->socket, &QTcpSocket::readyRead, this, &TPClientQt::onReadyRead);
	QObject::connect(d->socket, &QTcpSocket::disconnected, this, &TPClientQt::disconnected);
	QObject::connect(d->socket, &QTcpSocket::stateChanged, this, [this](QAbstractSocket::SocketState s) { d->onSockStateChanged(s); });
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
	QObject::connect(d->socket, qOverload<QAbstractSocket::SocketError>(&QAbstractSocket::error), this, [this](QAbstractSocket::SocketError e) { d->onSocketError(e); });
#else
	QObject::connect(d->socket, &QAbstractSocket::errorOccurred, this, [this](QAbstractSocket::SocketError e) { d->onSocketError(e); });
#endif
}

TPClientQt::~TPClientQt()
{
	disconnect();
	delete d;
}

bool TPClientQt::isConnected() const { return d_const->socket->state() == QAbstractSocket::ConnectedState && d_const->tpInfo.paired; }
QAbstractSocket::SocketState TPClientQt::socketState() const { return d_const->socket->state(); }
QAbstractSocket::SocketError TPClientQt::socketError() const { return d_const->socket->error(); }
QString TPClientQt::errorString() const { return d_const->lastError; }

const TPClientQt::TPInfo &TPClientQt::tpInfo() const { return d_const->tpInfo; }

QString TPClientQt::pluginId() const { return d_const->pluginId; }
bool TPClientQt::setPluginId(const char *pluginId)
{
	if (!pluginId || !strlen(pluginId)) {
		qCCritical(lcTPC()) << "Plugin ID is required!";
		return false;
	}
	if (d_const->socket->state() != QAbstractSocket::UnconnectedState) {
		qCCritical(lcTPC()) << "Cannot change Plugin ID while connected.";
		return false;
	}
	d->pluginId = pluginId;
	return true;
}

QString TPClientQt::hostName() const { return d_const->tpHost; }
uint16_t TPClientQt::hostPort() const { return d_const->tpPort; }
void TPClientQt::setHostProperties(const QString &nameOrAddress, uint16_t port)
{
	if (!nameOrAddress.isEmpty())
		d->tpHost = nameOrAddress;
	if (port > 0)
		d->tpPort = port;
}

int TPClientQt::connectionTimeout() const { return d_const->connTimeout; }
void TPClientQt::setConnectionTimeout(int timeoutMs) { d->connTimeout = timeoutMs; }

#if TP_CLIENT_ENABLE_SEND_QUEUE
void TPClientQt::setSendQueueEnabled(bool enable) { d->enableSendQueue = enable; }
bool TPClientQt::sendQueueEnabled() const { return d->enableSendQueue; }
#endif

void TPClientQt::connect()
{
	if (d_const->socket->state() != QAbstractSocket::UnconnectedState) {
		qCWarning(lcTPC()) << "Cannot connect while socket already connected or pending operation.";
		return;
	}

	if (d->pluginId.isEmpty()) {
		d->lastError = "Plugin ID is required!";
		qCCritical(lcTPC()) << d->lastError;
		Q_EMIT error(QAbstractSocket::OperationError);
		return;
	}

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0)) && defined(Q_OS_WIN)
	// According to Qt5 docs, on Windows this needs to be set before connection.
	d->socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
#endif

	d->tpInfo = TPInfo();
	d->socket->connectToHost(d->tpHost, d->tpPort);
}

void TPClientQt::disconnect() const
{
	d_const->socket->flush();
	d_const->socket->disconnectFromHost();
}

void TPClientQt::write(const QByteArray &data) const
{
#if TP_CLIENT_ENABLE_SEND_QUEUE
	if (d_const->enableSendQueue) {
		static qint64 lastSend = 0;

		d->messageQ.enqueue(data);
		if (d->inQueue)
			return;

		d->inQueue = true;
		QElapsedTimer tim;
		tim.start();
		qint64 nextEl = qMax(0LL, lastSend - tim.msecsSinceReference());
		while (!d->messageQ.isEmpty()) {
			//qDebug() << "Queue length" << d_const->messageQ.size() << "next:" << nextEl << "now:" << tim.elapsed() << tim.nsecsElapsed();
			while(tim.elapsed() < nextEl)
				QCoreApplication::processEvents(QEventLoop::AllEvents);
			d->write(d->messageQ.dequeue());
			++nextEl;
		}
		lastSend = tim.msecsSinceReference() + 1;
		d->inQueue = false;
		//qDebug() << "Queue length" << d_const->messageQ.size() << d_const->sendTimer->interval();
		return;
	}
#endif
	d_const->write(data);
}

// private

void TPClientQt::onReadyRead()
{
	QJsonParseError jpe;
	while (d->socket->canReadLine()) {
		const QByteArray &bytes = d->socket->readLine();
		if (bytes.isEmpty())
			continue;;
		const QJsonDocument &js = QJsonDocument::fromJson(bytes, &jpe);
		if (!js.isObject()) {
			if (jpe.error == QJsonParseError::NoError)
				qCWarning(lcTPC) << "Got empty or invalid JSON data, with no parsing error.";
			else
				qCWarning(lcTPC) << "Got invalid JSON data:" << jpe.errorString() << "; @" << jpe.offset;
			qCDebug(lcTPC) << bytes;
			continue;
		}
		const QJsonObject &msg = js.object();
		//	qCDebug(lcTPC) << msg;
		const QJsonValue &jMsgType = msg.value(QLatin1String("type"));
		if (!jMsgType.isString()) {
			qCWarning(lcTPC) << "TP message data missing the 'type' property.";
			qCDebug(lcTPC) << msg;
			continue;
		}

		bool ok;
		MessageType iMsgType = (MessageType)QMetaEnum::fromType<TPClientQt::MessageType>().keyToValue(qPrintable(jMsgType.toString()), &ok);
		if (!ok) {
			iMsgType = MessageType::Unknown;
			qCWarning(lcTPC) << "Unknown TP message 'type' property:" << jMsgType.toString();
		}
		d->onTpMessage(iMsgType, msg);
	}
}

#include "moc_TPClientQt.cpp"
