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

#pragma once

#include <QObject>
#include <QAbstractSocket>
#include <QByteArray>
#include <QLoggingCategory>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QVariant>

#define TP_CLIENT_VERSION_STR  "1.0.1"

// The client can be built as a dynamic library by defining `TPCLIENT_BUILD_DLL` (exports symbols),
// or the header can be used when linking to a DLL by defining `TPCLIENT_USE_DLL` (imports symbols).
// By default we assume the full source is being included directly in a build, or being built as a static lib.
#if !defined(TPCLIENT_LIB_EXPORT)
	#if defined(TPCLIENT_BUILD_DLL)
		#define TPCLIENT_LIB_EXPORT Q_DECL_EXPORT
	#elif defined(TPCLIENT_USE_DLL)
		#define TPCLIENT_LIB_EXPORT Q_DECL_IMPORT
	#else
		#define TPCLIENT_LIB_EXPORT
	#endif
#endif

#define qsvPrintable(SV)  (SV).toUtf8().constData()

Q_DECLARE_LOGGING_CATEGORY(lcTPC);

/**
The `TPClientQt` class is a simple TCP/IP network client for usage in Touch Portal plugins which wish to utilize the `Qt` C++ library/framework.

\note Familiarity with the [Touch Portal API](https://www.touch-portal.com/api) is assumed throughout this documentation.
The client performs no error checking in terms of what the user tries to send to TP, and in most cases delivers only the JSON data as it came from TP (parsed into `QJson*` types).

Messages from TP are delivered via the `message(MessageType, const QJsonObject &)` signal, which identifies the message type with an enumerator and
passes the message data on as a `QJsonObject`. Beyond determining the message type, no other processing is done on the incoming data.

Sending messages to TP can be done at 3 different levels:
- The (very overloaded) methods provided by this class which have names eponymous with the corresponding TP API message types. Eg: `stateUpdate()`, `showNotification()`, etc.
- Arbitrary JSON object via the `send()` method or from a serialized `QVariantMap` via `sendMap()`;
- Raw bytes with the `write()` method.

The client emits `connected()`, `disconnected()`, and `error()` signals to notify the plugin of connection status and network state changes.
Disconnections may happen spontaneously for a number of reasons (socket error, TP quitting, etc). Notably, `error()` is emitted if the initial connection to Touch Portal fails.

\note It is the responsibility of the plugin itself to take appropriate termination actions when the connection to TP is lost/closed, either on purpose or due to error,
and especially if/when TP sends the `closePlugin` message (which expects the plugin process to exit).

Some static convenience functions are provided for dealing with action/connector data coming from TP (for retrieving individual data values or coercing the incoming
arrays into more user-friendly structures). See the `actionData*()` function references.

The client depends on the `QtCore` and `QtNetwork` libraries/modules. Tested with Qt versions `5.12.12`, `5.15.7`, `6.4.1`.

Some minimal logging is performed via `QLoggingCategory` named "TPClientQt". By default, in Debug builds (`QT_DEBUG` defined) Debug-level messages (and above) are emitted,
while on other builds the minimum level is set to Warning.
This can be controlled as usual per Qt logging categories, eg. with `QT_LOGGING_RULES` env. var, config file, or eg. `QLoggingCategory::setFilterRules("TPClientQt.info = true");`.

__NOTE:__ \n
All methods and functions in this class are reentrant. Not thread-safe: do not attempt to send messages from multiple threads w/out serializing the access (mutex, etc).

The TPClientQt itself can be moved into a new thread if desired, as long as the above conditions remain true.
*/
class TPCLIENT_LIB_EXPORT TPClientQt : public QObject
{
	Q_OBJECT
	public:
		//! This enumeration is used in the `message()` signal to indicate message type. The names match the Touch Portal API message names, with the exception of `Unknown`.
		//! This is a strongly typed enum class because some of the names are common words. It is registered with Qt meta system and is suitable for queued signals/slots.
		enum class MessageType : short {
			Unknown,                       //!< An unknown event, perhaps from a newer version of TP which isn't supported yet.
			info,                          //!< The initial connection event, sent after pairing with TP.
			settings,                      //!< Emitted for both 'info' and 'settings' message type; value/settings array is flattened to `QJsonObject` of `{"setting name": "value", ...}` pairs.
			action,                        //!< An action click/touch event.
			down,                          //!< Action button press event.
			up,                            //!< Action button release event.
			connectorChange,               //!< Connector value change
			shortConnectorIdNotification,  //!< Connector ID mapping event.
			listChange,                    //!< Action data choice change event.
			broadcast,                     //!< Page change event.
			notificationOptionClicked,     //!< User clicked an option in a notification.
			closePlugin,                   //!< Plugin STOP event. It is the responsibility of the plugin to initiate disconnection after this message is received.
		};
		Q_ENUM(MessageType)

		//! Structure to hold information about current Touch Portal session. Populated from the initial 'info' message properties upon connection.
		//! Member names are eponymous with the properties of the 'info' message (except `paired`, see note on that).
		//! This struct is registered with Qt meta system and is suitable for queued signals/slots.  \sa tpInfo()
		struct TPInfo {
			bool paired = false;          //!< true if actively connected to TP; expects that 'status' == 'paired' in initial 'info' message. Reset to `false` when disconnected from TP.
			uint16_t sdkVersion = 0;      //!< Supported SDK version.
			uint32_t tpVersionCode = 0;   //!< Numeric Touch Portal version.
			uint32_t pluginVersion = 0;   //!< Numeric plugin version read from entry.tp file.
			QString tpVersionString;      //!< Touch Portal version number as text.
			QString status;               //!< The 'status' property from initial 'info' message (typically "paired"); This does _not_ get changed after disconnection (see `paired`).
		};

		//! Structure for action/connector data id = value pairs sent from TP. Each action/connector sends an array of these.
		//! Used with some convenience functions in this class.  \sa actionDataItem(), actionDataToItemArray()
		struct ActionDataItem {
			QString id;      //!< ID of the action data member.
			QString value;   //!< Current value of the data member.
		};

		//! Type alias for passing a JSON object-like structure via `std::vector<std::pair<const char *, const char*>>`.
		typedef std::vector<std::pair<const char*, const char *>> JsonObjectChars;
		//! Type alias for passing a JSON object-like structure via `std::vector<std::pair<std::string, std::string>>`.
		typedef std::vector<std::pair<std::string, std::string>> JsonObjectStdStr;

		//! The constructor creates the instance but does not attempt any connections.
		//! The `pluginId` will be used in the initial pairing message sent to Touch Portal, and must match ID in the plugin's entry.tp config file.
		//! You could pass a null ID here and set it later with `setPluginId()`, but an ID _is_ required before trying to connect to TP.
		//!  \sa connect()
		explicit TPClientQt(const char *pluginId, QObject *parent = nullptr);
		~TPClientQt();

