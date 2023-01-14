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

#include <QAbstractEventDispatcher>
#include <QCryptographicHash>
#include <QCoreApplication>
#include <QDir>
#include <QHash>
#include <QJSValue>
#include <QMetaEnum>
#include <QObject>
#include <QPointer>
#include <QProcessEnvironment>
#include <QReadWriteLock>
#include <QUrl>
#include <QTimer>
#include <QThread>

#include "common.h"
#include "utils.h"
#include "ScriptEngine.h"

#ifndef DOXYGEN
namespace ScriptLib {
#endif

struct TimerData
{
	enum TimerType : quint8 { NoneType, SingleShot, Repeating };
	using timPtr_t = QSharedPointer<QTimer>;

	int id;
	TimerType type = TimerType::NoneType;
	QJSValue expression;
	QJSValue thisObject;
	QJSValueList args;
	int interval;
	timPtr_t tim = nullptr;
	QByteArray instanceName;

	QString toString() const { return toString(type, id); }
	QString toString(int id) const { return toString(type, id); }
	QLatin1String typeName() const { return typeName(type); }
	static inline QString toString(quint8 type, int id) {
		return typeName(type) + " Timer ID " + QString::number(id);
	}
	static inline QLatin1String typeName(quint8 type) {
		switch(type) {
			case TimerType::SingleShot: return QLatin1String("SingleShot");
			case TimerType::Repeating: return QLatin1String("Repeating");
			default: return QLatin1String("Invalid");
		};
	}
};

// \ingroup Util
// Helper class for JavaScript environment. Contains miscellaneous functions not fitting into any other category.
// This includes some missing JS document/window functions like `setTimeout()`/`setInterval()`, environment variable access,
// binary <-> text conversion, URL manipulation, Touch Portal interaction, and an `include()` function for reusing chunks of code.
// All methods are "static" and can be access in JavaScript with `Util.` qualifier (also aliased as 'Utils' because I can never decide between the two anyway).
class Util : public QObject
{
		Q_OBJECT

	protected:
		friend class ::ScriptEngine;

	private:

		ScriptEngine *se = nullptr;
		QReadWriteLock m_timersMutex;
		std::atomic_int m_nextTimerId = 0;
		QHash<int, TimerData> m_timers;

		// Timers implementation

		int startScriptTimer(TimerData::TimerType type, QJSValue expression, int delay, const QJSValueList &args = QJSValueList())
		{
			if (!se || !(expression.isString() || expression.isCallable() || (expression.isArray() && expression.property("length").toInt() > 0)))
				return -1;
			int timerId = ++m_nextTimerId;
			QWriteLocker l(&m_timersMutex);
			QTimer *tim = nullptr;
			if (delay > 0) {
				tim = new QTimer();
				tim->setSingleShot(true);
				tim->setTimerType(Qt::PreciseTimer);
				tim->setInterval(delay);
				connect(tim, &QTimer::timeout, this, [=]() { execTimer(timerId); }, Qt::DirectConnection);
				connect(this, &Util::killTimers, tim, &QTimer::stop, Qt::QueuedConnection);
			}
			QJSValue thisObject;
			if (expression.isArray()) {
				if (expression.property("length").toInt() > 1)
					thisObject = expression.property(1);
				expression = expression.property(0);
			}
			const TimerData td = m_timers.insert(timerId,
			{
				timerId,
				type,
				expression,
				thisObject,
				args,
				delay,
				(tim ? TimerData::timPtr_t(tim, &QObject::deleteLater) : nullptr),
				se->currentInstanceName()
			}).value();
			//qCDebug(lcPlugin) << this << "TimerEvent: Created timer" << timerId << type << "itvl:" << delay << "for instance" << td.instanceName << "on thread" << QThread::currentThread() << "(app" << qApp->thread() << ')';
			timerStart(td);
			return timerId;
		}

		void timerStart(const TimerData &td)
		{
			if (td.interval > 0) {
				if (td.tim)
					td.tim->start();
				return;
			}
			QMetaObject::invokeMethod(this, "execTimer", Qt::QueuedConnection, Q_ARG(int, td.id));
		}

		void clearScriptTimer(int timerId)
		{
			QWriteLocker l(&m_timersMutex);
			m_timers.remove(timerId);
			//if (m_timers.remove(timerId))
			//	qCDebug(lcPlugin) << this << "TimerEvent: Killed timer" << timerId << "thread" << QThread::currentThread() << "app" << qApp->thread();
		}

