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

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QLoggingCategory>
#include <QSettings>
#include <csignal>
#include <cstdlib>
#include <iostream>

#include "common.h"
#include "Logger.h"
#include "Plugin.h"
#include "RunGuard.h"

// configure logging categories externally:
// Set env. var QT_LOGGING_RULES to override, eg:
//   QT_LOGGING_RULES="*.debug=false;DSE.debug=true"
// or set QT_LOGGING_CONF to point to a config file, see QLoggingCategory docs.
// Override message pattern by setting QT_MESSAGE_PATTERN env. var.

#ifdef QT_DEBUG
	#define MESSAGE_LOG_PATTERN  \
		"[%{time process}] " \
		"[%{if-debug}DBG%{endif}%{if-info}INF%{endif}%{if-warning}WRN%{endif}%{if-critical}ERR%{endif}%{if-fatal}CRT%{endif}] " \
		"%{if-category}|%{category}| %{endif}%{function}() @%{line} - %{message}"
#else
	#define MESSAGE_LOG_PATTERN  \
		"[%{time MM-dd HH:mm:ss.zzz}] " \
		"[%{if-debug}DBG%{endif}%{if-info}INF%{endif}%{if-warning}WRN%{endif}%{if-critical}ERR%{endif}%{if-fatal}CRT%{endif}] " \
		"%{if-category}|%{category}| %{endif}- %{message}"
#endif

#define APP_DBG_HANDLER_NO_REPLACE_QML  1

#define OPT_LOGSTDO   QStringLiteral("s")  // enable console/stdout @ level
#define OPT_LOGMAIN   QStringLiteral("f")  // enable primary log file @ level
#define OPT_LOGCNSL   QStringLiteral("j")  // enable JS console.log file @ level
#define OPT_LOGKEEP   QStringLiteral("k")  // keep log days
#define OPT_LOGPATH   QStringLiteral("p")  // log path
#define OPT_LOGSROT   QStringLiteral("r")  // rotate logs now
#define OPT_XITERLY   QStringLiteral("x")  // exit w/out starting
#define OPT_TPHOSTP   QStringLiteral("t")  // TP host:port


void sigHandler(int s)
{
	std::signal(s, SIG_DFL);
	qApp->quit();
}

