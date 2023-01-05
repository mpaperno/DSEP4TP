#!/usr/bin/env node

// Generates entry.tp JSON file.
// usage: node gen_entry.js [-v <plugin.version.numbers>] [-o <output/path or - for stdout>] [-d]
// A version number is required; By default it is read from the version.json file if it is present in this folder (generated by CMake from version.json.in).
// -d (dev/debug mode) will exclude the plugin_start commands in the TP file, for running the binary separately.
// Default output path depends on -d flag; if present assumes dist/Debug, otherwise dist/Release.

const fs = require('fs');
const path = require('path');
// import { fs } from 'fs';
// import { path } from 'path';

const buildInfo = fs.existsSync("./version.json") ? JSON.parse(fs.readFileSync("./version.json", "utf8")) : {};

// Defaults
var VERSION = buildInfo.VERSION_STR;
var VERSION_NUM = buildInfo.VERSION_NUM;
var OUTPUT_PATH = "";
var DEV_MODE = false;

// Const
const PLUGIN_ID = buildInfo.PLUGIN_ID;    
const SYSTEM_NAME = buildInfo.SYSTEM_NAME;
const SHORT_NAME = buildInfo.SHORT_NAME;  

// Handle CLI arguments
for (let i=2; i < process.argv.length; ++i) {
    const arg = process.argv[i];
    if      (arg == "-v") VERSION = process.argv[++i];
    else if (arg == "-o") OUTPUT_PATH = process.argv[++i];
    else if (arg == "-d") DEV_MODE = true;
}

// Validate the version
if (!VERSION) {
    console.error("No plugin version number, cannot continue :( \n Use -v <version.number> argument.");
    process.exit(1);
}

// Create integer version number from dotted notation in form of ((MAJ << 24) | (MIN << 16) | (PATCH << 8) | BUILD)
// Each version part is limited to the range of 0-99.
var iVersion = VERSION_NUM || 0;
if (!iVersion) {
    for (const part of VERSION.split('-', 1)[0].split('.', 4))
        iVersion = iVersion << 8 | (parseInt(part) & 0xFF);
}

// Set default output path if not specified, based on debug/release type.
if (!OUTPUT_PATH)
    OUTPUT_PATH = path.join(__dirname, "..", "dist", (DEV_MODE ? "Debug" : path.join("Release", buildInfo.PLATFORM_OS)));

// --------------------------------------
// Define the base entry.tp object here

const entry_base =
{
    sdk: 6,
    version: parseInt(iVersion.toString(16)),
    name: SHORT_NAME,
    id: PLUGIN_ID,
    plugin_start_cmd: DEV_MODE ? '' : 'sh %TP_PLUGIN_FOLDER%' + SYSTEM_NAME + '/start.sh',
    plugin_start_cmd_windows: DEV_MODE ? '' : '"%TP_PLUGIN_FOLDER%' + SYSTEM_NAME + '/bin/' + SYSTEM_NAME + '"',
    configuration: {
        colorDark: "#1D3345",
        colorLight: "#305676",
        parentCategory: "misc"
    },
    settings: [
        {
            name: "Script Files Base Directory",
            desc: "Paths to script/module files in actions will be relative to this directory, instead of the plugin's installation folder.",
            type: "text",
            default: "",
            readOnly: false
        }
    ],
    categories: [
        {
            id: PLUGIN_ID + ".cat.actions",
            name: SHORT_NAME,
            imagepath: '%TP_PLUGIN_FOLDER%' + SYSTEM_NAME + '/icon.png',
            states: [],
            actions: [],
            connectors: [],
            events: []
        },
        {
            id: PLUGIN_ID + ".cat.plugin",
            name: "Plugin",
            imagepath: '%TP_PLUGIN_FOLDER%' + SYSTEM_NAME + '/icon.png',
            states: [
                {
                    id: PLUGIN_ID + ".state.createdStatesList",
                    type: "text",
                    desc : SHORT_NAME + ": List of created named instances",
                    default : ""
                },
                {
                    id: PLUGIN_ID + ".state.lastError",
                    type: "text",
                    desc : SHORT_NAME + ": Last script instance error",
                    default : ""
                },
                {
                    id: PLUGIN_ID + ".state.errorCount",
                    type: "text",
                    desc : SHORT_NAME + ": Cumulative script error count",
                    default : ""
                }
            ],
            actions: [],
            connectors: [],
            events: []
        },
        {
            id: PLUGIN_ID + ".cat.values",
            name: "Dynamic Values",
            imagepath: '%TP_PLUGIN_FOLDER%' + SYSTEM_NAME + '/icon.png',
            states: [],
            actions: [],
            connectors: [],
            events: []
        }
    ]
};

