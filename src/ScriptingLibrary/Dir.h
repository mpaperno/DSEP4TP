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

//#include "common.h"

//! \file

#ifndef DOXYGEN
namespace ScriptLib {
#endif

//! \ingroup FileSystem
//! The `Dir` class provides static functions for directory operations, which are accessed in JS with the `Dir.` qualifier.
//! These are "atomic" operations such as getting information about a path, manipulating directories and so forth.
class Dir : public QObject
{
		Q_OBJECT
		explicit Dir(bool isStatic) : Dir()
		{
			if (isStatic)
				QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
		}

	public:
		using QObject::QObject;

		static Dir *instance()
		{
			static Dir instance(true);
			return &instance;
		}

		explicit Dir(QObject *p = nullptr) : QObject(p)
		{
			setObjectName("Dir");
		}

		// Dir helpers
		//! \{

		//! Creates the directory path `path`. The function will create all parent directories necessary to create the directory.
		//! Returns `true` if successful; otherwise returns `false`. If the path already exists when this function is called, it will return `true`.
		Q_INVOKABLE static bool mkpath(const QString &path)  { return QDir().mkpath(path); }
		//! Creates a sub-directory called `dirName`. Returns `true` on success; otherwise returns `false`.
		//! If the directory already exists when this function is called, it will return `false`.
		Q_INVOKABLE static bool mkdir(const QString &dirName)   { return QDir().mkdir(dirName);  }
		//! Removes the directory path `path`. The function will remove all parent directories in `path`, provided that they are empty. This is the opposite of `mkpath()`.
		//! Returns `true` if successful; otherwise returns `false`.
		Q_INVOKABLE static bool rmpath(const QString &path)  { return QDir().rmpath(path); }
		//! This function has two modes of operation:
		//! * If `recurse` = `false` (default): Removes the directory specified by `dirName`. The directory must be empty for `rmdir()` to succeed.
		//! * If `recurse` = `true`: Removes the directory specified by `dirName`, **including all its contents**. If a file or directory cannot be removed it keeps going and attempts to
		//!   delete as many files and sub-directories as possible, then returns `false`. If the directory was already removed, the method returns `true` (expected result already reached).
		//! Returns `true` if successful; otherwise returns `false`, except as noted above.
		Q_INVOKABLE static bool rmdir(const QString &dirName, bool recurse = false)  { return recurse ? QDir(dirName).removeRecursively() : QDir().rmdir(dirName);  }

		//! \}
		// Dir info
		//! \{

		//! Returns `true` if `path` exists in the file system, `false` otherwise.
		Q_INVOKABLE static bool exists(const QString &path)     { return QDir(path).exists(); }
		//! Returns `true` if `path` is absolute (that is, from a root directory), `false` otherwise (if it relative).
		Q_INVOKABLE static bool isAbs(const QString &path)      { return QDir::isAbsolutePath(path); }

		//! \}
		// Paths
		//! \{

		//! Returns the absolute path of the application's current directory. The current directory is the directory at which this application was started at by the parent process.
		Q_INVOKABLE static QString cwd()   { return QDir::currentPath(); }
		//! Returns the absolute path of the user's home directory. (Under Windows this function will return the directory of the current user's profile, eg `C:/Users/Username`)
		//! Under non-Windows operating systems the HOME environment variable is used if it exists, otherwise the path returned by the rootPath().
		Q_INVOKABLE static QString home()  { return QDir::homePath(); }
		//! Returns the absolute canonical path of the system's temporary directory. On Unix/Linux systems this is the path in the TMPDIR environment variable or /tmp if TMPDIR is not defined.
		//! On Windows this is usually the path in the TEMP or TMP environment variable. The path returned by this method doesn't end with a directory separator unless it is the root directory (of a drive).
		Q_INVOKABLE static QString temp()  { return QDir::tempPath(); }
		//! Returns the absolute path of the root directory. For Windows file systems this normally returns the boot drive letter (typically "c:/"). For Unix/Mac operating systems this returns "/".
		Q_INVOKABLE static QString root()  { return QDir::rootPath(); }

		//! Returns the native directory separator: "/" under Unix and "\" under Windows. Note: You do not need to use this function to build file paths. You can always use "/" and it will be adjusted if needed.
		Q_INVOKABLE static QString separator()             { return QDir::separator(); }
		//! Returns pathName with the '/' separators converted to separators that are appropriate for the underlying operating system.
		Q_INVOKABLE static QString toNative(const QString &path) { return QDir::toNativeSeparators(path); }
		//! Returns pathName using '/' as file separator.
		Q_INVOKABLE static QString fromNative(const QString &path) { return QDir::fromNativeSeparators(path); }
		//! Returns path with directory separators normalized (that is, platform-native separators converted to "/") and redundant ones removed, and "."s and ".."s resolved (as far as possible). Symbolic links are kept
		Q_INVOKABLE static QString clean(const QString &path)  { return QDir::cleanPath(path); }
		//! Returns the absolute path (a path that starts with "/" or with a drive specification), which may contain symbolic links, but never contains redundant ".", ".." or multiple separators.
		Q_INVOKABLE static QString abs(const QString &path)    { return QDir(path).absolutePath(); }
		//! Returns the canonical path, i.e. a path without symbolic links or redundant "." or ".." elements.
		Q_INVOKABLE static QString normalize(const QString &path)  { return QDir(path).canonicalPath(); }

		//! \}
};

#ifndef DOXYGEN
}  // ScriptLib
#endif

Q_DECLARE_METATYPE(ScriptLib::Dir *)
