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

// #include <QDateTime>
// #include <QDir>
#include <QFileInfo>
// #include <QQmlEngine>
#include <QObject>
// #include <QUrl>

// #include "common.h"
#include "FS.h"
//#include "ScriptEngine.h"

//! \file


#ifndef DOXYGEN
namespace ScriptLib {
#endif

using namespace FS;

//! \ingroup FileSystem
//! `FileInfo` provides read-only information about a file system object (file or directory).
//! This type cannot be created on its own. You can get an instance via `Dir.info()`, `File.info()`, `FileHandle.info()`, or a directory listing with `Dir.infoList()` functions.
class FileInfo : public QFileInfo
{
	private:
		Q_GADGET

	public:
		using QFileInfo::QFileInfo;
		FileInfo(const QFileInfo &fileinfo) : QFileInfo(fileinfo) {}

		//! \{
		//! Returns the name of current file (with suffix), excluding the path.
		Q_PROPERTY(QString fileName READ fileName WRITE setFile)
		//! Returns the name of current file (with suffix), excluding the path. Same as `fileName`.
		Q_PROPERTY(QString name READ fileName)
		//! Returns the current file's path, excluding the file name.
		Q_PROPERTY(QString path READ path)
		//! Returns the current file name, including the path (which may be absolute or relative).
		Q_PROPERTY(QString filePath READ filePath)
		//! Returns the base name of the current file without the path. The base name consists of all characters in the file up to (but not including) the first '.' character.
		Q_PROPERTY(QString baseName READ baseName)
		//! Returns the complete base name of the current file without the path. The full base name consists of all characters in the file up to (but not including) the last '.' character.
		Q_PROPERTY(QString fullBaseName READ completeBaseName)
		//! Returns the suffix (extension) of the current file.	The suffix consists of all characters in the file after (but not including) the last '.'.
		Q_PROPERTY(QString suffix READ suffix)
		//! Returns the "full" suffix (extension) of the current file. The full suffix consists of all characters in the file after (but not including) the first '.'.
		Q_PROPERTY(QString fullSuffix READ completeSuffix)
		//! Returns the current file's absolute path, excluding the file name.
		Q_PROPERTY(QString absPath READ absolutePath)
		//! Returns the current file's absolute path, including the file name (with extension).
		Q_PROPERTY(QString absFilePath READ absoluteFilePath)
		//! Returns the current file's path canonical path (excluding the file name), i.e. an absolute path without symbolic links or redundant "." or ".." elements.
		Q_PROPERTY(QString normPath READ canonicalPath)
		//! Returns the canonical path including the current file name, i.e. an absolute path without symbolic links or redundant "." or ".." elements.
		Q_PROPERTY(QString normFilePath READ canonicalFilePath)
		//! Returns the absolute path to the directory an NTFS junction points to, or an empty string if the object is not an NTFS junction.
		Q_PROPERTY(QString junctionTarget READ junctionTarget)
		//! Returns the absolute path to the file or directory a symbolic link points to, or an empty string if the object isn't a symbolic link.
		Q_PROPERTY(QString linkTarget READ symLinkTarget)
		//! \}
		//! \{
		//! Returns `true` if the current file path name is absolute, otherwise returns `false` if the path is relative
		Q_PROPERTY(bool isAbs READ isAbsolute)
		//! Returns `true` if this object points to a directory or to a symbolic link to a directory. Returns `false` if the object points to something that is not a directory (such as a file) or that does not exist.
		Q_PROPERTY(bool isDir READ isDir)
		//! Returns `true` if the current file is executable; otherwise returns `false`.
		Q_PROPERTY(bool isExec READ isExecutable)
		//! Returns `true` if this object points to a file or to a symbolic link to a file. Returns `false` if the object points to something that is not a file (such as a directory) or that does not exist.
		Q_PROPERTY(bool isFile READ isFile)
		//! Returns `true` if the file system entry this `FileInfo` refers to is `hidden'; otherwise returns `false`.
		Q_PROPERTY(bool isHidden READ isHidden)
		//! Returns `true` if the object points to a junction; otherwise returns `false`. Junctions only exist on Windows NTFS file systems.
		Q_PROPERTY(bool isJunction READ isJunction)
		//! Returns `true` if the user can read the file system entry this `FileInfo` refers to; otherwise returns `false`.
		Q_PROPERTY(bool isReadable READ isReadable)
		//! Returns `true` if the file system entry's path is relative; otherwise returns `false`.
		Q_PROPERTY(bool isRelative READ isRelative)
		//! Returns `true` if the object points to a directory or to a symbolic link to a directory, and that directory is the root directory; otherwise returns `false`.
		Q_PROPERTY(bool isRoot READ isRoot)
		//! Returns `true` if the object points to a shortcut; otherwise returns `false`. Shortcuts only exist on Windows.
		Q_PROPERTY(bool isShortcut READ isShortcut)
		//! Returns `true` if the object points to a symbolic link; otherwise returns `false`. Symlinks exist on all Unix-like operating systems and Windows NTFS file systems.
		Q_PROPERTY(bool isSymLink READ isSymbolicLink)
		//! Returns `true` if the user can write to the file system entry, `false` otherwise.
		Q_PROPERTY(bool isWritable READ isWritable)
		//! \}
		//! \{
		//! Returns `true` if the current file system entry this `FileInfo` refers to exists, `false` otherwise.
		Q_PROPERTY(bool exists READ exists)
		//! Size of the current file, in bytes.
		Q_PROPERTY(qint64 size READ size)
		//! Returns the complete OR-ed together combination of `FS.Permissions` for the file. \sa setPermissions()
		Q_PROPERTY(FS::Permissions permissions READ permissions)
		//! Returns the date and local time when the current file was last accessed. If the time cannot be determined, returns an invalid Date object. Same as `fileTime(S_ATIME)`.
		Q_PROPERTY(QDateTime atime READ atime)
		//! Returns the date and time when the current file was created / born.	If the time cannot be determined, returns an invalid Date object. Same as `fileTime(S_BTIME)`.
		Q_PROPERTY(QDateTime btime READ btime)
		//! Returns the date and time when the file's metadata was last changed.	If the time cannot be determined, returns an invalid Date object. Same as `fileTime(S_CTIME)`.
		Q_PROPERTY(QDateTime ctime READ ctime)
		//! Returns the date and local time when the current file was last modified. If the time cannot be determined, returns an invalid Date object. Same as `fileTime(S_MTIME)`.
		Q_PROPERTY(QDateTime mtime READ mtime)
		//! \}

		//! \{

		//! Returns the file time specified by `time`. If the time cannot be determined this function returns an invalid `Date` object. \sa atime, btime, ctime, mtime, FS.FileTime
		Q_INVOKABLE QDateTime fileTime(FS::FileTime time) const { return QFileInfo::fileTime((QFileDevice::FileTime)time); }

		//! \}

		QDateTime atime() const { return fileTime(S_ATIME); }
		QDateTime btime() const { return fileTime(S_BTIME); }
		QDateTime ctime() const { return fileTime(S_CTIME); }
		QDateTime mtime() const { return fileTime(S_MTIME); }

		FS::Permissions permissions()  const { return Permissions((quint16)QFileInfo::permissions()); }
};

#ifndef DOXYGEN
}  // ScriptLib
#endif

Q_DECLARE_METATYPE(ScriptLib::FileInfo *)
