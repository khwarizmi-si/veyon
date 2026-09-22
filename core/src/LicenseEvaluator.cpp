/*
 * LicenseEvaluator.cpp - licence status from verified claims
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#include <algorithm>

#include "LicenseEvaluator.h"

// Named, not anonymous: the unity build merges translation units.
namespace LicenseEvaluatorDetail
{

struct Axis
{
	LicenseLevel level = LicenseLevel::Normal;
	QDateTime escalatesAt;
};

// `start` is the moment this axis stops being healthy.
static Axis evaluateAxis(const QDateTime& now, const QDateTime& start, int graceDays)
{
	if (now <= start)
	{
		return {};
	}
	const auto limit = start.addDays(graceDays);
	if (now <= limit)
	{
		return { LicenseLevel::Warning, limit };
	}
	return { LicenseLevel::Suspended, {} };
}

}

LicenseState LicenseEvaluator::evaluate(const std::optional<LicenseClaims>& claims,
										const QDateTime& now,
										const QDateTime& lastServerTime)
{
	using namespace LicenseEvaluatorDetail;

	if (!claims)
	{
		return { LicenseLevel::Suspended, LicenseReason::NotActivated, {} };
	}

	auto reference = claims->issuedAt;
	if (lastServerTime.isValid() && lastServerTime > reference)
	{
		reference = lastServerTime;
	}
	if (now < reference.addSecs(-ClockRollbackToleranceSecs))
	{
		return { LicenseLevel::Suspended, LicenseReason::ClockRolledBack, {} };
	}

	const auto subscription = evaluateAxis(now, claims->subscriptionEnd, SubscriptionWarningDays);
	const auto connection = evaluateAxis(now, claims->expiresAt, OfflineGraceDays);
	const auto quota = claims->overageSince.isValid()
						   ? evaluateAxis(now, claims->overageSince, OverageGraceDays)
						   : Axis{};

	const auto worst = std::max({ subscription.level, connection.level, quota.level });
	if (worst == LicenseLevel::Normal)
	{
		return {};
	}

	// Ties go to the reason the school can act on first: billing, then quota,
	// then connectivity.
	if (subscription.level == worst)
	{
		// Offline too: we cannot tell whether they paid, so never accuse them.
		const auto reason = connection.level == LicenseLevel::Normal
								? LicenseReason::Subscription
								: LicenseReason::SubscriptionUnverified;
		return { worst, reason, subscription.escalatesAt };
	}
	if (quota.level == worst)
	{
		return { worst, LicenseReason::Quota, quota.escalatesAt };
	}
	return { worst, LicenseReason::Connection, connection.escalatesAt };
}
