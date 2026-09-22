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
#include <QTest>
#include <QtCrypto>

#include "LicenseEvaluator.h"
#include "LicenseToken.h"

Q_DECLARE_METATYPE(LicenseLevel)
Q_DECLARE_METATYPE(LicenseReason)

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
		QTest::addColumn<qint64>("overageAgeSecs");     // -1 = no overage; else seconds since overage_since
		QTest::addColumn<LicenseLevel>("level");
		QTest::addColumn<LicenseReason>("reason");

		const qint64 day = 86400;
		const qint64 none = -1;

		QTest::newRow("healthy")                    << 10 * day << 3 * day << none << LicenseLevel::Normal << LicenseReason::None;
		QTest::newRow("sub ends exactly now")       << 0ll      << 3 * day << none << LicenseLevel::Normal << LicenseReason::None;
		QTest::newRow("sub lapsed 1d")              << -1 * day << 3 * day << none << LicenseLevel::Warning << LicenseReason::Subscription;
		QTest::newRow("sub lapsed exactly 7d")      << -7 * day << 3 * day << none << LicenseLevel::Warning << LicenseReason::Subscription;
		QTest::newRow("sub lapsed 7d + 1s")         << -7 * day - 1 << 3 * day << none << LicenseLevel::Suspended << LicenseReason::Subscription;
		QTest::newRow("token expired 1d")           << 10 * day << -1 * day << none << LicenseLevel::Warning << LicenseReason::Connection;
		QTest::newRow("token expired exactly 14d")  << 10 * day << -14 * day << none << LicenseLevel::Warning << LicenseReason::Connection;
		QTest::newRow("token expired 14d + 1s")     << 10 * day << -14 * day - 1 << none << LicenseLevel::Suspended << LicenseReason::Connection;
		QTest::newRow("overage 1d")                 << 10 * day << 3 * day << 1 * day << LicenseLevel::Warning << LicenseReason::Quota;
		QTest::newRow("overage 14d + 1s")           << 10 * day << 3 * day << 14 * day + 1 << LicenseLevel::Suspended << LicenseReason::Quota;
		QTest::newRow("unpaid AND offline")         << -1 * day << -1 * day << none << LicenseLevel::Warning << LicenseReason::SubscriptionUnverified;
		QTest::newRow("unpaid long AND offline")    << -8 * day << -1 * day << none << LicenseLevel::Suspended << LicenseReason::SubscriptionUnverified;
		QTest::newRow("quota beats connection")     << 10 * day << -1 * day << 1 * day << LicenseLevel::Warning << LicenseReason::Quota;
		QTest::newRow("sub beats quota on a tie")   << -1 * day << 3 * day << 1 * day << LicenseLevel::Warning << LicenseReason::Subscription;
		QTest::newRow("worst level wins")           << -1 * day << 3 * day << 15 * day << LicenseLevel::Suspended << LicenseReason::Quota;
	}

	void evaluatesStatusMatrix()
	{
		QFETCH(qint64, subEndOffsetSecs);
		QFETCH(qint64, expOffsetSecs);
		QFETCH(qint64, overageAgeSecs);
		QFETCH(LicenseLevel, level);
		QFETCH(LicenseReason, reason);

		const auto now = t0().addDays(20);
		auto claims = healthyClaims();
		claims.subscriptionEnd = now.addSecs(subEndOffsetSecs);
		claims.expiresAt = now.addSecs(expOffsetSecs);
		claims.issuedAt = claims.expiresAt.addDays(-7);
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
	}

	void reportsWhenWarningEscalates()
	{
		const auto now = t0().addDays(20);
		auto claims = healthyClaims();
		claims.subscriptionEnd = now.addDays(-2);
		claims.expiresAt = now.addDays(3);
		claims.issuedAt = now.addDays(-4);
		const auto state = LicenseEvaluator::evaluate(claims, now, QDateTime());
		QCOMPARE(state.reason, LicenseReason::Subscription);
		QCOMPARE(state.escalatesAt, claims.subscriptionEnd.addDays(LicenseEvaluator::SubscriptionWarningDays));
	}

	void detectsClockRolledBackAgainstLastServerTime()
	{
		const auto claims = healthyClaims();
		const auto lastServerTime = t0().addDays(5);
		const auto rolledBack = lastServerTime.addSecs(-LicenseEvaluator::ClockRollbackToleranceSecs - 1);
		const auto state = LicenseEvaluator::evaluate(claims, rolledBack, lastServerTime);
		QCOMPARE(state.level, LicenseLevel::Suspended);
		QCOMPARE(state.reason, LicenseReason::ClockRolledBack);
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
};

QTEST_GUILESS_MAIN(LicenseTest)
#include "LicenseTest.moc"
