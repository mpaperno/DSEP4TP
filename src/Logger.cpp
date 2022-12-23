/*
	AppDebugMessageHandler

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

#include "Logger.h"

#include <QDir>
#include <QFileDevice>
#include <QThread>
#include <QMetaObject>
#include <QRegularExpression>
#include <qlogging.h>

#include <cstdlib>
#include <iostream>
#include <filesystem>

static QString normalizePath(const QString& messyPath, bool withFile = true) {
	std::filesystem::path path(messyPath.toStdString());
  std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(path);
	const QFileInfo fi(QString::fromStdString(canonicalPath.string()));
	return withFile ? fi.absoluteFilePath(): fi.absolutePath();
}

static QDateTime getFileCreationTime(const QFileInfo &fi)
{
	QDateTime fdate = fi.birthTime();
	if (fdate.isValid())
		return fdate;
	fdate = fi.metadataChangeTime();
	if (fdate.isValid())
		return fdate;
	return fi.lastRead();
}

static bool isFileFromPreviousDay(const QFileInfo &fi, QDateTime *dt = nullptr)
{
	if (!fi.exists())
		return false;
	QDateTime fdate = getFileCreationTime(fi);
	if (dt)
		*dt = fdate;
	if (fdate.date() != QDate::currentDate())
		return true;
	if (fdate.time().msecsSinceStartOfDay() <= 10*1000)
		return true;
	return false;
}

static QString timestampLogFile(const QString &file, int seq = 0)
{
	const QFileInfo fi(file);
	QDateTime fdate; // = getFileCreationTime(fi);
	if (isFileFromPreviousDay(fi, &fdate))
		fdate = fdate.addDays(-1);
	QString stampStr = fdate.toString("-yyyyMMdd");
	if (seq)
		stampStr += '-' + QString::number(seq);
	return fi.absolutePath() + '/' + fi.completeBaseName() + stampStr + '.' + fi.suffix();
}


static QByteArray &formatLogSting(QByteArray &p, const QByteArrayList &args)
{
	char argIdx;
	int patIdx = 0;
	while ((patIdx = p.indexOf('%', patIdx)) > -1) {
		argIdx = p.at(patIdx + 1);
		if (argIdx < 49 || argIdx > 57)
			continue;
		argIdx -= 48 + 1;
		if (args.size() > argIdx) {
			const QByteArray &arg = args.at(argIdx);
			p.replace(patIdx, 2, arg);
			patIdx += arg.length() + 1;
		}
		else {
			break;
		}
		//qDebug() << (int)argIdx << patIdx << args.value(argIdx-1);
		if (patIdx > p.length() - 2)
			break;
	}
	return p;
}

#ifdef QT_DEBUG
static const QByteArray defaultCategoryPattern QByteArrayLiteral("[%1] [%2] |%3| %5() @%6 - %7\n");
static const QString logDateTimeFormat = QStringLiteral("H:mm:ss.zzz");
#else
static const QByteArray defaultCategoryPattern {"[%1] [%2] |%3| %7\n"};
static const QString logDateTimeFormat = QStringLiteral("MM-dd HH:mm:ss.zzz");
#endif

using CategoryPatternsHash = QHash<QByteArray, QByteArray>;
Q_GLOBAL_STATIC_WITH_ARGS(const CategoryPatternsHash, categoryPatterns, ({
	{ "js",  "[%1] [%2] |%3| %4 @%6 %5() - %7\n" },
	{ "DSE", "[%1] [%2] |%3| %7\n" },
}));

using LevelNamesHash = QHash<quint8, QByteArray>;
Q_GLOBAL_STATIC_WITH_ARGS(const LevelNamesHash,  logLevelTypeNames, ({
	{0, "DBG"},
	{1, "INF"},
	{2, "WRN"},
	{3, "ERR"},
	{4, "CRT"},
}));

class LogFileDevice : public QFile
{
		Q_OBJECT
		quint8 m_logLevel;
		QByteArrayList m_category;
		bool m_rotate = true;
		int m_keep = 7;
		QThread *m_t = nullptr;
	public:

		explicit LogFileDevice(const QString &file, quint8 level, const QByteArrayList &category = QByteArrayList(), bool rotate = true, int keep = 7) :
		  QFile(normalizePath(file)),
		  m_logLevel(level),
		  m_category(category),
		  m_rotate(rotate),
		  m_keep(keep),
		  m_t(new QThread(this))
		{
			moveToThread(m_t);
		}

		//~LogFileDevice() { qDebug() << this << "Destroyed"; }

		bool isSameFile(const QString &otherFile) const { return normalizePath(otherFile) == fileName(); }

	public Q_SLOTS:

		void logMessage(const Logger::MessageLogContext &context)
		{
			static const QRegularExpression cleanFuncRx(R"(^(?:\w+ )+([\w:]+).*$)");
			if (!isOpen() || m_logLevel > context.level || (!m_category.isEmpty() && !m_category.contains(context.category)))
				return;
			QByteArray pattern = categoryPatterns->value(context.category, defaultCategoryPattern);
			formatLogSting(pattern, {
				QDateTime::currentDateTime().toString(logDateTimeFormat).toUtf8(),
				logLevelTypeNames->value(context.level),
				context.category,
				context.file.split('/').last().split('\\').last(),
				//QFileInfo(context.file).fileName().toUtf8(),
				QString(context.function).replace(cleanFuncRx, "\\1").toUtf8(),
				QByteArray::number(context.line),
				context.msg.toUtf8()
			});
			write(qPrintable(pattern));
			//write("\n");
			flush();
			if (pos() >= APP_DBG_HANDLER_ABS_MAX_FILE_SIZE) {
				Q_EMIT loggerError(fileName(), QStringLiteral("Maximum Log file exceeded; logging has been terminated."));
				stop();
			}
		}

		void log(const QString &msg, quint8 level, const QByteArray &cat)
		{
			if (isOpen() && m_logLevel <= level && (m_category.isEmpty() || m_category.contains(cat))) {
				write(qPrintable(msg));
				write("\n");
				flush();
				if (pos() >= APP_DBG_HANDLER_ABS_MAX_FILE_SIZE) {
					Q_EMIT loggerError(fileName(), QStringLiteral("Maximum Log file exceeded; logging has been terminated."));
					stop();
				}
			}
		}

		bool start()
		{
			if (m_t->isRunning())
				return false;
			QFileInfo fi(fileName());
			QDir dir = fi.absoluteDir();
			if (!dir.exists() && !dir.mkpath(dir.absolutePath())) {
				Q_EMIT loggerError(fileName(), QStringLiteral("Could not create directory path %1").arg(dir.absolutePath()));
				return false;
			}
			if (isFileFromPreviousDay(fi)) {
				rotate();
				if (!isOpen())
					return false;
			}
			else if (!openFile())
				return false;
			//moveToThread(m_t);
			m_t->start();
			qCInfo(lcLog) << "Created logger with file" << dir.absoluteFilePath(fileName()) << "at level" << m_logLevel
			              << (m_category.isEmpty() ? QStringLiteral("with no category filter.") : QStringLiteral("for category(ies): ") + m_category.join(", "));
			Q_EMIT started();
			return true;
		}

		bool openFile()
		{
			if (!open(QFile::WriteOnly | QFile::Text | QFile::Append)) {
				Q_EMIT loggerError(fileName(), QStringLiteral("Could not open file '%1' for writing.").arg(fileName()));
				return false;
			}
			if (!size()) {
				// Timestamp new files to properly track old versions om isFileFromPreviousDay()/
				if (!setFileTime(QDateTime::currentDateTime(), FileBirthTime))
					if (!setFileTime(QDateTime::currentDateTime(), FileMetadataChangeTime))
						setFileTime(QDateTime::currentDateTime(), FileAccessTime);
			}
			write("=+=+=+=+=+=+=+=+= " + QDateTime::currentDateTime().toString("MM-dd HH:mm:ss.zzz").toUtf8() + " Log Started =+=+=+=+=+=+=+=+=\n");
			flush();
			return true;
		}

		void closeFile()
		{
			if (!isOpen())
				return;
			write("-=-=-=-=-=-=-=-=- " + QDateTime::currentDateTime().toString("MM-dd HH:mm:ss.zzz").toUtf8() + " Log Stopped -=-=-=-=-=-=-=-=-\n");
			flush();
			close();
		}

		void stop()
		{
			//std::cout << this << " Stopping" << std::endl;
			closeFile();
			m_t->quit();
			//m_t->wait();
			Q_EMIT stopped();
		}

		void rotate()
		{
			if (!m_rotate && !size())
				return;
			blockSignals(true);
			const QString &origName = fileName();
			closeFile();
			int seq = 0;
			while (!rename(timestampLogFile(origName, seq)) && ++seq < 100);
			if (seq == 100) {
				Q_EMIT loggerError(fileName(), QStringLiteral("Failed to rotate files, too many already!"));
				stop();
				return;
			}
			setFileName(origName);
			if (!openFile()) {
				stop();
				return;
			}
			if (m_keep >= 0) {
				QFileInfo fi(origName);
				QDir dir(fi.absolutePath(), fi.completeBaseName() + "-*." + fi.suffix(), QDir::Time, QDir::Files);
				const auto &list = dir.entryInfoList();
				int i = 0;
				for (const QFileInfo &dfi : list) {
					if (i++ < m_keep)
						continue;
					if (dir.remove(dfi.fileName()))
						qCInfo(lcLog) << "Removed old log file" << dfi.fileName();
					else
						qCInfo(lcLog) << "Removal failed for log file" << dfi.fileName();
				}
			}
			qCInfo(lcLog) << "Log rotation complete for" << fileName();
		}

	Q_SIGNALS:
		void stopped();
		void started();
		void loggerError(const QString &file, const QString &error);
		void maxSizeReached(const QString &file);
};
// LogFileDevice


Logger::Logger() :
  QObject(),
  m_defaultHandler(nullptr)
{
	qRegisterMetaType<Logger::MessageLogContext>("MessageLogContext");
	setAppDebugOutputLevel(APP_DBG_HANDLER_DEFAULT_LEVEL);
	m_rotateTimer.setTimerType(Qt::VeryCoarseTimer);
	connect(&m_rotateTimer, &QTimer::timeout, this, &Logger::rotateLogs);
}

// static
Logger::~Logger()
{
	QReadLocker locker(&m_mutex);
	m_rotateTimer.stop();
	for (const auto &d : qAsConst(m_outputDevices)) {
		if (LogFileDevice *fd = qobject_cast<LogFileDevice*>(d.device)) {
			fd->stop();
			fd->deleteLater();
		}
	}
}

Logger *Logger::instance()
{
	static Logger instance;
	return &instance;
}

void Logger::setAppDebugOutputLevel(quint8 appDebugOutputLevel)
{
	m_appDebugOutputLevel = qMin<quint8>(appDebugOutputLevel, 4);
}

void Logger::addOutputDevice(QIODevice *device, quint8 level, const QByteArrayList &category)
{
	if (!device)
		return;
	QWriteLocker locker(&m_mutex);
	if (m_haveFileDevices)
		for (const auto &d : qAsConst(m_outputDevices))
			if (d.device == device)
				return;
	m_outputDevices.append({device, level, category});
	m_haveFileDevices = true;
}

void Logger::removeOutputDevice(QIODevice *device)
{
	if (!device)
		return;
	QWriteLocker locker(&m_mutex);
	int i = 0;
	for (const auto &d : qAsConst(m_outputDevices)) {
		if (d.device == device) {
			m_outputDevices.remove(i);
			if (d.device->parent() == this) {
				d.device->close();
				d.device->deleteLater();
			}
			break;
		}
		++i;
	}
	m_haveFileDevices = !m_outputDevices.isEmpty();
}

void Logger::addFileDevice(const QString &file, quint8 level, const QByteArrayList &category, bool rotate, int keep)
{
	QWriteLocker locker(&m_mutex);
	for (const auto &d : qAsConst(m_outputDevices)) {
		if (LogFileDevice *fd = qobject_cast<LogFileDevice*>(d.device)) {
			if (fd->isSameFile(file))
				return;
		}
	}
	LogFileDevice *fd = new LogFileDevice(file, level, category, rotate, keep);
	if (!fd->start()) {
		fd->deleteLater();
		qCCritical(lcLog) << "Cannot open file" << file;
		return;
	}
//	connect(this, &AppDebugMessageHandler::logOutput, fd, &LogFileDevice::log, Qt::QueuedConnection);
	connect(this, &Logger::messageOutput, fd, &LogFileDevice::logMessage, Qt::QueuedConnection);
	connect(this, &Logger::logRotationRequested, fd, &LogFileDevice::rotate, Qt::QueuedConnection);
	connect(fd, &LogFileDevice::loggerError, this, &Logger::onLoggerError, Qt::QueuedConnection);
	m_outputDevices.append({fd, level, category});
	m_haveFileDevices = true;

	if (!m_rotateTimer.isActive()) {
		int ms = QDateTime::currentDateTime().msecsTo(QDateTime(QDate::currentDate().addDays(1), QTime(0, 0, 10)));
		m_rotateTimer.setInterval(ms);
		m_rotateTimer.start();
		qCDebug(lcLog) << "Log rotation timer scheduled for" << QDateTime::currentDateTime().addMSecs(ms).toString("dd HH:mm:ss") << "(" << ms << "ms)";
	}
}

void Logger::removeFileDevice(const QString &file)
{
	QWriteLocker locker(&m_mutex);
	int i = 0;
	for (const auto &d : qAsConst(m_outputDevices)) {
		if (LogFileDevice *fd = qobject_cast<LogFileDevice*>(d.device)) {
			if (fd->isSameFile(file)) {
				m_outputDevices.remove(i);
				//QMetaObject::invokeMethod(fd, "stop", Qt::QueuedConnection);
				fd->stop();
				fd->deleteLater();
				break;
			}
		}
		++i;
	}
	m_haveFileDevices = !m_outputDevices.isEmpty();
	if (!m_haveFileDevices)
		m_rotateTimer.stop();
}

void Logger::installAppMessageHandler()
{
#if APP_DBG_HANDLER_ENABLE
	m_defaultHandler = qInstallMessageHandler(g_appDebugMessageHandler);
#else
	qInstallMessageHandler(nullptr);
#endif
}

void Logger::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	// normalize types, QtDebugMsg stays 0, QtInfoMsg becomes 1, rest are QtMsgType + 1
	quint8 lvl = type;
	if (type == QtInfoMsg)
		lvl = 1;
	else if (type > QtDebugMsg)
		++lvl;

#ifdef APP_DBG_HANDLER_REPLACE_QML
	if (!qstrncmp(context.category, "qml", 1)) {
//		const MessageLogContext newContext({lvl, context.line, context.file, context.function, APP_DBG_HANDLER_REPLACE_QML, msg});
		Q_EMIT messageOutput({lvl, context.line, context.file, context.function, APP_DBG_HANDLER_REPLACE_QML, msg});
//		if (m_haveFileDevices)
//			Q_EMIT logOutput(qFormatLogMessage(type, newContext, msg), lvl, newContext.category);
		if (m_defaultHandler && lvl >= m_appDebugOutputLevel /*&& !m_disableDefaultHandler*/)
			m_defaultHandler(type, context, msg);
		return;
	}
#endif
//	const MessageLogContext newContext({lvl, context.line, context.file, context.function, context.category, msg});
	Q_EMIT messageOutput({lvl, context.line, context.file, context.function, context.category, msg});
	if (m_defaultHandler && lvl >= m_appDebugOutputLevel)
		m_defaultHandler(type, context, msg);
//	if (m_haveFileDevices)
//		Q_EMIT logOutput(qFormatLogMessage(type, context, msg), lvl, context.category);
}

void Logger::rotateLogs()
{
	qCDebug(lcLog) << "Rotating log files";
	QWriteLocker locker(&m_mutex);
	m_rotateTimer.stop();
	Q_EMIT logRotationRequested();
	m_rotateTimer.start(QDateTime::currentDateTime().msecsTo(QDateTime(QDate::currentDate().addDays(1), QTime(0, 0, 10))));
}

void Logger::onLoggerError(const QString &file, const QString &err)
{
	removeFileDevice(file);
	qCWarning(lcLog) << "Fatal file logger error for" << file << ':' << err;
}

// Message handler which is installed using qInstallMessageHandler. This needs to be global.
void g_appDebugMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	if (Logger::instance())
		Logger::instance()->messageHandler(type, context, msg);
}

#include "Logger.moc"
