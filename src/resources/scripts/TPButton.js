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

// documentation in ./tpbutton.dox

export default class TPButton
{
	constructor(name, init = {})
	{
		if (!checkString(name))
			throw new TypeError("A button name must be specified in the TPButton() constructor argument.");

		this.stateNamePrefix = init?.stateNamePrefix ?? "Button - ";
		this.parentGroup = init?.parentGroup ?? this.stateNamePrefix + name;

		this._name = name;
		this._stateId = `${DSE.VALUE_STATE_PREFIX}tpb.${name}`;  // base for all property state IDs
		this._states = new Map();

		// getter propertis aren't enumerable by default but this one should be
		Object.defineProperty(this, 'name',        { enumerable: true,  get() { return this._name; } });
		// these are for internal use and shouldn't be enumerable, but are when declared by assignment in c'tor
		Object.defineProperty(this, '_name',       { enumerable: false, writable: false });
		Object.defineProperty(this, '_stateId',    { enumerable: false, writable: false });
		Object.defineProperty(this, '_states',     { enumerable: false, writable: false });
		Object.defineProperty(this, '_ac',         { enumerable: false, writable: true,  value: null });
		Object.defineProperty(this, '_restoreCnt', { enumerable: false, writable: true,  value: 0    });
		Object.defineProperty(this, '_restoreTim', { enumerable: false, writable: true,  value: 0    });

		// Set up default properties
		this._addProperty( "bgColor",      "Background Color",                "color-argb" );
		this._addProperty( "bgColorStart", "Background Gradient Start Color", "color-rgba" );
		this._addProperty( "bgColorEnd",   "Background Gradient End Color",   "color-rgba" );
		this._addProperty( "bgOrient",     "Background Gradient Orientation"               );
		this._addProperty( "bgRounded",    "Background Rounded",              "boolean"    );
		this._addProperty( "font",         "Font Family"                                   );
		this._addProperty( "fontSize",     "Font Size",                       "number"     );
		this._addProperty( "icon",         "Icon",                            "image"      );
		this._addProperty( "iconFullSize", "Icon Full Size",                  "boolean"    );
		this._addProperty( "text",         "Text"                                          );
		this._addProperty( "textColor",    "Text Color",                      "color-argb" );
		this._addProperty( "halign",       "Text Horizontal Alignment",                    );
		this._addProperty( "valign",       "Text Vertical Alignment",                      );
		this._addProperty( "hoffset",      "Text Horizontal Offset",          "number"     );
		this._addProperty( "voffset",      "Text Vertical Offset",            "number"     );
		this._addProperty( "restore",      "Restore Button Visuals",                       );

		// mark 'restore' property as "private" (not for cloning)
		this._states.get('restore').flags = 1;

		// this method will be called from an event handler which may not preserve `this` scope
		this._startResetRestoreTimer = this._startResetRestoreTimer.bind(this);

		// Assign any property values passed in `init` argument. This creates & updates states.
		if (typeof init == 'object' && !!init)
			Object.assign(this, init);

		// this._debug();
	}

	clone(newName, options = {})
	{
		if (!checkString(newName) || newName == this._name)
			throw new TypeError(`TPButton("${this._name}"): A new button name is required to clone this one.`);

		const btn = new TPButton(newName, Object.assign({ stateNamePrefix: this.stateNamePrefix }, options));
		for (const v of this._states.values()) {
			// don't copy internal "private" properties
			if (v.flags & 1)
				continue;
			// add custom property if needed
			if (!(v.id in btn))
				btn._addProperty(v.id, v.name, v.type, v.defaultVal);
			// Image types may still be loading asynchronously, so set property using the original source string, if any
			if (v.type == 'image' && v.image_pending && !!v.image_src)
				btn._setImageProperty(v.id, v.image_src);
			// ARGB colors would be parsed incorrectly and anyway no reason to re-parse the value,
			// so just set the color property directly w/out going through _setColorValue()
			else if (v.type.startsWith("color"))
				btn._setProperty(v.id, v.value);
			// otherwise just use the property setter
			else
				btn[v.id] = v.value;
		}
		return btn;
	}

