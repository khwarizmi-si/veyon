/*
 * LicenseEvaluator.h - licence status from verified claims
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

// Declaration order is load-bearing: LicenseEvaluator::evaluate() picks the
// worst axis with std::max() over this enum, which relies on
// Normal < Warning < Suspended.
enum class LicenseLevel
{
	Normal,
	Warning,
	Suspended,
};

enum class LicenseReason
{
	None,
	NotActivated,
	ClockRolledBack,
	Subscription,
	SubscriptionUnverified, // the token predates the lapse, so we cannot know whether they paid
	Quota,
	Connection,
};

struct VEYON_CORE_EXPORT LicenseState
{
	LicenseLevel level = LicenseLevel::Normal;
	LicenseReason reason = LicenseReason::None;
	QDateTime escalatesAt; // when a Warning becomes Suspended; invalid otherwise
};

class VEYON_CORE_EXPORT LicenseEvaluator
{
public:
	static constexpr int SubscriptionWarningDays = 7;
	static constexpr int OfflineGraceDays = 14;
	static constexpr int OverageGraceDays = 14;
	static constexpr int ClockRollbackToleranceSecs = 3600;

	/**
	 * Pure: never reads the clock. The caller passes `now` and the latest
	 * server time it has seen, so every branch is testable as a table.
	 */
	static LicenseState evaluate(const std::optional<LicenseClaims>& claims,
								 const QDateTime& now,
								 const QDateTime& lastServerTime);
};

VEYON_CORE_EXPORT bool licenseCheckInDue(const QDateTime& lastCheckIn, const QDateTime& now);
VEYON_CORE_EXPORT bool licenseBlocksNewSessions(const LicenseState& state);
VEYON_CORE_EXPORT bool licenseNeedsBanner(const LicenseState& state);
VEYON_CORE_EXPORT QStringList licenseDevicesWithinQuota(QStringList macAddresses, int maxDevices);