		//! \name  Properties
		//! \{

		//! Returns true if connected to Touch Portal, false otherwise.
		bool isConnected() const;
		//! Returns the current state of the TCP/IP network socket used to communicate with Touch Portal.
		QAbstractSocket::SocketState socketState() const;
		//! Returns the current TCP/IP network socket error, if any.  \sa QAbstractSocket::error()
		QAbstractSocket::SocketError socketError() const;
		//! Returns the current TCP/IP network error, if any, as a human-readable string. \sa QIODevice::errorString()
		QString errorString() const;
		//! Returns information about the currently connected Touch Portal instance. This data is saved from the initial connection's 'info' message. \sa TPInfo struct.
		//! \note This reference becomes invalid when `connect()` is called.
		const TPClientQt::TPInfo &tpInfo() const;

		//! Returns the plugin ID set in constructor or with `setPluginId()`.
		QString pluginId() const;
		//! Alternate way to set or change the plugin ID. **This cannot be changed when connected to TP.** Returns `false` if that is attempted or if `pluginId` is null or empty.
		bool setPluginId(const char *pluginId);

		//! Returns the currently set Touch Portal host name/address string. This is either the default or one explicitly set with `setHostProperties()`;
		QString hostName() const;
		//! Returns the currently set Touch Portal host port. This is either the default or one explicitly set with `setHostProperties()`;
		uint16_t hostPort() const;
		//! Set the Touch Portal host name/address and port number for connection. `nameOrAddress` can be a IPv4 dotted address or a host name which will be resolved.
		//! Default host is "127.0.0.1" (local computer) and port # 12136 (TP's default for plugin API). Call this method with no arguments to reset both properties to their defaults.
		//! Changing these settings when already connected to TP has no effect until the next connection attempt.  \sa hostName(), hostPort()
		void setHostProperties(const QString &nameOrAddress = QStringLiteral("127.0.0.1"), uint16_t port = 12136);

		//! Returns the currently set connection timeout value, in milliseconds. This is either the default or one explicitly set with `setConnectionTimeout()`.
		int connectionTimeout() const;
		//! Sets the timeout value for the initial pairing 'info' message to be received from Touch Portal, in milliseconds. An `error()` signal will be sent if connection fails after this period.
		//! A timeout of `<= 0` will make the client _not_ wait for a successful pair response from TP and will assume it is connected as long as the network socket is open.
		//! The default value is 10000 (10s). Call this method with no argument to reset the timeout value to default.  \sa connectionTimeout()
		void setConnectionTimeout(int timeoutMs = 10000);

		//! \}

	Q_SIGNALS:
		//! Emitted upon successful connection and pairing with Touch Portal. Data from the initial pairing 'info' message is passed in `tpInfo`
		//! struct along with the initial `settings` object. `settings` object is the original settings array sent from Touch Portal but flattened
		//! to `QJsonObject` of `{'setting name': 'value', ...}` pairs. \sa TPInfo, MessageType::settings
		void connected(const TPClientQt::TPInfo &tpInfo, const QJsonObject &settings);
		//! Emitted upon disconnection from Touch Portal, either from an explicit call to `close()` **or** if the connection is closed unexpectedly,
		//! by the remote host (eg. Touch Portal exits) or some other unrecoverable socket error. It is the responsibility of the plugin to take
		//! any appropriate action after a disconnection or error event. The last error, if any, can be retrieved via `socketError()` method.
		void disconnected();
		//! Emitted in case of error upon initial connection or unexpected termination. This would typically be what is reported by the `QTcpSocket` being used,
		//! but during initial connection attempt it may also return one of:
		//! * `QAbstractSocket::ConnectionRefusedError` - Touch Portal refused connection with an invalid status in 'info' message.
		//! * `QAbstractSocket::SocketTimeoutError` - Network connection was established but Touch Portal didn't respond to our 'pair' message within the `connectionTimeout()` period.
		//! * `QAbstractSocket::OperationError` - Parameter validation error, eg. pluginId is null.
		void error(QAbstractSocket::SocketError error);
		//! Emitted when any message is received from Touch Portal. Refer to the TP API for specifics of each message type and what data to expect
		//! in the JSON `message` object.  The `type` is simply derived from the 'type' value found in each TP message, or `TPClientQt::MessageType::Unknown`
		//! if the message type wasn't recognized (eg. TP is using a newer API than this client supports).
		void message(TPClientQt::MessageType type, const QJsonObject &message);

	public:
		//! \name  Convenience methods / High level API; primary overloads, not for signal/slots connections.
		//! \{

		//! Send a state update with given `id` and `value` strings.
		inline void stateUpdate(const char *id, const char *value) const;

		//! Create a new dynamic state with given `id`, `parentGroup`, `description` and default value strings. Passing `nullptr` to `defaultValue` is same as using an empty string.
		inline void createState(const char *id, const char *parentGroup, const char *desc, const char *defaultValue, bool force = false) const;
		//! Create a new dynamic state with given `id`, `parentGroup`, `description` and default value strings.
		inline void createState(const std::string &id, const std::string &parentGroup, const std::string &desc, const std::string &defaultValue = "", bool force = false) const { createState(id.c_str(), parentGroup.c_str(), desc.c_str(), defaultValue.c_str(), force); }
		//! Create a new dynamic state with given `id`, `description` and default value strings. Passing `nullptr` to `defaultValue` is same as using an empty string.
		inline void createState(const char *id, const char *desc, const char *defaultValue, bool force = false) const { createState(id, nullptr, desc, defaultValue, force); }
		//! Create a new dynamic state with given `id`, `description` and default value strings.
		inline void createState(const std::string &id, const std::string &desc, const std::string &defaultValue = "", bool force = false) const { createState(id.c_str(), nullptr, desc.c_str(), defaultValue.c_str(), force); }

		//! Delete (remove) a dynamic state with given `id` string.
		inline void removeState(const char *id) const;
		//! Delete (remove) a dynamic state with given `id` string.
		inline void removeState(const std::string &id) const { removeState(id.c_str()); }

		//! Update a list of state choices for a state with given `id` using a `QJsonArray` of strings. `QJsonArray` is most efficient as it requires no further conversion before sending. \since v1.1
		inline void stateListUpdate(const char *id, const QJsonArray &values) const;
		//! Update a list of state choices for a state with given `id` using a vector of const char strings. \since v1.1
		inline void stateListUpdate(const char *id, const QVector<const char *> &values) const { stateListUpdate(id, stringContainerToJsonArray(values)); }
		//! Update a list of state choices for a state with given `id` using a list of QStrings. \since v1.1
		inline void stateListUpdate(const char *id, const QStringList &values) const { stateListUpdate(id, QJsonArray::fromStringList(values)); }

