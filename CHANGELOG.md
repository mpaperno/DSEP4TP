# Change Log
**Dynamic Script Engine Plugin for Touch Portal**: changes by version number and release date.

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
