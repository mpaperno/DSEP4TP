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

#include <QClipboard>
#include <QGuiApplication>
#include <QJSEngine>
#include <QMimeData>
#include <QObject>
#include <QStringDecoder>
#include <QSemaphore>

#include "utils.h"

#ifndef DOXYGEN
namespace ScriptLib {
#else
#define QByteArray ArrayBuffer
#endif

/*!
	\ingroup Util
	The Clipboard class offers a simple mechanism to copy and paste data between applications.

	Clipboard features are implemented as a module named `clipboard`. Use an `import` statement (from another module) or
	`require()` function call (from any code) to import desired features.

	```js
	import clipboard from "clipboard";
	// or
	import { text as clipboardText } from "clipboard";
	```

	Or using `require()`:
	```js
	const clipboard = require("clipboard");
	// or
	const { text: clipboardText } = require("clipboard");
	```

	Clipboard data can potentially contain multiple types of data, identified by a MIME (or "media") type.
	For example copying text from a Web page may put it on the clipboard in both plain-text and HTML
	formats ("text/plain" and "text/html" MIME types, respectively).

	The `text()`, `setText()`, `data()` and `setData()` function versions with no parameters operate with "text/plain" and "application/octet-stream"
	default MIME types (as further explained in their documentation). Alternatively, the more explicit function overloads can be used which allow
	specifying a MIME type.

	On MacOS and Linux/X11 systems there are two other "clipboard modes" available besides the primary shared clipbaord. \n
	On X11 this can work with whatever text is currently selected in a focused application (`Clipboard.Selection` mode). \n
	On MacOS there is a special "search buffer" which can be shared between applications (`Clipboard.SearchBuffer` mode). \n
	Most of the functions in this class accept an optional `mode` parameter which sets which clipboard to use. By default the main
	clipboard is used, and on Windows this is the only clipboard mode available. \n
	See \ref supportsSelection, \ref supportsFindBuffer, and \ref Mode enumeration docs for more details.

	### Usage examples

	#### Copy text to clipboard
	```js
	const { setText } = require("clipboard");
	setText("This text will be copied to the clipboard");
	```

	#### Get text from clipboard
	```js
	const { text: clipboardText } = require("clipboard");
	let text = clipboardText();
	// text == "This text will be copied to the clipboard"
	```

	#### Copy the contents of a text file to the clipboard
	```js
	const { setText: copy } = require("clipboard");
	try {
		copy(File.readText("path/to/myFile.txt"));
	}
	catch (e) {
		console.error("Error reading file: ", e);
	}
	```

	#### Copy an image from a file to the clipboard
	```js
	const { setData: copy } = require("clipboard");
	try {
		copy("image/png", File.read("path/to/myImage.png", 'b'));
	}
	catch (e) {
		console.error("Error reading file: ", e);
	}
	```

	#### Get text from the clipboard, preferring HTML format if it is available.
	```js
	const clipboard = require("clipboard");
	let text = clipboard.hasMimeType("text/html") ? clipboard.text("html") : clipboard.text();
	```

	#### Loop over all available formats on the clipboard and log the contents of each one as a string.
	```js
	const { mimeTypes, data: clipboardData } = require("clipboard");
	const availableTypes = mimeTypes();
	for (const type of availableTypes)
		console.log(type + " :", '\n', clipboardData(type).toString());
	```

*/
class Clipboard : public QObject
{
		Q_OBJECT

		//! This property value is `true` if the system clipboard supports a separate search buffer, or `false` otherwise. This is only supported on MacOS and possibly some Android systems.
		Q_PROPERTY(bool supportsFindBuffer MEMBER supportsFindBuffer CONSTANT)
		//! This property value is `true` if the system clipboard supports mouse selection, or `false` otherwise. Mouse selection is generally only supported on X11-based systems (Linux, BSD, etc).
		Q_PROPERTY(bool supportsSelection MEMBER supportsSelection CONSTANT)

