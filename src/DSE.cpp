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

#include "version.h"
#include "DSE.h"
#include "DynamicScript.h"
#include "ScriptEngine.h"

using namespace ScriptLib;

Q_GLOBAL_STATIC(DSE::ScriptState, g_instances)
Q_GLOBAL_STATIC(QReadWriteLock, g_instanceMutex)
Q_GLOBAL_STATIC(DSE::EngineState, g_engines)
Q_GLOBAL_STATIC(QReadWriteLock, g_engineMutex)

DSE* DSE::sharedInstance = nullptr;
const quint32 DSE::pluginVersion { APP_VERSION };
const QByteArray DSE::pluginVersionStr { QByteArrayLiteral(APP_VERSION_STR) };
const QString DSE::platformOs {
#if defined(Q_OS_ANDROID)
	QStringLiteral("android")
#elif defined(Q_OS_IOS)
	QStringLiteral("ios")
#elif defined(Q_OS_TVOS)
	QStringLiteral("tvos")
#elif defined(Q_OS_MAC)
	QStringLiteral("osx")
#elif defined(Q_OS_WIN)
	QStringLiteral("windows")
#elif defined(Q_OS_LINUX)
	QStringLiteral("linux")
#elif defined(Q_OS_QNX)
	QStringLiteral("qnx")
#elif defined(Q_OS_WASM)
	QStringLiteral("wasm")
#elif defined(Q_OS_UNIX)
	QStringLiteral("unix")
#else
	QStringLiteral("unknown")
#endif
};

quint32 DSE::tpVersion {0};
QString DSE::tpVersionStr;
QString DSE::scriptsBaseDir;
QByteArray DSE::tpCurrentPage;
std::atomic_int DSE::defaultRepeatRate { -1 };
std::atomic_int DSE::defaultRepeatDelay { -1 };

DSE::DSE(ScriptEngine *se, QObject *p) :
  QObject(p), se(se)
{
	setObjectName("DSE");
	if (!DSE::sharedInstance)
		DSE::sharedInstance = this;
}

QByteArray DSE::engineInstanceName() const { return se->name(); }

DSE::ScriptState *DSE::instances() { return g_instances; }
QReadWriteLock *DSE::instances_mutex() { return g_instanceMutex; }

const QList<DynamicScript *> DSE::instances_const()
{
	QReadLocker l(g_instanceMutex);
	return g_instances->values();
}

DynamicScript *DSE::instance(const QByteArray &name)
{
	QReadLocker l(g_instanceMutex);
	return g_instances->value(name, nullptr);
}

DynamicScript *DSE::insert(const QByteArray &name, DynamicScript *ds)
{
	QWriteLocker l(g_instanceMutex);
	return g_instances->insert(name, ds).value();
}

bool DSE::removeInstance(const QByteArray &name)
{
	QWriteLocker l(g_instanceMutex);
	return g_instances->remove(name);
}

QByteArrayList DSE::instanceKeys()
{
	QReadLocker l(g_instanceMutex);
	return g_instances->keys();
}

QVariantList DSE::instanceNames()
{
	QReadLocker l(g_instanceMutex);
	return QVariant::fromValue(g_instances->keys()).toList();
}

QList<DynamicScript *> DSE::instanceList()
{
	QReadLocker l(g_instanceMutex);
	return g_instances->values();
}

DSE::EngineState *DSE::engines() { return g_engines; }
QReadWriteLock *DSE::engines_mutex() { return g_engineMutex; }

const QList<ScriptEngine *> DSE::engines_const()
{
	QReadLocker l(g_engineMutex);
	return g_engines->values();
}

ScriptEngine *DSE::insert(const QByteArray &name, ScriptEngine *se)
{
	QWriteLocker l(g_engineMutex);
	return g_engines->insert(name, se).value();
}

bool DSE::removeEngine(const QByteArray &name)
{
	QWriteLocker l(g_engineMutex);
	return g_engines->remove(name);
}

