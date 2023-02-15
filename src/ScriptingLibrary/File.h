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

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QQmlEngine>
#include <QObject>
#include <QUrl>

#include "common.h"
#include "FS.h"
//#include "ScriptEngine.h"

//! \file

#ifdef Q_OS_WIN
extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;  // for NTFS permission checking, see QFile docs.
#endif

class ScriptEngine;

#ifndef DOXYGEN
namespace ScriptLib {
#else
#define QByteArray ArrayBuffer
#endif

using namespace FS;

//! \ingroup FileSystem
//! The File class provides access to... files.  Shocking.
//!
//! It has many static functions which are accessed in JS with the `File.` qualifier. These are "atomic" operations, eg. read a whole file at once
//! or check if a file name exists, etc.
//! \note On Windows all file path arguments passed to functions in this class can use the `/` directory separator (recommended) as well as `\`
//! (which needs to be escaped as "\\"" per JS syntax rules).
class File : public QObject
{
	private:
		Q_OBJECT

		explicit File(bool isStatic) : File(nullptr)
		{
			if (isStatic)
				QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
		}

	public:
		static File *instance()
		{
			static File instance(true);
			return &instance;
		}

		explicit File(QObject *p = nullptr) : QObject(p)
		{
			setObjectName("File");
#ifdef Q_OS_WIN
			qt_ntfs_permission_lookup++; // turn NTFS checking on
#endif
		}

		virtual ~File() {
#ifdef Q_OS_WIN
			qt_ntfs_permission_lookup--; // turn NTFS checking off
#endif
			//qCDebug(lcPlugin) << this << "Destroyed";
		}

		// File Actions
		//! \{

		//! Reads a file and returns the contents as a byte array.
		//! Set `mode` to `FS.O_BIN` to read in binary mode. Default is to read as text.
		//! \throws ReferenceError is thrown if file loading fails (file not found/etc) and returns and empty string.
		Q_INVOKABLE QByteArray read(const QString &file, ScriptLib::FS::OpenMode mode = O_TEXT) const { return read_impl(qjsEngine(this), file, mode); }
		//! Reads a file and returns the contents as a byte array.
		//! Set `mode` to 'b' to read in binary mode. Default is to read as text.
		//! \throws ReferenceError is thrown if file loading fails (file not found/etc) and returns and empty string.
		Q_INVOKABLE QByteArray read(const QString &file, const QString &mode) const { return read(file, modeToFlags(mode)); }

		//! Reads a file in text mode and returns the contents as a string.
		//! Set `mode` to 'b' to read in binary mode and return byte array. Default is to read as text and return a string.
		//! \throws ReferenceError is thrown if file loading fails (file not found/etc) and returns and empty string.
		Q_INVOKABLE QString readText(const QString &file) const { return QString(read(file, O_TEXT)); }


		//! Reads up to `maxLines` lines from `file` starting at `fromLine` (default = 0, start of file) and returns the contents as a string. Can search from start or end of file.
		//! \n `maxLines` can be `0` in which case all remaining lines in the file are returned. This is useful when `fromLine != 0` to skip a number of lines
		//! and then return the rest.
		//! \n `fromLine` can be negative, in which case the file will be read from the end, backwards. In this case `-1` means starting at the end of the file,
		//! `-2` means to skip one line ("start at second-to-last line"), and so on.
		//! \n Set `trimTrailingNewlines` to `true` (default) to trim/skip over any empty lines from the end of the file. When searching backwards,
		//! setting `trimTrailingNewlines` to `false` will count every newline from the end, including any potential newline at end of the last line of the file.
		//!
		//! \throws ReferenceError is thrown if file loading fails (file not found/etc) and returns and empty string.
		//! \note Note that if no newlines are found in the file then the full contents will be returned (a potentially expensive operation).
		//! This operation works on all text files regardless of line endings.
		Q_INVOKABLE QString readLines(const QString &file, int maxLines, int fromLine = 0, bool trimTrailingNewlines = true)
		{
			if (maxLines < 0)
				return QString();
			QFile fh(file);
			if (!open_impl(fh, qjsEngine(this), O_RDONLY /*| O_TEXT*/))
				return QString();
			if (fh.size() < 2) {
				fh.close();
				return QString();
			}
			QString ret;
			if (fromLine >= 0) {
				ret = readLinesFromStart(fh, maxLines, fromLine, trimTrailingNewlines);
			}
			else {
				fh.seek(fh.size() - 1);
				ret = readLinesFromEnd(fh, maxLines, fromLine, trimTrailingNewlines);
			}
			fh.close();
			//qCDebug(lcPlugin) << ret.toUtf8().toHex(':');
			return ret;
		}


