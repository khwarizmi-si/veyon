# Klien Lisensi 2a — Inti `LicenseToken` & `LicenseEvaluator`

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Logika lisensi murni di `veyon-core` — memverifikasi token RS512 dari backend dengan public key tertanam, lalu menghitung status lisensi tiga sumbu — lengkap dengan unit test QTest.

**Architecture:** Dua unit tanpa I/O dan tanpa singleton: `LicenseToken` (parse + verifikasi tanda tangan → `LicenseClaims`) dan `LicenseEvaluator` (claims + waktu sekarang + waktu-server terakhir → `LicenseState`). Waktu selalu disuntikkan, tidak pernah dibaca sendiri, sehingga seluruh matriks status bisa diuji sebagai tabel. Jaringan, penyimpanan, dan UI adalah subsistem 2b/2c.

**Tech Stack:** C++20, Qt 5.15 (container dev) **dan** Qt 6 (build Windows produksi), QCA, QTest, CMake.

**Spec:** `docs/superpowers/specs/2026-09-19-khwarizmi-saas-licensing-design.md` §5, §8, §12.

---

## Lingkungan

Build dan test berjalan di container Docker `veyon-dev` (Ubuntu 22.04, Qt 5.15.3, QCA 2.3.4). Repo ter-mount di `/veyon`.

```bash
docker exec veyon-dev bash -lc 'cd /veyon/build-tests && cmake .. && make -j10 license-test && ./tests/unit/license-test'
```

`build-tests/` dikonfigurasi sekali dengan
`cmake -DWITH_QT6=OFF -DWITH_BUNDLED_LIBVNC=ON -DWITH_TESTS=ON -DWITH_TRANSLATIONS=OFF ..`.
Baseline `veyon-core` sudah terbukti ter-build bersih di sana.

Branch: `feat/license-client` di repo veyon.

## Global Constraints

Berlaku untuk **setiap** task.

- **Kompatibel Qt 5 dan Qt 6.** Container memakai Qt 5.15; build Windows produksi memakai Qt 6. Jangan pakai API yang hanya ada di salah satunya. Secara khusus: **jangan `QJsonValue::toInteger()`** (Qt 6 saja) — pakai `toDouble()`; **jangan overload `QDateTime` yang menerima `Qt::TimeSpec`** — pakai `fromSecsSinceEpoch(s).toUTC()`.
- **Unity build aktif** (`WITH_UNITY_BUILD=ON`): beberapa `.cpp` digabung jadi satu unit compile. Fungsi helper wajib di **namespace bernama** (`LicenseTokenDetail`, `LicenseEvaluatorDetail`), **bukan** namespace anonim.
- **Algoritma token RS512** = `RSASSA-PKCS1-v1_5` + SHA-512 = `QCA::EMSA3_SHA512` (`CryptoCore::DefaultSignatureAlgorithm`). Header wajib `alg: "RS512"` dan `kid` yang dikenal. **Algoritma verifikasi dikunci, tidak pernah dipilih dari header token.**
- **Issuer:** `license.khwarizmi.co.id`.
- **Klaim token tepat sepuluh:** `iss, sub, tenant, tenant_name, plan, max_devices, sub_end, overage_since, iat, exp`. `iat`/`exp` angka detik Unix; `sub_end`/`overage_since` string ISO-8601 UTC; `overage_since` boleh `null` tetapi kuncinya wajib ada. Tidak ada klaim `status`.
- **`verify()` TIDAK memeriksa kedaluwarsa.** Token yang `exp`-nya lewat tetap valid secara kriptografis — kedaluwarsa adalah sumbu koneksi di `LicenseEvaluator`, bukan kegagalan verifikasi. Menolaknya di `verify()` akan memperlakukan sekolah yang internetnya mati sebagai "belum aktivasi".
- **Dua tanggal tidak boleh dicampur:** `exp` = masalah koneksi (toleransi 14 hari); `sub_end` = masalah tagihan (peringatan 7 hari lalu suspend).
- **Masa tenggang:** peringatan langganan 7 hari, toleransi offline 14 hari, tenggat kelebihan kuota 14 hari, toleransi jam mundur 3600 detik.
- **Prioritas alasan saat level setara:** langganan → kuota → koneksi.
- **Sekolah yang sudah bayar tapi offline tidak boleh dituduh menunggak:** bila sumbu langganan bermasalah **dan** sumbu koneksi juga bermasalah, alasannya `SubscriptionUnverified`, bukan `Subscription`.
- Semua kelas publik diberi `VEYON_CORE_EXPORT` (build produksi memakai visibility hidden).
- Gaya kode mengikuti Veyon: tab untuk indentasi, `QStringLiteral`, header lisensi GPL di tiap file baru.

