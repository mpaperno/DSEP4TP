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

#include "ScriptEngine.h"
#include "utils.h"
#include "ScriptingLibrary/AbortController.h"
#include "ScriptingLibrary/Dir.h"
#include "ScriptingLibrary/File.h"
#include "ScriptingLibrary/Process.h"
#include "ScriptingLibrary/Util.h"

#if !SCRIPT_ENGINE_USE_QML
// use privates to inject Locale and Date/Number formatting features normally in QQmlEngine into QJSEngine
#include <private/qqmllocale_p.h>
#include <private/qv4engine_p.h>
#include "ScriptingLibrary/DOMException.h"
#include "ScriptingLibrary/XmlHttpRequest.h"
#endif

#define EE_RETURN_FILE_ERROR_OBJ(FN, RES, MSG)  {                      \
	RES.setProperty(QStringLiteral("fileName"), FN);                     \
	const QString msg = SCRIPT_ENGINE_FORMAT_ERR_MSG(RES, ' ' + MSG +);  \
	QJSValue ret = se->newErrorObject(RES.errorType(), msg);             \
	ret.setProperty("cause", RES);                                       \
	return ret;                                                          \
}

using namespace Utils;
using namespace ScriptLib;

extern QString g_scriptsBaseDir;

ScriptEngine::ScriptEngine(bool isStatic, const QByteArray &instanceName, QObject *p) :
  QObject(p),
  m_currInstanceName(instanceName), m_isShared(isStatic)
{
	setObjectName(QLatin1String("ScriptEngine"));
	if (isStatic) {
		qRegisterMetaType<ScriptLib::AbortController>("AbortController");
		qRegisterMetaType<ScriptLib::AbortSignal>("AbortSignal");
#if !SCRIPT_ENGINE_USE_QML
		qRegisterMetaType<QVariant>();
		qRegisterMetaType<QJSValue>();
		qRegisterMetaType<QList<QObject*> >();
		qRegisterMetaType<QList<int> >();
#endif
	}
	initScriptEngine();
}

ScriptEngine::~ScriptEngine() {
	delete ulib;
	ulib = nullptr;
	if (se) {
		se->collectGarbage();
		se->deleteLater();
		se = nullptr;
	}
	//qDebug() << this << "Destroyed";
}

void ScriptEngine::initScriptEngine()
{
	if (ulib) {
		ulib->clearAllTimers();
	}
	else {
		ulib = new ScriptLib::Util(this);  // 'this' is NOT the QObject parent.
		QJSEngine::setObjectOwnership(ulib, QJSEngine::CppOwnership);
	}

	QMutexLocker lock(&m_mutex);
	if (se) {
		se->collectGarbage();
		se->deleteLater();
		se = nullptr;
	}

	se = new SCRIPT_ENGINE_BASE_TYPE();
	QJSEngine::setObjectOwnership(this, QJSEngine::CppOwnership);
	se->setProperty("ScriptEngine", QVariant::fromValue(this));  // used by library scripts to raise uncaught exceptions

#if !SCRIPT_ENGINE_USE_QML
	//se->installExtensions(QJSEngine::AllExtensions);
	se->handle()->initializeGlobal();    // HACK - this injects Date/Number formatting features... also XMLHttpRequest but using that crashes the program.
	dse_add_qmlxmlhttprequest(se->handle());  // so we inject our own version which is modified to use a fixed netowrk manager and doesn't rely on qmlEngine.
	dse_add_domexceptions(se->handle());
	//	QQmlLocale::registerStringLocaleCompare(se->handle());  // this works
	//	QQmlDateExtension::registerExtension(se->handle());     // thsese 2 don't bucause they're declared
	//	QQmlNumberExtension::registerExtension(se->handle());   // static and defined in a qqmllocale.cpp
#else
	se->setNetworkAccessManagerFactory(&m_factory);
	se->setOutputWarningsToStandardError(false);
	connect(se, &QQmlEngine::warnings, this, [=](const QList<QQmlError> &w) {
		for (const auto &ww : w)
			qCWarning(lcPlugin).nospace() << ww;
	}, Qt::DirectConnection);

	//QQmlComponent root(se, ":/qml/root.qml", QQmlComponent::CompilationMode::PreferSynchronous, se);
	//root.create();
	//if (root.isError())
	//	qDebug() << root.errors();
	//se->globalObject().setProperty("rootQmlObject", se->newQObject(&root));
#endif

	se->globalObject().setProperty("ScriptEngine", se->newQObject(this));               // CPP ownership
	se->globalObject().setProperty("Util", se->newQObject(ulib));                       // CPP ownership
	se->globalObject().setProperty("Dir", se->newQObject(ScriptLib::Dir::instance()));  // static instance has CPP ownership
	se->globalObject().setProperty("File", se->newQObject(new ScriptLib::File));        // QJSEngine has ownership
	se->globalObject().setProperty("FS", se->newQMetaObject(&ScriptLib::FS::staticMetaObject));
	se->globalObject().setProperty("FileHandle", se->newQMetaObject<ScriptLib::FileHandle>());
	se->globalObject().setProperty("Process", se->newQMetaObject<ScriptLib::Process>());
	se->globalObject().setProperty("AbortController", se->newQMetaObject<ScriptLib::AbortController>());
	se->globalObject().setProperty("AbortSignal", se->newQMetaObject<ScriptLib::AbortSignal>());
	se->globalObject().setProperty("globalThis", se->globalObject());
#if !SCRIPT_ENGINE_USE_QML
	se->globalObject().setProperty("Locale", se->newQMetaObject(&QQmlLocale::staticMetaObject));  // HACK - makes Locale namespace enums available
#endif

	evalScript(QStringLiteral(":/scripts/jslib.min.js"));
	//evalScript(QStringLiteral(":/scripts/collections.js"));
	//evalScript(QStringLiteral(":/scripts/color.js"));
	//evalScript(QStringLiteral(":/scripts/date.js"));
	//evalScript(QStringLiteral(":/scripts/env.js"));
	//evalScript(QStringLiteral(":/scripts/fetch.js"));
	//evalScript(QStringLiteral(":/scripts/math.js"));
	//evalScript(QStringLiteral(":/scripts/number.js"));
	//evalScript(QStringLiteral(":/scripts/promise.js"));
	//evalScript(QStringLiteral(":/scripts/sprintf.js"));
	//evalScript(QStringLiteral(":/scripts/string.js"));
	//evalScript(QStringLiteral(":/scripts/stringformat.js"));
	//evalScript(QStringLiteral(":/scripts/global.js"));

	QJSValue dse = dseObject();
	dse.setProperty(QStringLiteral("PLUGIN_VERSION_NUM"), PLUGIN_VERSION);
	dse.setProperty(QStringLiteral("PLUGIN_VERSION_STR"), QStringLiteral(PLUGIN_VERSION_STR));
	dse.setProperty(QStringLiteral("SCRIPTS_BASE_DIR"), g_scriptsBaseDir.isEmpty() ? QDir::currentPath() : g_scriptsBaseDir);
	if (!m_isShared) {
		dse.setProperty(QStringLiteral("INSTANCE_TYPE"), QStringLiteral("Private"));
		if (!m_currInstanceName.isEmpty())
			dse.setProperty(QStringLiteral("INSTANCE_NAME"), QLatin1String(m_currInstanceName));
	}

}

