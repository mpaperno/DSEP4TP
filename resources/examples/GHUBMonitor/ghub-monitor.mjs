/* Dynamic Script Engine - G HUB Battery Monitor Script

 A script to monitor battery level of Logitech™ G HUB® devices using WebSockets protocol.

 This is in a lot of ways like a mini-plugin in itself, as it provides a lot of functionality one would otherwise need
 an actual Touch Portal plugin for.

Documentation and examples published at: https://dse.tpp.max.paperno.us/example_ghub_monitor.html

Any function or variable marked as `export` can be used in a Touch Portal DSE action
or imported into another script/module with `import` statement or `require()` function.

---------
Copyright Maxim Paperno; all rights reserved.

This file may be used under the terms of the GNU
General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

A copy of the GNU General Public License is available at <http://www.gnu.org/licenses/>.
*/

/**
	Script constant/default values.
	They can be read via the module's import alias, but not set.
	The module must be reloaded after editing any of these.
*/
export const
	/** A friendly name for this "app." */
	SCRIPT_NAME = "G HUB Monitor",
	/** This default image path assumes they were imported into Touch Portal as an "icon pack" named "GHUB Monitor Icons".
		Due to how `Dir.normalize()` works, the default image path will be empty if this icon pack doesn't exist, and status images are disabled. */
	DEFAULT_IMAGES_PATH = Dir.normalize(DSE.TP_USER_DATA_PATH + "/iconpacks/GHUB Monitor Icons"),
	/** GHUB WebSocket server address. GHUB only listens on localhost (127.0.0.1) address. */
	GHUB_WS_URL = "ws://localhost:9010",
	/** All TP states created by this script will have the following prefix (state IDs must be unique throughout TP). */
	STATES_BASE_ID = DSE.VALUE_STATE_PREFIX + DSE.currentInstanceName,
	/** Prefix all messages to the plugin's log files with this text (for filtering/etc). */
	LOG_PREFIX = SCRIPT_NAME + ":"
;

/**
	Script runtime settings. Changes will take affect w/out restarting the script.
	These can be modified directly using the module's import alias, or via the `setOption()` function (recommended).

	For example, if the import is aliased as "M":
		M.setOption("DeviceInactiveAction", "keep");

	Or, directly:
		M.Settings.DeviceInactiveAction = "keep";

	After modifying Settings values directly, call `M.saveSettings()` to persist settings between script runs.
	`setOption()` does this for you.
*/
export const Settings =
{
	/**
		How often to re-try connecting to G HUB if a connection attempt times out or G HUB disconnects unexpectedly (eg. when it exits or restarts).
		The value is specified in seconds. Set this to 0 (zero) to disable automatic re-connection attempts.
	*/
	ConnectRetryTimeSec: 30,
	/**
		Maximum number of times to attempt an automatic re-connection to G HUB.
		After this many tries, automatic re-connection is disabled until a new connection is attempted manually (eg. by calling `connect()`).
		If set to zero, connections will be attempted indefinitely.
	*/
	ConnectRetryMaxCount: 20,
	/**
		What to do when a monitored device becomes inactive (eg. going to sleep, turned off, GHUB connection is lost, etc).
		The choices are:
		* 'keep' - Keep all battery state data at their last known values, including the last sent image (if any).
			Essentially does nothing besides update the "active" state of the device.
		* 'reset' - Reset all battery state data to default "unknown" values (percentage = -1, charging = false, etc), and send default image (if images are enabled).
		* 'resetImage' - Keeps all battery values at their last values (like 'keep') but sends the "default" image if images are enabled.
			Because if using the images provided by this script, there's no other way to clear that image from the button.
		Note that the "active" state of the device will always be updated, so this change can also be handled entirely on the TP side by setting deviceInactiveAction='keep'.
	*/
	DeviceInactiveAction: 'reset',
	/**
		Absolute path to battery status image files.
		Set this to an empty string to disable sending images altogether.
		By default they are assumed to be next to the location of this script file in a sub-folder named "images".
	*/
	ImagesPath: DEFAULT_IMAGES_PATH,
	/**
		Status image files begin with this string (then suffix is appended based on battery level).
		See also `batteryLevelToImageName()` function below.
	*/
	ImagePrefix: "h-battery-",
	/** Status image file name suffix with image type extension. */
	ImageSuffix: ".png",
};

//
// The following functions may be useful to call from TP actions or other module consumers.
// They're accessed using the module's "import alias" name as defined in the plugin's action which started this script.
// For example if the import is named "M" then use syntax like `M.functionName()` to call these functions (eg. `M.connect()`).
//

