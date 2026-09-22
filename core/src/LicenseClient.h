/*
 * LicenseClient.h - HTTP client for the Khwarizmi licence backend
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#pragma once

#include <QList>
#include <QUrl>

#include "VeyonCore.h"

struct VEYON_CORE_EXPORT LicenseDevice
{
	QString mac;
	QString hostname;
};

enum class LicenseCallStatus
{
	Ok,
	InvalidCode,
	Unauthorized,
	NetworkError,
	ServerError,
	InvalidResponse,
};

struct VEYON_CORE_EXPORT ActivationResponse
{
	LicenseCallStatus status = LicenseCallStatus::NetworkError;
	QString masterId;
	QString secret;
	QString token;
};

struct VEYON_CORE_EXPORT CheckInResponse
{
	LicenseCallStatus status = LicenseCallStatus::NetworkError;
	QString token;
};

class VEYON_CORE_EXPORT LicenseClient
{
public:
	static constexpr int RequestTimeoutMsecs = 20000;

	explicit LicenseClient(const QString& serverUrl);

	ActivationResponse activate(const QString& code, const QString& hostname, const QString& version);
	CheckInResponse checkIn(const QString& masterId, const QString& secret, const QList<LicenseDevice>& devices,
							const QString& hostname, const QString& version);

	// Pure helpers, unit-tested.
	static QUrl endpoint(const QString& serverUrl, const QString& path);
	static QByteArray buildActivateBody(const QString& code, const QString& hostname, const QString& version);
	static QByteArray buildCheckInBody(const QString& masterId, const QList<LicenseDevice>& devices,
									   const QString& hostname, const QString& version);
	static ActivationResponse parseActivateResponse(int httpStatus, const QByteArray& body);
	static CheckInResponse parseCheckInResponse(int httpStatus, const QByteArray& body);

private:
	// Returns the HTTP status (0 when no response arrived) and fills `body`.
	int post(const QString& path, const QByteArray& payload, const QByteArray& bearer, QByteArray& body);

	QString m_serverUrl;
};
