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
var marquee = function(text, maxLength = 32, scrollDelay = 200, startDelay = 0)
{
  marqueeStop();

  if (text.length <= maxLength)
    return text;

  // track the first character of the text to show in the scrolling marquee
  var start = 0;
  var firstCycle = true;
  text += " - " + "\u2000".repeat(maxLength);

  function update()
  {
    if (!_data.runMarquee)
        return;

    const result = text.slice(start, start + maxLength);

    TP.stateUpdate(result);

    if (start + maxLength >= text.length) {
      start = 0;  // Restart the counter at first character.
      if (firstCycle) {
        text = "\u2000".repeat(maxLength) + text;   //  <<<  EDIT
        firstCycle = false;
      }
    }
    else {
      ++start;  // Increment the starting character position.
    }

    _data.timerId = setTimeout(update, start > 1 ? scrollDelay : startDelay);
  }

  _data.runMarquee = true;

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