		Q_PROPERTY(Clipboard::Mode Clipboard READ mode_Clipboard CONSTANT)
		Q_PROPERTY(Clipboard::Mode Selection READ mode_Selection CONSTANT)
		Q_PROPERTY(Clipboard::Mode FindBuffer READ mode_FindBuffer CONSTANT)

#if 0
		//! This property value is `true` if the current system clipboard contains any type of data at all (ie. is not empty), or `false` otherwise (if the clipboard is empty).
		//! \sa textAvailable
		Q_PROPERTY(bool dataAvailable READ dataAvailable CONSTANT)
		//! This property value is `true` if the current system clipboard contains any type of string data, or `false` otherwise.
		//! String data would be any type with a "text" MIME type or one that can be converted to printable text. \sa dataAvailable
		Q_PROPERTY(bool textAvailable READ textAvailable CONSTANT)
		//! This property gets or sets the current system clipboard value as a plain text String type (ASCII/UTF8/UTF16).
		//! When setting the value, it is written to the clipboard as "text/plain" MIME type.
		//! When reading the value, the closest-matched MIME type will be returned (eg. "text/plain" if it exists, or "text/html" if that is the next best options, etc).
		//! /sa, mimeTypes(), hasMimeType(), textType(), setTextType(), dataType(), setDataType()
		Q_PROPERTY(QString text READ text WRITE setText NOTIFY dataChanged)
		//! This property gets or sets the current system clipboard value as raw bytes. \n
		//! When setting the value, it is written to the clipboard as "application/octet-stream" MIME type. \n
		//! When reading the value:
		//! - if the clipboard has no data, an empty ArrayBuffer is returned;
		//! - if the clipboard contains data of only one MIME type (or with no type specified), then that data is returned;
		//! - if the clipboard contains multiple data types:
		//!		- it first looks for data with an "application/octet-stream" MIME type;
		//!		- if that fails, the first MIME type that starts with "application" will be returned;
		//!   - if no such types were found, then the first data type is returned.
		//!
		//! To specify a MIME type, use the `setDataType()` and `dataType()` methods instead.
		//! \sa mimeTypes(), hasMimeType()
		Q_PROPERTY(QByteArray data READ data WRITE setData NOTIFY dataChanged)
#endif

		explicit Clipboard(bool isStatic) : Clipboard()
		{
			if (isStatic)
				QJSEngine::setObjectOwnership(this, QJSEngine::CppOwnership);
		}

		const bool supportsSelection = QGuiApplication::clipboard()->supportsSelection();
		const bool supportsFindBuffer = QGuiApplication::clipboard()->supportsFindBuffer();

	public:

		//! This enumeration is used in the `mode` argument of several methods and controls which part of the system clipboard is used. \n
		//! With `Clipboard` mode (the deafult for all methods), the data is stored/retrieved with the global clipboard. \n
		//! If mode is `Selection`, the data is stored/retrieved with the global mouse selection (requires a supporting system, eg. X11). \n
		//! If mode is `FindBuffer`, the data is stored/retrieved with the search string buffer (eg. on MacOS).
		//! \sa supportsFindBuffer, supportsSelection
		enum class Mode {
			Clipboard = QClipboard::Clipboard,     //!< Indicates that data should be stored and retrieved from the global clipboard.
			Selection = QClipboard::Selection,     //!< Indicates that data should be stored and retrieved from the global mouse selection. Support for Selection is provided only on systems with a global mouse selection (e.g. X11).
			FindBuffer = QClipboard::FindBuffer,   //!< Indicates that data should be stored and retrieved from the Find buffer. This mode is used for holding search strings on macOS.
		};
		Q_ENUM(Mode)

		static Clipboard *instance()
		{
			static Clipboard c(true);
			return &c;
		}

		/*Q_INVOKABLE*/ explicit Clipboard(QObject *parent = nullptr) : QObject(parent)
		{
			setObjectName("DSE.Clipboard");
			QClipboard *clip = QGuiApplication::clipboard();
			connect(clip, &QClipboard::dataChanged, this, &Clipboard::clipboardChanged, Qt::QueuedConnection);
			connect(clip, &QClipboard::selectionChanged, this, &Clipboard::selectionChanged, Qt::QueuedConnection);
			connect(clip, &QClipboard::findBufferChanged, this, &Clipboard::findBufferChanged, Qt::QueuedConnection);
		}

		static Mode mode_Clipboard() { return Mode::Clipboard; }
		static Mode mode_Selection() { return Mode::Selection; }
		static Mode mode_FindBuffer() { return Mode::FindBuffer; }

		//! \fn bool dataAvailable(int mode = Mode.Clipboard)
		//! \memberof Clipboard
		//! Returns `true` if the current system clipboard contains any type of data at all (ie. is not empty), or `false` otherwise (if the clipboard is empty).
		//! \n The opitonal `mode` argument is used to control which part of the system clipboard is used.
		//! \sa textAvailable(), hasMimeType(), hasMediaType()
		Q_INVOKABLE bool dataAvailable(Clipboard::Mode mode = Mode::Clipboard) const
		{
			const QMimeData *m = mimeData(mode);
			return m && !m->formats().isEmpty();
		}

