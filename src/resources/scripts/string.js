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

// documentation in ./string.dox

String.prototype.simplified = function() { return Util.stringSimplify(this); };
String.prototype.trimStart = function() { return Util.stringTrimLeft(this); };
String.prototype.trimEnd = function() { return Util.stringTrimRight(this); };
String.prototype.appendLine = function(line, maxLines, separator = '\n') { return Util.appendLine(this, line, maxLines, separator); };
String.prototype.getLines = function(maxLines, fromLine = 0, separator = '\n') { return Util.getLines(this, maxLines, fromLine, separator); };

String.prototype.elideLeft = function(maxLen) {
    if (this.length <= maxLen)
        return this;
    return "…" + this.slice(-maxLen);
};

String.prototype.elideRight = function(maxLen) {
    if (this.length <= maxLen)
        return this;
    return this.slice(0, maxLen+1) + "…";
};

String.prototype.elideMiddle = function(maxLen) {
    if (this.length <= maxLen)
        return this;
    return this.slice(0, Math.ceil(maxLen / 2) + 1) + "…" + this.slice(-Math.floor(maxLen / 2));
};

String.appendLine = function(text, line, maxLines, separator = '\n') { return Util.appendLine(text, line, maxLines, separator); };
String.getLines = function(text, line, maxLines, separator = '\n') { return Util.getLines(text, maxLines, fromLine, separator); };
String.elideLeft  = function(str, maxLen) { return String(str).elideLeft(maxLen); };
String.elideRight = function(str, maxLen) { return String(str).elideRight(maxLen); };
String.elideMiddle = function(str, maxLen) { return String(str).elideMiddle(maxLen); };
