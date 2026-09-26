/*
 * ScreenRecorderFeaturePlugin.cpp - implementation of ScreenRecorderFeaturePlugin class
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

#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>
#include <QMessageBox>
#include <QStandardPaths>

#include "ScreenRecorderFeaturePlugin.h"
#include "ScreenRecorderSession.h"
#include "ComputerControlInterface.h"
#include "VeyonMasterInterface.h"


ScreenRecorderFeaturePlugin::ScreenRecorderFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_screenRecorderFeature( Feature( QStringLiteral( "ScreenRecorder" ),
							Feature::Flag::Mode | Feature::Flag::Master,
							Feature::Uid( "2e7b9d10-5c84-4a3f-9d61-8b0c4f7a2e95" ),
							Feature::Uid(),
							tr( "Record screens" ),
							tr( "Stop recording" ),
							tr( "Use this function to record the screens of the selected "
								"computers to video files for later evaluation." ),
							QStringLiteral(":/screenrecorder/media-record.png") ) ),
	m_features( { m_screenRecorderFeature } )
{
}



ScreenRecorderFeaturePlugin::~ScreenRecorderFeaturePlugin()
{
	stopAllSessions();
}



const FeatureList& ScreenRecorderFeaturePlugin::featureList() const
{
	return m_features;
}



bool ScreenRecorderFeaturePlugin::controlFeature( Feature::Uid featureUid, Operation operation,
												  const QVariantMap& arguments,
												  const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED(arguments)

	if( featureUid != m_screenRecorderFeature.uid() )
	{
		return false;
	}

	if( operation == Operation::Start )
	{
		return startRecording( computerControlInterfaces );
	}

	if( operation == Operation::Stop )
	{
		stopAllSessions();
		return true;
	}

	return false;
}



bool ScreenRecorderFeaturePlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
													  const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( feature.uid() != m_screenRecorderFeature.uid() )
	{
		return false;
	}
	if( !startRecording( computerControlInterfaces ) )
	{
		QMessageBox::warning( master.mainWindow(), tr("Screen recording"),
							  tr("Recording could not start. Select a connected computer and check that FFmpeg and the Videos folder are available.") );
		return false;
	}
	QMessageBox::information( master.mainWindow(), tr("Screen recording"),
						  tr("Recording requested. After stopping, look for MP4 files in %1 on this computer.").arg(m_outputDirectory) );
	return true;
}



bool ScreenRecorderFeaturePlugin::startRecording( const ComputerControlInterfaceList& computerControlInterfaces )
{
	const auto bundledEncoder = QCoreApplication::applicationDirPath()
#if defined(Q_OS_WIN)
									+ QStringLiteral("/ffmpeg.exe");
#else
									+ QStringLiteral("/ffmpeg");
#endif
	if( !QFileInfo::exists( bundledEncoder ) && QStandardPaths::findExecutable( QStringLiteral("ffmpeg") ).isEmpty() )
	{
		return false;
	}
	auto outputDirectory = QStandardPaths::writableLocation( QStandardPaths::MoviesLocation );
	if( outputDirectory.isEmpty() )
	{
		outputDirectory = QStandardPaths::writableLocation( QStandardPaths::HomeLocation );
	}
	if( outputDirectory.isEmpty() )
	{
		return false;
	}
	outputDirectory += QStringLiteral("/VeyonRecordings");
	if( !QDir().mkpath( outputDirectory ) )
	{
		return false;
	}
	m_outputDirectory = outputDirectory;

	// recording our own screen is pointless and would only capture Veyon Master itself
	auto targets = computerControlInterfaces;
	targets.removeLocalHostInterfaces();
	if( targets.isEmpty() )
	{
		return false;
	}

	bool foundTarget = false;
	for( const auto& computerControlInterface : targets )
	{
		if( computerControlInterface.isNull() ||
			computerControlInterface->state() != ComputerControlInterface::State::Connected )
		{
			continue;
		}
		foundTarget = true;
		if( m_sessions.contains( computerControlInterface.data() ) )
		{
			continue;
		}

		m_sessions.insert( computerControlInterface.data(),
						   new ScreenRecorderSession( computerControlInterface, outputDirectory, this ) );
	}
	return foundTarget;
}



void ScreenRecorderFeaturePlugin::stopAllSessions()
{
	// deleting a session flushes ffmpeg and finalizes its video file
	qDeleteAll( m_sessions );
	m_sessions.clear();
}
