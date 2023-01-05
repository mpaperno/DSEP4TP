/* Dynamic Script Engine - Color Picker Script

 This script is meant to accompany the Color Picker example Touch Portal page which should be distributed alongside.

 This is in a lot of ways like a mini-plugin in itself, as it provides a lot of functionality one would otherwise need
 an actual Touch Portal plugin for.

*/

// Import clipboard functions from Clipboard example module.  The import path assumes both examples are in the same folder.
import { valueToClipboard, clipboardToValue } from "clipboard.mjs";

// This script creates Touch Portal States dynamically as needed.
// This variable sets the title under which all the created States are grouped.
// The group will appear as a child of the Dynamic Script Engine main plugin grouping in TP menus.
export var STATES_GROUP_NAME = "Color Picker";

// Template for formatted color value. .NET String.Format() style.
// Template values are ("real" means 0-1 range, "int" means 0-255 (or 0-359 for Hue)):
//     0 = red (real); 1 = blue (real); 2 = green (real); 3 = alpha (real); 4 = hue (real); 5 = saturation (real); 6 = value (real); 7 = lightness (real);
//     8 = red (int); 9 = blue (int); 10 = green (int); 11 = alpha (int); 12 = hue (° int); 13 = saturation (int); 14 = value (int); 15 = lightness (int)
//export var FORMATTED_COLOR_TEMPLATE = "{0:F2} {1:F2} {2:F2} {3:F2}\n{4:F2} {5:F2} {6:F2} {7:F2}\n{8:X2} {9:X2} {10:X2} {11:X2}\n{12:000} {13:000} {14:000} {15:000}"; // uncomment this line to see what all the fields look like
export var FORMATTED_COLOR_TEMPLATE = "R: {8:000}\t\tH {12:000}°\nG: {9:000}\t\tS {13:000}\nB: {10:000}\t\tV {14:000}\nA: {11:000}\t\tL {15:000}";

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
	update(3);

	// Do first-run initialization if needed.
	if (!_data.init)
		init();

	return tpcolor();
}

// Sets the current color from an "#AARRGGBB" format string (this format is used by Touch Portal
// for button background colors in "Change visuals by plug-in state" actions).
export function setColorArgb(color)
{
	const rgba = color.length === 9 ? color.replace(/^#([0-9a-z]{2})([0-9a-z]{6})$/i, '#$2$1') : color;
	const newColor = new Color(rgba);
	if (newColor.isValid())
		return setColor(newColor);
	console.error(`'${color}' is not a valid color.`);
	return tpcolor();
}


// ------------------------------------------
// Color channel/component setting functions.

// The following functions set a corresponding channel of the current color to a percentage value (0 - 100) and return the new color as an #AARRGGBB string.
// If `updateConnector` is true then the corresponding slider will also be repositioned (when adjusting using a slider, this should be `false`).

export function setRed(percent, updateConnector = false)
{
	color()._r = percentToShort(percent);
	if (updateConnector)
		updateRedConnector(color()._r);
	return update(2);
}

export function setGreen(percent, updateConnector = false)
{
	color()._g = percentToShort(percent);
	if (updateConnector)
		updateGreenConnector(color()._g);
	return update(2);
}

export function setBlue(percent, updateConnector = false)
{
	color()._b = percentToShort(percent);
	if (updateConnector)
		updateBlueConnector(color()._b);
	return update(2);
}

export function setAlpha(alpha, updateConnector = false)
{
	color().setAlpha(alpha * 0.01);
	if (updateConnector)
		updateAlphaConnector(color().alpha());
	return update(0);  // alpha is only controlled by one slider, no other connector updates needed.
}

export function setHue(percent, updateConnector = false)
{
	var hsv = color().toHsv();
	hsv.h = percentToDegrees(percent);
	_data.color = new Color(hsv);
	if (updateConnector)
		updateHueConnector(hsv.h);
	return update(1);
}

export function setSaturation(percent, updateConnector = false)
{
	if (percent === 1)
		percent += .01;
	var hsv = color().toHsv();
	hsv.s = percent;
	_data.color = new Color(hsv);
	if (updateConnector)
		updateSatConnector(hsv.s);
	return update(1);
}

export function setValue(percent, updateConnector = false)
{
	if (percent === 1)
		percent += .01;
	var hsv = color().toHsv();
	hsv.v = percent;
	_data.color = new Color(hsv);
	if (updateConnector)
		updateValueConnector(hsv.v);
	return update(1);
}

// Note that this changes the color space of the current color to HSL.
export function setLightness(percent)
{
	if (percent === 1)
		percent += .01;
	var hsl = color().toHsl();
	hsl.l = percent;
	_data.color = new Color(hsl);
	return update(3);
}


// ------------------------------------------
// Adjustment (step increment/decrement) functions.

// The following functions are used to adjust the individual color channels by a set +/- amount from what they currently are and return the new color as an #AARRGGBB string.
// They will wrap the result values into a range between the minimum and maximum values of the corresponding channel (eg. for red: 255 + 1 = 0).

// For the RGBA channels the steps are integer values in the range of (-255 - +255).

export function adjustRed(amount)
{
	return setRed(shortToPercent(wrapShort(color()._r, amount)), true);
}

export function adjustGreen(amount)
{
	return setGreen(shortToPercent(wrapShort(color()._g, amount)), true);
}

export function adjustBlue(amount)
{
	return setBlue(shortToPercent(wrapShort(color()._b, amount)), true);
}

export function adjustAlpha(amount)
{
	return setAlpha(wrapPercent(color().alpha(), shortToPercent(amount)), true);
}

// The hue channel steps are integer values in the range of (-360 - +360).
export function adjustHue(amount)
{
	return setHue(degreesToPercent(wrapDegrees(color().toHsv().h, amount)), true);
}

// The S/V/L channel steps are percentage values in the range of (-100 - +100).

export function adjustSaturation(amount)
{
	return setSaturation(wrapPercent(color().toHsv().s,  amount), true);
}

export function adjustValue(amount)
{
	return setValue(wrapPercent(color().toHsv().v,  amount), true);
}

// Note that this changes the color space of the current color to HSL.
export function adjustLightness(amount)
{
	return setLightness(wrapPercent(color().toHsl().l,  amount));
}


// ------------------------------------------
// Text color functions

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
	_data.textColorAuto = (Color.equals(COLOR_WHITE, _data.textColor) || Color.equals(COLOR_BLACK, _data.textColor));
	sendTextColor();
}

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