		//! Writes `data` to `file` and returns the number of bytes written. Returns `-1` on error. `mode` flags can be a combination of `FileLib::OpenModeFlag` enums:
		//!		* `FS.O_BIN` write in binary mode (default is to write as text).
		//!   * `FS.O_APPEND` to append to file (default is to truncate/replace it).
		//!   * `FS.O_EXCL` to fail if the file exists, or `FS.O_NOCREAT` to to fail if the file _doesn't_ already exist (both will return an error if the condition isn't met).
		//!
		//! \throws Error is thrown if the file writing fails for any reason.
		Q_INVOKABLE qint64 write(const QString &file, const QByteArray &data, ScriptLib::FS::OpenMode mode = O_TEXT) const
		{
			qint64 ret;
			QFile fh(file);
			if (fh.open(toQfileFlags(mode.setFlag(O_RDONLY, false).setFlag(O_WRONLY)))) {
				ret = fh.write(data);
				fh.close();
			}
			else {
				ret = -1;
				if (QJSEngine *jse = qjsEngine(this))
					jse->throwError(QJSValue::GenericError, "Could not write to file '" + file + "': " + fh.errorString());
				else
					qCWarning(lcPlugin) << "Could not write file" << file << ":" << fh.errorString();
			}
			return ret;
		}

		//! Writes `data` to `file` and returns the number of bytes written. Returns `-1` on error.
		//! `mode` flags can be a combination of the following (in any order):
		//!		* `b` write in binary mode (default is to write as text).
		//!   * `a` to append to file (default is to truncate/replace it).
		//!   * `x` to fail if the file exists, or `n` to to fail if the file _doesn't_ already exist (both will return an error if the condition isn't met).
		//!
		//! \throws Error is thrown if the file writing fails for any reason.
		Q_INVOKABLE qint64 write(const QString &file, const QByteArray &data, const QString &mode) const { return write(file, data, modeToFlags(mode)); }

		//! Removes the file specified by `file`. Returns `true` if successful; otherwise returns `false`.
		Q_INVOKABLE static bool remove(const QString &file)                    { return QFile::remove(file); }
		//! Renames the `from` file to a file specified in `to`. Returns `true` if successful; otherwise returns `false`. If a file with the name `to` already exists, `rename()` returns `false`.
		Q_INVOKABLE static bool rename(const QString &from, const QString &to) { return QFile::rename(from, to); }
		//! Copies the `from` file to a file specified in `to`. Returns `true` if successful; otherwise returns `false`. If a file with the name `to` already exists, `copy()` returns `false`.
		Q_INVOKABLE static bool copy(const QString &from, const QString &to)   { return QFile::copy(from, to); }
		//! Creates a link named linkName that points to the file fileName. What a link is depends on the underlying filesystem (be it a shortcut on Windows or a symbolic link on Unix). Returns true if successful; otherwise returns false.
		Q_INVOKABLE static bool link(const QString &fileName, const QString &linkName)  { return QFile::link(fileName, linkName); }

		//! \}
		// Checks
		//! \{

		// File info
		//! Returns `tru`e if the file exists; otherwise returns `false`.
		Q_INVOKABLE static bool exists(const QString &file)      { return QFileInfo::exists(file); }
		//! Returns `true` if this object points to a file or to a symbolic link to a file. Returns `false` if the object points to something which isn't a file, such as a directory.
		Q_INVOKABLE static bool isFile(const QString &path)      { return QFileInfo(path).isFile(); }
		//! Returns `true` if this object points to a directory or to a symbolic link to a directory; otherwise returns `false`.
		Q_INVOKABLE static bool isDir(const QString &path)       { return QFileInfo(path).isDir(); }
		//! Returns `true` if the user can read the file; otherwise returns `false`.
		Q_INVOKABLE static bool isReadable(const QString &file)  { return QFileInfo(file).isReadable(); }
		//! Returns `true` if the user can write to the file; otherwise returns `false`.
		Q_INVOKABLE static bool isWritable(const QString &file)  { return QFileInfo(file).isWritable(); }
		//! Returns `true` if the file path name is absolute, otherwise returns `false` if the path is relative
		Q_INVOKABLE static bool isAbs(const QString &file)       { return QFileInfo(file).isAbsolute(); }
		//! Returns `true` if the file is executable; otherwise returns `false`.
		Q_INVOKABLE static bool isExec(const QString &file)      { return QFileInfo(file).isExecutable(); }

