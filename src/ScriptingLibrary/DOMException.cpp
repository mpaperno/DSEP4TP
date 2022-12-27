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
#include "private/qv4managed_p.h"
#include "private/qv4mmdefs_p.h"
#include <QtCore/qglobal.h>
#include <private/qqmlglobal_p.h>
#include <qqmlengine.h>
#include <private/qqmlengine_p.h>
#include <private/qv4object_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4errorobject_p.h>
#include <private/qv4jscall_p.h>

using namespace QV4;

namespace QV4 {

namespace Heap {

struct DOMExceptionObject : public Object {
		void init() {
			Object::init();
			code = 0;
		}
		int code;
};

struct DOMExceptionCtor : public FunctionObject {
    void init(QV4::ExecutionEngine *engine, const QString &message = QString());
};

}  // ns Heap

struct DOMExceptionObject : Object
{
		V4_OBJECT2(DOMExceptionObject, Object)
		V4_NEEDS_DESTROY
		int code() const { return d()->code; }
		void setCode(int code) { d()->code = code; }
};


struct DOMExceptionCtor : public FunctionObject
{
		V4_OBJECT2(DOMExceptionCtor, FunctionObject)

		static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value * /* newTarget */)
		{

			Scope scope(f->engine());
			Value msgVal = argc ? argv[0] : Value::undefinedValue();
			Value nameVal = argc > 1 && !argv[1].isEmpty() ? argv[1] : scope.engine->newString(QStringLiteral("DOMException"))->asReturnedValue();
			Value code = argc > 2 && argv[2].isNumber() ? argv[2] : Value::fromInt32(0);
			ScopedObject ex(scope, scope.engine->newErrorObject(msgVal));
			ex->put(ScopedString(scope, scope.engine->newIdentifier(QStringLiteral("name"))), ScopedString(scope, nameVal.stringValue()));
			ex->put(ScopedString(scope, scope.engine->newIdentifier(QStringLiteral("code"))), code);
			return ex.asReturnedValue();
		}

		static ReturnedValue virtualCall(const FunctionObject *f, const Value *, const Value *argv, int argc) {
			return f->callAsConstructor(argv, argc);
		}

#if 0
		void setupProto(const QString &/*message*/)
		{
			ExecutionEngine *v4 = engine();
	    Scope scope(v4);
	    ScopedObject p(scope, v4->newErrorObject(QStringLiteral("")));
//			ScopedObject p(scope, v4->newObject());
	    d()->proto.set(scope.engine, p->d());
			p->defineAccessorProperty(QStringLiteral("code"), method_get_code, method_set_code);
		}