// Default category to place actions and connectors into.
const category = entry_base.categories[0];


// Some space characters useful for forcing specific spacing in action formats/descriptions.
const SP = " ";   // non-breaking narrow space U+202F (TP ignores "no-break space" U+00AD)
const EN = " ";  // en quad space U+2000  (.5em wide)
const EM = " "; // em quad space U+2001  (1em wide)

// --------------------------------------
// Helper functions

// Replaces {N} placeholders with N value from args array/tuple.
String.prototype.format = function (args) {
    if (!Array.isArray(args))
        args = new Array(args);
    return this.replace(/{([0-9]+)}/g, function (match, index) {
        return typeof args[index] == 'undefined' ? match : args[index];
    });
};

// Functions for adding actions/connectors.

function addAction(id, name, descript, format, data, hold = false) {
    const action = {
        id: PLUGIN_ID + '.act.' + id,
        prefix: SHORT_NAME,
        name: name,
        type: "communicate",
        tryInline: true,
        description: descript,
        format: String(format).format(data.map(d => `{$${d.id}$}`)),
        hasHoldFunctionality: hold,
        data: data
    }
    category.actions.push(action);
}

// note that 'description' field is not actually used by TP as of v3.1
function addConnector(id, name, descript, format, data) {
    const action = {
        id: PLUGIN_ID + '.conn.' + id,
        name: name,
        description: descript,
        format: String(format).format(data.map(d => `{$${d.id}$}`)),
        data: data
    }
    category.connectors.push(action);
}

// Functions which create action/connector data members.

function makeActionData(id, type, label = "", deflt = "") {
    return {
        id: PLUGIN_ID + '.act.' + id,
        type: type,
        label:  label,
        default: deflt
    };
}

function makeTextData(id, label, dflt = "") {
    return makeActionData(id, "text", label, dflt + '');
}

function makeFileData(id, label, dflt = "") {
    return makeActionData(id, "file", label, dflt + '');
}

function makeChoiceData(id, label, choices, dflt) {
    const d = makeActionData(id, "choice", label, typeof dflt === "undefined" ? choices[0] : dflt);
    d.valueChoices = choices;
    return d;
}

function makeNumericData(id, label, dflt, min, max, allowDec = true) {
    const d = makeActionData(id, "number", label, dflt + '');
    d.allowDecimals = allowDec;
    d.minValue = min;
    d.maxValue = max;
    return d;
}

// Shared functions which create both a format string and data array.

function makeCommonData(id) {
    let format = "State\nName{0}";
    const data = [ makeTextData(id + ".name", "State Name") ];
    return [ format, data ];
}


function appendScopeData(id, data, defaultScope = "Shared") {
    let i = data.length;
    let format = ` Engine\nInstance{${i++}}`;
    data.push(
        makeChoiceData(id + ".scope", "Engine Instance", ["Shared", "Private"], defaultScope),
    );
    return format;
}


function appendSaveValueData(id, data) {
    let i = data.length;
    let format = ` | Create State\n| ${EM}at Startup{${i++}} Default\nValue/Expr{${i++}}`;
    data.push(
        makeChoiceData(id + ".save", "Create at Startup", ["No", "Fixed\nValue", "Custom\nExpression", "Use Action's\nExpression"]),
        makeTextData(id + ".default", "Default Value/Expression"),
    );
    return format;
}


// --------------------------------------
// Action creation functions

function addEvalAction(name) 
{
    const id = "script.eval";
    const descript = SHORT_NAME + ": Evaluate an Expression. Evaluation result, if any, is stored in the named State value.\n" + 
        "Any JS is valid here, from simple math to string formatting to short scripts. TP State and Value macros can be embedded.";
    let [format, data] = makeCommonData(id);
    let i = data.length;
    format += `Evaluate\nExpression{${i++}}`;
    data.push(
        makeTextData(id + ".expr", "Expression"),
    );
    format += appendScopeData(id, data);
    addConnector(id, name, descript, format, data);
    data = data.map(a => ({...a}));  // deep copy
    format += appendSaveValueData(id, data);
    addAction(id, name, descript, format, data);
}

function addScriptAction(name) 
{
    const id = "script.load";
    const descript = SHORT_NAME + ": Load and Run a Script. Evaluation result, if any, is stored in the named State value.\n" +
        "An optional expression can be appended to the file contents, for example to run a function with dynamic value arguments. The expression must follow JS syntax rules (quote all strings).";
    let [format, data] = makeCommonData(id);
    let i = data.length;
    format += `Script\n${EN}File{${i++}} Append\nExpression{${i++}}`;
    data.push(
        makeFileData(id + ".file", "Script File"),
        makeTextData(id + ".expr", "Append Expression", "run([arguments])"),
    );
    format += appendScopeData(id, data, "Private");
    format += appendSaveValueData(id, data);
    addAction(id, name, descript, format, data);
    // No connector for script types, too much I/O
}

