/*
Dynamic Script Engine Plugin for Touch Portal
Copyright Maxim Paperno; all rights reserved.

Original version from the QtWebSockets project's  (https://github.com/qt/qtwebsockets)
QML module QQmlWebSocket, used under the GPL3 license.
Original copyright and license headers:
-----------------------
Copyright (C) 2016 Kurt Pattyn <pattyn.kurt@gmail.com>.
SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
-----------------------

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

#include <QCoreApplication>
#include <QMetaEnum>
#include <QMetaObject>
#include <QRegularExpression>

// #include "common.h"
#include "ScriptEngine.h"
#include "utils.h"
#include "WebSocket.h"
// #include "WSP.h"

namespace ScriptLib {

WebSocket::WebSocket(QObject *parent) :
	QObject(parent),
  m_webSocket(nullptr),
  m_status(ReadyState::CLOSED),
  m_request(),
  m_errorString()
{
	setObjectName("WebSocket");
	m_request.setOriginatingObject(this);
}

// used by WebSocketServer
WebSocket::WebSocket(QWebSocket *socket, QObject *parent) :
  WebSocket(parent)
{
	m_request = socket->request();
	setSocket(socket);
}

WebSocket::WebSocket(const QString &url, const QStringList &protocols, const QJSValue &options) :
  WebSocket()
{
	setUrl(url);
	setRequestedSubprotocols(protocols);
	if (options.isObject())
		setOptions(options);
}

WebSocket::~WebSocket()
{
	if (m_webSocket) {
		if (m_status != CLOSED)
			m_webSocket->abort();
		m_webSocket->disconnect();
		m_webSocket->deleteLater();
		// delete m_webSocket;
		m_webSocket = nullptr;
	}
	// qCDebug(lcPlugin) << this << "Destroyed";
}

// ---------------------------------------

void WebSocket::setUrl(const QString &url)
{
	QUrl u(url);
	// qCDebug(lcPlugin) << url << u.toString() << u.isRelative() << u.scheme();
	if (u.isRelative() || u.scheme().isEmpty())
		u = QUrl(QStringLiteral("ws://%1").arg(u.toString().remove(QRegularExpression("^/+"))));

	if (m_request.url() == u)
		return;

	bool wasOpen = m_status == OPEN && !!m_webSocket;
	if (wasOpen)
		m_webSocket->close();

	m_request.setUrl(u);
	Q_EMIT urlChanged();

	if (wasOpen)
		open();
}

void WebSocket::setRequestedSubprotocols(const QStringList &requestedSubprotocols)
{
	if (m_options.handshake.subprotocols() == requestedSubprotocols)
		return;

	m_options.handshake.setSubprotocols(requestedSubprotocols);
	Q_EMIT optionsChanged();
}



QJSValue WebSocket::headers() const
{
	QJSEngine *jse = qjsEngine(this);
	QJSValue ret = jse ? jse->newObject() : QJSValue();
	const auto req = m_status == OPEN ? m_webSocket->request() : m_request;
	const QList<QByteArray> headers = req.rawHeaderList();
	for (const QByteArray &hdr : headers)
		ret.setProperty(QString::fromUtf8(hdr), QString::fromUtf8(req.rawHeader(hdr)));
	return ret;
}

void WebSocket::setHeaders(const QJSValue &headers)
{
	if (!headers.isObject())
		return;

	QJSValueIterator it(headers);
	while (it.next())
		m_request.setRawHeader(it.name().toUtf8(), it.value().toString().toUtf8());
}

void WebSocket::clearHeaders()
{
	const QList<QByteArray> headers = m_request.rawHeaderList();
	if (!headers.size())
		return;
	for (const QByteArray &hdr : headers)
		m_request.setRawHeader(hdr, QByteArray());
	Q_EMIT headersChanged();
}

qint64 WebSocket::bytesToWrite() const
{
	return Q_LIKELY(m_webSocket) ? m_webSocket->bytesToWrite() : 0;
}

void WebSocket::setBinaryType(const QString &type)
{
	WebSocket::DeliveryType dt = type.startsWith(QLatin1String("frag")) ? MessageFrames : FullMessage;
	if (dt != m_options.deliveryType) {
		m_options.deliveryType = dt;
		connectDeliveryType();
		Q_EMIT binaryTypeChanged(binaryType());
	}
}

void WebSocket::setOptions(const QJSValue &options)
{
	if (!options.isObject()) {
		return;
	}
	QJSValue optVal;

	if ((optVal = options.property(QStringLiteral("protocolVersion"))).isNumber()) {
		int v = optVal.toInt();
		if (QMetaEnum::fromType<QWebSocketProtocol::Version>().valueToKey(v))
			m_options.protocolVersion = QWebSocketProtocol::Version(v);
	}

	if ((optVal = options.property(QStringLiteral("origin"))).isString())
		m_options.origin = optVal.toString();

	// can set sub-protocols from options
	if (!(optVal = options.property(QStringLiteral("protocols"))).isUndefined()) {
		if (optVal.isArray())
			setRequestedSubprotocols(Utils::jsArrayToStringList(optVal));
		else if (optVal.isString())
			setRequestedSubprotocols({optVal.toString()});
	}

	// can set binary type from options
	if ((optVal = options.property(QStringLiteral("binaryType"))).isString())
		setBinaryType(optVal.toString());

	if ((optVal = options.property(QStringLiteral("headers"))).isObject())
		setHeaders(optVal);

	// frame/message size limits
	if ((optVal = options.property(QStringLiteral("maxPayload"))).isNumber()) {
		m_options.maxInFrameSize = optVal.toInt();
		m_options.maxOutFrameSize = optVal.toInt();
		m_options.maxInMessageSize = optVal.toInt();
	}
	else {
		if ((optVal = options.property(QStringLiteral("maxIncomingFrameSize"))).isNumber())
			m_options.maxInFrameSize = optVal.toInt();
		if ((optVal = options.property(QStringLiteral("maxOutgoingFrameSize"))).isNumber())
			m_options.maxOutFrameSize = optVal.toInt();
		if ((optVal = options.property(QStringLiteral("maxIncomingMessageSize"))).isNumber())
			m_options.maxInMessageSize = optVal.toInt();
	}

	// timeouts
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
	if ((optVal = options.property(QStringLiteral("handshakeTimeout"))).isNumber())
		m_request.setTransferTimeout(optVal.toInt());
	else if ((optVal = options.property(QStringLiteral("timeout"))).isNumber())
		m_request.setTransferTimeout(optVal.toInt());
#endif

	// redirection - currently unsupported
	/*
	if ((optVal = options.property(QStringLiteral("maxRedirects"))).isNumber()) {
		m_request.setMaximumRedirectsAllowed(optVal.toInt());
	}
	if ((optVal = options.property(QStringLiteral("followRedirects"))).isBool() && !optVal.toBool()) {
		if (optVal.toBool())
			m_request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
		else
			m_request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
	}
	else if ((optVal = options.property(QStringLiteral("redirectPolicy"))).isString()) {
		const QString policy = optVal.toString().toLower();
		if (!policy.compare(QLatin1String("manual")) || !policy.compare(QLatin1String("disable")))
			m_request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
		else if (!policy.compare(QLatin1String("error")))
			m_request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::UserVerifiedRedirectPolicy);
		else if (!policy.compare(QLatin1String("no-less-safe")) || !policy.compare(QLatin1String("follow")))
			m_request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
		else if (!policy.compare(QLatin1String("same-origin")))
			m_request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::SameOriginRedirectPolicy);
	}
	*/

	// update socket options if we have a QWebSocket instance already.
	setSocketOptions();
}

