
#include <QtConcurrent>
#include <QFuture>
#include <QJSEngine>
#include <QRegularExpression>
#include <QStorageInfo>
#include <QStringConverter>

#include "common.h"
#include "File.h"
#include "AbortController.h"
#include "ScriptEngine.h"
// #include "Util.h"

using namespace ScriptLib;
using namespace FS;
using namespace DOMException;
using namespace Qt::Literals::StringLiterals;

static constexpr const size_t chunkSize = 4096;
static constexpr const uchar utf8bom[] = { 0xEF, 0xBB, 0xBF };

// static inline constexpr uchar bomSizeForEncoding(QStringConverter::Encoding encoding) {
// 	return
// 			encoding == QStringConverter::Utf8 ? 3
// 		: encoding >= QStringConverter::Utf16 && encoding <= QStringConverter::Utf16BE ? 2
// 		: encoding >= QStringConverter::Utf32 && encoding <= QStringConverter::Utf32BE ? 4
// 		: 0;
// }

static std::optional<QStringEncoder::Encoding> detectFileEncoding(QFile &fh)
{
	const auto pos = fh.pos();
	fh.seek(0);
	const auto enc = QStringConverter::encodingForData(fh.peek(4)).value_or(QStringConverter::Utf8);
	fh.seek(pos);
	return enc;
}

static bool checkMode(int mode) {
	return !FS::openModeMeta().valueToKeys(mode).isEmpty();
};


