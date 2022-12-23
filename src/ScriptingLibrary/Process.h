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

#include <QProcess>

#include "FS.h"

#ifndef DOXYGEN
namespace ScriptLib {
#endif

//! \ingroup Util
//! The Process class allows interaction with external processes, such as launching a system command or running an application.
class Process : public QProcess
{
		Q_OBJECT

	public:
		Q_INVOKABLE explicit Process(QObject *parent = 0) : QProcess(parent)
		{
			setObjectName(QStringLiteral("Process"));
		}

		/*!
			\fn int execute(String program, Array<String> arguments = [])
			\memberof Process
			Starts the program `program` with the arguments `arguments` in a new process, waits for it to finish, and then returns the exit code of the process.

			Any data the new process writes to the console is forwarded to the calling process. The environment and working directory are inherited from the calling process.

			Argument handling is identical to the respective `start()` overload.

			If the process cannot be started, `-2` is returned. If the process crashes, `-1` is returned. Otherwise, the process' exit code is returned.
		*/
		Q_INVOKABLE static int execute(const QString &program, const QStringList &arguments = QStringList())
		{
			return QProcess::execute(program, arguments);
		}

		/*!
			\fn int startDetached(String program, Array<String> arguments = [], String workingDirectory = "")
			\memberof Process
			Starts the program `program` with the arguments `arguments` in a new process, and detaches from it.

			Returns the new process ID (pid) on success; otherwise returns zero. If the calling process exits, the detached process will continue to run unaffected.
			Argument handling is identical to the respective `start()` overload.

			The process will be started in the directory `workingDirectory`. If `workingDirectory` is empty, the working directory is inherited from the calling process.
		*/
		Q_INVOKABLE static quint32 startDetached(const QString &program, const QStringList &arguments = {}, const QString &workingDirectory = QString())
		{
			qint64 pid = 0;
			if (QProcess::startDetached(program, arguments, workingDirectory, &pid))
				return quint32(pid);
			return 0;
		}


		//! Returns the program the process was last started with.
		Q_INVOKABLE QString program() const { return QProcess::program(); }
		//! Set the program to use when starting the process. This function must be called before `start()`.
		Q_INVOKABLE void setProgram(const QString &program) { QProcess::setProgram(program); }

		//! \fn Array<String> arguments()
		//! \memberof Process
		//! Returns the command line arguments the process was last started with.
    Q_INVOKABLE QStringList arguments() const { return QProcess::arguments(); }
		//! \fn void setArguments(Array<String> arguments)
		//! \memberof Process
		//! Set the arguments to pass to the called program when starting the process. This function must be called before `start()`.
    Q_INVOKABLE void setArguments(const QStringList & arguments) { QProcess::setArguments(arguments); }

		//! If `Process` has been assigned a working directory, this function returns the working directory that the `Process` will enter before the program has started.
		//! Otherwise, (i.e., no directory has been assigned,) an empty string is returned, and `Process` will use the application's current working directory instead.
		Q_INVOKABLE QString workingDirectory() const { return QProcess::workingDirectory(); }
		//! Sets the working directory to `dir`. `Process` will start the process in this directory.
		//! The default behavior is to start the process in the working directory of the calling process.
    Q_INVOKABLE void setWorkingDirectory(const QString &dir) { QProcess::setWorkingDirectory(dir); }

#if defined(Q_OS_WIN) || defined(DOXYGEN)
		//! Returns the additional native command line arguments for the program. \sa setNativeArguments()
		//! \note This function is only available on the Windows platform.
		Q_INVOKABLE QString nativeArguments() const { return QProcess::nativeArguments(); }
		//! Sets additional native command line arguments for the program.
		//!
		//! On Windows where the system API for passing command line arguments to a subprocess natively uses a single string, one can conceive command lines which cannot be passed via `Process`'s portable list-based API.<br />
		//! In such cases this function must be used to set a string which is appended to the string composed from the usual argument list, with a delimiting space.
		//! \note This function is only available on the Windows platform.
		Q_INVOKABLE void setNativeArguments(const QString &arguments) { QProcess::setNativeArguments(arguments); }
#endif