		//! \fn bool textAvailable(int mode = Mode.Clipboard)
		//! \memberof Clipboard
		//! Returns `true` if the current system clipboard contains any type of string data, or `false` otherwise.
		//! String data would be any type with a "text" primary MIME type, eg "text/plain", "text/html", "text/csv", etc.
		//! \n The opitonal `mode` argument is used to control which part of the system clipboard is used.
		//! \sa dataAvailable(), hasMimeType(), hasMediaType()
		Q_INVOKABLE bool textAvailable(Clipboard::Mode mode = Mode::Clipboard) const
		{
			if (const QMimeData *m = mimeData(mode)) {
				if (m->hasText())
					return true;
				return hasMatchingMimeType(QStringLiteral("text"), m->formats());
			}
			return false;
		}

		//! \fn String text(int mode = Mode.Clipboard)
		//! \memberof Clipboard
		//! Returns the current system clipboard value as a plain text String type (ASCII/UTF8/UTF16).
		//! If the clipboard contains multipe value data types it will return:
		//!	- "text/plain" type with charset=utf-8 is preferred, if it exists (this supports the full Unicode character range, it is just expressed in the most portable UTF-8 format);
		//!	- "text/plain" type with any or no character encoding specified, if that exists;
		//!	- the first "text" type found will be returned (eg. "text/html", "text/csv", etc);
		//! - if no text types are found, or the clipboard is entirely empty, an empty string is returned.
		//!
		//! To specify an explicit text type, use the `text()` function version which takes that option as the first parameter (see below).
		//! \n The opitonal `mode` argument is used to control which part of the system clipboard is used.
		//! \sa mimeTypes(), hasMimeType(), setText(), data()
		Q_INVOKABLE QString text(Clipboard::Mode mode = Mode::Clipboard) const
		{
			if (const QMimeData *m = mimeData(mode)) {
				const auto formats = m->formats();
				if (!formats.isEmpty()) {
					if (hasMatchingMimeType(QStringLiteral("text/plain;charset=utf-8"), formats))
						return text(QStringLiteral("plain;charset=utf-8"), mode);
					if (hasMatchingMimeType(QStringLiteral("text/plain"), formats))
						return text(QStringLiteral("plain"), mode);
					const auto match = getMatchingMimeTypes(QStringLiteral("text"), formats);
					if (!match.isEmpty())
						return text(match.first().mid(5), mode);
				}
			}
			return QString();
		}
		//! \fn String text(String subType, int mode = Mode.Clipboard)
		//! \memberof Clipboard
		//! Returns the clipboard text with MIME subtype `subType`, or an empty string if the clipboard does not contain any text of the given subtype.
		//! The subtype is _just_ the part after "text/" of a MIME type, eg. "plain", "html", "csv", etc. It can optionally include peropery(ies),
		//! for example a character encoding like "plain;charset=utf-8".  The name search is case-insensitive.
		//! \n The opitonal `mode` argument is used to control which part of the system clipboard is used.
		//! \sa hasMimeType(), setText(), Clipboard.Mode
		Q_INVOKABLE QString text(const QString &subType, Clipboard::Mode mode = Mode::Clipboard) const
		{
			const QByteArray d = data(QStringLiteral("text/") + subType, mode);
			auto encoding = QStringConverter::encodingForData(d);
			if (!encoding)
					encoding = QStringConverter::Utf8;
			return QStringDecoder(*encoding).decode(d);
		}


		//! \fn String setText(String text, int mode = Mode.Clipboard)
		//! \memberof Clipboard
		//! This function sets the current system clipboard value as a plain text String type (ASCII/UTF8/UTF16).
		//! It is written to the clipboard as "text/plain" MIME type.
		//! \n The opitonal `mode` argument is used to control which part of the system clipboard is used.
		//! \sa setData()
		Q_INVOKABLE void setText(const QString &text, Clipboard::Mode mode = Mode::Clipboard) const {
			setText(QLatin1String("plain"), text, mode);
		}
		//! \fn String setText(String subType, String text, int mode = Mode.Clipboard)
		//! \memberof Clipboard
		//! Sets the clipboard text with MIME subtype `subType` to `text`.
		//! The subtype is _just_ the part after "text/" of a MIME type, eg. "plain", "html", "csv", etc.
		//! \n The opitonal `mode` argument is used to control which part of the system clipboard is used.
		//! \sa text(), Clipboard.Mode
		Q_INVOKABLE void setText(const QString &subType, const QString &text, Clipboard::Mode mode = Mode::Clipboard) const {
			setData(QLatin1String("text/") + subType, QStringEncoder(QStringEncoder::Utf8)(text), mode);
		}