bool File::parseFileOptions(FileOptions &fo, const QString &file, QJSValue mode_opt_cb, QJSValue callback) const
{
	const auto raiseError = [this, &fo](int type, const QString &msg) {
		this->raiseError(type, msg, fo.callback);
	};

	const auto checkEncoding = [&fo](const QString &v)
	{
		if (!v.compare("auto"_L1, Qt::CaseInsensitive)) {
			fo.encoding = QStringConverter::Utf8;  // use this default to indicate that decoding was requested...
			fo.autodetectEnc = true;               // but we'll attempt to be auto-detect the encoding type
		}
		else if (const auto enc = QStringConverter::encodingForName(v.toLatin1())) {
			fo.encoding = enc.value();
			fo.autodetectEnc = false;
		}
	};

	const auto checkBom = [&fo]() {
		if (fo.encoding.has_value() && *fo.encoding >= QStringConverter::Utf16 && *fo.encoding <= QStringConverter::Utf32BE)
			fo.writeBom = true;
	};

	if (callback.isCallable()) {
		fo.callback = callback;
	}
	else if (mode_opt_cb.isCallable()) {
		fo.callback = mode_opt_cb;
		return true;  // nothing else to do
	}
	else if (fo.async) {
		raiseError(ErrorType::TypeError, u"No callback handler defined for async method call"_s);
		return false;
	}

	if (file.trimmed().isEmpty()) {
		raiseError(ErrorType::TypeError, u"Missing or empty file argument"_s);
		return false;
	}

	if (mode_opt_cb.isUndefined() || mode_opt_cb.isNull())
		return true;  // nothing else to do

	if (mode_opt_cb.isString()) {
		// a string value could be encoding or mode flags
		const QString strVal = mode_opt_cb.toString();
		checkEncoding(strVal);
		if (fo.encoding.has_value()) {
			checkBom();
		}
		else {
			fo.mode = modeToFlags(strVal);
			if (!checkMode(fo.mode)) {
				raiseError(ErrorType::TypeError, u"Invalid argument value '%1' is neither a known encoding name nor valid file open mode flag(s)."_s.arg(strVal));
				return false;
			}
		}
	}
	else if (mode_opt_cb.isNumber()) {
		// numeric value must be mode flag(s)
		fo.mode = OpenMode(mode_opt_cb.toNumber());
		if (!checkMode(fo.mode)) {
			raiseError(ErrorType::TypeError, u"Invalid file open mode flags enumeration value '%1'"_s.arg(mode_opt_cb.toNumber()));
			return false;
		}
	}
	else if (mode_opt_cb.isObject()) {
		// object with options

		if (const auto p = mode_opt_cb.property(u"mode"_s); !p.isUndefined()) {
			if (p.isString()) {
				fo.mode = modeToFlags(p.toString());
			}
			else if (p.isNumber()) {
				fo.mode = OpenMode(p.toNumber());
			}
			else {
				raiseError(ErrorType::TypeError, u"'mode' option must be numeric or a string"_s);
				return false;
			}
			if (!checkMode(fo.mode)) {
				raiseError(ErrorType::TypeError, u"Invalid file open mode flags value '%1'"_s.arg(p.toString()));
				return false;
			}
		}

		if (const auto p = mode_opt_cb.property(u"signal"_s); p.isQObject()) {
			if (QJSEngine *jse = qjsEngine(this))
				fo.signal = jse->fromScriptValue<AbortSignal*>(p);
		}

		if (const auto p = mode_opt_cb.property(u"chunkSize"_s); p.isNumber() && p.toNumber() > 0)
			fo.chunkSize = p.toNumber();

		if (const auto p = mode_opt_cb.property(u"readMaxSize"_s); p.isNumber())
			fo.maxSize = p.toNumber();

		if (const auto p = mode_opt_cb.property(u"encoding"_s); p.isString()) {
			checkEncoding(p.toString());
			if (!fo.encoding.has_value()) {
				raiseError(ErrorType::TypeError, u"Could not find a suitable encoding for '%1'"_s.arg(p.toString()));
				return false;
			}
		}

		if (const auto p = mode_opt_cb.property(u"writeBOM"_s); p.isBool())
			fo.writeBom = p.toBool();
		else
			checkBom();

		if (const auto p = mode_opt_cb.property(u"writeCR"_s); p.isBool())
			fo.writeCr = p.toBool();

		if (const auto p = mode_opt_cb.property(u"writeSafe"_s); p.isBool() || p.isString())
			fo.safeWrite = p.isBool() && p.toBool() ? SafeWrite::WriteSafe : p.isString() && !p.toString().isEmpty() ? SafeWrite::WriteSafeFallback : SafeWrite::WriteNormal;

	}
	else {
		raiseError(ErrorType::TypeError, u"The 'options' argument must be a string, number, options object, or callback function"_s);
		return false;
	}

	// force text mode if encoding was specified
	if (fo.encoding.has_value())
		fo.mode.setFlag(FS::O_BIN, false).setFlag(FS::O_TEXT);

	// set default chunk size based on device block size
	if (!fo.chunkSize)
		fo.chunkSize = (size_t)QStorageInfo(file).blockSize();

	return true;
}

//
// Read
//

