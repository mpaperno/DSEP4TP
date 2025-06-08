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

#pragma once

#include <QDateTime>
#include <QJSValue>
#include <QObject>
#include <QUuid>
#include <QtWebSockets/QWebSocket>
#include <QtWebSockets/QWebSocketHandshakeOptions>
#include <QtWebSockets/QWebSocketProtocol>

#include "event_utils.h"

QT_BEGIN_NAMESPACE
namespace QWebSocketProtocol
{
	Q_NAMESPACE
	Q_ENUM_NS(Version)
	Q_ENUM_NS(CloseCode)
}
QT_END_NAMESPACE

#ifndef DOXYGEN
namespace ScriptLib {
#else
#define QAbstractSocket::SocketError Socket::SocketError
#define QByteArray ArrayBuffer
#define QUrl string
#define QString string
#define QJSValue object
#define QVariantMap object
#define QVariant any
#endif

class WebSocketServer;


/*!
	\class WebSocket
	\ingroup NetAPI
	\since 1.3

	\brief Implementation of the [WebSocket Web API](https://developer.mozilla.org/en-US/docs/Web/API/WebSocket)

	_WebSockets_ is a web technology providing full-duplex communications channels over a single TCP connection.

	The %WebSocket object provides the API for creating and managing a %WebSocket connection to a server, as well as for sending and receiving data on the connection.

	This implementation replicates all of the standard interface and most of the behavior and options, except as noted below. There are also many additions,
	some of which replicate the Node-js [websockets/ws](https://github.com/websockets/ws/blob/master/doc/ws.md) implementation.

	### Major differences vs. typical standard implementations:

	- The socket is _not_ automatically opened when a `WebSocket` instance is created. __You must explicitly call the `open()` method to initiate a connection.__
		Do this _after_ connecting all your event listeners/handlers.
	- `WebSocket` doesn't throw any exceptions. All errors are reported in the `error` event.
	- HTTP redirects are not supported (WebSocket will report an "Unhandled http status code: 301" error). This includes redirects from `ws:` to `wss:` protocol URLs.
	- The `binaryType` property only supports "arraybuffer", which is the default. (Note that a new type "fragments" is available, similar to the Node-js version).
	- A connected `WebSocket` instance automatically responds to all "ping" messages from the server.
	- No websocket protocol "extensions" are supported, notably _per-message-deflate_ (compression of individual messages).
		(Note that extensions are _not_ the same as "sub-protocols" describing the message exchange format, such as "json" or "xml".)

	### Example

	Exchange messages with an "echo" WebSockets server.
	```js
	// Create instance and specify the server URL. This public "echo" service sends back whatever we send it.
	const ws = new WebSocket('wss://websocket-echo.com/');

	// The "open" event is triggered once a connection to the server is established.
	ws.onopen = function() {
		console.log('WebSocket client connected at', Date.now());
		// Send a text message with the current timestamp.
		ws.send(Date.now());
	};

	// The "message" event is triggered when a message arrives from the server.
	// In this case it will be the echo reply to the message we sent in the "open" event handler above.
	ws.onmessage = function(messageEvent) {
		// Prints the time difference between current time and the time we sent to the echo server.
		console.log(`Round-trip time: ${Date.now() - messageEvent.data} ms`);
		// Once we're done with the socket, close it.  The code and reason are optional.
		ws.close(1000, "Finished!");
	};

	// The "close" event happens when the socket disconnects.
	ws.onclose = function(closeEvent) {
		console.log(`Disconnected at ${Date.now()} with code: ${closeEvent.code} reason: ${closeEvent.reason || "[none given]"}`);
		// This is a good place to delete the WebSocket instance __if we're not going to use it again__.
		ws = null;
	};

	// Error handler. The argument would be an instance of a generic `Error` type. Just connect to console output for this example.
	ws.onerror = console.error;

	// Start the connection
	ws.open();
	```
*/
class WebSocket : public QObject
{
		Q_OBJECT
		Q_DISABLE_COPY(WebSocket)