// ------------------------------------------
// Connector/slider feedback handling
// These functions are all about updating the slider positions in the page to correspond to the currently selected color.
// For example if the Hue is adjusted, the RGB sliders should be updated to reflect the current values, and vice versa.
// Or if the step buttons are used to adjust color instead of the sliders, the sliders should still reflect the current position (as closely as possible).

// This function queries the global DSE connector tracking database to try to find the "short ID" of the connector
// we're interested in.  We can do this based on some criteria; in this case we can use the current instance (State) Name,
// provided by the `DSE.INSTANCE_NAME` global variable, and also the expression being used. For example the red channel
// slider uses the expression "M.setRed(${connector_value})". So if we search for "*setRed*" (using wildcards), this should give us the
// slider for the red channel.
// We also cache (save) the short ID lookups, because they are not going to change very often, if ever, and searching for
// them every time a slider is moved is a waste. See the `init()` function at the bottom for how the cache is cleared if connectors are changed on the page.
function getShortId(channel)
{
	if (!_data.connectorIds[channel]) {
		const shortIds = TP.getConnectorShortIds({ instanceName: DSE.INSTANCE_NAME, expression: `*set${channel}*` });
		if (shortIds.length)
			_data.connectorIds[channel] = shortIds[0];
		//console.log(channel, _data.connectorIds[channel]);
	}
	return _data.connectorIds[channel];
}

// Generic function to update any of the color channel sliders by name with given value.
function updateConnector(channel, value)
{
	const shortId = getShortId(channel);
	if (shortId)
		TP.connectorUpdateShort(shortId, value.clamp(0, 100));
}

// These functions are basically just conveniences to set each slider individually, instead of repeating the same code in several of places.
function updateRedConnector(r)     { updateConnector("Red", round(shortToPercent(r))); }
function updateGreenConnector(g)   { updateConnector("Green", round(shortToPercent(g))); }
function updateBlueConnector(b)    { updateConnector("Blue", round(shortToPercent(b))); }
function updateAlphaConnector(a)   { updateConnector("Alpha", round(a * 100)); }
function updateHueConnector(h)     { updateConnector("Hue", round(degreesToPercent(h))); }
function updateSatConnector(s)     { updateConnector("Saturation", round(s * 100)); }
function updateValueConnector(v)   { updateConnector("Value", round(v * 100)); }
function updateTextHueConnector(h) { updateConnector("TextHue", round(degreesToPercent(h))); }