/** Attempts to start a WebSocket connection to GHUB (if not already connected). */
export function connect()
{
	_data.reconnectCount = 0;
	open();
}

/** Disconnects from G HUB (if connected).
	A new connection will not be attempted until the next time `connect()` is called.
*/
export function disconnect() {
	close();
}

/**
	Request a list of devices from GHUB. This will try to connect to GHUB if it isn't already connected.
	The response will be scanned for new/changed devices.
	An initial list request is automatically sent when a new connection to GHUB is opened.
	After that it can be initiated on demand, if needed.
	Note that this script also subscribes to the "/devices/state/changed" event when a new connection is established,
	which should (in theory) keep the devices list (and each device's status) updated automatically.
*/
export function listDevices()
{
	if (isConnected())
		send({
			verb: "GET",
			path: "/devices/list"
		});
	else
		open();
}

/** Returns `true` if a connection to GHUB is currently open, `false` otherwise. */
export function isConnected() {
	return _data.ws?.readyState == WebSocket.OPEN;
}

/**
	Change an option value in the Settings object.

	`name` argument can be any of the properties listed in the `Settings` object (above).
	`value` argument is the new value to assign to the property.
		The value's type (string/number) must match the type of the corresponding `Settings` property.

	For example, if the import is aliased as "M":
		M.setOption("deviceInactiveAction", "keep");

	Using this function will automatically save the updated settings to the script's persistend data (using `saveSettings()`).
*/
export function setOption(name, value)
{
	if (name in Settings && typeof Settings[name] == typeof value && Settings[name] !== value) {
		Settings[name] = value;
		console.info(LOG_PREFIX, `Set option '${name}' to '${value}'`);
		callLater(saveSettings);
	}
}

/**
	Saves current `Settings` object properties to the script's persistent storage.
	These values will be restored to the runtime `Settings` object the next time this script starts.
*/
export function saveSettings() {
	_data.instance.dataStore.Settings = structuredClone(Settings);
	console.info(LOG_PREFIX, "Saved settings.");
}


//
//  Internal use functions
//

/**
	This function returns an image file suffix corresponding to the given battery percentage `level` and `charging` status.
	For example if battery level is ~50% it may return "50" or if also charging then "50-charging".
	If level is < 0 it returns "inactive".
	This can be adjusted as desired based on how many images are being used and the percentage ranges at which each should be shown.
	The "inactive" image is also used as the default image when a new device is detected, before we read any battery info from it.
*/
function batteryLevelToImageName(level, charging = false)
{
	if (level < 0)
		return "inactive";
	const chrgSfx = charging ? "-charging" : "";
	if (level < 10)
		return "0" + chrgSfx;
	if (level < 35)
		return "25" + chrgSfx;
	if (level < 65)
		return "50" + chrgSfx;
	if (level < 90)
		return "75" + chrgSfx;
	return "100" + chrgSfx;
}


/**
	Instances of the `GHUBDevice` class are used to track detected GHUB devices.
	A new one is created for each device which reports a battery status.
*/
class GHUBDevice
{
	constructor(init = {})
	{
		this.id;              // current GHUB ID (may change between GHUB runs)
		this.name;            // short device name
		this.fullName;        // long device name
		this.type;            // GHUB device type string
		this.active = false;  // device is marked as "ACTIVE" in GHUB
		this.stateId;         // base prefix string for all TP states for this device
		this.lastImage = "";  // last battery image sent to TP (to avoid duplication)
		this.battery = {
			percentage: -1,     // reported percentage, -1 if unknown
			charging: false,    // flag to indicate current charging status
			mileage: null,      // reported battery hours remaining; null means not supported, -1 means currently unknown
			lastUpdate: 0,      // timestamp of last received update
		};
		// Assign any property values passed in `init` argument;
		// Object.assign() is not recursive so if `init.battery` object is present, assign those properties first and remove it
		if (init?.battery != undefined) {
			if (typeof init.battery == 'object')
				Object.assign(this.battery, init.battery);
			delete init.battery;
		}
		Object.assign(this, init);

		this.updateImageState = this.updateImageState.bind(this);
	}

