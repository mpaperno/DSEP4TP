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

var
  setTimeout = Util.setTimeout,
  clearTimeout = Util.clearTimeout,
  setInterval = Util.setInterval,
  clearInterval = Util.clearInterval,
  clearAllTimers = Util.clearAllTimers,
  clearInstanceTimers = Util.clearInstanceTimers,
  callLater = Qt.callLater,
  gcLater = Util.gcLater,
  btoa = Util.btoa,
  atob = Util.atob,
  hash = Util.hash,
  include = Util.include,
  require = Util.require,
  openUrlExternally = Util.openUrl,
  locale = Qt ? Qt.locale : function() {},
  TP = TPAPI,
  registeredModules = {
    "clipboard": undefined
  }
;

// Internal QObject signals to script slots event handling helpers. These are invoked from C++ side in event_utils.h.

function _getNamedEventSignal(obj, ev) {
  const f = obj[ev];
  if (typeof f != 'function')
    throw new TypeError(`Unknown event name: '${ev}' for object ${obj.objectName}.`);
  return f;
}

function _namedEventOnHandler(obj, ev, r, t) {
  return _onEventHandler(_getNamedEventSignal(obj, ev), r, t);
}

function _namedEventOnceHandler(obj, ev, r, t) {
  return _onceEventHandler(_getNamedEventSignal(obj, ev), r, t);
}

function _namedEventOffHandler(obj, ev, r, t) {
  _offEventHandler(_getNamedEventSignal(obj, ev), r, t);
}

function _connectSignalHandler(obj, ev, r, t) {
  _connectEventHandler(_getNamedEventSignal(obj, ev), r, t);
}

function _connectEventHandler(sender, receiver, thisObj)
{
  if (!!thisObj && typeof receiver === 'function')
    sender.connect(thisObj, receiver);
  else
    sender.connect(receiver);
}

function _onEventHandler(sender, receiver, thisObj)
{
  _connectEventHandler(sender, receiver, thisObj);
  var ref = new WeakSet([sender, receiver]);
  return function() {
    if (ref.delete(sender) && ref.delete(receiver))
      _offEventHandler(sender, receiver, thisObj);
    ref = undefined;
  };
}

function _onceEventHandler(sender, receiver, thisObj)
{
  var ref = new WeakSet([sender, receiver]);
  return _onEventHandler(sender, function handler() {
    if (ref.delete(sender) && ref.delete(receiver)) {
      receiver.apply(thisObj, arguments);
    }
    _offEventHandler(sender, handler);
    ref = undefined;
  });
}

function _offEventHandler(sender, receiver, thisObj)
{
  if (!!thisObj && typeof receiver === 'function')
    sender?.disconnect(thisObj, receiver);
  else
    sender?.disconnect(receiver);
}


// Public event connection global functions.

function connectEvent(sender, receiver, thisObj = null)
{
  if (typeof sender != 'function' || !receiver)
    throw new TypeError("sender and receiver must be valid function objects.");
  return _onEventHandler(sender, receiver, thisObj);
}

function connectOnce(sender, receiver, thisObj = null)
{
  if (typeof sender != 'function' || !receiver)
    throw new TypeError("sender and receiver must be valid function objects.");
  return _onceEventHandler(sender, receiver, thisObj);
}

function disconnectEvent(sender, receiver, thisObj = null)
{
  if (typeof sender != 'function' || !receiver)
    throw new TypeError("sender and receiver must be valid function objects.");
  _offEventHandler(sender, receiver, thisObj)
}


// Helper function to determine if an object instance is a URL type
// Net.Request uses this. Workaround for running in a QQmlEngine instance.
function isInstanceOfURL(instance) {
  return Object.prototype.toString.call(instance)?.endsWith("URL]");
}

// Global init script fired at end of engine init.

function _global_init()
{
  const addHasInstance = (obj, name) => {
    Object.defineProperty(obj, Symbol.hasInstance, {
      configurable: true,
      value(instance) {
        return instance.objectName?.startsWith(name);
      },
    });
  };

  [
    [ AbortController, "AbortController" ],
    [ AbortSignal,     "AbortSignal"     ],
    [ DynamicScript,   "DynamicScript"   ],
    [ FileHandle,      "FileHandle"      ],
    [ Process,         "Process"         ],
    [ WebSocket,       "WebSocket"       ],
  ].forEach((o) => addHasInstance(o[0], o[1]))

  try {
    // This will throw if we're running in a QQmlEngine instead of a QJSEngine
    Object.defineProperty(URL, Symbol.hasInstance, {
      configurable: true,
      value(instance) {
        return isInstanceOfURL(instance);
      },
    });
  }
  catch (_) { }

}
