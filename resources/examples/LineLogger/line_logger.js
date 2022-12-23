// This script stores and returns N last lines of text in a FIFO buffer style,
// discarding older lines as new ones come in.  Lines are added via the append() method.
//
// This particular implementation can also color-code an incoming script error log
// from the DSEP "Last script instance error" state.

// text lines storage
var log_data = log_data || [];

// regex for splitting up log lines for colorizing the different parts;
// captures error number, time stamp, instance name, and (optionally) error type.
// Anatomy & example of an error message:
// ID   TIMESTAMP     STATE NAME  ERROR TYPE     ... rest of the error.
// 015 [04:12:29.817] DSE_TestTwo ReferenceError: while evaluating 'run()' and script 'test_script.js' at line 33: FileHandl is not defined
const logLineRx = new RegExp(/^(\d+) \[([\d:\.]+)\] ([^\s]+) ([^\s]+:)?/, "g");

// Call this function from the TP action's expression with the line to append, the number of lines to keep,
// and whether to try to color-code parts of the log entry.
export function append(line, maxLines = 6, colorize = false)
{
	// optionally wrap special TP color code commands around the error number, time stamp, instance name and error type parts of the new line.
	if (colorize)
		line = line.replace(logLineRx, "[c#EB6A6A]$1[/c] [[c#1697E8]$2[/c]] [c#E8CC16]$3[/c] [c#CF62CF]$4[/c]");
	// clear out old data if needed
	if (log_data.length >= maxLines)
		log_data.splice(0, log_data.length - maxLines + 1);
	// add the new line
	log_data.push(line);
	// join the stored lines array and return it; this will get sent back to TP as a state update for this instance's state name.
	return log_data.join('\n');
}

// Convenience function to clear the saved lines. Optional message to return as TP state value.
export function clear(returnString = "Script Log Cleared!") {
	log_data = [];
	if (typeof returnString !== 'undefined')
		return returnString;
}