// This function sets all the RGB sliders, eg. when any of the HSV controls change.
function updateRgbConnectors()
{
	const rgb = color().toRgb();
	updateRedConnector(rgb.r);
	updateGreenConnector(rgb.g);
	updateBlueConnector(rgb.b);
}

// This function sets all the HSV slider, eg. when any of the RGB channels change.
function updateHsvConnectors()
{
	const hsv = color().toHsv();
	updateHueConnector(hsv.h);
	updateSatConnector(hsv.s);
	updateValueConnector(hsv.v);
}


// ------------------------------------------
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

// Generates up to `n` number of colors harmonious with the current color. Updates/creates States based on current instance name and numeric suffix.
export function polyad(n)
{
	_data.lastSeries = {t: "polyad", n: n, s: 0 };
	sendSeries();
	sendCurrentSeriesNameState();
}

// Generates up to `n` number of colors analogous to the current color. Updates/creates States based on current instance name and numeric suffix.
export function analogous(n, slices = 30)
{
	_data.lastSeries = {t: "analogous", n: n, s: slices };
	sendSeries();
	sendCurrentSeriesNameState();
}

// Generates up to `n` number of monochromatic colors based on the current color. Updates/creates States based on current instance name and numeric suffix.
export function monochromatic(n)
{
	_data.lastSeries = {t: "monochromatic", n: n, s: 0 };
	sendSeries();
	sendCurrentSeriesNameState();
}

// This function clears the current color series (polyad, analogous, mono),
// sending an array of `n` transparent colors instead. If `n` is -1 (default) then the count from the last
// requested series is used.  If `n` is 0, then no transparent colors are sent (the last series color state values stay intact).
// Can be used to simply clear any current series colors or to keep automatic updates of complement/split comp but w/out series updates.
export function clearCurrentSeries(n = -1)
{
	_data.lastSeries.t = "";
	if (n > -1)
		_data.lastSeries.n = n;
	sendSeries();
	sendCurrentSeriesNameState();
}

// This internal function generates the last requested color series.
function sendSeries()
{
	switch(_data.lastSeries.t) {
		case 'polyad':
			sendColorsArray(color().polyad(_data.lastSeries.n+1).slice(1));
			break;
		case 'analogous':
			sendColorsArray(color().analogous(_data.lastSeries.n+1, _data.lastSeries.s).slice(1));
			break;
		case 'monochromatic':
			sendColorsArray(color().monochromatic(_data.lastSeries.n+1).slice(1));
			break;
		case '':
			if (_data.lastSeries.n > 0) {
				let colors = [];
				[...Array(_data.lastSeries.n).keys()].forEach(() => colors.push(new Color('#00000000')));
				sendColorsArray(colors);
			}
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
		if (_data.colorSeriesStates.indexOf(stateId) < 0) {
			let stateDescript = DSE.INSTANCE_NAME + " ";
			if (stateNamePrefix)
				stateDescript += stateNamePrefix;
			else
				stateDescript += "series color";
			if (len > 1)
				stateDescript += ` ${i+1}`;
			//             ID,      Parent Category,   Description,   Default value
			TP.stateCreate(stateId, STATES_GROUP_NAME, stateDescript, "#00000000");
			_data.colorSeriesStates.push(stateId);
		}

		TP.stateUpdateById(stateId, arry[i].tpcolor());
	}
}

// Enables or disables the automatic updating of complement, split complement, and last selected series based on the current color.
export function autoUpdateSeries(enable = true)
{
	if (_data.autoUpdate === enable)
		return;

	_data.autoUpdate = enable;
	updateSeries();
	sendAutoUpdateSeriesState();
}

// Toggles the automatic series updates. Convenience for `autoUpdateSeries()` based on current setting.
export function autoUpdateSeriesToggle()
{
	autoUpdateSeries(!_data.autoUpdate);
}

// Internal function used to perform automatic updates of complement/split comp./series colors when current color changes.
function updateSeries()
{
	if (!_data.autoUpdate)
		return;
	complement();
	splitcomplement();
	sendSeries();
}

