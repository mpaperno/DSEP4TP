// Import clipboard functions from Clipboard example module.  The import path assumes both examples are in the same folder.
import { valueToClipboard, clipboardToValue } from "clipboard.mjs";

// This script creates Touch Portal States dynamically as needed.
// This variable sets the title under which all the created States are grouped.
// The group will appear as a child of the Dynamic Script Engine main plugin grouping in TP menus.
export var STATES_GROUP_NAME = "Color Picker";

// Template for formatted color value. .NET String.Format() style.
// Template values are: 0 = red; 1 = blue; 2 = green; 3 = alpha; 4 = hue; 5 = saturation; 6 = value; 7 = lightness
export var FORMATTED_COLOR_TEMPLATE = "R: {0:000}\nG: {1:000}\nB: {2:000}\nA: {3:000%}";

// Returns the current Color object, on which various methods can be invoked directly.
// See documentation at: https://dse.tpp.max.paperno.us/class_color.html
export function color()
{
	return _data.color;
}

// Returns current color in "Touch Portal" format of #AARRGGBB format,
// suitable for use in an "Change visuals by plug-in state" action.
export function tpcolor()
{
	return color().tpcolor();
}

// Sets the current color to `color` which can be:
// - An hex string in "#RRGGBB[AA]" format, with or w/out the leading "#";
// - A color name from CSS specification, eg "red", "darkblue", etc.;
// - Any other valid CSS-style color specifier string, eg. "rgba(255, 0, 0, .5)" or "hsl(0, 100%, 50%)" or "hsva(0, 100%, 50%, .5)", etc.;
// - An object representing rgb[a]/hsl[a]/hsv[a] values, eg. `{ r: 255, g: 0, b: 0 }` or  `{ h: 0, s: 1, l: .5, a: .75 }` or `{ h: 0, s: 100, v: 100 }`;
// - Another Color object instance.
export function setColor(color)
{
	_data.color = new Color(color);
	update();
	return tpcolor();
}

