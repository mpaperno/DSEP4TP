#pragma once

#include <QFile>

//! \file

#ifndef DOXYGEN
// The `ScriptLib` namespace contains helper classes for the JavaScript engine environment.
namespace ScriptLib {
#endif

//! \ingroup FileSystem
//! The `FS` namespace holds constants related to file system tasks. They are referenced in JavaScript as `FS.<constant>`, eg. `FS.O_RDONLY`.
#ifdef DOXYGEN  // need to mark this as a class for doxygen parsing
class FS {
	public:
#else
namespace FS
{
#endif

Q_NAMESPACE

	//! File handling mode flags. \sa OpenMode
	enum OpenModeFlag : quint8  {
		O_NOTOPEN = QIODevice::NotOpen,       //!< The file is not open in any mode, or no explicit mode flags are specified.
		O_DEFAULT = O_NOTOPEN,                //!< Default options for selected operation (depending on method/function being invoked).
		O_RDONLY  = QIODevice::ReadOnly,      //!< Read mode. Text value: 'r'
		O_WRONLY  = QIODevice::WriteOnly,     //!< Write mode. Default is usually to truncate/overwrite the destination file. Text value: 'w'
		O_RDWR    = QIODevice::ReadWrite,     //!< Read+Write mode. Text value: 'rw' or 'r+' or 'w+'
		O_APPEND  = QIODevice::Append,        //!< Append to file (implies O_WRONLY). Text value: 'a'
		O_TRUNC   = QIODevice::Truncate,      //!< Truncate file (implies O_WRONLY). Text value: no value, this is the default mode.
		O_TEXT    = QIODevice::Text,          //!< Handle file in text mode (returns strings instead of bytes, handles Windows line endings).  Text value: 't'
		O_BIN     = O_DEFAULT,                //!< Handle file in binary mode (returns results as byte arrays).  Text value: 'b'
		O_DIRECT  = QIODevice::Unbuffered,    //!< Open in un-buffered mode.   Text value: 's'
		O_EXCL    = QIODevice::NewOnly,       //!< Do not overwrite existing file when writing (returns error if file exists).  Text value: 'x'
		O_NOCREAT = QIODevice::ExistingOnly,  //!< Only write to file if it exists (returns error otherwise).  Text value: 'n'
	};
	Q_FLAG_NS(OpenModeFlag)
	//! The `FS.OpenMode` type stores an OR combination of `FS.OpenModeFlag` values.
	Q_DECLARE_FLAGS(OpenMode, OpenModeFlag)

	static inline QIODevice::OpenMode toQfileFlags(OpenMode mode) { return static_cast<QIODevice::OpenMode>((quint8)mode); }

	//! File date/time enumerations for `FileHandle.fileTime()` and `FileHandle.setFileTime()`. The names are meant to follow GNU/POSIX file info `stat` structure names.
	enum FileTime : qint8 {
		S_ATIME = QFileDevice::FileAccessTime,          //!< Access time.
		S_BTIME = QFileDevice::FileBirthTime,           //!< Birth/creation time (may not be available).
		S_CTIME = QFileDevice::FileMetadataChangeTime,  //!< Attribute/status modification time.
		S_MTIME = QFileDevice::FileModificationTime     //!< File modification time.
	};
	Q_ENUM_NS(FileTime)