		//! \}
		// Path info
		//! \{

		//! Returns the file's path, excluding the file name.
		Q_INVOKABLE static QString path(const QString &file)          { return QFileInfo(file).path(); }
		//! Returns the name of a file (with suffix), excluding the path.
		Q_INVOKABLE static QString name(const QString &file)          { return QFileInfo(file).fileName(); }
		//! Returns the file name, including the path (which may be absolute or relative).
		Q_INVOKABLE static QString filePath(const QString &file)      { return QFileInfo(file).filePath(); }
		//! Returns the base name of the file without the path. The base name consists of all characters in the file up to (but not including) the first '.' character.
		Q_INVOKABLE static QString baseName(const QString &file)      { return QFileInfo(file).baseName(); }
		//! Returns the complete base name of the file without the path. The full base name consists of all characters in the file up to (but not including) the last '.' character.
		Q_INVOKABLE static QString fullBaseName(const QString &file)  { return QFileInfo(file).completeBaseName(); }
		//! Returns the suffix (extension) of the file.	The suffix consists of all characters in the file after (but not including) the last '.'.
		Q_INVOKABLE static QString suffix(const QString &file)        { return QFileInfo(file).suffix(); }
		//! Returns the "full" suffix (extension) of the file. The full suffix consists of all characters in the file after (but not including) the first '.'.
		Q_INVOKABLE static QString fullSuffix(const QString &file)    { return QFileInfo(file).completeSuffix(); }
		//! Returns the file's absolute path, excluding the file name.
		Q_INVOKABLE static QString absPath(const QString &path)       { return QFileInfo(path).absolutePath(); }
		//! Returns the file's absolute path, including the file name (with extension).
		Q_INVOKABLE static QString absFilePath(const QString &file)   { return QFileInfo(file).absoluteFilePath(); }
		//! Returns the file's path canonical path (excluding the file name), i.e. an absolute path without symbolic links or redundant "." or ".." elements.
		Q_INVOKABLE static QString normPath(const QString &path)      { return QFileInfo(path).canonicalPath(); }
		//! Returns the canonical path including the file name, i.e. an absolute path without symbolic links or redundant "." or ".." elements.
		Q_INVOKABLE static QString normFilePath(const QString &file)  { return QFileInfo(file).canonicalFilePath(); }

		//! \}
		// Stat
		//! \{

		//! Returns the file size in bytes. If the file does not exist or cannot be fetched, 0 is returned.
		Q_INVOKABLE static quint32 size(const QString &file)        { return (quint32)QFileInfo(file).size(); }
		//! Returns the date and time when the file was created / born.	If the file birth time is not available, this function returns an invalid Date object.
		Q_INVOKABLE static QDateTime btime(const QString &file)   { return QFileInfo(file).birthTime(); }
		//! Returns the date and local time when the file was last modified. If the file is not available, this function returns an invalid Date object.
		Q_INVOKABLE static QDateTime mtime(const QString &file)  { return QFileInfo(file).lastModified(); }
		//! Returns the date and local time when the file was last accessed (read). If the file is not available, this function returns an invalid Date object.
		Q_INVOKABLE static QDateTime atime(const QString &file)  { return QFileInfo(file).lastRead(); }
		//! Returns the date and time when the file metadata (status, eg. permissions) was changed. If the file is not available, this function returns an invalid Date object.
		Q_INVOKABLE static QDateTime ctime(const QString &file)  { return QFileInfo(file).metadataChangeTime(); }
		//! Returns the complete OR-ed together combination of `FS.Permissions` for the file.
		Q_INVOKABLE static ScriptLib::FS::Permissions permissions(const QString &file)  { return (Permissions)(quint16)QFileInfo(file).permissions(); }
		//! Sets the permissions for `file` to the `FS.Permissions` flags specified in `p`. Returns `true` if successful, or `false` if the permissions cannot be modified.
		Q_INVOKABLE bool setPermissions(const QString &file, ScriptLib::FS::Permissions p) { return QFile(file).setPermissions((QFileDevice::Permissions)(int)p); }

		//! \}

	protected:
		friend class ::ScriptEngine;

		static const QString STR_INVALID_OP() {
			static const QString str = tr("Invalid operation on static instance of %1").arg(File::staticMetaObject.className());
			return str;
		}