// static
File::FileOpResult File::readFile(const QString &file, const FileOptions &fo)
{
	bool abort = false;
	if (fo.signal) {
		if (fo.signal->aborted())
			return { domToCustomError(DOMException::AbortError), u"File read request aborted before starting."_s };
		fo.signal->connect(fo.signal, &AbortSignal::abort, fo.signal, [&abort](){ abort = true; }, Qt::SingleShotConnection);
	}

	const OpenMode mode = OpenMode(fo.mode).setFlag(O_WRONLY, false).setFlag(O_RDONLY);

	int error;
	QString errStr;
	QFile fh(file);
	if (!fh.exists())
		return { domToCustomError(DOMException::NotFoundError), u"Source file '%1' not found."_s.arg(file) };

	if ((error = File::open_impl(fh, nullptr, mode, &errStr)) != ErrorType::NoError)
		return { error, errStr };

	// check if we're reading encoded text
	QStringConverter::Encoding enc;
	bool readDecoded = fo.autodetectEnc || fo.encoding.has_value();  // && *fo.encoding != QStringEncoder::Utf8;
	if (readDecoded) {
		if (fo.autodetectEnc)
			enc = detectFileEncoding(fh).value_or(QStringConverter::Utf8);
		else
			enc = fo.encoding.value_or(QStringConverter::Utf8);
		// there's no need to explicitly decode UTF-8 or Latin1
		if (enc == QStringConverter::Utf8 || enc == QStringConverter::Latin1)
			readDecoded = false;
	}

	QVariant ret;
	const int64_t readSize = fo.chunkSize > 0 ? fo.chunkSize : chunkSize;

	if (readDecoded) {
		// Read as decoded text into a string
		QString retStr;
		QStringDecoder fromUtf16(enc);
		retStr.reserve(fromUtf16.requiredSpace(fh.size()));
		// handle text translation and bypass the Text flag in the device.
		fh.setTextModeEnabled(false);
		while (!fh.atEnd() && !abort) {
			int64_t sz = readSize;
			if (fo.maxSize > -1)
				sz = std::max(std::min(sz, fo.maxSize - retStr.size()), 0LL);
			retStr.append(fromUtf16(fh.read(sz)));
			if (sz != readSize)
				break;
		}
		retStr.replace("\r\n"_L1, "\n"_L1);
		ret = retStr;
	}
	else {
		// Read as bytes
		QByteArray retBa;
		retBa.reserve(fh.size());
		while (!fh.atEnd() && !abort) {
			int64_t sz = readSize;
			if (fo.maxSize > -1)
				sz = std::max(std::min(sz, fo.maxSize - retBa.size()), 0LL);
			retBa.append(fh.read(sz));
			if (sz != readSize)
				break;
		}
		// return a string if an encoding was specified; assumes UTF-8
		if (fo.encoding.has_value())
			ret = QString::fromUtf8(retBa);
		else
			ret = retBa;
	}

	if (abort) {
		ret = u"File read request aborted."_s;
		error = domToCustomError(DOMException::AbortError);
	}
	else if (fh.error() != QFileDevice::NoError) {
		ret = u"Could not read from file '%1': %2"_s.arg(file).arg(fh.errorString());
		error = fileErrorToJsError(fh.error());
	}

	fh.close();
	return { error, ret };
}

QVariant File::read(const QString &file, QJSValue mode) const
{
	FileOptions fo;
	if (!parseFileOptions(fo, file, mode))
		return fo.encoding.has_value() ? QString() : QByteArray();

	const auto res = readFile(file, fo);
	if (res.first == ErrorType::NoError)
		return res.second;
	raiseError(res.first, res.second.toString());
	return fo.encoding.has_value() ? QString() : QByteArray();
}

void File::readAsyncCb(const QString &file, QJSValue mode_opt_cb, QJSValue callback)
{
	FileOptions fo{ true };
	if (!parseFileOptions(fo, file, mode_opt_cb, callback))
		return;

	callback = fo.callback;
	QtConcurrent::run(readFile, file, std::move(fo))
	.then(this, [this, callback](const FileOpResult &result) {
		// Q_EMIT resultReady(result.second, fo.callback, result.first);
		onResultReady(result.second, callback, result.first);
	});

	// .onCanceled(this, [this, fo]() {
	// 	Q_EMIT resultReady("Request aborted.", fo.callback, toErrorType(DOMException::AbortError));
	// });
}

//
// Lines
//

// static
File::FileOpResult File::readFileLines(const QString &file, int maxLines, int fromLine, bool trimTrailingNewlines)
{
	if (maxLines < 0)
		return { ErrorType::NoError, QString() };
	// const OpenMode mode = OpenMode(fo.mode).setFlag(O_WRONLY, false).setFlag(O_RDONLY);

	int error;
	QString errStr;
	QFile fh(file);
	if (!fh.exists())
		return { domToCustomError(DOMException::NotFoundError), u"Source file '%1' not found."_s.arg(file) };

	if ((error = File::open_impl(fh, nullptr, FS::O_RDONLY, &errStr)) != ErrorType::NoError)
		return { error, errStr };

	if (fh.size() < 2) {
		fh.close();
		return { ErrorType::NoError, QString() };
	}

	QString ret;

	if (fromLine >= 0)
		ret = readLinesForward(fh, maxLines, fromLine, trimTrailingNewlines);
	else if (fh.seek(fh.size() - 1))
		ret = readLinesBackward(fh, maxLines, fromLine, trimTrailingNewlines);

	if (fh.error() != QFileDevice::NoError) {
		ret = u"Could not read from file '%1': %2"_s.arg(file).arg(fh.errorString());
		error = fileErrorToJsError(fh.error());
	}
	fh.close();
	//qCDebug(lcPlugin) << ret.toUtf8().toHex(':');
	return { error, ret };
}

