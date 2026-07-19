/*
 * BuiltinDirectory.cpp - NetworkObjects from VeyonConfiguration
 *
 * Copyright (c) 2017-2026 Tobias Junghans <tobydox@veyon.io>
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

#include <QJsonArray>
#include <QJsonObject>

#include "BuiltinDirectoryConfiguration.h"
#include "BuiltinDirectory.h"

namespace
{

const auto DefaultSelfLocationUid = NetworkObject::Uid( QStringLiteral("4e6396c4-4f33-4bc2-b9be-337172b4fcb3") );
const auto DefaultSelfComputerUid = NetworkObject::Uid( QStringLiteral("86e828ef-9134-4c61-8778-e6ae9e775a2f") );

QJsonArray defaultSelfNetworkObjects()
{
	return {
		NetworkObject( NetworkObject::Type::Location,
					   QStringLiteral("Lab"),
					   {},
					   {},
					   {},
					   DefaultSelfLocationUid ).toJson(),
		NetworkObject( NetworkObject::Type::Host,
					   QStringLiteral("Self"),
					   QStringLiteral("127.0.0.1"),
					   {},
					   {},
					   DefaultSelfComputerUid,
					   DefaultSelfLocationUid ).toJson()
	};
}

}


BuiltinDirectory::BuiltinDirectory( BuiltinDirectoryConfiguration& configuration, QObject* parent ) :
	NetworkObjectDirectory( parent ),
	m_configuration( configuration )
{
}



void BuiltinDirectory::update()
{
	m_configuration.reloadFromStore();

	auto networkObjects = m_configuration.networkObjects();
	if( networkObjects.isEmpty() )
	{
		networkObjects = defaultSelfNetworkObjects();
	}

	NetworkObjectUidList groupUids;

	for( const auto& networkObjectValue : networkObjects )
	{
		const NetworkObject networkObject( networkObjectValue.toObject() );

		if( networkObject.type() == NetworkObject::Type::Location )
		{
			groupUids.append( networkObject.uid() ); // clazy:exclude=reserve-candidates

			addOrUpdateObject(networkObject, rootObject());

			updateLocation( networkObject, networkObjects );
		}
	}

	removeObjects(rootObject(), [groupUids](const NetworkObject& object) {
		return object.type() == NetworkObject::Type::Location && groupUids.contains(object.uid()) == false;
	});
}



void BuiltinDirectory::updateLocation( const NetworkObject& locationObject, const QJsonArray& networkObjects )
{
	NetworkObjectUidList computerUids;

	for( const auto& networkObjectValue : networkObjects )
	{
		NetworkObject networkObject( networkObjectValue.toObject() );

		if( networkObject.parentUid() == locationObject.uid() )
		{
			computerUids.append( networkObject.uid() ); // clazy:exclude=reserve-candidates
			addOrUpdateObject( networkObject, locationObject );
		}
	}

	removeObjects( locationObject, [computerUids]( const NetworkObject& object ) {
		return object.type() == NetworkObject::Type::Host && computerUids.contains( object.uid() ) == false; } );
}
