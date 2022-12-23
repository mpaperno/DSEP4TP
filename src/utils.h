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
#include <QThread>
#include <QTimer>

#include "common.h"

extern QString g_scriptsBaseDir;

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

static QString resolveFile(const QString &base)
{
	if (g_scriptsBaseDir.isEmpty() || base.isEmpty())
		return base;
	const QString tbase = QDir::fromNativeSeparators(base);
	if (QDir::isAbsolutePath(tbase))
		return tbase;
	return QDir::cleanPath(g_scriptsBaseDir + tbase);
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
