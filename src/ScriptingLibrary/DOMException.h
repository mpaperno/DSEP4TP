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
#include <private/qv4domerrors_p.h>

#define DOMEXCEPTION_SECURITY_ERR      18
#define DOMEXCEPTION_NETWORK_ERR       19
#define DOMEXCEPTION_ABORT_ERR         20
#define DOMEXCEPTION_URL_MISMATCH_ERR  21
#define DOMEXCEPTION_QUOTA_EXCEEDED_ERR  22
#define DOMEXCEPTION_TIMEOUT_ERR       23
#define DOMEXCEPTION_INVALID_NODE_ERR  24
#define DOMEXCEPTION_DATA_CLONE_ERR    25

#undef THROW_DOM

#define THROW_DOM(error, string, name) { \
  return throwDomError(scope.engine, error, QStringLiteral(string), QStringLiteral(name)); \
}

namespace QV4 {
struct ExecutionEngine;

extern ReturnedValue throwDomError(ExecutionEngine *e, int error, const QString &message, const QString &name = QString());
}

void dse_add_domexceptions(QV4::ExecutionEngine *e);
