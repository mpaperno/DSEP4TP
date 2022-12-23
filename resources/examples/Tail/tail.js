// This is a script to "tail" a file -- watching it for changes and reading the last N lines from the end.
// Typically used to monitor a log file, or some other text document, for changes.
// This version is fairly simplistic in that it only checks the monitored file for changes on a schedule,
// vs. some sort of system-level monitoring a file/directory for modifications.
// It demonstrates the use of timed recurring events, File object static functions,
// as well as creating and updating Touch Portal states from inside a script.
// It is meant to be loaded as a module.

// We save data in the global scope, for simplicity.
// This module is designed to run in a Private engine instance (so the global scope is not shared).
var fileName = fileName || "";
var maxLines = maxLines || 0;
var statusStateId = statusStateId || "";
var fLastMod = fLastMod || new Date(0);
var timerId = timerId === undefined ? -1 : timerId;

// This is the method to invoke from the TP action to start tailing a file.
// The arguments are the file path/name, the number of lines to read from the end,
// and how often to check for changes (in milliseconds);
export function tail(file, lines = 6, interval = 1000)
{
	// make sure to stop any existing interval timer
	cancel();
	// validate the function arguments.
	// DSE.INSTANCE_NAME is a constant representing the current instance which created and executed this module.
	if (!DSE.INSTANCE_NAME || !file || lines < 0 || interval < 1) {
		throw new TypeError(`Invalid arguments for 'tail(${file}, ${lines}, ${interval})' with instance name ${DSE.INSTANCE_NAME}.`);
		return;
	}
	// Now validate that the file exists using the File utility.
	if (!File.exists(file)) {
		throw new URIError("File not found: " + file);
		return;
	}
	// OK we're good to go... save the arguments for later use and set up the interval timer.
	fileName = file;
	maxLines = lines;
	fLastMod = new Date();
	// This will start a timer which calls our `checkFile()` function (below) every `interval` milliseconds.
	timerId = setInterval(checkFile, interval);

	// We want to notify the user that the tail is running/active. One way we can do this is to create a new TP State
	// which will reflect the status of this operation. We use the current instance's state ID and name as the basis for the new
	// state ID and description, and we save it for future use (in `cancel()` or for next time this function is invoked);
	const newStatusStateId = DSE.instanceStateId() + "_active";
	// Only do this once if we don't already have a statusStateId from previous runs, or if has changed.
	if (statusStateId !== newStatusStateId) {
		statusStateId = newStatusStateId;
		// The arguments are: the State ID (must be unique), parent category name (within this plugin's categories, or a new one),
		// a description/name for the new state (shown in TP UI), and finally the default value (0 for inactive, 1 for active).
		TP.stateCreate(statusStateId, "Dynamic Values", DSE.INSTANCE_NAME + " Active", "0");
	}
	// Now send the state update indicating the tail operation is active.
	// This state change can be used to trigger an event in TP, eg. visually indicate the tail is active.
	TP.stateUpdateById(statusStateId, "1");

	// Log what we did.
	console.info(`Tailing ${lines} lines from ${file} every ${interval}ms for state ${DSE.INSTANCE_NAME} with timerId ${timerId}`);

	// Run the initial file read operation now, unless it's going to be read very soon by the scheduled timer anyway.
	if (interval > 100)
		checkFile();
}

// This function is called periodically by the setInterval() timer we started in tail().
// This checks the file modification time for changes, reads in any new data and updates
// the corresponding State in TP.
function checkFile()
{
	// Catch any errors so we can stop the timer and exit "gracefully" if the file couldn't be read.
	try {
		// Get the watched file modification time, then compare it to the last saved time.
		var fmod = File.mtime(fileName);
		//console.log(fmod, fLastMod, fmod.getTime(), fLastMod.getTime(), fmod.getTime() === fLastMod.getTime());
		if (fmod.getTime() === fLastMod.getTime())
			return;

		// File has changed, save the new modification date.
		fLastMod = fmod;

		// Read in `maxLines` of text from the file, starting at the end (-1) and trimming any trailing newlines.
		var lines = File.readLines(fileName, maxLines, -1, true);
		// Rend the results to TP as a state. The TP.stateUpdate() version with one argument will automatically
		// update the state for the current instance which created and executed this module
		TP.stateUpdate(lines);
		//console.log('\n' + lines);
	}
	catch (e) {
		cancel();
		console.error(e);
		throw e;
	}
}

// Call this function to cancel the tailing operation.
export function cancel()
{
	if (timerId > -1) {
		// Stop the timer
		clearInterval(timerId);
		// Update the state we created earlier to indicate the tail operation has stopped.
		TP.stateUpdateById(statusStateId, "0");
		// Log what we did
		console.info(`Stopped tailing ${fileName} for state ${DSE.INSTANCE_NAME} with timerId ${timerId}`);
		// We could also now delete the status state we created... this is optional.
		// But is would be best to schedule that to run a little later so TP can first properly register the state change we just sent.
		//setTimeout(function (){ TP.stateRemove(statusStateId) }, 1000);
	}
	timerId = -1;
}
