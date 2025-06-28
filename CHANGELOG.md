# Change Log
**Dynamic Script Engine Plugin for Touch Portal**: changes by version number and release date.

## 1.3.0.0 (28-Jun-2025)

<div class="hide-on-site">
Note: Reading these notes <a href="https://dse.tpp.max.paperno.us/md__d___devel__touch_portal__dynamic_script_engine__d_s_e_p4_t_p__c_h_a_n_g_e_l_o_g.html">at the documentation's site</a> will provide links to all the new API features.
</div>

### Plugin Core
- The plugin's scripting engines now run in a single system thread by default. This should provide greater stability when running scripts, especially more complex
  ones with asynchronous event handlers, timers, the new WebSockets client, and so forth. In most cases there should be no appreciable practical difference. However,
  the old multi-threaded behavior can be restored using the new related plugin setting. If your current scripts runs stable and you think benefit from multi-threading,
  you may re-enable this feature (at your own risk :) ).

### New JS Library Features
- Added a full-featured `WebSocket` client and simplified `Net.wsGet()` and `Net.wsSend()` utility functions.
- Added `FSWatcher` class for monitoring changes to files and directories.
- The `File` utility type gets many enchancements (see docs for details):
  - Added async methods: `readAsync()`, `readLinesAsync()`, `copyAsync()`, `renameAsync()`, `removeAsync()` and `writeAsync()`. These can return a `Promise` or accept a callback argument.
  - Added support for text file encoding and decoding of UTF-16 and UTF-32 LE/BE formats. When reading files, encoding can be detected automatically (if a BOM is present), or specified explicitly.
  - Added "safe" options for `write()`, `copy()` and `rename()` functions (and their new async counterparts) which can handle errors more gracefully. `copy()` and `rename()` can now be set to overwrite existing files instead of failing.
  - Added option of writing Windows or "Unix" line endings for text files (CR+LF or LF, default depends on current OS).
  - Added option to write a BOM header for UTF-encoded files.
  - Calling synchronous function can now be configured to throw exceptions on error or fail "silently" (but log the error); see new `File.throwExceptions` property.