void ScriptEngine::clearInstanceData(const QByteArray &name)
{
	if (ulib)
		ulib->clearInstanceTimers(name);
}

void ScriptEngine::checkErrors() const
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	if (se->hasError()) {
		QJSValue res = se->catchError();
		if (!res.isUndefined() && !res.isNull()) {
			res.setProperty(QStringLiteral("message"), SCRIPT_ENGINE_FORMAT_ERR_MSG(res));
			Q_EMIT raiseError(res);
		}
	}
#endif
}

void ScriptEngine::throwError(const QJSValue &err) const
{
	if (!err.isUndefined() && !err.isNull()) {
		se->throwError(err);
		checkErrors();
		//Q_EMIT raiseError(err);
	}
}

void ScriptEngine::throwError(QJSValue err, const QByteArray &instName) const
{
	if (m_isShared && !instName.isEmpty())
		err.setProperty("instanceName", QLatin1String(instName));
	else
		err.setProperty("instanceName", QLatin1String(m_currInstanceName));
	throwError(err);
}

void ScriptEngine::throwError(QJSValue::ErrorType type, const QString &msg, const QJSValue &cause, const QByteArray &instName) const
{
	QJSValue err = se->newErrorObject(type, msg);
	if (!cause.isUndefined())
		err.setProperty("cause", cause);
	throwError(err, instName);
}

void ScriptEngine::throwError(QJSValue::ErrorType type, const QString &msg, const QByteArray &instName) const
{
	throwError(se->newErrorObject(type, msg), instName);
}

void ScriptEngine::throwError(QJSValue::ErrorType type, const QString &msg) const
{
	throwError(se->newErrorObject(type, msg), QByteArray());
}

QJSValue ScriptEngine::expressionValue(const QString &fromValue, const QByteArray &instName)
{
	QMutexLocker lock(&m_mutex);
	//AutoClearString acs(m_currInstanceName, instName, m_isShared);
	setInstanceProperties(instName);
	const QJSValue res = se->evaluate(fromValue);
	resetInstanceProperties();
	//se->collectGarbage();
	if (!res.isError())
		return res;
	QJSValue ret = se->newErrorObject(res.errorType(), res.property("name").toString() + ": " + tr("while evaluating the expression") + " '" + fromValue + "': " + res.property("message").toString());
	ret.setProperty("cause", res);
	return ret;
}

