/*
 * NetworkDiscoveryDirectory.cpp - implementation of NetworkDiscoveryDirectory class
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

#include <QNetworkInterface>
#include <QSet>

#include "NetworkDiscoveryDirectory.h"
#include "NetworkScanner.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"


NetworkDiscoveryDirectory::NetworkDiscoveryDirectory( QObject* parent ) :
	NetworkObjectDirectory( parent ),
	m_scanner( new NetworkScanner( this ) ),
	m_location( NetworkObject::Type::Location, NetworkDiscoveryDirectory::tr( "Discovered computers" ),
				{}, {}, QStringLiteral("sahid-network-discovery") )
{
	connect( m_scanner, &NetworkScanner::hostFound, this, &NetworkDiscoveryDirectory::onHostFound );
	connect( m_scanner, &NetworkScanner::finished, this, &NetworkDiscoveryDirectory::onScanFinished );
}



void NetworkDiscoveryDirectory::update()
{
	if( m_scanner->isRunning() )
	{
		return; // a scan is already in progress
	}

	addOrUpdateObject( m_location, rootObject() );

	m_discoveredUids.clear();
	m_scanner->start( localSubnetTargets(),
					  static_cast<quint16>( VeyonCore::config().veyonServerPort() ) );
}



void NetworkDiscoveryDirectory::onHostFound( QHostAddress address )
{
	const auto host = address.toString();
	const NetworkObject computer( NetworkObject::Type::Host, host, host, {}, host );

	addOrUpdateObject( computer, m_location );
	m_discoveredUids.append( computer.uid() );
}



void NetworkDiscoveryDirectory::onScanFinished()
{
	// drop computers that no longer responded (e.g. powered off or DHCP-reassigned)
	const auto discovered = m_discoveredUids;
	removeObjects( m_location, [discovered]( const NetworkObject& object ) {
		return object.type() == NetworkObject::Type::Host && discovered.contains( object.uid() ) == false;
	} );

	setObjectPopulated( m_location );
}



QList<QHostAddress> NetworkDiscoveryDirectory::localSubnetTargets() const
{
	constexpr int maxTargets = 2048;

	QList<QHostAddress> targets;
	QSet<quint32> seen;

	const auto interfaces = QNetworkInterface::allInterfaces();
	for( const auto& networkInterface : interfaces )
	{
		const auto flags = networkInterface.flags();
		if( flags.testFlag( QNetworkInterface::IsUp ) == false ||
			flags.testFlag( QNetworkInterface::IsLoopBack ) )
		{
			continue;
		}

		const auto entries = networkInterface.addressEntries();
		for( const auto& entry : entries )
		{
			const auto ip = entry.ip();
			if( ip.protocol() != QAbstractSocket::IPv4Protocol || ip.isLoopback() )
			{
				continue;
			}

			// scan the /24 the interface lives in (covers the typical single-subnet lab)
			const quint32 base = ip.toIPv4Address() & 0xFFFFFF00u;
			for( quint32 hostPart = 1; hostPart <= 254 && targets.size() < maxTargets; ++hostPart )
			{
				const quint32 candidate = base | hostPart;
				if( seen.contains( candidate ) == false )
				{
					seen.insert( candidate );
					targets.append( QHostAddress( candidate ) );
				}
			}
		}
	}

	return targets;
}
