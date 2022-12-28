// Import clipboard functions from Clipboard example module.
import { valueToClipboard, clipboardToValue } from "../Clipboard/clipboard.mjs";

// Stores the current color value. Not exported, access via `color()` and `setColor()` functions.
var _color = _color || new Color('#FFFFFF');

// Returns the current Color object, on which various methods can be invoked directly.
// See documentation at: https://dse.tpp.max.paperno.us/class_color.html
export function color()
{
	return _color;
}

// Returns current color in "Touch Portal" format of #AARRGGBB format,
// suitable for use in an "Change visuals by plug-in state" action.
export function tpcolor()
{
	return _color.tpcolor();
}

// Sets the current color to `color` which can be:
// - An hex string in "#RRGGBB[AA]" format, with or w/out the leading "#";
// - A color name from CSS specification, eg "red", "darkblue", etc.;
// - Any other valid CSS-style color specifier string, eg. "rgba(255, 0, 0, .5)" or "hsl(0, 100%, 50%)" or "hsva(0, 100%, 50%, .5)", etc.;
// - An object representing rgb[a]/hsl[a]/hsv[a] values, eg. `{ r: 255, g: 0, b: 0 }` or  `{ h: 0, s: 1, l: .5, a: .75 }` or `{ h: 0, s: 100, v: 100 }`;
// - Another Color object instance.
export function setColor(color)
{
	_color = new Color(color);
	return tpcolor();
}

// Set the red component of the current color to a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function setRed(percent)
{
	_color._r = percentToShort(percent);
	return tpcolor();
}

// Set the green component of the current color to a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function setGreen(percent)
{
	_color._g = percentToShort(percent);
	return tpcolor();
}

// Set the blue component of the current color to a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function setBlue(percent)
{
	_color._b = percentToShort(percent);
	return tpcolor();
}

// Set the hue component of the current color to a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function setHue(percent)
{
	var hsl = _color.toHsl();
	hsl.h = percentToDegrees(percent);
	return setColor(hsl);
}

// Set the saturation component of the current color to a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function setSaturation(percent)
{
	var hsl = _color.toHsl();
	if (percent === 1)
		percent += .01;
	hsl.s = percent;
	return setColor(hsl);
}

// Set the lightness component of the current color to a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function setLightness(percent)
{
	var hsl = _color.toHsl();
	if (percent === 1)
		percent += .01;
	hsl.l = percent;
	return setColor(hsl);
}

// Set the value component of the current color to a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function setValue(percent)
{
	var hsv = _color.toHsv();
	if (percent === 1)
		percent += .01;
	hsv.v = percent;
	return setColor(hsv);
}

// Set the alpha (transparency) component of the current color to a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function setAlpha(alpha)
{
	_color.setAlpha(alpha * 0.01);
	return tpcolor();
}

// Adjustment (step increment/decrement) functions.

// Changes the red component of the current color by a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function adjustRed(amount)
{
	return setRed(_color._r + percentToShort(amount));
}

// Changes the green component of the current color by a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function adjustGreen(amount)
{
	return setGreen(_color._g + percentToShort(amount));
}

// Changes the blue component of the current color by a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function adjustBlue(amount)
{
	return setBlue(_color._b + percentToShort(amount));
}

// Changes the alhpa component of the current color by a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function adjustAlpha(amount)
{
	return setAlpha(_color.alpha() + amount * 0.01);
}

// Changes the hue component of the current color by a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function adjustHue(amount)
{
	return setHue(_color.toHsl().h + percentToDegrees(amount));
}

// Changes the saturation component of the current color by a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function adjustSaturation(amount)
{
	return setSaturation(_color.toHsl().s + amount);
}

// Changes the lightness component of the current color by a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function adjustLightness(amount)
{
	return setLightness(_color.toHsl().l + amount);
}

// Changes the value component of the current color by a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function adjustValue(amount)
{
	return setValue(_color.toHsv().v + amount);
}

// Clipboard functions. Uses Clipboard example module, imported at top.
// NOTE: Linux requires `xclip` utility installed.

// Tries to set a new color from a clipboard value. Accepted formats are same as described in `setColor()` above.
// In case the clipboard does not hold a valid color value, this function will log a warning message and return
// the current color instead (no changes to current color will be made).
export function fromClipboard()
{
	const val = clipboardToValue();
	// console.log("Clipboard value:", val);
	const color = new Color(val);
	if (color.isValid())
		return setColor(color);
	console.error("Clipboard contents were not a valid color.");
	return tpcolor();
}

// Copy current color in "#RRGGBBAA" format to the clipboard.
export function copyRgba()
{
	valueToClipboard(_color.rgba());
}

// Copy current color in "#RRGGBB" format to the clipboard.
export function copyHex()
{
	valueToClipboard(_color.hex());
}

// Copy current color in "#AARRGGBB" format to the clipboard.
export function copyArgb()
{
	valueToClipboard(_color.argb());
}

// Copy current color in "rgb(r, g, b [, a])" format to the clipboard.
// The alpha component will be included if it is not fully opaque (< 1.0).
// eg: rgb(214, 173, 46) or rgba(214, 173, 46, 0.61)
export function copyRgb()
{
	valueToClipboard(_color.rgb());
}

// Copy current color in "hsv[a](h, s%, v% [, a])" format to the clipboard.
// The alpha component will be included if it is not fully opaque (< 1.0).
// eg: hsv(45, 79%, 84%) or hsva(45, 79%, 84%, 0.61)
export function copyHsv()
{
	valueToClipboard(_color.hsv());
}

// Copy current color in "hsl[a](h, s%, v% [, a])" format to the clipboard.
// The alpha component will be included if it is not fully opaque (< 1.0).
// eg: hsl(45, 67%, 51%) or hsla(45, 67%, 51%, 0.61)
export function copyHsl()
{
	valueToClipboard(_color.hsl());
}


// internal utility, not exported
function percentToShort(value) { return roundTo(value.clamp(0, 100) * 2.55, 0) }
function percentToDegrees(value) { return roundTo(value.clamp(0, 100) * 3.6, 0) }