void WebSocket::setListeners(const QJSValue &listeners)
{
	if (!listeners.isObject())
		return;
	for (const auto &ev : { "onbytesWritten", "onclose", "onclosed", "onerror", "onmessage", "onopen", "onopened", "onpong", "onreadyStateChanged" }) {
		if (const QJSValue optVal = listeners.property(ev); optVal.isCallable())
			setProperty(ev, optVal.toVariant(QJSValue::RetainJSObjects));
			// EventUtils::setScriptProperty(this, ev, optVal);
	}
}

// ---------------------------------------

void WebSocket::open()
{
	if (!m_request.url().isValid()) {
		setError(tr("Invalid URL."), QAbstractSocket::OperationError);
		return;
	}

	if (m_status != ReadyState::CLOSED) {
		setError(tr("Already open."), QAbstractSocket::OperationError);
		return;
	}

	if (!m_webSocket)
		setSocket();
	m_webSocket->open(m_request, m_options.handshake);
}

void WebSocket::close(qint16 closeCode, const QString &reason)
{
	if (m_status != ReadyState::OPEN && m_status != ReadyState::CONNECTING) {
		setError(tr("Socket already closed or closing."), QAbstractSocket::OperationError);
		return;
	}

	if (Q_LIKELY(m_webSocket)) {
		if (m_status == ReadyState::CONNECTING)
			m_webSocket->abort();
		else if (closeCode >= QWebSocketProtocol::CloseCode::CloseCodeNormal)
			m_webSocket->close(QWebSocketProtocol::CloseCode(closeCode), reason);
		else
			m_webSocket->close();
	}
}