QString File::readLines(const QString &file, int maxLines, int fromLine, bool trimTrailingNewlines) const
{
	const auto res = readFileLines(file, maxLines, fromLine, trimTrailingNewlines);
	if (res.first == ErrorType::NoError)
		return res.second.toString();
	raiseError(res.first, res.second.toString());
	return QString();
}

void File::readLinesAsyncCb(const QString &file, int maxLines, int fromLine, bool trimTrailingNewlines, QJSValue callback)
{
	// FileOptions fo{ true };
	// if (!parseFileOptions(fo, file, QJSValue(), callback))
	// 	return;
	if (!callback.isCallable()) {
		raiseError(ErrorType::TypeError, u"No callback handler defined for readLinesAsync() method call"_s);
		return;
	}

	QtConcurrent::run(readFileLines, file, maxLines, fromLine, trimTrailingNewlines)
	.then(this, [this, callback](const FileOpResult &result) {
		onResultReady(result.second, callback, result.first);
	});
}


//
// Write
//

// static
File::FileOpResult File::writeFile(const QString &file, const QJSValue &data, const FileOptions &fo)
{
	static const QRegularExpression lfToCrlf { R"((?<!\r)\n)" };

	bool abort = false;
	if (fo.signal) {
		if (fo.signal->aborted())
			return { domToCustomError(DOMException::AbortError), u"File write request aborted before starting."_s };
		fo.signal->connect(fo.signal, &AbortSignal::abort, fo.signal, [&abort](){ abort = true; }, Qt::SingleShotConnection);
	}

	const bool writeEncoded = fo.encoding.has_value() && *fo.encoding != QStringEncoder::Utf8;
	// open in write-only mode and bypass QFile buffer since we write in our own chunks
	const OpenMode mode = OpenMode(fo.mode).setFlag(O_RDONLY, false).setFlag(O_WRONLY).setFlag(FS::O_DIRECT);

	int error;
	QString errStr;
	QFileDevice *fh;
	QSaveFile *sf = nullptr;
	if (fo.safeWrite == SafeWrite::WriteNormal) {
		fh = new QFile(file);
	}
	else {
		fh = sf = new QSaveFile(file);
		sf->setDirectWriteFallback(fo.safeWrite == SafeWrite::WriteSafeFallback);
	}

	if ((error = File::open_impl(*fh, nullptr, mode, &errStr)) != ErrorType::NoError)
		return { error, errStr };

	const int64_t writeSz = fo.chunkSize > 0 ? fo.chunkSize : chunkSize;
	int32_t written = 0;
	QVariant ret;
	QByteArray inBa;

	if (fo.encoding.has_value()) {
		// Write as text
		QString inStr = data.toString();

		if (writeEncoded == fo.writeCr || fo.writeCr != OS_LINE_END_CRLF) {
			// change line endings
			if (fo.writeCr)
				inStr.replace(lfToCrlf, "\r\n"_L1);
			else
				inStr.replace("\r\n"_L1, "\n"_L1);
			fh->setTextModeEnabled(false);
		}

		if (writeEncoded) {
			// Write as encoded text from a string
			QStringEncoder encoder(*fo.encoding, fo.writeBom ? QStringConverter::Flag::WriteBom : QStringConverter::Flag::Default);
			inBa = encoder(inStr);

			if (encoder.hasError()) {
				ret = u"Character encoding error"_s;
				error = domToCustomError(DOMException::EncodingError);
			}
		}
		else {
			// don't re-encode the input
			inBa = inStr.toUtf8();
			// assume UTF-8 BOM if we're not re-encoding
			if (fo.writeBom)
				inBa.push_front((const char*)utf8bom);
		}
	}
	else {
		// Write as bytes
		inBa = data.toVariant(QJSValue::ConvertJSObjects).toByteArray();
	}

	if (!error) {
		int64_t sz, bw;
		const int64_t inLen = inBa.size();
		while (written < inLen && !abort) {
			sz = std::min(writeSz, inLen - written);
			bw = fh->write(inBa.constData() + written, sz);
			if (bw <= 0)
				break;
			written += bw;
		}

		if (abort) {
			ret = u"File write request aborted. Data may have been partially written."_s;
			error = domToCustomError(DOMException::AbortError);
		}
		else if (fh->error() != QFileDevice::NoError) {
			ret = u"Could not write to file '%1': %2"_s.arg(file).arg(fh->errorString());
			error = fileErrorToJsError(fh->error());
		}
		else if (written < inLen) {
			ret = u"Partial write to file '%1': Input length was %2 bytes but wrote %3"_s.arg(file).arg(inLen).arg(written);
			error = domToCustomError(DOMException::DataError);
		}
		else {
			ret = written;
		}
	}

	if (sf) {
		if (error)
			sf->cancelWriting();
		else
			sf->commit();
	}
	else {
		fh->close();
	}
	fh->deleteLater();

	return { error, ret };
}

