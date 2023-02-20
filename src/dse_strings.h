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

#pragma once

#include <QByteArray>
#include <QHash>

namespace Strings {

enum StateIdToken
{
	SID_CreatedInstanceList,
	SID_LastError,
	SID_ErrorCount,
	SID_ActRepeatDelay,
	SID_ActRepeatRate,
	SID_TpDataPath,
	SID_TpCurrentPage,
	SID_PluginState,

	SID_ENUM_MAX
};

enum ActionHandlerToken
{
	AHID_Script = SID_ENUM_MAX,
	AHID_Plugin,

	AHID_ENUM_MAX,
};

enum ActionIdToken
{
	AID_Eval = AHID_ENUM_MAX,
	AID_Load,
	AID_Import,
	AID_Update,
	AID_SingleShot,  // deprecated, BC with v < 1.2

	AID_InstanceControl,
	AID_RepeatRate,
	AID_Shutdown,   // dev/debug builds only

	AID_ENUM_MAX
};

enum ActionDataIdToken
{
	ADID_InstanceName = AID_ENUM_MAX,
	ADID_EngineScope,
	ADID_Persistence,
	ADID_StateOption,
	ADID_StateDefault,
	ADID_Activation,
	ADID_Expression,
	ADID_ScriptFile,
	ADID_ModuleAlias,

	ADID_ENUM_MAX
};

enum StringDataTokensLast { STRING_TOKENS_COUNT = ADID_ENUM_MAX };

static constexpr const char * g_tokenStrings[STRING_TOKENS_COUNT]
{
	"createdStatesList",
	"lastError",
	"errorCount",
	"actRepeatDelay",
	"actRepeatRate",
	"tpDataPath",
	"currentPage",
	"pluginState",

	"script",
	"plugin",

	"eval",
	"load",
	"import",
	"update",
	"oneshot",

	"instance",
	"repRate",
	"shutdown",

	"name",
	"scope",
	"save",
	"state",
	"default",
	"activation",
	"expr",
	"file",
	"alias"
};

static const char * const * tokenStrings() { return g_tokenStrings; }


enum ChoiceListIdToken
{
	CLID_ScriptUpdateInstanceName,
	CLID_RepeatPropertyScriptName,
	CLID_ScriptActionEngineScope,
	CLID_PluginControlInstanceName,

	CLID_ENUM_MAX
};

static constexpr const char * g_listTokenStrings[CLID_ENUM_MAX]
{
	"script.update.name",
	"plugin.repRate.name",
	"script.d.scope",
	"plugin.instance.name",
};

static const char * const * choiceListTokenStrings() { return g_listTokenStrings; }

enum ActionTokens {
	AT_Unknown = STRING_TOKENS_COUNT + 1,

	CA_DelScript,
	CA_DelEngine,
	CA_ResetEngine,
	CA_SetStateValue,  // deprecated, BC with v < 1.2
	CA_SaveInstance,
	CA_LoadInstance,
	CA_DelSavedInstance,

	ST_ScriptsBaseDir,
	ST_SettingsVersion,
	ST_LoadScriptAtStart,

	AT_Script,
	AT_Engine,
	AT_Shared,
	AT_Private,
	AT_Default,
	AT_All,
	AT_Rate,
	AT_Delay,
	AT_RateDelay,
	AT_Set,
	AT_Increment,
	AT_Decrement,

