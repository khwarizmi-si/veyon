/*
 * ScreenRecorderFeaturePlugin.h - declaration of ScreenRecorderFeaturePlugin class
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

#include <QHash>

#include "Feature.h"
#include "FeatureProviderInterface.h"

class ScreenRecorderSession;

class ScreenRecorderFeaturePlugin : public QObject, FeatureProviderInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.ScreenRecorder")
	Q_INTERFACES(PluginInterface FeatureProviderInterface)
public:
	explicit ScreenRecorderFeaturePlugin( QObject* parent = nullptr );
	~ScreenRecorderFeaturePlugin() override;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("9c41a7e5-2d83-4b16-bf90-3a6e1d2c8b47") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral("ScreenRecorder");
	}

	QString description() const override
	{
		return tr( "Record the screens of computers to video files" );
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

private:
	void startRecording( const ComputerControlInterfaceList& computerControlInterfaces );
	void stopAllSessions();

	const Feature m_screenRecorderFeature;
	const FeatureList m_features;

	// one active recording session per computer being recorded
	QHash<ComputerControlInterface*, ScreenRecorderSession*> m_sessions;

};
