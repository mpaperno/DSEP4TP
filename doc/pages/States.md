# Settings, States and Events {#plugin_states_page}

[TOC]

## Settings {#plugin_settings}

The plugin settings screen is accessed from the Touch Portal main Settings dialog, which is found by clicking on the gear icon at top right of the main
screen, the selecting _Settings -> Plug-ins -> Dynamic Script Engine_.

<a href="images/misc/settings-v1_2.png" target="fullSizeImg"><img src="images/misc/settings-v1_2.png?" style="max-width: 470px"></a>

Besides providing access to some basic plugin settings, this screen also shows the currently installed plugin version, although in a somewhat "encoded"
format. To "translate" it to published version numbers, read it from right to left and put a dot after every 2 digits.<br/>
So in this example the version number is `1.02.00.00`, which, w/out the "extra" zeros, comes out as 1.2.0.0 and corresponds to the version number
format used everywhere else in the real world. (This version format is a "TP thing" and beyond my control.)

* **Script Files Base Directory** - This controls where the plugin looks for script and module files when they are specified in actions/connectors
  using relative system paths (vs. absolute paths which include the drive letter or root directory). By default the plugin will look for scripts
  relative to its installation folder, which generally is not very useful. This settings can be any valid folder/directory path on your system
  to which Touch Portal (and this plugin) will have at least read-only permission.

* **Load Script At Startup** - Optionally any script file can be loaded and evaluated at plugin start, right after a connection to Touch Portal
  has been established (and before any other saved scripts load). The script will be loaded into the Shared engine instance.<br/>
  Relative paths are resolved using the _Script Files Base Directory_ setting, above.

* **Settings Version** - This is a read-only "setting" for internal plugin use in case of future changes to the settings structure.

## States {#plugin_states}

* **List of created named instances** - This state contains a list of all the dynamic expression/script instances created by the plugin.
  It may be useful to check (visually or with a conditional expression) whether an instance exists, for example to indicate an error condition if it doesn't
  or only create a new instance inside an Event if it doesn't already exist.

* **Cumulative script error count** - A running count of script errors which increments on each exception.
  This is useful as a trigger for some Touch Portal event which would notify you that a new error occurred. A button that turns red or something.

* **Last script instance error** - This state will update every time the script error count changes and will contain the text of the last script error message.
  This could be shown somewhere in a Touch Portal page for example. The error message includes the error number (which always increments) and a time stamp,
  so therefore will always be unique and can be safely used in Touch Portal's "when plug-in state changes" event.

* **Plugin running state** - This value is always one of: "Stopped", "Starting", or "Started". It reflects the current state of the plugin
  _when it is operating normally._ Meaning that if it crashes unexpectedly, this state will _not_ get updated. If the plugin is stopped or started
  in a normal fashion (eg. via Settings -> Plug-ins -> Stop/Start) then this status will be correct. Also see the corresponding Event, below.

* **Default held action Repeat Delay (ms)** - This reflects the currently set default Action Repeat Delay setting, which is controllable
  by provided actions/connectors (as described in the respective documentation), or from within the scripting environment itself.

* **Default held action Repeat Rate (ms)** - This reflects the currently set default Action Repeat Rate setting, which is controllable
  by provided actions/connectors (as described in the respective documentation), or from within the scripting environment itself.

* **Touch Portal data folder (current user)** - This is a "convenience" State not directly related to this plugin but is useful since Touch Portal doesn't
  currently have a value to reflect this setting. The value of this State always corresponds to the "data" folder, where all the settings, pages, plugins, etc,
  are stored. This should be the same path as shown in TP's _Settings -> Info_ dialog for the _Data folder_ entry.

* **Name of Page currently active on TP device** - Another "convenience" since there is no way to get this information right now in a Touch Portal actin flow.
  The value reflects the last page that was opened on the connected Touch Portal device, and changes accordingly.
  **Note:** This value is empty/blank until at least one page change has happened on the device _after_ the plugin has been started. Touch Portal only
  sends page change notices to plugins when the change happens, not when the plugin first connects.

## Event {#plugin_events}

* **Plugin running state change** - This Event corresponds to the _Plugin running state_ State described above. It has the 3 corresponding choices for
  plugin status -- "Stopped", "Starting", and "Started". It could be used to change an indicator on a button, for example.

<a href="images/misc/running-state-event.png" target="fullSizeImg"><img src="images/misc/running-state-event.png?" style="max-width: 470px"></a>

---

<span class="next_section_button">
Read Next: [Status and Logging](Status.md)
</span>
