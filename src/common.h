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

#include <QDebug>
#include <QLoggingCategory>

#include "version.h"

#ifdef QT_DEBUG
static Q_LOGGING_CATEGORY(lcPlugin, PLUGIN_SYSTEM_NAME, QtDebugMsg)
#else
static Q_LOGGING_CATEGORY(lcPlugin, PLUGIN_SYSTEM_NAME, QtInfoMsg)
#endif
static Q_LOGGING_CATEGORY(lcDse, "DSE", QtDebugMsg)

static constexpr const char* DYNAMIC_VALUE_STATE_PRFX = "dsep.";