---

### Task 1: Harness QTest, fixture dari kode backend, dan smoke test QCA

**Files:**
- Create: `/Volumes/rherdians/programming/alkhwarizmi/khwarizmi-license/scripts/generate-client-fixtures.ts`
- Create: `tests/unit/CMakeLists.txt`
- Create: `tests/unit/LicenseTest.cpp`
- Create: `tests/unit/fixtures/README.md` (+ file fixture yang dihasilkan)
- Modify: `tests/CMakeLists.txt`
- Modify: `.gitignore`

**Interfaces:**
- Produces: target `license-test`; direktori fixture via definisi `LICENSE_TEST_FIXTURES`; file `test-public.pem`, `valid.jwt`, `expired.jwt`, `overage.jwt`, `wrong-key.jwt`, `wrong-issuer.jwt`, `unknown-kid.jwt`, `hs256-header.jwt`, `missing-field.jwt`.

Fixture dibuat oleh **kode penandatangan backend yang sama dengan produksi** (`signToken` di repo `khwarizmi-license`), memakai keypair uji sekali-pakai. Itu yang membuktikan interoperabilitas lintas bahasa: token yang dihasilkan TypeScript/WebCrypto harus lolos verifikasi QCA di C++.

- [ ] **Step 1: Tulis generator fixture di repo lisensi**

`/Volumes/rherdians/programming/alkhwarizmi/khwarizmi-license/scripts/generate-client-fixtures.ts`:

```typescript
/**
 * Menghasilkan fixture token untuk unit test klien Veyon (C++/QCA).
 * Memakai signToken() yang sama dengan produksi, dengan keypair uji
 * sekali-pakai — bukan kunci produksi.
 *
 * Jalankan: npx tsx scripts/generate-client-fixtures.ts <direktori-keluaran>
 */
import { mkdirSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';
import { generateTestKeyPair, importPrivateKey, signToken } from '../src/token';
import type { TokenPayload } from '../src/types';

const out = process.argv[2];
if (!out) {
  console.error('usage: npx tsx scripts/generate-client-fixtures.ts <out-dir>');
  process.exit(1);
}
mkdirSync(out, { recursive: true });

const ALG = { name: 'RSASSA-PKCS1-v1_5', hash: 'SHA-512' } as const;

function b64url(bytes: Uint8Array): string {
  let s = '';
  for (const b of bytes) s += String.fromCharCode(b);
  return btoa(s).replace(/\+/g, '-').replace(/\//g, '_').replace(/=+$/, '');
}

/** Menandatangani header arbitrer — untuk kasus yang tidak bisa dibuat signToken(). */
async function signWithHeader(header: object, payload: object, key: CryptoKey): Promise<string> {
  const enc = new TextEncoder();
  const h = b64url(enc.encode(JSON.stringify(header)));
  const p = b64url(enc.encode(JSON.stringify(payload)));
  const sig = await crypto.subtle.sign(ALG, key, enc.encode(`${h}.${p}`));
  return `${h}.${p}.${b64url(new Uint8Array(sig))}`;
}

const main = await generateTestKeyPair();
const other = await generateTestKeyPair();
const mainKey = await importPrivateKey(main.privatePem);
const otherKey = await importPrivateKey(other.privatePem);

const base: TokenPayload = {
  iss: 'license.khwarizmi.co.id',
  sub: 'mst_fixture',
  tenant: 'sch_fixture',
  tenant_name: 'SMAN Fixture',
  plan: 'paid',
  max_devices: 40,
  sub_end: '2026-10-19T00:00:00.000Z',
  overage_since: null,
  iat: 1790000000,
  exp: 1790000000 + 7 * 86400,
};

const { max_devices: _omitted, ...withoutMaxDevices } = base;

const files: Record<string, string> = {
  'test-public.pem': main.publicPem + '\n',
  'valid.jwt': await signToken(base, mainKey, 'test-k1'),
  // iat in 2023: exp is in the past on any clock this test will ever run on.
  'expired.jwt': await signToken({ ...base, iat: 1700000000, exp: 1700000000 + 7 * 86400 }, mainKey, 'test-k1'),
  'overage.jwt': await signToken({ ...base, overage_since: '2026-09-01T00:00:00.000Z' }, mainKey, 'test-k1'),
  'wrong-key.jwt': await signToken(base, otherKey, 'test-k1'),
  'wrong-issuer.jwt': await signToken({ ...base, iss: 'evil.example' }, mainKey, 'test-k1'),
  'unknown-kid.jwt': await signToken(base, mainKey, 'test-k2'),
  'hs256-header.jwt': await signWithHeader({ alg: 'HS256', typ: 'JWT', kid: 'test-k1' }, base, mainKey),
  'missing-field.jwt': await signWithHeader({ alg: 'RS512', typ: 'JWT', kid: 'test-k1' }, withoutMaxDevices, mainKey),
};

for (const [name, content] of Object.entries(files)) writeFileSync(join(out, name), content);
console.log(`wrote ${Object.keys(files).length} fixtures to ${out}`);
```