	addProperty(name, stateName, type, value)
	{
		if (!checkString(name))
			throw new TypeError(`TPButton("${this._name}"): A property name is required to create a property.`);
		if (!checkString(stateName))
			throw new TypeError(`TPButton("${this._name}"): A state name is required to create a property.`);

		if (this._states.has(name))
			this._removeProperty(name);

		this._addProperty(name, stateName, type, value);
	}

	forceUpdate()
	{
		let i = 0;
		for (const p of this._states.values()) {
			if (!p.created || (p.flags & 1))
				continue;

			if (p.value !== p.defaultVal && !p.image_pending)
				setTimeout([this._delayedUpdate, this], ++i, p.id, "", p.value+"");
			else
				setTimeout([this._updateState, this], ++i, p.id, "");
		}
	}

	removeStates()
	{
		for (const p of this._states.values()) {
			if (p.created)
				TP.stateRemove(`${this._stateId}.${p.id}`);
			p.created = false;
		}
	}

	resetToDefaults()
	{
		for (const p of this._states.values()) {
			if (!p.created)
				continue;
			p.value = p.defaultVal;
			if (p.type == 'image') {
				p.image_src = "";
				if (p.image_pending && this._ac)
					this._ac.abort();
			}
			this._updateState(p.id, "");
		}
	}

	restoreBackgroundVisuals() { this.restoreBgVisuals(); }
	restoreBgVisuals()   { this._restoreVisuals('bg'); }
	restoreIconVisuals() { this._restoreVisuals('icon'); }
	restoreTextVisuals() { this._restoreVisuals('text'); }


	_createPropertyState(p) {
		// console.debug(`TPButton("${this._name}"): `, "Creating state", p.id, p.name);
		p.created = true;
		TP.stateCreate(`${this._stateId}.${p.id}`, this.parentGroup, `${this.stateNamePrefix}${this._name} ${p.name}`, "", true, 2);
	}

	_updateState(id, value) {
		// console.debug(`TPButton("${this._name}"): `, "Sending", id, String.elideMiddle(value, 75));
		TP.stateUpdateById(`${this._stateId}.${id}`, value);
	}

	_delayedUpdate(id, value, nextValue) {
		this._updateState(id, value);
		setTimeout([this._updateState, this], 1, id, nextValue);
	}

	_restoreVisuals(v)
	{
		// spread out the 'restore' state updates so TP can handle it
		if (!this._restoreCnt)
			this._setProperty('restore', v);
		else
			setTimeout([this._setProperty, this], this._restoreCnt, 'restore', v);
		++this._restoreCnt;
		// need to eventually reset the `restore` value so it can be triggered again for the same visuals type
		// callLater() is a global which consolidates multiple concurrent calls to the same function into one call once control returns to the main event loop
		callLater(this._startResetRestoreTimer);
	}

	_startResetRestoreTimer() {
		if (!!this._restoreTim)
			clearTimeout(this._restoreTim);
		this._restoreTim = setTimeout([this._resetRestore, this], 10);
	}

	_resetRestore() {
		this._restoreCnt = 0;
		this._restoreTim = 0;
		this._setProperty('restore', "");
	}

	_addProperty(id, stateName, type, value)
	{
		const p = newStateObject(id, stateName, type, value);
		this._states.set(id, p);

		const d = {
			enumerable: true,
			get() { return this._states.get(id)?.value; }
		}
		if (p.type.startsWith("color"))
			d.set = function(v) { this._setColorProperty(id, v); }
		else if (p.type == "image")
			d.set = function(v) { this._setImageProperty(id, v); }
		else
			d.set = function(v) { this._setProperty(id, v); }
		Object.defineProperty(this, p.id, d);
	}

	_removeProperty(id)
	{
		const p = this._states.get(id);
		if (!p)
			return;
		if (p.created)
			TP.stateRemove(`${this._stateId}.${p.id}`);
		this._states.delete(id);
		delete this[id];
	}

