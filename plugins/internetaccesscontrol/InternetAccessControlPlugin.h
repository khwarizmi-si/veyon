/*
 * InternetAccessControlPlugin.h - declaration of InternetAccessControlPlugin class
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

#include "Feature.h"
#include "FeatureProviderInterface.h"

class InternetAccessControlPlugin : public QObject, FeatureProviderInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.InternetAccessControl")
	Q_INTERFACES(PluginInterface FeatureProviderInterface)
public:
	explicit InternetAccessControlPlugin( QObject* parent = nullptr );
	~InternetAccessControlPlugin() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("6b8f2e44-1a93-4c7d-9e02-7c5b4a1d6f08") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral("InternetAccessControl");
	}

	QString description() const override
	{
		return tr( "Block or allow internet access on computers" );
	}

	QString vendor() const override
	{
		return QStringLiteral("Khwarizmi");
	}

	QString copyright() const override
	{
		return QStringLiteral("Khwarizmi");
	}

	const FeatureList& featureList() const override;

	bool controlFeature( Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						const ComputerControlInterfaceList& computerControlInterfaces ) override;

	bool handleFeatureMessage( VeyonServerInterface& server,
							   const MessageContext& messageContext,
							   const FeatureMessage& message ) override;

private:
	enum class FeatureCommand {
		BlockInternet,
		UnblockInternet,
	};

	const Feature m_internetAccessControlFeature;
	const FeatureList m_features;

};