void WebSocket::abort() const
{
	if (Q_LIKELY(m_webSocket))
		m_webSocket->abort();
}

void WebSocket::ping(const QByteArray &payload) const
{
	if (Q_LIKELY(m_webSocket))
		m_webSocket->ping(payload);
}

qint64 WebSocket::send(const QJSValue &message, bool isBinary)
{
	if (!isBinary && !message.isObject())
		return sendText(message.toString());

	if (QJSEngine *jse = qjsEngine(this)) {
		const QByteArray ba = jse->fromScriptValue<QByteArray>(message);
		if (!ba.isNull())
			return sendBinary(ba);
	}
	else if (!message.isUndefined() && !message.isNull()) {
		const QVariant v = message.toVariant();
		if (v.canConvert<QByteArray>())
			return sendBinary(v.toByteArray());
	}

	setError(tr("Cannot send message data, unknown data type."), QAbstractSocket::OperationError);
	return -1;
}

qint64 WebSocket::sendText(const QString &message)
{
	if (m_status != OPEN) {
		setError(tr("Messages can only be sent when the socket is open."), QAbstractSocket::OperationError);
		return -1;
	}
	return m_webSocket->sendTextMessage(message);
}

qint64 WebSocket::sendBinary(const QByteArray &message)
{
	if (m_status != OPEN) {
		setError(tr("Messages can only be sent when the socket is open."), QAbstractSocket::OperationError);
		return -1;
	}
	return m_webSocket->sendBinaryMessage(message);
}


// ---------------------------------------

void WebSocket::onError(QAbstractSocket::SocketError err)
{
	if (Q_LIKELY(m_webSocket))
		setError(m_webSocket->errorString(), qint16(err));
}

void WebSocket::onStateChanged(QAbstractSocket::SocketState state)
{
	switch (state)
	{
		case QAbstractSocket::ConnectingState:
		case QAbstractSocket::BoundState:
		case QAbstractSocket::HostLookupState:
			setStatus(ReadyState::CONNECTING);
			break;

		case QAbstractSocket::ConnectedState:
		case QAbstractSocket::ListeningState:
			setStatus(ReadyState::OPEN);
			break;

		case QAbstractSocket::ClosingState:
			setStatus(ReadyState::CLOSING);
			break;

		case QAbstractSocket::UnconnectedState:
		default:
			setStatus(ReadyState::CLOSED);
			break;
	}
}

void WebSocket::onTextMessage(const QString &msg)
{
	// if (m_deliveryType == FullMessage)
	Q_EMIT message({
		 { "data",     msg },
		 { "isBinary", false },
		 { "isLast",   true },
	});
}

void WebSocket::onBinaryMessage(const QByteArray &msg)
{
	// if (m_deliveryType == FullMessage)
	Q_EMIT message({
		 { "data",     msg },
		 { "isBinary", true },
		 { "isLast",   true },
	});
}

void WebSocket::onTextFrame(const QString &frame, bool isLast)
{
	// if (m_deliveryType == MessageFrames)
	Q_EMIT message({
		 { "data",     frame },
		 { "isBinary", false },
		 { "isLast",   isLast },
	});
}

void WebSocket::onBinaryFrame(const QByteArray &frame, bool isLast)
{
	// if (m_deliveryType == MessageFrames)
	Q_EMIT message({
		 { "data",     frame },
		 { "isBinary", true },
		 { "isLast",   isLast },
	});
}

// ---------------------------------------