QJSValue ScriptEngine::scriptValue(const QString &fileName, const QString &expr, const QByteArray &instName)
{
	bool ok;
	QString script = readFile(fileName, &ok);
	if (!ok)
		return se->newErrorObject(QJSValue::URIError, tr("Could not read script file '%1': %2").arg(fileName, script));
	if (script.trimmed().isEmpty())
		return se->newErrorObject(QJSValue::URIError, tr("Script file '%1' was empty.").arg(fileName));
	if (!expr.isEmpty())
		script += '\n' + expr;
	//qCDebug(lcPlugin) << "File:" << fileName << "Contents:\n" << script;
	QMutexLocker lock(&m_mutex);
	//AutoClearString acs(m_currInstanceName, instName, m_isShared);
	setInstanceProperties(instName);
	QJSValue res = se->evaluate(script, fileName);
	//collectGarbage();
	resetInstanceProperties();
	if (!res.isError())
		return res;
	EE_RETURN_FILE_ERROR_OBJ(fileName, res, tr("while evaluating '%1'").arg(expr)+);
}

QJSValue ScriptEngine::moduleValue(const QString &fileName, const QString &alias, const QString &expr, const QByteArray &instName)
{
	QMutexLocker lock(&m_mutex);
	QJSValue mod = se->importModule(fileName);
	if (mod.isError()) {
		EE_RETURN_FILE_ERROR_OBJ(fileName, mod, tr("while importing module"));
	}
	globalObject().setProperty(alias, mod);
	lock.unlock();
	//se->collectGarbage();
	return expr.isEmpty() ? QJSValue(QJSValue::UndefinedValue) : expressionValue(expr, instName);
}

bool ScriptEngine::timerExpression(const ScriptLib::TimerData *timData)
{
	QJSValue res;
	bool ok = true;
	m_mutex.lock();
	setInstanceProperties(timData->instanceName);
	QJSManagedValue m(timData->expression, se);
	if (m.isFunction()) {
		if (!timData->thisObject.isUndefined() && !timData->thisObject.isNull())
			res = m.callWithInstance(timData->thisObject, timData->args);
		else
			res = m.call(timData->args);
	}
	else if (m.isObject())
		res = m.callAsConstructor(timData->args);
	else if (m.isString())
		res = engine()->evaluate(m.toString());
	else
		ok = false;
	resetInstanceProperties();
	m_mutex.unlock();

	//qCDebug(lcPlugin) << this << "TimerEvent:" << Util::TimerData::toString(timerType, timerId) << instName << "invalid?" << remove << "error?" << res.isError() << "expr:" << expression.toString() << "; on thread" << QThread::currentThread() << "app" << qApp->thread();
	if (!ok) {
		const QString msg =  tr("TypeError: (with %1) expression '%2' cannot be evaluated (must be a callable or a string). Timer event cancelled.'").arg(timData->toString(), timData->expression.toString());
		throwError(QJSValue::TypeError, msg, timData->instanceName);
		return false;
	}
	if (se->hasError()) {
		checkErrors();
		return false;
	}
	if (res.isError()) {
		const QString msg = SCRIPT_ENGINE_FORMAT_ERR_MSG(res, " using " + timData->toString() +);
		throwError(QJSValue::EvalError, msg, res, timData->instanceName);
		return false;
	}

	return true;
}

void ScriptEngine::include(const QString &file) const
{
	const QString script(File::read_impl(se, Utils::resolveFile(file), ScriptLib::FS::O_TEXT));
	if (script.isEmpty()) {
		checkErrors();
		return;
	}

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	QStringList stack;
	const QJSValue res = se->evaluate(script, file, 1, &stack);
	if (res.isError() || !stack.isEmpty()) {
#else
	const QJSValue res = se->evaluate(script, file);
	if (res.isError()) {
#endif
		throwError(res);
	}
}

void ScriptEngine::showNotification(const QByteArray &id, const QByteArray &title, const QByteArray &msg, QVariantList options, QJSValue callback)
{
	if (options.isEmpty())
		options << QVariantMap({{ QStringLiteral("id"), QStringLiteral("option") }, { QStringLiteral("title"), QStringLiteral(" ") } });
	if (callback.isCallable() || callback.isString())
		m_notificationCallbacks.insert(id, callback);
	emit tpNotification(id, title, msg, options);
}

void ScriptEngine::onNotificationClicked(const QString &notifyId, const QString &optionId)
{
	QJSValue cb = m_notificationCallbacks.value(notifyId.toUtf8());
	if (!cb.isCallable() && !cb.isString())
		return;

	QJSValue res;
	QJSValue thisObj;
	const QJSValueList args { optionId, notifyId };
	QJSManagedValue m;
	if (m.isArray()) {
		if (cb.property("length").toInt() > 1)
			thisObj = cb.property(1);
		m = QJSManagedValue(cb.property(0), se);
	}
	else {
		m = QJSManagedValue(cb, se);
	}
	if (m.isFunction()) {
		if (!thisObj.isUndefined() && !thisObj.isNull())
			res = m.callWithInstance(thisObj, args);
		else
			res = m.call(args);
	}
	else if (m.isObject())
		res = m.callAsConstructor(args);
	else if (m.isString())
		res = engine()->evaluate(m.toString());
	else
		return;

	checkErrors();
}