		static bool open_impl(QFile &fh, QJSEngine *jse, ScriptLib::FS::OpenMode mode)
		{
			if (fh.open(toQfileFlags(mode)))
				return true;
			if (jse)
				jse->throwError(QJSValue::ReferenceError, tr("Could not read file '%1': %2").arg(fh.fileName(), fh.errorString()));
			else
				qCWarning(lcPlugin) << "Could not read file" << fh.fileName() << ":" << fh.errorString();
			return false;
		}

		static QByteArray read_impl(QJSEngine *jse, const QString &file, ScriptLib::FS::OpenMode mode = O_TEXT)
		{
			QFile fh(file);
			if (!open_impl(fh, jse, mode.setFlag(O_WRONLY, false).setFlag(O_RDONLY)))
				return QByteArray();
			QByteArray ret = fh.readAll();
			fh.close();
			return ret;
		}

		// Truncates str to exclude trailing newline(s) including any CRs.
		static QString &trimTrailingNewlines(QString &str)
		{
			for (int n = str.size() - 1; n > -1; --n) {
				const QChar &ch = str.at(n);
				if (ch != '\n' && ch != '\r') {
					str.truncate(n+1);
					break;
				}
			}
			return str;
		}

		static QString readLinesFromStart(QFile &fh, int maxLines, int fromLine, bool trimTrailing)
		{
			// Seek forward
			int count = 0;
			if (fromLine > 0) {
				// Skip lines
				const qint64 len = fh.size() - fh.pos();
				int p = 0;
				char ch;
				for ( ; p < len; ++p)
					if (fh.read(&ch, 1) != 1 || (ch == '\n' && ++count == fromLine))
						break;
				if (p >= len)
					return QString();
				count = 0;
			}
			QString ret;
			// If !maxLines then returns the rest of the file from current position, otherwise add line-by-line.
			if (maxLines) {
				QTextStream strm(&ret, QIODevice::WriteOnly);
				while (count < maxLines && !fh.atEnd()) {
					const QString l = fh.readLine();
					strm << l;
					// end or error is signaled by empty return or lack of nl
					if (l.isEmpty() || !l.endsWith('\n'))
						break;
					++count;
				}
			}
			else {
				ret = fh.readAll();
			}
			return trimTrailing ? trimTrailingNewlines(ret) : ret;
		}

		static QString readLinesFromEnd(QFile &fh, int maxLines, int fromLine, bool trimTrailing)
		{
			// Seek backwards
			int count = 0;
			int skipLines = (1 + fromLine) * -1;
			qint64 p = fh.pos();
			if (p < 1)
				return QString();
			char ch;

			// ignore trailing newlines
			if (trimTrailing) {
				while (p >= 0 && fh.peek(&ch, 1) == 1 && (ch == '\n' || ch == '\r'))
					fh.seek(--p);
			}

			qint64 endPos = p;
			//  loop backward through file until count == maxLines or start of file is reached
			while (p >= 0) {
				if (fh.peek(&ch, 1) != 1)
					break;
				// is this newline and then did we get enough lines yet?
				if (ch == '\n') {
					// check for a CR before of this NL
					quint8 skipChars = 1;
					if (fh.seek(p-1) && fh.peek(&ch, 1) == 1) {
						if (ch == '\r') {
							--p;
							skipChars = 2;
						}
						else {
							fh.seek(p);
						}
					}
					// check if this line should be skipped
					if (skipLines) {
						--skipLines;
						endPos = p - 1;
						//qDebug() << endPos << p << skipLines;
					}
					else if (maxLines && ++count == maxLines) {
						fh.seek(p += skipChars);  // don't include the last newline we found
						--p;
						break;
					}
				}
				// check the previous character on next iteration.
				fh.seek(--p);
			}
//			if (trimTrailing && endPos) {
//				fh.seek(endPos);
//				while (endPos >= 0 && fh.peek(&ch, 1) == 1 && (ch == '\n' || ch == '\r'))
//					fh.seek(--endPos);
//				fh.seek(p);
//			}

			//qDebug() << endPos << p << skipLines << maxLines << count;
			// Read all bytes from current position to end position
			if (!skipLines && endPos > p)
				return fh.read(endPos - p);
			return QString();
		}