	/**
		Sets the active state of this device. Updates TP states as needed.
		If active, subscribes to GHUB battery updates and requests immediate status.
		If inactive, also updates the battery icon for this device with the default image (if images are enabled).
	*/
	setActive(active)
	{
		if (active == this.active)
			return;
		this.active = active;
		// let TP know our new status
		TP.stateUpdateById(`${this.stateId}.active`, active ? "1" : "0");
		// sub/unsub to/from GHUB battery state change messages
		subscribeDeviceBatteryState(this, active);
		if (active) {
			// if switching to active mode, request an immediate update from GHUB;
			// battery values and TP states will be updated once the response is received
			getDeviceBatteryState(this);
		}
		// if switching to being inactive, we have some options...
		else if (Settings.DeviceInactiveAction == 'reset') {
			// We could clear the current battery data and send state updates
			this.resetBatteryData(true);
		}
		else if (Settings.DeviceInactiveAction == 'resetImage') {
			// Or keep the last data intact and just send default "inactive" image as a visual indicator that device is inactive
			this.updateBatteryStateImage(batteryLevelToImageName(-1));
		}
		// Or do nothing and have handlers on TP side react to the "active" state change to change visuals/etc.
	}

	/** Resets battery data to default "unknown" values and optionally sends TP state updates for this device. */
	resetBatteryData(withUpdate = false)
	{
		this.battery.percentage = -1;
		this.battery.charging = false;
		if (this.battery.mileage != null)
			this.battery.mileage = -1;
		if (withUpdate)
			this.updateBatteryStates();
	}

	/** Creates all TP States for this device. */
	createStates()
	{
		const stateId = this.stateId,
			name = this.name,
			parentGroup = `G HUB Device - ${name}`;

		if (!stateId || !name)
			return;

		// All states for this device will appear grouped under this sub-category of "Dynamic Script Engine" plugin states.
		TP.stateCreate(`${stateId}.level`,    parentGroup, `${name} Battery Level`, "-1");
		TP.stateCreate(`${stateId}.mileage`,  parentGroup, `${name} Battery Hours Remaining`, "-1");
		TP.stateCreate(`${stateId}.name`,     parentGroup, `${name} Device Name`, name);
		TP.stateCreate(`${stateId}.active`,   parentGroup, `${name} Is Active`, "0");
		TP.stateCreate(`${stateId}.charging`, parentGroup, `${name} Is Charging`, "0");
		if (Settings.ImagesPath) {
			// Create image state with empty default value
			TP.stateCreate(`${stateId}.image`,  parentGroup, `${name} Battery Image`, "");
			// Send the initial "inactive" battery image asynchronously
			this.updateBatteryStateImage(batteryLevelToImageName(-1));
		}
	}

	/**
		Sends TP battery state updates for this device.
		The states should already have been created before this.
	*/
	updateBatteryStates()
	{
		if (!this.stateId)
			return;

		const b = this.battery;
		TP.stateUpdateById(`${this.stateId}.level`,    b.percentage.toFixed(0));
		TP.stateUpdateById(`${this.stateId}.charging`, b.charging ? "1" : "0");
		if (b.mileage != null)
			TP.stateUpdateById(`${this.stateId}.mileage`,  b.mileage.toFixed(2));
		// Update battery status image if it changes.
		this.updateBatteryStateImage(batteryLevelToImageName(b.percentage, b.charging));
	}

	/**
		Sends a TP state update for this device with image data read from `imgName`.
		The image name is first compared to the last image sent for this device, and the update is skipped if they match.
		Otherwise the device's last sent image name is set to the new one from `imgName` and an update is sent.
		Image name is qualified into a full name & path using `ImagesPath`, `ImagePrefix` and `ImageSuffix` settings.
		Reads and sends image data asynchronously by default, unless `synchronous` is to to `true`
		or the `_data.aboutToQuit` flag is `true`.
	*/
	updateBatteryStateImage(imgName, synchronous = _data.aboutToQuit)
	{
		// Do nothing if images are disabled or this particular image has already been sent for this device
		if (!Settings.ImagesPath || this.lastImage == imgName)
			return;

		this.lastImage = imgName;
		const imgPath = `${Settings.ImagesPath}/${Settings.ImagePrefix}${imgName}${Settings.ImageSuffix}`;
		// console.debug(LOG_PREFIX, `Sending new image for "${this.stateId}.image" from ${this.battery.percentage}%/${this.battery.charging}:`, imgPath);

		if (synchronous) {
			try { this.updateImageState(File.read(imgPath, FS.O_BIN)); }
			catch (ex) { console.exception(ex); }
			return;
		}

		File.readAsync(imgPath, FS.O_BIN)
		.then(this.updateImageState)
		.catch(console.exception);
	}

