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

#define PRAGMA_STR(x) _Pragma(#x)
#define GCC_DIAGNOSTIC_IGNORE(warnoption, ...)      \
  PRAGMA_STR(GCC diagnostic push)                 \
  PRAGMA_STR(GCC diagnostic ignored #warnoption)  \
  __VA_ARGS__                                     \
  PRAGMA_STR(GCC diagnostic pop)

#ifdef _MSC_VER
#define DISABLE_GCC_WARNING(warnoption, ...)      \
  PRAGMA_STR(warning (push))                      \
  PRAGMA_STR(warning (disable: 4068))             \
  GCC_DIAGNOSTIC_IGNORE(warnoption, __VA_ARGS__)  \
  PRAGMA_STR(warning (pop))
#else
#define DISABLE_GCC_WARNING(warnoption, ...)  GCC_DIAGNOSTIC_IGNORE(warnoption, __VA_ARGS__)
#endif


#ifdef QT_DEBUG
  #define LOGMINLEVEL  QtDebugMsg
#else
  #define LOGMINLEVEL  QtInfoMsg
#endif

DISABLE_GCC_WARNING(-Wunused-function,
static Q_LOGGING_CATEGORY(lcPlugin, PLUGIN_SYSTEM_NAME, LOGMINLEVEL)
static Q_LOGGING_CATEGORY(lcDse,    "DSE", QtDebugMsg)
)


#define Q_ENUM_NS_N(ENUM, NS) \
  Q_ENUMS(ENUM) \
  inline constexpr const QMetaObject *qt_getEnumMetaObject(ENUM) noexcept { return & NS::staticMetaObject; } \
  inline constexpr const char *qt_getEnumName(ENUM) noexcept { return #ENUM; }
