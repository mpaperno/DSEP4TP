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

#include "common.h"
#include <QJSEngine>
#include <QJSValue>
#include <QPair>

// Note: See resources/scripts/global.js for the JS functions being called from these methods.
// These must be coordinated!

#define NAMED_EVENT_HANDLER(...)                             \
	protected:                                                 \
	static QString evNameToSignal(const QString &ev) {         \
		static QMap<QString, QString> map { __VA_ARGS__ };       \
		return map.value(ev, ev);                                \
	}                                                          \
	public:                                                    \
	Q_INVOKABLE QJSValue on(const QString &ev, QJSValue cb, QJSValue thisObj = QJSValue()) {                \
		return EventUtils::callHandler(this, "_namedEventOnHandler", { evNameToSignal(ev), cb, thisObj });    \
	}                                                                                                       \
	Q_INVOKABLE QJSValue once(const QString &ev, QJSValue cb, QJSValue thisObj = QJSValue()) {              \
		return EventUtils::callHandler(this, "_namedEventOnceHandler", { evNameToSignal(ev), cb, thisObj });  \
	}                                                                                                       \
	Q_INVOKABLE void off(const QString &ev, QJSValue cb, QJSValue thisObj = QJSValue()) {                   \
		EventUtils::callHandler(this, "_namedEventOffHandler", { evNameToSignal(ev), cb, thisObj }, false);   \
	}                                                                                                       \
	Q_INVOKABLE QJSValue addEventListener(const QString &ev, QJSValue cb, QJSValue o = QJSValue()) {   \
		bool os;                                                                                         \
		const QJSValue ctx = EventUtils::resolveListenerOptions(o, &os);                                 \
		return os ? once(ev, cb, ctx) : on(ev, cb, ctx);                                                 \
	}                                                                                                  \
	Q_INVOKABLE void removeEventListener(const QString &ev, QJSValue cb, QJSValue o = QJSValue()) {    \
		off(ev, cb, EventUtils::resolveListenerOptions(o));                                              \
	}

#define CONNECT_EVENT_HANDLER(NAME, CB) \
	EventUtils::callHandler(this, "_connectSignalHandler", { evNameToSignal(#NAME), CB }, false)

#define ON_NAMED_EVENT(EV_NAME)                                                     \
	Q_INVOKABLE QJSValue on ## EV_NAME(QJSValue cb, QJSValue thisObj = QJSValue()) {  \
		return on(#EV_NAME, cb, thisObj);                                               \
	}

#define EVENT_PROPERTY(NAME)                                      \
	Q_PROPERTY(QJSValue on##NAME READ _on_##NAME WRITE _on_##NAME)  \
	QJSValue _ev_##NAME {};                                         \
	QJSValue _on_##NAME() const { return _ev_##NAME; }              \
	void _on_##NAME(QJSValue cb) {                                  \
		if (_ev_##NAME.isCallable())                                  \
			off(#NAME, _ev_##NAME);                                     \
		if (cb.isCallable()) {                                        \
			const QJSValue df = CONNECT_EVENT_HANDLER(NAME, cb);        \
			if (!df.isNull()) {                                         \
				_ev_##NAME = cb;                                          \
				return;                                                   \
			}                                                           \
		}                                                             \
		_ev_##NAME = QJSValue();                                      \
	}

#define EVENT_PROPERTY_ALIAS(NAME, ALIAS)                         \
	Q_PROPERTY(QJSValue on##ALIAS READ _on_##NAME WRITE _on_##NAME)

#define INVOKE_EVENT_PROP(EVNAME, ...)  EventUtils::invokeHandlerProperty(this, "on" #EVNAME, __VA_ARGS__ )

#define EMIT_EVENT2(SIG, EVNAME, ...)      \
	Q_EMIT SIG(__VA_ARGS__);                 \
	INVOKE_EVENT_PROP(EVNAME, __VA_ARGS__ )

#define EMIT_EVENT(SIG, ...)  EMIT_EVENT2(SIG, SIG, __VA_ARGS__)

namespace EventUtils {

// Returns a QJSValue::NullValue if handler couldn't be invoked or it returns an error or expectFunctionReturn == true and handler returns a non-callable result.
inline QJSValue callHandler(QObject *o, const char *handler, QJSValueList vals, bool expectFunctionReturn = true)
{

	if (QJSEngine *jse = qjsEngine(o)) {
		const auto objScriptVal = jse->toScriptValue(o);
		if (!objScriptVal.isQObject()) {
			qCWarning(lcDse) << "callHandler(" << handler << "): Could not get this object as scriptValue() for" << o;
		}
		else if (const QJSValue hdlr = jse->globalObject().property(handler); hdlr.isCallable()) {
			vals.prepend(objScriptVal);
			const QJSValue ret = hdlr.call(vals);
			if (ret.isError())
				jse->throwError(ret);
			else if (expectFunctionReturn && !ret.isCallable())
				jse->throwError(jse->newErrorObject(QJSValue::TypeError, QStringLiteral("Event handler '%1' returned invalid value.").arg(handler)));
			else
				return ret;
		}
	}
	else {
		qCWarning(lcDse) << "callHandler(" << handler << "): Could not find QJSEngine for this object" << o;
	}
	return QJSValue(QJSValue::NullValue);
}

inline QJSValue resolveListenerOptions(const QJSValue o, bool *once = nullptr) {
	if (o.isObject()) {
		if (once) {
			const QJSValue os = o.property("once");
			*once = os.isBool() && os.toBool();
		}
		return o.property("context");
	}
	if (once)
		*once = false;
	return QJSValue();
}

template<class... Args>
inline QJSValueList toJsValueList(QJSEngine *jse, Args &&... args)
{
	QJSValueList list;
	list.reserve(sizeof...(args));
	((list << jse->toScriptValue(args)), ...);
	// ([&] { list << jse->toScriptValue(args); } (), ...);
	return list;
}

template<class... Args>
inline void invokeHandlerProperty(QObject *o, const char *propName, Args &&... args)
{
	if (QJSEngine *jse = qjsEngine(o)) {
		if (const auto thisVal = jse->toScriptValue(o); thisVal.isQObject()) {
			if (const QJSValue hdlr = thisVal.property(propName); hdlr.isCallable())
				hdlr.call(EventUtils::toJsValueList(jse, args... ));
		}
	}
}

}  // namespace EventUtils
