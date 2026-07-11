/*
 * NetworkScanner.h - asynchronous TCP port scanner for a list of hosts
 *
 * Copyright (c) 2026 Al-Khwarizmi
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#pragma once

#include <QHostAddress>
#include <QObject>

// Probes a list of hosts for an open TCP port using a bounded pool of
// non-blocking sockets, so scanning a whole subnet never blocks the UI.
class NetworkScanner : public QObject
{
	Q_OBJECT
public:
	explicit NetworkScanner( QObject* parent = nullptr );

	void start( const QList<QHostAddress>& targets, quint16 port );
	bool isRunning() const { return m_running; }

Q_SIGNALS:
	void hostFound( QHostAddress address );
	void finished();

private:
	void launchProbes();

	static constexpr int kMaxConcurrentProbes = 48;
	static constexpr int kProbeTimeoutMs = 600;

	QList<QHostAddress> m_queue;
	quint16 m_port = 0;
	int m_activeProbes = 0;
	bool m_running = false;

};