	//! Permission flags returned by `permissions()` and used with `setPermissions()`. These conform to GNU/Node/etc file mode constants,
	//! with the addition of "OWN" (owner) bits. On NTFS the owner may differ from the user. On Unix/POSIX the "USR" bits return owner information as per convention.
	//! \note Because of differences in the platforms, the semantics of S_IRUSR, S_IWUSR and S_IXUSR are platform-dependent:
	//! On Unix, the rights of the owner of the file are returned and on Windows the rights of the current user are returned.
	enum Permission : quint16 {
		S_IROWN = QFileDevice::ReadOwner,  //!< The file is readable by the owner of the file.
		S_IWOWN = QFileDevice::WriteOwner,  //!< The file is writable by the owner of the file.
		S_IXOWN = QFileDevice::ExeOwner,  //!< The file is executable by the owner of the file.
		S_IRWXN = S_IROWN | S_IWOWN | S_IXOWN, //!< S_IROWN | S_IWOWN | S_IXOWN
		S_IRUSR = QFileDevice::ReadUser,   //!< The file is readable by the user.
		S_IWUSR = QFileDevice::WriteUser,   //!< The file is writable by the user.
		S_IXUSR = QFileDevice::ExeUser,   //!< The file is executable by the user.
		S_IRWXU = S_IRUSR | S_IWUSR | S_IXUSR, //!< S_IRUSR | S_IWUSR | S_IXUSR
		S_IRGRP = QFileDevice::ReadGroup,  //!< The file is readable by the group.
		S_IWGRP = QFileDevice::WriteGroup,  //!< The file is writable by the group.
		S_IXGRP = QFileDevice::ExeGroup,  //!< The file is executable by the group.
		S_IRWXG = S_IRGRP | S_IWGRP | S_IXGRP, //!< S_IRGRP | S_IWGRP | S_IXGRP
		S_IROTH = QFileDevice::ReadOther,  //!< The file is readable by anyone.
		S_IWOTH = QFileDevice::WriteOther,  //!< The file is writable by anyone.
		S_IXOTH = QFileDevice::ExeOther,  //!< The file is executable by anyone.
		S_IRWXO = S_IROTH | S_IWOTH | S_IXOTH, //!< S_IROTH | S_IWOTH | S_IXOTH
	};
	Q_FLAG_NS(Permission)
	//! The `FS.Permissions` type stores an OR combination of `FS.Permission` values.
	Q_DECLARE_FLAGS(Permissions, Permission)

	//! Error type enumeration returned by the `FileHandle.error()` method.
	enum FileError : qint8 {
		NoError = QFileDevice::NoError, //!< No error occurred.
		ReadError = QFileDevice::ReadError,  //!< An error occurred when reading from the file.
		WriteError = QFileDevice::WriteError,  //!< An error occurred when writing to the file.
		FatalError = QFileDevice::FatalError,  //!< A fatal error occurred.
		ResourceError = QFileDevice::ResourceError,  //!< Out of resources (e.g., too many open files, out of memory, etc.)
		OpenError = QFileDevice::OpenError,  //!< The file could not be opened.
		AbortError = QFileDevice::AbortError,  //!< The operation was aborted.
		TimeOutError = QFileDevice::TimeOutError,  //!< A timeout occurred.
		UnspecifiedError = QFileDevice::UnspecifiedError,  //!< An unspecified error occurred.
		RemoveError = QFileDevice::RemoveError,  //!< The file could not be removed.
		RenameError = QFileDevice::RenameError,  //!< The file could not be renamed.
		PositionError = QFileDevice::PositionError,  //!< The position in the file could not be changed.
		ResizeError = QFileDevice::ResizeError,  //!< The file could not be resized.
		PermissionsError = QFileDevice::PermissionsError,  //!< The file could not be accessed.
		CopyError = QFileDevice::CopyError,  //!< The file could not be copied.
	};
	Q_ENUM_NS(FileError)

};  // namespace FS

#ifndef DOXYGEN
};  // ScriptLib
#endif


Q_DECLARE_METATYPE(ScriptLib::FS::FileError)
Q_DECLARE_METATYPE(ScriptLib::FS::FileTime)
Q_DECLARE_METATYPE(ScriptLib::FS::OpenMode)
Q_DECLARE_OPERATORS_FOR_FLAGS(ScriptLib::FS::OpenMode)
Q_DECLARE_METATYPE(ScriptLib::FS::Permissions)
Q_DECLARE_OPERATORS_FOR_FLAGS(ScriptLib::FS::Permissions)
