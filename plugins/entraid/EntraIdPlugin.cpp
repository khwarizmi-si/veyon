/*
 * EntraIdPlugin.cpp - Microsoft Entra ID connector plugin
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

#include "EntraIdConfigurationPage.h"
#include "EntraIdGraphClient.h"
#include "EntraIdNetworkObjectDirectory.h"
#include "EntraIdPlugin.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"


EntraIdPlugin::EntraIdPlugin( QObject* parent ) :
	QObject( parent ),
	m_configuration( &VeyonCore::config() )
{
}



EntraIdPlugin::~EntraIdPlugin()
{
	delete m_graphClient;
	m_graphClient = nullptr;
}



NetworkObjectDirectory* EntraIdPlugin::createNetworkObjectDirectory( QObject* parent )
{
	return new EntraIdNetworkObjectDirectory( m_configuration, parent );
}



void EntraIdPlugin::reloadConfiguration()
{
	delete m_graphClient;
	m_graphClient = nullptr;
	m_groupSecurityIds.clear();
}



QStringList EntraIdPlugin::userGroups( bool queryDomainGroups )
{
	Q_UNUSED(queryDomainGroups)

	if( m_configuration.useSecurityGroupsForAccessControl() == false )
	{
		return {};
	}

	QString errorString;
	const auto groups = graphClient().securityGroups( &errorString );
	if( errorString.isEmpty() == false )
	{
		vWarning() << "Microsoft Entra ID group sync failed:" << errorString;
		return {};
	}

	QStringList names;
	m_groupSecurityIds.clear();
	for( const auto& value : groups )
	{
		const auto group = value.toObject();
		const auto name = EntraIdGraphClient::jsonStringValue( group, QStringLiteral("displayName") );
		if( name.isEmpty() == false )
		{
			names.append( name );
			m_groupSecurityIds[name] = EntraIdGraphClient::jsonStringValue( group, QStringLiteral("securityIdentifier") );
		}
	}
	names.sort( Qt::CaseInsensitive );
	return names;
}



QStringList EntraIdPlugin::groupsOfUser( const QString& username, bool queryDomainGroups )
{
	Q_UNUSED(queryDomainGroups)

	if( m_configuration.useSecurityGroupsForAccessControl() == false )
	{
		return {};
	}

	QString errorString;
	const auto groups = graphClient().groupsOfUser( username, &errorString );
	if( errorString.isEmpty() == false )
	{
		vWarning() << "Microsoft Entra ID user group query failed:" << errorString;
		return {};
	}

	QStringList names;
	for( const auto& value : groups )
	{
		const auto group = value.toObject();
		if( group.value( QStringLiteral("securityEnabled") ).toBool() == false )
		{
			continue;
		}
		const auto name = EntraIdGraphClient::jsonStringValue( group, QStringLiteral("displayName") );
		if( name.isEmpty() == false )
		{
			names.append( name );
			m_groupSecurityIds[name] = EntraIdGraphClient::jsonStringValue( group, QStringLiteral("securityIdentifier") );
		}
	}
	names.sort( Qt::CaseInsensitive );
	return names;
}



QString EntraIdPlugin::userGroupSecurityIdentifier( const QString& groupName )
{
	return m_groupSecurityIds.value( groupName );
}



ConfigurationPage* EntraIdPlugin::createConfigurationPage()
{
	return new EntraIdConfigurationPage( m_configuration );
}



EntraIdGraphClient& EntraIdPlugin::graphClient()
{
	if( m_graphClient == nullptr )
	{
		m_graphClient = new EntraIdGraphClient( m_configuration, this );
	}
	return *m_graphClient;
}
