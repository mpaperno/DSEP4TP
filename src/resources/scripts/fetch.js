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

// Implementation of standard Headers type, see MDN for docs.
var Headers = class
{
	constructor(init)
	{
		var xhr = null;
		if (init instanceof XMLHttpRequest) {
			xhr = init;
		}
		else if (Array.isArray(init)) {
			init.forEach(function (header) {
				this.append(header[0], header[1])
			}, this)
		}
		else if (init) {
			for (const [k, v] of Object.entries(init))
				this.set(k, v);
		}
		Object.defineProperty(this, '_xhr', { value: xhr });
		Object.defineProperty(this, '_xhrParsed', { value: false, writable: true });
	}

	_normlName(name) {
		if (typeof name !== 'string') {
			name = String(name);
		}
		if (/[^a-z0-9\-#$%&'*+.^_`|~!]/i.test(name) || name === '') {
			throw new TypeError('Invalid character in header field name: "' + name + '"');
		}
		return name.toLowerCase();
	}

	_normlVal(value) {
		if (typeof value !== 'string') {
			value = String(value);
		}
		return value;
	}

	append(name, value) {
		name = this._normlName(name);
		if (this.hasOwnProperty(name))
			this[name] += ', ' + this._normlVal(value);
		else
			this[name] = this._normlVal(value);
	}

	delete(name) {
		if (!this._xhr)
			delete this[this._normlName(name)];
	}

	get(name) {
		name = this._normlName(name)
		return this._xhr ? this._xhr.getResponseHeader(name) : this.hasOwnProperty(name) ? this[name] : null;
	}

	has(name) {
		return this._xhr ? this._xhr.getResponseHeader(name) != null : this.hasOwnProperty(this._normlName(name));
	}

	set(name, value) {
		if (!this._xhr)
			this[this._normlName(name)] = this._normlVal(value);
	}

	_checkXhrParse() {
		if (!this._xhr || this._xhrParsed)
			return;
		if (this._xhr.headers) {
			this._xhr.headers.forEach((m) => this.append(m[0], m[1]));
		}
		else {
			var hdrs = [];
			this._xhr.getAllResponseHeaders().split(/^(.+?):/gm).forEach((m, i) => { if (i % 2) hdrs.push(m); });
			hdrs.forEach((m) => this.append(m, this._xhr.getResponseHeader(m)));
		}
		this._xhrParsed = true;
	}

	forEach(callback, thisArg) {
		for (const [k, v]  of this.entries())
			callback.call(thisArg, k, v, this);
	}

	keys() { this._checkXhrParse(); return Object.keys(this); }
	values() { this._checkXhrParse(); return Object.values(this); }
	entries() { this._checkXhrParse(); return Object.entries(this); }

	*[Symbol.iterator]() {
		const items = this.entries();
		for (const i of items)
			yield i;
	}
};

////////////////////////////////////

var Response = class
{
	constructor(xhr)
	{
		if (!xhr || !(xhr instanceof XMLHttpRequest))
			throw new TypeError("Response initializer must be an XMLHttpRequest object.");

		Object.defineProperty(this, '_xhr', { value: xhr });
		Object.defineProperty(this, '_headers', { value: new Headers(xhr) });
		Object.defineProperty(this, '_bodyUsed', { value: false, writable: true } );

		Object.defineProperty(this, 'async', {
      enumerable: true,
      get() { return this._xhr.async; }
    });
		Object.defineProperty(this, 'body', {
      enumerable: true,
      get() { return this.bodyAs(); }
    });
		Object.defineProperty(this, 'bodyUsed', {
			enumerable: true,
			get() { return this._bodyUsed; }
		});
		Object.defineProperty(this, 'headers', {
      enumerable: true,
      get() { return this._headers; }
    });
		Object.defineProperty(this, 'ok', {
      enumerable: true,
      get() { return ( this._xhr.status / 100 | 0) === 2; } // 200-299
    });
		Object.defineProperty(this, 'redirected', {
      enumerable: true,
      get() { return this._xhr.responseURL != this._xhr.url; }
    });
		Object.defineProperty(this, 'responseType', {
      enumerable: true,
      get() { return this._xhr.responseType; },
      set(v) { this._xhr.responseType = v; }
    });
		Object.defineProperty(this, 'status', {
      enumerable: true,
      get() { return this._xhr.status; }
    });
		Object.defineProperty(this, 'statusText', {
      enumerable: true,
      get() { return this._xhr.statusText; }
    });
		Object.defineProperty(this, 'url', {
      enumerable: true,
      get() { return this._xhr.responseURL; }
    });
		// the underlying XMLHttpRequest object
		Object.defineProperty(this, 'xhr', {
      enumerable: true,
      get() { return this._xhr; }
    });
	}

	clone() { return new Response(this._xhr); }

	bodyAs(type = "") {
		this._xhr.responseType = type;
		return this._xhr.response;
	}

	text() { return this.async ? this.textAsync() : this.textSync(); }
	textAsync() { return Promise.resolve(this._xhr.responseText); }
	textSync() { return this._xhr.responseText; }

	json() { return this.async ? this.jsonAsync() : this.jsonSync(); }
	jsonAsync() { return Promise.resolve(this.jsonSync()); }
	jsonSync() { return this.bodyAs('json'); }

	arrayBuffer() { return this.buffer(); }
	buffer() { return this.async ? this.bufferAsync() : this.bufferSync(); }
	bufferAsync() { return Promise.resolve(this.bufferSync()); }
	bufferSync() { return this.bodyAs('arraybuffer'); }

	base64() { return this.async ? this.base64Async() : this.base64Sync(); }
	base64Async() { return Promise.resolve(this.base64Sync()); }
	base64Sync() { return this.bufferSync().toBase64(); }

	xml() { return this.async ? this.xmlAsync() : this.xmlSync(); }
	xmlAsync() { return Promise.resolve(this.xmlSync()); }
	xmlSync() { return this.bodyAs('document'); }

	blob() { throw TypeError("Blob type not supported."); }
};

////////////////////////////////////

var Request = function(init, url = "")
{
	if (!(this instanceof Request))
  	return new Request(init, url);

	this.async = true;
	this.body = null;                // data for POST
	this.credentials = "include";    // omit, include
	this.headers = new Headers();    // headers to send to server
	this.method = "GET";
	this.noThrow = false;            // do not throw exceptions in synchronous mode (fetchSync())
	this.onprogress = null;          // progress callback, see XMLHttpRequest: progress event
	this.redirect = "no-less-safe";  // manual, no-less-safe, same-origin, error, follow
	this.rejectOnError = false;      // reject on non-2xx response
	this.responseType = "";          // arraybuffer, json, document, text
	this.signal = null;              // AbortSignal
	this.timeout = 30 * 1000;        // ms
	this.url = url;
	this.xhr = null;
	this.error = null;

	if (typeof init === 'string' || init instanceof URL)
		init = { url: init };
	return Object.assign(this, Request.GlobalDefaults, init); // {...this, ...init};
}

Request.GlobalDefaults = {};

Request.prototype._fetch = function()
{
	Request._fetchSetupXhr(this);
	return this.async ? Net.fetchAsync(this) : Net.fetchSync(this);
}

Request.prototype.get = function()
{
	this.method = "GET";
	return this._fetch();
}

Request.prototype.head = function()
{
	this.method = "HEAD";
	return this._fetch();
}

Request.prototype.post = function(data)
{
	this.method = "POST";
	this.body = data;
	return this._fetch();
}

Request.prototype.put = function(data)
{
	this.method = "PUT";
	this.body = data;
	return this._fetch();
}

Request._fetchSetupOptions = function(options, url)
{
	if (!(options instanceof Request))
		options = new Request(options, url);
	else if (url)
		options.url = url;
	return options;
}

Request._fetchSetupXhr = function(req)
{
	try {
		if (!req.url)
			throw new ReferenceError("A valid URL is required before any network operation.");
		if (!req.method)
			throw new ReferenceError("A valid HTTP method is required before any network operation.");

		if (!req.xhr)
			req.xhr = new XMLHttpRequest();
		else if (!(req.xhr instanceof XMLHttpRequest))
			throw new ReferenceError("A valid XMLHttpRequest object is required when provided in 'options' object.");
		if (req.xhr.readyState > XMLHttpRequest.OPENED)
			throw new ReferenceError("An XMLHttpRequest object cannot be reused after send() method.");
		if (req.xhr.readyState < XMLHttpRequest.OPENED)
			req.xhr.open(req.method, req.url, req.async);

		if (!(req.headers instanceof Headers))
			req.headers = new Headers(req.headers);
		for (const [k, v] of req.headers)
			req.xhr.setRequestHeader(k, v);
		req.xhr.timeout = req.timeout;
		req.xhr.responseType = req.responseType;
		req.xhr.withCredentials = req.credentials === 'include';
		if (req.xhr.hasOwnProperty('redirect'))
			req.xhr.redirect = req.redirect;
	}
	catch (e) {
		console.error(e);
		req.error = e;
	}
};

////////////////////////////////////

(function() {
	"use strict";

	function fetch(url, options = {})
	{
		options = Request._fetchSetupOptions(options, url);
		options.async = true;
		Request._fetchSetupXhr(options);
		return fetchAsync(options);
	}

	function request(url, options = {})
	{
		options = Request._fetchSetupOptions(options, url);
		options.async = false;
		return options;
	}

	////////////////////////////////////

	function fetchAsync(req)
	{
		return new Promise((resolve, reject) =>
		{
			if (!(req instanceof Request)) {
				reject(new ReferenceError("First argument to fetchAsync() must be a Request object."));
				return;
			}
			if (!(req.xhr instanceof XMLHttpRequest)) {
				reject(req.error || new ReferenceError("A valid XMLHttpRequest object is required in Request argument."));  // an Error
				return;
			}

			req.xhr.onload = () => {
				if (!req.rejectOnError || (req.xhr.status / 100 | 0) === 2)
					resolve(response());
				else
					reject(domError(`Server responded with status code ${req.xhr.status}`, "NetworkError", DOMException.NETWORK_ERR));
			};
			req.xhr.onerror   = () => { reject(domError("Request network error", "NetworkError", DOMException.NETWORK_ERR)); };
			req.xhr.ontimeout = () => { reject(domError("Request timed out", "TimeoutError", DOMException.TIMEOUT_ERR)); };
			req.xhr.onabort   = () => { reject(domError("Request aborted", "AbortError", DOMException.ABORT_ERR)); };

			if (typeof req.onprogress === 'function')
				req.xhr.onprogress = req.onprogress;

			if (req.signal && typeof req.signal.onabort === 'function')
				req.signal.abort.connect(req.xhr, req.xhr.abort);

			req.xhr.send(req.body);

			function response() { return new Response(req.xhr); }

			function domError(msg, type, code) {
				let err = new DOMException(msg, type, code);
				err.response = response();
				return err;
			}

		});
	}

	function fetchSync(req)
	{
		if (!(req instanceof Request)) {
			if (req.noThrow)
				return;
			throw new ReferenceError("First argument to fetchSync() must be a Request object.");
		}
		if (!(req.xhr instanceof XMLHttpRequest)) {
			if (req.noThrow)
				return;
			throw req.error || new ReferenceError("A valid XMLHttpRequest object is required in Request argument.");
		}

		let status = 0;
		req.xhr.onload = () => {
			if (!req.rejectOnError || (req.xhr.status / 100 | 0) === 2)
				status = 1;
		};
		req.xhr.ontimeout = () => { status = 2; };
		req.xhr.onabort   = () => { status = 3; };
		if (typeof req.onprogress === 'function')
			req.xhr.onprogress = req.onprogress;
		if (req.signal && typeof req.signal.onabort === 'function')
			req.signal.abort.connect(req.xhr, req.xhr.abort);

		req.xhr.send(req.body);
		if (status == 1 || req.noThrow)
			return response();
		if (status == 2)
			throw domError("Request timed out", "TimeoutError", DOMException.TIMEOUT_ERR);
		if (status == 3)
			throw domError("Request aborted", "AbortError", DOMException.ABORT_ERR);
		throw domError("Request network error", "NetworkError", DOMException.NETWORK_ERR);

		function response() { return new Response(req.xhr); }

		function domError(msg, type, code) {
			let err = new DOMException(msg, type, code);
			err.response = response();
			return err;
		}
	}

	Net = {
		fetch: fetch,
		request: request,
		fetchSync: fetchSync,
		fetchAsync: fetchAsync,
	};

	// legacy, remove
	GlobalRequestDefaults = Request.GlobalDefaults;

})();

////////////////////////////////////

var Net;
var GlobalRequestDefaults;  // legacy, remove
