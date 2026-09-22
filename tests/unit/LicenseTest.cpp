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
};

QTEST_GUILESS_MAIN(LicenseTest)
#include "LicenseTest.moc"
