/*
 * AuvidusFeaturePlugin.h - declaration of AuvidusFeaturePlugin class
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

// Auvidus: one-click control of audio, webcam and USB storage devices on the
// managed computers (e.g. block USB sticks and mute microphones during exams).
class AuvidusFeaturePlugin : public QObject, FeatureProviderInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.Auvidus")
	Q_INTERFACES(PluginInterface FeatureProviderInterface)
public:
	explicit AuvidusFeaturePlugin( QObject* parent = nullptr );
	~AuvidusFeaturePlugin() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("0d5e8a31-4c7b-4f29-9a16-3e8b2c1d7f04") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral("Auvidus");
	}

	QString description() const override
	{
		return tr( "Control audio, webcam and USB devices on computers" );
	}

	QString vendor() const override
	{
		return QStringLiteral("Sahid");
	}

	QString copyright() const override
	{
		return QStringLiteral("Sahid");
	}

	const FeatureList& featureList() const override;

	bool controlFeature( Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						const ComputerControlInterfaceList& computerControlInterfaces ) override;

	bool handleFeatureMessage( VeyonServerInterface& server,
							   const MessageContext& messageContext,
							   const FeatureMessage& message ) override;

private:
	enum class FeatureCommand {
		Activate,
		Deactivate,
	};

	const Feature m_muteAudioFeature;
	const Feature m_blockUsbStorageFeature;
	const Feature m_disableWebcamFeature;
	const FeatureList m_features;

};