	// send only
	AT_Starting,
	AT_Started,
	AT_Stopped,
};

static QByteArray tokenToName(int token, const QByteArray &deflt = QByteArray())
{
	static const QHash<int, QByteArray> hash = {
	  // These are mappings to human-usable names, not IDs
		{ AID_Eval,            "Eval" },
		{ AID_Load,            "Load" },
		{ AID_Import,          "Import" },
		{ AID_Update,          "Update" },
	  { AID_SingleShot,      "Anonymous (One-Time)" },  // deprecated
	  { AID_InstanceControl, "Instance Control" },
	  { AID_RepeatRate,      "Repeat Rate/Delay" },
	  { AID_Shutdown,        "Shutdown" },  // debug builds only

	  // These are actual strings used in selector lists to determine action to perform
	  { CA_DelScript,        "Delete Script Instance" },
	  { CA_DelEngine,        "Delete Engine Instance" },
	  { CA_ResetEngine,      "Reset Engine Environment" },
	  { CA_SetStateValue,    "Set State Value" },  // deprecated
	  { CA_SaveInstance,     "Save Script Instance" },
	  { CA_LoadInstance,     "Load Script Instance" },
	  { CA_DelSavedInstance, "Remove Saved Instance Data" },

	  // Settings are all key'd by their names, so these are verbatim as they'll come from TP
	  { ST_ScriptsBaseDir, "Script Files Base Directory" },
	  { ST_SettingsVersion,   "Settings Version" },
	  { ST_LoadScriptAtStart, "Load Script At Startup" },

	  // Plugin running state State values, used in Event evaluation.
	  { AT_Starting,  "Starting" },
	  { AT_Started,   "Started" },
	  { AT_Stopped,   "Stopped" },

	  // Misc. keywords used in various places but which need to be consistent
	  { AT_Script,    "Script" },
	  { AT_Engine,    "Engine" },
	  { AT_Shared,    "Shared" },
	  { AT_Private,   "Private" },
	  { AT_Default,   "Default" },
	  { AT_All,       "All" },
	  { AT_Rate,      "Rate" },
	  { AT_Delay,     "Delay" },
	  { AT_RateDelay, "Rate & Delay" },
	  { AT_Set,       "Set" },
	  { AT_Increment, "Increment" },
	  { AT_Decrement, "Decrement" },

	};
	return hash.value(token, deflt);
}

static QByteArray tokenToId(int token, const QByteArray &deflt = QByteArray())
{
	static const QHash<int, QByteArray> hash = {
	  { AHID_Script, g_tokenStrings[AHID_Script] },
	  { AHID_Plugin, g_tokenStrings[AHID_Plugin] },

		{ AID_Eval,            g_tokenStrings[AID_Eval] },
		{ AID_Load,            g_tokenStrings[AID_Load] },
		{ AID_Import,          g_tokenStrings[AID_Import] },
		{ AID_Update,          g_tokenStrings[AID_Update] },
	  { AID_SingleShot,      g_tokenStrings[AID_SingleShot] },  // deprecated
	  { AID_InstanceControl, g_tokenStrings[AID_InstanceControl] },
	  { AID_RepeatRate,      g_tokenStrings[AID_RepeatRate] },
	  { AID_Shutdown,        g_tokenStrings[AID_Shutdown] },  // debug builds only

	  { ADID_InstanceName, g_tokenStrings[ADID_InstanceName] },
		{ ADID_EngineScope,  g_tokenStrings[ADID_EngineScope] },
		{ ADID_Persistence,  g_tokenStrings[ADID_Persistence] },
		{ ADID_StateOption,  g_tokenStrings[ADID_StateOption] },
		{ ADID_StateDefault, g_tokenStrings[ADID_StateDefault] },
		{ ADID_Activation,   g_tokenStrings[ADID_Activation] },
		{ ADID_Expression,   g_tokenStrings[ADID_Expression] },
		{ ADID_ScriptFile,   g_tokenStrings[ADID_ScriptFile] },
		{ ADID_ModuleAlias,  g_tokenStrings[ADID_ModuleAlias] },
	};
	return hash.value(token, deflt);
}

static int tokenFromName(const QByteArray &name, int deflt = AT_Unknown)
{
	static const QHash<QByteArray, int> hash = {
		{ g_tokenStrings[AHID_Script],  AHID_Script },
	  { g_tokenStrings[AHID_Plugin],  AHID_Plugin },

		{ g_tokenStrings[AID_Eval],       AID_Eval },
	  { g_tokenStrings[AID_Load],       AID_Load },
	  { g_tokenStrings[AID_Import],     AID_Import },
	  { g_tokenStrings[AID_Update],     AID_Update },
	  { g_tokenStrings[AID_SingleShot], AID_SingleShot },

	  { g_tokenStrings[AID_InstanceControl], AID_InstanceControl },
	  { g_tokenStrings[AID_RepeatRate],      AID_RepeatRate },
	  { g_tokenStrings[AID_Shutdown],        AID_Shutdown },

	  { tokenToName(CA_DelScript),        CA_DelScript },
	  { "Delete Instance",                CA_DelScript },       // deprecated, BC with v < 1.2
	  { tokenToName(CA_DelEngine),        CA_DelEngine },
	  { tokenToName(CA_ResetEngine),      CA_ResetEngine },
	  { tokenToName(CA_SetStateValue),    CA_SetStateValue },   // deprecated, BC with v < 1.2
	  { tokenToName(CA_SaveInstance),     CA_SaveInstance },
	  { tokenToName(CA_LoadInstance),     CA_LoadInstance },
	  { tokenToName(CA_DelSavedInstance), CA_DelSavedInstance },

	  { tokenToName(ST_ScriptsBaseDir),    ST_ScriptsBaseDir },
	  { tokenToName(ST_SettingsVersion),   ST_SettingsVersion },
	  { tokenToName(ST_LoadScriptAtStart), ST_LoadScriptAtStart },

	  { tokenToName(AT_Script),    AT_Script },
	  { tokenToName(AT_Engine),    AT_Engine },
	  { tokenToName(AT_Shared),    AT_Shared },
	  { tokenToName(AT_Private),   AT_Private },
	  { tokenToName(AT_Default),   AT_Default },
	  { tokenToName(AT_All),       AT_All },
	  { tokenToName(AT_Rate),      AT_Rate },
	  { tokenToName(AT_Delay),     AT_Delay },
	  { tokenToName(AT_RateDelay), AT_RateDelay },
	  { tokenToName(AT_Set),       AT_Set },
	  { tokenToName(AT_Increment), AT_Increment },
	  { tokenToName(AT_Decrement), AT_Decrement },
	};
	return hash.value(name, deflt);
}

}  // namespace Strings