qint64 File::write(const QString &file, const QJSValue &data, QJSValue options) const
{
	FileOptions fo;
	if (!parseFileOptions(fo, file, options))
		return -1;

	const auto res = writeFile(file, data, fo);
	if (res.first == ErrorType::NoError)
		return res.second.toLongLong();
	raiseError(res.first, res.second.toString());
	return -1;
}

void File::writeAsyncCb(const QString &file, QJSValue data, QJSValue options, QJSValue callback)
{
	FileOptions fo{ true };
	if (!parseFileOptions(fo, file, options, callback))
		return;

	QtConcurrent::run(writeFile, file, std::move(data), fo)
	.then(this, [this, cb = fo.callback](const FileOpResult &result) {
		onResultReady(result.second, cb, result.first);
	});

}

//
// Copy & Rename shared
//

// static
File::FileOpResult File::doOverwriteFileOp(FileOp op, const QString &from, const QString &to, OverwriteMode mode)
{
	static const QString opNames[] { u"copy"_s, u"rename"_s };

	const QString opName = opNames[op];
	QString dest(to.trimmed());

	if (from.trimmed().isEmpty() || dest.isEmpty())
		return { ErrorType::TypeError, u"Source and destination names are required for %1 operation."_s.arg(opName) };

	QFileInfo srcFi(from);
	if (!srcFi.exists())
		return { domToCustomError(DOMException::NotFoundError), u"Source file '%1' not found."_s.arg(from) };

	QFileInfo destFi(dest);
	if (destFi.isDir()) {
		dest.append('/'_L1 + srcFi.fileName());
		destFi.setFile(dest);
	}

	if (!QFileInfo::exists(destFi.absolutePath()))
		return {
			domToCustomError(DOMException::NotFoundError),
			u"Destination directory '%1' not found."_s.arg(destFi.absolutePath())
		};

	if (srcFi == destFi)
		return {
			domToCustomError(DOMException::NotFoundError),
			u"Source and Destination are the same for '%1' and '%2'."_s.arg(srcFi.absoluteFilePath(), destFi.absoluteFilePath())
		};

	QString tmpFn;
	if (QFile::exists(dest)) {
		if (mode == FS::OW_EXCL)
			return { domToCustomError(DOMException::InvalidAccessError), u"Destination file '%1' already exists."_s.arg(to) };

		if (mode == FS::OW_SAFE || mode == FS::OW_SAFE_FB) {
			tmpFn = to + '.'_L1 + QUuid::createUuid().toString(QUuid::WithoutBraces);
			QFile tmpFh (to);
			if (!tmpFh.rename(tmpFn)) {
				// fail if couldn't rename
				if (mode == FS::OW_SAFE)
					return {
						domToCustomError(DOMException::InvalidAccessError),
						u"Destination file '%1' already exists and could rename it for safe overwrite due to error: %2"_s.arg(to, tmpFh.errorString())
					};

				// fall back to deletion mode
				tmpFn.clear();
				mode = FS::OW_DELETE;
			}
		}

		if (mode == FS::OW_DELETE) {
			QFile tmpFh(to);
			if (!tmpFh.remove())
				return {
					domToCustomError(DOMException::InvalidAccessError),
					u"Destination file '%1' already exists and could not delete it due to error: %2"_s.arg(to, tmpFh.errorString())
				};
		}
	}

	FileOpResult ret;
	bool ok;
	QFile fh(from);
	if (op == FileOp::FileCopy)
		ok = fh.copy(dest);
	else /* if (op == FileOp::FileRename) */
		ok = fh.rename(dest);

	if (ok)  {
		ret = { ErrorType::NoError, true };
	}
	else {
		const int code = fh.error();
		if (code != QFileDevice::NoError)
			ret = { fileErrorToJsError(code), fh.errorString() };
		else
			ret = {
			  domToCustomError(DOMException::UnknownError),
			  u"File %1 from '%2' to '%3' failed for an unknown reason."_s.arg(opName, from, to)
			};
	}

	// Remove or rename temp file if we made one
	if (!tmpFn.isEmpty()) {
		QFile tmpFh(tmpFn);
		if (ret.first == ErrorCode::NoError) {
			if (!tmpFh.remove())
				qCWarning(lcDse) << "Could not remove temporary file" << tmpFn << "after copy operation! Error:" << tmpFh.errorString();
		}
		else if (!tmpFh.rename(to)) {
			ret.second = QVariant(
				ret.second.toString().append(u"\nFurthermore, could not rename temporary file '%1' back to original `%2` due to error: %3"_s.arg(tmpFn, to, tmpFh.errorString()))
			);
		}
	}
	return ret;
}