		//! \fn ArrayBuffer data(int mode = Mode.Clipboard)
		//! \memberof Clipboard
		//! Returns the current system clipboard value as raw bytes. \n
		//! When reading the value:
		//! - if the clipboard has no data, an empty `ArrayBuffer` is returned;
		//! - if the clipboard contains data of only one MIME type (or with no type specified), then that data is returned;
		//! - if the clipboard contains multiple data types:
		//!		- it first looks for data with an "application/octet-stream" MIME type;
		//!		- if that fails, the first MIME type that starts with "application" will be returned;
		//!   - if no such types were found, then the first data type is returned.
		//!
		//! To specify a MIME type, use `data()` function version which takes that as the first parameter.
		//! \n The opitonal `mode` argument is used to control which part of the system clipboard is used.
		//! \sa setData(), mimeTypes(), hasMimeType()
		Q_INVOKABLE QByteArray data(Clipboard::Mode mode = Mode::Clipboard) const
		{
			const QMimeData *m = mimeData(mode);
			if (!m || m->formats().isEmpty())
				return QByteArray();
			const auto &formats = m->formats();
			if (formats.isEmpty())
				return QByteArray();
			if (formats.length() == 1)
				return m->data(formats.first());

			QString type = QLatin1String("application/octet-stream");
			if (!m->hasFormat(type)) {
				const auto match = getMatchingMimeTypes(QStringLiteral("application"), formats);
				type = match.isEmpty() ? formats.first() : match.first();
			}
			return m->data(type);
		}
		//! \fn ArrayBuffer data(String mimeType, int mode = Mode.Clipboard)
		//! \memberof Clipboard
		//! Get the raw byte data associated with MIME type `mimeType` from the current system clipboard.
		//! If the clipboard didn't have the specified MIME type, then an empty ArrayBuffer is returned.
		//! \n The opitonal `mode` argument is used to control which part of the system clipboard is used.
		//! \sa hasMimeType(), setData(), data, text, Clipboard.Mode
		Q_INVOKABLE QByteArray data(const QString &mimeType, Clipboard::Mode mode = Mode::Clipboard) const
		{
			if (const QMimeData *m = mimeData(mode))
				return m->data(mimeType);
			return QByteArray();
		}

		//! \fn void setData(ArrayBuffer data, int mode = Mode.Clipboard)
		//! \memberof Clipboard
		//! Sets the current system clipboard value as raw bytes. It is written to the clipboard as "application/octet-stream" MIME type.
		//! \n The opitonal `mode` argument is used to control which part of the system clipboard is used.
		//! \sa data(), setDataType()
		Q_INVOKABLE void setData(const QByteArray &data, Clipboard::Mode mode = Mode::Clipboard) { setData(QLatin1String("application/octet-stream"), data, mode); }

		//! \fn void setData(String mimeType, ArrayBuffer data, int mode = Mode.Clipboard)
		//! \memberof Clipboard
		//! Set raw byte data with MIME type `mimeType` to `data` on the current system clipboard.
		//! The opitonal `mode` argument is used to control which part of the system clipboard is used.
		//! \sa data(), data, text, Clipboard.Mode
		Q_INVOKABLE void setData(const QString &mimeType, const QByteArray &data, Clipboard::Mode mode = Mode::Clipboard) const
		{
			QMimeData *m = data.isNull() || mimeType.isNull() ? nullptr : new QMimeData();
			if (m)
				m->setData(mimeType, data);
#ifdef Q_OS_WIN
			Utils::runOnThread(qGuiApp->thread(), [=]() {
				QGuiApplication::clipboard()->setMimeData(m, (QClipboard::Mode)mode);
			});
#else
			QGuiApplication::clipboard()->setMimeData(m, (QClipboard::Mode)mode);
#endif
		}

		//! \fn void clear(int mode = Mode.Clipboard)
		//! \memberof Clipboard
		//! Clears the system clipboard of all values.
		//! \n The opitonal `mode` argument is used to control which part of the system clipboard is used.
		Q_INVOKABLE void clear(Clipboard::Mode mode = Mode::Clipboard) const { setData(QString(), QByteArray(), mode); }

