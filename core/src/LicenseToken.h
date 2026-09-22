/*
 * LicenseToken.h - verification of Khwarizmi licence tokens
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#pragma once

#include <optional>

#include <QDateTime>
#include <QMap>
#include <QString>

#include "CryptoCore.h"

struct VEYON_CORE_EXPORT LicenseClaims
{
	QString issuer;
	QString masterId;
	QString tenantId;
	QString tenantName;
	QString plan;
	int maxDevices = 0;
	QDateTime subscriptionEnd;
	QDateTime overageSince; // invalid when the school is within quota
	QDateTime issuedAt;
	QDateTime expiresAt;
};

class VEYON_CORE_EXPORT LicenseToken
{
public:
	/**
	 * Verifies an RS512 licence token and returns its claims.
	 *
	 * Deliberately does NOT reject an expired token: expiry is the connection
	 * axis of LicenseEvaluator. Rejecting it here would turn a school whose
	 * internet is down into "not activated". This function never reads the
	 * clock.
	 */
	static std::optional<LicenseClaims> verify(const QString& token,
											   const QMap<QString, CryptoCore::PublicKey>& keysByKid);

	static QMap<QString, CryptoCore::PublicKey> productionKeys();
};
