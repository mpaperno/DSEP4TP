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

/*!
	\fn string arg(value)
	\memberof String
	Returns a copy of this string with the lowest numbered place marker replaced by value.
	Place markers begin with a `%` symbol followed by a number in the 1 through 99 range, i.e. %1, %2, ..., %99.

	The following example prints "There are 20 items":
	```js
	var message = "There are %1 items"
	var count = 20
	console.log(message.arg(count))
	```
	\sa `String.format()`, `sprintf()`



	\fn string format(format, args)
	\memberof String
	Formats `format` string using the provided arguments in `args`, if any, using .NET-style formatting syntax.
	\param format The formatting string used to format the additional arguments.
	\param args Zero or more arguments for the format string.

	Formatting is done using format strings almost completely compatible with the String.Format method in Microsoft .NET Framework.

	For example:
	```js
	String.format(
			"Welcome back, {0}! Last seen {1:M}",
			"John Doe", new Date(1985, 3, 7, 12, 33)
	);
	// output: "Welcome back, John Doe! Last seen April 07"
	```

	There is also a convenience global function available as an alias, named simply `Format()`. It works identically to `String.format()`
	but is shorter to type.

	See formatting string reference at <br />
	https://learn.microsoft.com/en-us/dotnet/standard/base-types/formatting-types

	This extension is originally from the [String.format for JavaScript](https://github.com/dmester/sffjs) project.
	There are some examples and references available at that site. The original `sffjs` object referenced in their documentation
	is available in this script engine as `Sffjs`.

	\sa `Number.format()`, `Date.format()`, and `sprintf()` for "printf-style" string formatting.

*/

/*!
	\fn string simplified()
	\memberof String
	Returns this string with leading, trailing, and all redundant internal whitespace removed. contents of this byte array as a Base64-encoded string.
	Example:
	```js
  var str = "  lots\t of\nwhitespace\r\n ";
  str = str.simplified();
  // str == "lots of whitespace";
	```
*/

/*!
	\fn string appendLine(String line, int maxLines, separator = '\n')
	\memberof String
	Appends string `line` to current string, and returns a new block with up to `maxLines` lines. 
	If `maxLines` is zero or negative, returns the full resulting string.
	The newline (`\n`) is used as separator by default, or a custom separator string can be specified in `separator` argument.
*/

/*!
	\fn string appendLine(String text, String line, int maxLines, String separator = '\n')
	\memberof String
	\static
	Appends string `line` to `text` and returns a new block with up to `maxLines` lines.
	Static version of `String.prototype.appendLine()`, above. Used, for example, like this: `String.appendLine("Original string", "New line to append", 4)`
*/

/*!
	\fn string getLines(int maxLines, int fromLine = 0, String separator = '\n')
	\memberof String
	Splits string `text` into lines based on `separator` and returns `maxLines` of text starting at `fromLine` (zero-based index).
	Default `maxLines` is 1,  `fromLine` is zero (the start) and default `separator` is "\n". Specify a negative `fromLine` to count lines from the end
	instead of the beginning (so `-1` returns one line from the end). The result may be the complete input if there were fewer then `maxLines` found in it.
*/

/*!
	\fn string getLines(String text, int maxLines, int fromLine = 0, String separator = '\n')
	\memberof String
	\static
	Splits string `text` into lines based on `separator` and returns `maxLines` of text starting at `fromLine` (zero-based index).
	Static version of `String.prototype.getLines()`, above. Used, for example, like this: `String.getLines("Text\nWith some\nLines\n", 1, -1)`
*/

String.prototype.simplified = function() { return Util.stringSimplify(this); };
String.prototype.trimStart = function() { return Util.stringTrimLeft(this); };
String.prototype.trimEnd = function() { return Util.stringTrimRight(this); };
String.prototype.appendLine = function(line, maxLines, separator = '\n') { return Util.appendLine(this, line, maxLines, separator); };
String.prototype.getLines = function(maxLines, fromLine = 0, separator = '\n') { return Util.getLines(this, maxLines, fromLine, separator); };

String.appendLine = function(text, line, maxLines, separator = '\n') { return Util.appendLine(text, line, maxLines, separator); };
String.getLines = function(text, line, maxLines, separator = '\n') { return Util.getLines(text, maxLines, fromLine, separator); };