void WebSocket::setSocket(QWebSocket *socket)
{
	if (m_webSocket) {
		m_webSocket->abort();
		m_webSocket->disconnect();
		m_webSocket->deleteLater();
	}

	if (socket) {
		m_webSocket = socket;
		// explicit ownership
		m_webSocket->setParent(nullptr);
		m_request = m_webSocket->request();
		setRequestedSubprotocols(socket->handshakeOptions().subprotocols());
		m_options.readBufferSize = socket->readBufferSize();
		Q_EMIT headersChanged();
	}
	else {
		m_webSocket = new QWebSocket(m_options.origin, m_options.protocolVersion, nullptr);
	}
	setSocketOptions();

	connect(m_webSocket, &QWebSocket::stateChanged, this, &WebSocket::onStateChanged);
	connect(m_webSocket, &QWebSocket::bytesWritten, this, &WebSocket::bytesWritten);
	connect(m_webSocket, &QWebSocket::pong, this, &WebSocket::pong);
#if (QT_VERSION < QT_VERSION_CHECK(6, 5, 0))
	connect(m_webSocket, qOverload<QAbstractSocket::SocketError>(&QWebSocket::error), this, &WebSocket::onError);
#else
	connect(m_webSocket, &QWebSocket::errorOccurred, this, &WebSocket::onError);
#endif

	connectDeliveryType();
	if (socket) {
		onStateChanged(m_webSocket->state());
		onError(m_webSocket->error());
	}
}

void WebSocket::setSocketOptions() const
{
	if (!m_webSocket)
		return;

	if (m_options.readBufferSize)
		m_webSocket->setReadBufferSize(m_options.readBufferSize);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
		if (m_options.maxInFrameSize)
			m_webSocket->setMaxAllowedIncomingFrameSize(m_options.maxInFrameSize);
		if (m_options.maxInMessageSize)
			m_webSocket->setMaxAllowedIncomingMessageSize(m_options.maxInMessageSize);
		if (m_options.maxOutFrameSize)
			m_webSocket->setOutgoingFrameSize(m_options.maxOutFrameSize);
#endif

}

void WebSocket::connectDeliveryType()
{
	if (!m_webSocket)
		return;
	if (m_options.deliveryType == MessageFrames) {
		connect(m_webSocket, &QWebSocket::textFrameReceived, this, &WebSocket::onTextFrame, Qt::UniqueConnection);
		connect(m_webSocket, &QWebSocket::binaryFrameReceived, this, &WebSocket::onBinaryFrame, Qt::UniqueConnection);
		disconnect(m_webSocket, &QWebSocket::textMessageReceived, this, &WebSocket::onTextMessage);
		disconnect(m_webSocket, &QWebSocket::binaryMessageReceived, this, &WebSocket::onBinaryMessage);
	}
	else {
		connect(m_webSocket, &QWebSocket::textMessageReceived, this, &WebSocket::onTextMessage, Qt::UniqueConnection);
		connect(m_webSocket, &QWebSocket::binaryMessageReceived, this, &WebSocket::onBinaryMessage, Qt::UniqueConnection);
		disconnect(m_webSocket, &QWebSocket::textFrameReceived, this, &WebSocket::onTextFrame);
		disconnect(m_webSocket, &QWebSocket::binaryFrameReceived, this, &WebSocket::onBinaryFrame);
	}
}

void WebSocket::setStatus(WebSocket::ReadyState status)
{
	if (m_status == status)
		return;

	m_status = status;
	Q_EMIT readyStateChanged(status);

	setError();  // clears error state

	if (status == OPEN) {
		m_request = m_webSocket->request();
		Q_EMIT opened();
	}
	else if (status == CLOSED) {
		Q_EMIT closed({
			{ "code",     closeCode()   },
			{ "reason",   closeReason() },
			{ "wasClean", closeCode() < ProtocolErrorCloseCode },
		});
	}
}

void WebSocket::setError(const QString &errorMsg, int code)
{
	static const QMetaMethod errSig = QMetaMethod::fromSignal(&WebSocket::error);

	m_errorString = errorMsg;
	if (errorMsg.isEmpty() && code == QAbstractSocket::UnknownSocketError)
		return;

	if (QJSEngine *jse = qjsEngine(this)) {
		QJSValue err = jse->newErrorObject(QJSValue::GenericError, errorMsg);
		err.setProperty("code", code);
		if (isSignalConnected(errSig))
			Q_EMIT error(err);
		else
			ScriptEngine::throwError(jse, err);
	}
}

}

#include "moc_WebSocket.cpp"