- Added a "Generic Script Event" Touch Portal Event with "local states" feature which can be triggered from scripts with an event name and up to 10 other arbitrary local state values using `TP.triggerGenericEvent()`.
- Added `TPButton` utility class which can synchronize visual state properties with an actual Touch Portal button via automatic state updates (see docs for details and button template).
- Added new event handler connection options and documentation, eg. `on()`, `off()` and `once()` methods, more inline with common standards. General documentation in new [`Event Handling`](https://dse.tpp.max.paperno.us/group___event_handlers.html) section. Also updated docs for all events throughout the various APIs.
- Added debugging utilities for dumping object properties and formatting output (similar to NodeJS): `inspect()`, `console.dir()`, `printf()` and `%%o`/`%%O` specifier support for `sprintf()`/`printf()`. Also convenience formatting functions `console.infof()`,  `console.warnf()`,  `console.errorf()`.
- Added global `structuredClone()` JS standard function (as polyfill).
- Added `openUrlExternally()` global function which launches the system-defined handler for various URL schemes or file types.
- Added `callLater()` and `gcLater()` global functions which automatically consolidate multiple calls.
- Added `URL` class extensions (eg. for working with local file URLs) and also the missing `URL.parse()` and `URL.canParse()` standards. `URL` type now has own documentation section.
- Added `String.elideLeft()`, `String.elideMiddle()`, and `String.elideRight()` methods and static functions for trimming strings with an added visual indicator.
- Added `Color.toWebColor()`/`Color.web()` methods to return RGB/RRGGBB/RGBA/RRGGBBAA format based on alpha value being opaque or not, and static `Color.fromArgb()` and `Color.argb2rgba()` functions. Updated documentation to list the many suppoorted color input formats.
- Added `DSE.aboutToQuit` event which is fired just before plugin exits (stops running for whatever reason, besides crashing).
- Added `DOMException` constant equivalents of all standard exception names (eg. `DOMException.AbortError`), which can be compared to `code` property instead of using string names. Constructor also accepts constant as 2nd argument instead of name.

### Fixes
- In some cases scripts will now get a better chance of capturing errors (with try/catch) instead of them being caught/reported by the script engine itself before the catch clause can be invoked.
  Previously, the script engine could be capturing some errors before returning to the main event loop, preventing any user code from running and catching them. Now it will check "lazily" and allow user events to run before checking for uncaught exceptions.
- Fixed that an expression value couldn't be cleared (deleted) in Script File and Module actions after initially set to some non-empty value (had to delete the script instance first).
- `instanceof DOMException` now evaluates correctly for `DOMException` types. Note that `DOMException` inherits from `Error` so `instanceof Error` also returns `true`, which it already did prior to this fix.

### Other Changes
- When loading a module-type script, the plugin will log a warning if a different module has already been loaded into the current script environment using the same module alias.
- Log files are now written with a leading UTF-8 BOM.

**Full log:** [v1.2.1...v1.3.0](https://github.com/mpaperno/DSEP4TP/compare/1.2.1.0...1.3.0.0)

---
## 1.2.1.0 (14-May-2025)

- Changes for "On Hold" action options:
  - Activation/repeat options are now only shown in "On Hold" button setup flow.
  - Split activation type (On Press, On Release, etc) and repeat (yes/no) options into two separate choices.
  - Added Repeat Delay and Repeat Rate options which only affect the current button (not the global or instance-specific rate/delay settings).
- Changes for state default value handling:
  - Default type & value can be used on instances with any kind of persistence (saved or not).
  - Default type (Fixed Value, Custom Expression, Last Expression) and persistence (Session, Temporary, Saved) settings are now separate action data fields.
  - Changed to use actual default values when creating states and force TP evaluation (for non-empty values), removing workaround for TP v3 which created the state with an empty default value first and then sent the actual default value (if it wasn't empty).
  - Always create a TP state (with default value, unless disabled) even if evaluation results in undefined/null result.
  - Renamed `SavedDefaultType.NoSavedDefault` enum to `NoDefaultValue` (old version deprecated but still available in scripts).
- Added methods for getting file system directory listings and details about individual files/folders:
  - New `Dir.list()` and `Dir.infoList()` methods to list directory contents, with filtering and sorting options.
  - New `Dir.info()` and `File.info()` methods to get details about individual file system entries (the methods are equivalent and return information about either files or folders).
  - New `FileInfo` object type containing details about a file system object (name, type, size, timestamps, etc). Returned by `Dir.info()`/`File.info()` or as a list of entries from `Dir.infoList()`.
- Added a short delay after creating new states for script instances and before the plugin tries to update that new state with a value.
  This is a workaround for the TP issue where it may not register the new state before handling the state update message, which can result in TP log warnings about the state being updated "not belonging to this plugin".
- Added new arguments & overloads for `DSE.stateCreate()` method:
  - `delayMs` (integer) argument to delay further execution after creating a state to allow TP to process the state creation message before sending a `stateUpdate()` for the new state. (See previous item about using this as workaround for TP issue.)
  - `force` (boolean) argument to force TP to evaluate the default value in action flows as if it was a state update (otherwise it ignores the default in "plug-in state changed" events).
  - `stateCreate(id, description, defaultValue = "", delayMs = 0)` overload that automatically populates the parent category field.
  - `stateCreate(id, description, options)` overload taking an object of all optional arguments (see docs for details).
- Added new TP v4 properties to the `TP.broadcastEvent` event data for `pageChange` event (`previousPageName`, `deviceName`, `deviceId`, `deviceIp`).
- Fixed that `AbortSignal`'s `aborted` property was never set to `true` after the parent `AbortController`'s `abort()` method was called (the `abort` event was still emitted, but the property value didn't change).
- Fixed a possible (but apparently very rare) issue with none of the plugin's actions working on some particular setups due to "silent" Touch Portal error which resulted in the action never being sent to the plugin at all (thanks to `@Raeas` on TP's Discord for reporting and troubleshooting with us).
- The plugin's actions/connectors are now sorted into the Touch Portal "Tools" category instead of "Misc."
- Added current plugin version (in "dotted" format) and option descriptions to the plugin's Settings page.

**Full log:** [v1.2.0-beta1...v1.2.1](https://github.com/mpaperno/DSEP4TP/compare/1.2.0.1...1.2.1.0)

---
## 1.2.0.1-beta1 (20-Feb-2023)

This version bring a number of fundamental changes, all of which hopefully make the system more flexible overall.
I've preserved backwards compatibility with actions/connectors from the previous versions, except in the case of the "One-Time"
action (details below). I encourage everyone to update to the new versions of the actions/connectors, but please
do let me know if you find "regression" issues which break existing actions/connectors. An update to the new version is meant to be
as seamless as possible.

### Plugin Core
- New paradigm decouples "Script" (Expression/File/Module) instances from Engine instances and from Touch Portal States:
  - Creating a Touch Portal State for each named Script instance is now optional.
    **Consequently, the "State Name" field for each action has been renamed to "Instance Name."**
  - Private Engine instances can now be independent from Script instances; they become their own named entities which can be re-used.
  - Script instances can be run in any existing Private Engine instances (or create their own, or run in Shared engine, as before).
  - Engine instances now "own" the system thread they (and all associated Scripts) run in.
- Added support for using Actions in "On Hold" Touch Portal button setup, with multiple behavior options:
  - Activation options: On Press and/or On Release, Press and Repeat, Repeat After Delay.
  - Repeat delay and rate (interval) are controllable at both global default and per-instance levels.
  - Current repeat delay, rate, and maximum number of repeats can be set via instance properties.
- Script instance properties and methods are now available in the JavaScript environment, allowing extended code-level functionality and customization (details below).
- Scripts can now keep data in persistent storage which is saved and restored at plugin startup (for example to preserve state/settings between sessions).
- Any Script Instance can now be set to "temporary persistence" and deleted automatically after a delay. This removes the need for a separate "one-time" action.
- Added a plugin Setting to load/run a script file at plugin startup.
- Added stack trace logging for unhandled script exceptions.
- Added command-line switch to specify the Touch Portal Plugin ID to use in all communications (for advanced usage with custom entry.tp configuration).
- Fixed/removed automatic replacement of spaces with underscores in Touch Portal State names/IDs.
- Fixed all saved instances being removed if plugin is started w/out available Touch Portal host.
- The "Shared" Engine instance now runs in its own system thread so as not to impact core plugin functions.
- The Touch Portal network client (responsible for core communications) has also been moved to its own thread.
- Miscellaneous optimizations, cleanups, and stability improvements.

### Changes to Actions/Connectors
- Added "Create State" option to each of the primary Script actions (Expression/File/Module). This is now also where the State's default value type is set.
- Added "Instance Persistence" option which controls if a Script instance only exists for the current session, is saved at shutdown and loaded at startup, or is temporary (replaces "One-Time" action).
  - **REMOVED:** the "Anonymous (One-Time) Script" action (redundant with the more flexible new system).<br/>
    Unfortunately due to internal changes backwards compatibility could not be preserved in this one case.
- Script Instances can now use any existing Engine instance (eg. created by another Script) in addition to Shared and Private engine types as previously;
  - Selecting the "Private" choice effectively creates a new private Engine with the same name as the Script. This reproduces the previous behavior.
- Added "On Hold" option to each Script action type which controls when exactly Script evaluation happens if an Action is used in "On Hold" button setup tab.
- The "Update Existing Instance" action/connector:
  - Now uses a choice list to select an existing Script instance to update (instead of having to type one in).
  - Can run an expression in a "Shared Default" instance which evaluates in the Shared engine scope but independent of any specific named Script instance (and does not create a Touch Portal State).
- Renamed "Plugin Actions" to "Instance Control Actions" and:
  - Resets are now performed on named Engine instances, not Script Instances (this matters if multiple scripts share the same engine).
  - Added option to Delete Engine Instance(s).
  - Added options to Save, Load, and Remove Script Instance data to/from persistent storage.
  - **DEPRECATED:** "Set State Value" option;<br/>
    Existing instances will continue to work for now but cannot be edited; Support will be fully removed in the next major version;
- Added "Set Held Action Repeat Rate/Delay" action & connector to set/adjust repeat rate/delay at both global default and instance-specific levels.

### New States/Event
- Added "Plugin running state" (stopped/starting/started) Touch Portal State and Event;
- Added "Default held action Repeat Delay" and "Default held action Repeat Rate" States;

### JavaScript Library
- Added global `require()` function to import JS modules or JSON objects (whole or individual components) into any script,
  as an alternative to the `import ... from ...` syntax (which only works from other modules).
  This mimics the Node.js/CommonJS function(ality) of the same name.
- Script Instance properties and methods are now exposed to the JavaScript environment as `DynamicScript` objects (see docs for details).
  - This essentially gives direct access to all properties of the Touch Portal action/connector which is being used to create or run the Script instance,
    as well as extended features and options which are not practical to configure using the Touch Portal interface.
  - Among other features, scripts can now control the parent category their State will be sorted into as well as the "friendly" name
    displayed in Touch Portal selector lists (which no longer has to match the underlying State ID).
- Added native `Clipboard` module for interacting with operating system clipboard features.
- Added properties and methods to `DSE` object for working with `DynamicScript` and Engine instances. A few properties have also been deprecated.
  - Added properties:
    - `DSE.VALUE_STATE_PARENT_CATEOGRY`
    - `DSE.defaultActionRepeatRate`, `DSE.defaultActionRepeatDelay`
    - `DSE.engineInstanceType`, `DSE.engineInstanceName`, `DSE.currentInstanceName`
  - Added methods:
    - `DSE.instance()`,  `DSE.currentInstance()`, `DSE.instanceNames()`;
    - `DSE.setActionRepeat()`, `DSE.adjustActionRepeat()`
  - Added enumeration constants for various `DynamicScript` property values.
  - **DEPRECATED:**  These will continue to work for now but will be removed in the next major version. See documentation for alternatives.
    - `DSE.instanceStateId()` method;
    - `DSE.INSTANCE_TYPE`, `DSE.INSTANCE_NAME`, `DSE.INSTANCE_DEFAULT_VALUE` properties;
- Touch Portal API:
  - Added `TP.messageEvent()` which passes through every command/data message received from Touch Portal by the plugin.
  - Added `TP.choiceUpdateInstance()` and `TP.settingUpdate()` methods.
- Fixed that `Net` API functions (internal and public) were mistakenly available in global scope.
- Moved/renamed `GlobalRequestDefaults` to `Request.GlobalDefaults` (the former is now an alias for the latter but will be removed in a future version).
- Added `Math.percentOfRange()` and `Math.rangeValueToPercent()` functions.
- Fixed `Color.polyad()`, `Color.splitcomplement()` and `Color.analogous()` to preserve original alpha channel value.
- **CHANGED:** The global `include()` function when used with relative paths will now first try to resolve the specified file
  relative to the _current script/module file's location_. If that fails, it falls back to the previous behavior using the plugin's _Script Files Base Directory_ setting.

### Documentation
- Updated main _Plugin Documentation_ pages to reflect changes in actions/connectors and features
  (including the new overall relationship between Script instances, Engine instances, and Touch Portal States).
- Added _Plugin Documentation -> "Quick Start"_ page.
- Added _Plugin Documentation -> "Settings, States and Events"_ page.
- Added new JS Library Reference category for _Plugin API_ objects (`DSE` and `DynamicScript`).
- Added and updated references for all new or changed API.

---
## 1.1.0.2 (20-Feb-2023)
- No changes since 1.1.0.1-beta1.
- Final version in the v1.1.x series.

---
## 1.1.0.1-beta1 (05-Jan-2023)
- Added Connector (Slider) counterparts for most of the plugin's existing Actions (except "Load Script").
- Added Connector/Slider instance tracking database for managing mapping and lookups of "short" connector IDs (for sending updates back to Touch Portal).
- Added new State to reflect current system user's Touch Portal settings directory path (workaround until TP v3.2 adds native values for that).
- Added new State: "Name of Page currently active on TP device."
- Touch Portal API:
  - Added methods and an event for Connector ID lookup/search/tracking, eg. for sending Slider position updates to Touch Portal.
  - Added Touch Portal "broadcast" message event passthrough (eg. to notify scripts when user navigates to a new page).
  - Added `TP.currentPageName()` lookup function.
- JavaScript Library:
  - Added `String.appendLine()` and `String.getLines()` functions (both static and prototype).
  - Added `DSE.TP_USER_DATA_PATH` property (current system user's Touch Portal settings directory), `DSE.TP_VERSION_CODE` and `DSE.TP_VERSION_STR`.
- Documentation & Examples:
  - Moved Touch Portal API references into their own category.
  - Added documentation for new Touch Portal API functions/features.
  - New v3 of Color Picker example page and script, using new "native" connector/slider functionality and adding some more features.
  - Other minor updates and clarifications.

Thanks for everyone's input so far!

---
## 1.0.0.3 (05-Jan-2023)
- Fix evaluation at startup of default value expressions for saved states when the default type was set to "use action's expression" and the default value field was left blank.
- Fix "Anonymous (One-Shot)" type script instances not being properly deleted after finishing.
- Touch Portal API:
  - Set default showNotification() option name to a minimal required empty space character instead of "Dismiss" prompt which doesn't actually do anything anyway.

---
## 1.0.0.2-beta2 (28-Dec-2022)
- Fix crash on engine environment reset action.
- Fix missing `Number.prototype.round()` function.
- Add `TP.connectorUpdateShort()` method.
- Add Clipboard and Color Picker examples.

---
## 1.0.0.1-beta1 (26-Dec-2022)
- Initial release!
