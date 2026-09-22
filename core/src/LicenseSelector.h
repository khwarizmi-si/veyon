/*
 * LicenseSelector.h - pick the licence token currently in force
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#pragma once

#include "LicenseToken.h"

struct VEYON_CORE_EXPORT SelectedLicense
{
	QString token;
	LicenseClaims claims;
};

class VEYON_CORE_EXPORT LicenseSelector
{
public:
	/**
	 * Verifies both stored tokens and returns the newest one that belongs to
	 * this installation. Raw tokens are re-verified on every call, so claims
	 * that were never signed can never reach LicenseEvaluator. Pure: never
	 * reads the clock.
	 */
	static std::optional<SelectedLicense> select(const QString& activationToken,
												 const QString& cachedToken,
												 const QString& masterId,
												 const QMap<QString, CryptoCore::PublicKey>& keys);
};
