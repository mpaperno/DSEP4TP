#pragma once

#include <QJSValue>
#include <QMetaEnum>
#include <QMetaObject>
#include "common.h"

namespace ScriptLib
{
	Q_NAMESPACE

	enum ErrorType {
		NoError,
		GenericError,
		EvalError,
		RangeError,
		ReferenceError,
		SyntaxError,
		TypeError,
		URIError,
		DomException,
	};
	Q_ENUM_NS_N(ErrorType, ScriptLib);

	inline QJSValue::ErrorType toValueErrorType(int code) {
		if (code < DomException)
			return QJSValue::ErrorType(code);
		return QJSValue::GenericError;
	}


	inline const QMetaEnum errorTypeMeta() { static const QMetaEnum m = QMetaEnum::fromType<ScriptLib::ErrorType>(); return m; }

	inline QString errorTypeName(int code) {
		if (code < DomException)
			return errorTypeMeta().key(code);
		return errorTypeMeta().key(ErrorType::NoError);
	}

	inline bool isDomError(int code) {
		return code >= ScriptLib::DomException;
	}

	//! converts a DOMException::ErrorCode to a custom ErrorType code
	inline int domToCustomError(int domErrorCode) {
		return ScriptLib::DomException + domErrorCode;
	}

	//! converts from a custom ErrorType code to a DOMException::ErrorCode;
	//! if `errorType` is one of the standard error types, returns it as-is
	inline int domFromCustomError(int errorType) {
		if (isDomError(errorType))
			return errorType - ScriptLib::DomException;
		return errorType;
	}

namespace DOMException {

	Q_NAMESPACE

	// From DOM-Level-3-Core spec
	// http://www.w3.org/TR/DOM-Level-3-Core/core.html
	enum ErrorCode
	{
		NoError = 0,
		IndexSizeError,
		StringSizeError,
		HierarchyRequestError,
		WrongDocumentError,
		InvalidCharacterError,
		NoDataAllowedError,
		NoModificationAllowedError,
		NotFoundError,
		NotSupportedError,
		InUseAttributeError,
		InvalidStateError,
		SyntaxError,
		InvalidModificationError,
		NamespaceError,
		InvalidAccessError,
		ValidationError,
		TypeMismatchError,
		SecurityError,
		NetworkError,
		AbortError,
		URLMismatchError,
		QuotaExceededError,
		TimeoutError,
		InvalidNodeTypeError,
		DataCloneError,
		// custom codes for standard error types
		EncodingError,
		NotReadableError,
		UnknownError,
		ConstraintError,
		DataError,
		TransactionInactiveError,
		ReadOnlyError,
		VersionError,
		OperationError,
		NotAllowedError,
		STD_ERROR_LAST = DataCloneError,
		DOM_ERROR_LAST = NotAllowedError,
	};
	Q_ENUM_NS_N(ErrorCode, DOMException)

	inline const QMetaEnum errorCodeMeta() { static const QMetaEnum m = QMetaEnum::fromType<ScriptLib::DOMException::ErrorCode>(); return m; }

	inline const char *errorName(int code) {
		return errorCodeMeta().key(code);
	}

	inline int codeFromName(const char *name) {
		const int v = errorCodeMeta().keyToValue(name);
		return v > -1 ? v : UnknownError;
	}

	inline bool hasStdConstant(int code) {
		return code >= 0 && code <= ErrorCode::STD_ERROR_LAST;
	}

	inline QString standardConstant(int code)
	{
		static const QString names[ErrorCode::STD_ERROR_LAST+1]
		{
		  QStringLiteral("NO_ERROR"),
		  QStringLiteral("INDEX_SIZE_ERR"),
			QStringLiteral("DOMSTRING_SIZE_ERR"),
			QStringLiteral("HIERARCHY_REQUEST_ERR"),
			QStringLiteral("WRONG_DOCUMENT_ERR"),
			QStringLiteral("INVALID_CHARACTER_ERR"),
			QStringLiteral("NO_DATA_ALLOWED_ERR"),
			QStringLiteral("NO_MODIFICATION_ALLOWED_ERR"),
			QStringLiteral("NOT_FOUND_ERR"),
			QStringLiteral("NOT_SUPPORTED_ERR"),
			QStringLiteral("INUSE_ATTRIBUTE_ERR"),
			QStringLiteral("INVALID_STATE_ERR"),
			QStringLiteral("SYNTAX_ERR"),
			QStringLiteral("INVALID_MODIFICATION_ERR"),
			QStringLiteral("NAMESPACE_ERR"),
			QStringLiteral("INVALID_ACCESS_ERR"),
			QStringLiteral("VALIDATION_ERR"),
			QStringLiteral("TYPE_MISMATCH_ERR"),
			QStringLiteral("SECURITY_ERR"),
			QStringLiteral("NETWORK_ERR"),
			QStringLiteral("ABORT_ERR"),
			QStringLiteral("URL_MISMATCH_ERR"),
			QStringLiteral("QUOTA_EXCEEDED_ERR"),
			QStringLiteral("TIMEOUT_ERR"),
			QStringLiteral("INVALID_NODE_ERR"),
			QStringLiteral("DATA_CLONE_ERR"),
		};
		if (hasStdConstant(code))
			return names[code];
		return QString();
	}

}  // namespace DOMException

}  // namespace ScriptLib