- [ ] **Step 2: Hasilkan fixture ke repo veyon**

```bash
cd /Volumes/rherdians/programming/alkhwarizmi/khwarizmi-license
npx tsx scripts/generate-client-fixtures.ts /Volumes/rherdians/programming/alkhwarizmi/veyon/tests/unit/fixtures
```

Expected: `wrote 9 fixtures to ...`. Pastikan tidak ada file `*private*` di direktori fixture — keypair uji hanya menyimpan public key-nya.

`tests/unit/fixtures/README.md`:

```markdown
# Fixture token lisensi

Dihasilkan oleh `scripts/generate-client-fixtures.ts` di repo `khwarizmi-license`,
memakai `signToken()` yang sama dengan backend produksi dan **keypair uji
sekali-pakai** — bukan kunci produksi. Private key uji tidak disimpan.

Regenerasi hanya bila format token berubah:

    npx tsx scripts/generate-client-fixtures.ts <path-ke>/veyon/tests/unit/fixtures

Semua token memakai `iat = 1790000000` (2026-09-21 UTC); test menyuntikkan waktu
sendiri, jadi fixture tidak pernah "kedaluwarsa".
```

- [ ] **Step 3: Daftarkan direktori test**

`tests/CMakeLists.txt` — tambahkan di akhir:

```cmake
if(WITH_TESTS)
	add_subdirectory(unit)
endif()
```

`tests/unit/CMakeLists.txt`:

```cmake
add_executable(license-test LicenseTest.cpp)
target_link_libraries(license-test PRIVATE veyon-core Qt${QT_MAJOR_VERSION}::Test)
target_compile_definitions(license-test PRIVATE LICENSE_TEST_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
set_default_target_properties(license-test)
add_test(NAME license-test COMMAND license-test)
```

`.gitignore` — tambahkan baris `build-tests/` di bawah `build/`.

- [ ] **Step 4: Tulis smoke test yang membuktikan harness dan QCA RSA bekerja**

`tests/unit/LicenseTest.cpp`:

```cpp
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
```

- [ ] **Step 5: Build dan jalankan**

```bash
docker exec veyon-dev bash -lc 'cd /veyon/build-tests && cmake .. >/dev/null && make -j10 license-test && ./tests/unit/license-test'
```

Expected: `Totals: 4 passed, 0 failed` (dua test + `initTestCase`/`cleanupTestCase` bawaan).

