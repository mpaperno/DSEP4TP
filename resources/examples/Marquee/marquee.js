/*
This script is designed to run in a Private engine instance
and will update the State Name of whichever action invoked the `marquee()` function.

`marquee()` function parameters:
 `text` - The full text to show.
 `maxLength` - The maximum number of characters to show at one time.
               If `text` length is <= `maxLength` then the whole text will be shown at once, with no scrolling.
 `scrollDelay` - This is the scroll delay in milliseconds; it dictates how quickly the text scrolls.
 `startDelay` - How many milliseconds to wait before (re)starting the scroll. This can create an extra pause at the start and at the end of the text.
 `restartAtEnd` - Whether to restart the scroll animation after all the text has been shown once.
*/
var marquee = function(text, maxLength = 32, scrollDelay = 200, startDelay = 1000, restartAtEnd = true)
{
	// Make sure any currently running marquee animation is stopped.
	marqueeStop();

	// If text fits into maximum length then just return the text as-is
	// (this automatically sends the correct State update to Touch Portal).
	if (text.length <= maxLength)
		return text;

	// track the first character of the text to show in the scrolling marquee
	var start = 0;

	// This function sends the State update to TP with a portion of the full text.
	function update()
	{
		// Bail out if animation stop has been requested.
		if (!_data.runMarquee)
			return;

		// Get just the portion of the text to send this time.
		const result = text.slice(start, start + maxLength);
		// And send it.
		TP.stateUpdate(result);

		// Check if end of text is visible
		if (start + maxLength >= text.length)
			start = 0;  // Restart the counter at first character.
		else
			++start;  // Increment the starting character position.

		// if we're at the end of the text and restart wasn't requested then exit now.
		if (!start && !restartAtEnd)
			return;

		// Schedule a timer for next scroll animation "frame."
		// If we're at the start or end of the text then use the startDelay, otherwise the scrollDelay.
		_data.timerId = setTimeout(update, start > 1 ? scrollDelay : startDelay);
	}

	// Set the "run" flag (the animation can be cancelled with `marqueeStop()`)
	_data.runMarquee = true;
	// Send the first part of the text now.
	update();
}

// This function can be used to stop/cancel the scrolling animation at any point.
// It is also called by `marquee()` to ensure any current animation is stopped before
// starting a new one.
var marqueeStop = function() {
	// Set flag for `update()` function
	_data.runMarquee = false;
	// Cancel any running timer
	if (_data.timerId > -1)
		clearTimeout(_data.timerId);
	// reset timer ID
	_data.timerId = -1;
}

// These variables are used internally to stop/cancel the marquee scroll.
var _data = _data || {
	timerId: -1,
	runMarquee: false
}
