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

#include <QFileSystemWatcher>

#include "event_utils.h"

#ifndef DOXYGEN
namespace ScriptLib {
#endif

/*!
	\class FSWatcher
	\ingroup FileSystem
	\brief The `FSWatcher` class provides an interface for monitoring files and directories for modifications.

	`%FSWatcher` monitors the file system for changes to files
	and directories by watching a list of specified paths.

	Call `addPath()` to watch a particular file or directory. Multiple
	paths can be added using the `addPaths()` method. Existing paths can
	be removed by using the `removePath()` and `removePaths()` methods.

	`%FSWatcher` examines each path added to it. Files that have
	been added to the `%FSWatcher` can be accessed using the
	`files()` method, and directories using `directories()`.

	The `fileChanged()` event is emitted when a file has been modified,
	renamed or removed from disk. Similarly, the `directoryChanged()`
	event is emitted when a directory or its contents is modified or
	removed.  Note that `%FSWatcher` stops monitoring files once
	they have been renamed or removed from disk, and directories once
	they have been removed from disk.

	Keep in mind that the instance of `%FSWatcher` must persist for as long as you need the
	path(s) to be watched. If the variable holding the instance goes out of scope, it may
	get garbage-collected at any point and stop emitting events.

	To terminate watching all paths and delete an instance of `%FSWatcher`,
	use the `destroy()` method and set your instance variable value to `null` or `undefined`.

	Notes:
	- On systems running a Linux kernel without inotify support,
		file systems that contain watched paths cannot be unmounted.
	- The act of monitoring files and directories for
		modifications consumes system resources. This implies there is a
		limit to the number of files and directories your process can
		monitor simultaneously. Some system limits the number of open file descriptors to 256
		by default. This means that `addPath()` and `addPaths()` will fail if
		your process tries to add more than 256 files or directories to
		the file system monitor. Also note that your process may have
		other file descriptors open in addition to the ones for files
		being monitored, and these other open descriptors also count in
		the total. MacOS uses a different backend and does not
		suffer from this issue.


	\note The `%FSWatcher` class is a proxy for [QFileSystemWatcher](https://doc.qt.io/qt-6/qfilesystemwatcher.html),
	part of the underlying Qt C++ library. Most of the documentation text here is originally from
	the QFileSystemWatcher documentation (used under the GNU Free Documentation License version 1.3).

	\sa File, Dir
	\since v1.3
*/
class FSWatcher : public QFileSystemWatcher
{
		Q_OBJECT
		NAMED_EVENT_HANDLER();
		EVENT_PROPERTY(fileChanged);
		EVENT_PROPERTY(directoryChanged);

	public:

		//! Constructs a new file system watcher object that isn't monitoring any paths. Path(s) can be added later with `addPath()` or `addPaths()`.
		Q_INVOKABLE explicit FSWatcher() :
		  QFileSystemWatcher()
		{
			setObjectName(QStringLiteral("FSWatcher"));
		}

		//! Constructs a new file system watcher object which monitors the specified paths list.
		//! See `addPaths()` for details of how the `paths` argument is treated.
		Q_INVOKABLE explicit FSWatcher(const QStringList &paths) :
		  QFileSystemWatcher(paths)
		{
			setObjectName(QStringLiteral("FSWatcher"));
		}

		~FSWatcher() {
			// qCDebug(lcPlugin) << "Destroyed" << this;
		}

		/*!
			Adds `path` to the file system watcher if `path` exists.
			The path is not added if it does not exist, or if it is already being monitored by this file system watcher.

			If `path` specifies a directory, the `directoryChanged()` event will be emitted when `path` is modified or removed from disk;
			otherwise the `fileChanged()` event is emitted when `path` is modified, renamed or removed.

			If the watch was successful, `true` is returned.

			Reasons for a watch failure are generally system-dependent, but may include the resource not existing,
			access failures, or the total watch count limit, if the platform has one.

			\sa addPaths(), removePath()
		*/
		Q_INVOKABLE bool addPath(const QString &path) { return QFileSystemWatcher::addPath(path); }

		/*!
			Adds each path in \a paths to the file system watcher.
			Paths are not added if they do not exist, or if they are already being monitored by the file system watcher.

			If a path specifies a directory, the `directoryChanged()` signal will be emitted when the path is modified or removed from disk;
			otherwise the `fileChanged()` signal is emitted when the path is modified, renamed, or removed.

			The return value is a list of paths that could __not__ be watched.

			Reasons for a watch failure are generally system-dependent, but may include the resource not existing,
			access failures, or the total watch count limit, if the platform has one.

			\note There may be a system dependent limit to the number of files and directories that can be monitored simultaneously.
			If this limit has been reached, the excess \a paths will not be monitored, and they will be added to the returned list.

			\sa addPath(), removePaths()
		*/
		Q_INVOKABLE QStringList addPaths(const QStringList &paths) { return QFileSystemWatcher::addPaths(paths); }

		/*!
			Removes the specified \a path from the file system watcher.
			If the watch is successfully removed, `true` is returned.

			Reasons for watch removal failing are generally system-dependent, but may be due to the path having already been deleted, for example.
			\sa removePaths(), addPath()
		*/
		Q_INVOKABLE bool removePath(const QString &path) { return QFileSystemWatcher::removePath(path); }

		/*!
			Removes the specified \a paths from the file system watcher.
			The return value is a list of paths which were __not__ able to be un-watched successfully.

			Reasons for watch removal failing are generally system-dependent, but may be due to the path having already been deleted, for example.
			\sa removePath(), addPaths()
		*/
		Q_INVOKABLE QStringList removePaths(const QStringList &paths) { return QFileSystemWatcher::removePaths(paths); }

		/*!
			Returns a list of paths to directories that are being watched.
			\sa files()
		*/
		Q_INVOKABLE QStringList directories() const { return QFileSystemWatcher::directories(); }
		/*!
			Returns a list of paths to files that are being watched.
			\sa directories()
		*/
		Q_INVOKABLE QStringList files() const { return QFileSystemWatcher::files(); }

		//! Stops watching all paths and destroys this instance of FSWatcher. Trying to use this instance after calling `destroy()` is undefined behavior.
		Q_INVOKABLE void destroy() { this->deleteLater(); }

		/*!
			\name Events
			See \ref EventHandlers for details about connecting handlers to events.
			\{

			\fn void FSWatcher::fileChanged(string path)
			\memberof FSWatcher
			This event is emitted when the file at the specified \a path is modified, renamed or removed from disk.

			\note As a safety measure, many applications save an open file by writing a new file and then deleting the old one.
			In your handler function, you can check if `watcher.files().contains(path)`.
			If it returns \c false, check whether the file still exists and then call \c addPath() to continue watching it.
			\sa directoryChanged()


			\fn void FSWatcher::directoryChanged(string path)
			\memberof FSWatcher
			This event is emitted when the directory at a specified \a path is modified (e.g., when a file is added or deleted) or removed from disk.

			Note that if there are several changes during a short period of time, some of the changes might not emit this event.
			However, the last change in the sequence of changes will always generate this event.
			\sa fileChanged()

			\}
		*/
};

#ifndef DOXYGEN
}  // namespace ScriptLib
#endif
