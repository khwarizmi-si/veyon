/*
 * LicenseService.cpp - licence activation, check-in and status
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#include <QHostInfo>
#include <QLocale>

#include "LicenseSelector.h"
#include "LicenseService.h"
#include "LicenseStorage.h"

// M4: acceptToken() refuses a token whose iat is more than MaxFutureIssueSecs
// ahead of `now` (see the comment there). LicenseEvaluator treats a
// lastServerTime more than ClockRollbackToleranceSecs ahead of `now` as a
// rolled-back clock. If MaxFutureIssueSecs were ever larger than
// ClockRollbackToleranceSecs, a token acceptToken() lets through as "not from
// the future" could still exceed the evaluator's tolerance once stored,
// locking the school in ClockRolledBack until the token expires.
static_assert(LicenseService::MaxFutureIssueSecs == LicenseEvaluator::ClockRollbackToleranceSecs,
			 "MaxFutureIssueSecs must match ClockRollbackToleranceSecs, or an accepted token's iat "
			 "can exceed the evaluator's own tolerance and lock the school in ClockRolledBack");

namespace LicenseServiceDetail
{

static QDateTime parseStoredTime(const QString& value)
{
	const auto dateTime = QDateTime::fromString(value, Qt::ISODateWithMs);
	return dateTime.isValid() ? dateTime.toUTC() : QDateTime{};
}

static LicenseActionResult fromCallStatus(LicenseCallStatus status)
{
	switch (status)
	{
	case LicenseCallStatus::Ok: return LicenseActionResult::Ok;
	case LicenseCallStatus::InvalidCode: return LicenseActionResult::InvalidCode;
	case LicenseCallStatus::Unauthorized: return LicenseActionResult::Unauthorized;
	case LicenseCallStatus::NetworkError: return LicenseActionResult::NetworkError;
	case LicenseCallStatus::ServerError: return LicenseActionResult::ServerError;
	case LicenseCallStatus::InvalidResponse: return LicenseActionResult::InvalidResponse;
	}
	return LicenseActionResult::ServerError;
}

// I2: JsonStore::flush() returns void and swallows write errors. Reloading a
// fresh instance (which re-reads from disk) and comparing against what we
// meant to write is the only way this layer can detect a failed save.
static bool flushAndVerify(LicenseActivation& store)
{
	store.flushStore();
	const LicenseActivation reloaded;
	return reloaded.masterId() == store.masterId()
		&& reloaded.secret() == store.secret()
		&& reloaded.activationToken() == store.activationToken();
}

static bool flushAndVerify(LicenseCache& store)
{
	store.flushStore();
	const LicenseCache reloaded;
	return reloaded.refreshedToken() == store.refreshedToken()
		&& reloaded.lastServerTime() == store.lastServerTime();
}

}

LicenseActionResult LicenseService::acceptToken(const QString& candidate, const QString& currentToken,
												const QString& masterId, const QDateTime& now,
												const QMap<QString, CryptoCore::PublicKey>& keys)
{
	const auto claims = LicenseToken::verify(candidate, keys);
	if (!claims || masterId.isEmpty() || claims->masterId != masterId)
	{
		return LicenseActionResult::TokenRejected;
	}
	// Storing it would raise lastServerTime for good and lock the school in
	// ClockRolledBack, whichever clock is actually wrong.
	if (claims->issuedAt > now.addSecs(MaxFutureIssueSecs))
	{
		return LicenseActionResult::ClockSkew;
	}
	const auto current = LicenseToken::verify(currentToken, keys);
	if (current && current->masterId == masterId && claims->issuedAt < current->issuedAt)
	{
		return LicenseActionResult::StaleToken;
	}
	return LicenseActionResult::Ok;
}

LicenseSnapshot LicenseService::snapshot()
{
	using namespace LicenseServiceDetail;

	const LicenseActivation activation;
	const LicenseCache cache;

	LicenseSnapshot result;
	result.masterId = activation.masterId();

	const auto selected = LicenseSelector::select(activation.activationToken(), cache.refreshedToken(),
												  result.masterId, LicenseToken::productionKeys());
	if (selected)
	{
		result.claims = selected->claims;
	}
	result.state = LicenseEvaluator::evaluate(result.claims, QDateTime::currentDateTimeUtc(),
											  parseStoredTime(cache.lastServerTime()));
	return result;
}

LicenseActionResult LicenseService::activate(const QString& code)
{
	using namespace LicenseServiceDetail;

	LicenseActivation activation;
	// Check before touching the network: the code is single-use, so failing
	// to store its result would burn it for nothing.
	if (activation.isStoreWritable() == false)
	{
		return LicenseActionResult::NotWritable;
	}

	LicenseClient client(activation.serverUrl());
	const auto response = client.activate(code.trimmed(), QHostInfo::localHostName(), VeyonCore::versionString());
	if (response.status != LicenseCallStatus::Ok)
	{
		auto result = fromCallStatus(response.status);
		// M6: at this point in the flow there are no credentials to reject
		// yet, so a 401/403 here is a WAF or rate limit, not a credentials
		// rejection - reporting it as such would be misleading.
		if (result == LicenseActionResult::Unauthorized)
		{
			result = LicenseActionResult::ServerError;
		}
		return result;
	}

	// I1: the server has already consumed the single-use code. Persist
	// masterId/secret unconditionally so a retry (e.g. after fixing a wrong
	// clock) can finish the job via checkIn(), which only needs these two.
	activation.setMasterId(response.masterId);
	activation.setSecret(response.secret);

	const auto accepted = acceptToken(response.token, {}, response.masterId, QDateTime::currentDateTimeUtc(),
									  LicenseToken::productionKeys());
	if (accepted == LicenseActionResult::Ok)
	{
		activation.setActivationToken(response.token);
	}

	if (flushAndVerify(activation) == false)
	{
		return LicenseActionResult::NotWritable;
	}

	if (accepted != LicenseActionResult::Ok)
	{
		return accepted;
	}

	LicenseCache cache;
	const auto issuedAt = LicenseToken::verify(response.token, LicenseToken::productionKeys())->issuedAt;
	const auto stored = parseStoredTime(cache.lastServerTime());
	if (stored.isValid() == false || issuedAt > stored)
	{
		cache.setLastServerTime(issuedAt.toString(Qt::ISODateWithMs));
	}
	if (flushAndVerify(cache) == false)
	{
		return LicenseActionResult::NotWritable;
	}

	return LicenseActionResult::Ok;
}

LicenseActionResult LicenseService::checkIn(const QList<LicenseDevice>& devices)
{
	using namespace LicenseServiceDetail;

	LicenseActivation activation;
	if (activation.masterId().isEmpty() || activation.secret().isEmpty())
	{
		return LicenseActionResult::NotActivated;
	}

	LicenseClient client(activation.serverUrl());
	const auto response = client.checkIn(activation.masterId(), activation.secret(), devices,
										 QHostInfo::localHostName(), VeyonCore::versionString());
	// Unauthorized and network failures change nothing: the stored token keeps
	// working and the connection axis takes its course (spec section 11).
	if (response.status != LicenseCallStatus::Ok)
	{
		return fromCallStatus(response.status);
	}

	LicenseCache cache;
	const auto keys = LicenseToken::productionKeys();
	const auto current = LicenseSelector::select(activation.activationToken(), cache.refreshedToken(),
												 activation.masterId(), keys);
	const auto accepted = acceptToken(response.token, current ? current->token : QString(),
									  activation.masterId(), QDateTime::currentDateTimeUtc(), keys);
	if (accepted != LicenseActionResult::Ok)
	{
		return accepted;
	}

	// I4: lastServerTime is a per-profile rollback reference and always lives
	// in the user cache. Where the refreshed token itself goes depends on who
	// is running: if the system store is writable (the Configurator, running
	// elevated as the administrator), store it there so the administrator's
	// own page and every teacher's master pick it up. Otherwise (the master,
	// running as the teacher) store it in the user cache as before.
	const auto issuedAt = LicenseToken::verify(response.token, keys)->issuedAt;
	const auto stored = parseStoredTime(cache.lastServerTime());
	if (stored.isValid() == false || issuedAt > stored)
	{
		cache.setLastServerTime(issuedAt.toString(Qt::ISODateWithMs));
	}

	const bool activationWritable = activation.isStoreWritable();
	if (activationWritable == false)
	{
		cache.setRefreshedToken(response.token);
	}

	if (flushAndVerify(cache) == false)
	{
		return LicenseActionResult::NotWritable;
	}

	if (activationWritable)
	{
		activation.setActivationToken(response.token);
		if (flushAndVerify(activation) == false)
		{
			return LicenseActionResult::NotWritable;
		}
	}

	return LicenseActionResult::Ok;
}

QString LicenseService::describe(const LicenseState& state)
{
	const QString deadline = state.escalatesAt.isValid()
								 ? QLocale().toString(state.escalatesAt.toLocalTime(), QLocale::ShortFormat)
								 : QString();
	switch (state.reason)
	{
	case LicenseReason::None:
		return tr("The licence is active.");
	case LicenseReason::NotActivated:
		return tr("This computer has not been activated. Enter an activation code in the Khwarizmi Configurator.");
	case LicenseReason::ClockRolledBack:
		return tr("This computer's clock is behind the licence server's time. Correct the date and time, then check again.");
	case LicenseReason::Subscription:
		return state.level == LicenseLevel::Suspended
				   ? tr("The subscription has ended and the service is suspended. Please contact your school administrator.")
				   : tr("The subscription has ended. The service will be suspended on %1 unless it is renewed.").arg(deadline);
	case LicenseReason::SubscriptionUnverified:
		return state.level == LicenseLevel::Suspended
				   ? tr("We could not verify the subscription and the service is suspended. Check the internet connection, then check again.")
				   : tr("We could not verify the subscription. Check the internet connection. The service will be suspended on %1 if this continues.").arg(deadline);
	case LicenseReason::Quota:
		return state.level == LicenseLevel::Suspended
				   ? tr("The school is using more devices than its licence allows, and the service is suspended.")
				   : tr("The school is using more devices than its licence allows. The service will be suspended on %1 unless the quota is raised.").arg(deadline);
	case LicenseReason::Connection:
		return state.level == LicenseLevel::Suspended
				   ? tr("The licence server could not be reached for too long, and the service is suspended. Check the internet connection.")
				   : tr("The licence server cannot be reached. Check the internet connection. The service will be suspended on %1 if this continues.").arg(deadline);
	}
	return {};
}

QString LicenseService::describe(LicenseActionResult result)
{
	switch (result)
	{
	case LicenseActionResult::Ok: return tr("Done.");
	case LicenseActionResult::InvalidCode: return tr("The activation code is invalid, expired or already used.");
	case LicenseActionResult::NetworkError: return tr("The licence server could not be reached. Check the internet connection.");
	case LicenseActionResult::ServerError: return tr("The licence server reported an error. Please try again later.");
	case LicenseActionResult::InvalidResponse: return tr("The licence server sent an unexpected response.");
	case LicenseActionResult::Unauthorized: return tr("The licence server did not accept this computer's credentials.");
	case LicenseActionResult::NotActivated: return tr("This computer has not been activated.");
	case LicenseActionResult::ClockSkew: return tr("This computer's clock appears to be wrong. Correct the date and time, then try again.");
	case LicenseActionResult::StaleToken: return tr("The licence server returned an older licence than the one already stored.");
	case LicenseActionResult::NotWritable: return tr("The licence could not be saved. Run the Khwarizmi Configurator as an administrator.");
	case LicenseActionResult::TokenRejected: return tr("The licence returned by the server could not be verified.");
	}
	return {};
}