		//! Update a list of action data choices for action data with given `id` using a `QJsonArray` of strings. `QJsonArray` is most efficient as it requires no further conversion before sending.
		inline void choiceUpdate(const char *id, const QJsonArray &values) const;
		//! Update a list of action data choices for action data with given `id` using a vector of const char strings.
		inline void choiceUpdate(const char *id, const QVector<const char *> &values) const { choiceUpdate(id, stringContainerToJsonArray(values)); }
		//! Update a list of action data choices for action data with given `id` using a list of QStrings.
		inline void choiceUpdate(const char *id, const QStringList &values) const { choiceUpdate(id, QJsonArray::fromStringList(values)); }
		//! Update a list of action data choices for action data with given `id` using a `std::vector` of `std::string` types.
		inline void choiceUpdate(const std::string &id, const std::vector<std::string> &values) const { choiceUpdate(id.c_str(), stringContainerToJsonArray(values)); }
		//! Update a list of action data choices for action data with given `id` and specific `instanceId` reported by TP, using a `QJsonArray` of strings. `QJsonArray` is most efficient as it requires no further conversion before sending.
		inline void choiceUpdate(const char *id, const char *instanceId, const QJsonArray &values) const;
		//! Update a list of action data choices for action data with given `id` and specific `instanceId` reported by TP, using a vector of const char strings.
		inline void choiceUpdate(const char *id, const char *instanceId, const QVector<const char *> &values) const { choiceUpdate(id, instanceId, stringContainerToJsonArray(values)); }
		//! Update a list of action data choices for action data with given `id` and specific `instanceId` reported by TP, using a vector of const char strings.
		inline void choiceUpdate(const char *id, const char *instanceId, const QStringList &values) const { choiceUpdate(id, instanceId, QJsonArray::fromStringList(values)); }
		//! Update a list of action data choices for action data with given `id` and specific `instanceId` reported by TP, using a `std::vector` of `std::string` types.
		inline void choiceUpdate(const std::string &id, const std::string &instanceId, const std::vector<std::string> &values) const { choiceUpdate(id.c_str(), instanceId.c_str(), stringContainerToJsonArray(values)); }

		//! Update a Connector value with given `shortId` as reported by TP. Valid value range is 0-100.
		inline void connectorUpdate(const char *shortId, uint8_t value) const;
		//! Update a Connector value with given `shortId` as reported by TP. Valid value range is 0-100.
		inline void connectorUpdate(const std::string &shortId, uint8_t value) const { connectorUpdate(shortId.c_str(), value); }

		//! Update a Connector value with given full `connectortId` (see TP API docs for details). Valid value range is 0-100.
		//! If `addPrefix` is set to `true` then the string `"pc_" + the plugin ID + "_"` is prepended to the given `connectorId` value.
		inline void connectorUpdate(const char *connectortId, uint8_t value, bool addPrefix) const;
		//! Update a Connector value with given full `connectortId` (see TP API docs for details). Valid value range is 0-100.
		//! If `addPrefix` is set to `true` then the string `"pc_" + the plugin ID + "_"` is prepended to the given `connectorId` value.
		inline void connectorUpdate(const std::string &connectortId, uint8_t value, bool addPrefix) const { connectorUpdate(connectortId.c_str(), value, addPrefix); }
		//! Update a Connector value with given `connectortId`. This overload takes a mapping of action data id/value pairs and builds up the long connectorId string for you
		//! (see TP API docs for details on long connectorId format). Valid value range is 0-100.
		//! If `addPrefix` is set to `true` then the string `"pc_" + the plugin ID + "_"` is prepended to the given `connectorId` value.
		inline void connectorUpdate(const char *connectortId, const QMap<const char*, const char*> &nvPairs, uint8_t value, bool addPrefix = true) const;

		//! Update a plugin setting value with given `name` to `value`.
		inline void settingUpdate(const char *name, const char *value) const;
		//! Update a plugin setting value with given `name` to `value`.
		inline void settingUpdate(const std::string &name, const std::string &value) const { settingUpdate(name.c_str(), value.c_str()); }

		//! Send a notification message to TP. See TP SDK documentation for details on each field. The `options` variant list should be some kind of array containing
		//! the notification option `id` and `title` pairs/objects, for example a `QJsonArray<QJsonObject>({'id', 'title'})`.
		inline void showNotification(const char *notificationId, const char *title, const char *msg, const QJsonArray &options) const;
		//! Send a notification message to TP. See TP SDK documentation for details on each field. The `options` variant list should be some kind of array containing
		//! the notification option `id` and `title` pairs/objects, for example a `QVariantList<QVariantMap>({'id', 'title'})`.
		inline void showNotification(const char *notificationId, const char *title, const char *msg, const QVariantList &options) const { showNotification(notificationId, title, msg, QJsonArray::fromVariantList(options)); }
		//! Send a notification message to TP. See TP SDK documentation for details on each field. The options are passed as an array of id/title pairs which are converted to appropriate JSON objects, eg: `{'id': pair.first, 'title': pair.second}`.
		inline void showNotification(const char *notificationId, const char *title, const char *msg, const QVector<QPair<const char*, const char *>> &options) const;
		//! Send a notification message to TP. See TP SDK documentation for details on each field. The options are passed as an array of id/title pairs which are converted to appropriate JSON objects, eg: `{'id': pair.first, 'title': pair.second}`.
		inline void showNotification(const char *notificationId, const char *title, const char *msg, const JsonObjectChars &options) const;
		//! Send a notification message to TP. See TP SDK documentation for details on each field. The options are passed as an array of id/title pairs which are converted to appropriate JSON objects, eg: `{'id': pair.first, 'title': pair.second}`.
		inline void showNotification(const std::string &notificationId, const std::string &title, const std::string &msg, const JsonObjectStdStr &options = JsonObjectStdStr()) const;


