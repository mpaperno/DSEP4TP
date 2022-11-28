# Dynamic Script Engine Plugin for Touch Portal

[![GitHub release (latest by date including pre-releases)](https://img.shields.io/github/v/release/mpaperno/DSEP4TP?include_prereleases)](https://github.com/mpaperno/DSEP4TP/releases)
[![GPLv3 License](https://img.shields.io/badge/license-GPLv3-blue.svg)](LICENSE.GPL.txt)


**A complete, standalone, multi-threaded JavaScript environment available as a plugin
for use with [Touch Portal](https://www.touch-portal.com/) macro launcher software.**

----------
<div class="darkmode_inverted_image">
<img align="right" src="https://mpaperno.github.io/DSEP4TP/images/logo/banner_540x135.png">
</div>

[TOC]

<div class="hide-on-site">

**Visit [https://mpaperno.github.io/DSEP4TP/](https://mpaperno.github.io/DSEP4TP/) for all details, including this README.**

</div>

## Features
**This plugin evaluates expressions using a JavaScript "engine" and returns results as a Touch Portal _State_.**<br />
(If you're not familiar, Touch Portal uses states to display information to the user and/or trigger events.)

You can send other dynamic state/values to this plugin as part of the expressions it evaluates, for example to do some math on a numeric value,
or to format a piece of text in a particular way. It can evaluate anything from basic math to complex JavaScript modules.

You do **not** need to understand JavaScript to start using the basic features of this plugin.<br />
The original idea was for something to do math operations in one step and with more features (not a Touch Portal strong point), and it can certainly do that with aplomb.<br />
It then grew into a full-blown scripting environment, which you may choose to utilize at any level you wish.

So, what can yo udo with this? A lot! Here's a non-exhaustive list of **some examples**.<br />
(Note: The `${value:MyVariable}` in the examples represents Touch Portal's notation for a dynamic
global Value or a plugin State and will be replaced by Touch Portal with the actual variable value _before_ it is sent to the plugin.)

### One-liner Expressions
* Evaluate basic math using using a dynamic value from Touch Portal:
  * `${value:MyVariable} * 100 / (33 + 25)`
* Use wide array of math functions from basic rounding to trig and calc:
  * `sin( ( round(${value:MyVariable} * 100) / 100 ) % 360 ) * (180 / Math.PI)`
* Format strings in multiple ways:
  * For example with ".NET" style formatting:
    * `Format("MyVariable to 2 decimals:\n {0:F2}", ${value:MyVariable})`
  * Or if you prefer "printf" style:
    * `sprintf("MyVariable to 2 decimals:\n %.2f", ${value:MyVariable})`
  * Or with JavaScript "interpolated strings" if you like that:
    * `` `MyVariable to 2 decimals:\n ${(${value:MyVariable}).toFixed(2)}` ``
* Use inline conditional evaluation ("if X then do Y otherwise do Z"); E.g. show text based on a value:
  * `(${value:MyVariable} == 0 ? "The value is False" : "The value is True")`
* Do math on dates and times, also with formatting support:
  * `new Date().addDays(${value:test.dynamic_value_1}).format("yyyy  MMM dd, dddd")`  (eg. "2022 Dec 15, Thursday")
* Calculate a color and use it in a Touch Portal button:
  * `Color("#FF000088").spin(${value:MyVariable} * 3.6).argb()`
* Send an image to use as a Touch Portal button icon. E.g. with dynamic image name:
  * `Util.toBase64(File.read("images/status_icon_${value:MyVariable}.png", 'b'))`
* Read any number of lines from a file, starting from beginning, end, or a specific line number. E.g. read the last 5 lines of a log with a dynamic name:
  * `` File.readLines(`../logs/console-${new Date().format("yyyyMMdd")}.log`, 5, -1)) ``
* Extract a value from a JSON object in a file:
  * `JSON.parse(File.read("data.json")).myProperty`
* Send yourself a notification via Touch Portal:
  * `TP.showNotification("myNotififyId", "Something Happened", "Hey, something happened, check it out!")`

### Scripts
* JavaScript scripts of any complexity can be loaded from file(s) and functions within those scripts can be invoked with dynamic arguments (eg. from TP states/values).
* Use simple "standalone" scripts or full **JS modules**, which can import other modules and are also cached between uses for excellent performance.
* An `include()` function is available to read an evaluate any block of code from another file, for easy code re-use within standalone scripts or modules.
* Full `console.log()` (and family) support for debugging/etc; output is sent to a separate plugin log file dedicated for scripting output and errors.
* Scripts can run independently and in the background, for example processing timed events or callbacks from asynchronous processes (eg. a network request or file system watcher).
* Scripts can send state updates to Touch Portal at any time, and they can also create and remove states, among other things (you can literally write a simple TP "plugin" using
  the scripting engine itself).

### Overall
* Extensive support for **ECMAScript level 7**, plus custom extensions:
  * **File system** interaction, from one-line read/write utilities to full byte-by-byte access.
  * Flexible **string, date, and number formatting** with .NET [String.Format style](@ref String.format()) or ["printf"](@ref sprintf()) style functions.
  * Support for **timed/recurring operations** with `setTimeout()`/`setInterval()`.
  * **Network requests** supported via the familiar (if awkwardly named) `XMLHttpRequest` object.
  * **Touch Portal [interaction](@ref TP)**: update any State or global Value, create and remove States, change Slider positions, send Notifications, and more.
  * **Color manipulation**/utility library.
  * Lots of convenience extensions to built-in JS objects like Date, Number, Math and String.
  * Other [utilities](@ref Util), eg. for encoding/decoding **base-64 data**, **environment variable** access, **URL parsing**, **string extraction**, and more.
  * Infinitely **extensible** via either JavaScript libraries/modules _or_ C++ integration.
* Any expression/script action can be **saved to persistent settings** and re-created automatically when Touch Portal (or the plugin) starts up,
  with a number of options for what the default should be (fixed value, custom expression, etc).
* Expressions and scripts can run in either a single shared scripting engine instance or in separated **private engine instances** to keep environments isolated
  (each engine instance has its own global 'this' scope).
* Written in **optimized C++** for high performance.
* **Multi-threaded** for quick response times and non-blocking behavior (long-running scripts will not prevent other scripts from running).
* Runs on **Windows, MacOS, or Linux**.

-------------
## Download and Install

Note: As with all plugins, this requires the Touch Portal Pro (paid) version to function. Use the latest available Touch Portal version for best results.

1. Get the latest version of this plugin for your operating system from the  [Releases](https://github.com/mpaperno/DSEP4TP/releases) page.
2. The plugin is distributed and installed as a standard Touch Portal `.tpp` plugin file. If you know how to import a plugin,
just do that and skip to step 5. There is also a [short guide](https://www.touch-portal.com/blog/post/tutorials/import-plugin-guide.php) on the Touch Portal site.
3. Import the plugin:
    1. Start/open _Touch Portal_.
    2. Click the "gear" icon at the top and select "Import plugin..." from the menu.
    3. Browse to where you downloaded this plugin's `.tpp` file and select it.
4. Restart _Touch Portal_
    * When prompted by _Touch Portal_ to trust the plugin startup script, select "Yes" (the source code is public!).
5. That's it. You should now have the plugin's actions available to you in Touch Portal.

### Update Notifications

_Watch_ -> _Custom_ -> _Releases_ this repository (button at top) or subscribe to the [ATOM feed](https://github.com/mpaperno/WASimCommander/releases.atom) for release notifications.

Release announcements are also made in the Touch Portal Discord Server room [#dynamic-script-engine](https://discord.com/channels/548426182698467339/750791488501448887)

-------------
## Documentation

[Full Documentation @ https://mpaperno.github.io/DSEP4TP/](https://mpaperno.github.io/DSEP4TP/) - includes this README as well.

-------------
## Support and Discussion

Use the [Issues](https://github.com/mpaperno/DSEP4TP/issues) feature for bug reports and concise feature suggestions.

Use [Discussions](https://github.com/mpaperno/DSEP4TP/discussions) for any other topic.

There is also the [Touch Portal Discord Server](https://discord.gg/MgxQb8r) room [#dynamic-script-engine](https://discord.com/channels/548426182698467339/750791488501448887) for discussion, ideas, and support.

-------------
## Troubleshooting

The most likely cause of something not working right is going to be scripting syntax errors or data being passed to functions in unexpected format (eg. un-escaped backslashes).

The plugin uses 2 Touch Portal States to notify you of errors in scripts:
* The "Cumulative script error count" state is a running count of script errors which increments on each exception. This is useful as a trigger for some TP event which would
  notify you that a new error occurred.
* The "Last script instance error" state will update with the text of the last error message. This could be shown somewhere in a TP page for example.

See the [Status and Logging](doc/pages/Status.md) documentation page for more details on script error reporting.

The plugin also logs message to a file. Two, actually. They are both located in your Touch Portal configuration folder, in the 'plugins' section; the path looks like:<br/>
* Windows: `C:\Users\<User_Name>\AppData\Roaming\TouchPortal\plugins\DSEP4TP\logs`
* Mac: `~/Documents/TouchPortal/plugins/DSEP4TP/logs`
* Linux: Unknown...

The two log files are:
* The main plugin log, cleverly named `plugin.log`, is where all messages are written, including general operational information from the plugin,
  especially any errors it may have encountered that are not directly related to your scripts (but maybe to the actions you're sending to the plugin, or some other overall issue).
* The 2nd log is named `console.log` and this contains only messages from the scripting engine -- errors which are thrown or your own `console.log()` (and family) logging.

The logs are rotated daily. The previous days' log is renamed with a date stamp and a new one is started.
By default the last 3 days' worth of logs are kept (the current one + 3 historical logs).

You can force a rotation of the logs by going to the plugin's executable directory (`DSEP4TP/bin`) in a terminal/command window and run the plugin (w/out starting it) with the following command:<br />
* Windows: `DSEP4TP.exe -f1 -j1 -rx`
* Mac/Linux: `./DSEP4TP -f1 -j0 -rx`
* Optionally add a `-k N` switch to the command line to keep `N` number of logs instead of the default 3. Clear out all old logs by using `-k 0`
* Use `-h`  to see all command-line options.

The log files have an "emergency limit" of 1GB in case something goes haywire. If this limit is reached, further logging to that file is disabled until the plugin is restarted.

-------------
## Credits

This project is written, tested, and documented by myself, Maxim (Max) Paperno.<br/>
https://github.com/mpaperno/

**Contributions are welcome!**

Uses portions of the [_Qt Library_](http://qt.io) under the terms of the GPL v3 license.

Uses and includes a slightly modified version of the
[String.format for JavaScript](https://github.com/dmester/sffjs) library,
used under the terms of the zlib license.

Uses and includes a modified version of the
[TinyColor](https://github.com/bgrins/TinyColor) library,
used under the terms of the MIT license.

Uses and includes the `sprintf` function from
[Locutus](https://github.com/locutusjs/locutus) project,
used under the terms of the Locutus license.

Documentation generated with [Doxygen](https://www.doxygen.nl/) and styled with the most excellent [Doxygen Awesome](https://jothepro.github.io/doxygen-awesome-css).

-------------
## Copyright, License, and Disclaimer

Dynamic Script Engine Project <br />
COPYRIGHT: Maxim Paperno; All Rights Reserved.

This program and associated files may be used under the terms of the GNU
General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

A copy of the GNU General Public License is included in this repository
and is also available at <http://www.gnu.org/licenses/>.

This project may also use 3rd-party Open Source software under the terms
of their respective licenses. The copyright notice above does not apply
to any 3rd-party components used within.
