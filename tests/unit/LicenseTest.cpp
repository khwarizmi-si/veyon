/*
 * LicenseTest.cpp - unit tests for licence token verification and evaluation
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QTest>
#include <QtCrypto>

#include "LicenseClient.h"
#include "LicenseEvaluator.h"
#include "LicenseSelector.h"
#include "LicenseService.h"
#include "LicenseToken.h"

Q_DECLARE_METATYPE(LicenseLevel)
Q_DECLARE_METATYPE(LicenseReason)
Q_DECLARE_METATYPE(LicenseCallStatus)

class LicenseTest : public QObject
{
	Q_OBJECT
private:
	QCA::Initializer m_qcaInitializer;

	static QString fixture(const QString& name)
	{
		QFile file(QStringLiteral(LICENSE_TEST_FIXTURES "/") + name);
		if (!file.open(QFile::ReadOnly)) {
			qFatal("missing fixture %s", qPrintable(name));
		}
		return QString::fromUtf8(file.readAll()).trimmed();
	}

	static QMap<QString, CryptoCore::PublicKey> testKeys()
	{
		return { { QStringLiteral("test-k1"),
				   CryptoCore::PublicKey::fromPEM(fixture(QStringLiteral("test-public.pem"))) } };
	}

	struct TsMessage
	{
		QString source;
		QString translation;
	};

	static QString readWholeFile(const QString& path)
	{
		QFile file(path);
		if (!file.open(QFile::ReadOnly)) {
			qFatal("missing file %s", qPrintable(path));
		}
		return QString::fromUtf8(file.readAll());
	}

	// Simple XML scan (no QDomDocument dependency): pulls out one <context>
	// block by <name>, then every <source>/<translation> pair inside it.
	static QList<TsMessage> messagesInContext(const QString& ts, const QString& contextName)
	{
		const QRegularExpression contextRe(
			QStringLiteral(R"(<context>\s*<name>%1</name>(.*?)</context>)").arg(QRegularExpression::escape(contextName)),
			QRegularExpression::DotMatchesEverythingOption);
		const auto contextMatch = contextRe.match(ts);
		if (!contextMatch.hasMatch())
		{
			return {};
		}

		QList<TsMessage> messages;
		const QRegularExpression messageRe(
			QStringLiteral(R"(<source>(.*?)</source>.*?<translation[^>]*>(.*?)</translation>)"),
			QRegularExpression::DotMatchesEverythingOption);
		auto it = messageRe.globalMatch(contextMatch.captured(1));
		while (it.hasNext())
		{
			const auto match = it.next();
			messages.append({ match.captured(1), match.captured(2) });
		}
		return messages;
	}

	// A school that paid through T0 + 30d, fresh token issued at T0.
	static QDateTime t0()
	{
		return QDateTime::fromString(QStringLiteral("2026-09-21T00:00:00.000Z"), Qt::ISODateWithMs);
	}

	static LicenseClaims healthyClaims()
	{
		LicenseClaims c;
		c.issuer = QStringLiteral("license.khwarizmi.co.id");
		c.masterId = QStringLiteral("mst_x");
		c.tenantId = QStringLiteral("sch_x");
		c.tenantName = QStringLiteral("X");
		c.plan = QStringLiteral("paid");
		c.maxDevices = 40;
		c.subscriptionEnd = t0().addDays(30);
		c.issuedAt = t0();
		c.expiresAt = t0().addDays(7);
		return c;
	}

private Q_SLOTS:
	void qcaSupportsRsa()
	{
		QVERIFY(QCA::isSupported("pkey"));
		QVERIFY(QCA::PKey::supportedIOTypes().contains(QCA::PKey::RSA));
	}

	void fixturePublicKeyLoads()
	{
		const auto key = QCA::PublicKey::fromPEM(fixture(QStringLiteral("test-public.pem")));
		QVERIFY(!key.isNull());
		QVERIFY(key.isRSA());
		QCOMPARE(key.bitSize(), 4096);
	}

	void verifiesBackendSignedToken()
	{
		const auto claims = LicenseToken::verify(fixture(QStringLiteral("valid.jwt")), testKeys());
		QVERIFY(claims.has_value());
		QCOMPARE(claims->issuer, QStringLiteral("license.khwarizmi.co.id"));
		QCOMPARE(claims->masterId, QStringLiteral("mst_fixture"));
		QCOMPARE(claims->tenantId, QStringLiteral("sch_fixture"));
		QCOMPARE(claims->tenantName, QStringLiteral("SMAN Fixture"));
		QCOMPARE(claims->plan, QStringLiteral("paid"));
		QCOMPARE(claims->maxDevices, 40);
		QCOMPARE(claims->subscriptionEnd, QDateTime::fromString(QStringLiteral("2026-10-19T00:00:00.000Z"), Qt::ISODateWithMs));
		QVERIFY(!claims->overageSince.isValid());
		QCOMPARE(claims->issuedAt.toSecsSinceEpoch(), qint64(1790000000));
		QCOMPARE(claims->expiresAt.toSecsSinceEpoch(), qint64(1790000000 + 7 * 86400));
	}

	void parsesOverageSince()
	{
		const auto claims = LicenseToken::verify(fixture(QStringLiteral("overage.jwt")), testKeys());
		QVERIFY(claims.has_value());
		QCOMPARE(claims->overageSince, QDateTime::fromString(QStringLiteral("2026-09-01T00:00:00.000Z"), Qt::ISODateWithMs));
	}

	void rejectsTokenSignedByAnotherKey()
	{
		QVERIFY(!LicenseToken::verify(fixture(QStringLiteral("wrong-key.jwt")), testKeys()).has_value());
	}

	void rejectsWrongIssuer()
	{
		QVERIFY(!LicenseToken::verify(fixture(QStringLiteral("wrong-issuer.jwt")), testKeys()).has_value());
	}

	void rejectsUnknownKid()
	{
		QVERIFY(!LicenseToken::verify(fixture(QStringLiteral("unknown-kid.jwt")), testKeys()).has_value());
	}

	// The HS256 header carries a genuine RS512 signature, so this proves the
	// algorithm is pinned rather than read from the token.
	void rejectsNonRs512HeaderEvenWithValidSignature()
	{
		QVERIFY(!LicenseToken::verify(fixture(QStringLiteral("hs256-header.jwt")), testKeys()).has_value());
	}

	void rejectsMissingClaim()
	{
		QVERIFY(!LicenseToken::verify(fixture(QStringLiteral("missing-field.jwt")), testKeys()).has_value());
	}

	void rejectsTamperedPayload()
	{
		auto parts = fixture(QStringLiteral("valid.jwt")).split(QLatin1Char('.'));
		QCOMPARE(parts.size(), 3);
		auto payload = QByteArray::fromBase64(parts[1].toLatin1(), QByteArray::Base64UrlEncoding);
		payload.replace("\"max_devices\":40", "\"max_devices\":9999");
		parts[1] = QString::fromLatin1(payload.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
		QVERIFY(!LicenseToken::verify(parts.join(QLatin1Char('.')), testKeys()).has_value());
	}

	void rejectsAlgNoneWithEmptySignature()
	{
		const auto parts = fixture(QStringLiteral("valid.jwt")).split(QLatin1Char('.'));
		const auto noneHeader = QByteArrayLiteral("{\"alg\":\"none\",\"typ\":\"JWT\",\"kid\":\"test-k1\"}")
			.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
		// QString, not auto: chaining this many QStringBuilder operator+ terms into
		// an `auto` keeps references to temporaries that don't survive to the
		// point of use (a documented QStringBuilder pitfall) and segfaults here.
		const QString forged = QString::fromLatin1(noneHeader) + QLatin1Char('.') + parts[1] + QLatin1Char('.');
		QVERIFY(!LicenseToken::verify(forged, testKeys()).has_value());
	}

	void rejectsMalformedInputWithoutCrashing()
	{
		const auto keys = testKeys();
		QVERIFY(!LicenseToken::verify(QString(), keys).has_value());
		QVERIFY(!LicenseToken::verify(QStringLiteral("not-a-token"), keys).has_value());
		QVERIFY(!LicenseToken::verify(QStringLiteral("a.b"), keys).has_value());
		QVERIFY(!LicenseToken::verify(QStringLiteral("!!!.!!!.!!!"), keys).has_value());
		QVERIFY(!LicenseToken::verify(QStringLiteral("a.b.c.d"), keys).has_value());
	}

	// max_devices is a whole, non-negative JSON number, but far too big to
	// cast to int without UB. verify() must reject it with an explicit upper
	// bound, not just "is a whole non-negative number".
	void rejectsHugeMaxDevices()
	{
		QVERIFY(!LicenseToken::verify(fixture(QStringLiteral("huge-max-devices.jwt")), testKeys()).has_value());
	}

	// exp is a whole, non-negative JSON number, but far too big to cast to
	// qint64 without UB. Same bounds-checking requirement as max_devices.
	void rejectsHugeExpiry()
	{
		QVERIFY(!LicenseToken::verify(fixture(QStringLiteral("huge-exp.jwt")), testKeys()).has_value());
	}

	// The payload segment is valid JSON, just not an object.
	void rejectsArrayPayload()
	{
		QVERIFY(!LicenseToken::verify(fixture(QStringLiteral("array-payload.jwt")), testKeys()).has_value());
	}

	// A caller can hand verify() a kid that resolves to a default-constructed
	// (null) key, e.g. a partially loaded key set. No fixture needed: this
	// exercises the null-key guard directly against a genuinely-signed token.
	void rejectsNullPublicKeyForKnownKid()
	{
		const QMap<QString, CryptoCore::PublicKey> keys = { { QStringLiteral("test-k1"), CryptoCore::PublicKey() } };
		QVERIFY(!LicenseToken::verify(fixture(QStringLiteral("valid.jwt")), keys).has_value());
	}

	// Expiry is the connection axis of LicenseEvaluator, not a verification
	// failure. expired.jwt's exp is in 2023, so it is in the past on any real
	// clock, yet verify() must still accept it.
	void acceptsExpiredToken()
	{
		const auto claims = LicenseToken::verify(fixture(QStringLiteral("expired.jwt")), testKeys());
		QVERIFY(claims.has_value());
		QVERIFY(claims->expiresAt < QDateTime::currentDateTimeUtc());
	}

	// sub_end without a "Z" or explicit offset would parse as local time,
	// making licence dates depend on the machine's timezone. The backend
	// always emits an offset, so this must be rejected outright.
	void rejectsSubscriptionEndWithoutTimezoneOffset()
	{
		QVERIFY(!LicenseToken::verify(fixture(QStringLiteral("no-offset-date.jwt")), testKeys()).has_value());
	}

	void productionKeyIsEmbedded()
	{
		const auto keys = LicenseToken::productionKeys();
		QVERIFY(keys.contains(QStringLiteral("k1")));
		const auto key = keys.value(QStringLiteral("k1"));
		QVERIFY(!key.isNull());
		QVERIFY(key.isRSA());
		QCOMPARE(key.bitSize(), 4096);
	}

	void evaluatesStatusMatrix_data()
	{
		QTest::addColumn<qint64>("subEndOffsetSecs");   // sub_end relative to now
		QTest::addColumn<qint64>("expOffsetSecs");      // exp relative to now
		QTest::addColumn<qint64>("iatOffsetSecs");      // iat relative to now: iat vs sub_end decides Subscription vs SubscriptionUnverified
		QTest::addColumn<qint64>("overageAgeSecs");     // -1 = no overage; else seconds since overage_since
		QTest::addColumn<LicenseLevel>("level");
		QTest::addColumn<LicenseReason>("reason");

		const qint64 day = 86400;
		const qint64 none = -1;
		const qint64 staleIat = -20 * day; // well before any sub_end used below: "the token predates the lapse"

		QTest::newRow("healthy")                    << 10 * day << 3 * day << staleIat        << none << LicenseLevel::Normal << LicenseReason::None;
		QTest::newRow("sub ends exactly now")       << 0ll      << 3 * day << staleIat        << none << LicenseLevel::Normal << LicenseReason::None;
		// iat after sub_end: backend signed this token after seeing the lapse, so it really is unpaid.
		QTest::newRow("sub lapsed 1d")              << -1 * day << 3 * day << -12 * 3600ll      << none << LicenseLevel::Warning << LicenseReason::Subscription;
		QTest::newRow("sub lapsed exactly 7d")      << -7 * day << 3 * day << -1 * day        << none << LicenseLevel::Warning << LicenseReason::Subscription;
		QTest::newRow("sub lapsed 7d + 1s")         << -7 * day - 1 << 3 * day << -1 * day    << none << LicenseLevel::Suspended << LicenseReason::Subscription;
		QTest::newRow("token expired 1d")           << 10 * day << -1 * day << staleIat        << none << LicenseLevel::Warning << LicenseReason::Connection;
		QTest::newRow("token expired exactly 14d")  << 10 * day << -14 * day << staleIat       << none << LicenseLevel::Warning << LicenseReason::Connection;
		QTest::newRow("token expired 14d + 1s")     << 10 * day << -14 * day - 1 << staleIat   << none << LicenseLevel::Suspended << LicenseReason::Connection;
		QTest::newRow("overage 1d")                 << 10 * day << 3 * day << staleIat        << 1 * day << LicenseLevel::Warning << LicenseReason::Quota;
		QTest::newRow("overage exactly 14d")        << 10 * day << 3 * day << staleIat        << 14 * day << LicenseLevel::Warning << LicenseReason::Quota;
		QTest::newRow("overage age 0")              << 10 * day << 3 * day << staleIat        << 0ll << LicenseLevel::Normal << LicenseReason::None;
		QTest::newRow("overage 14d + 1s")           << 10 * day << 3 * day << staleIat        << 14 * day + 1 << LicenseLevel::Suspended << LicenseReason::Quota;
		// iat before sub_end (stale token): cannot tell whether they paid since, regardless of connection.
		QTest::newRow("unpaid AND offline")         << -1 * day << -1 * day << staleIat        << none << LicenseLevel::Warning << LicenseReason::SubscriptionUnverified;
		QTest::newRow("unpaid long AND offline")    << -8 * day << -1 * day << staleIat        << none << LicenseLevel::Suspended << LicenseReason::SubscriptionUnverified;
		QTest::newRow("quota beats connection")     << 10 * day << -1 * day << staleIat        << 1 * day << LicenseLevel::Warning << LicenseReason::Quota;
		QTest::newRow("sub beats quota on a tie")   << -1 * day << 3 * day << -12 * 3600ll       << 1 * day << LicenseLevel::Warning << LicenseReason::Subscription;
		QTest::newRow("worst level wins")           << -1 * day << 3 * day << staleIat        << 15 * day << LicenseLevel::Suspended << LicenseReason::Quota;
		// sub and connection both Suspended, stale token: still SubscriptionUnverified, not Subscription.
		QTest::newRow("both suspended, stale token") << -10 * day << -20 * day << -15 * day   << none << LicenseLevel::Suspended << LicenseReason::SubscriptionUnverified;
		// sub only Warning but connection Suspended: level follows the worst axis, reason is Connection.
		QTest::newRow("sub warning, conn suspended") << -1 * day << -20 * day << staleIat      << none << LicenseLevel::Suspended << LicenseReason::Connection;
	}

	void evaluatesStatusMatrix()
	{
		QFETCH(qint64, subEndOffsetSecs);
		QFETCH(qint64, expOffsetSecs);
		QFETCH(qint64, iatOffsetSecs);
		QFETCH(qint64, overageAgeSecs);
		QFETCH(LicenseLevel, level);
		QFETCH(LicenseReason, reason);

		const auto now = t0().addDays(20);
		auto claims = healthyClaims();
		claims.subscriptionEnd = now.addSecs(subEndOffsetSecs);
		claims.expiresAt = now.addSecs(expOffsetSecs);
		claims.issuedAt = now.addSecs(iatOffsetSecs);
		if (overageAgeSecs >= 0)
		{
			claims.overageSince = now.addSecs(-overageAgeSecs);
		}

		const auto state = LicenseEvaluator::evaluate(claims, now, QDateTime());
		QCOMPARE(state.level, level);
		QCOMPARE(state.reason, reason);
		QCOMPARE(state.escalatesAt.isValid(), level == LicenseLevel::Warning);
	}

	void reportsNotActivatedWithoutClaims()
	{
		const auto state = LicenseEvaluator::evaluate(std::nullopt, t0(), QDateTime());
		QCOMPARE(state.level, LicenseLevel::Suspended);
		QCOMPARE(state.reason, LicenseReason::NotActivated);
		QVERIFY(!state.escalatesAt.isValid());
	}

	// An invalid `now` is a caller bug: it must fail closed rather than being
	// reported as ClockRolledBack, which would imply the licence itself is
	// the problem.
	void reportsNotActivatedWithInvalidNow()
	{
		const auto state = LicenseEvaluator::evaluate(healthyClaims(), QDateTime(), QDateTime());
		QCOMPARE(state.level, LicenseLevel::Suspended);
		QCOMPARE(state.reason, LicenseReason::NotActivated);
		QVERIFY(!state.escalatesAt.isValid());
	}

	// verify() never produces claims with invalid required dates, but
	// LicenseClaims is a public struct, so a hand-built (here: default
	// constructed) instance must still fail closed.
	void reportsNotActivatedWithInvalidClaimDates()
	{
		const LicenseClaims claims; // subscriptionEnd/issuedAt/expiresAt are all default-invalid
		const auto state = LicenseEvaluator::evaluate(claims, t0(), QDateTime());
		QCOMPARE(state.level, LicenseLevel::Suspended);
		QCOMPARE(state.reason, LicenseReason::NotActivated);
		QVERIFY(!state.escalatesAt.isValid());
	}

	void reportsWhenWarningEscalates()
	{
		const auto now = t0().addDays(20);
		auto claims = healthyClaims();
		claims.subscriptionEnd = now.addDays(-2);
		claims.expiresAt = now.addDays(3);
		claims.issuedAt = now.addDays(-1); // after sub_end: backend confirmed the lapse
		const auto state = LicenseEvaluator::evaluate(claims, now, QDateTime());
		QCOMPARE(state.reason, LicenseReason::Subscription);
		QCOMPARE(state.escalatesAt, claims.subscriptionEnd.addDays(LicenseEvaluator::SubscriptionWarningDays));
	}

	// Reviewer's exact probe: a token issued before a lapse that hasn't yet
	// hit the connection axis at all. Even though the connection is fully
	// healthy (exp is still 4 days away), the token predates sub_end, so we
	// still cannot tell whether the school has paid since.
	void reportsSubscriptionUnverifiedWhenTokenPredatesLapseEvenIfOnline()
	{
		const auto issuedAt = t0();
		const auto subscriptionEnd = issuedAt.addDays(2);
		const auto expiresAt = issuedAt.addDays(7);
		const auto now = issuedAt.addDays(3);

		auto claims = healthyClaims();
		claims.issuedAt = issuedAt;
		claims.subscriptionEnd = subscriptionEnd;
		claims.expiresAt = expiresAt;

		const auto state = LicenseEvaluator::evaluate(claims, now, QDateTime());
		QCOMPARE(state.level, LicenseLevel::Warning);
		QCOMPARE(state.reason, LicenseReason::SubscriptionUnverified);
	}

	// The bug this fix removes: subscription lapsed 1 day ago (Warning, its
	// own deadline 6 days out) while the token itself expired 13 days ago
	// (also Warning under the 14-day tolerance, deadline only 1 day out).
	// Subscription wins the reason tie, but escalatesAt must reflect the
	// soonest suspension across every Warning axis, not just the winner's.
	void escalatesAtIsEarliestAmongWarningAxes()
	{
		const auto now = t0().addDays(20);
		auto claims = healthyClaims();
		claims.subscriptionEnd = now.addDays(-1);
		claims.expiresAt = now.addDays(-13);
		claims.issuedAt = now.addSecs(-LicenseEvaluator::ClockRollbackToleranceSecs); // after sub_end and exp: confirmed lapse

		const auto state = LicenseEvaluator::evaluate(claims, now, QDateTime());
		QCOMPARE(state.level, LicenseLevel::Warning);
		QCOMPARE(state.reason, LicenseReason::Subscription);
		QCOMPARE(state.escalatesAt, claims.expiresAt.addDays(LicenseEvaluator::OfflineGraceDays));
		QVERIFY(state.escalatesAt < claims.subscriptionEnd.addDays(LicenseEvaluator::SubscriptionWarningDays));
	}

	void detectsClockRolledBackAgainstLastServerTime()
	{
		const auto claims = healthyClaims();
		const auto lastServerTime = t0().addDays(5);
		const auto rolledBack = lastServerTime.addSecs(-LicenseEvaluator::ClockRollbackToleranceSecs - 1);
		const auto state = LicenseEvaluator::evaluate(claims, rolledBack, lastServerTime);
		QCOMPARE(state.level, LicenseLevel::Suspended);
		QCOMPARE(state.reason, LicenseReason::ClockRolledBack);
		QVERIFY(!state.escalatesAt.isValid());
	}

	void toleratesSmallClockDrift()
	{
		const auto claims = healthyClaims();
		const auto lastServerTime = t0().addDays(1);
		const auto drifted = lastServerTime.addSecs(-LicenseEvaluator::ClockRollbackToleranceSecs);
		QCOMPARE(LicenseEvaluator::evaluate(claims, drifted, lastServerTime).reason, LicenseReason::None);
	}

	// A token's iat is the server's clock at issue time, so a clock set before
	// it is rolled back even when nothing was stored yet.
	void detectsClockRolledBackAgainstTokenIssueTime()
	{
		const auto claims = healthyClaims();
		const auto beforeIssue = claims.issuedAt.addSecs(-LicenseEvaluator::ClockRollbackToleranceSecs - 1);
		QCOMPARE(LicenseEvaluator::evaluate(claims, beforeIssue, QDateTime()).reason, LicenseReason::ClockRolledBack);
	}

	// lastServerTime older than iat must not relax the check: the reference
	// stays iat, so a `now` just over the tolerance before iat is rolled back
	// even though it is comfortably after lastServerTime.
	void usesIssuedAtAsReferenceWhenLastServerTimeIsOlder()
	{
		const auto claims = healthyClaims();
		const auto lastServerTime = claims.issuedAt.addDays(-5);
		const auto now = claims.issuedAt.addSecs(-LicenseEvaluator::ClockRollbackToleranceSecs - 1);
		QVERIFY(now > lastServerTime);
		const auto state = LicenseEvaluator::evaluate(claims, now, lastServerTime);
		QCOMPARE(state.reason, LicenseReason::ClockRolledBack);
	}

	void selectsNothingWithoutMasterId()
	{
		QVERIFY(!LicenseSelector::select(fixture(QStringLiteral("valid.jwt")), {}, QString(), testKeys()).has_value());
	}

	void selectsActivationTokenWhenCacheIsEmpty()
	{
		const auto selected = LicenseSelector::select(fixture(QStringLiteral("valid.jwt")), {},
													  QStringLiteral("mst_fixture"), testKeys());
		QVERIFY(selected.has_value());
		QCOMPARE(selected->claims.issuedAt.toSecsSinceEpoch(), qint64(1790000000));
	}

	void prefersNewerCachedToken()
	{
		const auto selected = LicenseSelector::select(fixture(QStringLiteral("valid.jwt")),
													  fixture(QStringLiteral("newer.jwt")),
													  QStringLiteral("mst_fixture"), testKeys());
		QVERIFY(selected.has_value());
		QCOMPARE(selected->token, fixture(QStringLiteral("newer.jwt")));
	}

	void keepsNewerActivationOverOlderCache()
	{
		const auto selected = LicenseSelector::select(fixture(QStringLiteral("newer.jwt")),
													  fixture(QStringLiteral("valid.jwt")),
													  QStringLiteral("mst_fixture"), testKeys());
		QCOMPARE(selected->token, fixture(QStringLiteral("newer.jwt")));
	}

	void rejectsTokenForAnotherMaster()
	{
		const auto selected = LicenseSelector::select(fixture(QStringLiteral("valid.jwt")),
													  fixture(QStringLiteral("other-master.jwt")),
													  QStringLiteral("mst_fixture"), testKeys());
		QCOMPARE(selected->token, fixture(QStringLiteral("valid.jwt")));
	}

	void rejectsCacheFromAnotherTenant()
	{
		const auto selected = LicenseSelector::select(fixture(QStringLiteral("valid.jwt")),
													  fixture(QStringLiteral("other-tenant.jwt")),
													  QStringLiteral("mst_fixture"), testKeys());
		QCOMPARE(selected->token, fixture(QStringLiteral("valid.jwt")));
	}

	void fallsBackToCacheWhenActivationTokenIsCorrupt()
	{
		const auto selected = LicenseSelector::select(QStringLiteral("garbage"),
													  fixture(QStringLiteral("newer.jwt")),
													  QStringLiteral("mst_fixture"), testKeys());
		QVERIFY(selected.has_value());
		QCOMPARE(selected->token, fixture(QStringLiteral("newer.jwt")));
	}

	void selectsNothingWhenNoTokenVerifies()
	{
		QVERIFY(!LicenseSelector::select(fixture(QStringLiteral("wrong-key.jwt")), QStringLiteral("x.y.z"),
										 QStringLiteral("mst_fixture"), testKeys()).has_value());
	}

	void checkInBodyCarriesOnlyLicenceFields()
	{
		const QByteArray body = LicenseClient::buildCheckInBody(
			QStringLiteral("mst_1"),
			{ { QStringLiteral("aa:bb:cc:dd:ee:ff"), QStringLiteral("LAB1-PC01") } },
			QStringLiteral("guru-pc"), QStringLiteral("4.11.0"));
		const auto object = QJsonDocument::fromJson(body).object();

		auto keys = object.keys();
		keys.sort();
		// INVARIANT: the licence path is sterile - never monitoring data.
		QCOMPARE(keys, QStringList({ QStringLiteral("devices"), QStringLiteral("hostname"),
									 QStringLiteral("master_id"), QStringLiteral("version") }));

		const auto device = object.value(QStringLiteral("devices")).toArray().at(0).toObject();
		auto deviceKeys = device.keys();
		deviceKeys.sort();
		QCOMPARE(deviceKeys, QStringList({ QStringLiteral("hostname"), QStringLiteral("mac") }));
		QCOMPARE(object.value(QStringLiteral("master_id")).toString(), QStringLiteral("mst_1"));
	}

	void checkInBodyNeverContainsTheSecret()
	{
		const QByteArray body = LicenseClient::buildCheckInBody(QStringLiteral("mst_1"), {},
																QStringLiteral("h"), QStringLiteral("v"));
		QVERIFY(!body.contains("secret"));
	}

	void activateBodyHasCodeHostnameVersion()
	{
		const auto object = QJsonDocument::fromJson(
			LicenseClient::buildActivateBody(QStringLiteral("KHW-ABC"), QStringLiteral("guru-pc"), QStringLiteral("4.11.0"))).object();
		auto keys = object.keys();
		keys.sort();
		QCOMPARE(keys, QStringList({ QStringLiteral("code"), QStringLiteral("hostname"), QStringLiteral("version") }));
	}

	void parsesSuccessfulActivation()
	{
		const auto response = LicenseClient::parseActivateResponse(
			200, R"({"master_id":"mst_1","secret":"s3cr3t","token":"a.b.c"})");
		QCOMPARE(response.status, LicenseCallStatus::Ok);
		QCOMPARE(response.masterId, QStringLiteral("mst_1"));
		QCOMPARE(response.secret, QStringLiteral("s3cr3t"));
		QCOMPARE(response.token, QStringLiteral("a.b.c"));
	}

	void mapsActivationErrors_data()
	{
		QTest::addColumn<int>("httpStatus");
		QTest::addColumn<QByteArray>("body");
		QTest::addColumn<LicenseCallStatus>("expected");

		QTest::newRow("used code")       << 400 << QByteArray(R"({"error":"invalid_or_used_code"})") << LicenseCallStatus::InvalidCode;
		QTest::newRow("bad request")     << 400 << QByteArray(R"({"error":"invalid_request"})") << LicenseCallStatus::ServerError;
		QTest::newRow("server error")    << 500 << QByteArray(R"({"error":"internal_error"})") << LicenseCallStatus::ServerError;
		QTest::newRow("no status")       << 0   << QByteArray() << LicenseCallStatus::NetworkError;
		QTest::newRow("200 not json")    << 200 << QByteArray("<html>") << LicenseCallStatus::InvalidResponse;
		QTest::newRow("200 missing key") << 200 << QByteArray(R"({"master_id":"mst_1","token":"a.b.c"})") << LicenseCallStatus::InvalidResponse;
		// I3: a redirect must never be followed; it is surfaced as a server error.
		QTest::newRow("redirect")        << 302 << QByteArray() << LicenseCallStatus::ServerError;
	}

	void mapsActivationErrors()
	{
		QFETCH(int, httpStatus);
		QFETCH(QByteArray, body);
		QFETCH(LicenseCallStatus, expected);
		QCOMPARE(LicenseClient::parseActivateResponse(httpStatus, body).status, expected);
	}

	void mapsCheckInStatuses_data()
	{
		QTest::addColumn<int>("httpStatus");
		QTest::addColumn<QByteArray>("body");
		QTest::addColumn<LicenseCallStatus>("expected");

		QTest::newRow("ok")           << 200 << QByteArray(R"({"token":"a.b.c"})") << LicenseCallStatus::Ok;
		QTest::newRow("unauthorized") << 401 << QByteArray(R"({"error":"unauthorized"})") << LicenseCallStatus::Unauthorized;
		QTest::newRow("forbidden")    << 403 << QByteArray() << LicenseCallStatus::Unauthorized;
		QTest::newRow("gateway")      << 502 << QByteArray() << LicenseCallStatus::ServerError;
		QTest::newRow("no status")    << 0   << QByteArray() << LicenseCallStatus::NetworkError;
		QTest::newRow("no token")     << 200 << QByteArray(R"({})") << LicenseCallStatus::InvalidResponse;
		QTest::newRow("redirect")     << 302 << QByteArray() << LicenseCallStatus::ServerError;
	}

	void mapsCheckInStatuses()
	{
		QFETCH(int, httpStatus);
		QFETCH(QByteArray, body);
		QFETCH(LicenseCallStatus, expected);
		QCOMPARE(LicenseClient::parseCheckInResponse(httpStatus, body).status, expected);
	}

	void refusesNonHttpsEndpoint()
	{
		QVERIFY(!LicenseClient::endpoint(QStringLiteral("http://license.khwarizmi.co.id"), QStringLiteral("/v1/activate")).isValid());
		QCOMPARE(LicenseClient::endpoint(QStringLiteral("https://license.khwarizmi.co.id/"), QStringLiteral("/v1/activate")),
				 QUrl(QStringLiteral("https://license.khwarizmi.co.id/v1/activate")));
	}

	// I3: a redirect must never carry the bearer secret to another host.
	void buildRequestSetsBearerHeaderWithSpace()
	{
		const auto request = LicenseClient::buildRequest(
			QUrl(QStringLiteral("https://license.khwarizmi.co.id/v1/checkin")), QByteArrayLiteral("abc"));
		QCOMPARE(request.rawHeader("Authorization"), QByteArray("Bearer abc"));
	}

	void buildRequestOmitsAuthorizationForEmptyBearer()
	{
		const auto request = LicenseClient::buildRequest(
			QUrl(QStringLiteral("https://license.khwarizmi.co.id/v1/activate")), QByteArray());
		QVERIFY(!request.hasRawHeader("Authorization"));
	}

	// Bite-proof: remove the RedirectPolicyAttribute line in buildRequest()
	// and this fails.
	void buildRequestNeverFollowsRedirects()
	{
		const auto request = LicenseClient::buildRequest(
			QUrl(QStringLiteral("https://license.khwarizmi.co.id/v1/activate")), QByteArray());
		const auto attribute = request.attribute(QNetworkRequest::RedirectPolicyAttribute);
		// An absent attribute reads back as an invalid QVariant whose toInt()
		// coincidentally equals ManualRedirectPolicy's value (0) - checking
		// isValid() first is what actually makes this bite-proof.
		QVERIFY2(attribute.isValid(), "RedirectPolicyAttribute was never set");
		QCOMPARE(attribute.toInt(), static_cast<int>(QNetworkRequest::ManualRedirectPolicy));
	}

	void acceptsFirstValidToken()
	{
		const auto now = QDateTime::fromSecsSinceEpoch(1790000000 + 60).toUTC();
		QCOMPARE(LicenseService::acceptToken(fixture(QStringLiteral("valid.jwt")), {}, QStringLiteral("mst_fixture"), now, testKeys()),
				 LicenseActionResult::Ok);
	}

	void rejectsTokenForAnotherInstallation()
	{
		const auto now = QDateTime::fromSecsSinceEpoch(1790000000 + 86400 + 60).toUTC();
		QCOMPARE(LicenseService::acceptToken(fixture(QStringLiteral("other-master.jwt")), {}, QStringLiteral("mst_fixture"), now, testKeys()),
				 LicenseActionResult::TokenRejected);
	}

	void rejectsReplayOfOlderToken()
	{
		const auto now = QDateTime::fromSecsSinceEpoch(1790000000 + 86400 + 60).toUTC();
		QCOMPARE(LicenseService::acceptToken(fixture(QStringLiteral("valid.jwt")), fixture(QStringLiteral("newer.jwt")),
											 QStringLiteral("mst_fixture"), now, testKeys()),
				 LicenseActionResult::StaleToken);
	}

	// A token issued more than an hour "in the future" must not be stored:
	// its iat would permanently raise lastServerTime and lock the school
	// in ClockRolledBack.
	void refusesTokenFromTheFuture()
	{
		const auto now = QDateTime::fromSecsSinceEpoch(1790000000 - 3601).toUTC();
		QCOMPARE(LicenseService::acceptToken(fixture(QStringLiteral("valid.jwt")), {}, QStringLiteral("mst_fixture"), now, testKeys()),
				 LicenseActionResult::ClockSkew);
	}

	// Boundary: iat exactly now + MaxFutureIssueSecs must still be accepted.
	// Bite-proof: change the acceptToken() comparison from `>` to `>=` and
	// this fails.
	void acceptsTokenAtExactFutureBoundary()
	{
		const auto now = QDateTime::fromSecsSinceEpoch(1790000000 - LicenseService::MaxFutureIssueSecs).toUTC();
		QCOMPARE(LicenseService::acceptToken(fixture(QStringLiteral("valid.jwt")), {}, QStringLiteral("mst_fixture"), now, testKeys()),
				 LicenseActionResult::Ok);
	}

	// Re-delivery of the same token the caller already holds must not be
	// treated as stale.
	void reDeliveryOfTheSameTokenIsOk()
	{
		const auto now = QDateTime::fromSecsSinceEpoch(1790000000 + 60).toUTC();
		const auto token = fixture(QStringLiteral("valid.jwt"));
		QCOMPARE(LicenseService::acceptToken(token, token, QStringLiteral("mst_fixture"), now, testKeys()),
				 LicenseActionResult::Ok);
	}

	// A currentToken belonging to another master (e.g. a stale cache from a
	// previous activation) must never make a genuine candidate look stale.
	void currentTokenForAnotherMasterIsIgnored()
	{
		const auto now = QDateTime::fromSecsSinceEpoch(1790000000 + 60).toUTC();
		QCOMPARE(LicenseService::acceptToken(fixture(QStringLiteral("valid.jwt")), fixture(QStringLiteral("other-master.jwt")),
											 QStringLiteral("mst_fixture"), now, testKeys()),
				 LicenseActionResult::Ok);
	}

	void describesEveryReason()
	{
		const QList<LicenseReason> reasons{ LicenseReason::None, LicenseReason::NotActivated, LicenseReason::ClockRolledBack,
											LicenseReason::Subscription, LicenseReason::SubscriptionUnverified,
											LicenseReason::Quota, LicenseReason::Connection };
		for (const auto reason : reasons)
		{
			LicenseState state;
			state.reason = reason;
			state.level = reason == LicenseReason::None ? LicenseLevel::Normal : LicenseLevel::Warning;
			QVERIFY2(!LicenseService::describe(state).isEmpty(), "every reason needs a message");
		}
	}

	// SubscriptionUnverified must never read like an accusation.
	void unverifiedMessageDoesNotClaimNonPayment()
	{
		LicenseState state;
		state.level = LicenseLevel::Warning;
		state.reason = LicenseReason::SubscriptionUnverified;
		const auto message = LicenseService::describe(state).toLower();
		QVERIFY(message.contains(QStringLiteral("verify")));
		QVERIFY(!message.contains(QStringLiteral("has not been paid")));

		// Same invariant for the Indonesian translation, loaded straight from
		// the source tree so a bad translation cannot slip past review.
		const auto ts = readWholeFile(QStringLiteral(VEYON_ID_TS_PATH));
		const auto messages = messagesInContext(ts, QStringLiteral("LicenseService"));
		QVERIFY2(!messages.isEmpty(), "LicenseService context missing or empty in veyon_id.ts");

		bool checkedAny = false;
		for (const auto& message : messages)
		{
			if (!message.source.contains(QStringLiteral("verify the subscription")))
			{
				continue;
			}
			checkedAny = true;
			const auto translation = message.translation.toLower();
			QVERIFY2(!translation.contains(QStringLiteral("belum membayar")),
					 qPrintable(QStringLiteral("translation claims non-payment: ") + message.translation));
			QVERIFY2(!translation.contains(QStringLiteral("belum bayar")),
					 qPrintable(QStringLiteral("translation claims non-payment: ") + message.translation));
		}
		QVERIFY2(checkedAny, "no LicenseService source mentions 'verify the subscription'");
	}

	// Every source string actually used by these two contexts must have a
	// translation, so the UI never falls back to raw English by accident.
	void everyLicenseSourceHasATranslation()
	{
		const auto ts = readWholeFile(QStringLiteral(VEYON_ID_TS_PATH));
		for (const auto& contextName : { QStringLiteral("LicenseService"), QStringLiteral("LicensePage") })
		{
			const auto messages = messagesInContext(ts, contextName);
			QVERIFY2(!messages.isEmpty(), qPrintable(QStringLiteral("context missing or empty: ") + contextName));
			for (const auto& message : messages)
			{
				QVERIFY2(!message.translation.trimmed().isEmpty(),
						 qPrintable(QStringLiteral("empty translation for: ") + message.source));
			}
		}
	}
};

QTEST_GUILESS_MAIN(LicenseTest)
#include "LicenseTest.moc"