// Sets the current color from an "#AARRGGBB" format string (this format is used by Touch Portal
// for button background and text colors in "Change visuals by plug-in state" actions).
export function setColorArgb(color)
{
	const rgba = color.length === 9 ? color.replace(/^#([0-9a-z]{2})([0-9a-z]{6})$/i, '#$2$1') : color;
	const newColor = new Color(rgba);
	if (newColor.isValid())
		return setColor(newColor);
	console.error(`'${color}' is not a valid color.`);
	return tpcolor();
}

// Returns the current text Color object.
export function textColor()
{
	return _data.textColor;
}

// Sets the current text color and updates the "Text Color" State with the color as an "#RRGGBB" format string.
// See `setColor()` for allowed format specifics.
export function setTextColor(color)
{
	_data.textColor = new Color(color);
	sendTextColor();
}

// Color channel/component setting functions.

// Set the red component of the current color to a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function setRed(percent)
{
	color()._r = percentToShort(percent);
	update();
	return tpcolor();
}

// Set the green component of the current color to a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function setGreen(percent)
{
	color()._g = percentToShort(percent);
	update();
	return tpcolor();
}

// Set the blue component of the current color to a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function setBlue(percent)
{
	color()._b = percentToShort(percent);
	update();
	return tpcolor();
}

// Set the alpha (transparency) component of the current color to a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function setAlpha(alpha)
{
	color().setAlpha(alpha * 0.01);
	update();
	return tpcolor();
}

// Set the hue component of the current color to a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function setHue(percent)
{
	var hsl = color().toHsl();
	hsl.h = percentToDegrees(percent);
	return setColor(hsl);
}

// Set the saturation component of the current color to a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function setSaturation(percent)
{
	var hsl = color().toHsl();
	if (percent === 1)
		percent += .01;
	hsl.s = percent;
	return setColor(hsl);
}

// Set the lightness component of the current color to a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function setLightness(percent)
{
	var hsl = color().toHsl();
	if (percent === 1)
		percent += .01;
	hsl.l = percent;
	return setColor(hsl);
}

// Set the value component of the current color to a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function setValue(percent)
{
	var hsv = color().toHsv();
	if (percent === 1)
		percent += .01;
	hsv.v = percent;
	return setColor(hsv);
}

// Adjustment (step increment/decrement) functions.

// Changes the red component of the current color by a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function adjustRed(amount)
{
	return setRed(color()._r + percentToShort(amount));
}

// Changes the green component of the current color by a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function adjustGreen(amount)
{
	return setGreen(color()._g + percentToShort(amount));
}

// Changes the blue component of the current color by a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function adjustBlue(amount)
{
	return setBlue(color()._b + percentToShort(amount));
}

// Changes the alhpa component of the current color by a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function adjustAlpha(amount)
{
	return setAlpha(color().alpha() + amount * 0.01);
}

// Changes the hue component of the current color by a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function adjustHue(amount)
{
	return setHue(color().toHsl().h + percentToDegrees(amount));
}

// Changes the saturation component of the current color by a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function adjustSaturation(amount)
{
	return setSaturation(color().toHsl().s + amount);
}

// Changes the lightness component of the current color by a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function adjustLightness(amount)
{
	return setLightness(color().toHsl().l + amount);
}

// Changes the value component of the current color by a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
export function adjustValue(amount)
{
	return setValue(color().toHsv().v + amount);
}

// Color complement/series array generators.

// Generates a color complementary to the current color.
// Updates/creates 1 State based on current instance name + "_complement".
export function complement()
{
	sendColorsArray([color().complement()], "complement");
}

// Generates 2 colors which are split-complementary to the current color.
// Updates/creates 2 States based on current instance name + "_splitcomp_1" and  + "_splitcomp_2".
export function splitcomplement()
{
	sendColorsArray(color().splitcomplement().slice(1), "splitcomp");
}

// Enables or disables the automatic updating of complement, split complement, and last selected series based on the current color.
export function autoUpdateSeries(enable = true)
{
	if (_data.autoUpdate === enable)
		return;

	_data.autoUpdate = enable;
	updateSeries();

	// Send a State update with the new value of the autoUpdateSeries setting (eg. to use as button visual change trigger).
	// Create the State first if necessary.
	if (!_data.autoUpdateEnabledState) {
		_data.autoUpdateEnabledState = DSE.instanceStateId() + '_autoUpdateSeries';
		const stateDescript = DSE.INSTANCE_NAME + " Auto Update Complement/Series";
		// the default should be 0, but due to a "bug" in TP <= v3.1 we need to force it to a blank value first.
		TP.stateCreate(_data.autoUpdateEnabledState, STATES_GROUP_NAME, stateDescript, "");
	}
	TP.stateUpdateById(_data.autoUpdateEnabledState, _data.autoUpdate ? "1" : "0");
}

// Toggles the automatic series updates. Convenience for `autoUpdateSeries()` based on current setting.
export function autoUpdateSeriesToggle()
{
	autoUpdateSeries(!_data.autoUpdate);
}

// This function clears the current color series (polyad, analogous, mono),
// sending an array of `n` transparent colors instead. If `n` is -1 (default) then the count from the last
// requested series is used.  If `n` is 0, then no transparent colors are sent (the last series color state values stay intact).
// Can be used to simply clear any current series colors or to keep automatic updates of complement/split comp but w/out series updates.
export function clearCurrentSeries(n = -1)
{
	_data.lastSeries.t = "";
	var colors = [];
	if (n < 0)
		n = _data.lastSeries.n;
	for (let i=0; i < n; ++i)
		colors.push(new Color('#00000000'));
	sendColorsArray(colors);
}

// Generates up to `n` number of colors harmonious with the current color. Updates/creates States based on current instance name and numeric suffix.
export function polyad(n)
{
	_data.lastSeries.t = "polyad";
	_data.lastSeries.n = n;
	sendColorsArray(color().polyad(n+1).slice(1));
}

// Generates up to `n` number of colors analogous to the current color. Updates/creates States based on current instance name and numeric suffix.
export function analogous(n, slices = 30)
{
	_data.lastSeries.t = "analogous";
	_data.lastSeries.n = n;
	_data.lastSeries.s = slices;
	sendColorsArray(color().analogous(n+1, slices).slice(1));
}

// Generates up to `n` number of monochromatic colors based on the current color. Updates/creates States based on current instance name and numeric suffix.
export function monochromatic(n)
{
	_data.lastSeries.t = "monochromatic";
	_data.lastSeries.n = n;
	sendColorsArray(color().monochromatic(n+1).slice(1));
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
	valueToClipboard(color().rgba());
}

// Copy current color in "#RRGGBB" format to the clipboard.
export function copyHex()
{
	valueToClipboard(color().hex());
}

// Copy current color in "#AARRGGBB" format to the clipboard.
export function copyArgb()
{
	valueToClipboard(color().argb());
}

// Copy current color in "rgb(r, g, b [, a])" format to the clipboard.
// The alpha component will be included if it is not fully opaque (< 1.0).
// eg: rgb(214, 173, 46) or rgba(214, 173, 46, 0.61)
export function copyRgb()
{
	valueToClipboard(color().rgb());
}

// Copy current color in "hsv[a](h, s%, v% [, a])" format to the clipboard.
// The alpha component will be included if it is not fully opaque (< 1.0).
// eg: hsv(45, 79%, 84%) or hsva(45, 79%, 84%, 0.61)
export function copyHsv()
{
	valueToClipboard(color().hsv());
}

// Copy current color in "hsl[a](h, s%, v% [, a])" format to the clipboard.
// The alpha component will be included if it is not fully opaque (< 1.0).
// eg: hsl(45, 67%, 51%) or hsla(45, 67%, 51%, 0.61)
export function copyHsl()
{
	valueToClipboard(color().hsl());
}

// Text color functions

// Set the hue component of the current text color to a percentage value (0 - 100) and returns the new color as an #AARRGGBB string.
// If `percent` is zero, the text color is placed into "auto" mode which will show either light or dark text based on current main
// color value (dark text on light color and vice versa).
export function setTextHue(percent)
{
	_data.textColorAuto = !percent;
	if (_data.textColorAuto) {
		updateTextColor();
		return;
	}

	// need to reset pure white or black colors to some primary color before shifting hue.
	if (Color.equals(COLOR_WHITE, textColor()) || Color.equals(COLOR_BLACK, textColor()))
		_data.textColor = new Color({ r: 255, g: 0, b: 0 });
	var hsl = textColor().toHsl();
	hsl.h = percentToDegrees(percent);
	setTextColor(hsl);
}

function updateTextColor()
{
	if (!_data.textColorAuto)
		return;
	if (color().isLight()) {
		if (!Color.equals(COLOR_BLACK, textColor()))
			setTextColor(COLOR_BLACK.clone());
	}
	else {
		if (!Color.equals(COLOR_WHITE, textColor()))
			setTextColor(COLOR_WHITE.clone());
	}
}

function sendTextColor()
{
	// Send a State update with the new text color value. Create the State first if necessary.
	if (!_data.textColorState) {
		_data.textColorState = DSE.instanceStateId() + '_textColor';
		const stateDescript = DSE.INSTANCE_NAME + " Text Color";
		//                               ID, Category,          Description,   Default
		TP.stateCreate(_data.textColorState, STATES_GROUP_NAME, stateDescript, "");
	}
	// Send the actual state value update.
	TP.stateUpdateById(_data.textColorState, textColor().hex());
}

// Private/internal utility functions

// Internal function used to perform automatic updates of complement/split comp./series colors when current color changes.
function updateSeries()
{
	if (!_data.autoUpdate)
		return;
	complement();
	splitcomplement();
	switch(_data.lastSeries.t) {
		case 'polyad':
			polyad(_data.lastSeries.n);
			break;
		case 'analogous':
			analogous(_data.lastSeries.n, _data.lastSeries.s);
			break;
		case 'monochromatic':
			monochromatic(_data.lastSeries.n);
			break;
		default:
			return;
	}
}

// This internal function is used by the array generator functions above to send results back to Touch Portal as States.
// The states are created dynamically as needed (in case they do not exist yet), and then updated to the current value from the array.
// The state IDs and names are based on the current Dynamic Script Engine instance which is running this module.
function sendColorsArray(arry, stateNamePrefix = "")
{
	const len = arry.length;
	for (let i=0; i < len; ++i) {
		let stateId = DSE.instanceStateId() + '_';
		if (stateNamePrefix)
			stateId += stateNamePrefix;
		else
			stateId += "series";
		if (len > 1)
			stateId += `_${i+1}`;
		// Create new state if needed
		if (_data.createdStates.indexOf(stateId) < 0) {
			let stateDescript = DSE.INSTANCE_NAME + " ";
			if (stateNamePrefix)
				stateDescript += stateNamePrefix;
			else
				stateDescript += "series color";
			if (len > 1)
				stateDescript += ` ${i+1}`;
			//             ID,      Parent Category,   Description,   Default value
			TP.stateCreate(stateId, STATES_GROUP_NAME, stateDescript, "#00000000");
			_data.createdStates.push(stateId);
		}

		TP.stateUpdateById(stateId, arry[i].tpcolor());
	}
}

function update()
{
	updateTextColor();
	updateSeries();

	// Send a State update with the new color as formatted text. Create the State first if necessary.
	if (!_data.formattedColorState) {
		_data.formattedColorState = DSE.instanceStateId() + '_formatted';
		const stateDescript = DSE.INSTANCE_NAME + " Formatted Color";
		TP.stateCreate(_data.formattedColorState, STATES_GROUP_NAME, stateDescript, "");
	}
	const rgb = color().toRgb();
	const hsv = color().toHsv();
	const value = Format(FORMATTED_COLOR_TEMPLATE, rgb.r, rgb.g, rgb.b, rgb.a, hsv.h, hsv.s, hsv.v, color().toHsl().l);
	TP.stateUpdateById(_data.formattedColorState, value);
}

const COLOR_BLACK = new Color({ r: 0, g: 0, b: 0 });
const COLOR_WHITE = new Color({ r: 255, g: 255, b: 255 });

// Internal data storage.
var _data = _data ||
{
	color: COLOR_WHITE.clone(),         // the current color
	textColor: COLOR_WHITE.clone(),     // the current text color (eg. to show on color swatch)
	textColorAuto: true,                // whether to automatically set the text color based on overall darkness of current main color
	autoUpdate: false,                  // enable/disable automatic update of complement/split comp./last series when current color changes
	lastSeries: {	t: "", n: 0, s: 0 },  // track which series type was last selected (if any)
	createdStates: [],                  // keep track of created state IDs so we do not re-create them each time
	formattedColorState: "",            // ID of created formatted color state, blank if not created yet
	textColorState: "",                 // ID of created text color state, blank if not created yet
	autoUpdateEnabledState: "",         // ID of created series auto-update status state, blank if not created yet
};

// Conversion utilities
function percentToShort(value) { return roundTo(value.clamp(0, 100) * 2.55, 0) }
function percentToDegrees(value) { return roundTo(value.clamp(0, 100) * 3.6, 0) }
