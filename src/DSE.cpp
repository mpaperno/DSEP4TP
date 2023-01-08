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

#include "version.h"
#include "DSE.h"

const quint32 DSE::pluginVersion { APP_VERSION };
const QByteArray DSE::pluginVersionStr { QByteArrayLiteral(APP_VERSION_STR) };
const QString DSE::platformOs {
#if defined(Q_OS_ANDROID)
	QStringLiteral("android")
#elif defined(Q_OS_IOS)
	QStringLiteral("ios")
#elif defined(Q_OS_TVOS)
	QStringLiteral("tvos")
#elif defined(Q_OS_MAC)
	QStringLiteral("osx")
#elif defined(Q_OS_WIN)
	QStringLiteral("windows")
#elif defined(Q_OS_LINUX)
	QStringLiteral("linux")
#elif defined(Q_OS_QNX)
	QStringLiteral("qnx")
#elif defined(Q_OS_WASM)
	QStringLiteral("wasm")
#elif defined(Q_OS_UNIX)
	QStringLiteral("unix")
#else
	QStringLiteral("unknown")
#endif
};

quint32 DSE::tpVersion {0};
QString DSE::tpVersionStr;
QString DSE::scriptsBaseDir;
QByteArray DSE::tpCurrentPage;
std::atomic_int DSE::defaultRepeatRate { 350 };
