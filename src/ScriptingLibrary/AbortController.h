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

#include <QObject>
#include <QJSValue>
#include <QJSEngine>

#include "ScriptEngine.h"
#include "event_utils.h"

namespace ScriptLib {

class AbortController;

class AbortSignal : public QObject
{
	private:
		Q_OBJECT
		Q_PROPERTY(bool aborted READ aborted NOTIFY abort)
		Q_PROPERTY(QJSValue reason READ reason CONSTANT)
		EVENT_PROPERTY(abort)
		friend class AbortController;

		bool m_aborted = false;
		QJSValue m_reason;

		void setAborted(QJSValue reason)
		{
			m_aborted = true;
			m_reason = std::move(reason);
			Q_EMIT abort(m_reason);
		}

	public:
		using QObject::QObject;

		bool aborted() const { return m_aborted; }
		QJSValue reason() const { return m_reason; }

		NAMED_EVENT_HANDLER();

	public Q_SLOTS:
		void throwIfAborted() const {
			if (m_aborted)
				ScriptEngine::throwError(this, m_reason);
		}

	Q_SIGNALS:
		void abort(const QJSValue &reason);
};


class AbortController : public QObject
{
	private:
		Q_OBJECT
		Q_PROPERTY(ScriptLib::AbortSignal *signal READ signal CONSTANT)
		EVENT_PROPERTY(aborted)
		AbortSignal m_signal;

	public:
		Q_INVOKABLE AbortController(QObject *p = nullptr) : QObject(p)
		{
			setObjectName(QLatin1String("AbortController"));
			m_signal.setObjectName(QLatin1String("AbortSignal"));
			connect(&m_signal, &AbortSignal::abort, this, &AbortController::aborted);
		}

		Q_INVOKABLE ScriptLib::AbortSignal *signal() { return &m_signal; }
		NAMED_EVENT_HANDLER();

	public Q_SLOTS:
		void abort(QJSValue reason = QJSValue())
		{
			if (reason.isUndefined()) {
				if (QJSEngine *jse = qjsEngine(this)) {
					reason = jse->evaluate(QStringLiteral("new DOMException('The operation was aborted', 'AbortError');"));
				}
			}
			m_signal.setAborted(reason);
		}

	Q_SIGNALS:
		void aborted(const QJSValue &reason);
};

}

Q_DECLARE_METATYPE(ScriptLib::AbortSignal *)
Q_DECLARE_METATYPE(ScriptLib::AbortController *)
