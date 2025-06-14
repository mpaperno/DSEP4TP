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

#include "DOMException.h"
#include "ScriptLibNS.h"
#include "private/qv4managed_p.h"
#include <QtCore/qglobal.h>
#include <private/qqmlglobal_p.h>
#include <qjsengine.h>
// #include <private/qjsengine_p.h>
#include <private/qjsvalue_p.h>
#include <private/qv4object_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4errorobject_p.h>
#include <private/qv4jscall_p.h>

using namespace QV4;
using namespace ScriptLib;

namespace QV4 {

namespace Heap {

struct DOMExceptionObject : public Object {
		void init() {
			Object::init();
		}
};

#define DOMExceptionCtorMembers(class, Member) \
	Member(class, Pointer, Object *, proto)

DECLARE_HEAP_OBJECT(DOMExceptionCtor, FunctionObject) {
	DECLARE_MARKOBJECTS(DOMExceptionCtor)
	void init(ExecutionEngine *engine);
};
}  // ns Heap

struct DOMExceptionObject : Object
{
		V4_OBJECT2(DOMExceptionObject, Object)
		V4_NEEDS_DESTROY
};


struct DOMExceptionCtor : public FunctionObject
{
		V4_OBJECT2(DOMExceptionCtor, FunctionObject)

		static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value * /* newTarget */)
		{
			Scope scope(f->engine());
			Value msgVal = argc ? argv[0] : Value::undefinedValue();
			Value code = Value::undefinedValue();
			Value nameVal = Value::undefinedValue();
			if (argc > 1) {
				if (argv[1].isString() && !argv[1].toQStringNoThrow().trimmed().isEmpty())
					nameVal = argv[1];
				else if (argv[1].isNumber() && argv[1].toNumber() > -1)
					code = argv[1];
			}
			if (code.isUndefined()) {
				if (argc > 2 && argv[2].isNumber() && argv[2].toNumber() > -1)
					code = argv[2];
				else if (!nameVal.isUndefined())
					code = Value::fromInt32(DOMException::codeFromName(nameVal.toQStringNoThrow().toLocal8Bit()));
				else
					code = Value::fromInt32(DOMException::UnknownError);
			}
			if (nameVal.isUndefined()) {
				QString name = !code.isUndefined() ? DOMException::errorName(code.toInt32()) : QString();
				if (name.isEmpty())
					name = QStringLiteral("DOMException");
				nameVal = Value::fromReturnedValue(scope.engine->newString(name)->asReturnedValue());
			}
			ScopedObject ex(scope, scope.engine->newErrorObject(msgVal));
			ex->put(ScopedString(scope, scope.engine->newIdentifier(QStringLiteral("name"))), ScopedString(scope, nameVal.stringValue()));
			ex->put(ScopedString(scope, scope.engine->newIdentifier(QStringLiteral("code"))), code);
			return ex.asReturnedValue();
		}

		static ReturnedValue virtualCall(const FunctionObject *f, const Value *, const Value *argv, int argc) {
			return f->callAsConstructor(argv, argc);
		}
};

void Heap::DOMExceptionCtor::init(QV4::ExecutionEngine *e)
{
	Heap::FunctionObject::init(
#if (QT_VERSION >= QT_VERSION_CHECK(6, 8, 0))
    e,
#else
    e->rootContext(),
#endif
    QStringLiteral("DOMException")
  );
	Scope scope(e);
  Scoped<QV4::DOMExceptionCtor> ctor(scope, this);

	if (!ctor->d()->proto) {
		ScopedObject p(scope, e->newErrorObject("")->prototype());
		ctor->d()->proto.set(scope.engine, p->d());
	}
	ScopedObject p(scope, ctor->d()->proto);
	ctor->defineDefaultProperty(ScopedString(scope, e->id_prototype()), p, Attr_Data);

	auto defConstant = [e, ctor, &p](int v) mutable {
		const QString en(DOMException::errorName(v));
		Value vv = Value::fromInt32(v);
		if (DOMException::hasStdConstant(v))
			ctor->defineReadonlyProperty(DOMException::standardConstant(v), vv);
		ctor->defineReadonlyProperty(en, vv);
		p->insertMember(ScopedString(Scope(e), e->newIdentifier(en)), vv, Attr_NotWritable | Attr_NotConfigurable);
	};

	for (int i=0; i <= DOMException::DOM_ERROR_LAST; ++i)
		defConstant(i);
}

QV4::ReturnedValue newDomError(QV4::ExecutionEngine *e, int error, const QString &message, const QString &name)
{
	QV4::Scope scope(e);
	FunctionObject *fo = e->functionCtor();
	JSCallArguments jsCallData(scope, 3);
	jsCallData.args[0] = QV4::ScopedValue(scope, scope.engine->newString(message));
	jsCallData.args[1] = QV4::ScopedValue(scope, scope.engine->newString(name));
	jsCallData.args[2] = QV4::ScopedValue(scope, QV4::Value::fromInt32(error));
	return DOMExceptionCtor::virtualCallAsConstructor(fo, jsCallData.args, jsCallData.argc, nullptr);
}

QV4::ReturnedValue throwDomError(QV4::ExecutionEngine *e, int error, const QString &message, const QString &name) {
	return e->throwError(ScopedObject(QV4::Scope(e), newDomError(e, error, message, name)));
}

QJSValue newDomErrorObject(QV4::ExecutionEngine *e, int error, const QString &message, const QString &name) {
	return QJSValuePrivate::fromReturnedValue(newDomError(e, error, message, name));
}

QJSValue newDomErrorObject(ExecutionEngine *e, const QString &name, const QString &message) {
	return newDomErrorObject(e, -1, message, name);
}

} // namespace QV4

DEFINE_OBJECT_VTABLE(DOMExceptionObject);
DEFINE_OBJECT_VTABLE(DOMExceptionCtor);

void dse_add_domexceptions(QV4::ExecutionEngine *e)
{
	QV4::Scope scope(e);
  QV4::Scoped<DOMExceptionCtor> ctor(scope, e->memoryManager->allocate<DOMExceptionCtor>(e));
  QV4::ScopedString s(scope, e->newString(QStringLiteral("DOMException")));
  e->globalObject->defineReadonlyConfigurableProperty(s, ctor);

}