- [ ] **Step 6: Commit di kedua repo**

```bash
cd /Volumes/rherdians/programming/alkhwarizmi/khwarizmi-license
git add scripts/generate-client-fixtures.ts
git commit -m "test: generate licence token fixtures for the Veyon client"

cd /Volumes/rherdians/programming/alkhwarizmi/veyon
git add .gitignore tests/CMakeLists.txt tests/unit
git commit -m "test: add QTest harness and backend-signed licence fixtures"
```

---

### Task 2: `LicenseToken` — parse dan verifikasi

**Files:**
- Create: `core/src/LicenseToken.h`
- Create: `core/src/LicenseToken.cpp`
- Create: `core/resources/license-public.pem`
- Modify: `core/resources/core.qrc`
- Modify: `tests/unit/LicenseTest.cpp`

**Interfaces:**
- Consumes: `CryptoCore::PublicKey` (`core/src/CryptoCore.h`)
- Produces:
  - `struct LicenseClaims { QString issuer, masterId, tenantId, tenantName, plan; int maxDevices; QDateTime subscriptionEnd, overageSince, issuedAt, expiresAt; }` — `overageSince` tidak valid bila `null`
  - `LicenseToken::verify(const QString& token, const QMap<QString, CryptoCore::PublicKey>& keysByKid) -> std::optional<LicenseClaims>`
  - `LicenseToken::productionKeys() -> QMap<QString, CryptoCore::PublicKey>` — `{"k1": <kunci dari :/core/license-public.pem>}`

- [ ] **Step 1: Tanam public key produksi**

```bash
cp ~/khwarizmi-keys/license-public.pem /Volumes/rherdians/programming/alkhwarizmi/veyon/core/resources/license-public.pem
```

Ini **public** key — aman di-commit ke repo GPL, dan memang harus ikut di binary. Private key tidak pernah menyentuh repo ini.

Di `core/resources/core.qrc`, tambahkan baris berikut tepat di bawah `<file>default-pkey.pem</file>`:

```xml
    <file>license-public.pem</file>
```

- [ ] **Step 2: Tulis test yang gagal**

Tambahkan ke `tests/unit/LicenseTest.cpp` — `#include "LicenseToken.h"` di bagian include, helper di bagian `private:`, dan slot di bagian `private Q_SLOTS:`:

```cpp
	static QMap<QString, CryptoCore::PublicKey> testKeys()
	{
		return { { QStringLiteral("test-k1"),
				   CryptoCore::PublicKey::fromPEM(fixture(QStringLiteral("test-public.pem"))) } };
	}
```

```cpp
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
		const auto forged = QString::fromLatin1(noneHeader) + QLatin1Char('.') + parts[1] + QLatin1Char('.');
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
```

- [ ] **Step 3: Jalankan untuk memastikan gagal**

Run: `docker exec veyon-dev bash -lc 'cd /veyon/build-tests && cmake .. >/dev/null && make -j10 license-test 2>&1 | tail -5'`
Expected: gagal compile — `LicenseToken.h: No such file or directory`.

- [ ] **Step 4: Implementasi**

`core/src/LicenseToken.h`:

```cpp
/*
 * LicenseToken.h - verification of Khwarizmi licence tokens
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#pragma once

#include <optional>

#include <QDateTime>
#include <QMap>
#include <QString>

#include "CryptoCore.h"

struct VEYON_CORE_EXPORT LicenseClaims
{
	QString issuer;
	QString masterId;
	QString tenantId;
	QString tenantName;
	QString plan;
	int maxDevices = 0;
	QDateTime subscriptionEnd;
	QDateTime overageSince; // invalid when the school is within quota
	QDateTime issuedAt;
	QDateTime expiresAt;
};

class VEYON_CORE_EXPORT LicenseToken
{
public:
	/**
	 * Verifies an RS512 licence token and returns its claims.
	 *
	 * Deliberately does NOT reject an expired token: expiry is the connection
	 * axis of LicenseEvaluator. Rejecting it here would turn a school whose
	 * internet is down into "not activated". This function never reads the
	 * clock.
	 */
	static std::optional<LicenseClaims> verify(const QString& token,
											   const QMap<QString, CryptoCore::PublicKey>& keysByKid);

	static QMap<QString, CryptoCore::PublicKey> productionKeys();
};
```