		static ReturnedValue method_get_code(const FunctionObject *b, const Value *thisObject, const Value *, int)
		{
			Scope scope(b);
		  Scoped<DOMExceptionObject> w(scope, thisObject->as<DOMExceptionObject>());
			if (!w)
				return scope.engine->throwTypeError();
			return Encode(w->code());
		}
		static ReturnedValue method_set_code(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
		{
			Scope scope(b);
		  Scoped<DOMExceptionObject> w(scope, thisObject->as<DOMExceptionObject>());
			if (!w)
				return scope.engine->throwTypeError();
			if (argc < 1)
				return scope.engine->throwSyntaxError(QStringLiteral("Incorrect argument count."));
			w->setCode(argv[0].toInt32());
			return Encode::undefined();
		}
#endif
};

void Heap::DOMExceptionCtor::init(QV4::ExecutionEngine *engine, const QString &/*message*/)
{
	Heap::FunctionObject::init(engine->rootContext(), QStringLiteral("DOMException"));
	Scope scope(engine);
  Scoped<QV4::DOMExceptionCtor> ctor(scope, this);

	ctor->defineReadonlyProperty(QStringLiteral("INDEX_SIZE_ERR"), Value::fromInt32(DOMEXCEPTION_INDEX_SIZE_ERR));
	ctor->defineReadonlyProperty(QStringLiteral("DOMSTRING_SIZE_ERR"), Value::fromInt32(DOMEXCEPTION_DOMSTRING_SIZE_ERR));
	ctor->defineReadonlyProperty(QStringLiteral("HIERARCHY_REQUEST_ERR"), Value::fromInt32(DOMEXCEPTION_HIERARCHY_REQUEST_ERR));
	ctor->defineReadonlyProperty(QStringLiteral("WRONG_DOCUMENT_ERR"), Value::fromInt32(DOMEXCEPTION_WRONG_DOCUMENT_ERR));
	ctor->defineReadonlyProperty(QStringLiteral("INVALID_CHARACTER_ERR"), Value::fromInt32(DOMEXCEPTION_INVALID_CHARACTER_ERR));
	ctor->defineReadonlyProperty(QStringLiteral("NO_DATA_ALLOWED_ERR"), Value::fromInt32(DOMEXCEPTION_NO_DATA_ALLOWED_ERR));
	ctor->defineReadonlyProperty(QStringLiteral("NO_MODIFICATION_ALLOWED_ERR"), Value::fromInt32(DOMEXCEPTION_NO_MODIFICATION_ALLOWED_ERR));
	ctor->defineReadonlyProperty(QStringLiteral("NOT_FOUND_ERR"), Value::fromInt32(DOMEXCEPTION_NOT_FOUND_ERR));
	ctor->defineReadonlyProperty(QStringLiteral("NOT_SUPPORTED_ERR"), Value::fromInt32(DOMEXCEPTION_NOT_SUPPORTED_ERR));
	ctor->defineReadonlyProperty(QStringLiteral("INUSE_ATTRIBUTE_ERR"), Value::fromInt32(DOMEXCEPTION_INUSE_ATTRIBUTE_ERR));
	ctor->defineReadonlyProperty(QStringLiteral("INVALID_STATE_ERR"), Value::fromInt32(DOMEXCEPTION_INVALID_STATE_ERR));
	ctor->defineReadonlyProperty(QStringLiteral("SYNTAX_ERR"), Value::fromInt32(DOMEXCEPTION_SYNTAX_ERR));
	ctor->defineReadonlyProperty(QStringLiteral("INVALID_MODIFICATION_ERR"), Value::fromInt32(DOMEXCEPTION_INVALID_MODIFICATION_ERR));
	ctor->defineReadonlyProperty(QStringLiteral("NAMESPACE_ERR"), Value::fromInt32(DOMEXCEPTION_NAMESPACE_ERR));
	ctor->defineReadonlyProperty(QStringLiteral("INVALID_ACCESS_ERR"), Value::fromInt32(DOMEXCEPTION_INVALID_ACCESS_ERR));
	ctor->defineReadonlyProperty(QStringLiteral("VALIDATION_ERR"), Value::fromInt32(DOMEXCEPTION_VALIDATION_ERR));
	ctor->defineReadonlyProperty(QStringLiteral("TYPE_MISMATCH_ERR"), Value::fromInt32(DOMEXCEPTION_TYPE_MISMATCH_ERR));
	ctor->defineReadonlyProperty(QStringLiteral("SECURITY_ERR"), Value::fromInt32(DOMEXCEPTION_SECURITY_ERR));
	ctor->defineReadonlyProperty(QStringLiteral("NETWORK_ERR"), Value::fromInt32(DOMEXCEPTION_NETWORK_ERR));
	ctor->defineReadonlyProperty(QStringLiteral("ABORT_ERR"), Value::fromInt32(DOMEXCEPTION_ABORT_ERR));
	ctor->defineReadonlyProperty(QStringLiteral("URL_MISMATCH_ERR"), Value::fromInt32(DOMEXCEPTION_URL_MISMATCH_ERR));
	ctor->defineReadonlyProperty(QStringLiteral("QUOTA_EXCEEDED_ERR"), Value::fromInt32(DOMEXCEPTION_QUOTA_EXCEEDED_ERR));
	ctor->defineReadonlyProperty(QStringLiteral("TIMEOUT_ERR"), Value::fromInt32(DOMEXCEPTION_TIMEOUT_ERR));
	ctor->defineReadonlyProperty(QStringLiteral("INVALID_NODE_ERR"), Value::fromInt32(DOMEXCEPTION_INVALID_NODE_ERR));
	ctor->defineReadonlyProperty(QStringLiteral("DATA_CLONE_ERR"), Value::fromInt32(DOMEXCEPTION_DATA_CLONE_ERR));

//	if (!ctor->d()->proto)
//      ctor->setupProto(message);
//  ScopedString s(scope, engine->id_prototype());
//  ctor->defineDefaultProperty(s, ScopedObject(scope, ctor->d()->proto), Attr_Data);
}

ReturnedValue throwDomError(QV4::ExecutionEngine *e, int error, const QString &message, const QString &name)
{
	QV4::Scope scope(e);
	FunctionObject *fo = e->functionCtor();
	JSCallArguments jsCallData(scope, 3);
	jsCallData.args[0] = QV4::ScopedValue(scope, scope.engine->newString(message));
	jsCallData.args[1] = QV4::ScopedValue(scope, scope.engine->newString(name));
	jsCallData.args[2] = QV4::ScopedValue(scope, QV4::Value::fromInt32(error));
	ReturnedValue o = DOMExceptionCtor::virtualCallAsConstructor(fo, jsCallData.args, jsCallData.argc, nullptr);
	ScopedObject ex(scope, o);
	return e->throwError(ex);
}

}  // ns QV4

DEFINE_OBJECT_VTABLE(DOMExceptionObject);
DEFINE_OBJECT_VTABLE(DOMExceptionCtor);

void dse_add_domexceptions(ExecutionEngine *e)
{
	Scope scope(e);
  Scoped<DOMExceptionCtor> ctor(scope, e->memoryManager->allocate<DOMExceptionCtor>(e));
  ScopedString s(scope, e->newString(QStringLiteral("DOMException")));
  e->globalObject->defineReadonlyConfigurableProperty(s, ctor);

}