		/*!
			Redirects the process' standard input to the file indicated by `fileName`. When an input redirection is in place, the `Process` object will be in read-only mode (calling `write()` will result in error).

			To make the process read EOF right away, pass `nullDevice()` here. This is cleaner than using `closeWriteChannel()` before writing any data, because it can be set up prior to starting the process.

			If the file `fileName` does not exist at the moment `start()` is called or is not readable, starting the process will fail. Calling `setStandardInputFile()` after the process has started has no effect.
		*/
		void setStandardInputFile(const QString &fileName) { QProcess::setStandardInputFile(fileName); }
		/*!
			Redirects the process' standard output to the file `fileName`. When the redirection is in place, the standard output read channel is closed: reading from it using `read()` will always fail, as will `readAllStandardOutput()`.

			To discard all standard output from the process, pass `nullDevice()` here. This is more efficient than simply never reading the standard output, as no `Process` buffers are filled.

			If the file `fileName` doesn't exist at the moment `start()` is called, it will be created. If it cannot be created, the starting will fail.

			If the file exists and mode is `FS.O_TRUNC`, the file will be truncated. Otherwise (if mode is `FS.O_APPEND`), the file will be appended to.

			Calling `setStandardOutputFile()` after the process has started has no effect.
		*/
		void setStandardOutputFile(const QString &fileName, FS::OpenMode mode = FS::O_TRUNC) { QProcess::setStandardOutputFile(fileName, toQfileFlags(mode)); }
		/*!
			Redirects the process' standard error to the file `fileName`.
			When the redirection is in place, the standard error read channel is closed: reading from it using `read()` will always fail, as will `readAllStandardError()`.

			To discard all standard error from the process, pass `nullDevice()` here. This is more efficient than simply never reading the standard error, as no `Process` buffers are filled.

			If the file `fileName` doesn't exist at the moment `start()` is called, it will be created. If it cannot be created, the starting will fail.

			If the file exists and mode is `FS.O_TRUNC`, the file will be truncated. Otherwise (if mode is `FS.O_APPEND`), the file will be appended to.

			Calling `setStandardErrorFile()` after the process has started has no effect.
		*/
    void setStandardErrorFile(const QString &fileName, FS::OpenMode mode = FS::O_TRUNC) { QProcess::setStandardErrorFile(fileName, toQfileFlags(mode)); }

		//! Starts the program set by `setProgram()` with arguments set by `setArguments()`/`setNativeArguments()`.
		Q_INVOKABLE void start() { QProcess::start(); }

		/*!
			\fn void start(String program, Array<String> arguments = [])
			\memberof Process
			Starts the given `program` in a new process, passing the command line arguments in `arguments`. See `setProgram()` for information about how `Process` searches for the executable to be run.
			The `Process` object will immediately enter the Starting state. If the process starts successfully, `Process` will emit `started()`; otherwise, `errorOccurred()` will be emitted.
		*/
		Q_INVOKABLE void start(const QString &program, const QStringList &arguments = QStringList())
		{
			QProcess::start(program, arguments);
		}

		/*!
			\fn void startCommand(String command)
			\memberof Process
			\brief Starts the command command in a new process.

			`command` is a single string of text containing both the program name and its arguments. The arguments are separated by one or more spaces. For example:
			```js
			var process = new Process();
			process.startCommand("del /s *.txt");
			// same as process.start("del", ["/s", "*.txt"]);
			```
			Arguments containing spaces must be quoted to be correctly supplied to the new process. For example:
			```js
			process.startCommand("dir \"My Documents\"");
			```
			Literal quotes in the command string are represented by triple quotes. For example:
			```
			process.startCommand("dir \"Epic 12\"\"\" Singles\"");
			```

			After the command string has been split and unquoted, this function behaves like `start()`.
		*/
		Q_INVOKABLE void startCommand(const QString &command)
		{
			QProcess::startCommand(command);
		}

