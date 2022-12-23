/*
	Originally known as:  AppDebugMessageHandler
	https://github.com/mpaperno/maxLibQt

	COPYRIGHT: (c)2017 Maxim Paperno; All Right Reserved.
	Contact: http://www.WorldDesign.com/contact

	LICENSE:

	Commercial License Usage
	Licensees holding valid commercial licenses may use this file in
	accordance with the terms contained in a written agreement between
	you and the copyright holder.

	GNU General Public License Usage
	Alternatively, this file may be used under the terms of the GNU
	General Public License as published by the Free Software Foundation,
	either version 3 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	A copy of the GNU General Public License is available at <http://www.gnu.org/licenses/>.
*/

#ifndef APPDEBUGMESSAGEHANDLER_H
#define APPDEBUGMESSAGEHANDLER_H

#include <QtCore>
#include <QDebug>
#include <QIODevice>
#include <QObject>
#include <QRegularExpression>
#include <QReadWriteLock>
#include <QTimer>

//! Enable/disable this custom handler handler entirely \relates AppDebugMessageHandler
#ifndef APP_DBG_HANDLER_ENABLE
	#define APP_DBG_HANDLER_ENABLE                1
#endif

//! Default log level. \sa AppDebugMessageHandler::appDebugOutputLevel  \relates AppDebugMessageHandler
#ifndef APP_DBG_HANDLER_DEFAULT_LEVEL
	#define APP_DBG_HANDLER_DEFAULT_LEVEL         0
#endif

#ifndef APP_DBG_HANDLER_ABS_MAX_FILE_SIZE
	#define APP_DBG_HANDLER_ABS_MAX_FILE_SIZE    1024*1024*1024LL  // 1GB max file size
#endif

#ifndef APP_DBG_HANDLER_NO_REPLACE_QML
	#define APP_DBG_HANDLER_REPLACE_QML          "JS"
#endif

#ifdef QT_DEBUG
static Q_LOGGING_CATEGORY(lcLog, "Logger", QtDebugMsg)
#else
static Q_LOGGING_CATEGORY(lcLog, "Logger", QtInfoMsg)
#endif

/*!
	\class AppDebugMessageHandler
	\version 2.1.0-custom

	\brief Custom debug/message handler class to work in conjunction with \c qDebug() family of functions.

	It can be used by other event listeners to intercept messages they may be interested in (connect to \c messageOutput() signal
	or add a \c QIODevice to receive the output).

	You can set a minimum logging level for all messages by setting the \ref appDebugOutputLevel property or \c APP_DBG_HANDLER_DEFAULT_LEVEL macro.

	This is an app-wide "global" thread-safe singleton class, use it with \c AppDebugMessageHandler::instance().
	For example, at start of application:

	\code
		QApplication app(argc, argv);
		AppDebugMessageHandler::instance()->installAppMessageHandler();
	\endcode

	To connect to a signal:

	\code
		connect(AppDebugMessageHandler::instance(), &AppDebugMessageHandler::messageOutput, this, &DebugOutput::onAppDebugMessage);}
	\endcode

	To add a new I/O stream for receiving messages (eg. a log file):

	\code
		addOutputDevice(myQFile);
	\endcode

	When adding a QIODevice in this way, you may optionally set a minimum severity level below which you do not want messages
	written to this device.  Use the Qt property system to set a properly named \a level on the QIODevice. For example:

	\code
		myQFile.setProperty("level", 2);  // only warning and higher
	\endcode

*/
class Logger : public QObject
{
		Q_OBJECT
		//! Minimum severity level of messages to print. 0 = debug+; 1 = info+; 2 = warning+; 3 = critical+; 4 = fatal only. \pacc appDebugOutputLevel(void), setAppDebugOutputLevel()
		Q_PROPERTY(int appDebugOutputLevel READ appDebugOutputLevel WRITE setAppDebugOutputLevel)

	public:

		struct MessageLogContext
		{
			quint8 level;
			int line;
			const QByteArray file;
			const QByteArray function;
			const QByteArray category;
			const QString msg;
		};

		~Logger();

		//! Get the singleton instance of this object. The instance is created automatically if necessary.
		static Logger *instance();
		//! This function must be called to initialize this custom message handler.  E.g. \c AppDebugMessageHandler::instance()->installAppMessageHandler()
		void installAppMessageHandler();

		inline quint8 appDebugOutputLevel() const { return m_appDebugOutputLevel; }  //!< \sa appDebugOutputLevel
		void setAppDebugOutputLevel(quint8 appDebugOutputLevel);  //!< \sa appDebugOutputLevel

		inline bool defaultHandlerDisabled() const { return m_disableDefaultHandler; }
		void setDisableDefaultHandler(bool disable = true) { m_disableDefaultHandler = disable; }

		//! Add a new I/O stream for receiving messages.
		void addOutputDevice(QIODevice *device, quint8 level, const QByteArrayList &category = QByteArrayList());
		//! Remove a previously-added I/O stream.
		void removeOutputDevice(QIODevice *device);

		//! Add a new file stream for receiving messages.
		void addFileDevice(const QString &file, quint8 level, const QByteArrayList &category = QByteArrayList(), bool rotate = true, int keep = 5);
		//! Remove a previously-added file stream.
		void removeFileDevice(const QString &file);

		//! Handle a debug message. This is typically called by the installed global message handler callback ( \c g_appDebugMessageHandler() ).
		void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

		static quint8 levelForCategory(const QLoggingCategory &lc) {
			return lc.isDebugEnabled() ? 0 : lc.isInfoEnabled() ? 1 : lc.isWarningEnabled() ? 2 : lc.isCriticalEnabled() ? 3 : 4;
		}

		static const char * logruleNameForLevel(quint8 lvl) {
			return !lvl ? "debug" : lvl == 1 ? "info" : lvl == 2 ? "warning" : lvl == 3 ? "critical" : lvl == 4 ? "fatal" : nullptr;
		}

	Q_SIGNALS:
		//! Notifies when a new log massage is available.
		void logOutput(const QString &msg, quint8 level, const QByteArray &cat);
		void messageOutput(const Logger::MessageLogContext &context);
		void logRotationRequested();
		void stopping();

	public Q_SLOTS:
		void rotateLogs();

	private Q_SLOTS:
		void onLoggerError(const QString &file, const QString &err);

	private:
		explicit Logger();
		Q_DISABLE_COPY(Logger)

		struct OutputDevice {
			QIODevice *device;
			quint8 logLevel;
			QByteArrayList category;
		};

		bool m_disableDefaultHandler = false;
		bool m_haveFileDevices = false;
		QtMessageHandler m_defaultHandler;
		quint8 m_appDebugOutputLevel;
		QVector<OutputDevice> m_outputDevices;
		QReadWriteLock m_mutex;
		QTimer m_rotateTimer;
};

//! \relates AppDebugMessageHandler
//! Message handler which is installed using qInstallMessageHandler. This needs to be global.
//! Use AppDebugMessageHandler::instance()->installAppMessageHandler();
//!  instead of installing this function directly (it will verify this handler is enabled, etc).
void g_appDebugMessageHandler(QtMsgType, const QMessageLogContext &, const QString &);

Q_DECLARE_METATYPE(Logger::MessageLogContext)

#endif // APPDEBUGMESSAGEHANDLER_H