function addModuleAction(name) 
{
    const id = "script.import";
    const descript = SHORT_NAME + ": Import a JavaScript Module. Modules can load other modules and are cached for improved performance.\n" +
        "Module properties are accessed via the alias (like in JS). An optional JavaScript expression can be evaluated after the import, eg. to run a function with dynamic value arguments.";
    let [format, data] = makeCommonData(id);
    let i = data.length;
    format += `import {*}\nfrom (file){${i++}} as\n(alias){${i++}} and evaluate\nexpression{${i++}}`;
    data.push(
        makeFileData(id + ".file", "Module File"),
        makeTextData(id + ".alias", "Module Alias", "M"),
        makeTextData(id + ".expr", "Expression", "M.run([arguments])"),
    );
    format += appendScopeData(id, data, "Private");
    addConnector(id, name, descript, format, data);
    data = data.map(a => ({...a}));  // deep copy
    format += appendSaveValueData(id, data);
    addAction(id, name, descript, format, data);
}

function addUpdateAction(name) 
{
    const id = "script.update";
    const descript = SHORT_NAME + ": Update an Existing Instance Expression.\n" + 
        "Use this action as a quick way to update an existing DSE instance with the same name. For example when using a script from several places it may be simpler to have the main definition only once.";
    let [format, data] = makeCommonData(id);
    let i = data.length;
    format += `Evaluate\nExpression{${i++}}`;
    data.push(
        makeTextData(id + ".expr", "Expression"),
    );
    addAction(id, name, descript, format, data);
    addConnector(id, name, descript, format, data);
}

function addSingleShotAction(name) 
{
    const id = "script.oneshot";
    const descript = SHORT_NAME + ": Run a One-Time/Anonymous Expression or Script/Module function.\n" + 
        "This action does not create a State. It is meant for running code which does not return any value. Otherwise can be used the same as \"Evaluate,\" \"Load\" and \"Module\" actions.";  // (module alias is always \"M\")
    let i = 0;
    let format = `Action\nType{${i++}} Evaluate\nExpression{${i++}} File\n(if req'd){${i++}} Module\nalias (if req'd){${i++}}`;
    // First the connector, which has fewer options than the action.
    let data = [
        makeChoiceData(id + ".type", "Script Type", ["Expression", "Module"]),
        makeTextData(id + ".expr", "Expression"),
        makeActionData(id + ".file", "file", "Script File", ""),
        makeTextData(id + ".alias", "Module Alias", "M"),
    ];
    addConnector(id, name, descript, format, data);
    // Then the action, with more options.
    data = data.map(a => ({...a}));  // deep copy
    data[0].valueChoices = ["Expression", "Script", "Module"];
    format += appendScopeData(id, data);
    addAction(id, name, descript, format, data);
}

// System utility action

function addSystemActions() 
{
    const id = "plugin.instance";
    addAction(id, "Plugin Actions", 
        SHORT_NAME + ": Plugin Actions. Choose an action to perform and which script instance(s) it should affect. \n" +
            "'Delete Instance' removes the corresponding State from TP and any associated Private Engine. `Set State Value` updates the TP State. 'Reset Engine' means setting the global script environment back to default.", 
        "Action: {0} Instance(s): {1} Value: {2} (if required)", 
        [
        makeChoiceData(id + ".action", "Action to Perform", ["Delete Instance", "Set State Value", "Reset Engine Environment"], ""),
        makeChoiceData(id + ".name", "Instance for Action", ["[ no instances created ]"], ""),
        makeTextData(id + ".value", "Set to Value"),
        ]);
}


// ------------------------
// Build the full entry.tp object for JSON dump

addEvalAction("Evaluate Expression");
addScriptAction("Load Script File");
addModuleAction("Import Module File");
addUpdateAction("Update Existing Instance");
addSingleShotAction("Anonymous (One-Time) Script");
// Misc actions
addSystemActions();


// Output

const output = JSON.stringify(entry_base, null, 4);
if (OUTPUT_PATH === '-') {
    console.log(output);
    process.exit(0);
}

const outfile = path.join(OUTPUT_PATH, "entry.tp");
fs.writeFileSync(outfile, output);
console.log("Wrote version", iVersion.toString(16), "output to file:", outfile);
if (DEV_MODE) {
    console.warn("!!!=== Generated DEV MODE entry.tp file ===!!!");
    process.exit(1);  // exit with error to prevent accidental usage with build scripts.
}
process.exit(0);
