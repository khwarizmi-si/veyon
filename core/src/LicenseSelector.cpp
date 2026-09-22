/*
 * LicenseSelector.cpp - pick the licence token currently in force
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#include "LicenseSelector.h"

std::optional<SelectedLicense> LicenseSelector::select(const QString& activationToken,
														 const QString& cachedToken,
														 const QString& masterId,
														 const QMap<QString, CryptoCore::PublicKey>& keys)
{
	if (masterId.isEmpty())
	{
		return std::nullopt;
	}

	std::optional<SelectedLicense> activation;
	if (const auto claims = LicenseToken::verify(activationToken, keys); claims && claims->masterId == masterId)
	{
		activation = SelectedLicense{ activationToken, *claims };
	}

	std::optional<SelectedLicense> cached;
	if (const auto claims = LicenseToken::verify(cachedToken, keys); claims && claims->masterId == masterId)
	{
		// A cache left over from an earlier activation of this user profile
		// must not override the school the administrator activated.
		if (!activation || claims->tenantId == activation->claims.tenantId)
		{
			cached = SelectedLicense{ cachedToken, *claims };
		}
	}

	if (activation && cached)
	{
		return cached->claims.issuedAt > activation->claims.issuedAt ? cached : activation;
	}
	return activation ? activation : cached;
}
