/*
 * ScreenRecorderSession.h - records one computer's framebuffer to a video file
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

#include <QObject>
#include <QSize>

#include "ComputerControlInterface.h"

class QProcess;
class QTimer;

// Grabs the framebuffer of a single computer at a fixed frame rate and pipes the
// raw frames into an ffmpeg process which encodes them into an MP4 file.
class ScreenRecorderSession : public QObject
{
	Q_OBJECT
public:
	ScreenRecorderSession( ComputerControlInterface::Pointer computerControlInterface,
						   const QString& outputDirectory, QObject* parent = nullptr );
	~ScreenRecorderSession() override;

private:
	void captureFrame();
	void startEncoder( QSize frameSize );

	ComputerControlInterface::Pointer m_computerControlInterface;
	const QString m_outputFilePath;
	ComputerControlInterface::UpdateMode m_previousUpdateMode;

	QTimer* m_timer = nullptr;
	QProcess* m_encoder = nullptr;
	QSize m_frameSize;

};
