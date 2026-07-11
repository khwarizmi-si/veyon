/*
 * EntraIdPlugin.h - Microsoft Entra ID connector plugin
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

#include "ConfigurationPagePluginInterface.h"
#include "EntraIdConfiguration.h"
#include "NetworkObjectDirectoryPluginInterface.h"
#include "PluginInterface.h"
#include "UserGroupsBackendInterface.h"

class EntraIdGraphClient;

class EntraIdPlugin : public QObject,
					  PluginInterface,
					  NetworkObjectDirectoryPluginInterface,
					  UserGroupsBackendInterface,
					  ConfigurationPagePluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.EntraId")
	Q_INTERFACES(PluginInterface
				 NetworkObjectDirectoryPluginInterface
				 UserGroupsBackendInterface
				 ConfigurationPagePluginInterface)
public:
	explicit EntraIdPlugin( QObject* parent = nullptr );
	~EntraIdPlugin() override;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("2beaf30e-7fcb-42ba-a858-e96d3bb0a232") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 0, 1 );
	}

	QString name() const override
	{
		return QStringLiteral("EntraID");
	}

	QString description() const override
	{
		return tr( "Microsoft Entra ID connector for devices, users and groups" );
	}

	QString vendor() const override
	{
		return QStringLiteral("Khwarizmi");
	}

	QString copyright() const override
	{
		return QStringLiteral("Khwarizmi");
	}

	QString directoryName() const override
	{
		return tr( "Microsoft Entra ID (load devices and locations from Microsoft Graph)" );
	}

	NetworkObjectDirectory* createNetworkObjectDirectory( QObject* parent ) override;

	QString userGroupsBackendName() const override
	{
		return tr( "Microsoft Entra ID (load security groups from Microsoft Graph)" );
	}

	void reloadConfiguration() override;
	QStringList userGroups( bool queryDomainGroups ) override;
	QStringList groupsOfUser( const QString& username, bool queryDomainGroups ) override;
	QString userGroupSecurityIdentifier( const QString& groupName ) override;

	ConfigurationPage* createConfigurationPage() override;

private:
	EntraIdGraphClient& graphClient();

	EntraIdConfiguration m_configuration;
	EntraIdGraphClient* m_graphClient = nullptr;
	QHash<QString, QString> m_groupSecurityIds;

};
