# Basic Usage Examples {#example_basic}
A collection of simple examples of using math, formatting, and other basic functions.

<div class="hide-on-site">

**See the [published documentation](https://mpaperno.github.io/DSEP4TP/example_basic.html) for a properly formatted version of this README.**
</div>

These examples use States coming from another plugin (for Microsoft Flight Simulator), but any State or Value could be substituted in their place.<br />
The dynamic values (State names) are all in Touch Portal's usual `${value:value_name}` format as they appear when selected with the `+` button.

### Round a value to a whole number
Uses [Number.toFixed()](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Number/toFixed) standard JavaScript function.

<a href="ex_basic_round.png" target="image" title="Click for full version in new window."><img src="ex_basic_round.png" /></a>

### Convert feet to meters
Simple math operation on any numeric value (there are 0.3048 meters to one foot).

<a href="ex_basic_convert_value.png" target="image" title="Click for full version in new window."><img src="ex_basic_convert_value.png" /></a>

### Format a string to include a numeric value
This example results in "Ground Speed: 12 mph" based on current ground speed in knots (there are 1.150779448 knots to one mile). It uses the [Format](@ref Format) function.

<a href="ex_basic_format_number.png" target="image" title="Click for full version in new window."><img src="ex_basic_format_number.png" /></a>

### Inline conditional (ternary operator) results
A [ternary operator](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/Conditional_Operator) means,
in English, "if condition is true then do A, otherwise do B". The corresponding syntax is `condition ? A : B`<br />
This example sets the 'MSFS_StallWarning' state to "STALL" when the warning is active and to an empty string when the warning is not.

<a href="ex_basic_ternary_string.png" target="image" title="Click for full version in new window."><img src="ex_basic_ternary_string.png" /></a>
