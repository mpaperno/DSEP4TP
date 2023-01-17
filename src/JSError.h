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

#include <QJSValue>

struct JSError
{
	QJSValue::ErrorType type = QJSValue::NoError;
	QString name;
	QString message;
	QString fileName;
	QString lineNumber;
	QString stack;
	QString instanceName;
	QJSValue cause;

	JSError() {}

	explicit JSError(const QJSValue &err, const QByteArray &instName = QByteArray())
	{
		QJSValue tmp;
		type = err.isError() ? err.errorType() : QJSValue::GenericError;
		if (!(tmp = err.property(QStringLiteral("name"))).isUndefined())
			name = tmp.toString();
		else
			name = QStringLiteral("Error");
		if (!(tmp = err.property(QStringLiteral("message"))).isUndefined())
			message = tmp.toString();
		else
			message = err.toString();
		if (!(tmp = err.property(QStringLiteral("fileName"))).isUndefined())
			fileName = tmp.toString();
		if (!(tmp = err.property(QStringLiteral("lineNumber"))).isUndefined())
			lineNumber = tmp.toString();
		if (!(tmp = err.property(QStringLiteral("stack"))).isUndefined())
			stack = tmp.toString();
		if (!instName.isEmpty())
			instanceName = QString::fromUtf8(instName);
		else if (!(tmp = err.property(QStringLiteral("instanceName"))).isUndefined())
			instanceName = tmp.toString();
		if (err.hasOwnProperty(QStringLiteral("cause"))) {
			cause = err.property(QStringLiteral("cause"));
			if (stack.isEmpty() && cause.isObject() && !(tmp = cause.property(QStringLiteral("stack"))).isUndefined())
				stack = tmp.toString();
		}
	}

	QString toString(QStringView msg = QStringView()) const
	{
		QString ret = name + ": " + message;
		if (!fileName.isEmpty()) {
			ret += " (in file '" + fileName + '\'';
			if (!lineNumber.isEmpty())
				ret += " at line " + lineNumber;
			if (!msg.isEmpty())
				ret += ' ' + msg;
			ret += ')';
		}
	  return ret;
	}
};

Q_DECLARE_METATYPE(JSError)
