/*
 * ScreenRecorderSession.cpp - implementation of ScreenRecorderSession class
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

#include <QDateTime>
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QProcess>
#include <QRegularExpression>
#include <QTimer>

#include "ScreenRecorderSession.h"

namespace
{
constexpr int kFrameRate = 4; // frames per second - enough for exam monitoring, light on storage
constexpr int kMaxRecordingWidth = 1280; // cap so a high-res screen stays readable but not huge

QString buildOutputPath( const ComputerControlInterface::Pointer& computerControlInterface, const QString& directory )
{
	auto safeName = computerControlInterface->computerName();
	safeName.replace( QRegularExpression( QStringLiteral("[^A-Za-z0-9_-]") ), QStringLiteral("_") );
	if( safeName.isEmpty() )
	{
		safeName = QStringLiteral("computer");
	}

	const auto timestamp = QDateTime::currentDateTime().toString( QStringLiteral("yyyyMMdd-HHmmss") );
	return QStringLiteral("%1/%2_%3.mp4").arg( directory, safeName, timestamp );
}

QString ffmpegProgram()
{
#if defined(Q_OS_WIN)
	const auto bundledFfmpeg = QCoreApplication::applicationDirPath() + QStringLiteral("/ffmpeg.exe");
#else
	const auto bundledFfmpeg = QCoreApplication::applicationDirPath() + QStringLiteral("/ffmpeg");
#endif
	return QFileInfo::exists( bundledFfmpeg ) ? bundledFfmpeg : QStringLiteral("ffmpeg");
}
}


ScreenRecorderSession::ScreenRecorderSession( ComputerControlInterface::Pointer computerControlInterface,
											  const QString& outputDirectory, QObject* parent ) :
	QObject( parent ),
	m_computerControlInterface( computerControlInterface ),
	m_outputFilePath( buildOutputPath( computerControlInterface, outputDirectory ) ),
	m_previousUpdateMode( computerControlInterface->updateMode() ),
	m_previousScaledFramebufferSize( computerControlInterface->scaledFramebufferSize() )
{
	// request full framebuffer updates so the recording is sharp, not just thumbnail quality
	m_computerControlInterface->setUpdateMode( ComputerControlInterface::UpdateMode::Live );

	m_timer = new QTimer( this );
	m_timer->setInterval( 1000 / kFrameRate );
	connect( m_timer, &QTimer::timeout, this, &ScreenRecorderSession::captureFrame );
	m_timer->start();
}



ScreenRecorderSession::~ScreenRecorderSession()
{
	if( m_timer )
	{
		m_timer->stop();
	}

	if( m_encoder )
	{
		// flush the remaining frames and let ffmpeg finalize the file
		m_encoder->closeWriteChannel();
		m_encoder->waitForFinished( 10000 );
		if( m_encoder->state() != QProcess::NotRunning )
		{
			m_encoder->kill();
		}
	}

	// restore the master's original thumbnail size and update mode
	if( m_requestedFullResolution )
	{
		m_computerControlInterface->setScaledFramebufferSize( m_previousScaledFramebufferSize );
	}
	m_computerControlInterface->setUpdateMode( m_previousUpdateMode );
}



void ScreenRecorderSession::captureFrame()
{
	// the master decodes the framebuffer at the "scaled" size, so by default only a
	// small thumbnail is available here - request a near-full-resolution decode once
	// the real screen size is known so the recording is actually readable
	if( m_requestedFullResolution == false )
	{
		const auto screen = m_computerControlInterface->screenSize();
		if( screen.isEmpty() == false )
		{
			auto target = screen;
			if( target.width() > kMaxRecordingWidth )
			{
				target = QSize( kMaxRecordingWidth, kMaxRecordingWidth * screen.height() / screen.width() );
			}
			m_computerControlInterface->setScaledFramebufferSize( target );
			m_requestedFullResolution = true;
			return; // let the new-size frame arrive before we lock the encoder dimensions
		}
	}

	// the master decodes the framebuffer at the "scaled" size, so that is the
	// image actually available here (full framebuffer() stays empty in monitoring)
	const auto image = m_computerControlInterface->scaledFramebuffer();

	if( image.isNull() || image.width() < 2 || image.height() < 2 )
	{
		return;
	}

	if( m_encoder == nullptr )
	{
		startEncoder( image.size() );
		if( m_encoder == nullptr )
		{
			return;
		}
	}

	// ffmpeg expects a continuous stream of fixed-size raw RGB frames
	const auto frame = image.scaled( m_frameSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation )
						   .convertToFormat( QImage::Format_RGB888 );

	const int lineBytes = m_frameSize.width() * 3;
	for( int y = 0; y < m_frameSize.height(); ++y )
	{
		m_encoder->write( reinterpret_cast<const char*>( frame.constScanLine( y ) ), lineBytes );
	}
}



void ScreenRecorderSession::startEncoder( QSize frameSize )
{
	// libx264 with yuv420p requires even dimensions
	const int width = frameSize.width() - ( frameSize.width() % 2 );
	const int height = frameSize.height() - ( frameSize.height() % 2 );
	if( width < 2 || height < 2 )
	{
		return; // framebuffer not ready yet, try again on the next tick
	}

	m_frameSize = QSize( width, height );

	const auto encoder = new QProcess( this );
	encoder->setProcessChannelMode( QProcess::ForwardedErrorChannel );
	encoder->start( ffmpegProgram(),
					{
						QStringLiteral("-y"),
						QStringLiteral("-f"), QStringLiteral("rawvideo"),
						QStringLiteral("-pixel_format"), QStringLiteral("rgb24"),
						QStringLiteral("-video_size"), QStringLiteral("%1x%2").arg( width ).arg( height ),
						QStringLiteral("-framerate"), QString::number( kFrameRate ),
						QStringLiteral("-i"), QStringLiteral("-"),
						QStringLiteral("-an"),
						QStringLiteral("-c:v"), QStringLiteral("libx264"),
						QStringLiteral("-preset"), QStringLiteral("ultrafast"),
						QStringLiteral("-pix_fmt"), QStringLiteral("yuv420p"),
						m_outputFilePath,
					} );

	if( encoder->waitForStarted( 3000 ) == false )
	{
		qWarning() << "ScreenRecorderSession: failed to start ffmpeg - is it installed and in PATH?";
		delete encoder;
		return;
	}

	m_encoder = encoder;
}
