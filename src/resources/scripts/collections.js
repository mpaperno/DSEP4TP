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
	\fn string fromBase64(data)
	\memberof ArrayBuffer
	\static
	Creates and returns a new `ArrayBuffer` from the contents of Base64-encoded string `data`.
  This is a static function. Use it like `var buffer = ArrayBuffer.fromBase64(data)`.

	Equivalent to `atob(data)`.
*/
ArrayBuffer.fromBase64 = function(data) { return Util.fromBase64(data); };

/*!
	\fn string toBase64()
	\memberof ArrayBuffer
	Returns the contents of this buffer as a Base64-encoded string.
	Equivalent to `btoa(buffer)`.
*/
ArrayBuffer.prototype.toBase64 = function() { return Util.toBase64(this); };

/*!
  \fn string toHex(char separator)
  \memberof ArrayBuffer
  Returns the contents of this buffer as a hex encoded string.
  \param separator Optional **single** separator character to place between each byte in the output.
	The separator must be a single character otherwise the function will fail.

  For example:
  ```js
  let macAddress = new Uint8Array([0x12, 0x34, 0x56, 0xab, 0xcd, 0xef]).buffer;
  macAddress.toHex(':');  // returns "12:34:56:ab:cd:ef"
  macAddress.toHex();     // returns "123456abcdef"
  ```
*/
ArrayBuffer.prototype.toHex = function(separator) { return Util.baToHex(this, separator || 0); };

/*
	\fn string toBase64()
	\memberof TypedArray
	Returns the contents of this array's buffer as a Base64-encoded string.
	Equivalent to `btoa(array.buffer)`.
*/
//TypedArray.prototype.toBase64 = function() { return Util.toBase64(this.buffer); };