	private Q_SLOTS:
		void execTimer(int id)
		{
			QReadLocker l(&m_timersMutex);
			const TimerData td = m_timers.value(id);
			l.unlock();
			if (td.type == TimerData::NoneType) {
				qCDebug(lcPlugin) << this << "TimerEvent: Expired ID" << id << "thread" << QThread::currentThread() << "app" << qApp->thread();
				return;
			}

			const bool ok = se->timerExpression(&td);
			if (ok && td.type == TimerData::Repeating)
				timerStart(td);
			else
				clearScriptTimer(id);
		}

		// /end Timers

	public:
		explicit Util(ScriptEngine *se, QObject *p = nullptr) :
		  QObject(p), se(se)
		{
			setObjectName("DSE.Util");
		}

		~Util()
		{
			clearAllTimers();
			//qCDebug(lcPlugin) << this << "Destroyed";
		}

	public Q_SLOTS:

		// \name Timers
		// \{<img alt="Conda" src="https://img.shields.io/conda/pn/platform/win--x64">

		// Set `expression` to be evaluated every `delay` milliseconds until canceled. `expression` maybe be either a functor to be called or a string to be evaluated.
		// Returns an ID which can be used to cancel the timer.  \sa clearInterval(), setTimeout()
		int setInterval(QJSValue expression, int delay) { return startScriptTimer(TimerData::Repeating, expression, delay); }
		int setInterval(QJSValue expression, int delay, const QJSValueList &args) { return startScriptTimer(TimerData::Repeating, expression, delay, args); }
		int setInterval(QJSValue expression, int delay, QJSValue p1) { return setInterval(expression, delay, (p1.isArray() ? Utils::jsArrayToValueList(p1) : QJSValueList() << p1)); }
		int setInterval(QJSValue expression, int delay, QJSValue p1, QJSValue p2) { return setInterval(expression, delay, QJSValueList() << p1 << p2); }
		int setInterval(QJSValue expression, int delay, QJSValue p1, QJSValue p2, QJSValue p3) { return setInterval(expression, delay, QJSValueList() << p1 << p2 << p3); }
		int setInterval(QJSValue expression, int delay, QJSValue p1, QJSValue p2, QJSValue p3, QJSValue p4) { return setInterval(expression, delay, QJSValueList() << p1 << p2 << p3 << p4); }
		int setInterval(QJSValue expression, int delay, QJSValue p1, QJSValue p2, QJSValue p3, QJSValue p4, QJSValue p5) { return setInterval(expression, delay, QJSValueList() << p1 << p2 << p3 << p4 << p5); }
		// Cancels recurring event started with `setInterval()`.  The `timerId` is the value returned from `setInterval()` call. This function has no effect if the timer has already been stopped.
		void clearInterval(int timerId) { clearScriptTimer(timerId); }

		// Set `expression` to be evaluated once after `delay` milliseconds. `expression` maybe be either a functor to be called or a string to be evaluated.
		// Returns an ID which can be used to cancel the timer.  \sa clearTimeout(), setInterval()
		int setTimeout(QJSValue expression, int delay) { return startScriptTimer(TimerData::SingleShot, expression, delay); }
		int setTimeout(QJSValue expression, int delay, const QJSValueList &args) { return startScriptTimer(TimerData::SingleShot, expression, delay, args); }
		int setTimeout(QJSValue expression, int delay, QJSValue p1) { return setTimeout(expression, delay, (p1.isArray() ? Utils::jsArrayToValueList(p1) : QJSValueList() << p1)); }
		int setTimeout(QJSValue expression, int delay, QJSValue p1, QJSValue p2) { return setTimeout(expression, delay, QJSValueList() << p1 << p2); }
		int setTimeout(QJSValue expression, int delay, QJSValue p1, QJSValue p2, QJSValue p3) { return setTimeout(expression, delay, QJSValueList() << p1 << p2 << p3); }
		int setTimeout(QJSValue expression, int delay, QJSValue p1, QJSValue p2, QJSValue p3, QJSValue p4) { return setTimeout(expression, delay, QJSValueList() << p1 << p2 << p3 << p4); }
		int setTimeout(QJSValue expression, int delay, QJSValue p1, QJSValue p2, QJSValue p3, QJSValue p4, QJSValue p5) { return setTimeout(expression, delay, QJSValueList() << p1 << p2 << p3 << p4 << p5); }
		// Cancels a scheduled event started with `setTimeout()`.  The `timerId` is the value returned from `setTimeout()` call. This function has no effect if the timer has already timed out.
		void clearTimeout(int timerId) { clearScriptTimer(timerId); }


