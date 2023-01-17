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
  btoa = Util.btoa,
  atob = Util.atob,
  hash = Util.hash,
  include = Util.include,
  require = Util.require,
  locale = Qt ? Qt.locale : function() {},
  TP = TPAPI
;

TPAPI.onconnectorIdsChanged = function(r, t=null) { onEventHandler(TPAPI.connectorIdsChanged, r, t); }
TPAPI.onbroadcastEvent = function(r, t=null)  { onEventHandler(TPAPI.broadcastEvent, r, t); }

function onEventHandler(sender, receiver, thisObj)
{
  if (thisObj && typeof receiver === 'function')
    sender.connect(thisObj, receiver);
  else
    sender.connect(receiver); 
}

Object.defineProperty(AbortController, Symbol.hasInstance, {
  configurable: true,
  value(instance) {
    return instance.objectName === "AbortSignal";
  },
});

Object.defineProperty(AbortSignal, Symbol.hasInstance, {
  configurable: true,
  value(instance) {
    return instance.objectName === "AbortSignal";
  },
});

Object.defineProperty(FileHandle, Symbol.hasInstance, {
  configurable: true,
  value(instance) {
    return instance.objectName === "FileHandle";
  },
});

Object.defineProperty(Process, Symbol.hasInstance, {
  configurable: true,
  value(instance) {
    return instance.objectName === "Process";
  },
});


Object.defineProperty(URL, Symbol.hasInstance, {
  configurable: true,
  value(instance) {
    return Object.prototype.toString.call(instance).endsWith("URL]");
  },
});