//
// Copy
//

bool File::copy(const QString &from, const QString &to, FS::OverwriteMode mode) const
{
	const auto res = copyFile(from, to, mode);
	if (res.first == ErrorType::NoError)
		return res.second.toBool();
	raiseError(res.first, res.second.toString());
	return false;
}

void File::copyAsyncCb(const QString &from, const QString &to, QJSValue mode_cb, QJSValue callback)
{
	FS::OverwriteMode mode = FS::OW_EXCL;
	if (mode_cb.isNumber())
		mode = FS::OverwriteMode(mode_cb.toNumber());
	else if (mode_cb.isCallable() && callback.isUndefined())
		callback = mode_cb;

	if (!callback.isCallable()) {
		raiseError(ErrorType::TypeError, u"No callback handler defined for async method call"_s);
		return;
	}

	QtConcurrent::run(copyFile, from, to, mode)
	.then(this, [this, callback](const FileOpResult &result) {
		onResultReady(result.second, callback, result.first);
	});
}

//
// Rename
//

bool File::rename(const QString &from, const QString &to, FS::OverwriteMode mode) const
{
	const auto res = renameFile(from, to, mode);
	if (res.first == ErrorType::NoError)
		return res.second.toBool();
	raiseError(res.first, res.second.toString());
	return false;
	// return QFile::copy(from, to);
}

void File::renameAsyncCb(const QString &from, const QString &to, QJSValue mode_cb, QJSValue callback)
{
	FS::OverwriteMode mode = FS::OW_EXCL;
	if (mode_cb.isNumber())
		mode = FS::OverwriteMode(mode_cb.toNumber());
	else if (mode_cb.isCallable() && callback.isUndefined())
		callback = mode_cb;

	if (!callback.isCallable()) {
		raiseError(ErrorType::TypeError, u"No callback handler defined for async method call"_s);
		return;
	}

	QtConcurrent::run(renameFile, from, to, mode)
	.then(this, [this, callback](const FileOpResult &result) {
		onResultReady(result.second, callback, result.first);
	});
}


//
// Remove
//

