
# Status and Logging {#plugin_status_and_logging}

[TOC]

## Introduction

Inevitably, writing scripting expressions of any complexity beyond basic "2 + 2" is going to cause an error sooner or later. Maybe due to syntax or some
unexpected value being passed to a function. It happens.

The plugin has a number of ways to provide feedback about errors and facilitate debugging. There are also extensive file logging options available.

## Status States

First, the plugin provides two permanent States which reflect scripting errors as they happen:
* **Cumulative script error count** - A running count of script errors which increments on each exception.
  This is useful as a trigger for some Touch Portal event which would notify you that a new error occurred. A button that turns red or something.
* **Last script instance error** - This state will update every time the error count changes and will contain the text of the last error message.
  This could be shown somewhere in a Touch Portal page for example. The error message includes the error number (which always increments) and a time stamp,
	so therefore will always be unique and can be safely used in Touch Portal's "when plug-in state changes" event.

Check the [Line Logger example](@ref example_line_logger) for a script/button combination which will store and display the last few errors received.

There is also:
* **List of created named instances** - This state contains a list of all the dynamic expression/script instances created by the plugin.
  It may be useful to check (visually or with a conditional expression) whether an instance exists, for example to indicate an error condition if it doesn't
	or only create a new instance inside an Event if it doesn't already exist.

## Logging

The plugin logs operational message to a file. Two, actually. They are both located in your Touch Portal configuration folder, in the 'plugins/DSEP4TP' subfolder;
the path looks like this:<br/>
* Windows: `C:\Users\<User_Name>\AppData\Roaming\TouchPortal\plugins\DSEP4TP\logs`
* Mac: `~/Documents/TouchPortal/plugins/DSEP4TP/logs`
* Linux: `~/.config/TouchPortal/plugins/DSEP4TP/logs`

The two log files are:

### "Console" Log

Scripting errors are logged to their own file in the log folder named `console.log`.
You can also write your own messages to this log by using the typical JavaScript `console.log()` (and [family](@ref std-console)) of functions.

This should be the first point of reference when something doesn't work right (usual symptom: "nothing happens when I run the script.")
When actively working on a script or expression, it will save a lot of time to keep a constant eye on the log (and the error states described above).

Check the [Tail example](@ref example_tail) for a script/button combination which will "tail" the console log on demand (that is, show the last few
lines of the log and update whenever the log changes).

You can also "tail" a file using operating system utilities. For example (showing last 10 lines)
- Windows [PowerShell](https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.management/get-content)<br />
  ```ps
  Get-Content -Tail 10 -Wait $Env:APPDATA\TouchPortal\plugins\DSEP4TP\logs\console.log
  ```
- [OS X](https://ss64.com/osx/tail.html) terminal
  ```shell
  tail -F -n 10 ~/Documents/TouchPortal/plugins/DSEP4TP/logs/console.log
  ```
- [Linux](https://ss64.com/bash/tail.html) terminal
  ```shell
  tail -F -n 10 ~/.config/TouchPortal/plugins/DSEP4TP/logs/console.log
  ```

### Plugin Log

The main plugin log, cleverly named `plugin.log`, is where all messages are written, including general operational information from the plugin,
especially any errors it may have encountered that are not directly related to your scripts (but maybe to the actions you're sending to the plugin,
like a missing required data field, or some other overall operational issue).

This log _also_ includes script engine errors (same ones as in _console.log_). It can be helpful to get the "full picture" if something particularly
insidious is going wrong.


### Logging Options

The logs are rotated daily. The previous days' log is renamed with a date stamp and a new one is started.
By default the last 3 days' worth of logs are kept (the current one + 3 historical logs).

You can force a rotation of the logs by going to the plugin's install directory (*)
in a terminal/command window and run the plugin (w/out starting it) with the following command:<br />
* Windows: `bin/DSEP4TP.exe -f1 -j1 -rx`
* Mac: `./DSEP4TP.app/Contents/MacOS/DSEP4TP -f1 -j0 -rx`
* Linux: `./bin/DSEP4TP -f1 -j0 -rx`
* Optionally add a `-k N` switch to the command line to keep `N` number of logs instead of the default 3. Clear out all old logs by using `-k 0`
* Use `-h`  to see all command-line options.

The log files have an "emergency limit" of 1GB in case something goes haywire. If this limit is reached, further logging to that file is
disabled until the plugin is restarted.

(*) Plugin install location:
* Windows: `C:\Users\<User_Name>\AppData\Roaming\TouchPortal\plugins\DSEP4TP`
* Mac: `~/Documents/TouchPortal/plugins/DSEP4TP`
* Linux: `~/.config/TouchPortal/plugins/DSEP4TP`


<span class="next_section_button">
Read Next: [Scripting Syntax and Caveats](Scripting.md)
</span>