		//! Trigger a plugin's TP event with given `id` with optional `states` object. The `states` array should contain
		//! key/value pairs corresponding to how the event is defined in the plugins entry.tp. For example a `QJsonArray<QJsonObject>({ {"localState1", "value 1"}, {"localState2", "value 2"} })`.
		inline void triggerEvent(const char *eventId, const QJsonObject &states = QJsonObject()) const;
		//! Trigger a plugin's TP event with given `id` and `states`. The `states` variant list should be some kind of array containing
		//! key/value pairs corresponding to how the event is defined in the plugins entry.tp. For example a `QVariantList<QVariantMap>({ {"localState1", "value 1"}, {"localState2", "value 2"} })`.
		inline void triggerEvent(const char *eventId, const QVariantMap &states) const { triggerEvent(eventId,  QJsonObject::fromVariantMap(states)); }
		//! Trigger a plugin's TP event with given `id` and `states`. The `states` array should contain
		//! key/value pairs corresponding to how the event is defined in the plugins entry.tp. For example a `std::vector<std::pair<const char*, const char *>>({ {"localState1", "value 1"}, {"localState2", "value 2"} })`.
		inline void triggerEvent(const char *eventId, const JsonObjectChars &states) const { triggerEvent(eventId, arrayOfObjectsToJsonOject(states)); }
		//! Trigger a plugin's TP event with given `id` with optional `states` object. The `states` array should contain
		//! key/value pairs corresponding to how the event is defined in the plugins entry.tp. For example a `std::vector<std::pair<std::string, std::string>>({ {"localState1", "value 1"}, {"localState2", "value 2"} })`.
		inline void triggerEvent(const std::string &eventId, const JsonObjectStdStr &states = JsonObjectStdStr()) const { triggerEvent(eventId.c_str(), arrayOfObjectsToJsonOject(states)); }

		//! \}

		//! \name  Low level API
		//! \{

		//! Low-level API: Serializes a JSON object to UTF8 bytes. `object` should contain one TP message. This can be then be sent to TP via `write()` method.
		QByteArray encode(const QJsonObject &object) const {
			// const QByteArray ba = QJsonDocument(object).toJson(QJsonDocument::Compact);
			// qCDebug(lcTPC).noquote() << ba;
			return QJsonDocument(object).toJson(QJsonDocument::Compact);
		}
		//! \}

	public Q_SLOTS:

		//! \name  Connection handlers
		//! \{

		//! Initiate a connection to Touch Portal. The plugin ID (set in constructor or with `setPluginId()` must be valid.
		//! This first tries to open a network socket connection, and if that succeeds then the initial 'pair' message is sent to Touch Portal.
		//! Upon actual successful pairing, meaning an 'info' message response was received from TP with 'status' == "paired", the `connected()` signal is emitted.
		//! If either the initial socket connection or pairing with TP fails within the `connectionTimeout()` period, then the `error(QAbstractSocket::SocketError)` signal is emitted.
		//! If `connectionTimeout()` is `<= 0` then the client will not wait for a successful pair response from TP and will assume it is connected as long as the network socket is open.
		//! \sa error() signal, connectionTimeout(), setConnectionTimeout()
		void connect();
		//! Initiate a connection to Touch Portal. The plugin ID (set in constructor or with `setPluginId()` must be valid.
		//! This is a "shortcut" method which first calls `setConnectionTimeout(timeout)` and `setHostProperties(host, port)` before calling `connect()`.
		//! To ignore any of the arguments (not change the corresponding TPClientQt properties), pass `timeout < 0`, `host = QString()` (default), and/or `port = 0` (default).
		//! \sa connect()
		inline void connect(int timeout, const QString &host = QString(), uint16_t port = 0);
		//! Initiates disconnection from Touch Portal. This flushes and gracefully closes any open network sockets.
		//! The `disconnected()` signal will be emitted upon actual disconnection (which may not be immediate).
		//! Returns immediately if the connection isn't already open.
		void disconnect() const;

		//! \}

		//! \name  Low level API
		//! \{

		//! Low-level API: Send JSON message data to Touch Portal. `object` should contain one TP message.
		//! All other methods for sending structured data are conveniences for this method.
		inline void send(const QJsonObject &object) const { write(encode(object)); }
		//! Low-level API: Send a JSON representation of a variant map to Touch Portal. `map` should contain one TP message. The map is serialized as QJsonObject type.
		inline void sendMap(const QVariantMap &map) const { write(encode(QJsonObject::fromVariantMap(map))); }
		//! Low-level API: Write UTF-8 bytes directly to Touch Portal. `data` should contain one TP message in the form of a serialized (UTF8 text) JSON object.
		//! A newline is automatically added after `data` is sent (as per TP API specs). All messages are ultimately sent via this method.
		void write(const QByteArray &data) const;

		//! \}

		//! \name Convenience methods / High level API, Overloads & Slots
		//! \{

		//! Send a state update with given `id` and `value` strings.
		inline void stateUpdate(const QByteArray &id, const QByteArray &value) const { stateUpdate(id.constData(), value.constData()); }
		//! Send a state update with given `id` and `value` strings.
		inline void stateUpdate(QStringView id, QStringView value) const { stateUpdate(qsvPrintable(id), qsvPrintable(value)); }
		//! Send a state update with given `id` and `value` strings.
		inline void stateUpdate(const QByteArray &id, QStringView value) const { stateUpdate(id, value.toUtf8()); }

		//! Create a new dynamic state with given `id`, `parentGroup`, `description` and `defaultValue` strings
		inline void createState(const QByteArray &id, const QByteArray &parentGroup, const QByteArray &desc, const QByteArray &defaultValue) const { createState(id.constData(), parentGroup.constData(), desc.constData(), defaultValue.constData()); }
		inline void createState(const QByteArray &id, const QByteArray &parentGroup, const QByteArray &desc, const QByteArray &defaultValue, bool force) const { createState(id.constData(), parentGroup.constData(), desc.constData(), defaultValue.constData(), force); }
		//! Create a new dynamic state with given `id`, `parentGroup`, `description` and `defaultValue` strings
		inline void createState(QStringView id, QStringView parentGroup, QStringView desc, QStringView defaultValue) const { createState(qsvPrintable(id), qsvPrintable(parentGroup), qsvPrintable(desc), qsvPrintable(defaultValue)); }
		inline void createState(QStringView id, QStringView parentGroup, QStringView desc, QStringView defaultValue, bool force) const { createState(qsvPrintable(id), qsvPrintable(parentGroup), qsvPrintable(desc), qsvPrintable(defaultValue), force); }
		//! Create a new dynamic state with given `id`, `description` and `defaultValue` strings.
		inline void createState(const QByteArray &id, const QByteArray &desc, const QByteArray &defaultValue) const { createState(id.constData(), nullptr, desc.constData(), defaultValue.constData()); }
		inline void createState(const QByteArray &id, const QByteArray &desc, const QByteArray &defaultValue, bool force) const { createState(id.constData(), nullptr, desc.constData(), defaultValue.constData(), force); }
		//! Create a new dynamic state with given `id`, `description` and `defaultValue` strings.
		inline void createState(QStringView id, QStringView desc, QStringView defaultValue) const { createState(qsvPrintable(id), nullptr, qsvPrintable(desc), qsvPrintable(defaultValue)); }
		inline void createState(QStringView id, QStringView desc, QStringView defaultValue, bool force) const { createState(qsvPrintable(id), nullptr, qsvPrintable(desc), qsvPrintable(defaultValue), force); }

