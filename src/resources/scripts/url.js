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

// documentation in ./url.dox

URL.fromLocalPath = Util.urlFromLocalPath;
URL.fromPathOrUrl = Util.urlFromInput;
URL.isEmpty       = Util.urlIsEmpty;
URL.isLocalPath   = Util.urlIsLocalPath;
URL.isRelative    = Util.urlIsRelative;
URL.isValid       = Util.urlIsValid;
URL.resolved      = Util.urlResolved;
URL.scheme        = Util.urlScheme;
URL.toLocalPath   = Util.urlToLocalPath;
URL.url           = Qt.url;

if (!URL.parse) {
	URL.parse = function(url, base) {
		try {
			if (typeof base == 'undefined')
				return new URL(url);
			return new URL(url, base);
		}
		catch(_) {
			return null;
		}
	}
}

if (!URL.canParse) {
	URL.canParse = function(url, base) {
		try {
			if (typeof base == 'undefined')
				return !!new URL(url);
			return !!new URL(url, base);
		}
		catch(_) {
			return false;
		}
	}
}

{
	const proto = Object.getPrototypeOf(URL.__proto__);

	Object.defineProperty(proto, 'isLocalPath', {
		enumerable: true,
		get() { return Util.urlIsLocalPath(this); }
	});

	Object.defineProperty(proto, 'isRelative', {
		enumerable: true,
		get() { return Util.urlIsRelative(this); }
	});

	Object.defineProperty(proto, 'isValid', {
		enumerable: true,
		get() { return Util.urlIsValid(this); }
	});

	proto.resolved = function(relative) { return Util.urlResolved(this, relative); }
	proto.toLocalPath = function()      { return Util.urlToLocalPath(this); }

	Object.setPrototypeOf(URL.__proto__, proto);
}
