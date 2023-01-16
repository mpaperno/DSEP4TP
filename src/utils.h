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

#include <QDir>
#include <QJSValue>
#include <QJSValueIterator>
#include <QMutex>
#include <QThread>
#include <QTimer>
#include <QWaitCondition>

#include "common.h"

namespace Utils {

template <typename Func>
static inline void runOnThread(QThread *qThread, Func &&func)
{
	QTimer *t = new QTimer();
	t->moveToThread(qThread);
	t->setSingleShot(true);
	t->setInterval(0);
	QObject::connect(t, &QTimer::timeout, [=]()
	{
		func();
		t->deleteLater();
	});
	QMetaObject::invokeMethod(t, "start", Qt::QueuedConnection);
}

template <typename Func>
static inline void runOnThreadSync(QThread *qThread, Func &&func)
{
	QMutex m;
	QWaitCondition wc;
	runOnThread(qThread, [=, &wc]() {
		func();
		wc.notify_all();
	});
	m.lock();
	wc.wait(&m);
	m.unlock();
}

// unpacks an value which is a JS array into a list of individual JS values
static QJSValueList jsArrayToValueList(const QJSValue &array)
{
	QJSValueList list;
	int e = 0;
	if (array.isArray() && (e = array.property("length").toInt()) > 0) {
		list.reserve(e);
		for (int i=0; i < e; ++i)
			list << array.property(i);
	}
	return list;
}

static float percentOfRange(float value, float rangeMin, float rangeMax)
{
  return ((rangeMax - rangeMin) * 0.01f * qAbs(value)) + rangeMin;
}

static float rangeValueToPercent(float value, float rangeMin, float rangeMax)
{
  const float dlta = rangeMax - rangeMin;
  const float scale = dlta == 0.0f ? 100.0f : 100.0f / dlta;
  return qBound(0.0f, (value - rangeMin) * scale, 100.0f);
}

static float connectorValueToRange(int value, float minRangeValue, float maxRangeValue,
                                   const QMap<QString, QString> &dataMap, bool *ok = nullptr
                                   /*, float *rMin = nullptr, float *rMax = nullptr*/ )
{
	float rangeMin = minRangeValue,
	    rangeMax = maxRangeValue;
	bool kk = true;
	if (dataMap.contains("rangeMin")) {
		float tmp = dataMap.value("rangeMin", "0").toFloat(&kk);
		if (kk) {
			rangeMin = qBound(minRangeValue, tmp, maxRangeValue);
			tmp = dataMap.value("rangeMax", "0").toFloat(&kk);
			if (kk)
				rangeMax = qBound(minRangeValue, tmp, maxRangeValue);
		}
	}
	if (ok)
		*ok = kk;
	//if (rMin)
	//	*rMin = rangeMin;
	//if (rMax)
	//	*rMax = rangeMax;
	return percentOfRange((float)qBound(0, value, 100), rangeMin, rangeMax);
}

// `o` is the object to iterate over
static void dumpJsvRecursive(const QJSValue &o, int level = 0)
{
	QJSValue obj = o;
	while (obj.isObject()) {
		QJSValueIterator it(obj);
		while (it.hasNext()) {
			it.next();
			qCDebug(lcPlugin).noquote() << QString("    ").repeated(level) << it.name() << "=" << it.value().toString();
			if (it.value().isQObject())
				dumpJsvRecursive(it.value(), level + 1);
		}
		obj = obj.prototype();
	}
}

static QByteArray tpDataPath()
{
#ifdef Q_OS_WIN
	const QByteArray ret = QDir::fromNativeSeparators(qgetenv("APPDATA")).toUtf8();
#elif defined(Q_OS_MAC)
	const QByteArray ret = QByteArrayLiteral("~/Documents");
#else // Linux
	const QByteArray ret = QByteArrayLiteral("~/.config");
#endif
	return ret + QByteArrayLiteral("/TouchPortal");
}

template<typename S>
struct AutoResetString
{
	S &original;
	S temp;
	bool doReset;
	AutoResetString(S &orig, const S &temp, bool doReset = true) :
	  original(orig), doReset(doReset)
	{
		if (doReset) {
			this->temp = std::move(orig);
			original = temp;
		}
	}
	~AutoResetString() { if (doReset) original = std::move(temp); }
};

template<typename S>
struct AutoClearString
{
	S &original;
	bool doReset;
	AutoClearString(S &orig, const S &temp, bool doReset = true) :
	  original(orig), doReset(doReset)
	{
		if (doReset) {
			original = temp;
		}
	}
	~AutoClearString() { if (doReset) original.clear(); }
};

}