		static OpenMode modeToFlags(const QString &mode)
		{
			OpenMode f;
			bool hasA = mode.contains('a');
			if (mode.contains('+'))
				f |= O_RDWR;
			else if (mode.contains('r'))
				f |= O_RDONLY;
			if (hasA || f.testFlag(O_WRONLY) || mode.contains('w'))
				f.setFlag(O_WRONLY).setFlag(hasA ? O_APPEND : O_TRUNC);
			if (!mode.contains('b'))
				f |= O_TEXT;
			if (mode.contains('x'))
				f |= O_EXCL;
			else if (mode.contains('n'))
				f |= O_NOCREAT;
			if (mode.contains('s'))
				f |= O_DIRECT;
			return f;
		}

};

//! \ingroup FileSystem
//! `FileHandle` provides an object instance for working with a file in multiple steps (stat, open, read/write bytes, etc). In JS it is
//! instantiated in the usual object creation method with "new" keyword, eg. `var fh = new FileHandle("file.txt");` (the file name
//! can also be set or changed later using the `fileName` property).
class FileHandle : private File
{
	private:
		Q_OBJECT
		QFile m_file;
		QFileInfo m_fi;

	public:
		//! Creates a new object instance with given `fileName`. The file name could be empty and specified later by setting the `fileName` property.
		//! The name can have no path, a relative path, or an absolute path. Relative paths are based on current application directory (see `Dir.cwd()`).
		//! \n **Note** that the directory separator "/" works for all operating systems. On Windows backslash ("\") can also be used but must be escaped as "\\".
		Q_INVOKABLE explicit FileHandle(const QString &fileName = QString()) : File(nullptr)
		{
			setObjectName("FileHandle");
			if (!fileName.isEmpty())
				setFileName(fileName);
		}

		~FileHandle() {
			if (m_file.isOpen())
				m_file.close();
			//qCDebug(lcPlugin) << this << "Destroyed";
		}

