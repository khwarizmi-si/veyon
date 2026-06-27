/*
 * AuvidusFeaturePlugin.cpp - implementation of AuvidusFeaturePlugin class
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

#include "AuvidusFeaturePlugin.h"
#include "DeviceController.h"
#include "ComputerControlInterface.h"
#include "VeyonServerInterface.h"


AuvidusFeaturePlugin::AuvidusFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_muteAudioFeature( Feature( QStringLiteral( "MuteAudio" ),
								 Feature::Flag::Mode | Feature::Flag::Master | Feature::Flag::Service,
								 Feature::Uid( "b1c2d3e4-5f60-4a71-8b92-1c3d5e7f9a02" ), Feature::Uid(),
								 tr( "Mute audio" ), tr( "Unmute audio" ),
								 tr( "Mute the speakers and microphone on the selected computers." ),
								 QStringLiteral(":/auvidus/audio-volume-muted.png") ) ),
	m_blockUsbStorageFeature( Feature( QStringLiteral( "BlockUsbStorage" ),
								 Feature::Flag::Mode | Feature::Flag::Master | Feature::Flag::Service,
								 Feature::Uid( "c2d3e4f5-6071-4b82-9ca3-2d4e6f8a0b13" ), Feature::Uid(),
								 tr( "Block USB storage" ), tr( "Allow USB storage" ),
								 tr( "Block access to USB storage devices on the selected computers, e.g. during exams." ),
								 QStringLiteral(":/auvidus/drive-removable-media.png") ) ),
	m_disableWebcamFeature( Feature( QStringLiteral( "DisableWebcam" ),
								 Feature::Flag::Mode | Feature::Flag::Master | Feature::Flag::Service,
								 Feature::Uid( "d3e4f506-7182-4c93-adb4-3e5f7a9b0c24" ), Feature::Uid(),
								 tr( "Disable webcam" ), tr( "Enable webcam" ),
								 tr( "Disable webcam access on the selected computers." ),
								 QStringLiteral(":/auvidus/camera-web.png") ) ),
	m_features( { m_muteAudioFeature, m_blockUsbStorageFeature, m_disableWebcamFeature } )
{
}



const FeatureList& AuvidusFeaturePlugin::featureList() const
{
	return m_features;
}



bool AuvidusFeaturePlugin::controlFeature( Feature::Uid featureUid,
										   Operation operation,
										   const QVariantMap& arguments,
										   const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED(arguments)

	if( featureUid != m_muteAudioFeature.uid() &&
		featureUid != m_blockUsbStorageFeature.uid() &&
		featureUid != m_disableWebcamFeature.uid() )
	{
		return false;
	}

	// never touch the teacher's own devices
	auto targets = computerControlInterfaces;
	targets.removeLocalHostInterfaces();

	if( operation == Operation::Start )
	{
		sendFeatureMessage( FeatureMessage{ featureUid, FeatureCommand::Activate }, targets );
		return true;
	}

	if( operation == Operation::Stop )
	{
		sendFeatureMessage( FeatureMessage{ featureUid, FeatureCommand::Deactivate }, targets );
		return true;
	}

	return false;
}



bool AuvidusFeaturePlugin::handleFeatureMessage( VeyonServerInterface& server,
												 const MessageContext& messageContext,
												 const FeatureMessage& message )
{
	Q_UNUSED(server)
	Q_UNUSED(messageContext)

	const auto featureUid = message.featureUid();
	if( featureUid != m_muteAudioFeature.uid() &&
		featureUid != m_blockUsbStorageFeature.uid() &&
		featureUid != m_disableWebcamFeature.uid() )
	{
		return false;
	}

	// the Sahid Service runs with system privileges, so the change happens here
	const bool active = ( message.command<FeatureCommand>() == FeatureCommand::Activate );

	if( featureUid == m_muteAudioFeature.uid() )
	{
		DeviceController::setAudioMuted( active );
	}
	else if( featureUid == m_blockUsbStorageFeature.uid() )
	{
		DeviceController::setUsbStorageBlocked( active );
	}
	else if( featureUid == m_disableWebcamFeature.uid() )
	{
		DeviceController::setWebcamDisabled( active );
	}

	return true;
}