// Internal function to send the name of the currently selected color series as a State,
// eg. for highlighting the corresponding button on the page.
function sendCurrentSeriesNameState()
{
	// Create the State first if necessary.
	if (!_data.lastSeriesNameState) {
		_data.lastSeriesNameState = DSE.instanceStateId() + '_selectedSeries';
		const stateDescript = DSE.INSTANCE_NAME + " Selected Color Series";
		// the default should be 0, but due to a "bug" in TP <= v3.1 we need to force it to a blank value first.
		TP.stateCreate(_data.lastSeriesNameState, STATES_GROUP_NAME, stateDescript, "");
	}
	var seriesName = '';
	// Format the series name to look like the function call which invoked it (this way the parameters can also be matched);
	// eg: "analogous(6, 18)"
	if (_data.lastSeries.t) {
		seriesName = `${_data.lastSeries.t}(${_data.lastSeries.n}`;
		if (_data.lastSeries.s > 0)
			seriesName += ", " + _data.lastSeries.s;
		seriesName += ')';
	}
	TP.stateUpdateById(_data.lastSeriesNameState, seriesName);
}

	// Send a State update with the new value of the autoUpdateSeries setting (eg. to use as button visual change trigger).
function sendAutoUpdateSeriesState()
{
	// Create the State first if necessary.
	if (!_data.autoUpdateEnabledState) {
		_data.autoUpdateEnabledState = DSE.instanceStateId() + '_autoUpdateSeries';
		const stateDescript = DSE.INSTANCE_NAME + " Auto Update Complement/Series";
		// the default should be 0, but due to a "bug" in TP <= v3.1 we need to force it to a blank value first.
		TP.stateCreate(_data.autoUpdateEnabledState, STATES_GROUP_NAME, stateDescript, "");
	}
	TP.stateUpdateById(_data.autoUpdateEnabledState, _data.autoUpdate ? "1" : "0");
}


// ------------------------------------------
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

// Copy current color in "#RRGGBB[AA]" format to the clipboard.
// The alpha component will be included if it is not fully opaque (< 1.0).
export function copyHex()
{
	if (color().alpha() < 1)
		valueToClipboard(color().rgba());
	else
		valueToClipboard(color().hex());
}

// Copy current color in "#RRGGBBAA" format to the clipboard.
export function copyRgba()
{
	valueToClipboard(color().rgba());
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


// ------------------------------------------
// Other internal utility functions/data.

// This function runs each time the current color is updated. It runs any related updaters as needed
// (eg. the automatic text color and series generators), potentially updates connectors, and most
// importantly it sends the current color to TP, both as an `#AARRGGBB` value for displaying on the swatch,
// and as a formatted string (using the FORMATTED_COLOR_TEMPLATE template defined at the top)
// with the current color values for displaying on top of the swatch (or wherever).
// connectors to update: 0 = none; 1 = rgb; 2 = hsv; 3 = rgb + hsv + alpha
function update(connectors = 0)
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
	const lum = color().luminance();
	//const hsl = color().toHsl();
	const value = Format(
		FORMATTED_COLOR_TEMPLATE,
		shortToPercent(rgb.r) * .01, shortToPercent(rgb.g) * .01, shortToPercent(rgb.b) * .01, rgb.a, degreesToPercent(hsv.h) * .01, hsv.s, hsv.v, lum,
		rgb.r, rgb.g, rgb.b, percentToShort(rgb.a * 100), hsv.h, percentToShort(hsv.s * 100), percentToShort(hsv.v * 100), percentToShort(lum * 100)
	);
	TP.stateUpdateById(_data.formattedColorState, value);
	//console.log(lum, hsl.l, hsv.h, hsl.h, hsv.s, hsl.s);

	// loop over all the color channel values and create/update States as needed.
	for (const [k, v] of Object.entries(_data.channelValues)) {
		if (!v.state) {
			// Create a state with a friendly name for this channel.
			v.state = DSE.instanceStateId() + '_channel_' + k;
			const stateDescript = DSE.INSTANCE_NAME + " Channel Value: " + v.name;
			TP.stateCreate(v.state, STATES_GROUP_NAME, stateDescript, "");
		}
		const newVal = v.get(rgb, hsv, lum);
		if (newVal != v.v) {
			v.v = newVal;
			TP.stateUpdateById(v.state, Format(v.fmt, v.v));
		}
	}

	if (connectors & 1)
		updateRgbConnectors();
	if (connectors & 2)
		updateHsvConnectors();
	if (connectors == 3)
		updateAlphaConnector(rgb.a);

	return tpcolor();
}

