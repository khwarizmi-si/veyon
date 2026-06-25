/*
 * NetworkScanner.cpp - implementation of NetworkScanner class
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

#include <memory>

#include <QTcpSocket>
#include <QTimer>

#include "NetworkScanner.h"


NetworkScanner::NetworkScanner( QObject* parent ) :
	QObject( parent )
{
}



void NetworkScanner::start( const QList<QHostAddress>& targets, quint16 port )
{
	if( m_running )
	{
		return;
	}

	m_queue = targets;
	m_port = port;
	m_running = true;

	if( m_queue.isEmpty() )
	{
		m_running = false;
		Q_EMIT finished();
		return;
	}

	launchProbes();
}



void NetworkScanner::launchProbes()
{
	while( m_activeProbes < kMaxConcurrentProbes && m_queue.isEmpty() == false )
	{
		const auto target = m_queue.takeFirst();
		++m_activeProbes;

		auto socket = new QTcpSocket( this );
		auto timer = new QTimer( this );
		timer->setSingleShot( true );

		// guard so the connected/error/timeout handlers run their cleanup only once
		auto done = std::make_shared<bool>( false );

		auto complete = [this, socket, timer, target, done]( bool found ) {
			if( *done )
			{
				return;
			}
			*done = true;

			timer->stop();
			timer->deleteLater();
			socket->abort();
			socket->deleteLater();

			--m_activeProbes;

			if( found )
			{
				Q_EMIT hostFound( target );
			}

			// keep the pipeline full, then detect completion
			launchProbes();
			if( m_running && m_queue.isEmpty() && m_activeProbes == 0 )
			{
				m_running = false;
				Q_EMIT finished();
			}
		};

		connect( socket, &QTcpSocket::connected, this, [complete]() { complete( true ); } );
		connect( socket, &QTcpSocket::errorOccurred, this, [complete]() { complete( false ); } );
		connect( timer, &QTimer::timeout, this, [complete]() { complete( false ); } );

		timer->start( kProbeTimeoutMs );
		socket->connectToHost( target, m_port );
	}
}
