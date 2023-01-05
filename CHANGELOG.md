# Change Log
**Dynamic Script Engine Plugin for Touch Portal**: changes by version number and release date.

---
## 1.0.0.3 (05-Jan-2023)
- Fix evaluation at startup of default value expressions for saved states when the default type was set to "use action's expression" and the default value field was left blank.
- Fix "Anonymous (One-Shot)" type script instances not being properly deleted after finishing.
- Touch Portal API:
  - Set default showNotification() option name to a minimal required empty space character instead of "Dismiss" prompt which doesn't actually do anything anyway.

---
## 1.0.0.2-beta2 (28-Dec-2022)
- Fix crash on engine environment reset action.
- Fix missing `Number.prototype.round()` function.
- Add `TP.connectorUpdateShort()` method.
- Add Clipboard and Color Picker examples.

---
## 1.0.0.1-beta1 (26-Dec-2022)
- Initial release!
