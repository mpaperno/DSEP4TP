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

#include <private/qv4scopedvalue_p.h>

namespace QV4 {
struct ExecutionEngine;

extern QV4::ReturnedValue newDomError(QV4::ExecutionEngine *e, int error, const QString &message = QString(), const QString &name = QString());
extern QV4::ReturnedValue throwDomError(QV4::ExecutionEngine *e, int error, const QString &message = QString(), const QString &name = QString());
extern QJSValue newDomErrorObject(QV4::ExecutionEngine *e, int error, const QString &message = QString(), const QString &name = QString());
extern QJSValue newDomErrorObject(QV4::ExecutionEngine *e, const QString &name, const QString &message = QString());
}

void dse_add_domexceptions(QV4::ExecutionEngine *e);
