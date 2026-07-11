/*
 * EntraIdNetworkObjectDirectory.cpp - NetworkObjectDirectory backed by Microsoft Graph devices
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

#include <QJsonObject>
#include <QUrl>

#include "EntraIdConfiguration.h"
#include "EntraIdGraphClient.h"
#include "EntraIdNetworkObjectDirectory.h"
#include "VeyonCore.h"


EntraIdNetworkObjectDirectory::EntraIdNetworkObjectDirectory( const EntraIdConfiguration& configuration, QObject* parent ) :
	NetworkObjectDirectory( parent ),
	m_configuration( configuration ),
	m_graphClient( new EntraIdGraphClient( configuration, this ) )
{
}



void EntraIdNetworkObjectDirectory::update()
{
	if( m_graphClient->isConfigured() == false )
	{
		vWarning() << "Microsoft Entra ID connector is not configured";
		return;
	}

	QString errorString;
	const auto devices = m_graphClient->devices( &errorString );
	if( errorString.isEmpty() == false )
	{
		vWarning() << "Microsoft Entra ID device sync failed:" << errorString;
		return;
	}

	QHash<NetworkObject::Uid, NetworkObject> locationsByUid;
	QHash<NetworkObject::Uid, NetworkObjectList> computersByLocation;

	for( const auto& value : devices )
	{
		const auto device = value.toObject();
		const auto location = locationObject( EntraIdGraphClient::jsonStringValue( device, m_configuration.deviceLocationField() ) );
		const auto computer = deviceObject( device, location );

		if( computer.hostAddress().isEmpty() )
		{
			continue;
		}

		locationsByUid[location.uid()] = location;
		computersByLocation[location.uid()].append( computer );
	}

	NetworkObjectList locations;
	for( auto it = locationsByUid.constBegin(), end = locationsByUid.constEnd(); it != end; ++it )
	{
		locations.append( it.value() );
	}

	replaceObjects( locations, rootObject() );

	for( const auto& location : std::as_const( locations ) )
	{
		replaceObjects( computersByLocation.value( location.uid() ), location );
		setObjectPopulated( location );
	}
}



NetworkObject EntraIdNetworkObjectDirectory::locationObject( const QString& locationName ) const
{
	auto name = locationName.trimmed();
	if( name.isEmpty() )
	{
		name = m_configuration.fallbackLocationName().trimmed();
	}
	if( name.isEmpty() )
	{
		name = tr( "Entra ID devices" );
	}

	const auto directoryAddress = QStringLiteral("entra://locations/%1")
								  .arg( QString::fromUtf8( QUrl::toPercentEncoding( name ) ) );
	return NetworkObject( NetworkObject::Type::Location, name, {}, {}, directoryAddress );
}



NetworkObject EntraIdNetworkObjectDirectory::deviceObject( const QJsonObject& device, const NetworkObject& location ) const
{
	auto displayName = EntraIdGraphClient::jsonStringValue( device, m_configuration.deviceDisplayNameField() );
	auto hostAddress = EntraIdGraphClient::jsonStringValue( device, m_configuration.deviceHostAddressField() );
	const auto macAddress = EntraIdGraphClient::jsonStringValue( device, m_configuration.deviceMacAddressField() );
	const auto graphId = EntraIdGraphClient::jsonStringValue( device, QStringLiteral("id") );

	if( displayName.isEmpty() )
	{
		displayName = hostAddress;
	}
	if( hostAddress.isEmpty() )
	{
		hostAddress = displayName;
	}

	const auto directoryAddress = QStringLiteral("entra://devices/%1/%2")
								  .arg( QString::fromUtf8( QUrl::toPercentEncoding( location.name() ) ),
										graphId );

	return NetworkObject( NetworkObject::Type::Host,
						  displayName,
						  hostAddress,
						  macAddress,
						  directoryAddress,
						  {},
						  location.uid() );
}
