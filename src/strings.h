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

enum ActionIdToken
{
	AID_Eval = SID_ENUM_MAX,
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

static const char * const * tokenStrings()
{
	static constexpr const char * tokens[STRING_TOKENS_COUNT]
	{
		"createdStatesList",
		"lastError",
		"errorCount",
		"actRepeatDelay",
		"actRepeatRate",
		"tpDataPath",
		"currentPage",
		"pluginState",

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
	return tokens;
}

enum ChoiceListIdToken
{
	CLID_ScriptUpdateInstanceName,
	CLID_RepeatPropertyScriptName,
	CLID_ScriptActionEngineScope,
	CLID_PluginControlInstanceName,

	CLID_ENUM_MAX
};

static const char * const * choiceListTokenStrings()
{
	static constexpr const char * tokens[CLID_ENUM_MAX]
	{
		"script.update.name",
		"plugin.repRate.name",
		"script.d.scope",
		"plugin.instance.name",
	};
	return tokens;
}

enum ActionTokens {
	AT_Unknown = STRING_TOKENS_COUNT + 1,

	AH_Script,
	AH_Plugin,

	CA_DelScript,
	CA_DelEngine,
	CA_ResetEngine,
	CA_SetStateValue,  // deprecated, BC with v < 1.2
	CA_SaveInstance,
	CA_LoadInstance,
	CA_DelSavedInstance,

	ST_SettingsVersion,

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

static int tokenFromName(const QByteArray &name, int deflt = AT_Unknown)
{
	static const QHash<QByteArray, int> hash = {
		{ "script",  AH_Script },
	  { "plugin",  AH_Plugin },

		{ tokenStrings()[AID_Eval],       AID_Eval },
	  { tokenStrings()[AID_Load],       AID_Load },
	  { tokenStrings()[AID_Import],     AID_Import },
	  { tokenStrings()[AID_Update],     AID_Update },
	  { tokenStrings()[AID_SingleShot], AID_SingleShot },

	  { tokenStrings()[AID_InstanceControl], AID_InstanceControl },
	  { tokenStrings()[AID_RepeatRate],      AID_RepeatRate },
	  { tokenStrings()[AID_Shutdown],        AID_Shutdown },

	  { "Delete Script Instance",   CA_DelScript },
	  { "Delete Instance",          CA_DelScript },       // deprecated, BC with v < 1.2
	  { "Delete Engine Instance",   CA_DelEngine },
	  { "Reset Engine Environment", CA_ResetEngine },
	  { "Set State Value",          CA_SetStateValue },   // deprecated, BC with v < 1.2
	  { "Save Script Instance",     CA_SaveInstance },
	  { "Load Script Instance",     CA_LoadInstance },
	  { "Remove Saved Instance Data", CA_DelSavedInstance },

	  { "Settings Version", ST_SettingsVersion },

	  { "Script",       AT_Script },
	  { "Engine",       AT_Engine },
	  { "Shared",       AT_Shared },
	  { "Private",      AT_Private },
	  { "Default",      AT_Default },
	  { "All",          AT_All },
	  { "Rate",         AT_Rate },
	  { "Delay",        AT_Delay },
	  { "Rate & Delay", AT_RateDelay },
	  { "Set",          AT_Set },
	  { "Increment",    AT_Increment },
	  { "Decrement",    AT_Decrement },
	};
	return hash.value(name, deflt);
}

static QByteArray tokenToName(int token, const QByteArray &deflt = QByteArray())
{
	// These are mappings to human-usable names, not necessarily IDs
	static const QHash<int, QByteArray> hash = {
		{ AID_Eval,            "Eval" },
		{ AID_Load,            "Load" },
		{ AID_Import,          "Import" },
		{ AID_Update,          "Update" },
	  { AID_SingleShot,      "Anonymous (One-Time)" },  // deprecated
	  { AID_InstanceControl, "Instance Control" },
	  { AID_RepeatRate,      "Repeat Rate/Delay" },
	  { AID_Shutdown,        "Shutdown" },  // debug builds only

	  { CA_DelScript,        "Delete Script Instance" },
	  { CA_DelScript,        "Delete Engine Instance" },
	  { CA_ResetEngine,      "Reset Engine Environment" },
	  { CA_SetStateValue,    "Set State Value" },  // deprecated
	  { CA_SaveInstance,     "Save Script Instance" },
	  { CA_LoadInstance,     "Load Script Instance" },
	  { CA_DelSavedInstance, "Remove Saved Instance Data" },

	  { ST_SettingsVersion, "Settings Version" },

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

	  { AT_Starting,  "Starting" },
	  { AT_Started,   "Started" },
	  { AT_Stopped,   "Stopped" },
	};
	return hash.value(token, deflt);
}

}  // namespace Strings