		//! \{
		//! Name of current file, as set in constructor or using this property. This property can also set the name of the file  (eg. `fh.fileName = "myfile.txt"`).
		//! The name can have no path, a relative path, or an absolute path. Relative paths are based on current application directory (see `Dir.cwd()`).
		//! \note The directory separator "/" works for all operating systems. On Windows the backslash ("\") can also be used but must be escaped in string literals as "\\\\".
		//! \warning Do not set this property if the file has already been opened.
		Q_PROPERTY(QString fileName READ fileName WRITE setFileName)
		//! Returns the file error status code. For example, if `open()` returns `false`, or a read/write operation returns `-1`,
		//! this function can be called to find out the reason why the operation failed. \sa errorString, unsetError()
		Q_PROPERTY(ScriptLib::FS::FileError error READ error)
		//! Returns a human-readable description of the last device error that occurred.
		Q_PROPERTY(QString errorString READ errorString)
		//! \}
		//! \{
		//! Returns `true` if the current file exists, `false` otherwise.
		Q_PROPERTY(bool exists READ exists)
		//! Returns `true` if data can be read from the current file, `false` otherwise. This checks that the file has been opened with `FS.O_RDONLY` flag set.
		Q_PROPERTY(bool isReadable READ isReadable)
		//! Returns `true` if data can be written to current the file, `false` otherwise. This checks that the file has been opened with `FS.O_WRONLY` flag set.
		Q_PROPERTY(bool isWritable READ isWritable)
		//! Returns `true` if the file has been successfully opened, `false` otherwise.
		Q_PROPERTY(bool isOpen READ isOpen)
		//! Returns the current `FS.OpenMode` of the file handle, or `FS.O_NOTOPEN` if the file hasn't been opened.
		Q_PROPERTY(ScriptLib::FS::OpenMode openMode READ openMode)
		//! Size of the current file, in bytes. Same as `length`.
		Q_PROPERTY(qint64 size READ size)
		//! Size of the current file, in bytes. Same as `size`.
		Q_PROPERTY(qint64 length READ length)
		//! Returns the complete OR-ed together combination of `FS.Permissions` for the file. \sa setPermissions()
		Q_PROPERTY(ScriptLib::FS::Permissions permissions READ permissions)
		//! Returns the date and local time when the current file was last accessed. If the file is not available, this function returns an invalid Date object. Same as `fileTime(S_ATIME)`.
		Q_PROPERTY(QDateTime atime READ atime)
		//! Returns the date and time when the current file was created / born.	If the file birth time is not available, this function returns an invalid Date object. Same as `fileTime(S_BTIME)`.
		Q_PROPERTY(QDateTime btime READ btime)
		//! Returns the date and local time when the current file was last modified. If the file is not available, this function returns an invalid Date object. Same as `fileTime(S_MTIME)`.
		Q_PROPERTY(QDateTime mtime READ mtime)
		//! \}
		//! \{
		//! Returns `true` if current position in file is at the end, `false` otherwise. Same as `if (fh.pos == fh.size - 1)`;
		Q_PROPERTY(bool atEnd READ atEnd)
		//! Returns the position that data is written to or read from.
		Q_PROPERTY(qint64 pos READ pos)
		//! Returns the number of bytes that are available for reading from current position in file.
		Q_PROPERTY(qint64 bytesAvailable READ bytesAvailable)
		//! For files opened in buffered mode (default), this function returns the number of bytes waiting to be written. For unbuffered writes, this function returns 0.
		Q_PROPERTY(qint64 bytesToWrite READ bytesToWrite)
		//! \}
		//! \{
		//! Returns `true` if the current file path name is absolute, otherwise returns `false` if the path is relative
		Q_PROPERTY(bool isAbs READ isAbs)
		//! Returns `true` if the current file is executable; otherwise returns `false`.
		Q_PROPERTY(bool isExec READ isExec)
		//! Returns the current file's path, excluding the file name.
		Q_PROPERTY(QString path READ path)
		//! Returns the name of current file (with suffix), excluding the path.
		Q_PROPERTY(QString name READ name)
		//! Returns the current file name, including the path (which may be absolute or relative).
		Q_PROPERTY(QString filePath READ filePath)
		//! Returns the base name of the current file without the path. The base name consists of all characters in the file up to (but not including) the first '.' character.
		Q_PROPERTY(QString baseName READ baseName)
		//! Returns the complete base name of the current file without the path. The full base name consists of all characters in the file up to (but not including) the last '.' character.
		Q_PROPERTY(QString fullBaseName READ fullBaseName)
		//! Returns the suffix (extension) of the current file.	The suffix consists of all characters in the file after (but not including) the last '.'.
		Q_PROPERTY(QString suffix READ suffix)
		//! Returns the "full" suffix (extension) of the current file. The full suffix consists of all characters in the file after (but not including) the first '.'.
		Q_PROPERTY(QString fullSuffix READ fullSuffix)
		//! Returns the current file's absolute path, excluding the file name.
		Q_PROPERTY(QString absPath READ absPath)
		//! Returns the current file's absolute path, including the file name (with extension).
		Q_PROPERTY(QString absFilePath READ absFilePath)
		//! Returns the current file's path canonical path (excluding the file name), i.e. an absolute path without symbolic links or redundant "." or ".." elements.
		Q_PROPERTY(QString normPath READ normPath)
		//! Returns the canonical path including the current file name, i.e. an absolute path without symbolic links or redundant "." or ".." elements.
		Q_PROPERTY(QString normFilePath READ normFilePath)
		//! \}

		QString fileName() const { return m_file.fileName(); }
		void setFileName(const QString &name) { m_file.setFileName(name); m_fi = QFileInfo(name);}
		QString errorString() const { return m_file.errorString(); }
		ScriptLib::FS::FileError error() const { return (FileError)m_file.error(); }

		bool exists()      const { return m_file.exists(); }
		bool isReadable()  const { return m_file.isReadable(); }
		bool isWritable()  const { return m_file.isWritable(); }
		bool isOpen()      const { return m_file.isOpen(); }
		bool atEnd()       const { return m_file.atEnd(); }
		qint64 size()      const { return m_file.size(); }
		qint64 length()    const { return m_file.size(); }
		qint64 pos()       const { return m_file.pos(); }
		qint64 bytesAvailable() const { return m_file.bytesAvailable(); }
		qint64 bytesToWrite()   const { return m_file.bytesToWrite(); }

		QDateTime btime() const { return fileTime(S_BTIME); }
		QDateTime mtime() const { return fileTime(S_MTIME); }
		QDateTime atime() const { return fileTime(S_ATIME); }

		ScriptLib::FS::OpenMode openMode() const { return OpenMode((quint8)m_file.openMode()); }
		ScriptLib::FS::Permissions permissions()  const { return (Permissions)(quint16)m_file.permissions(); }

		//! \{

		//! Sets the permissions for the file to the `FS.Permissions` flags specified in `p`. Returns `true` if successful, or `false` if the permissions cannot be modified.
		Q_INVOKABLE bool setPermissions(ScriptLib::FS::Permissions p) { return m_file.setPermissions((QFileDevice::Permissions)(int)p); }
		//! Sets the file's error to `FS.NoError`. \sa error, errorString
		Q_INVOKABLE void unsetError() { m_file.unsetError(); }