		//! Delete (remove) a dynamic state with given `id` string.
		inline void removeState(const QByteArray &id) const { removeState(id.constData()); }
		//! Delete (remove) a dynamic state with given `id` string.
		inline void removeState(QStringView id) const { removeState(qsvPrintable(id)); }

		//! Update a list of state choices for a state with given `id` using a `QJsonArray` of strings. `QJsonArray` is most efficient as it requires no further conversion before sending. \since v1.1
		inline void stateListUpdate(const QByteArray &id, const QJsonArray &values) const { stateListUpdate(id.constData(), values); }
		//! Update a list of state choices for a state with given `id` using a vector of `QByteArray` types. \since v1.1
		inline void stateListUpdate(const QByteArray &id, const QByteArrayList &values) const { stateListUpdate(id.constData(), stringContainerToJsonArray(values)); }
		//! Update a list of state choices for a state with given `id` using a list of `QString`s. \since v1.1
		inline void stateListUpdate(const QByteArray &id, const QStringList &values) const { stateListUpdate(id.constData(), QJsonArray::fromStringList(values)); }

		//! Update a list of action data choices for action data with given `id` using a `QJsonArray` of strings. `QJsonArray` is most efficient as it requires no further conversion before sending.
		inline void choiceUpdate(const QByteArray &id, const QJsonArray &values) const { choiceUpdate(id.constData(), values); }
		//! Update a list of action data choices for action data with given `id` using a vector of `QByteArray` types.
		inline void choiceUpdate(const QByteArray &id, const QVector<QByteArray> &values) const { choiceUpdate(id.constData(), stringContainerToJsonArray(values)); }
		//! Update a list of action data choices for action data with given `id` using a list of `QString`s.
		inline void choiceUpdate(const QByteArray &id, const QStringList &values) const { choiceUpdate(id.constData(), QJsonArray::fromStringList(values)); }
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0)) || defined(DOXYGEN)
		//! Update a list of action data choices for action data with given `id` using a `QByteArray` list.
		//! \note This method only exists on Qt < v6. With v6+ `QByteArrayList` is the same as `QList<QByteArray>`.
		inline void choiceUpdate(const QByteArray &id, const QByteArrayList &values) const { choiceUpdate(id.constData(), stringContainerToJsonArray(values)); }
#endif
		//! Update a list of action data choices for action data with given `id` using a vector of `QJsonArray` of strings. `QJsonArray` is most efficient as it requires no further conversion before sending.
		inline void choiceUpdate(QStringView id, const QJsonArray &values) const { choiceUpdate(qsvPrintable(id), values); }
		//! Update a list of action data choices for action data with given `id` using a vector of `QStringView`-compatible types.
		inline void choiceUpdate(QStringView id, const QVector<QStringView> &values) const { choiceUpdate(qsvPrintable(id), stringContainerToJsonArray(values)); }
		//! Update a list of action data choices for action data with given `id` using a list of `QString`s.
		inline void choiceUpdate(QStringView id, const QStringList &values) const { choiceUpdate(qsvPrintable(id), QJsonArray::fromStringList(values)); }
		//! Update a list of action data choices for action data with given `id` using a `QByteArray` list.
		inline void choiceUpdate(QStringView id, const QByteArrayList &values) const { choiceUpdate(qsvPrintable(id), stringContainerToJsonArray(values)); }

		//! Update a list of action data choices for action data with given `id` and specific `instanceId` reported by TP, using a `QJsonArray` of strings. `QJsonArray` is most efficient as it requires no further conversion before sending.
		inline void choiceUpdate(const QByteArray &id, const QByteArray &instanceId, const QJsonArray &values) const { choiceUpdate(id.constData(), instanceId.constData(), values); }
		//! Update a list of action data choices for action data with given `id` and specific `instanceId` reported by TP, using a vector of `QByteArray` types.
		inline void choiceUpdate(const QByteArray &id, const QByteArray &instanceId, const QVector<QByteArray> &values) const { choiceUpdate(id.constData(), instanceId.constData(), stringContainerToJsonArray(values)); }
		//! Update a list of action data choices for action data with given `id` and specific `instanceId` reported by TP, using a list of `QString`s.
		inline void choiceUpdate(const QByteArray &id, const QByteArray &instanceId, const QStringList &values) const { choiceUpdate(id.constData(), instanceId.constData(), QJsonArray::fromStringList(values)); }
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0)) || defined(DOXYGEN)
		//! Update a list of action data choices for action data with given `id` and specific `instanceId` reported by TP, using a `QByteArray` list.
		//! \note This method only exists on Qt < v6. With v6+ `QByteArrayList` is the same as `QVector<QByteArray>`.
		inline void choiceUpdate(const QByteArray &id, const QByteArray &instanceId, const QByteArrayList &values) const { choiceUpdate(id.constData(), instanceId.constData(), stringContainerToJsonArray(values)); }
