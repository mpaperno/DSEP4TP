/*
Dynamic Script Engine Plugin for Touch Portal
Copyright Maxim Paperno; all rights reserved.

This file may be used under the terms of the GNU
General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

A copy of the GNU General Public License is available at <http://www.gnu.org/licenses/>.

This project may also use 3rd-party Open Source software under the terms
of their respective licenses. The copyright notice above does not apply
to any 3rd-party components used within.
*/

/*!
	\class Env
	\ingroup Util
	The Env object provides access to the system environment variables.

	It has various modes of operation, depending on how it is created/used:

	- As `new Env()` creates a new instance with all current environment variables as properties.
		- Variables could then be accessed directly, for example:
			```js
			var env = new Env();
			console.log(env.PATH);
			```
		- You can check if a variable exists in the usual ways of JS object properties (check if !== undefined, `hasOwnProperty()`, etc).
		- You can iterate over the variables as key-value pairs using a `for...of` loop. For example:
			```js
			var env = new Env();
			for (const [name, value] of env)
				console.log(name, "=", value);
			```
			Or Java style:
			```js
			var it = new Env().iterator;
			while (it.hasNext()) {
				const [name, value] = it.next();
				console.log(name, "=", value);
			}
			```
		- The variable names can be retrieved with `names`, the values with `values` and the name-value pairs with `entries` properties (`entries` is an Iteratable).
		- `toString()` and `valueOf()` return a `JSON.stringify()` representation of the full variables list with an indentation of 2 spaces.

	- As `new Env(variableName)` or `new Env(variableName, defaultValue)` creates a new instance of a named variable, optionally with a default value.<br/>
		Eg. `new Env("PATH")` or `new Env("MYVAR", "Not defined yet")`.
		- `isSet` returns true/false based on if the variable exists in the environment;
		- `value` is the current value of the variable. If the variable doesn't exist in the current environment then `defaultValue` used in constructor is returned, or `undefined` if one wasn't set.
		- `name` is the variable name the instance was created with.
		- `isValid` indicates that a variable name was specified in the constructor and is not blank.
		- `set(value)` sets the value if the variable in the current environment (this will create it if it doesn't exist). Returns true/false to indicate success or failure.
			Note that setting a variable with an `undefined` value will remove it from the environment (same as `unset()`);
		- `unset()` removes the variable from the current environment. Returns true/false to indicate success or failure.
		- `toString()` and `valueOf()` return the variable value (same as `value()`).

	- Static methods:
		- `Env` or `Env.entries` - Iteratable of name-value pairs of all current environment variables, eg. use in `for ... of` loop (see example above).
		- `Env.iterator` - Java style iterator of name-value pairs of all current environment variables (see example above).
		- `Env.names` - Array of all current environment variable names.
		- `Env.values` - Array of all current environment variable values.
		- `Env.isSet(variableName)` - returns true/false based on if the `variableName` exists in the environment.
		- `Env.value(variableName, defaultValue)` - returns the value of `variableName` or `defaultValue` if variable doesn't exist in the environment.
		- `Env.set(variableName, value)` - sets the value of `variableName` in the current environment (this will also create it if it doesn't exist). Returns true/false to indicate success or failure.
			Note that setting a variable with an `undefined` value will remove it from the environment (same as `unset(variableName)`);
		- `Env.unset(variableName)` - removes `variableName` from the current environment. Returns true/false to indicate success or failure.

	\fn Env()
	\memberof Env
	Creates a new instance with all current environment variables as properties.

	\fn Env(varName, defaultValue)
	\memberof Env
	Creates a new instance of a named variable with a default value. The default is used if trying to get the value of this variable before it exists in the actual environment.

	\fn Env(varName)
	\memberof Env
	Creates a new instance of a named variable with an undefined default value.

*/

var Env = class Env
{
	constructor(varName, defaultValue)
	{
		Object.defineProperty(this, 'varName', { value: varName });
		Object.defineProperty(this, 'defVal', { value: defaultValue });

		if (varName === undefined || varName === null) {
			const obj = Object.entries(Util.env());
			for (const [k, v] of obj)
				this[k] = v;
		}
		else if (this.defVal === undefined || this.defVal == null) {
			this[varName] = String(Util.env(this.varName));
		}
		else {
			this[varName] = Util.env(this.varName, this.defVal);
		}
	}

	valueOf() {
		if (this.isValid) {
			if (!this.isSet)
				return undefined;
			if (this.defVal === undefined || this.defVal == null)
				return String(Util.env(this.varName));
			return Util.env(this.varName, this.defVal);
		}
		return JSON.stringify(Util.env(), null, 2);
	}

	toString() { return this.valueOf(); }

	get name()  { return this.varName; }
	get value() { return this.valueOf(); }
	get isValid() { return typeof this.varName !== "undefined" && this.varName; }
	get isSet() { return this.isValid && Util.envIsSet(this.varName); }

	set(value) { return this.isValid && Util.envPut(this.varName, value); }
	unset() { return this.isValid && Util.envUnset(this.varName); }

	entries() { return Object.entries(this); }
	names() { return Object.keys(this); }
	values() { return Object.values(this); }

	get iterator() {
		const items = Object.entries(this);
		var count = 0;
		var it = {
			hasNext: function () { return count < items.length; },
			next: function () { return this.hasNext() ? items[count++] : null; }
		};
		return it;
	}

	*[Symbol.iterator]() {
		const items = Object.entries(this);
		for (const i of items)
			yield i;
	}

	static isSet(variableName) { return Uril.isSet(variableName); }
	static value(variableName, defaultValue) { return Util.env(variableName, defaultValue); }
	static set(variableName, value) { return Util.envPut(variableName, value); }
	static unset(variableName) { return Util.envUnset(variableName); }

	static get entries() { return Object.entries(Util.env()); }
	static get names() { return Object.keys(Util.env()); }
	static get values() { return Object.values(Util.env()); }
	static get iterator() { return new Env().iterator; }

	static *[Symbol.iterator]() {
		const items = Object.entries(Util.env());
		for (const i of items)
			yield i;
	}

}

// var Env = new Proxy(Env, { apply(target, _thisArg, args) { return new target(...args); } });