	/** @param img {ArrayBuffer} */
	updateImageState(img) {
		if (img?.byteLength)
			TP.stateUpdateById(`${this.stateId}.image`, img.toBase64());
	}

}  // GHUBDevice class


// GHUB device and battery state message parsing

/** Creates a new instance of GHUBDevice from a GHUB "logi.protocol.devices.Device.Info" type message. */
function newDevice(devInfo)
{
	let name = devInfo.displayName;
	// Make sure device name is unique. GHUB won't do this for us!
	let count = 0;
	Array.from(_data.devices.values()).forEach((d) => { if (d.name.startsWith(name)) ++count; } )
	if (count)
		name += ` (${count})`;
	const d = new GHUBDevice({
		id: devInfo.id,
		name: name,
		fullName: devInfo.extendedDisplayName,
		type: devInfo.deviceType,
		stateId: `${STATES_BASE_ID}.${name}`,
	});
	console.info(LOG_PREFIX, `Added new device '${d.fullName}' of type ${d.type}`);
	d.createStates();
	return d;
}

/**
	Processes a list of devices from GHUB and calls `parseDevice()` for each one.
	`deviceInfos` should be an array of GHUB "logi.protocol.devices.Device.Info" type messages.
*/
function parseDeviceList(deviceInfos)
{
	if (Array.isArray(deviceInfos)) {
		for (const dev of deviceInfos)
			parseDevice(dev);
	}
}

/**
	Parse a device info structure from GHUB "logi.protocol.devices.Device.Info" type message.
	If the device supports battery status monitoring then it is added to the list of monitored devices.
	This also detects changes in existing device status, eg. going from ACTIVE to INACTIVE.
*/
function parseDevice(devInfo)
{
	if (!devInfo?.id) {
		console.error(LOG_PREFIX, "Missing device data in device info message.");
		return;
	}
	// We only care about devices which have batteries
	if (!devInfo.capabilities?.hasBatteryStatus) {
		console.info(LOG_PREFIX, `Skipping device '${devInfo.displayName}' because GHUB says it doesn't have a battery.`);
		return;
	}
	// console.debugf("Processing device info message: %1O", devInfo);

	let d = _data.devices.get(devInfo.id);
	if (!d)
		_data.devices.set(devInfo.id, (d = newDevice(devInfo)));

	d.setActive(devInfo.state == "ACTIVE" || devInfo.state == "PRESENT");
}

/** Parses a GHUB battery status message and updates our monitored device with the new data.
	`state` should be a "logi.protocol.wireless.Battery" type message object.
*/
function parseBatteryState(state)
{
	if (!state || !state.deviceId) {
		console.error(LOG_PREFIX, "Missing device data in battery status message.");
		return;
	}
	const d = _data.devices.get(state.deviceId);
	if (!d) {
		console.error(LOG_PREFIX, "Unknown Device ID in battery status message:", state.deviceId);
		return;
	}
	// console.debugf("Processing device battery status message: %2O", payload);

	// update device battery data
	d.battery.percentage = state.percentage;
	d.battery.charging = state.charging;

	// Apparently not all devices support "mileage" estimates...
	// if we already set the mileage in a previous update then it will be non-null,
	// otherwise check the payload's `batteryMileageSupport` property value
	if (d.battery.mileage != null || state.batteryMileageSupport == "MILEAGE_SUPPORTED")
		d.battery.mileage = state.mileage;

	// update timestamp
	d.battery.lastUpdate = Date.now();
	// send state updates
	d.updateBatteryStates();
}


// General status update utilities

/**
	For all tracked devices, this updates the "Active" state value to "0" and
	sends the default battery icon image (if images are enabled).
	This function is called automatically when GHUB gets disconnected (for whatever reason).
*/
function deactivateAllDevices()
{
	for (const d of _data.devices.values())
		d.setActive(false);
}

/** Updates the "GHUB Connection Status" state value. Called when the WebSocket to GHUB connects or disconnects.
	`active` meaning is: 0 = disconnected; 1 = connecting; 2 = connected
	If `active` is 0 then also calls `deactivateAllDevices()`. */
function updateGHubStatus(active) {
	TP.stateUpdateById(_data.ghubStatusStateId, active+"");
	if (!active)
		deactivateAllDevices();
}


// WebSocket open/close/reconnect functions

