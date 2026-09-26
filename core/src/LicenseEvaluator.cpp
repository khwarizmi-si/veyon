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

// The overall result is only Warning when every axis reported is Normal or
// Warning, but several axes can be in Warning at once with different
// deadlines. The school must be told the earliest one, or a banner naming a
// later date could outlive the actual suspension.
static QDateTime earliestWarningLimit(std::initializer_list<Axis> axes)
{
	QDateTime earliest;
	for (const auto& axis : axes)
	{
		if (axis.level != LicenseLevel::Warning)
		{
			continue;
		}
		if (!earliest.isValid() || axis.escalatesAt < earliest)
		{
			earliest = axis.escalatesAt;
		}
	}
	return earliest;
}

}

LicenseState LicenseEvaluator::evaluate(const std::optional<LicenseClaims>& claims,
										const QDateTime& now,
										const QDateTime& lastServerTime)
{
	using namespace LicenseEvaluatorDetail;

	// An invalid `now` is a caller bug, not a licence problem: fail closed
	// instead of reporting a misleading reason such as ClockRolledBack.
	if (!now.isValid())
	{
		return { LicenseLevel::Suspended, LicenseReason::NotActivated, {} };
	}

	if (!claims)
	{
		return { LicenseLevel::Suspended, LicenseReason::NotActivated, {} };
	}

	// verify() never produces claims with invalid required dates, but the
	// struct is public, so a hand-built (e.g. default-constructed) instance
	// must still fail closed rather than feed invalid QDateTimes into the
	// comparisons below.
	if (!claims->subscriptionEnd.isValid() || !claims->issuedAt.isValid() || !claims->expiresAt.isValid())
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

	// std::max() over LicenseLevel relies on Normal < Warning < Suspended.
	static_assert(LicenseLevel::Normal < LicenseLevel::Warning && LicenseLevel::Warning < LicenseLevel::Suspended,
				 "evaluate() picks the worst axis via std::max(), which needs this declaration order");
	const auto worst = std::max({ subscription.level, connection.level, quota.level });
	if (worst == LicenseLevel::Normal)
	{
		return {};
	}

	// A Warning result may still have other axes also in Warning with an
	// earlier deadline; the school must hear the soonest one, not just the
	// winning axis's own.
	const auto escalatesAt = worst == LicenseLevel::Warning
								  ? earliestWarningLimit({ subscription, connection, quota })
								  : QDateTime{};

	// Ties go to the reason the school can act on first: billing, then quota,
	// then connectivity.
	if (subscription.level == worst)
	{
		// The token's iat is when the backend last looked at sub_end. If iat
		// is after sub_end, the backend saw the lapse and signed anyway: the
		// school really is unpaid. Otherwise this token predates the lapse
		// and cannot know about a payment made since, so we must not accuse
		// them of not paying - regardless of whether we are currently online.
		const auto reason = claims->issuedAt > claims->subscriptionEnd
								? LicenseReason::Subscription
								: LicenseReason::SubscriptionUnverified;
		return { worst, reason, escalatesAt };
	}
	if (quota.level == worst)
	{
		return { worst, LicenseReason::Quota, escalatesAt };
	}
	return { worst, LicenseReason::Connection, escalatesAt };
}

bool licenseCheckInDue(const QDateTime& lastCheckIn, const QDateTime& now)
{
	return !lastCheckIn.isValid() || now < lastCheckIn || lastCheckIn.secsTo(now) >= 24 * 60 * 60;
}

bool licenseBlocksNewSessions(const LicenseState& state)
{
	return state.level == LicenseLevel::Suspended && state.reason != LicenseReason::Quota;
}

bool licenseNeedsBanner(const LicenseState& state)
{
	return state.level != LicenseLevel::Normal;
}

QStringList licenseDevicesWithinQuota(QStringList macAddresses, int maxDevices)
{
	if (maxDevices <= 0)
	{
		return {};
	}

	for (auto& mac : macAddresses)
	{
		mac = mac.trimmed().toLower();
	}
	macAddresses.removeAll(QString());
	std::sort(macAddresses.begin(), macAddresses.end());
	macAddresses.erase(std::unique(macAddresses.begin(), macAddresses.end()), macAddresses.end());
	if (macAddresses.size() > maxDevices)
	{
		macAddresses.erase(macAddresses.begin() + maxDevices, macAddresses.end());
	}
	return macAddresses;
}
