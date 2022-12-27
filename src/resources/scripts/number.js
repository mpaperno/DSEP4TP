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

//! \fn number round(number precision)
//! \memberof Number
//! Round value to `precision` decimal places.
Number.prototype.round = function(precision = 0) {
	return Math.roundTo(this, precision);
};

//! \fn number clamp(number min, number max)
//! \memberof Number
//! Constrain value to between `min` and `max` values.
Number.prototype.clamp = function(min, max) {
	return Math.clamp(this, min, max);
};

//! \fn number constrain(number min, number max)
//! \memberof Number
//! Alias for Number.clamp()
Number.prototype.constrain = Number.prototype.clamp;

/*!

	\fn string format(format)
	\memberof Number
	Formats this number according the specified .NET-style `format` string.
	This function emulates the .NET numeric types' `*.ToString()` methods (eg. `Int32.ToString()`).
		For example:
		```js
		console.log((1234.5678).format("F2"));  // Prints: "1234.56"
		```
	See formatting string reference at
	https://learn.microsoft.com/en-us/dotnet/standard/base-types/standard-numeric-format-strings#standard-format-specifiers
	\sa String.format(), toLocaleString(), toLocaleCurrencyString()


	\fn Date fromLocaleString(locale, numberString)
	\memberof Number
	Returns a Number by parsing `numberString` using the conventions of the supplied locale.
  \param locale A Locale object, obtained with the global `locale()` method. If not specified, the default `locale()` will be used.
	\param numberString The input string to parse.

	For example, using the German locale:
	```js
	var german = locale("de_DE");
	var d;
	d = Number.fromLocaleString(german, "1234,56")   // d == 1234.56
	d = Number.fromLocaleString(german, "1.234,56") // d == 1234.56
	d = Number.fromLocaleString(german, "1234.56")  // throws exception
	d = Number.fromLocaleString(german, "1.234")    // d == 1234.0
	```
	\sa locale(), Locale


	\fn string toLocaleString(locale, format, precision)
	\memberof Number
	Converts the Number to a string suitable for the specified `locale` in the specified `format`, with the specified `precision`.
	\param locale A Locale object, obtained with the global `locale()` method. If not specified, the default `locale()` will be used.
	\param format One of the format specifier strings shown below. Default is `'f'`
	\param precision Number of decimal places to show. Default is 2 decimals.

	Valid formats are:

			'f' Decimal floating point, e.g. 248.65
			'e' Scientific notation using e character, e.g. 2.4865e+2
			'E' Scientific notation using E character, e.g. 2.4865E+2
			'g' Use the shorter of e or f
			'G' Use the shorter of E or f

	The following example shows a number formatted for the German locale:
	```js
	console.log(Number(4742378.423).toLocaleString(locale("de_DE"));
	// 4.742.378,423
	```

	You can apply toLocaleString() directly to constants, provided the decimal is included in the constant, e.g.
	```js
	123.0.toLocaleString(locale("de_DE")) // OK
	123..toLocaleString(locale("de_DE"))  // OK
	123.toLocaleString(locale("de_DE"))   // fails
	```
	\sa locale(), Locale


	\fn string toLocaleCurrencyString(locale, symbol)
	\memberof Number
	Converts the Number to a currency using the currency and conventions of the specified locale.
	If `symbol` is specified it will be used as the currency symbol.

	```js
	console.log(Number(2378.45).toLocaleCurrencyString(locale("de_DE")));
	// 2.378,45€
	console.log(Number(2378.45).toLocaleCurrencyString(locale("de_DE", "\u2133")));
	// 2.378,45ℳ
	```

	\sa locale(), Locale
*/
