/*
 * NetworkDiscoveryPlugin.h - declaration of NetworkDiscoveryPlugin class
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

#include "NetworkObjectDirectoryPluginInterface.h"
#include "PluginInterface.h"

class NetworkDiscoveryPlugin : public QObject, PluginInterface, NetworkObjectDirectoryPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.NetworkDiscovery")
	Q_INTERFACES(PluginInterface NetworkObjectDirectoryPluginInterface)
public:
	explicit NetworkDiscoveryPlugin( QObject* parent = nullptr );
	~NetworkDiscoveryPlugin() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("7e3b1c0a-9d52-4f86-bb14-5a2e8c7d019f") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral("NetworkDiscovery");
	}

	QString description() const override
	{
		return tr( "Discover computers by scanning the network" );
	}

	QString vendor() const override
	{
		return QStringLiteral("Al-Khwarizmi");
	}

	QString copyright() const override
	{
		return QStringLiteral("Al-Khwarizmi");
	}

	QString directoryName() const override
	{
		return tr( "Network Discovery" );
	}

	NetworkObjectDirectory* createNetworkObjectDirectory( QObject* parent ) override;

};