		/*! A string indicating the type of binary data being transmitted by the connection.
			This should be one of "arraybuffer" or "fragments". Defaults to "arraybuffer".

			Type "fragments" will emit the array of fragments as received from the sender, without copyfull concatenation,
			is useful for the performance of binary protocols transferring large messages with multiple fragments. The `message()`
			event's `isLast` property will be `false` for all fragments of a message except the final one.
		*/
		Q_PROPERTY(QString binaryType READ binaryType WRITE setBinaryType NOTIFY binaryTypeChanged)
		//! The number of bytes of data that have been queued using calls to send() but not yet transmitted to the network.
		Q_PROPERTY(qint64 bufferedAmount READ bytesToWrite NOTIFY bytesWritten)
		//! Returns the code indicating why the socket was closed.
		Q_PROPERTY(qint16 closeCode READ closeCode NOTIFY closed)
		//! Returns the reason why the socket was closed.
		Q_PROPERTY(QString closeReason READ closeReason NOTIFY closed)
		//! Returns the type of error that last occurred, if any, or `Socket.UnknownSocketError` if no error has been raised.  \sa errorString
#ifdef DOXYGEN
		Q_PROPERTY(Socket::SocketError errorCode READ errorCode)
#else
		Q_PROPERTY(QAbstractSocket::SocketError errorCode READ errorCode NOTIFY error)
#endif
		//! Contains a description of the last error that occurred. When no error occurred, this string is empty. \sa errorCode
		Q_PROPERTY(QString errorString READ errorString NOTIFY error)
		//! WS Extensions are not supported. This read-only property always returns an empty string.
		Q_PROPERTY(QString extensions READ extensions CONSTANT)
		//! Extra headers to include with the WebSocket requests. Object of `{ header_name: "header_value" } pairs.\n
		//! Note that setting the headers on an open (connected) socket will not have any effect until the connection is closed and re-opened.
		Q_PROPERTY(QJSValue headers READ headers WRITE setHeaders RESET clearHeaders NOTIFY headersChanged)
		//! Returns the local IP address of this socket.
		Q_PROPERTY(QString localAddress READ localAddress CONSTANT)
		//! Returns the local IP port of this socket.
		Q_PROPERTY(quint16 localPort READ localPort CONSTANT)
		//! Returns the peered server IP address.
		Q_PROPERTY(QString peerAddress READ peerAddress CONSTANT)
		//! Returns the peered server host name, if it can be resolved, or the IP address otherwise.
		Q_PROPERTY(QString peerName READ peerName CONSTANT)
		//! Returns the peered server IP port.
		Q_PROPERTY(quint16 peerPort READ peerPort CONSTANT)
		//! The WebSocket subprotocol that has been negotiated with the server.
		Q_PROPERTY(QString protocol READ subprotocol CONSTANT)
		/*! Status of the WebSocket.
			The state can have the following values:
			\li `WebSocket.CONNECTING` (0)
			\li `WebSocket.OPEN` (1)
			\li `WebSocket.CLOSING` (2)
			\li `WebSocket.CLOSED`  (3)

			\sa readyStateChanged()
		*/
		Q_PROPERTY(WebSocket::ReadyState readyState READ readyState NOTIFY readyStateChanged)
		//! The list of WebSocket subprotocols to send in the WebSocket handshake.
		Q_PROPERTY(QStringList requestedProtocols READ requestedSubprotocols WRITE setRequestedSubprotocols NOTIFY optionsChanged)
		//! Server URL to connect to. The URL must have one of 2 protocols: \e ws:// or \e wss://. When not supplied, then \e ws:// is used.
		//! If setting this property while a connection is already open then the current connection will first be closed, then re-opened with the new URL.
		Q_PROPERTY(QString url READ url WRITE setUrl NOTIFY urlChanged)
		//! Returns the protocol version the socket is currently using. Read-only. Returns `-1` if version is unknown for some reason.
		//! The desired protocol version can be set with `WebSocket::Options` either in the constructor or with `setOptions()` method, before the initial call to `open()`.
		Q_PROPERTY(qint8 version READ protocolVersion)

		EVENT_PROPERTY(bytesWritten)
		EVENT_PROPERTY(close)
		EVENT_PROPERTY_ALIAS(close, closed)
		EVENT_PROPERTY(error)
		EVENT_PROPERTY(message)
		EVENT_PROPERTY(open)
		EVENT_PROPERTY_ALIAS(open, opened)
		EVENT_PROPERTY(pong)
		EVENT_PROPERTY(readyStateChanged)