		/*!
			\memberof Process
			Starts the program set by `setProgram()` with arguments set by `setArguments()` in a new process, and detaches from it.
			Returns the new process ID (pid) on success; otherwise returns zero. If the calling process exits, the detached process will continue to run unaffected.
		*/
		Q_INVOKABLE quint32 startDetached()
		{
			qint64 pid = 0;
			if (QProcess::startDetached(&pid))
				return quint32(pid);
			return 0;
		}

		//! Closes all communication with the process and kills it. After calling this function, `Process` will no longer emit `readyRead()`, and data can no longer be read or written.
		Q_INVOKABLE void close() override { QProcess::close(); }

		//! \fn ArrayBuffer readAll()
		//! \memberof Process
		//! Reads all remaining data from the device, and returns it as an `ArrayBuffer`.
		Q_INVOKABLE QByteArray readAll() { return QProcess::readAll(); }
		//! \fn ArrayBuffer readAllStandardOutput()
		//! \memberof Process
		//! This function returns all data available from the standard output of the process as a `ArrayBuffer`.
    Q_INVOKABLE QByteArray readAllStandardOutput() { return QProcess::readAllStandardOutput(); }
		//! \fn ArrayBuffer readAllStandardError()
		//! \memberof Process
		//! This function returns all data available from the standard error of the process as a `ArrayBuffer`.
    Q_INVOKABLE QByteArray readAllStandardError() { return QProcess::readAllStandardError(); }

		//! Returns the exit code of the last process that finished.
    Q_INVOKABLE int exitCode() const { return QProcess::exitCode(); }
    Q_INVOKABLE QProcess::ExitStatus exitStatus() const { return QProcess::exitStatus(); }

		//! Blocks until the process has started and the `started()` signal has been emitted, or until `msecs` milliseconds have passed.
		//! Returns `true` if the process was started successfully; otherwise returns `false` (if the operation timed out or if an error occurred).
		//! If msecs is -1, this function will not time out
		Q_INVOKABLE bool waitForStarted(int msecs = 30000) { return QProcess::waitForStarted(msecs); }
		//! Blocks until new data is available for reading and the `readyRead()` signal has been emitted, or until `msecs` milliseconds have passed. If `msecs` is -1, this function will not time out.
		//! Returns `true` if new data is available for reading; otherwise returns `false` (if the operation timed out or if an error occurred).
    Q_INVOKABLE bool waitForReadyRead(int msecs = 30000) override { return QProcess::waitForReadyRead(msecs); }
		//! This function waits until a payload of buffered data has been written to the process and the `bytesWritten()` signal has been emitted, or until `msecs` milliseconds have passed.
		//! If `msecs` is -1, this function will not time out.
		//! Returns `true` if a payload of data was written to the device; otherwise returns `false` (i.e. if the operation timed out, or if an error occurred).
    Q_INVOKABLE bool waitForBytesWritten(int msecs = 30000) override { return QProcess::waitForBytesWritten(msecs); }
		//! Blocks until the process has finished and the `finished()` signal has been emitted, or until `msecs` milliseconds have passed.
		//! Returns `true` if the process was finished; otherwise returns `false` (if the operation timed out or if an error occurred, or if this `Process` is already finished).
		//! If `msecs` is -1, this function will not time out
    Q_INVOKABLE bool waitForFinished(int msecs = 30000) { return QProcess::waitForFinished(msecs); }

		// The null device of the operating system. Use this to discard output from the program if it not needed, as this improves disables all buffering done by `Process` on the output.
		Q_INVOKABLE static QString nullDevice() { return QProcess::nullDevice(); }

};

#ifndef DOXYGEN
}  // namespace ScriptLib
#endif