`core/src/LicenseToken.cpp`:

```cpp
/*
 * LicenseToken.cpp - verification of Khwarizmi licence tokens
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#include <cmath>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

#include "LicenseToken.h"

// Named, not anonymous: the unity build merges translation units.
namespace LicenseTokenDetail
{

static std::optional<QByteArray> decodeSegment(QByteArray segment)
{
	if (segment.isEmpty() || segment.size() % 4 == 1)
	{
		return std::nullopt;
	}
	while (segment.size() % 4 != 0)
	{
		segment.append('=');
	}
	const auto result = QByteArray::fromBase64Encoding(
		segment, QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
	if (result.decodingStatus != QByteArray::Base64DecodingStatus::Ok)
	{
		return std::nullopt;
	}
	return result.decoded;
}

static std::optional<QJsonObject> parseObject(const QByteArray& json)
{
	QJsonParseError error{};
	const auto document = QJsonDocument::fromJson(json, &error);
	if (error.error != QJsonParseError::NoError || document.isObject() == false)
	{
		return std::nullopt;
	}
	return document.object();
}

static QDateTime parseIsoUtc(const QJsonValue& value)
{
	if (value.isString() == false)
	{
		return {};
	}
	const auto dateTime = QDateTime::fromString(value.toString(), Qt::ISODateWithMs);
	return dateTime.isValid() ? dateTime.toUTC() : QDateTime{};
}

static QDateTime parseUnixSeconds(const QJsonValue& value)
{
	if (value.isDouble() == false)
	{
		return {};
	}
	const auto seconds = value.toDouble();
	if (seconds < 0 || seconds != std::floor(seconds))
	{
		return {};
	}
	return QDateTime::fromSecsSinceEpoch(qint64(seconds)).toUTC();
}

static bool isWholeNonNegative(const QJsonValue& value)
{
	return value.isDouble() && value.toDouble() >= 0 && value.toDouble() == std::floor(value.toDouble());
}

}

std::optional<LicenseClaims> LicenseToken::verify(const QString& token,
												  const QMap<QString, CryptoCore::PublicKey>& keysByKid)
{
	using namespace LicenseTokenDetail;

	const auto parts = token.toLatin1().split('.');
	if (parts.size() != 3)
	{
		return std::nullopt;
	}

	const auto headerJson = decodeSegment(parts[0]);
	const auto payloadJson = decodeSegment(parts[1]);
	const auto signature = decodeSegment(parts[2]);
	if (!headerJson || !payloadJson || !signature)
	{
		return std::nullopt;
	}

	const auto header = parseObject(*headerJson);
	if (!header || header->value(QStringLiteral("alg")).toString() != QStringLiteral("RS512"))
	{
		return std::nullopt;
	}

	const auto kid = header->value(QStringLiteral("kid")).toString();
	if (kid.isEmpty() || keysByKid.contains(kid) == false)
	{
		return std::nullopt;
	}

	auto key = keysByKid.value(kid); // verifyMessage() is not const
	if (key.isNull())
	{
		return std::nullopt;
	}

	// The algorithm is fixed here, never taken from the header.
	const QByteArray signingInput = parts[0] + '.' + parts[1];
	if (key.verifyMessage(signingInput, *signature, CryptoCore::DefaultSignatureAlgorithm) == false)
	{
		return std::nullopt;
	}

	const auto payload = parseObject(*payloadJson);
	if (!payload)
	{
		return std::nullopt;
	}
	const auto& p = *payload;

	LicenseClaims claims;
	claims.issuer = p.value(QStringLiteral("iss")).toString();
	claims.masterId = p.value(QStringLiteral("sub")).toString();
	claims.tenantId = p.value(QStringLiteral("tenant")).toString();
	claims.plan = p.value(QStringLiteral("plan")).toString();

	if (claims.issuer != QStringLiteral("license.khwarizmi.co.id") ||
		claims.masterId.isEmpty() || claims.tenantId.isEmpty() ||
		(claims.plan != QStringLiteral("trial") && claims.plan != QStringLiteral("paid")) ||
		p.value(QStringLiteral("tenant_name")).isString() == false ||
		isWholeNonNegative(p.value(QStringLiteral("max_devices"))) == false ||
		p.contains(QStringLiteral("overage_since")) == false)
	{
		return std::nullopt;
	}

	claims.tenantName = p.value(QStringLiteral("tenant_name")).toString();
	claims.maxDevices = int(p.value(QStringLiteral("max_devices")).toDouble());

	claims.subscriptionEnd = parseIsoUtc(p.value(QStringLiteral("sub_end")));
	claims.issuedAt = parseUnixSeconds(p.value(QStringLiteral("iat")));
	claims.expiresAt = parseUnixSeconds(p.value(QStringLiteral("exp")));
	if (!claims.subscriptionEnd.isValid() || !claims.issuedAt.isValid() || !claims.expiresAt.isValid() ||
		claims.expiresAt <= claims.issuedAt)
	{
		return std::nullopt;
	}

	const auto overage = p.value(QStringLiteral("overage_since"));
	if (overage.isNull() == false)
	{
		claims.overageSince = parseIsoUtc(overage);
		if (!claims.overageSince.isValid())
		{
			return std::nullopt;
		}
	}

	return claims;
}

QMap<QString, CryptoCore::PublicKey> LicenseToken::productionKeys()
{
	return { { QStringLiteral("k1"), CryptoCore::PublicKey::fromPEMFile(QStringLiteral(":/core/license-public.pem")) } };
}
```

