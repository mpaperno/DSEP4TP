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
  include = ScriptEngine.include,
  btoa = Util.btoa,
  atob = Util.atob,
  locale = Qt ? Qt.locale : function() {},
  hash = Util.hash,
  DSE = {
    PLUGIN_VERSION_NUM: 0,
    PLUGIN_VERSION_STR: "",
    SCRIPTS_BASE_DIR: "",
    INSTANCE_TYPE: "Shared",
    INSTANCE_NAME: "",
    INSTANCE_DEFAULT_VALUE: "",
    VALUE_STATE_PREFIX: "dsep.",
    PLATFORM_OS: Qt ? Qt.platform.os : "unknown",
    TP_USER_DATA_PATH: "",
    instanceStateId: function() { return this.VALUE_STATE_PREFIX + this.INSTANCE_NAME; }
  },
  TP = {
    stateUpdate: ScriptEngine.stateUpdate,
    stateUpdateById: ScriptEngine.stateUpdateById,
    stateCreate: ScriptEngine.stateCreate,
    stateRemove: ScriptEngine.stateRemove,
    choiceUpdate: ScriptEngine.choiceUpdate,
    connectorUpdate: ScriptEngine.connectorUpdateShort,
    connectorUpdateShort: ScriptEngine.connectorUpdateShort,
    connectorUpdateByLongId: ScriptEngine.connectorUpdate,
    showNotification: ScriptEngine.showNotification,
    getConnectorRecords: ScriptEngine.getConnectorRecords,
    getConnectorShortIds: ScriptEngine.getConnectorShortIds,
    getConnectorByShortId: ScriptEngine.getConnectorByShortId,
    connectorIdsChanged: ScriptEngine.connectorIdsChanged,
    broadcastEvent: ScriptEngine.tpBroadcast,
    onconnectorIdsChanged: (r, t=null) => onEventHandler(ScriptEngine.connectorIdsChanged, r, t),
    onbroadcastEvent:      (r, t=null) => onEventHandler(ScriptEngine.tpBroadcast, r, t),
  }
;

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