/**
	Attempts to open a WebSocket connection to GHUB.
	@param cb Optional callback to run after a successful socket connection.
		This is connected to the WebSocket's "opened" event.
*/
function open(cb = null)
{
	if (!_data.init)
		init();
	if (_data.ws && !isConnected() && _data.ws.readyState != WebSocket.CONNECTING) {
		console.info(LOG_PREFIX, "Connecting to G HUB @", GHUB_WS_URL);
		cancelConnectionAttempt();
		updateGHubStatus(1);
		_data.ws.open(cb);
	}
}

/** Closes an open WebSocket connection to GHUB. */
function close()
{
	if (_data.ws && _data.ws.readyState < WebSocket.CLOSING)
		_data.ws.close();
}

/** Resets reconnection timer and attempts to open a connection. Called from timer. */
function attempConnection() {
	_data.reconnectTimerId = null;
	open();
}

/** Cancels re-connection timer if one has been started. */
function cancelConnectionAttempt() {
	if (_data.reconnectTimerId) {
		clearTimeout(_data.reconnectTimerId);
		_data.reconnectTimerId = null;
	}
}

/** Possibly schedules a re-connection attempt if auto-connection is enabled and retry count hasn't been exceeded.
	Cancels any currently pending reconnection timer first. */
function scheduleConnectionAttempt() {
	cancelConnectionAttempt();
	if (Settings.ConnectRetryTimeSec > 0 && (Settings.ConnectRetryMaxCount <= 0 || _data.reconnectCount <= Settings.ConnectRetryMaxCount)) {
		++_data.reconnectCount;
		_data.reconnectTimerId = setTimeout(attempConnection, Settings.ConnectRetryTimeSec * 1000);
		console.info(LOG_PREFIX, `Attempting to re-connect in ${Settings.ConnectRetryTimeSec} seconds...`);
	}
}

/** Plugin shutdown event handler, runs when plugin sends an 'aboutToQuit' event. */
function onAboutToQuit() {
	_data.aboutToQuit = true;
	updateGHubStatus(0);
	saveSettings();
}

// GHUB WebSocket message senders

/**
	Send a message to GHUB WebSockets server.
	`message` should be an object with, at minimum, `verb` and `path` properties.
	If `connectIfClosed` is `true`, this will attempt to open a new connection
	to the GHUB server if one isn't already opened, then send the message once connected.
*/
function send(message, connectIfClosed = true)
{
	if (!_data?.ws)
		return;
	if (!('verb' in message) || !('path' in message))
		throw new TypeError("'verb' and 'path' message properties are required for send(message)");

	if (!isConnected()) {
		if (connectIfClosed)
			open(() => send(message));
		else
			console.error(LOG_PREFIX, "WebSocket connection is closed.");
		return;
	}
	if (!('msgId' in message))
		message.msgId = "";
	_data.ws.send(JSON.stringify(message));
}

/**
	Subscribe to GHUB devices list change notifications.
	This is done automatically when a new connection to GHUB is opened.
*/
function subscribeDevices() {
	send({
		verb: "SUBSCRIBE",
		path: "/devices/state/changed"
	});
}

/**
	Requests an battery status update from GHUB for the given GHUBDevice.
	@param d {GHUBDevice}
*/
function getDeviceBatteryState(d) {
	if (!d?.id)
		return;
	send({
		verb: "GET",
		path: `/battery/${d.id}/state`
	});
}

/**
	Subscribes to GHUB battery status update events for the given GHUBDevice.
	Fires only if already currently connected to GHUB.
	@param d {GHUBDevice}
*/
function subscribeDeviceBatteryState(d, sub = true) {
	if (!d?.id || !isConnected())
		return;
	send({
		verb: sub ? "SUBSCRIBE" : "UNSUBSCRIBE",
		path: `/battery/${d.id}/state/changed`
	});
}

/** Subscribes to general GHUB battery status update events (not for any particular device).
	Currently unused since per-device subscriptions seem to be enough. */
function subscribeBatteryState(sub = true) {
	send({
		verb: sub ? "SUBSCRIBE" : "UNSUBSCRIBE",
		path: `/battery/state/changed`
	});
}

// WebSocket event handlers

/** This handler runs when a WebSocket connection to GHUB is successfully opened. */
function onWsOpened()
{
	console.info(LOG_PREFIX, 'connected to G HUB.');
	_data.reconnectCount = 0;
	updateGHubStatus(2);
	listDevices();
	subscribeDevices();
	// subscribeBatteryState();
}

