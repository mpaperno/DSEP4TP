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

//! \fn number clamp(number value, number min, number max)
//! \memberof Math
//! Constrain `value` between `min` and `max` values.
Math.clamp = function(value, min, max) {
	return Math.min(max, Math.max(min, value));
};

//! \fn number constrain(number value, number min, number max)
//! \memberof Math
//! Alias for Math.clamp()
Math.constrain = Math.clamp;

//! \fn number roundTo(number value, number precision)
//! \memberof Math
//! Round `value` to `precision` decimal places.
Math.roundTo = function(value, precision) {
  return Math.round(value * Math.pow(10, precision)) / Math.pow(10, precision);
};

//! \fn number toDegrees(number radians)
//! \memberof Math
//! Convert `radians` to degrees.
Math.toDegrees = function(radians) {
	return radians * 180 / Math.PI;
};

//! \fn number toRadians(number degrees)
//! \memberof Math
//! Convert `degrees` to radians.
Math.toRadians = function(degrees) {
	return degrees / 180 * Math.PI;
};


//! \fn number percentOfRange(number percentValue, number rangeMin, number rangeMax)
//! \memberof Math
//! Scales `percentValue` into the numeric range specified by `rangeMin` and `rangeMax` and returns the new value.
//! In other words, "what value does this percentage represent within this range." This may return a value outside
//! the given range if `percentValue` is greater or less than 100. `percentValue` is always treated as absolute.
//! \n Either of `rangeMin` or `rangeMax` can be the larger number. The returned value will be proportional in either case.
Math.percentOfRange = function(value, rangeMin, rangeMax)
{
  return ((rangeMax - rangeMin) * 0.01 * Math.abs(value)) + rangeMin;
};

//! \fn number rangeValueToPercent(number value, number rangeMin, number rangeMax)
//! \memberof Math
//! Returns the percentage of the `rangeMin` through `rangeMax` numeric range represented by `value`.
//! In other words, "what percentage does this value represent of the given range."  This may return
//! a percentage greater than 100 if `value` falls outside of `rangeMin` and `rangeMax`.
//! \n Either of `rangeMin` or `rangeMax` can be the larger number. The returned percentage will be proportional in either case.
Math.rangeValueToPercent = function(value, rangeMin, rangeMax)
{
  const dlta = rangeMax - rangeMin;
  const scale = dlta == 0.0 ? 100.0 : 100.0 / dlta;
  return (value - rangeMin) * scale;
};


// Math method aliases
var abs = Math.abs,
	acos = Math.acos,
	acosh = Math.acosh,
	asin = Math.asin,
	asinh = Math.asinh,
	atan = Math.atan,
	atan2 = Math.atan2,
	atanh = Math.atanh,
	cbrt = Math.cbrt,
	ceil = Math.ceil,
	clz32 = Math.clz32,
	cos = Math.cos,
	cosh = Math.cosh,
	exp = Math.exp,
	expm1 = Math.expm1,
	floor = Math.floor,
	fround = Math.fround,
	hypot = Math.hypot,
	imul = Math.imul,
	log = Math.log,
	log10 = Math.log10,
	log1p = Math.log1p,
	log2 = Math.log2,
	max = Math.max,
	min = Math.min,
	pow = Math.pow,
	random = Math.random,
	round = Math.round,
	sign = Math.sign,
	sin = Math.sin,
	sinh = Math.sinh,
	sqrt = Math.sqrt,
	tan = Math.tan,
	tanh = Math.tanh,
	trunc = Math.trunc,
	clamp = Math.clamp,
	constrain = Math.constrain,
	roundTo = Math.roundTo,
	toDegrees = Math.toDegrees,
	toRadians = Math.toRadians,
	percentOfRange = Math.percentOfRange,
	rangeValueToPercent = Math.rangeValueToPercent
;