		NAMED_EVENT_HANDLER(
			{"open", "opened"},
			{"close", "closed"}
		);

	public:
		//! Values for the \ref readyState read-only property.
		enum ReadyState
		{
			CONNECTING  = 0,  //!< (0) Socket has been created but the connection is not yet open.
			OPEN        = 1,  //!< (1) The connection is open and ready to communicate.
			CLOSING     = 2,  //!< (2) The connection is in the process of closing.
			CLOSED      = 3   //!< (3) The connection is closed or couldn't be opened.
		};
		Q_ENUM(ReadyState)

		//! The close codes supported by WebSockets V13.
		enum CloseCode
		{
			NormalCloseCode                 = 1000,  //!< (1000)  Normal closure.
			GoingAwayCloseCode              = 1001,  //!< (1001)  Going away.
			ProtocolErrorCloseCode          = 1002,  //!< (1002)  Protocol error.
			DatatypeNotSupportedCloseCode   = 1003,  //!< (1003)  Unsupported data.
			Reserved1004CloseCode           = 1004,  //!< (1004)  Reserved.
			MissingStatusCodeCloseCode      = 1005,  //!< (1005)  No status received.
			AbnormalDisconnectionCloseCode  = 1006,  //!< (1006)  Abnormal closure.
			WrongDatatypeCloseCode          = 1007,  //!< (1007)  Invalid frame payload data.
			PolicyViolatedCloseCode         = 1008,  //!< (1008)  Policy violation.
			TooMuchDataCloseCode            = 1009,  //!< (1009)  Message too big.
			MissingExtensionCloseCode       = 1010,  //!< (1010)  Mandatory extension missing.
			BadOperationCloseCode           = 1011,  //!< (1011)  Internal server error.
			TlsHandshakeFailedCloseCode     = 1015,  //!< (1015)  TLS handshake failed.
		};
		Q_ENUM(CloseCode)

#ifdef DOXYGEN
		//! %Options object for use in WebSocket constructor or `setOptions()` method. All properties are optional.
		struct Options {
			//! WebSockets protocol version to use. Supported versions are: 0, 4, 5, 6, 7, 8, and 13 (default). Note that the protocol version must be set _before_ the initial call to `open()`.
			int protocolVersion = 13;
			//! Value for the `Origin` or `Sec-WebSocket-Origin` header (depending on the `protocolVersion`). Note that the origin must be set _before_ the initial call to `open()`. To change the origin afterwards, re-create the WebSocket instance entirely.
			string origin;
			//! A single string or an array of strings representing the sub-protocol(s) that the client would like to use, in order of preference. This is an alternative to setting the protocol(s) in the WebSocket constructor, and will override any specified there.
			< string | string[] > protocols;
			//! A string indicating the type of binary data being transmitted by the connection. This should be one of "arraybuffer" or "fragments". See `WebSocket::binaryType` property for details.
			string binaryType = "arraybuffer";
			//! Extra headers to include with the WebSocket requests. Object of `{ header_name: "header_value" }` pairs. Sets the `WebSocket::headers` property.
			object headers;
			//! Sets the maximum allowed size of an incoming websocket message. If an incoming message exceeds this limit, the peer gets disconnected.
			int maxIncomingMessageSize;
			//! Sets the maximum allowed size of an incoming websocket frame. If an incoming frame exceeds this limit, the peer gets disconnected.
			int maxIncomingFrameSize;
			//! Sets the maximum size of an outgoing websocket frame. Default is 512KB.
			int maxOutgoingFrameSize;
			//! The maximum allowed message size in bytes. This is the same as setting `maxIncomingMessageSize`, `maxIncomingFrameSize`, and `maxOutgoingFrameSize` properties all to the same value.
			//! If this property is present, the other 3 are ignored.
			int maxPayload;
			//! Timeout for the initial server connection, in milliseconds.
			int handshakeTimeout;
			//! Alias for `handshakeTimeout`. Timeout for the initial server connection, in milliseconds.
			int timeout;
		};
#else
		using Options = QJSValue;
#endif

		// c'tors
		explicit WebSocket(QObject *parent);

