# Dynamic Script Engine Plugin for Touch Portal

[![Made for Touch Portal](https://img.shields.io/static/v1?style=flat&labelColor=5884b3&color=black&label=made%20for&message=Touch%20Portal&logo=data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAA4AAAAOCAYAAAAfSC3RAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAetJREFUeNp0UruqWlEQXUePb1HERi18gShYWVqJYGeXgF+Qzh9IGh8QiOmECIYkpRY21pZWFnZaqWBhUG4KjWih4msys8FLbrhZMOfsx6w1e9beWjAYBOMtx0eOGBEZzuczrtcreAyTyQSz2QxN04j3f3J84vim8+cNR4s3rKfTSUQQi8UQjUYlGYvFAtPpVIQ0u90eZrGvnHLXuOKcB1GpkkqlUCqVEA6HsVqt4HA4EAgEMJvNUC6XMRwOwWTRfhIi3e93WK1W1Go1dbTBYIDj8YhOp4NIJIJGo4FEIoF8Po/JZAKLxQIIUSIUChGrEy9Sr9cjQTKZJJvNRtlsVs3r9Tq53W6Vb+Cy0rQyQtd1OJ1O9b/dbpCTyHoul1O9z+dzGI1Gla7jFUiyGBWPx9FsNpHJZNBqtdDtdlXfAv3vZLmCB6SiJIlJhUIB/X7/cS0viXI8n8+nrBcRIblcLlSrVez3e4jrD6LsK3O8Xi8Vi0ViJ4nVid2kB3a7HY3HY2q325ROp8nv94s5d0XkSsR90OFwoOVySaPRiF6DiHs8nmdXn+QInIxKpaJclWe4Xq9fxGazAQvDYBAKfssDeMeD7zITc1gR/4M8isvlIn2+F3N+cIjMB76j4Ha7fb7bf8H7v5j0hYef/wgwAKl+FUPYXaLjAAAAAElFTkSuQmCC)](https://www.touch-portal.com)
[![GitHub release (latest by date including pre-releases)](https://img.shields.io/github/v/release/mpaperno/DSEP4TP?include_prereleases)](https://github.com/mpaperno/DSEP4TP/releases)
![Supported Platvorms](https://img.shields.io/badge/platforms-windows%20|%20osx%20|%20linux-AA7722)
[![GPLv3 License](https://img.shields.io/badge/license-GPLv3-blue.svg)](LICENSE.GPL.txt)
[![Discord](https://img.shields.io/static/v1?style=flat&color=7289DA&&labelColor=7289DA&message=Discord%20Chat&label=&logo=discord&logoColor=white)](https://discord.gg/FhYsZNFgyw)
<!-- <img alt="Lines of code" src="https://img.shields.io/tokei/lines/github/mpaperno/DSEP4TP?color=green&label=LoC&logo=cplusplus&logoColor=f34b7d"> -->


**A complete, standalone, multi-threaded JavaScript environment available as a plugin
for use with [Touch Portal](https://www.touch-portal.com/) macro launcher software.**

**This is the ultimate in Touch Portal extensibility short of writing your own plugin.**

<div class="hide-on-site">

**Visit [dse.tpp.max.paperno.us](https://dse.tpp.max.paperno.us) for all details, including this README.**

</div>

----------
<div class="darkmode_inverted_image">
<img align="right" src="https://dse.tpp.max.paperno.us/images/logo/banner_420x105.png">
</div>

## Features
**This plugin evaluates expressions using a JavaScript "engine" and (optionally) returns results as a Touch Portal _State_.**<br />
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
  * `File.read("images/status_icon_${value:MyVariable}.png", 'b').toBase64()`
* Get an image from the Internet for a button icon:
  * `Net.request("https://dse.tpp.max.paperno.us/images/logo/icon_64.png").get().base64()`
* Read any number of lines from a file, starting from beginning, end, or a specific line number. E.g. read the last 5 lines of a log with a dynamic name:
  * `` File.readLines(`../logs/console-${new Date().format("yyyyMMdd")}.log`, 5, -1)) ``
* Extract a value from a JSON object in a file:
  * `JSON.parse(File.read("data.json")).myProperty`
* Or get JSON from a Web site and display a named value from the data:
  * `Net.request("https://jsonplaceholder.typicode.com/todos/1").get().json().title`
* Update any State or Value in Touch Portal:
  * `TP.stateUpdateById("MyVariable", ${value:MyVariable} * 25)`
* Send yourself a notification via Touch Portal:
  * `TP.showNotification("myNotififyId", "Something Happened", "Hey, something happened, check it out!")`

### Scripts
* JavaScript scripts of any complexity can be loaded from file(s) and functions within those scripts can be invoked with dynamic arguments
  (eg. from Touch Portal states/values).
* Use simple "standalone" scripts or full **JS modules**, which can import other modules and are also cached between uses for excellent performance.
* An `include()` function is available to read an evaluate any block of code from another file, for easy code re-use within standalone scripts or modules.
* Full `console.log()` (and family) support for debugging/etc; output is sent to a separate plugin log file dedicated for scripting output and errors.
* Scripts can **run in the background**, for example processing timed events or callbacks from asynchronous processes (eg. a network request or file system watcher).
* Scripts can **send State/Value updates to Touch Portal** at any time, and they can also create and remove states, among other things
  (you can literally write a simple Touch Portal "plugin" using the scripting engine itself).

### Overall
* Extensive support for **ECMAScript level 7**, plus custom extensions:
  * **[File system](@ref FileSystem)** interaction, from one-line read/write utilities to full byte-by-byte access.
  * Flexible **string, date, and number formatting** with .NET [String.Format style](@ref String.format()) or ["printf"](@ref sprintf()) style functions.
  * Support for **timed/recurring operations** with `setTimeout()`/`setInterval()`.
  * **Network requests** supported via familiar [Fetch API](@ref FetchAPI) and [XMLHttpRequest](@ref stdlib-xmlhttpreq).
  * **Touch Portal [interaction](@ref TP)**: update any State or global Value, create and remove States, change Slider positions, send Notifications, and more.
  * **Color manipulation**/utility library.
  * **External command and application launching** with optional inter-process data exchange using `Process` class.
  * **Clipboard interactions** (see [Clipboard example](@ref example_clipboard) module);
  * Lots of convenience extensions to built-in JS objects like Date, Number, Math and String.
  * Other [global](@ref Global) object extensions and [utilities](@ref Util),
    eg. for encoding/decoding **base-64 data**, **environment variable** access, **locale data**, **hashing algorithms**, and more.
  * Infinitely **extensible** via either JavaScript libraries/modules _or_ C++ integration.
* Any expression/script action can be **saved to persistent settings** and re-created automatically at startup,
  with a number of options for what the default value should be (fixed, custom expression, etc).
* Expressions and scripts can run in either a single Shared scripting engine instance or in separated **Private engine instances** to keep environments isolated.
  Instances are _persistent_ throughout the life of the plugin (or until deleted explicitly).
* **Multi-threaded** for quick response times and non-blocking behavior (long-running scripts will not prevent other scripts from running).
* Extensively **documented**, with **examples** and scripting **reference**.
* Written in **optimized C++** for high performance.
* Runs on **Windows, MacOS, or Linux**.

-------------
## Download and Install

Note: As with all plugins, this requires the Touch Portal Pro (paid) version to function. Use the latest available Touch Portal version for best results.

1. Get the latest version of this plugin for your operating system from the [Releases](https://github.com/mpaperno/DSEP4TP/releases) page.
2. The plugin is distributed and installed as a standard Touch Portal `.tpp` plugin file. If you know how to import a plugin,
  just do that and skip to step 5. There is also a [short guide](https://www.touch-portal.com/blog/post/tutorials/import-plugin-guide.php) on the Touch Portal site.
3. Import the plugin:
    1. Start/open _Touch Portal_.
    2. Click the Settings "gear" icon at the top-right and select "Import plugin..." from the menu.
    3. Browse to where you downloaded this plugin's `.tpp` file and select it.
    4. When prompted by _Touch Portal_ to trust the plugin startup script, select "Trust Always" or "Yes" (the source code is public!).
       * "Trust Always" will automatically start the plugin each time Touch Portal starts.
       * "Yes" will start the plugin this time and then prompt again each time Touch Portal starts.
       * If you select "No" then you can still start the plugin manually from Touch Portal's _Settings -> Plug-ins_ dialog.
4. That's it. You should now have the plugin's actions available to you in Touch Portal.

### Updates

Unless noted otherwise in the notes of a particular release version, it is OK to just re-install a newer version of the plugin "on top of"
a previous version without uninstalling the old version first. Either way is OK, just keep in mind that uninstalling the plugin via Touch Portal
will also remove any current log files as well.

### Update Notifications

In GitHub (with an account) you can _Watch_ -> _Custom_ -> _Releases_ this repository (button at top right).

Or subscribe to the [ATOM feed](https://github.com/mpaperno/DSEP4TP/releases.atom) for release notifications.

Release announcements are also made in the Touch Portal Discord Server room [#dynamic-script-engine](https://discord.gg/FhYsZNFgyw)

Or use the provided [example script](@ref example_fetch_and_notify) to check for new versions right from Touch Portal!

<div class="hide-on-site">

-------------
## Documentation

[Full Documentation & Examples @ dse.tpp.max.paperno.us](https://dse.tpp.max.paperno.us) - includes this README as well.

</div>

-------------
## Support and Discussion

Use the [Issues](https://github.com/mpaperno/DSEP4TP/issues) feature for bug reports and concise feature suggestions.

Use [Discussions](https://github.com/mpaperno/DSEP4TP/discussions) for any other topic.

There is also the [Touch Portal Discord Server](https://discord.gg/MgxQb8r) room [#dynamic-script-engine](https://discord.com/channels/548426182698467339/750791488501448887) for discussion, ideas, and support.

-------------
## Troubleshooting

The most likely cause of something not working right is going to be scripting syntax errors or data being passed to functions in unexpected format (eg. un-escaped backslashes).

See the [Status and Logging](doc/pages/Status.md) documentation page for more details on script error reporting and plugin logging in general.

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

<span class="next_section_button">
Go To: [Scripting Library Reference](modules.html)
</span>

<span class="next_section_button">
Go To: [Examples](@ref plugin_examples)
</span>

<span class="next_section_button">
Go To: [Plugin Documentation](@ref documentation)
</span>