	_setProperty(id, value)
	{
		const p = this._states.get(id);
		if (!p) {
			console.error(`TPButton("${this._name}"): Can't update unknown property ID '${id}`);
			return;
		}

		// reset image pending flag even if we're not going to update
		if (p.image_pending)
			p.image_pending = false;

		if (p.value === value)
			return;   // no update needed

		if (!p.created)
			this._createPropertyState(p);

		p.value = value;
		if (value === p.defaultVal)
			this._updateState(p.id, "");  // reset
		else
			this._updateState(p.id, value+"");
	}

	_setColorProperty(id, value)
	{
		if (!value) {
			this._setProperty(id, "");
			return;
		}
		const argb = this._states.get(id)?.type.endsWith("argb"),
			colorStr = formatColor(value, argb);
		if (!colorStr)
			throw TypeError(`TPButton("${this._name}"): The input value '${value}' cannot be parsed as a valid color.`);
		this._setProperty(id, colorStr);
	}

	_setImageProperty(id, v)
	{
		const p = this._states.get(id);
		if (!p)
			return;   // unlikely

		// cancel any currently pending async image request
		if (p.image_pending) {
			this._ac?.abort();
			p.image_pending = false;
		}

		// Test to see if this is a URL or a file name (ending in .ext), otherwise assume b64 string.
		if (!(/^\w{2,6}:\/\//.test(v)) && !(/\.\w{2,5}$/.test(v))) {
			this._setProperty(id, v);
			return;
		}

		// Check if this property was already set using this same original source value, before resolving or loading image.
		if (p.image_src === v)
			return;
		p.image_src = v;

		// Make a URL from the input value, which could be an absolute or relative file system path, or an actual URL already.
		// Relative paths are resolved against the currently running script file's directory, or (but unlikely) the configured scripts base folder.
		const url = Util.urlFromInput(v, File.path(DSE.currentInstance()?.scriptFileResolved) || DSE.SCRIPTS_BASE_DIR, 0);
		if (!Util.urlIsValid(url))
			throw new TypeError(`TPButton("${this._name}"): The given image URL '${v}' is invalid or could not be resolved. If it is a local file, check that the file exists.`);

		// Set up a way to abort the async fetch request and flag this property as pending.
		if (!this._ac)
			this._ac = new AbortController();
		p.image_pending = true;

		// Fetch the image and set the actual image property value using the result encoded as b64 string.
		Net.fetch(url, { signal: this._ac.signal })
		.then(r => r.base64())
		.then(b64 => this._setProperty(id, b64))
		.catch((ex) => {
			p.image_pending = false;
			if (!(ex instanceof AbortError))
				console.error(`TPButton("${this._name}"): Exception while trying to load image from URL "${url}" :`, ex);
		});
	}

	debug()
	{
		const quoted = (s) => { const q = (typeof(s) == 'string' ? '"' : ''); return `${q}${s}${q}`; };
		let m = `\nTPButton("${this._name}"); Properties: {\n`;
		for (const p of this._states.values()) {
			m += sprintf(
				'  { id: %-14s name: %-35s type: %-12s default: %-6s value: %s',
				`${p.id},`, `"${p.name}",`, `${p.type},`, `${quoted(p.defaultVal)},`, `${quoted(p.value)}`.elideMiddle(75)
			);
			if (!!p.image_src)
				m += `,\n      image_src: "${p.image_src}", image_pending: ${p.image_pending},\n `;
			m += " }\n";
		}
		m += "}";
		console.debug(m);
	}

}

function newStateObject(id, name, type, value)
{
	type = type || 'string';
	if (value === undefined)
		value = defaultForType(type);
	return {
		id,
		name,
		type,
		value,
		defaultVal: value,
		created: false,
		flags: 0,  // 1 = don't clone this property
		image_src: "",
		image_pending: false,
	};
}

function defaultForType(type) {
	switch (type) {
		case 'number':
			return -1;
		case 'boolean':
			return null;
		default:
			return "";
	}
}

function formatColor(v, argbFormat = true)
{
	v = new Color(v);  // already checks if (v instanceof Color)
	if (!v.isValid)
		return "";

	if (argbFormat)
		return v.argb();
	return v.web();
}

function checkString(s) {
	return typeof(s) == 'string' && s.trim().length > 0;
}