- [ ] **Step 5: Jalankan test**

Run: `docker exec veyon-dev bash -lc 'cd /veyon/build-tests && cmake .. >/dev/null && make -j10 license-test && ./tests/unit/license-test'`
Expected: semua lulus, `0 failed`.

- [ ] **Step 6: Buktikan tiga test kunci benar-benar menggigit**

Untuk tiap eksperimen, ubah sementara, jalankan, catat bahwa test yang disebut GAGAL, lalu kembalikan:

1. Ganti pengecekan `alg` agar menerima nilai apa pun (hapus `|| header->value(...) != "RS512"`) → `rejectsNonRs512HeaderEvenWithValidSignature` harus gagal.
2. Tambahkan penolakan token kedaluwarsa di `verify()` (mis. `if (claims.expiresAt < QDateTime::currentDateTimeUtc()) return std::nullopt;`) → `acceptsExpiredToken` harus gagal.
3. Ganti `CryptoCore::DefaultSignatureAlgorithm` dengan `QCA::EMSA3_SHA256` → `verifiesBackendSignedToken` harus gagal. Ini membuktikan kecocokan algoritma dengan backend.

Setelah dikembalikan, `git diff core/src/LicenseToken.cpp` hanya menampilkan file baru utuh, dan seluruh test hijau.

- [ ] **Step 7: Commit**

```bash
git add core/src/LicenseToken.h core/src/LicenseToken.cpp core/resources/license-public.pem core/resources/core.qrc tests/unit/LicenseTest.cpp
git commit -m "feat: verify RS512 licence tokens against the embedded public key"
```

---

### Task 3: `LicenseEvaluator` — status tiga sumbu

**Files:**
- Create: `core/src/LicenseEvaluator.h`
- Create: `core/src/LicenseEvaluator.cpp`
- Modify: `tests/unit/LicenseTest.cpp`

**Interfaces:**
- Consumes: `LicenseClaims` (Task 2)
- Produces:
  - `enum class LicenseLevel { Normal, Warning, Suspended }`
  - `enum class LicenseReason { None, NotActivated, ClockRolledBack, Subscription, SubscriptionUnverified, Quota, Connection }`
  - `struct LicenseState { LicenseLevel level; LicenseReason reason; QDateTime escalatesAt; }` — `escalatesAt` = kapan `Warning` menjadi `Suspended`; tidak valid selain itu
  - `LicenseEvaluator::evaluate(const std::optional<LicenseClaims>& claims, const QDateTime& now, const QDateTime& lastServerTime) -> LicenseState`
  - konstanta `SubscriptionWarningDays = 7`, `OfflineGraceDays = 14`, `OverageGraceDays = 14`, `ClockRollbackToleranceSecs = 3600`