File::FileOpResult File::removeFile(const QString &file)
{
	if (file.trimmed().isEmpty())
		return { ErrorType::TypeError, u"Source file argument is required for remove operation."_s };

	QFile fh(file);
	if (!fh.exists())
		return { domToCustomError(DOMException::NotFoundError), u"Source file '%1' not found."_s.arg(file) };

	if (fh.remove())
		return { ErrorType::NoError, true };

	const int code = fh.error();
	if (code != QFileDevice::NoError)
		return { fileErrorToJsError(code), fh.errorString() };

	return {
		domToCustomError(DOMException::UnknownError),
		u"Removing file '%1' failed for an unknown reason."_s.arg(file)
	};
}

bool File::remove(const QString &file) const
{
	const auto res = removeFile(file);
	if (res.first == ErrorType::NoError)
		return res.second.toBool();
	raiseError(res.first, res.second.toString());
	return false;
}

void File::removeAsyncCb(const QString &file, QJSValue callback)
{
	if (!callback.isCallable()) {
		raiseError(ErrorType::TypeError, u"No callback handler defined for async method call"_s);
		return;
	}

	QtConcurrent::run(removeFile, file)
	.then(this, [this, callback](const FileOpResult &result) {
		onResultReady(result.second, callback, result.first);
	});
}


//
// Shared
//

// static
int File::open_impl(QFileDevice &fh, QJSEngine *jse, OpenMode mode, QString *err)
{
	if (fh.open(toQfileFlags(mode)))
		return ErrorType::NoError;
	const QString msg = u"Could not open file '%1': %2"_s.arg(fh.fileName(), fh.errorString());
	const int errType = fileErrorToJsError(fh.error());
	if (err)
		*err = msg;
	else
		ScriptEngine::throwError(jse, errType, msg);
	return errType;
}

// static
QByteArray File::fileReadAll(QJSEngine *jse, const QString &file, OpenMode mode)
{
	QFile fh(file);
	if (open_impl(fh, jse, mode) == ErrorType::NoError) {
		const QByteArray ret = fh.readAll();
		fh.close();
		return ret;
	}
	return QByteArray();
}

// static
QString File::readLinesForward(QFile &fh, int maxLines, int fromLine, bool trimTrailing, std::optional<QStringConverter::Encoding> encoding)
{
	if (fh.atEnd() || !fh.isReadable())
		return QString();

	// check encoding
	QStringConverter::Encoding enc;
	if (encoding.has_value())
		enc = encoding.value();
	else
		enc = detectFileEncoding(fh).value_or(QStringConverter::Utf8);

	// Number of trailing bytes to skip after a LF is found; for little endian encoding only
	const uchar extraChars = enc == QStringConverter::Utf16LE ? 1 : enc == QStringConverter::Utf32LE ? 3 : 0;

	// Seek forward
	int count = 0;
	if (fromLine > 0) {
		// Skip lines
		char ch;
		while (!fh.atEnd()) {
			if (fh.read(&ch, 1) != 1 || (ch == '\n' && ++count == fromLine)) {
				if (extraChars)
					fh.seek(fh.pos() + extraChars);
				break;
			}
			else if (extraChars)
				fh.seek(fh.pos() + extraChars);
		}
		if (fh.atEnd())
			return QString();
		count = 0;
	}

	QString ret;
	QString line;
	bool tm = fh.isTextModeEnabled();
	if (tm)
		fh.setTextModeEnabled(false);

	QStringDecoder toUtf16(enc);
	while (!fh.atEnd() && ++count <= maxLines) {
		line = toUtf16(fh.readLine());
		if (extraChars)
			line.append(toUtf16(fh.read(extraChars)));
		line.replace('\r'_L1, ""_L1);
		ret.append(line);
		if (line.isEmpty() || !line.endsWith('\n'_L1))
			break;
	}
	if (tm)
		fh.setTextModeEnabled(true);

	return trimTrailing ? trimTrailingNewlines(ret) : ret;
}

