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


//! \fn Date clone()
//! \memberof Date
//! Creates a deep copy of Date object.
Date.prototype.clone = function(){
   return new Date(+this);
};

//! \fn Date addSeconds(seconds)
//! \memberof Date
//! Adds `seconds` to current date and returns new Date object.
//! `seconds` can be negative to subtract from current time and can be in any integer value range.
Date.prototype.addSeconds = function(seconds) {
   if (!seconds) return this;
   let date = this;
   date.setSeconds(date.getSeconds() + seconds);
   return date;
};

//! \fn Date addMinutes(seconds)
//! \memberof Date
//! Adds `minutes` to current date and returns new Date object.
//! `minutes` can be negative to subtract from current time and can be in any integer value range.
Date.prototype.addMinutes = function(minutes) {
   if (!minutes) return this;
   let date = this;
   date.setMinutes(date.getMinutes() + minutes);
   return date;
};

//! \fn Date addHours(seconds)
//! \memberof Date
//! Adds `hours` to current date and returns new Date object.
//! `hours` can be negative to subtract from current time and can be in any integer value range.
Date.prototype.addHours = function(hours) {
   if (!hours) return this;
   let date = this;
   date.setHours(date.getHours() + hours);
   return date;
};

//! \fn Date addDays(days)
//! \memberof Date
//! Adds `days` to current date and returns new Date object.
//! `days` can be negative to subtract from current date and can be in any integer value range.
Date.prototype.addDays = function(days) {
   if (!days) return this;
   let date = this;
   date.setDate(date.getDate() + days);
   return date;
};

//! \fn Date addMonths(months)
//! \memberof Date
//! Adds `months` to current date and returns new Date object.
//! `months` can be negative to subtract from current date and can be in any integer value range.
Date.prototype.addMonths = function(months) {
   if (!months) return this;
   let date = this;
   date.setMonth(date.getMonth() + months);
   return date;
};

//! \fn Date addYears(years)
//! \memberof Date
//! Adds `years` to current date and returns new Date object.
//! `years` can be negative to subtract from current date and can be in any integer value range.
Date.prototype.addYears = function(years) {
   if (!years) return this;
   let date = this;
   date.setYear(date.getYear() + years);
   return date;
};

//! \fn Date compareToDate(otherDate)
//! \memberof Date
//! Compares this Date to `otherDate` based only on the year, month and day parts, ignoring time part.
//! Returns -1 if `otherDate` is earlier than this date, 0 if the dates match, and 1 if `otherDate` is later than this date.
Date.prototype.compareToDate = function (otherDate)  {
  if (!otherDate)
    return 1;
  if (this.getFullYear() !== date.getFullYear())
    return this.getFullYear() > date.getFullYear() ? -1 : 1;
  if (this.getMonth() !== date.getMonth())
    return this.getMonth() > date.getMonth() ? -1 : 1;
  if (this.getDate() !== date.getDate())
    return this.getDate() > date.getDate() ? -1 : 1;
  return 0;
};