		//! Creates a new WebSocket instance with no URL or other options. The `url` property must be set before trying to open a connection with this instance.
		//! \sa url, setOptions
		Q_INVOKABLE explicit WebSocket() : WebSocket((QObject *)nullptr) { }
		//! Creates a new WebSocket instance with the given server URL. Further options may be specified in the `options` argument.
		//! \sa url, Options, setOptions()
		Q_INVOKABLE explicit WebSocket(const QString &url, const Options &options = Options()) : WebSocket(url, QStringList(), options) { }
		//! Creates a new WebSocket instance with the given server URL and an array of sub-protocol names that the client would like to use, in order of preference. Further options may be specified in the `options` argument.
		//! \sa url, requestedProtocols, protocol, Options, setOptions()
		Q_INVOKABLE explicit WebSocket(const QString &url, const QStringList &protocols, const Options &options = Options());
		//! Creates a new WebSocket instance with the given server URL and a single sub-protocol name. Further options may be specified in the `options` argument.
		//! \sa url, requestedProtocols, protocol, Options, setOptions()
		Q_INVOKABLE explicit WebSocket(const QString &url, const QString &protocol, const Options &options = Options()) : WebSocket(url, QStringList({ protocol }), options) { }

	protected:
		explicit WebSocket(QWebSocket *socket, QObject *parent = nullptr);
		friend class WebSocketServer;

	public:
		~WebSocket() override;

		QString url() const { return m_request.url().toString(); }
		void setUrl(const QString &url);

		QStringList requestedSubprotocols() const { return m_options.handshake.subprotocols(); }
		void setRequestedSubprotocols(const QStringList &subprotocols);

		QString subprotocol() const { return Q_LIKELY(m_webSocket) ? m_webSocket->subprotocol() : QString(); }

		WebSocket::ReadyState readyState() const { return m_status; }
		QAbstractSocket::SocketError errorCode() const { return Q_LIKELY(m_webSocket) ? m_webSocket->error() : QAbstractSocket::UnknownSocketError; }
		QString errorString() const { return m_errorString; }
		QString closeReason() const { return Q_LIKELY(m_webSocket) ? m_webSocket->closeReason() : QString(); }
		qint16 closeCode() const { return Q_LIKELY(m_webSocket) ? m_webSocket->closeCode() : CloseCode::NormalCloseCode; }
		qint8 protocolVersion() const { return qint8(Q_LIKELY(m_webSocket) ? m_webSocket->version() : m_options.protocolVersion); }

		QString localAddress() const { return Q_LIKELY(m_webSocket) ? m_webSocket->localAddress().toString() : QString(); }
		quint16 localPort() const { return Q_LIKELY(m_webSocket) ? m_webSocket->localPort() : 0; }
		QString peerAddress() const { return Q_LIKELY(m_webSocket) ? m_webSocket->peerAddress().toString() : QString(); }
		QString peerName() const { return Q_LIKELY(m_webSocket) ? m_webSocket->peerName() : QString(); }
		quint16 peerPort() const { return Q_LIKELY(m_webSocket) ? m_webSocket->peerPort() : 0; }

		QJSValue headers() const;
		void setHeaders(const QJSValue &headers);
		void clearHeaders();

		qint64 bytesToWrite() const;

		// "arraybuffer" or "fragments".
		QString binaryType() const { return m_options.deliveryType == FullMessage ? QStringLiteral("arraybuffer") : QStringLiteral("fragments"); }
		void setBinaryType(const QString &type);

		// WS Extensions are not supported.
		QString extensions() const { return QString(); }

	public Q_SLOTS:
		//! Immediately terminate any open connection. As opposed to `close()`, this method doesn't attempt to process any pending data.
		//! \sa close()
		void abort() const;
		//! Gracefully closes the server connection by initiating a closing handshake. Any pending data is flushed before the socket is closed, if possible.
		//! \param closeCode Optional code to indicate reason for closure. Typically one of the standard protocol codes, as enumerated by `WebSocket.CloseCode`. Default is `1000`, or "normal."
		//! \param reason Optional reason text for the closure. Default is an empty string.
		//!
		//! \sa abort()
		void close(qint16 closeCode = CloseCode::NormalCloseCode, const QString &reason = "");
		//! Attempts to open a connection to the server.
		//!
		//! An optional `callback` Function may be provided as the first argument, which will then be invoked upon a successful connection, as if it was a handler for the usual `opened` event.
		//! The callback handler is always removed after this connection attempt, even if it fails (errors are emitted in the `error` event as usual).
		//! Any successive calls to `open()` would need to specify the callback again.
#ifdef DOXYGEN
		void open(Function callback = undefined);
#else
		void open(QJSValue callback = QJSValue());
#endif
		// Extended, Node ws compatible
		//! Send a "ping" to the connected server. The server should respond with a "pong" message.
		//! \param payload Optional data to send with the ping. This will be returned by the server in the "pong" message. Default payload value is empty.
		//!
		//! \sa pong() event
		void ping(const QByteArray &payload = QByteArray()) const;

