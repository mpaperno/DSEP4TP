// This is a script to "tail" a file -- watching it for changes and reading the last N lines from the end.
// Typically used to monitor a log file, or some other text document, for changes.
// This version uses the `FSMonitor` file system monitor utility class to detect changes to the file being tailed.
//
// It demonstrates the use of `FSMonitor`, `File` utility static functions,
// as well as creating and updating Touch Portal states from inside a script.
//
// It is meant to be loaded as a module.

// We save data in the global scope, for simplicity.
// This module is designed to run in a Private engine instance (so the global scope is not shared).
var fileName = fileName || "";
var maxLines = maxLines || 0;
var statusStateId = statusStateId || "";
var fsWatch = fsWatch || null;
var fLastMod = fLastMod || null;

// This is the method to invoke from the TP action to start tailing a file.
// The arguments are the file path/name, and, optionally, the number of lines to read from the end (default is 6 lines).
export function tail(file, lines = 6)
{
	// make sure to stop any existing tail first
	cancel();

	// validate the function arguments.
	// DSE.INSTANCE_NAME is a constant representing the current instance which created and executed this module.
	if (!DSE.INSTANCE_NAME || !file || lines < 0)
		throw new TypeError(`Invalid arguments for 'tail(${file}, ${lines})' with instance name ${DSE.INSTANCE_NAME}.`);

	// Now validate that the file exists using the File utility.
	if (!File.exists(file))
		throw new URIError("File not found: " + file);

	// OK we're good to go... save the arguments for later use and reset the last file modified timestamp.
	fileName = File.normFilePath(file);
	maxLines = lines;
	fLastMod = new Date(0);

	// Log what we're doing (before starting to monitor the file in case we're tailing the plugin's own logs!)
	console.info(`Tailing ${lines} lines from ${fileName} for state ${DSE.INSTANCE_NAME}`);

	// Configure the FSWatcher instance to monitor the file for changes.
	fsWatch = new FSWatcher([fileName]);
	// Connect the watcher's `fileChanged` event to our `checkFile()` function (below).
	fsWatch.fileChanged.connect(checkFile);

	// We want to notify the user that the tail is running/active. One way we can do this is to create a new TP State
	// which will reflect the status of this operation. We use the current instance's state ID and name as the basis for the new
	// state ID and description, and we save it for future use (in `cancel()` or for next time this function is invoked);
	const newStatusStateId = DSE.instanceStateId() + "_active";
	// Only do this once if we don't already have a statusStateId from previous runs, or if has changed.
	if (statusStateId !== newStatusStateId) {
		statusStateId = newStatusStateId;
		// The arguments are: the State ID (must be unique), parent category name (within this plugin's categories, or a new one),
		// a description/name for the new state (shown in TP UI), the default value (0 for inactive),
		// flag to force TP to evaluate event handlers for this state, and adding a 2ms delay after the state is created.
		TP.stateCreate(statusStateId, "Dynamic Values", DSE.INSTANCE_NAME + " Active", "0", true, 2);
	}
	// Now send the state update indicating the tail operation is active.
	// This state change can be used to trigger an event in TP, eg. visually indicate the tail is active.
	TP.stateUpdateById(statusStateId, "1");

	// Run the initial file read operation now.
	checkFile();
}

// This function is called by the FSWatcher we created in tail() when the monitored file is modified.
// This checks the file modification time for changes, reads in any new data and updates
// the corresponding State in TP.
function checkFile()
{
	// Catch any errors so we can stop the tail and exit "gracefully" if the file couldn't be read.
	try {
		// Check if the file still exists. If it has been renamed or removed, try looking for a new file with the same name.
		if (!fsWatch.files().includes(fileName)) {
			if (!File.exists(fileName) || !fsWatch.addPath(fileName))
				throw new URIError("Monitored file has been removed");
		}

		// Get the watched file modification time, then compare it to the last saved time.
		const fmod = File.mtime(fileName);
		if (fmod.getTime() === fLastMod.getTime())
			return;

		// File has changed, save the new modification date.
		fLastMod = fmod;

		// Read in `maxLines` of text from the file, starting at the end (-1) and trimming any trailing newlines.
		// The lines are read asynchronously and then sent to Touch Portal as a state.
		// The `TP.stateUpdate(string)` version with one argument will automatically
		// update the state for the current instance which created and executed this module
		File.readLinesAsync(fileName, maxLines, -1, true)
		.then(TP.stateUpdate)
		.catch((ex) => { throw ex; });
	}
	catch (e) {
		cancel();
		console.exception(e);
		throw e;
	}
}

// Call this function to cancel the tailing operation.
export function cancel()
{
	if (fsWatch) {
		// Delete the file watcher
		fsWatch.destroy();
		fsWatch = null;

		// Update the state we created earlier to indicate the tail operation has stopped.
		TP.stateUpdateById(statusStateId, "0");
		// Log what we did
		console.info(`Stopped tailing ${fileName} for state ${DSE.INSTANCE_NAME}`);
		// We could also now delete the status state we created... this is optional.
		// But is would be best to schedule that to run a little later so TP can first properly register the state change we just sent.
		//setTimeout(function (){ TP.stateRemove(statusStateId) }, 1000);
	}
}

// Call this function to clear the contents of the log text state.
export function clear() {
	TP.stateUpdate("Log cleared.");
}