/** This handler runs when a WebSocket connection to GHUB is closed. */
function onWsClosed(event) {
	console.info(LOG_PREFIX, 'disconnected.');
	updateGHubStatus(0);
	// console.dir(event);
}

/** Handles WebSocket errors. */
function onWsError(event) {
	console.error(LOG_PREFIX, 'WebSocket Error:', event, "code:", event.code);
	// Possibly retry connection on some error types
	if (event.code == Socket.ConnectionRefusedError || event.code == Socket.RemoteHostClosedError)
		scheduleConnectionAttempt();
}

/** Main handler of all incoming WebSocket messages from GHUB. */
function onWsMessage(event)
{
	try {
		const msg = JSON.parse(event.data);
		if (!msg.verb) {
			console.warn(LOG_PREFIX, "Message is missing 'verb' property:", event.data);
			return;
		}
		if (msg.verb != 'BROADCAST' && msg.verb != 'GET') {
			console.info(LOG_PREFIX, "Skipping un-requested message type:", msg.verb);
			return;
		}
		if (msg.result && msg.result.code != "SUCCESS") {
			console.warn(LOG_PREFIX, "Got error response from G HUB:", event.data);
			return;
		}
		if (!msg.payload) {
			console.warn(LOG_PREFIX, "G HUB message is missing payload:", event.data);
			return;
		}
		// printf("%s Message received: %12o", LOG_PREFIX, msg);
		if (msg.path == '/devices/list')
			parseDeviceList(msg.payload.deviceInfos);
		else if (msg.path == '/devices/state/changed')
			parseDevice(msg.payload);
		else if (msg.path.match(/battery\/.*?\/?state/))
			parseBatteryState(msg.payload);
		else
			console.warn(`Received unknown message with path:`, msg.path);
	}
	catch (ex) {
		console.errorf("%s Error decoding message data: %s\nMessage data was:\n%2O", LOG_PREFIX, ex, event.data);
	}
};

//

/**
	Initialize required variables and objects, such as the WebSocket client used by this script.
	Runs only once after module is first loaded.
	Note that this does _not_ automatically open a connection to GHUB.
	The `open()` function must be called to do that (which will run `init()` first if needed).
*/
function init()
{
	if (_data.init)
		return;
	console.info(LOG_PREFIX, "initializing...");

	// Get the current script instance object (`DynamicScript` type);
	_data.instance = DSE.currentInstance();
	if (!_data.instance) {
		console.error(LOG_PREFIX, "Something went wrong, could not find current DynamicScript instance!");
		return;
	}
	_data.init = true;

	// Get saved settings from the stored persistent data object, if any
	const ds = _data.instance.dataStore;
	if (ds.Settings) {
		console.info(LOG_PREFIX, "Restoring saved settings");
		Object.assign(Settings, ds.Settings);
	}
	else {
		console.info(LOG_PREFIX, "No saved settings found, creating new data store");
		ds.Settings = structuredClone(Settings);
	}

	// Create a state to reflect current overall GHUB connection status.
	_data.ghubStatusStateId = `${STATES_BASE_ID}.ghub.active`;
	TP.stateCreate(_data.ghubStatusStateId, SCRIPT_NAME, "G HUB Connection Status", "0");

	// Connect to plugin quit event so we can mark all devices as inactive.
	DSE.aboutToQuit.connect(onAboutToQuit);

	// Create WebSocket instance for GHUB connection and connect our event listeners.
	// The "json" subprotocol and the origin value are required.
	_data.ws = new WebSocket(GHUB_WS_URL, "json", { origin: "file://" });
	// Connect event listeners.
	_data.ws.closed.connect(onWsClosed);
	_data.ws.error.connect(onWsError);
	_data.ws.message.connect(onWsMessage);
	_data.ws.opened.connect(onWsOpened);

	console.info(LOG_PREFIX, "initialization completed.");
}

// Runtime local data storage
var _data =
{
	init: false,                        // initialization flag, set to true after first time `init()` is called.
	instance: null,                     // the primary DynamicScript instance, set in init()
	ws: null,                           // WebSocket instance
	devices: new Map(),                 // tracked devices indexed by deviceId
	reconnectTimerId: null,             // timer used to schedule connection attempts
	reconnectCount: 0,                  // how many re-connection attempts have been made
	ghubStatusStateId: "",              // state ID for GHUB active/inactive status
	aboutToQuit: false,                 // flag indicating if plugin is about to exit, forces synchronous state updates
};


// Initialize on first load (or import, since this is a module).
if (!_data.init)
	init();
