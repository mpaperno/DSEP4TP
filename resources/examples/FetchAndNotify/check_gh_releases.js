
// Function to check a GitHub repository's releases for the latest version tag, compare that to a current version, log the result
// and optionally send a Touch Portal notification if a new version is found. 
// This can also process a user click in the notification to open the download link for a new version in the default web browser.
// 
// - `user_repo` is a GitHub URL path in the form of 'user_name/repository_name'.
// - `current_version` is the current version string to check against, in "semantic versioning" format, eg. "1.22.3-beta2".
// - If `show_notification` is true, a Touch Portal Notification will be sent if a newer version is detected.
// 
// In any case it will log what happened to 'console.log()' which will show up in the plugin's console.log.
// 
// Example usage, check for updates of this very DSEP4TP plugin.  
// This uses the current plugin version number constant from the global environment (DSE.PLUGIN_VERSION_STR) which is something like "1.1.0.4".
// checkReleaseVersion("mpaperno/DSEP4TP", DSE.PLUGIN_VERSION_STR, true);

var checkReleaseVersion = function(user_repo, current_version, show_notification = false)
{
    // URL to get JSON data about a repository's latest Release.
    const url = "https://api.github.com/repos/" + user_repo + "/releases/latest";
    // Initiate the request here with fetch(). It returns a "promise". Here we set the 'rejectOnError' option to 'true' so anything
    // besides a successful HTTP result (200-299) will simply result in an error, instead of having to check the response in a
    // separate step.
    Net.fetch(url, { rejectOnError: true })
    // When the response is received, we want to parse it as JSON into a JavaScript object (`JSON.parse()`) and then work 
    // on the result. `response.json()` returns another promise which resolves with the actual de-serialized JS object.
    .then(response => response.json())
    // At the next step we get a 'result' which is the JSON from the response body parsed into a JavaScript Object (just like from JSON.parse()).
    .then((result) => {
        // Now we can actually work with the data. First check that a 'tag_name' field is even present, otherwise something didn't go right.
        if (result.tag_name !== undefined) {
            // make numeric version number values from both the passed-in current version and the new tag version.
            const currVersion = versionStringToNumber(current_version);
            const newVersion = versionStringToNumber(result.tag_name);
            const hasNewVersion = newVersion > currVersion;

            // Prepare the message, this will go to the console log and optionally as a TP Notification.
            let logText = `The latest release date is ${result.published_at};\n`;
            logText += `The latest tag is ${result.tag_name}; Current version: ${current_version};\n`;
            logText += `Latest release version ${newVersion.toString(16)}; Current version: ${currVersion.toString(16)}\n\n`;
            
            if (hasNewVersion)
                logText += "The latest released version is newer!\n" + 
                           `Click below or go to\n<${result.html_url}>\nfor downloads.`;
            else if (newVersion == currVersion)
                logText += "The latest release version is same as current.";
            else
                logText += "Congratulations, you're running a development version! (-8";

            // log it
            console.log('Version Check Report:\n' + logText);

            // Send the notification message if the option was enabled and a new version detected.
            if (show_notification && hasNewVersion) 
            {
                // We are going to put a "Download" link in the notification message. If the user clicks that link, we can get a callback
                // about that event, and then do something useful based on what the user chose. There could be multiple choices, but we just have one.
                // The choices are defined as an array of objects with `id` and `title` keys. The `id` is sent back to our callback handler
                // while the `title` is what the user sees and can click on. 
                let options = [
                    {
                        id: 'goto', 
                        title: "Go To Download" 
                    }
                ];

                // This function will handle the callback.
                // The callback is given the ID of the notification and of the option which was clicked.
                let callback = function(choiceId, notificationId) { 
                    console.log(`The option ID '${choiceId}' was selected for notification ID '${notificationId}'.`);
                    // This should open the default browser with the download URL. The command depends on the operating system.
                    if (DSE.PLATFORM_OS === "windows")
                        new Process().startCommand(`explorer "${result.html_url}"`);  // also possibly 'start firefox' or 'start chrome'
                    else if (DSE.PLATFORM_OS === "osx")
                        new Process().startCommand(`open "${result.html_url}"`);
                    else if (DSE.PLATFORM_OS === "linux")
                        new Process().startCommand(`xdg-open "${result.html_url}"`);  // or possibly 'sensible-browser' on Ubuntu
                }

                TP.showNotification(
                    result.tag_name,          // notification ID, should be unique per event so version tag makes sense here.
                    "New Version Available",  // notification title
                    logText,                  // main message body
                    options,                  // array of choices which will show in the notification
                    callback                  // callback for reacting to option choice clicks
                );
            }

            // Optionally dump the whole JSON document to console log to see what's in it.
            //console.log(JSON.stringify(result, null, 2));
        }
        else {
            // Something went wrong with the request.
            console.warn("Could not find tag name in returned data :-( ");
            // we could show the data or do some other diagnostic...
            // console.log(JSON.stringify(result, null, 2));
        }
    })
    // Catch is where any errors will end up, from either the network request itself (eg. couldn't connect), 
    // or potentially from parsing the JSON data.  Here we'll just log the error to keep it simple. 
    // Check the `Net.fetch()` documentation for possible error types.
    .catch((e) => {
        console.error(e);
    });
}

// Helper function to create integer version number from dotted notation string by shifting and ORing the numeric parts.
// eg. "1.20.3.44" -> ((1 << 24) | (20 << 16) | (3 << 8) | 44)
// Each version part is limited to the range of 0-99. Up to 4 parts can be used (or fewer). 
// Anything besides the dotted numerals should be ignored (eg. "v" prefix or "-beta1" suffix).
function versionStringToNumber(version) 
{
    var iVersion = 0;
    version = version.replace(/([\d\.]+)/, '$1');
    for (const part of version.split('.', 4))
        iVersion = iVersion << 8 | ((parseInt(part) || 0) & 0xFF);
    return iVersion;
}