		//! Sends a message to the server.
		//! \param data { primitive | ArrayBuffer | TypedArray | DataView } The data to send.
		//! \param options { object | boolean } Optional object specifying additional sending options. If the argument is a boolean type, it is interpreted as the `binary` option described below.
		//!		\li `binary` {boolean} Specifies whether `data` should be sent as a binary or not. Default is auto-detected based on `data` type (any primitive convertible to a string vs. an ArrayBuffer/typed array/view).
		//! \returns The number of bytes written to the socket or `-1` in case of error.
		//!
		//! \sa sendText(), sendBinary()
#ifdef DOXYGEN
		int send(any data, <object | boolean> options = false);
#else
		qint64 send(const QJSValue &data, bool isBinary = false);
		qint64 send(const QJSValue &data, const QVariantMap &options) { return send(data, options.value("binary", false).toBool()); }
#endif
		//! Sends \a message to the server as UTF-8 text. `message` values that are not already strings are coerced to strings.
		//! Returns the number of bytes written to the socket or `-1` in case of error.
		//! \sa send(), sendBinary()
#ifdef DOXYGEN
		int sendText(any message);
#else
		qint64 sendText(const QJSValue &message);
#endif
		//! Sends the contents of \a bytes array buffer to the server. Returns the number of bytes written to the socket or `-1` in case of error.
		//! \sa send(), sendText()
		qint64 sendBinary(const QByteArray &bytes);

		//! Sets options on this `WebSocket` instance.\n
		//! Note that some options must be set before the socket is initially opened. And some options will not take effect until the _next_ time the socket is opened.\n
		//! The same options can be specified here as in the various constructor overloads. Any options not included in the `options` argument object are left unchanged.
		void setOptions(const Options &options);
		//! Alias for `abort()` method.
		void terminate() { abort(); }

#ifdef DOXYGEN_IGNORE_FOR_NOW
			struct Listeners {
				//! Event listener for `bytesWritten()` event.
				Function onbytesWritten;
				//! Event listener for `closed()` event.
				Function onclose;
				//! Alias for `onclose`.
				Function onclosed;
				//! Event listener for `error()` event.
				Function onerror;
				//! Event listener for `message()` event.
				Function onmessage;
				//! Event listener for `opened()` event.
				Function onopen;
				//! Alias for `onopen`.
				Function onopened;
				//! Event listener for `pong()` event.
				Function onpong;
				//! Event listener for `readyStateChanged()` event.
				Function onreadyStateChanged;
			};
			// Sets multiple event listeners at once.
			void setListeners(Listeners listeners);
#endif
		void setListeners(const QJSValue &listeners);

	Q_SIGNALS:
		/*!
			\name Events
			\sa \ref EventHandlers for general information about connecting listeners to events.
			\{
		*/

		//! Emitted after data has been sent to the server (with `send()` method). The event listener argument is the number of bytes transferred.
		//!
		//! This event has a corresponding `onbytesWritten` property to which a single event handler may be assigned.
		//! \sa bufferedAmount
		void bytesWritten(qint64 bytes);

		//! Emitted when a connection is closed. An object with the following properties is passed as an argument to event handlers:
		//! - `code` {number} - Numeric value indicating the status code explaining why the connection has been closed. This could be one of the standard websocket close codes as enumerated by `WebSocket::CloseCode`.
		//! - `reason` {string} - May contain a textual explanation of the reason for the socket closure.
		//! - `wasClean` {boolean} - A flag indicating if the socket closure was expected/"clean" (`true`) or not (`false`).
		//!
		//! This event has a corresponding `onclose` property to which a single event handler may be assigned (also aliased as `onclosed`). \n
		//! When connecting to this event with `on()` or `addEventListener()` methods, the event name may be specified as either "close" or "closed".
		void closed(const QVariantMap &closeEvent);