		//! Returns the file time specified by `time`. If the time cannot be determined this function returns an invalid `Date` object. \sa btime, mtime, atime, FS.FileTime
		Q_INVOKABLE QDateTime fileTime(ScriptLib::FS::FileTime time) const { return m_file.fileTime((QFileDevice::FileTime)time); }
		//! Sets the file time specified by `fileTime` to `newDate` on the current file, returning `true` if successful; otherwise returns `false`. \n **Note:** The file must be open to use this function.
		Q_INVOKABLE bool setFileTime(const QDateTime &newDate, ScriptLib::FS::FileTime fileTime) { return m_file.setFileTime(newDate, (QFileDevice::FileTime)fileTime); }

		//! \}
		//! \{

		//! Copies the current (\ref fileName) file to a file called `newName`. Returns `true` if successful; otherwise returns `false`.
		//! Note that if a file with the name `newName` already exists, `copy()` returns `false`. The source file is closed before it is copied.
		Q_INVOKABLE bool copy(const QString &newName)    { return m_file.copy(newName); }
		//! Creates a link named `linkName` that points to the file currently specified by \ref fileName. What a link is depends on the underlying file system.
		//! This function will not overwrite an already existing entity in the file system; in this case, `link()` will return false and set `error` to return `RenameError`.
		Q_INVOKABLE bool link(const QString &linkName)   { return m_file.link(linkName); }
		//! Copies the current (\ref fileName) file to a file called `newName`. Returns `true` if successful; otherwise returns `false`.
		//! Note that if a file with the name `newName` already exists, `rename()` returns `false`. The source file is closed before it is copied.
		Q_INVOKABLE bool rename(const QString &newName)  { return m_file.rename(newName); }
		//! Removes the file specified by \ref fileName. Returns `true` if successful; otherwise returns `false`. The file is closed before it is removed.
		Q_INVOKABLE bool remove() { return m_file.remove(); }
		//! Sets the current file's size (in bytes) to `newSize`. Returns `true` if the resize succeeds; `false` otherwise. If `newSize` is larger than the file currently is,
		//! the new bytes will be set to `0`; if it is smaller, the file is simply truncated
		Q_INVOKABLE bool resize(qint64 newSize)              { return m_file.resize(newSize); }

		//! \}
		//! \{

		//! Opens the current file using `FS.OpenMode` `mode` flags, returning `true` if successful; otherwise `false`.
		Q_INVOKABLE bool open(ScriptLib::FS::OpenMode mode)  { return m_file.open(toQfileFlags(mode)); }
		//! Opens the current file using `mode` text flag(s), returning `true` if successful; otherwise `false`. See `FS.OpenMode` documentation for text equivalents.
		Q_INVOKABLE bool open(const QString &mode)           { return open(modeToFlags(mode)); }
		//! Flushes and closes the current file if it is open and resets `error()` status. Does nothing if the file is not open. Errors from flush are ignored.
		Q_INVOKABLE void close()                             { m_file.close(); }

		//! Seeks to the start of the file. Returns `true` on success, `false` otherwise (eg. file is not open).
		Q_INVOKABLE bool reset()                             { return m_file.reset(); }
		//! Flush any buffered data waiting to be written out to the file. Returns `true` on success, `false` otherwise (eg. file is not open in write mode).
		Q_INVOKABLE bool flush()                             { return m_file.flush(); }
		//! Sets the current position to `pos`, returning `true` on success, or `false` if an error occurred.
		//! If the position is beyond the end of a file, then `seek()` will not immediately extend the file. If a write is performed at this position, then the file will be extended.
		Q_INVOKABLE bool seek(qint64 pos)                    { return m_file.seek(pos); }
		//! Writes the content of `data` to the file at the current position. Returns the number of bytes that were actually written, or `-1` if an error occurred.
		Q_INVOKABLE qint64 write(const QByteArray &data)     { return m_file.write(data); }

