/*
 * LicenseService.h - licence activation, check-in and status
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#pragma once

#include <QCoreApplication>

#include "LicenseClient.h"
#include "LicenseEvaluator.h"

enum class LicenseActionResult
{
	Ok,
	InvalidCode,
	NetworkError,
	ServerError,
	InvalidResponse,
	Unauthorized,
	NotActivated,
	ClockSkew,
	StaleToken,
	NotWritable,
	TokenRejected,
};

struct VEYON_CORE_EXPORT LicenseSnapshot
{
	LicenseState state;
	std::optional<LicenseClaims> claims;
	QString masterId;
};

// The impure edge: files, network and the clock are only touched here.
class VEYON_CORE_EXPORT LicenseService
{
	Q_DECLARE_TR_FUNCTIONS(LicenseService)
public:
	static constexpr int MaxFutureIssueSecs = 3600;

	static LicenseSnapshot snapshot();
	static LicenseActionResult activate(const QString& code);
	static LicenseActionResult checkIn(const QList<LicenseDevice>& devices);

	static QString describe(const LicenseState& state);
	static QString describe(LicenseActionResult result);

	// Pure, unit-tested.
	static LicenseActionResult acceptToken(const QString& candidate, const QString& currentToken,
										   const QString& masterId, const QDateTime& now,
										   const QMap<QString, CryptoCore::PublicKey>& keys);
};