		//! Emitted when an error occurs. A standard `Error` type object is passed as an argument to event handlers.
		//! - `message` {string} - This standard property will contain text describing the error.
		//! - `code` {number} - Custom property indicating the corresponding error code. This should be one of the values enumerated by `Socket.SocketError`.
		//!
		//! This event has a corresponding `onerror` property to which a single event handler may be assigned.
		//!
		//! \note If no listener is assigned to this event then errors will bubble up to the global level and will be reported & logged as an "engine instance" error.
#ifdef DOXYGEN
		void error(Error error);
#else
		void error(QJSValue error);
#endif
		//! Emitted when a connection is established. This event passes no arguments to the handler.
		//!
		//! This event has a corresponding `onopen` property to which a single event handler may be assigned (also aliased as `onopened`). \n
		//! When connecting to this event with `on()` or `addEventListener()` methods, the event name may be specified as either "open" or "opened".
		void opened();

		//! Emitted when a message is received. An object with the following properties is passed as an argument to event handlers:
		//! - `data` { String | ArrayBuffer } - The message content.
		//! - `isBinary` {boolean} - specifies whether the message in `data` is binary or not.
		//! - `isLast` {boolean} - Used when `binaryType` is set to "fragments". This will be `false` for all fragments of a message except the final one.
		//!
		//! This event has a corresponding `onmessage` property to which a single event handler may be assigned.
		void message(const QVariantMap &messageEvent);

		//! This event is emitted when the `readyState` property changes.
		//! \param newState - the current state value.
		//!
		//! This event has a corresponding `onreadyStateChanged` property to which a single event handler may be assigned.
		//! \sa readyState, WebSocket.ReadyState
		void readyStateChanged(WebSocket::ReadyState newState);

		//! Emitted when a "pong" message is received from the current server. Two arguments are passed to event listeners:
		//! \param elapsedTime {number} - Round-trip time in milliseconds.
		//! \param payload {ArrayBuffer} - Optional payload data that was sent with the original "ping" message.
		//!
		//! This event has a corresponding `onpong` property to which a single event handler may be assigned.
		//! \sa ping()
		void pong(quint64 elapsedTime, const QByteArray &payload);

		//! \}

		void urlChanged();
		void headersChanged();
		void optionsChanged();
		void binaryTypeChanged(const QString &type);

	private Q_SLOTS:
		void onError(QAbstractSocket::SocketError /*sockError*/);
		void onStateChanged(QAbstractSocket::SocketState state);
		void onTextMessage(const QString &msg);
		void onBinaryMessage(const QByteArray &msg);
		void onTextFrame(const QString &frame, bool isLast);
		void onBinaryFrame(const QByteArray &frame, bool isLast);
		void setSocket(QWebSocket *socket = nullptr);  // takes ownership of the socket, or creates new socket if nullptr
		void setSocketOptions() const;
		void setStatus(ScriptLib::WebSocket::ReadyState status);
		void setError(const QString &errorMsg = QString(), int code = QAbstractSocket::UnknownSocketError);
		void connectDeliveryType();

	private:
		enum DeliveryType : quint8 { FullMessage, MessageFrames };

		struct WsOptions
		{
			QString origin;
			quint64 maxInFrameSize = 0;
			quint64 maxInMessageSize = 0;
			quint64 maxOutFrameSize = 0;
			qint64 readBufferSize = 0;
			QWebSocketHandshakeOptions handshake;
			QWebSocketProtocol::Version protocolVersion { QWebSocketProtocol::VersionLatest };
			DeliveryType deliveryType { DeliveryType::FullMessage };
		};

		QWebSocket *m_webSocket;
		ReadyState m_status;
		QNetworkRequest m_request;
		WsOptions m_options;
		QString m_errorString;
		QJSValue m_onOpenCallback;

};

#ifndef DOXYGEN
}  // namespace ScriptLib
#endif

Q_DECLARE_METATYPE(ScriptLib::WebSocket *)
Q_DECLARE_METATYPE(ScriptLib::WebSocket::ReadyState)
Q_DECLARE_METATYPE(ScriptLib::WebSocket::CloseCode)