// Do first-run initialization tasks.
function init()
{
	sendCurrentSeriesNameState();
	sendAutoUpdateSeriesState();

	// Register an event handler with the global connectors database to get notified
	// when connector data has been updated (eg. a slider has been added/removed/changed).
	// For our needs it's simpler to just clear the cache when this happens, and it will
	// be re-populated automatically as needed (in `getShortId()`);
	// The `connectorIdsChanged()` event has one parameter, which is the name of the instance/State
	// for which the changed connector(s) are used (this is in their State Name field). So
	// we only need to react to changes made to "our" connectors, where the instanceName matches the
	// current one in `DSE.INSTANCE_NAME`.
	TP.onconnectorIdsChanged( (instanceName) => {
		if (instanceName == DSE.INSTANCE_NAME) {
			_data.connectorIds = {};
			console.info("Connector ID cache cleared.");
		}
	});
	_data.init = true;
}

const COLOR_BLACK = new Color({ r: 0, g: 0, b: 0 });
const COLOR_WHITE = new Color({ r: 255, g: 255, b: 255 });

// Internal data storage.
var _data = _data ||
{
	color: COLOR_WHITE.clone(),         // the current color
	textColor: COLOR_BLACK.clone(),     // the current text color (eg. to show on color swatch)
	init: false,                        // initialization flag, set to true after first time `setColor()` is called.
	textColorAuto: true,                // whether to automatically set the text color based on overall darkness of current main color
	autoUpdate: false,                  // enable/disable automatic update of complement/split comp./last series when current color changes
	lastSeries: {	t: "", n: 0, s: 0 },  // track which series type was last selected (if any)
	colorSeriesStates: [],              // keep track of created state IDs for color series so we do not re-create them each time
	formattedColorState: "",            // ID of created formatted color state, blank if not created yet
	textColorState: "",                 // ID of created text color state, blank if not created yet
	lastSeriesNameState: "",            // ID of created state for the last selected color series name, blank if not created yet
	autoUpdateEnabledState: "",         // ID of created series auto-update status state, blank if not created yet
	connectorIds: {},                   // track connector `shortId`s which are used in the page to set color values; these are used for updating slider positions.
	// track values and states of color channels (to show under sliders, for example);
	// track the values so that we do not send needless updates when they do not actually change; also makes it possible to reference the current channel value at any time
	channelValues: {
	//C:   Name for State,     Value, State ID,  Get value function,                              Formatting string  (.NET style for `String.format()`)
		r: { name: "Red",        v: -1, state: "", get: (rgb, _, __) => shortToPercent(rgb.r), fmt: "{0:00.0}" },
		g: { name: "Green",      v: -1, state: "", get: (rgb, _, __) => shortToPercent(rgb.g), fmt: "{0:00.0}" },
		b: { name: "Blue",       v: -1, state: "", get: (rgb, _, __) => shortToPercent(rgb.b), fmt: "{0:00.0}" },
		a: { name: "Alpha",      v: -1, state: "", get: (rgb, _, __) => rgb.a * 100,           fmt: "{0:00.0}" },
		h: { name: "Hue",        v: -1, state: "", get: (_, hsv, __) => hsv.h,                 fmt: "{0:000}"  },
		s: { name: "Saturation", v: -1, state: "", get: (_, hsv, __) => hsv.s * 100,           fmt: "{0:00.0}" },
		v: { name: "Value",      v: -1, state: "", get: (_, hsv, __) => hsv.v * 100,           fmt: "{0:00.0}" },
		l: { name: "Lightness",  v: -1, state: "", get: (_, __,   l) => l * 100,               fmt: "{0:00.0}" },
		/*
		// For example to display the RGB values in 0-256 range instead of percent, change the result value and the format string:
		r: { name: "Red",        v: -1, state: "", get: (rgb, _, __) => rgb.r,                 fmt: "{0:000}"  },
		g: { name: "Green",      v: -1, state: "", get: (rgb, _, __) => rgb.g,                 fmt: "{0:000}"  },
		b: { name: "Blue",       v: -1, state: "", get: (rgb, _, __) => rgb.b,                 fmt: "{0:000}"  },
		*/
	},
};

// Conversion utilities
function percentToShort(value) { return round(value.clamp(0, 100) * 2.55); }
function shortToPercent(value) { return value.clamp(-255, 255) / 2.55 }
function percentToDegrees(value) { return round(value.clamp(0, 100) * 3.594); }
function degreesToPercent(value) { return value.clamp(-360, 360) / 3.6; }

function wrapShort(v, d)
{
	var p = (v + d.clamp(-255, 255)) % 256;
  return (p < 0 ? 256 + p : p);
}

function wrapDegrees(v, d)
{
	var p = (v + d.clamp(-360, 360)) % 360;
  return (p < 0 ? 360 + p : p);
}

function wrapPercent(v, d)
{
	var p = v + clamp(d * 0.01, -1, 1);
  return p - floor(p);
}
