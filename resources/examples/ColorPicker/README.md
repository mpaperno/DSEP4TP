# Color Picker Example {#example_color_picker}
A script and page example of an interface for choosing a color.

<div class="hide-on-site">

**See the [published documentation](https://dse.tpp.max.paperno.us/example_color_picker.html) for a properly formatted version of this README.**
</div>

This example demonstrate some uses of the Color library to create a basic "color picker" type interface (scroll down for page screenshot).
It also uses the [Clipboard example](@ref example_clipboard) module for interacting with the system clipboard (copying colors to and from).

The script itself is mostly just an "interface" to the various color manipulation features available with a Color object. It stores an instance
of a Color value, which is the "current color," and all operations are performed on this color, after which it is usually sent back to Touch Portal
as a new color value to be displayed somewhere (like the central "swatch" in the example page).

The example page uses 8 global Touch Portal Values as inputs to control various aspects of the color (color channel values, opacity, saturation, etc).
Each Value is controlled by a Slider and also the corresponding up/down buttons. The central "color swatch" button is where the input value changes are
detected and sent to the loaded script (which then returns the new resulting color to use as the swatch background color).

Additionally there are buttons to invoke clipboard functions to copy the current color to clipboard in various formats, and another button to
set the current color _from_ a value on the clipboard (it has to be in one of the recognized formats... see `setColor()` notes in the code below).

The up/down buttons for each input value are there because Touch Portal Sliders only have a resolution of 0 through 100, whereas most of the actual color values
can be in a range of 0 through 255 (or 0-360 for Hue).  So, the buttons provide a way to "fine tune" the value to account for the missing resolution of the sliders.

@note Assets for this example, including the code and page shown below, can be found in the project's repository at<br />
https://github.com/mpaperno/DSEP4TP/tree/main/resources/examples/ColorPicker/

### Example page using this script

<a href="example_color_picker_screenshot.jpg" target="image" title="Click for full version in new window.">
<img src="example_color_picker_screenshot.jpg" />
</a>

### Initial page setup

1. Download the [DSE_Color_Picker_v2 page](https://github.com/mpaperno/DSEP4TP/raw/main/resources/examples/ColorPicker/DSE_Color_Picker_v2.tpz), 
	[color_picker.mjs](https://github.com/mpaperno/DSEP4TP/raw/main/resources/examples/ColorPicker/color_picker.mjs), and
	[clipboard.mjs](https://github.com/mpaperno/DSEP4TP/raw/main/resources/examples/Clipboard/clipboard.mjs) script files.
2. Place the two script files anywhere you want in your file system, as long as they're both in the same folder. 
	I recommend starting a new folder for scripts somewhere like in your _Documents_.
3. Import the page into Touch Portal
   - If you have _not_ imported any previous version of this page, make sure the "Values" option is selected.
   - On the other hand, if you're updating a previous version of this page, make sure the "Values" option is _not_ selected.
4. Open/go to the page in the Touch Portal desktop application. In place of the central color swatch in the screenshot above will be an "INITIALIZE" button.
   - Edit that button's _On Pressed_ action and make sure the path to `color_picker.mjs` matches where you placed the script on your computer.
     - You can either type or paste in the script location (path) or use the `...` button to find and select the script with a standard file dialog.
   - You could also now remove the button with the "INITIAL ONE-TIME SETUP" instructions text if you want (it serves no other purpose).
5. Create a new button somewhere (eg. on your (main) page) that opens this page on your Touch Portal Android/iOS device.
6. Navigate to the new page on your device, and then press the INITIALIZE button.
   - This only needs to be done once when you first import the page. After that everything should be re-created automatically next time you start
   Touch Portal (or restart this plugin).
7. The "INITIALIZE" button should turn into a white color swatch and the page is ready for use.
8. Optional: If you make any changes to the included script file, you need to use the "Reset Module" button to clear out the old script version before your changes will be detected.
   - This button also needs to have the location of `color_picker.mjs` properly set up, just like in the "INITILIZE" button.

**Note**<br/>
You can set a default location (directory) for script files in the plugin's Settings (in _Touch Portal -> Settings -> Plug-ins_). That way you could use
only relative paths to script files in the plugin's actions (which is how the page comes set up by default).

### Color Picker script

@include{lineno} color_picker.mjs