		//! Returns `maxSize` bytes from current position in file without moving the position. If `maxSize` would read past the end, then returns the entire contents.
		Q_INVOKABLE QByteArray peek(qint64 maxSize) { return m_file.peek(maxSize); }
		//! Returns `maxSize` bytes as a byte array from current position in file and advances the position. If `maxSize` would read past the end, then returns the entire contents.
		Q_INVOKABLE QByteArray read(qint64 maxSize) { return m_file.read(maxSize); }
		//! Returns `maxSize` bytes from current position in file and advances the position. If `maxSize` would read past the end, then returns the entire contents.
		//! Result is returned as a string, regardless of the file's `OpenMode`.
		Q_INVOKABLE QString readText(qint64 maxSize) { return QString(m_file.read(maxSize)); }
		//! Returns all contents of the file from the current position though the end, as a byte array.
		Q_INVOKABLE QByteArray readAll() { return m_file.readAll(); }
		//! Returns all contents of the file from the current position though the end. Result is returned as a string, regardless of the file's `OpenMode`.
		Q_INVOKABLE QString readAllText() { return QString(m_file.readAll()); }
		//! Tries to read one line from current position in file and advances the position to the next line (if any). The trailing newline is included in the result.
		//! \n If `maxSize` is > 0, then at most `maxSize` or up to the first newline encountered. The newline character is included unless `maxSize` is exceeded.
		//! \n If `maxSize` is `0` (default) then it will return a whole line regardless of size, or the rest of the file if no newline is found.
		//! \n Result is returned as a string, regardless of the file's `OpenMode`.
		Q_INVOKABLE QString readLine(qint64 maxSize = 0) { return QString(m_file.readLine(maxSize)); }
		//! Reads up to `maxLines` lines from current position in file and returns the contents as a string. The file position is advanced during the read.
		//! \n `fromLine` can be used to skip a number of lines; the default is zero, meaning to start reading from the current position of the file handle.
		//! `fromLine` can be negative, in which case the file will be read backwards from current position. In this case `-1` means starting at the current position,
		//! `-2` means to skip one line ("start at second-to-last line"), and so on.
		//! \n Set `trimTrailingNewlines` to `true` (default) to skip over any empty lines from the end of the file. When searching backwards,
		//! setting `trimTrailingNewlines` to `false` will count every newline from the end, including any potential newline at end of the last line of the file.
		//! \n This operation works on all text files regardless of line endings.
		//! \throws Error is thrown if the file reading fails for any reason.
		//! \note - The file should be opened in binary (not text) mode, otherwise results are unpredictable depending on line ending type.
		//! \note \n - If no newlines are found in the file then the full contents will be returned (a potentially expensive operation).
		Q_INVOKABLE QString readLines(int maxLines, int fromLine = 0, bool trimTrailingNewlines = true)
		{
			if (maxLines < 0 || !m_file.isOpen() || !m_file.isReadable() || !m_file.size()) {
				if (QJSEngine *jse = qjsEngine(this))
					jse->throwError(QJSValue::GenericError, tr("Could not readLines(%1, %2) on file '%3': File not open/readable, is empty, or maxLines is < 0.").arg(maxLines).arg(fromLine).arg(m_file.fileName()));
				return QString();
			}
			if (fromLine >= 0)
				return readLinesFromStart(m_file, maxLines, fromLine, trimTrailingNewlines);
			if (m_file.pos() < 2) {
				if (QJSEngine *jse = qjsEngine(this))
					jse->throwError(QJSValue::GenericError, tr("Could not readLines(%1, %2) on file '%3': Current position is invalid or at start.").arg(maxLines).arg(fromLine).arg(m_file.fileName()));
				return QString();
			}
			return readLinesFromEnd(m_file, maxLines, 0, trimTrailingNewlines);
		}

		//! \}

		// File Info for current instance

		bool isAbs()           const { return m_fi.isAbsolute(); }
		bool isExec()          const { return m_fi.isExecutable(); }
		QString path()         const { return m_fi.path(); }
		QString name()         const { return m_fi.fileName(); }
		QString filePath()     const { return m_fi.filePath(); }
		QString baseName()     const { return m_fi.baseName(); }
		QString fullBaseName() const { return m_fi.completeBaseName(); }
		QString suffix()       const { return m_fi.suffix(); }
		QString fullSuffix()   const { return m_fi.completeSuffix(); }
		QString absPath()      const { return m_fi.absolutePath(); }
		QString absFilePath()  const { return m_fi.absoluteFilePath(); }
		QString normPath()     const { return m_fi.canonicalPath(); }
		QString normFilePath() const { return m_fi.canonicalFilePath(); }
};

#ifndef DOXYGEN
}  // ScriptLib
#endif

Q_DECLARE_METATYPE(ScriptLib::File *)
//Q_DECLARE_METATYPE(ScriptLib::FileHandle *)
