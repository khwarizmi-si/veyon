/*
 * DeviceController.cpp - implementation of DeviceController
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

#include <QtGlobal>

#include "DeviceController.h"

#if defined(Q_OS_WIN)

#include <QSettings>

#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>

namespace
{

// USBSTOR\Start: 3 = enabled (default), 4 = disabled
bool writeUsbStorageStart( int value )
{
	QSettings reg( QStringLiteral("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\USBSTOR"),
				   QSettings::NativeFormat );
	reg.setValue( QStringLiteral("Start"), value );
	reg.sync();
	return reg.status() == QSettings::NoError;
}


// global camera access consent: "Allow" / "Deny"
bool writeWebcamConsent( const QString& value )
{
	QSettings reg( QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion"
								  "\\CapabilityAccessManager\\ConsentStore\\webcam"),
				   QSettings::NativeFormat );
	reg.setValue( QStringLiteral("Value"), value );
	reg.sync();
	return reg.status() == QSettings::NoError;
}


// mute/unmute the default endpoint of the given data flow via Core Audio
bool muteAudioEndpoint( EDataFlow dataFlow, bool muted )
{
	bool ok = false;

	const auto coInit = CoInitializeEx( nullptr, COINIT_APARTMENTTHREADED );
	const bool needUninit = ( coInit == S_OK || coInit == S_FALSE );

	IMMDeviceEnumerator* enumerator = nullptr;
	if( CoCreateInstance( __uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
						  __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>( &enumerator ) ) == S_OK && enumerator )
	{
		IMMDevice* device = nullptr;
		if( enumerator->GetDefaultAudioEndpoint( dataFlow, eConsole, &device ) == S_OK && device )
		{
			IAudioEndpointVolume* endpointVolume = nullptr;
			if( device->Activate( __uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr,
								   reinterpret_cast<void**>( &endpointVolume ) ) == S_OK && endpointVolume )
			{
				ok = ( endpointVolume->SetMute( muted ? TRUE : FALSE, nullptr ) == S_OK );
				endpointVolume->Release();
			}
			device->Release();
		}
		enumerator->Release();
	}

	if( needUninit )
	{
		CoUninitialize();
	}

	return ok;
}

}


bool DeviceController::setUsbStorageBlocked( bool blocked )
{
	return writeUsbStorageStart( blocked ? 4 : 3 );
}


bool DeviceController::setWebcamDisabled( bool disabled )
{
	return writeWebcamConsent( disabled ? QStringLiteral("Deny") : QStringLiteral("Allow") );
}


bool DeviceController::setAudioMuted( bool muted )
{
	// mute both speakers (render) and microphone (capture)
	const bool renderOk = muteAudioEndpoint( eRender, muted );
	const bool captureOk = muteAudioEndpoint( eCapture, muted );
	return renderOk && captureOk;
}

#else // non-Windows: no device control available, treat as a successful no-op

bool DeviceController::setUsbStorageBlocked( bool ) { return true; }
bool DeviceController::setWebcamDisabled( bool ) { return true; }
bool DeviceController::setAudioMuted( bool ) { return true; }

#endif