// static
QString File::readLinesBackward(QFile &fh, int maxLines, int fromLine, bool trimTrailing, std::optional<QStringConverter::Encoding> encoding)
{
	auto p = fh.pos();
	if (p < 1 || !fh.isReadable())
		return QString();

	// check encoding
	QStringConverter::Encoding enc;
	if (encoding.has_value())
		enc = encoding.value();
	else
		enc = detectFileEncoding(fh).value_or(QStringConverter::Utf8);

	// Number of trailing bytes to include after a LF is found; for little endian encoding only
	const uchar extraChars = enc == QStringConverter::Utf16LE ? 1 : enc == QStringConverter::Utf32LE ? 3 : 0;

	// Seek backwards
	int count = 0;
	int skipLines = (1 + fromLine) * -1;
	char ch;

	// ignore trailing newlines
	if (trimTrailing) {
		while (p > extraChars && fh.seek(p-extraChars) && fh.peek(&ch, 1) == 1 && (ch == '\n' || ch == '\r'))
			fh.seek(p -= 1 + extraChars);
	}

	//  loop backward through file until count == maxLines or start of file is reached
	while (p > 0) {
		if (fh.peek(&ch, 1) != 1)
			break;
		// is this newline and then did we get enough lines yet?
		if (ch == '\n') {
			// check if this line should be skipped
			if (skipLines) {
				--skipLines;
				p -= extraChars;
				//qDebug() << p << skipLines;
			}
			else if (++count; maxLines && count == maxLines) {
				fh.seek(p + 1 + extraChars);
				break;
			}
		}
		// check the previous character on next iteration.
		fh.seek(--p);
	}
	// qCDebug(lcPlugin) << extraChars << p << skipLines << maxLines << count;
	// Read `count` lines forward from current position
	if (count)
		return readLinesForward(fh, count, 0, trimTrailing, enc);
	return QString();
}

void File::raiseError(int type, const QString &msg, QJSValue handler) const
{
	if (handler.isCallable())
		onResultReady(msg, handler, type);
	else if (m_throwExceptions)
		ScriptEngine::throwError(this, type, msg);
	else
		qCCritical(lcDse) << msg;
}

void File::onResultReady(const QVariant &data, QJSValue callback, int error) const
{
	QJSEngine *jse = qjsEngine(this);
	if (!jse) {
		qCCritical(lcDse) << "Could not return File result w/out a JS Engine instance.";
		return;
	}
	if (error > 0) {
		const QJSValue e = ScriptEngine::newErrorObject(jse, error, data.toString());
		if (callback.isCallable())
			callback.call({ e, QJSValue(QJSValue::NullValue) });
		else if (m_throwExceptions)
			ScriptEngine::throwError(jse, e);
		else
			qCCritical(lcDse) << e.toString();
	}
	else if (callback.isCallable()) {
		callback.call({ QJSValue(QJSValue::NullValue), jse->toScriptValue(data) });
	}
	else {
		ScriptEngine::throwError(jse, ErrorType::ReferenceError, u"No callback handler defined for File process result!"_s);
	}
}


// =========================
//
// FileHandle
//

QString FileHandle::readLines(int maxLines, int fromLine, bool trimTrailingNewlines)
{
	if (maxLines < 0 || !m_file.isOpen() || !m_file.isReadable() || !m_file.size()) {
		ScriptEngine::throwError(this, QJSValue::GenericError,
			u"Could not readLines(%1, %2) on file '%3': File not open/readable, is empty, or maxLines is < 0."_s.arg(maxLines).arg(fromLine).arg(m_file.fileName())
		);
		return QString();
	}
	if (fromLine >= 0)
		return readLinesForward(m_file, maxLines, fromLine, trimTrailingNewlines);
	if (m_file.pos() < 2) {
		ScriptEngine::throwError(this, QJSValue::GenericError,
			u"Could not readLines(%1, %2) on file '%3': Current position is invalid or at start."_s.arg(maxLines).arg(fromLine).arg(m_file.fileName())
		);
		return QString();
	}
	return readLinesBackward(m_file, maxLines, 0, trimTrailingNewlines);
}