#endif
		//! Update a list of action data choices for action data with given `id` and specific `instanceId` reported by TP, using a `QJsonArray` of strings. `QJsonArray` is most efficient as it requires no further conversion before sending.
		inline void choiceUpdate(QStringView id, QStringView instanceId, const QJsonArray &values) const { choiceUpdate(qsvPrintable(id), qsvPrintable(instanceId), values); }
		//! Update a list of action data choices for action data with given `id` and specific `instanceId` reported by TP, using a vector of `QStringView`-compatible types.
		inline void choiceUpdate(QStringView id, QStringView instanceId, const QVector<QStringView> &values) const { choiceUpdate(qsvPrintable(id), qsvPrintable(instanceId), stringContainerToJsonArray(values)); }
		//! Update a list of action data choices for action data with given `id` and specific `instanceId` reported by TP, using a list of `QString`s.
		inline void choiceUpdate(QStringView id, QStringView instanceId, const QStringList &values) const { choiceUpdate(qsvPrintable(id), qsvPrintable(instanceId), QJsonArray::fromStringList(values)); }
		//! Update a list of action data choices for action data with given `id` and specific `instanceId` reported by TP, using a `QByteArray` list.
		inline void choiceUpdate(QStringView id, QStringView instanceId, const QByteArrayList &values) const { choiceUpdate(qsvPrintable(id), qsvPrintable(instanceId), stringContainerToJsonArray(values)); }

		//! Update a Connector value with given `shortId` as reported by TP. Valid value range is 0-100.
		inline void connectorUpdate(const QByteArray &shortId, uint8_t value) const { connectorUpdate(shortId.constData(), value); }
		//! Update a Connector value with given `shortId` as reported by TP. Valid value range is 0-100.
		inline void connectorUpdate(QStringView shortId, uint8_t value) const { connectorUpdate(qsvPrintable(shortId), value); }

		//! Update a Connector value with given full `connectortId` (see TP API docs for details). Valid value range is 0-100.
		//! If `addPrefix` is set to `true` then the string `"pc_" + the plugin ID + "_"` is prepended to the given `connectorId` value.
		inline void connectorUpdate(const QByteArray &connectortId, uint8_t value, bool addPrefix) const { connectorUpdate(connectortId.constData(), value, addPrefix); }
		//! Update a Connector value with given full `connectortId` (see TP API docs for details). Valid value range is 0-100.
		//! If `addPrefix` is set to `true` then the string `"pc_" + the plugin ID + "_"` is prepended to the given `connectorId` value.
		inline void connectorUpdate(QStringView connectortId, uint8_t value, bool addPrefix) const { connectorUpdate(qsvPrintable(connectortId), value, addPrefix); }
		//! Update a Connector value with given `connectortId`. This overload takes a mapping of action data id/value pairs and builds up the long connectorId string for you
		//! (see TP API docs for details on long connectorId format). Valid value range is 0-100.
		//! If `addPrefix` is set to `true` then the string `"pc_" + the plugin ID + "_"` is prepended to the given `connectorId` value.
		inline void connectorUpdate(const QByteArray &connectortId, const QMap<const QByteArray, const QByteArray> &nvPairs, uint8_t value, bool addPrefix = true) const;
		//! Update a Connector value with given `connectortId`. This overload takes a mapping of action data id/value pairs and builds up the long connectorId string for you
		//! (see TP API docs for details on long connectorId format). Valid value range is 0-100.
		//! If `addPrefix` is set to `true` then the string `"pc_" + the plugin ID + "_"` is prepended to the given `connectorId` value.
		inline void connectorUpdate(QStringView connectortId, const QMap<QStringView, QStringView> &nvPairs, uint8_t value, bool addPrefix = true) const;

		//! Update a plugin setting value with given `name` to `value`.
		inline void settingUpdate(const QByteArray &name, const QByteArray &value) const { settingUpdate(name.constData(), value.constData()); }
		//! Update a plugin setting value with given `name` to `value`.
		inline void settingUpdate(QStringView name, QStringView value) const { settingUpdate(qsvPrintable(name), qsvPrintable(value)); }

		//! Send a notification message to TP. See TP SDK documentation for details on each field. The `options` variant list should be some kind of array containing
		//! the notification option `id` and `title` pairs/objects, for example a `QVariantList<QVariantMap({'id', 'title'})>`.
		inline void showNotification(const QByteArray &notificationId, const QByteArray &title, const QByteArray &msg, const QJsonArray &options) const { showNotification(notificationId.constData(), title.constData(), msg.constData(), options); }
		//! Send a notification message to TP. See TP SDK documentation for details on each field. The `options` variant list should be some kind of array containing
		//! the notification option `id` and `title` pairs/objects, for example a `QVariantList<QVariantMap({'id', 'title'})>`.
		inline void showNotification(const QByteArray &notificationId, const QByteArray &title, const QByteArray &msg, const QVariantList &options) const { showNotification(notificationId.constData(), title.constData(), msg.constData(), QJsonArray::fromVariantList(options)); }
		//! Send a notification message to TP. See TP SDK documentation for details on each field. The `options` variant list should be some kind of array containing
		//! the notification option `id` and `title` pairs/objects, for example a `QVariantList<QVariantMap({'id', 'title'})>`.
		inline void showNotification(QStringView notificationId, QStringView title, QStringView msg, const QVariantList &options) const { showNotification(qsvPrintable(notificationId), qsvPrintable(title), qsvPrintable(msg), options); }
		//! Send a notification message to TP. See TP SDK documentation for details on each field. The options are passed as an array of id/title pairs which are converted to appropriate JSON objects, eg: `{'id': pair.first, 'title': pair.second}`.
		inline void showNotification(const QByteArray &notificationId, const QByteArray &title, const QByteArray &msg, const QVector<QPair<QLatin1String, QStringView>> &options) const;
		//! Send a notification message to TP. See TP SDK documentation for details on each field. The options are passed as an array of id/title pairs which are converted to appropriate JSON objects, eg: `{'id': pair.first, 'title': pair.second}`.
		inline void showNotification(QStringView notificationId, QStringView title, QStringView msg, const QVector<QPair<QStringView, QStringView> > &options) const;

		//! Trigger a plugin's TP event with given `id` with optional `states` object. The `states` array should contain
		//! key/value pairs corresponding to how the event is defined in the plugins entry.tp. For example a `QJsonArray<QJsonObject>({ {"localState1", "value 1"}, {"localState2", "value 2"} })`.
		inline void triggerEvent(const QByteArray &eventId, const QJsonObject &states = QJsonObject()) const { triggerEvent(eventId.constData(), states); }
		//! Trigger a plugin's TP event with given `id` and `states`. The `states` variant list should be some kind of array containing
		//! key/value pairs corresponding to how the event is defined in the plugins entry.tp. For example a `QVariantList<QVariantMap>({ {"localState1", "value 1"}, {"localState2", "value 2"} })`.
		inline void triggerEvent(const QByteArray &eventId, const QVariantMap &states) const { triggerEvent(eventId.constData(), QJsonObject::fromVariantMap(states)); }
		//! Trigger a plugin's TP event with given `id` with optional `states` object. The `states` array should contain
		//! key/value pairs corresponding to how the event is defined in the plugins entry.tp. For example a `QVector<QPair<QLatin1String, QStringView>>({ {"localState1", "value 1"}, {"localState2", "value 2"} })`.
		inline void triggerEvent(const QByteArray &eventId, const QVector<QPair<QLatin1String, QStringView>> &states) const { triggerEvent(eventId.constData(), arrayOfObjectsToJsonOject(states)); }
		//! Trigger a plugin's TP event with given `id` with optional `states` object. The `states` array should contain
		//! key/value pairs corresponding to how the event is defined in the plugins entry.tp. For example a `QVector<QPair<QStringView, QStringView>>({ {"localState1", "value 1"}, {"localState2", "value 2"} })`.
		inline void triggerEvent(QStringView eventId, const QVector<QPair<QStringView, QStringView>> &states) const { triggerEvent(eventId.toUtf8(), arrayOfObjectsToJsonOject(states)); }

		//! \}

	public:
		//! \name  Static helper functions
		//! \{

		//! Returns the action data object (id and value as `ActionDataItem` struct) at given index in the given array of action/connector data values (as sent from TP),
		//! or the optional `defaultItem` item if the index is out of bounds (or a default-constructed `ActionDataItem` if `defaultItem` is not provided).
		static inline ActionDataItem actionDataItem(int index, const QJsonArray &data, const ActionDataItem &defaultItem = ActionDataItem());
		//! Returns the 'value' member of the action data object at given index in the given array of action/connector data values (as sent from TP),
		//! or the given `defaultValue` if the data member doesn't exist in the array (or for some reason doesn't have a 'value' member).
		static inline QString actionDataValue(int index, const QJsonArray &data, const QString &defaultValue = QLatin1String(""));
		//! Returns the 'value' member of the action data object with given id in the given array of action/connector data values (as sent from TP),
		//! or the given `defaultValue` if the data member doesn't exist in the array (or for some reason doesn't have a 'value' member).
		//! \note This has to loop over the data array until the member with given `id` is found (or not). If needing named data values
		//! more than once per action processing, it would likely be more efficient to use `actionDataToMap()` and store the result.
		static inline QString actionDataValue(const char *id, const QJsonArray &data, const QString &defaultValue = QLatin1String(""));
		//! Convert JSON array of action data objects: [ { 'id': id, 'value': value }, ... ] to a vector of `ActionDataItem` types.
		//! If `separator` is not zero, the data IDs will be split on this character and only the last part will be saved in the returned `ActionDataItem`s' id.
		static inline QVector<ActionDataItem> actionDataToItemArray(const QJsonArray &data, char separator = 0);
		//! Flattens array of objects: [ { 'id': id, 'value': value }, ... ] to a single mapping of { {id, value}, ... }  (assuming each data id key is unique).
		//! This allows simplified lookup by data ID string (vs. index), and is also iterable. The returned map does not preserve the original data order.
		//! If `separator` is not zero, the data IDs will be split on this character and only the last part will be saved in the returned map's key names.
		static inline QMap<QString, QString> actionDataToMap(const QJsonArray &data, char separator = 0);

		//! \}

	private Q_SLOTS:
		void onReadyRead();

	private:
		struct Private;
		friend struct Private;
		Private * const d;

		template <typename Vect, typename T = typename Vect::value_type>
		QJsonArray stringContainerToJsonArray(const Vect &list) const
		{
			QJsonArray ret;
			for (typename Vect::const_iterator it = list.cbegin(); it != list.cend(); ++it) {
				const T &val = static_cast<T>(* it);
				if constexpr (std::is_same_v<T, QStringView>)
					ret.append(val.toString());
				else if constexpr (std::is_same_v<T, QByteArray>)
					ret.append(QString(val));
				else if constexpr (std::is_same_v<T, std::string>)
					ret.append(val.c_str());
				else if constexpr (std::is_same_v<T, std::wstring>)
					ret.append(QString::fromStdWString(val));
				else
					ret.append(QJsonValue(val));
			}
			return ret;
		}

		template <typename Vect,
		          typename Pair = typename Vect::value_type,
		          typename Key = std::remove_const_t<typename Pair::first_type>,
		          typename Val = std::remove_const_t<typename Pair::second_type> >
		QJsonArray arrayOfObjectsToJsonArray(const Vect &list) const
		{
			QJsonArray ret;
			for (typename Vect::const_iterator it = list.cbegin(); it != list.cend(); ++it) {
				const Key &key = static_cast<Key>(it->first);
				const Val &val = static_cast<Val>(it->second);
				QJsonValue jval;
				if constexpr (std::is_same_v<Val, QStringView>)
					jval = val.toString();
				else if constexpr (std::is_same_v<Val, QByteArray>)
					jval = QString(val);
				else if constexpr (std::is_same_v<Val, std::string>)
					jval = val.c_str();
				else if constexpr (std::is_same_v<Val, std::wstring>)
					jval = QString::fromStdWString(val);
				else
					jval = val;

				if constexpr (std::is_same_v<Key, QStringView>)
					ret.append({ {key.toString(), jval} });
				else if constexpr (std::is_same_v<Key, std::string>)
					ret.append({ {QString::fromStdString(key), jval} });
				else
					ret.append({ {QString(key), jval} });
			}
			return ret;
		}

		template <typename Vect,
		          typename Pair = typename Vect::value_type,
		          typename Key = std::remove_const_t<typename Pair::first_type>,
		          typename Val = std::remove_const_t<typename Pair::second_type> >
		QJsonObject arrayOfObjectsToJsonOject(const Vect &list) const
		{
			QJsonObject ret;
			for (typename Vect::const_iterator it = list.cbegin(); it != list.cend(); ++it) {
				const Key &key = static_cast<Key>(it->first);
				const Val &val = static_cast<Val>(it->second);
				QJsonValue jval;
				if constexpr (std::is_same_v<Val, QStringView>)
					jval = val.toString();
				else if constexpr (std::is_same_v<Val, QByteArray>)
					jval = QString(val);
				else if constexpr (std::is_same_v<Val, std::string>)
					jval = val.c_str();
				else if constexpr (std::is_same_v<Val, std::wstring>)
					jval = QString::fromStdWString(val);
				else
					jval = val;

				if constexpr (std::is_same_v<Key, QStringView>)
					ret.insert(key.toString(), jval);
				else if constexpr (std::is_same_v<Key, std::string>)
					ret.insert(QString::fromStdString(key), jval);
				else
					ret.insert(QString(key), jval);
			}
			return ret;
		}
};