**Aturan (spec §8):**

| Sumbu | Mulai bermasalah | Warning selama | Lalu |
|---|---|---|---|
| Langganan | `now > sub_end` | 7 hari | Suspended |
| Koneksi | `now > exp` | 14 hari | Suspended |
| Kuota | `overage_since` terisi | 14 hari sejak itu | Suspended |

- Tanpa claims → `{Suspended, NotActivated}`.
- Waktu acuan = yang terbaru dari `lastServerTime` dan `claims.issuedAt`. Bila `now` lebih awal dari acuan dikurangi 3600 detik → `{Suspended, ClockRolledBack}`.
- Level akhir = terburuk dari tiga sumbu. Alasan = sumbu pada level itu, prioritas langganan → kuota → koneksi.
- Bila alasannya langganan **dan** sumbu koneksi tidak Normal → `SubscriptionUnverified`.
- Batas inklusif: tepat pada `sub_end` masih Normal; tepat pada `sub_end + 7 hari` masih Warning; satu detik sesudahnya Suspended.

- [ ] **Step 1: Tulis test tabel yang gagal**

Tambahkan `#include "LicenseEvaluator.h"` dan `Q_DECLARE_METATYPE` di atas kelas test:

```cpp
Q_DECLARE_METATYPE(LicenseLevel)
Q_DECLARE_METATYPE(LicenseReason)
```

Helper di bagian `private:`:

```cpp
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
```

Slot:

```cpp
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
```

- [ ] **Step 2: Jalankan untuk memastikan gagal**

Run: `docker exec veyon-dev bash -lc 'cd /veyon/build-tests && cmake .. >/dev/null && make -j10 license-test 2>&1 | tail -5'`
Expected: gagal compile — `LicenseEvaluator.h: No such file or directory`.

- [ ] **Step 3: Implementasi**

`core/src/LicenseEvaluator.h`:

```cpp
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
	SubscriptionUnverified, // unpaid as far as we know, but we are also offline
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
```

`core/src/LicenseEvaluator.cpp`:

```cpp
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
```

- [ ] **Step 4: Jalankan test**

Run: `docker exec veyon-dev bash -lc 'cd /veyon/build-tests && cmake .. >/dev/null && make -j10 license-test && ./tests/unit/license-test'`
Expected: semua lulus, `0 failed`.

- [ ] **Step 5: Buktikan dua aturan kunci menggigit**

1. Hapus cabang `SubscriptionUnverified` (selalu kembalikan `Subscription`) → baris `unpaid AND offline` harus gagal.
2. Ganti `now <= limit` menjadi `now < limit` → baris `sub lapsed exactly 7d` harus gagal.

Kembalikan keduanya; seluruh test hijau.

- [ ] **Step 6: Commit**

```bash
git add core/src/LicenseEvaluator.h core/src/LicenseEvaluator.cpp tests/unit/LicenseTest.cpp
git commit -m "feat: evaluate licence status across subscription, quota, and connection"
```

---

## Setelah rencana ini selesai

`veyon-core` bisa memverifikasi token produksi dan menghitung statusnya, tanpa jaringan dan tanpa UI. Belum ada yang memanggilnya.

- **2b:** penyimpanan token dan secret di `VeyonConfiguration`, klien HTTP ke `/v1/activate` dan `/v1/checkin` (Qt Network), check-in harian dengan backoff, halaman Lisensi di Configurator.
- **2c:** gerbang saat master start (`master/src/main.cpp`), banner peringatan, dan kuota lokal di `ComputerControlListModel.cpp:347`.

Kompilasi Qt 6 baru terbukti saat build Windows di CI; container hanya Qt 5.