int main(int argc, char *argv[])
{
	// set up message logging pattern
	qSetMessagePattern(MESSAGE_LOG_PATTERN);
	// enable XMLHttpRequest to read/write local files
	qputenv("QML_XHR_ALLOW_FILE_READ", "1");
	qputenv("QML_XHR_ALLOW_FILE_WRITE", "1");
	// set to enable debug output
	//qputenv("QML_XHR_DUMP", "1");

	QCoreApplication a(argc, argv);
	QCoreApplication::setOrganizationName(PLUGIN_NAME);
	QCoreApplication::setApplicationName(PLUGIN_SYSTEM_NAME);
	QCoreApplication::setOrganizationDomain(PLUGIN_DOMAIN);
	QCoreApplication::setApplicationVersion(PLUGIN_VERSION_STR);
	QSettings::setDefaultFormat(QSettings::IniFormat);

	Logger::instance()->installAppMessageHandler();

	QString logPath = "../logs",
	    tpHost;
	uint16_t tpPort = 0;
	// Set default logging levels for file and stderr.
	qint8 keep = 3,
	    fileLevel = 1,
	    jsFileLevel = 0;
#ifdef QT_DEBUG
	qint8 stdoutLevel = 0;  // enable @ debug level
#else
	qint8 stdoutLevel = 5;  // disable for release builds
#endif

	QCommandLineParser clp;
	clp.setApplicationDescription("\n" PLUGIN_NAME "\n\n" "Logging levels for options, most to least verbose: 0 = Debug; 1 = Info; 2 = Warning; 3 = Error; 4 = Fatal; 5 = disable logging.");
	clp.setSingleDashWordOptionMode(QCommandLineParser::ParseAsCompactedShortOptions);
	clp.addOptions({
		{ {OPT_LOGMAIN, QStringLiteral("file")},    qApp->translate("main", "Enable logging to primary plugin log file at given verbosity level (this includes messages from all sources)."), QStringLiteral("level") },
		{ {OPT_LOGCNSL, QStringLiteral("jsfile")},  qApp->translate("main", "Enable script-related logging to console.log file at given verbosity level (from 'console.*' commands and script errors)."), QStringLiteral("level") },
		{ {OPT_LOGSTDO, QStringLiteral("stdout")},  qApp->translate("main", "Enable logging output to the system console/stdout at given verbosity level."), QStringLiteral("level") },
		{ {OPT_LOGPATH, QStringLiteral("path")},    qApp->translate("main", "Path for log files. Default is '%1'").arg(logPath), QStringLiteral("path") },
		{ {OPT_LOGKEEP, QStringLiteral("keep")},    qApp->translate("main", "Keep this number of previous logs (logs are rotated daily, default is to keep %1 days plus the current day).").arg(keep), QStringLiteral("days") },
		{ {OPT_LOGSROT, QStringLiteral("rotate")},  qApp->translate("main", "Rotate log file(s) on startup (starts with empty logs). Only enabled log(s) (with -f or -j) are rotated.") },
		{ {OPT_XITERLY, QStringLiteral("exit")},    qApp->translate("main", "Exit w/out starting. For example after rotating logs.") },
		{ {OPT_TPHOSTP, QStringLiteral("tphost")},  qApp->translate("main", "Touch Portal host address and optional port number in the format of 'host_name_or_address[:port_number]'. Default is '127.0.0.1:12136'."), QStringLiteral("host[:port]") },
	});
	clp.addHelpOption();
	clp.addVersionOption();
	clp.process(a);

	// Prevent multiple instances.
	RunGuard guard( PLUGIN_NAME );
	if (!guard.tryToRun()) {
		std::cout << "Another instance is already running. Quitting now." << std::endl;
		return 0;
	}


	if (clp.isSet(OPT_LOGPATH))
		logPath = clp.value(OPT_LOGPATH);

	bool ok;
	if (clp.isSet(OPT_LOGKEEP)) {
		quint8 k = clp.value(OPT_LOGKEEP).toUInt(&ok);
		if (ok)
			keep = k;
		else
			clp.showHelp(1);
	}

	if (clp.isSet(OPT_LOGMAIN)) {
		quint8 k = clp.value(OPT_LOGMAIN).toUInt(&ok);
		if (ok)
			fileLevel = k;
		else
			clp.showHelp(1);
	}

	if (clp.isSet(OPT_LOGCNSL)) {
		quint8 k = clp.value(OPT_LOGCNSL).toUInt(&ok);
		if (ok)
			jsFileLevel = k;
		else
			clp.showHelp(1);
	}

	if (clp.isSet(OPT_LOGSTDO)) {
		quint8 k = clp.value(OPT_LOGSTDO).toUInt(&ok);
		if (ok)
			stdoutLevel = k;
		else
			clp.showHelp(1);
	}

	if (clp.isSet(OPT_TPHOSTP)) {
		const auto optlist = clp.value(OPT_TPHOSTP).split(':');
		tpHost = optlist.first();
		if (optlist.length() > 1) {
			quint8 k = optlist.at(1).toUInt(&ok);
			if (ok)
				tpPort = k;
			else
				clp.showHelp(1);
		}
	}

	QString logFilterRules = "qt.qml.compiler.warning = false\n";

	quint8 effectiveLevel = fileLevel > -1 ? std::min(stdoutLevel, fileLevel) : stdoutLevel;
	//std::cerr << (int)effectiveLevel << ' ' << (int)Logger::levelForCategory(lcPlugin()) << std::endl;
	if (effectiveLevel < 5 && effectiveLevel > Logger::levelForCategory(lcPlugin())) {
		while (effectiveLevel > 0 ) {
			const QLatin1String lvlName = QLatin1String(Logger::logruleNameForLevel(effectiveLevel-1));
			logFilterRules += QLatin1String(lcPlugin().categoryName()) + '.' + lvlName + " = false\n";
			logFilterRules += QLatin1String(lcLog().categoryName()) + '.' + lvlName + " = false\n";
			logFilterRules += QLatin1String("TPClientQt.") + lvlName + " = false\n";
			--effectiveLevel;
		}
	}
	else {
		while (effectiveLevel < Logger::levelForCategory(lcPlugin())) {
			const QLatin1String lvlName = QLatin1String(Logger::logruleNameForLevel(effectiveLevel));
			logFilterRules += QLatin1String(lcPlugin().categoryName()) + '.' + lvlName + " = true\n";
			logFilterRules += QLatin1String(lcLog().categoryName()) + '.' + lvlName + " = true\n";
			logFilterRules += QLatin1String("TPClientQt.") + lvlName + " = true\n";
			++effectiveLevel;
		}
	}

	effectiveLevel = jsFileLevel > -1 ? std::min(stdoutLevel, jsFileLevel) : stdoutLevel;
	if (effectiveLevel < 5 && effectiveLevel > Logger::levelForCategory(lcDse())) {
		while (effectiveLevel > 0 ) {
			const QLatin1String lvlName = QLatin1String(Logger::logruleNameForLevel(effectiveLevel-1));
			logFilterRules += QLatin1String(lcDse().categoryName()) + '.' + lvlName + " = false\n";
			logFilterRules += QLatin1String("qml.") + lvlName + " = false\n";
			logFilterRules += QLatin1String("js.") + lvlName + " = false\n";
			--effectiveLevel;
		}
	}

	if (!logFilterRules.isEmpty())
		QLoggingCategory::setFilterRules(logFilterRules);
	//std::cout << logFilterRules.toStdString() << std::endl;

	Logger::instance()->setAppDebugOutputLevel(stdoutLevel);
	if (fileLevel > -1 && fileLevel < 5)
		Logger::instance()->addFileDevice(logPath + "/plugin.log", fileLevel, {}, true, keep);
	if (jsFileLevel > -1 && jsFileLevel < 5)
		Logger::instance()->addFileDevice(logPath + "/console.log", jsFileLevel, {"DSE", "js"}, true, keep);

	if (clp.isSet(OPT_LOGSROT))
		Logger::instance()->rotateLogs();

	if (clp.isSet(OPT_XITERLY)) {
		QTimer::singleShot(1000, qApp, [&]() { qApp->quit(); });
		return a.exec();
	}

	std::signal(SIGTERM, sigHandler);
  std::signal(SIGABRT, sigHandler);
  std::signal(SIGINT, sigHandler);
  std::signal(SIGBREAK, sigHandler);

	Plugin p(tpHost, tpPort);
	return a.exec();
}