inline
void TPClientQt::connect(int timeout, const QString &host, uint16_t port)
{
	if (timeout > -1)
		setConnectionTimeout(timeout);
	setHostProperties(host, port);
	connect();
}

inline
void TPClientQt::stateUpdate(const char *id, const char *value) const
{
	send({
		{"type", "stateUpdate"},
		{"id", id},
		{"value", value}
	});
}

inline
void TPClientQt::createState(const char *id, const char *parentGroup, const char *desc, const char *defaultValue, bool force) const
{
	send({
		{"type", "createState"},
		{"id", id},
		{"desc", desc ? desc : ""},
		{"defaultValue", defaultValue ? defaultValue : ""},
		{"parentGroup", parentGroup ? parentGroup : ""},
		{"forceUpdate", force}
	});
}

inline
void TPClientQt::removeState(const char *id) const
{
	send({
		{"type", "removeState"},
		{"id", id},
	});
}

inline
void TPClientQt::choiceUpdate(const char *id, const QJsonArray &values) const
{
	send({
		{"type", "choiceUpdate"},
		{"id", id},
		{"value", values}
	});
}

inline
void TPClientQt::stateListUpdate(const char *id, const QJsonArray &values) const
{
	send({
		{"type", "stateListUpdate"},
		{"id", id},
		{"value", values}
	});
}

inline
void TPClientQt::choiceUpdate(const char *id, const char *instanceId, const QJsonArray &values) const
{
	send({
		{"type", "choiceUpdate"},
		{"id", id},
		{"instanceId", instanceId},
		{"value", values}
	});
}

