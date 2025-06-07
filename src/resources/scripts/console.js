
if (!globalThis.inspect) {
  globalThis.inspect = function(obj, { depth = 6, showHidden = true, showType = false } = {}) {
    // return JSON.stringify(obj, null, 2);
    function dir(obj, currentDepth = 0) {
      if (!obj || typeof obj != 'object' || ArrayBuffer.isView(obj) || obj instanceof ArrayBuffer) {
        if (typeof obj == 'string')
          return `"${obj}"`;
        else if (ArrayBuffer.isView(obj) || obj instanceof ArrayBuffer)
          return `{${obj.toString()}}`;
        return ''+obj;
      }
      const sp = "  ".repeat(currentDepth + 1);
      if (currentDepth > depth) {
        return ''+obj;
      }
      const delim = Array.isArray(obj) ? ['[',']'] : ['{','}'];
      const list = showHidden ? Object.getOwnPropertyNames(obj) : Object.keys(obj);
      if (showHidden)
        list.push(...Object.getOwnPropertySymbols(obj));
      let msg = "";
      for (const key of list) {
        const value = obj[key],
          isBytes = ArrayBuffer.isView(value) || value instanceof ArrayBuffer;
        let type = "";
        if (showType)
          type = " {" + (Array.isArray(value) ? "Array" : isBytes ? "ArrayBuffer" : typeof value) + "}";
        msg += `\n${sp}${String(key)}${type}: `;
        if (currentDepth < depth && !!value && !isBytes && typeof value === 'object' && value != obj && value != obj.prototype && value != value.prototype) {
          msg += dir(value, currentDepth + 1);
        }
        else {
          if (typeof value == 'string')
            msg += `"${value}"`;
          else if (isBytes)
            msg += `{${value.toString()}}`;
          else
            msg += `${value}`;
        }
      }
      if (msg)
        msg += "\n" + "  ".repeat(currentDepth);
      return (!currentDepth ? obj + " " : "") + delim[0] + msg + delim[1];
    }
    return dir(obj);
  };
}

if (!console.dir) {
  console.dir = function(obj, options = {}) {
    console.debug(inspect(obj, options));
  };
}

if (!console.printf) {
  console.printf = console.logf = console.debugf = function(format, ...args) {
    console.log(sprintf(format, ...args));
  };
  console.infof = function(format, ...args) {
    console.info(sprintf(format, ...args));
  };
  console.warnf = function(format, ...args) {
    console.warn(sprintf(format, ...args));
  };
  console.errorf = function(format, ...args) {
    console.error(sprintf(format, ...args));
  };
}

if (!globalThis.printf) {
  globalThis.printf = console.printf;
}