/*!

   \fn string format(format)
   \memberof Date
   Formats this date according the specified .NET-style `format` string.
   This function emulates the .NET `DateTime.ToString()` method.

   For example:
   ```js
   console.log(new Date().format("yyyy MMM DD @ HH:mm::ss"));  // Prints: "2022 Jan 05 @ 22:03:55"
   ```
   See formatting string reference at
   https://learn.microsoft.com/en-us/dotnet/standard/base-types/standard-date-and-time-format-strings#table-of-format-specifiers
   \sa String.format(), toLocaleString(), toLocaleDateString(), toLocaleTimeString()


   \fn Date fromLocaleDateString(locale, dateString, format)
   \memberof Date
   \static
   Converts the date string `dateString` to a `Date` object using given `locale` and `format`.
   \param locale A Locale object, obtained with the global `locale()` method.
   \param dateString The input string to parse
   \param format A Locale [format constant](@ref locale-format-type) or string [format specifier](@ref locale-date-format).

   If `format` is not specified, [Locale.LongFormat](@ref locale-format-type) will be used. <br />
   If `locale` is not specified, the default locale will be used.

   The following example shows a datetime being parsed from a datetime string in a certain format using the default locale

   ```js
   var dateString = "Freitag 2022-12-09";
   var date = Date.fromLocaleString(locale("de_DE"), dateString, "dddd yyyy-MM-dd");
   ```
   \sa locale(), Locale


   \fn Date fromLocaleString(locale, dateTimeString, format)
   \memberof Date
   \static
   Converts the date string `dateTimeString` to a `Date` object using given `locale` and `format`.
   \param locale A Locale object, obtained with the global `locale()` method.
   \param dateTimeString The input string to parse
   \param format A Locale [format constant](@ref locale-format-type) or string [format specifier](@ref locale-date-format).

   If `format` is not specified,  [Locale.LongFormat](@ref locale-format-type) will be used. <br />
   If `locale` is not specified, the default locale will be used.

   The following example shows a datetime being parsed from a datetime string in a certain format using the default locale

   ```js
   var dateTimeString = "Freitag 2022-12-09 22:56:06";
   var date = Date.fromLocaleString(locale(), dateTimeString, "dddd yyyy-MM-dd hh:mm:ss");
   ```
   \sa locale(), Locale


   \fn Date fromLocaleTimeString(locale, timeString, format)
   \memberof Date
   Converts the date string `timeString` to a Date object using given `locale` and `format`.
   \param locale A Locale object, obtained with the global `locale()` method.
   \param timeString The input string to parse
   \param format A Locale [format constant](@ref locale-format-type) or string [format specifier](@ref locale-date-format).
   \static

   If `format` is not specified, [Locale.LongFormat](@ref locale-format-type) will be used. <br />
   If `locale` is not specified, the default locale will be used.

   The following example shows a datetime being parsed from a datetime string in a certain format using the German locale:
   ```js
   Date date = Date.fromLocaleTimeString(locale("de_DE"), "22:56:06", "HH:mm:ss");
   ```
   \sa locale(), Locale


   \fn string toLocaleDateString(locale, format)
   \memberof Date
   Converts the Date to a string containing the date suitable for the specified `locale` in the specified `format`.
   \param locale A Locale object, obtained with the global `locale()` method.
   \param format A Locale [format constant](@ref locale-format-type) or string [format specifier](@ref locale-date-format).

   If `format` is not specified, [Locale.LongFormat](@ref locale-format-type) will be used. <br />
   If `locale` is not specified, the default 'locale()` will be used. <br />

   The following example shows the current date formatted for the German locale:
   ```js
   new Date().toLocaleDateString(locale("de_DE"))
   // "Freitag, 9. Dezember 2022"
   ```
   \sa locale(), Locale


   \fn string toLocaleString(locale, format)
   \memberof Date
   Converts the Date to a string containing the date and time suitable for the specified `locale` in the specified `format`.
   \param locale A Locale object, obtained with the global `locale()` method.
   \param format A Locale [format constant](@ref locale-format-type) or string [format specifier](@ref locale-date-format).

   If `format` is not specified,  [Locale.LongFormat](@ref locale-format-type) will be used.
   If `locale` is not specified, the default 'locale()` will be used.

   The following example shows the current date and time formatted for the German locale:
   ```js
   new Date().toLocaleString(locale("de_DE", dddd, d. MMMM yyyy HH:mm:ss))
   // "Freitag, 9. Dezember 2022 09:05:27"
   ```
   \sa locale(), Locale


   \fn string toLocaleTimeString(locale, format)
   \memberof Date
   Converts the Date to a string containing the time suitable for the specified `locale` in the specified `format`.
   \param locale A Locale object, obtained with the global `locale()` method.
   \param format A Locale [format constant](@ref locale-format-type) or string [format specifier](@ref locale-date-format).

   If `format` is not specified, [Locale.LongFormat](@ref locale-format-type) will be used.
   If `locale` is not specified, the default locale will be used.

   The following example shows the current date and time formatted for the German locale:
   ```js
   new Date().toLocaleTimeString(locale("de_DE"))
   // "19:05:27 Eastern Standard Time"
   ```
   \sa locale(), Locale

*/