ScriptEngine *DSE::engine(const QByteArray &name)
{
	QReadLocker l(g_engineMutex);
	return g_engines->value(name, nullptr);
}

QByteArrayList DSE::engineKeys()
{
	QReadLocker l(g_engineMutex);
	return g_engines->keys();
}


QByteArray DSE::instanceDefault() const {
	if (DynamicScript *ds = instance(instanceName))
		return ds->defaultValue();
	return QByteArray();
}

void DSE::setDefaultActionRepeatRate(int ms)
{
	if (ms < 50)
		ms = 50;
	if (ms != defaultRepeatRate) {
		defaultRepeatRate = ms;
		Q_EMIT DSE::sharedInstance->defaultActionRepeatRateChanged(ms);
	}
}

void DSE::setDefaultActionRepeatDelay(int ms)
{
	if (ms < 50)
		ms = 50;
	if (ms != defaultRepeatDelay) {
		defaultRepeatDelay = ms;
		Q_EMIT DSE::sharedInstance->defaultActionRepeatDelayChanged(ms);
	}
}

void DSE::setActionRepeat_impl(quint8 property, quint8 action, int ms, const QByteArray &forInstance, uint repeaterId)
{
	if (repeaterId && m_actionData[ACT_ADJ_REPEAT].repeaterId != repeaterId)
		return;
	int repRate = 0;
	if (forInstance.isEmpty()) {
		if (action > SetAbsolute)
			setDefaultActionRepeatProperty(property, ms + defaultActionRepeatProperty(property));
		else
			setDefaultActionRepeatProperty(property, ms);
		if (repeaterId)
			repRate = defaultActionRepeatProperty(m_actionData[ACT_ADJ_REPEAT].isRepeating ? RepeatRateProperty : RepeatDelayProperty);
	}
	else if (DynamicScript *ds = instance(forInstance)) {
		if (action > SetAbsolute)
			ds->setRepeatProperty(property, ms + ds->repeatProperty(property));
		else
			ds->setRepeatProperty(property, ms);
		if (repeaterId)
			repRate = ds->repeatProperty(m_actionData[ACT_ADJ_REPEAT].isRepeating ? RepeatRateProperty : RepeatDelayProperty);
	}
	else {
		se->throwError(QJSValue::GenericError, tr("setActionRepeat() - Script instance name %1 not found.").arg(forInstance));
		return;
	}
	//qDebug() << property << action << ms << forInstance << repeaterId << m_repeaterId.load() << m_repeatingActions[ACT_ADJ_REPEAT_PROP] << repRate;
	if (repeaterId && m_actionData[ACT_ADJ_REPEAT].repeaterId == repeaterId) {
		if (repRate >= 50) {
			m_actionData[ACT_ADJ_REPEAT].isRepeating = true;
			QTimer::singleShot(repRate, this, [=]() { setActionRepeat_impl(property, action, ms, forInstance, repeaterId); } );
		}
		else {
			cancelRepeatingAction(ACT_ADJ_REPEAT);
		}
	}
}

void DSE::setActionRepeat(quint8 property, quint8 action, int ms, const QByteArray &forInstance, bool repeat)
{
	if (property > AllRepeatProperties || action > Decrement || !ms) {
		se->throwError(QJSValue::RangeError, tr("setActionRepeat() - Invalid property/action/value parameters."));
		return;
	}
	m_actionData[ACT_ADJ_REPEAT].repeaterId = repeat ? ++m_nextRepeaterId : 0;
	if ((action == Decrement && ms > 0) || (action == Increment && ms < 0))
		ms = -ms;
	setActionRepeat_impl(property, action, ms, forInstance, repeat ? m_actionData[ACT_ADJ_REPEAT].repeaterId.load() : 0);
}

void DSE::cancelRepeatingAction(quint8 act)
{
	m_actionData[act].repeaterId = 0;
	m_actionData[act].isRepeating = false;
	//qDebug() << m_repeatingActions[act].repeaterId << m_repeatingActions[act].isRepeating;
}

#include "moc_DSE.cpp"