		//! \fn bool hasMimeType(String mimeType, int mode = Mode.Clipboard)
		//! \memberof Clipboard
		//! Checks if the clipboard contains data with full MIME type `mimeType` and returns `true` or `false`.
		//! The full MIME type is a combination of type, subtype, and possibly one or more parameters/values.
		//! For example: "text/plain" or "text/plain;charset=UTF-8"
		//! \n The opitonal `mode` argument is used to control which part of the system clipboard is used. \sa mimeTypes(), Clipboard.Mode
		Q_INVOKABLE bool hasMimeType(const QString &mimeType, Clipboard::Mode mode = Mode::Clipboard) const
		{
			const QMimeData *m = mimeData(mode);
			return m && m->hasFormat(mimeType);
		}

		//! \fn bool hasMediaType(String mediaType, int mode = Mode.Clipboard)
		//! \memberof Clipboard
		//! Checks if the clipboard contains data with base MIME _type_ `mediaType` and returns `true` or `false`.
		//! The "media type" is the first part of a full MIME type before the slash (`/`), w/out a subtype or parameters.
		//! For example: "text", "application", "image", etc.
		//! \n The opitonal `mode` argument is used to control which part of the system clipboard is used. \sa mimeTypes(), Clipboard.Mode
		Q_INVOKABLE bool hasMediaType(const QString &mediaType, Clipboard::Mode mode = Mode::Clipboard) const
		{
			const QMimeData *m = mimeData(mode);
			return m && hasMatchingMimeType(mediaType, m->formats());
		}

		//! \fn Array<String> mimeTypes(String mimeType, int mode = Mode.Clipboard)
		//! \memberof Clipboard
		//! Returns an array of formats contained in the clipboard, if any.
		//! This is a list of MIME types for which the object can return suitable data. The formats in the list are in a priority order.
		//! \n The opitonal `mode` argument is used to control which part of the system clipboard is used.
		//! \sa hasMimeType(), Clipboard.Mode, text(), data()
		Q_INVOKABLE QStringList mimeTypes(Clipboard::Mode mode = Mode::Clipboard) const
		{
			const QMimeData *m = mimeData(mode);
			return m ? m->formats() : QStringList();
		}

	Q_SIGNALS:
		//! \name Events
		//! \{

		//! This event is raised whenever the contents of the system clipboard change.
		//! Use `text()`, `data()`, or the other methods to get the current value after receiving this event.
		void clipboardChanged();
		//! This event is raised whenever the contents of the Find text change on MacOS.
		//! Use `text()`, `data()`, etc methods with `mode` argument set to `Clipboard.FindBuffer` to get the current value after receiving this event.
		//! \sa supportsFindBuffer
		void findBufferChanged();
		//! This event is raised whenever the contents of the system selection change.
		//! Use `text()`, `data()`, etc, methods with `mode` argument set to `Clipboard.Selection` to get the current value after receiving this event.
		//! \sa supportsSelection
		void selectionChanged();

		//! \}

	private:

		QStringList getMatchingMimeTypes(QStringView type, const QStringList &formats) const
		{
			QStringList ret {};
			for (const auto &f : formats) {
				if (f.startsWith(type, Qt::CaseInsensitive))
					ret << f;
			}
			return ret;
		}

		bool hasMatchingMimeType(QStringView type, const QStringList &formats) const
		{
			for (const auto &f : formats)
				if (f.startsWith(type, Qt::CaseInsensitive))
					return true;
			return false;
		}

		const QMimeData *mimeData(Mode mode = Mode::Clipboard) const
		{
#ifdef Q_OS_WIN
			// Direct access to clipboard from non-gui thread causes timeout warnings and possibly other issues.
			const QMimeData *m;
			runOnThreadSync(qGuiApp->thread(), [&, mode]() {
				m = QGuiApplication::clipboard()->mimeData((QClipboard::Mode)mode);
			});
			return m;
#else
			return QGuiApplication::clipboard()->mimeData((QClipboard::Mode)mode);
#endif
		}

#ifdef Q_OS_WIN
		template <typename Func>
		inline void runOnThreadSync(QThread *qThread, Func &&func) const
		{
			QSemaphore wc(1);
			wc.acquire(1);
			Utils::runOnThread(qThread, [=, &wc]() {
				func();
				wc.release(1);
			});
			while (!wc.tryAcquire())
				QGuiApplication::processEvents();
		}
#endif

};

#ifndef DOXYGEN
}  // ScriptLib
#endif

Q_DECLARE_METATYPE(ScriptLib::Clipboard *)
Q_DECLARE_METATYPE(ScriptLib::Clipboard::Mode)
