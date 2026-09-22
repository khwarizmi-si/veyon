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

#include "LicenseToken.h"

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
};

QTEST_GUILESS_MAIN(LicenseTest)
#include "LicenseTest.moc"