inline
void TPClientQt::connectorUpdate(const char *shortId, uint8_t value) const
{
	send({
		{"type", "connectorUpdate"},
		{"shortId", shortId},
		{"value", value}
	});
}

inline
void TPClientQt::connectorUpdate(const char *connectortId, uint8_t value, bool addPrefix) const
{
	send({
		{"type", "connectorUpdate"},
		{"connectorId", addPrefix ? qsvPrintable("pc_" + pluginId() + '_' + connectortId) : connectortId},
		{"value", value}
	});
}

inline
void TPClientQt::connectorUpdate(const char *connectortId, const QMap<const char *, const char *> &nvPairs, uint8_t value, bool addPrefix) const
{
	QString fullId(connectortId);
	for (auto nvp = nvPairs.cbegin(), en = nvPairs.cend(); nvp != en; ++nvp)
		fullId += '|' + QByteArray(nvp.key()) + '=' + QByteArray(nvp.value());
	connectorUpdate(fullId.toUtf8(), value, addPrefix);
}

inline
void TPClientQt::connectorUpdate(const QByteArray &connectortId, const QMap<const QByteArray, const QByteArray> &nvPairs, uint8_t value, bool addPrefix) const
{
	QString fullId(connectortId);
	for (auto nvp = nvPairs.cbegin(), en = nvPairs.cend(); nvp != en; ++nvp)
		fullId += '|' + nvp.key() + '=' + nvp.value();
	connectorUpdate(fullId.constData(), value, addPrefix);
}

inline
void TPClientQt::connectorUpdate(QStringView connectortId, const QMap<QStringView, QStringView> &nvPairs, uint8_t value, bool addPrefix) const
{
	QString fullId(connectortId.toString());
	for (auto nvp = nvPairs.cbegin(), en = nvPairs.cend(); nvp != en; ++nvp)
		fullId += '|' + nvp.key() + '=' + nvp.value();
	connectorUpdate(fullId.toUtf8(), value, addPrefix);
}

inline
void TPClientQt::settingUpdate(const char *name, const char *value) const
{
	send({
		{"type", "settingUpdate"},
		{"name", name},
		{"value", value}
	});
}

inline
void TPClientQt::showNotification(const char *notificationId, const char *title, const char *msg, const QJsonArray &options) const
{
	send({
		{"type", "showNotification"},
		{"notificationId", notificationId},
		{"title", title},
		{"msg", msg},
		{"options", options}
	});
}

inline
void TPClientQt::showNotification(const char *notificationId, const char *title, const char *msg, const QVector<QPair<const char *, const char *> > &options) const
{
	showNotification(notificationId, title, msg, arrayOfObjectsToJsonArray(options));
}

inline
void TPClientQt::showNotification(const char *notificationId, const char *title, const char *msg, const JsonObjectChars &options) const
{
	showNotification(notificationId, title, msg, arrayOfObjectsToJsonArray(options));
}

inline void TPClientQt::showNotification(const std::string &notificationId, const std::string &title, const std::string &msg, const JsonObjectStdStr &options) const
{
	showNotification(notificationId.c_str(), title.c_str(), msg.c_str(), arrayOfObjectsToJsonArray(options));
}

inline
void TPClientQt::showNotification(const QByteArray &notificationId, const QByteArray &title, const QByteArray &msg, const QVector<QPair<QLatin1String, QStringView> > &options) const
{
	showNotification(notificationId.constData(), title.constData(), msg.constData(), arrayOfObjectsToJsonArray(options));
}

inline
void TPClientQt::showNotification(QStringView notificationId, QStringView title, QStringView msg, const QVector<QPair<QStringView, QStringView> > &options) const
{
	showNotification(notificationId.toUtf8(), title.toUtf8(), msg.toUtf8(), arrayOfObjectsToJsonArray(options));
}


inline
void TPClientQt::triggerEvent(const char *eventId, const QJsonObject &states) const
{
	send({
		{"type", "triggerEvent"},
		{"eventId", eventId},
		{"states", states}
	});
}


// static
inline
TPClientQt::ActionDataItem TPClientQt::actionDataItem(int index, const QJsonArray &data, const ActionDataItem &defaultItem)
{
	// QJsonArray::at(i) = "The returned QJsonValue is Undefined, if i is out of bounds."
	if (data.at(index).isObject()) {
		const QJsonObject &o = data.at(index).toObject();
		return { o.value(QLatin1String("id")).toString(), o.value(QLatin1String("value")).toString() };
	}
	return defaultItem;
}

// static
inline
QString TPClientQt::actionDataValue(int index, const QJsonArray &data, const QString &defaultValue)
{
	return TPClientQt::actionDataItem(index, data, {QString(), defaultValue}).value;
}

// static
inline
QString TPClientQt::actionDataValue(const char *id, const QJsonArray &data, const QString &defaultValue)
{
	for (const QJsonValue &v : data) {
		const QJsonObject o = v.toObject();
		if (o.value(QLatin1String("id")).toString() == QString(id))
			return o.value(QLatin1String("value")).toString(defaultValue);
	}
	return defaultValue;
}

#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
#define QtSkipEmptyParts  QString::SkipEmptyParts
#else
#define QtSkipEmptyParts  Qt::SkipEmptyParts
#endif

// static
inline
QVector<TPClientQt::ActionDataItem> TPClientQt::actionDataToItemArray(const QJsonArray &data, char separator)
{
	QVector<ActionDataItem> ret;
	ret.reserve(data.size());
	for (const QJsonValue &v : data) {
		const QJsonObject &vObj = v.toObject();
		if (!vObj.isEmpty() && vObj.contains(QLatin1String("id"))) {
			QString key = vObj.value(QLatin1String("id")).toString();
			if (separator)
				key = key.split(separator, QtSkipEmptyParts).last();
			ret << ActionDataItem({ key, vObj.value(QLatin1String("value")).toString() });
		}
	}
	return ret;
}

// static
inline
QMap<QString, QString> TPClientQt::actionDataToMap(const QJsonArray &data, char separator)
{
	QMap<QString, QString> ret;
	for (const QJsonValue &v : data) {
		const QJsonObject vObj = v.toObject();
		if (!vObj.isEmpty() && vObj.contains(QLatin1String("id"))) {
			QString key = vObj.value(QLatin1String("id")).toString();
			if (separator)
				key = key.split(separator, QtSkipEmptyParts).last();
			ret.insert(key, vObj.value(QLatin1String("value")).toString());
		}
	}
	return ret;
}

#undef QtSkipEmptyParts
#undef qsvPrintable

Q_DECLARE_METATYPE(TPClientQt::MessageType)
Q_DECLARE_METATYPE(TPClientQt::TPInfo)