		// Immediately terminates any and all timers started with `setTimeout()` or `setInterval()` within the current engine instance.
		// If invoked in a Shared engine, will stop all timers started by any scripts/expressions running in the Shared instance.
		// If invoked in a Private engine, affects only the timers for that particular named instance.
		void clearAllTimers()
		{
			if (m_timers.empty()) {
				//qCDebug(lcPlugin) << this << "No timers were active.";
				return;
			}
			Q_EMIT killTimers();
			// give timers a chance to stop... ?
			//if (QAbstractEventDispatcher *ed = thread()->eventDispatcher())
			//	ed->processEvents(QEventLoop::AllEvents);
			QWriteLocker l(&m_timersMutex);
			m_timers.clear();
			qCDebug(lcPlugin) << this << "Cleared all timers";
		}

		// Cancel timer(s) for a given instance name.
		void clearInstanceTimers(const QByteArray &name)
		{
			int count = 0;
			QWriteLocker l(&m_timersMutex);
			QMutableHashIterator it(m_timers);
			while (it.hasNext()) {
				if (it.next().value().instanceName == name) {
					it.remove();
					++count;
				}
			}
			qCDebug(lcPlugin) << this << "Cleared" << count << "timer(s) for instance" << name;
		}

		// \}

	Q_SIGNALS:
		// internal
		void killTimers();

	public:
		// \name Environment variables;  Env.js uses these methods to provide the Env JS object in the engine.
		// \{

		// Returns an object with all current environment variable names as keys and their their associated values.
		// Modifying the resulting object does _not_ change the actual environment variable. \sa env(string &), env(string &, string &), envPut()
		Q_INVOKABLE QJSValue env() const
		{
			if (!se)
				return QJSValue();
			const QProcessEnvironment &pe = QProcessEnvironment::systemEnvironment();
			const QStringList &keys = pe.keys();
			QVariantMap m;
			for (const QString &key : keys)
				m.insert(key, pe.value(key));
			return se->engine()->toScriptValue(m);
		}

		// Returns the value of environment variable `name` as a byte array, or an empty value if it doesn't exist.
 		Q_INVOKABLE static QByteArray env(const QByteArray &name)                              { return qgetenv(name); }
		// Returns the value of environment variable `name` as a string, or `defaultValue` if it doesn't exist.
		Q_INVOKABLE static QString    env(const QByteArray &name, const QString &defaultValue) { return qEnvironmentVariable(name, defaultValue).toUtf8(); }
		// Sets environment variable `name` to `value`.
		Q_INVOKABLE static bool       envPut(const QByteArray &name, const QByteArray &value)  { return qputenv(name, value); }
		// Removes environment variable `name`.
		Q_INVOKABLE static bool       envUnset(const QByteArray &name)                         { return qunsetenv(name); }
		// Returns `true` if environment variable `name` exists, `false` otherwise.
		Q_INVOKABLE static bool       envIsSet(const QByteArray &name)                         { return qEnvironmentVariableIsSet(name); }

		// \}

		// \name Miscellaneous
		// \{

		// Passthrough for ScriptEngine to avoid needing to register it in the global JS object.
		Q_INVOKABLE void include(const QString &file) const { se->include(file); }

		Q_INVOKABLE QString hash(const QByteArray &data, QString algorithm = QStringLiteral("md5"))
		{
			if (data.isEmpty())
				return QString();
			if (algorithm.isEmpty() || algorithm == QStringLiteral("md5"))
				return QString(QCryptographicHash::hash(data, QCryptographicHash::Md5));
			bool ok;
			algorithm.replace(0, 1, algorithm.at(0).toUpper());
			QCryptographicHash::Algorithm a = (QCryptographicHash::Algorithm)QMetaEnum::fromType<QCryptographicHash::Algorithm>().keyToValue(qPrintable(algorithm), &ok);
			if (!ok) {
				if (se)
					se->throwError(QJSValue::TypeError, "in hash() - The specified algorithm '" + algorithm + "' was not valid.");
				return QString();
			}
			return QString(QCryptographicHash::hash(data, a));
		}

		Q_INVOKABLE static QString baToHex(const QByteArray &ba, const QChar &sep = '\0') { return ba.toHex(sep.toLatin1()); }


		Q_INVOKABLE QString currentThread() {
			QString str;
			QDebug(&str) << "Current thread:" << QThread::currentThread() << "; Main thread:" << qApp->thread();
			return str;
		}

		// \}

		//! \name Text Processing
		//! \{

		// Right trim a string
		Q_INVOKABLE static QString stringTrimRight(QString str)
		{
			for (int n = str.size() - 1; n > -1; --n) {
				const QChar &ch = str.at(n);
				if (!ch.isSpace() && ch != QChar(0xFFEF)) {
					str.truncate(n+1);
					break;
				}
			}
			return str;
		}

		// Left trim a string
		Q_INVOKABLE static QString stringTrimLeft(QString str)
		{
			for (int n = 0, e = str.size(); n < e; ++n) {
				const QChar &ch = str.at(n);
				if (!ch.isSpace() && ch != QChar(0xFFEF)) {
					if (n)
						str.remove(0, n);
					break;
				}
			}
			return str;
		}

