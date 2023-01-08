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

#include <QCoreApplication>
#include <QObject>

#include "utils.h"
//#include "Plugin.h"
//#include "ScriptEngine.h"

#define PLUGIN_STATE_ID_PREFIX  "dsep."

class DSE : public QObject
{
		Q_OBJECT
		Q_PROPERTY(quint32 PLUGIN_VERSION_NUM MEMBER pluginVersion CONSTANT)
		Q_PROPERTY(QString PLUGIN_VERSION_STR MEMBER pluginVersionStr CONSTANT)
		Q_PROPERTY(QString SCRIPTS_BASE_DIR READ getScriptsBaseDir CONSTANT)
		Q_PROPERTY(QString INSTANCE_TYPE READ instanceType CONSTANT)
		Q_PROPERTY(QString INSTANCE_NAME MEMBER instanceName CONSTANT)
		Q_PROPERTY(QString INSTANCE_DEFAULT_VALUE MEMBER instanceDefault CONSTANT)
		Q_PROPERTY(QString VALUE_STATE_PREFIX READ valueStatePrefix CONSTANT)
		Q_PROPERTY(QString PLATFORM_OS MEMBER platformOs CONSTANT)
		Q_PROPERTY(QString TP_USER_DATA_PATH READ tpDataPath CONSTANT)
		Q_PROPERTY(quint32 TP_VERSION_CODE MEMBER tpVersion CONSTANT)
		Q_PROPERTY(QString TP_VERSION_STR MEMBER tpVersionStr CONSTANT)

		//ScriptEngine *se = nullptr;

	public:
		static const quint32 pluginVersion;
		static const QByteArray pluginVersionStr;
		static const QString platformOs;
		static quint32 tpVersion;
		static QString tpVersionStr;
		static QString scriptsBaseDir;
		static QByteArray tpCurrentPage;
		static std::atomic_int defaultRepeatRate;

		bool privateInstance { false };
		QByteArray instanceName;
		QByteArray instanceDefault;

		explicit DSE(/*ScriptEngine *se = nullptr,*/ QObject *p = nullptr) :
		  QObject(p)/*, se(se)*/
		{
			setObjectName("DSE");
		}

		inline QString instanceType() const { return privateInstance ? QStringLiteral("Private") : QStringLiteral("Shared"); };
		Q_INVOKABLE inline QString instanceStateId() { return valueStatePrefix() + instanceName; }

		static inline QString valueStatePrefix() { return QStringLiteral(PLUGIN_STATE_ID_PREFIX); }
		static inline QString tpDataPath() { return QString::fromUtf8(Utils::tpDataPath()); }
		static inline QString getScriptsBaseDir() { return scriptsBaseDir.isEmpty() ? QDir::currentPath() : scriptsBaseDir; }
		static inline QString resolveFile(const QString &base)
		{
			if (scriptsBaseDir.isEmpty() || base.isEmpty())
				return base;
			const QString tbase = QDir::fromNativeSeparators(base);
			if (QDir::isAbsolutePath(tbase))
				return tbase;
			return QDir::cleanPath(scriptsBaseDir + tbase);
		}

};
