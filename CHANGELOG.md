# Change Log
**Dynamic Script Engine Plugin for Touch Portal**: changes by version number and release date.

---
## 1.1.0.2 (20-Feb-2023)
- No changes since 1.1.0.1-beta1.
- Final version in the v1.1.x series.

---
## 1.1.0.1-beta1 (05-Jan-2023)
- Added Connector (Slider) counterparts for most of the plugin's existing Actions (except "Load Script").
- Added Connector/Slider instance tracking database for managing mapping and lookups of "short" connector IDs (for sending updates back to Touch Portal).
- Added new State to reflect current system user's Touch Portal settings directory path (workaround until TP v3.2 adds native values for that).
- Added new State: "Name of Page currently active on TP device."
- Touch Portal API:
  - Added methods and an event for Connector ID lookup/search/tracking, eg. for sending Slider position updates to Touch Portal.
  - Added Touch Portal "broadcast" message event passthrough (eg. to notify scripts when user navigates to a new page).
  - Added `TP.currentPageName()` lookup function.
- JavaScript Library:
  - Added `String.appendLine()` and `String.getLines()` functions (both static and prototype).
  - Added `DSE.TP_USER_DATA_PATH` property (current system user's Touch Portal settings directory), `DSE.TP_VERSION_CODE` and `DSE.TP_VERSION_STR`.
- Documentation & Examples:
  - Moved Touch Portal API references into their own category.
  - Added documentation for new Touch Portal API functions/features.
  - New v3 of Color Picker example page and script, using new "native" connector/slider functionality and adding some more features.
  - Other minor updates and clarifications.

Thanks for everyone's input so far!

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