		// Removes leading, trailing, and redundant whitespace from a string.
		Q_INVOKABLE static QString stringSimplify(const QString &str) { return str.simplified(); }

		// Appends string `line` to a block of `text`, and returns a new block with up to `maxLines` lines.
		// If `maxLines` is zero or negative, returns the full resulting string.
		// The newline (`\n`) is used as separator by default, or a custom separator string can be specified in `separator` argument.
		Q_INVOKABLE static QString appendLine(const QString &text, const QString &line, int maxLines, const QString &separator = QStringLiteral("\n"))
		{
			if (text.isEmpty() || maxLines == 1)
				return line;
			auto v = QStringView(text).split(separator);
			if (maxLines < 1 || v.length() < maxLines)
				return text + separator + std::move(line);
			QString ret;
			QTextStream strm(&ret, QIODevice::WriteOnly);
			v = v.mid(v.length() - maxLines + 1);
			for (const auto &sv : qAsConst(v))
				strm << sv.toString() << separator;
			return ret + std::move(line);
		}

		// Splits string `text` into lines based on `separator` and returns `maxLines` of text starting at `fromLine` (zero-based index).
		// Default `maxLines` is 1,  `fromLine` is zero (the start) and default `separator` is "\n". Specify a negative `fromLine` to count lines from the end
		// instead of the beginning (so `-1` returns one line from the end). The result may be the complete input if there were fewer then `maxLines` found in it.
		Q_INVOKABLE static QString getLines(const QString &text, int maxLines = 1, int fromLine = 0, const QString &separator = QStringLiteral("\n"))
		{
			if (text.isEmpty())
				return text;
			if (maxLines < 1)
				return QString();
			auto v = QStringView(text).split(separator);
			if (fromLine < 0)
				fromLine += v.length();
			if (fromLine < 0 || (fromLine + v.length() <= maxLines))
				return text;
			QString ret;
			QTextStream strm(&ret, QIODevice::WriteOnly);
			v = v.mid(fromLine, maxLines);
			for (const auto &sv : qAsConst(v))
				strm << sv.toString() << separator;
			return ret;
		}
		//! \}

		// \name Binary <-> Text data encoding/decoding
		// \{
		// Returns `data` bytes as base-64 encoded plain text ("binary to ascii"). Same as `toBase64()`.
		Q_INVOKABLE static QString btoa(const QByteArray &data)    { return data.toBase64(); }
		Q_INVOKABLE static QString btoa(const QString &data)       { return data.toLatin1().toBase64(); }
		// Returns `data` bytes as base-64 encoded plain text. Same as `btoa()`.
		Q_INVOKABLE static QString toBase64(const QByteArray &data)   { return data.toBase64(); }
		// Decodes `data` bytes of base-64 encoded plain text and returns it as binary data ("ascii to binary"). Same as `fromBase64()` except takes an `ArrayBuffer` natively vs. needing to convert from a string.
		Q_INVOKABLE static QByteArray atob(const QByteArray &data)       { return QByteArray::fromBase64(data); }
		Q_INVOKABLE static QByteArray atob(const QString &data)       { return QByteArray::fromBase64(data.toUtf8()); }
		// Decodes `data` string of base-64 encoded plain text and returns it as binary data ("ascii to binary"). Same as `atob()` except a string input type natively vs. needing to convert from `ArrayBuffer` type.
		Q_INVOKABLE static QByteArray fromBase64(const QString &data) { return QByteArray::fromBase64(data.toLatin1()); }
		// \}


		//! \name URL helpers.
		//! \{
		Q_INVOKABLE static QString urlScheme(const QString &url)         { return QUrl(url).scheme(); }
		Q_INVOKABLE static bool    urlIsValid(const QString &url)        { return QUrl(url).isValid(); }
		Q_INVOKABLE static bool    urlIsEmpty(const QString &url)        { return QUrl(url).isEmpty(); }
		Q_INVOKABLE static bool    urlIsRelative(const QString &url)     { return QUrl(url).isRelative(); }
		Q_INVOKABLE static bool    urlIsLocalPath(const QString &url)    { return QUrl(url).isLocalFile(); }
		Q_INVOKABLE static QString urlFromLocalPath(const QString &file) { return QUrl::fromLocalFile(file).toString(); }
		Q_INVOKABLE static QString urlToLocalPath(const QString &url)    { return QDir::toNativeSeparators(QUrl(url).toString(QUrl::PreferLocalFile)); }
		//! \}

};

#ifndef DOXYGEN
}  // ScriptLib
#endif

Q_DECLARE_METATYPE(ScriptLib::Util *)
