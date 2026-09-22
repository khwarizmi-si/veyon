# Klien Lisensi 2b — Penyimpanan, Jaringan, dan Halaman Lisensi

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Veyon dapat diaktivasi dengan kode dari Configurator, menyimpan kredensialnya dengan aman, melakukan check-in ke backend, dan menampilkan status lisensi — di atas inti `LicenseToken`/`LicenseEvaluator` dari 2a.

**Architecture:** Logika keputusan tetap murni dan teruji (`LicenseSelector`, pembentuk request/pengurai response `LicenseClient`). Tepi yang tidak murni — file konfigurasi, jaringan, jam — dikumpulkan di satu fasad `LicenseService`. Data lisensi disimpan di dua file JSON khusus, **bukan** di `VeyonConfiguration`.

**Tech Stack:** C++20, Qt 5.15 + Qt 6, Qt Network, QCA, QTest.

**Spec:** `docs/superpowers/specs/2026-09-19-khwarizmi-saas-licensing-design.md` §5, §8, §9, §10, §11.
**Dibangun di atas:** `docs/superpowers/plans/2026-09-22-license-client-core.md` (2a, selesai; CI Qt 5 & Qt 6 hijau).

---

## Lingkungan

Sama dengan 2a: build dan test di container `veyon-dev`, repo di `/veyon`, direktori `build-tests`:

```bash
docker exec veyon-dev bash -lc 'cd /veyon/build-tests && cmake .. >/dev/null && make -j10 license-test && ./tests/unit/license-test'
```

Test saat ini: 50/50 lulus. Branch: `feat/license-client`.

## Keputusan desain yang membentuk rencana ini

### 1. Data lisensi TIDAK disimpan di `VeyonConfiguration`

Dua alasan, keduanya terverifikasi di kode:

- **Ekspor konfigurasi membocorkannya.** "Save settings to file" di Configurator menulis seluruh `VeyonCore::config()` ke JSON (`configurator/src/MainWindow.cpp`, `saveSettingsToFile`). Flag `Hidden` hanya menyembunyikan di UI, tidak mengecualikan dari ekspor. Secret master akan ikut di setiap file ekspor dan template GPO — lalu terbawa ke sekolah lain.
- **Master tidak bisa menulisnya.** `VeyonConfiguration` memakai scope System (HKLM di Windows, `/etc` di Linux); master berjalan sebagai user guru. Token hasil check-in harian tidak akan bisa disimpan.

Jadi dua `Configuration::Object` terpisah, keduanya backend JSON dengan nama store `KhwarizmiLicense`:

| Objek | Scope | Ditulis oleh | Isi |
|---|---|---|---|
| `LicenseActivation` | System (`globalAppDataPath()/KhwarizmiLicense.json`) | Configurator (admin), saat aktivasi | `masterId`, `secret`, `activationToken`, `serverUrl` |
| `LicenseCache` | User (`userConfigurationDirectory()/KhwarizmiLicense.json`) | Master dan Configurator, saat check-in | `refreshedToken`, `lastServerTime` |

Keduanya bukan bagian dari `VeyonCore::config()`, sehingga **tidak pernah ikut terekspor**.

Konsekuensi yang sengaja diterima: **satu kode aktivasi per PC guru.** Kode bersifat sekali-pakai (spec §8), jadi sekolah dengan lima PC guru menerbitkan lima kode dari admin console. Ini konsisten dengan spec yang sudah disetujui.

### 2. Token yang berlaku = yang terbaru dan sah dari kedua file

`LicenseSelector` memverifikasi token aktivasi dan token cache, membuang yang `sub`-nya bukan `masterId` terpasang atau yang tenant-nya berbeda dari token aktivasi, lalu memilih `iat` terbesar. Menyimpan token mentah dan memverifikasi ulang tiap kali dimuat berarti claims yang tidak sah tidak pernah sampai ke `LicenseEvaluator`.

### 3. Aturan menerima token baru (di `LicenseService`)

- Token harus lolos `verify()`, dan `sub` = `masterId` terpasang.
- **Tolak token dengan `iat` lebih tua dari token tersimpan** — mencegah token lama diputar ulang untuk mengembalikan `sub_end` yang lebih panjang setelah langganan dipotong.
- **Tolak token dengan `iat` lebih dari 1 jam di depan jam lokal.** Simpan token lama, laporkan `ClockSkew`. Tanpa ini, satu token dengan `iat` salah (jam lokal mundur atau server bermasalah) akan menaikkan `lastServerTime` secara permanen — nilai itu hanya boleh naik — dan mengunci sekolah dalam `ClockRolledBack` selamanya.
- `lastServerTime` hanya diambil dari `iat` token yang **diterima**, dan hanya naik. Header HTTP `Date` tidak pernah dipakai: tidak ditandatangani.

**Yang sengaja TIDAK dilakukan:** menyimpan "waktu lokal terakhir" sebagai acuan jam mundur. Kalau jam PC pernah salah ke depan (misal 2030) lalu dikoreksi, acuan itu akan mengunci sekolah dalam `ClockRolledBack` tanpa jalan keluar. Hanya waktu server yang ditandatangani yang boleh menjadi acuan.

### 4. Kegagalan condong membiarkan sekolah bekerja (spec §11)

`401`/`403` dari check-in diperlakukan seperti kegagalan jaringan: token lama tetap dipakai, sumbu koneksi yang berjalan. Tidak ada respons server yang langsung menghapus lisensi lokal.

### 5. Plugin TLS Qt dan plugin OpenSSL QCA menjadi wajib di installer Windows

Keduanya kini berstatus opsional (`copy_qt_plugin_optional tls qopensslbackend.dll`, `File /nonfatal`, dan `libqca-ossl.dll` hanya memicu peringatan). Tanpa plugin TLS, Qt 6 tidak bisa HTTPS sama sekali — aktivasi mustahil. Tanpa `libqca-ossl.dll`, QCA tidak punya RSA — setiap token ditolak dan semua sekolah melihat "belum diaktivasi". Keduanya ada di build terakhir, tetapi satu perubahan paket MSYS2 cukup untuk menghilangkannya diam-diam. Build harus gagal, bukan mengirim installer yang rusak.

---

## Global Constraints

- **Qt 5 dan Qt 6.** Tidak ada `QJsonValue::toInteger()`, tidak ada overload `QDateTime` dengan `Qt::TimeSpec`. Pola HTTP mengikuti `plugins/entraid/EntraIdGraphClient.cpp` (sudah ter-compile di keduanya).
- **Unity build:** helper di namespace bernama, bukan anonim.
- **`QT_USE_QSTRINGBUILDER` aktif:** gabungan string tidak pernah disimpan ke `auto` — pakai tipe eksplisit.
- **INVARIANT — jalur lisensi steril.** Body check-in hanya boleh berisi `master_id`, `devices` (`mac`, `hostname`), `hostname`, `version`. Tidak pernah data pemantauan. Ada test yang mengunci kunci-kunci ini.
- **Secret master tidak pernah dicatat ke log**, tidak pernah ditampilkan di UI, dan tidak pernah dimasukkan ke URL.
- URL backend default: `https://license.khwarizmi.co.id`. Hanya HTTPS; URL `http://` ditolak.
- Batas waktu request: 20 detik.
- Semua kelas publik `VEYON_CORE_EXPORT`. Gaya Veyon: tab, `QStringLiteral`, header GPL.
- **String UI:** sumber dalam bahasa Inggris lewat `tr()` (konvensi fork ini), terjemahan Indonesia ditambahkan ke `translations/veyon_id.ts`.
- Tidak ada supresi warning baru.

---

### Task 1: Plugin TLS dan QCA OpenSSL wajib di installer Windows

**Files:**
- Modify: `.ci/windows/stage-installer.sh`
- Modify: `nsis/veyon.nsi.in`

- [ ] **Step 1: Jadikan wajib di skrip staging**

Di `.ci/windows/stage-installer.sh`:

- Ganti `copy_qt_plugin_optional tls qopensslbackend.dll` menjadi `copy_qt_plugin_required tls qopensslbackend.dll`.
- Ganti blok `libqca-ossl.dll` yang kini hanya mencetak `WARNING` menjadi gagal:

```bash
mkdir -p "${install_files}/crypto"
qca_ossl_plugin="$(find "${mingw_prefix}" -path '*/crypto/libqca-ossl.dll' -print -quit)"
if [ -z "${qca_ossl_plugin}" ]; then
	# Without it QCA has no RSA provider, so every licence token fails verification.
	echo "ERROR: libqca-ossl.dll not found under ${mingw_prefix}" >&2
	exit 1
fi
cp "${qca_ossl_plugin}" "${install_files}/crypto"
```

Tambahkan komentar satu baris di atas baris `copy_qt_plugin_required tls ...`: tanpa backend TLS, Qt 6 tidak bisa HTTPS dan aktivasi lisensi mustahil.

- [ ] **Step 2: Jadikan wajib di installer NSIS**

Di `nsis/veyon.nsi.in`, hapus `/nonfatal` dari baris `File /nonfatal "tls/qopensslbackend.dll"`, dan pastikan baris yang memasang `crypto/*.dll` tidak memakai `/nonfatal` untuk `libqca-ossl.dll`. Bila `crypto/*.dll` saat ini `File /nonfatal "crypto/*.dll"`, ganti dengan `File "crypto/libqca-ossl.dll"` yang wajib.

- [ ] **Step 3: Verifikasi sintaks**

```bash
bash -n .ci/windows/stage-installer.sh && echo "syntax OK"
grep -n "qopensslbackend\|libqca-ossl" .ci/windows/stage-installer.sh nsis/veyon.nsi.in
```

Expected: `syntax OK`; tidak ada `optional` atau `/nonfatal` pada kedua plugin itu.

Bukti sesungguhnya adalah build installer Windows di CI; controller akan menjalankannya lewat `workflow_dispatch` setelah task ini.

- [ ] **Step 4: Commit**

```bash
git add .ci/windows/stage-installer.sh nsis/veyon.nsi.in
git commit -m "build: require the TLS backend and QCA OpenSSL plugin in the Windows installer"
```

---

### Task 2: `LicenseSelector` — memilih token yang berlaku

**Files:**
- Create: `core/src/LicenseSelector.h`
- Create: `core/src/LicenseSelector.cpp`
- Modify: `tests/unit/LicenseTest.cpp`
- Modify: `/Volumes/rherdians/programming/alkhwarizmi/khwarizmi-license/scripts/generate-client-fixtures.ts` + regenerasi fixture

**Interfaces:**
- Consumes: `LicenseToken::verify`, `LicenseClaims` (2a)
- Produces:
  - `struct SelectedLicense { QString token; LicenseClaims claims; }`
  - `LicenseSelector::select(const QString& activationToken, const QString& cachedToken, const QString& masterId, const QMap<QString, CryptoCore::PublicKey>& keys) -> std::optional<SelectedLicense>`

**Aturan:**
- `masterId` kosong → `std::nullopt` (belum diaktivasi).
- Kandidat sah bila lolos `verify()` **dan** `claims.masterId == masterId`.
- Bila token aktivasi sah, kandidat cache juga harus bertenant sama dengannya; kalau tidak, cache dibuang (bisa sisa aktivasi lama di profil user yang sama).
- Dari kandidat sah, kembalikan yang `iat`-nya terbesar. Seri → token aktivasi.
- Tidak membaca jam; aturan "iat di masa depan" ada di `LicenseService` (Task 4).

- [ ] **Step 1: Tambah fixture**

Di generator, tambahkan ke `files`:

```typescript
  // Same master, issued a day later: the one a check-in would return.
  'newer.jwt': await signToken({ ...base, iat: 1790000000 + 86400, exp: 1790000000 + 8 * 86400 }, mainKey, 'test-k1'),
  'other-master.jwt': await signToken({ ...base, sub: 'mst_other', iat: 1790000000 + 86400, exp: 1790000000 + 8 * 86400 }, mainKey, 'test-k1'),
  'other-tenant.jwt': await signToken({ ...base, tenant: 'sch_other', iat: 1790000000 + 86400, exp: 1790000000 + 8 * 86400 }, mainKey, 'test-k1'),
```

Regenerasi ke `tests/unit/fixtures`. Semua fixture berganti (keypair uji baru); seluruh test lama harus tetap lulus. Commit generator di repo lisensi sebagai commit tersendiri. Jangan pernah membaca `~/khwarizmi-keys`.

- [ ] **Step 2: Tulis test yang gagal**

`#include "LicenseSelector.h"`; slot:

```cpp
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
```

- [ ] **Step 3: Jalankan untuk memastikan gagal** — gagal compile, `LicenseSelector.h` tidak ada.

- [ ] **Step 4: Implementasi**

`core/src/LicenseSelector.h`:

```cpp
/*
 * LicenseSelector.h - pick the licence token currently in force
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

struct VEYON_CORE_EXPORT SelectedLicense
{
	QString token;
	LicenseClaims claims;
};

class VEYON_CORE_EXPORT LicenseSelector
{
public:
	/**
	 * Verifies both stored tokens and returns the newest one that belongs to
	 * this installation. Raw tokens are re-verified on every call, so claims
	 * that were never signed can never reach LicenseEvaluator. Pure: never
	 * reads the clock.
	 */
	static std::optional<SelectedLicense> select(const QString& activationToken,
												 const QString& cachedToken,
												 const QString& masterId,
												 const QMap<QString, CryptoCore::PublicKey>& keys);
};
```

`core/src/LicenseSelector.cpp`:

```cpp
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
```

- [ ] **Step 5: Jalankan test** — semua lulus.

- [ ] **Step 6: Bite-proof** — hapus pengecekan tenant pada cache → `rejectsCacheFromAnotherTenant` harus gagal. Kembalikan.

- [ ] **Step 7: Commit**

```bash
git add core/src/LicenseSelector.h core/src/LicenseSelector.cpp tests/unit/LicenseTest.cpp tests/unit/fixtures
git commit -m "feat: select the newest licence token that belongs to this installation"
```

---

### Task 3: `LicenseClient` — request dan response HTTP

**Files:**
- Create: `core/src/LicenseClient.h`
- Create: `core/src/LicenseClient.cpp`
- Modify: `tests/unit/LicenseTest.cpp`

**Interfaces:**
- Produces:
  - `struct LicenseDevice { QString mac; QString hostname; }`
  - `enum class LicenseCallStatus { Ok, InvalidCode, Unauthorized, NetworkError, ServerError, InvalidResponse }`
  - `struct ActivationResponse { LicenseCallStatus status; QString masterId; QString secret; QString token; }`
  - `struct CheckInResponse { LicenseCallStatus status; QString token; }`
  - `LicenseClient::buildActivateBody(code, hostname, version) -> QByteArray`
  - `LicenseClient::buildCheckInBody(masterId, devices, hostname, version) -> QByteArray`
  - `LicenseClient::parseActivateResponse(httpStatus, body) -> ActivationResponse`
  - `LicenseClient::parseCheckInResponse(httpStatus, body) -> CheckInResponse`
  - `LicenseClient::endpoint(serverUrl, path) -> QUrl` — invalid bila bukan `https`
  - `LicenseClient(QUrl serverUrl)`; `activate(code, hostname, version)`, `checkIn(masterId, secret, devices, hostname, version)` — sinkron, timeout 20 detik

Fungsi `build*`/`parse*`/`endpoint` murni dan diuji; transport (`activate`/`checkIn`) mengikuti pola `EntraIdGraphClient::request` dan diverifikasi end-to-end di Task 5.

**Pemetaan status HTTP → `LicenseCallStatus`:**
- 200 dengan JSON lengkap → `Ok`; 200 tanpa field wajib → `InvalidResponse`
- 400 dengan `{"error":"invalid_or_used_code"}` → `InvalidCode`; 400 lainnya → `ServerError`
- 401 atau 403 → `Unauthorized`
- 5xx → `ServerError`
- tanpa status (jaringan/timeout/TLS) → `NetworkError`

- [ ] **Step 1: Tulis test yang gagal**

`#include "LicenseClient.h"` dan `#include <QJsonDocument>`, `#include <QJsonObject>`, `#include <QJsonArray>`; slot:

```cpp
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
```

Tambahkan `Q_DECLARE_METATYPE(LicenseCallStatus)` bersama deklarasi metatype lain.

- [ ] **Step 2: Jalankan untuk memastikan gagal** — gagal compile.

- [ ] **Step 3: Implementasi**

`core/src/LicenseClient.h`:

```cpp
/*
 * LicenseClient.h - HTTP client for the Khwarizmi licence backend
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#pragma once

#include <QList>
#include <QUrl>

#include "VeyonCore.h"

struct VEYON_CORE_EXPORT LicenseDevice
{
	QString mac;
	QString hostname;
};

enum class LicenseCallStatus
{
	Ok,
	InvalidCode,
	Unauthorized,
	NetworkError,
	ServerError,
	InvalidResponse,
};

struct VEYON_CORE_EXPORT ActivationResponse
{
	LicenseCallStatus status = LicenseCallStatus::NetworkError;
	QString masterId;
	QString secret;
	QString token;
};

struct VEYON_CORE_EXPORT CheckInResponse
{
	LicenseCallStatus status = LicenseCallStatus::NetworkError;
	QString token;
};

class VEYON_CORE_EXPORT LicenseClient
{
public:
	static constexpr int RequestTimeoutMsecs = 20000;

	explicit LicenseClient(const QString& serverUrl);

	ActivationResponse activate(const QString& code, const QString& hostname, const QString& version);
	CheckInResponse checkIn(const QString& masterId, const QString& secret, const QList<LicenseDevice>& devices,
							const QString& hostname, const QString& version);

	// Pure helpers, unit-tested.
	static QUrl endpoint(const QString& serverUrl, const QString& path);
	static QByteArray buildActivateBody(const QString& code, const QString& hostname, const QString& version);
	static QByteArray buildCheckInBody(const QString& masterId, const QList<LicenseDevice>& devices,
									   const QString& hostname, const QString& version);
	static ActivationResponse parseActivateResponse(int httpStatus, const QByteArray& body);
	static CheckInResponse parseCheckInResponse(int httpStatus, const QByteArray& body);

private:
	// Returns the HTTP status (0 when no response arrived) and fills `body`.
	int post(const QString& path, const QByteArray& payload, const QByteArray& bearer, QByteArray& body);

	QString m_serverUrl;
};
```

`core/src/LicenseClient.cpp`:

```cpp
/*
 * LicenseClient.cpp - HTTP client for the Khwarizmi licence backend
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

#include "LicenseClient.h"

namespace LicenseClientDetail
{

static std::optional<QJsonObject> parseObject(const QByteArray& body)
{
	QJsonParseError error{};
	const auto document = QJsonDocument::fromJson(body, &error);
	if (error.error != QJsonParseError::NoError || document.isObject() == false)
	{
		return std::nullopt;
	}
	return document.object();
}

static LicenseCallStatus statusFor(int httpStatus)
{
	if (httpStatus == 0)
	{
		return LicenseCallStatus::NetworkError;
	}
	if (httpStatus == 401 || httpStatus == 403)
	{
		return LicenseCallStatus::Unauthorized;
	}
	return LicenseCallStatus::ServerError;
}

}

LicenseClient::LicenseClient(const QString& serverUrl) :
	m_serverUrl(serverUrl)
{
}

QUrl LicenseClient::endpoint(const QString& serverUrl, const QString& path)
{
	QUrl url(serverUrl);
	if (url.isValid() == false || url.scheme() != QStringLiteral("https") || url.host().isEmpty())
	{
		return {};
	}
	QString basePath = url.path();
	while (basePath.endsWith(QLatin1Char('/')))
	{
		basePath.chop(1);
	}
	url.setPath(basePath + path);
	return url;
}

QByteArray LicenseClient::buildActivateBody(const QString& code, const QString& hostname, const QString& version)
{
	return QJsonDocument(QJsonObject{
		{ QStringLiteral("code"), code },
		{ QStringLiteral("hostname"), hostname },
		{ QStringLiteral("version"), version },
	}).toJson(QJsonDocument::Compact);
}

QByteArray LicenseClient::buildCheckInBody(const QString& masterId, const QList<LicenseDevice>& devices,
										   const QString& hostname, const QString& version)
{
	QJsonArray deviceArray;
	for (const auto& device : devices)
	{
		deviceArray.append(QJsonObject{
			{ QStringLiteral("mac"), device.mac.isEmpty() ? QJsonValue() : QJsonValue(device.mac) },
			{ QStringLiteral("hostname"), device.hostname },
		});
	}
	// INVARIANT: only licence fields. Never monitoring data.
	return QJsonDocument(QJsonObject{
		{ QStringLiteral("master_id"), masterId },
		{ QStringLiteral("devices"), deviceArray },
		{ QStringLiteral("hostname"), hostname },
		{ QStringLiteral("version"), version },
	}).toJson(QJsonDocument::Compact);
}

ActivationResponse LicenseClient::parseActivateResponse(int httpStatus, const QByteArray& body)
{
	using namespace LicenseClientDetail;
	ActivationResponse response;

	if (httpStatus == 200)
	{
		const auto object = parseObject(body);
		const QString masterId = object ? object->value(QStringLiteral("master_id")).toString() : QString();
		const QString secret = object ? object->value(QStringLiteral("secret")).toString() : QString();
		const QString token = object ? object->value(QStringLiteral("token")).toString() : QString();
		if (masterId.isEmpty() || secret.isEmpty() || token.isEmpty())
		{
			response.status = LicenseCallStatus::InvalidResponse;
			return response;
		}
		response.status = LicenseCallStatus::Ok;
		response.masterId = masterId;
		response.secret = secret;
		response.token = token;
		return response;
	}

	if (httpStatus == 400)
	{
		const auto object = parseObject(body);
		response.status = object && object->value(QStringLiteral("error")).toString() == QStringLiteral("invalid_or_used_code")
							  ? LicenseCallStatus::InvalidCode
							  : LicenseCallStatus::ServerError;
		return response;
	}

	response.status = statusFor(httpStatus);
	return response;
}

CheckInResponse LicenseClient::parseCheckInResponse(int httpStatus, const QByteArray& body)
{
	using namespace LicenseClientDetail;
	CheckInResponse response;

	if (httpStatus == 200)
	{
		const auto object = parseObject(body);
		const QString token = object ? object->value(QStringLiteral("token")).toString() : QString();
		response.status = token.isEmpty() ? LicenseCallStatus::InvalidResponse : LicenseCallStatus::Ok;
		response.token = token;
		return response;
	}

	response.status = statusFor(httpStatus);
	return response;
}

ActivationResponse LicenseClient::activate(const QString& code, const QString& hostname, const QString& version)
{
	QByteArray body;
	const auto httpStatus = post(QStringLiteral("/v1/activate"), buildActivateBody(code, hostname, version), {}, body);
	return parseActivateResponse(httpStatus, body);
}

CheckInResponse LicenseClient::checkIn(const QString& masterId, const QString& secret,
									   const QList<LicenseDevice>& devices,
									   const QString& hostname, const QString& version)
{
	QByteArray body;
	const auto httpStatus = post(QStringLiteral("/v1/checkin"), buildCheckInBody(masterId, devices, hostname, version),
								 secret.toUtf8(), body);
	return parseCheckInResponse(httpStatus, body);
}

int LicenseClient::post(const QString& path, const QByteArray& payload, const QByteArray& bearer, QByteArray& body)
{
	const auto url = endpoint(m_serverUrl, path);
	if (url.isValid() == false)
	{
		return 0;
	}

	QNetworkRequest request(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
	if (bearer.isEmpty() == false)
	{
		request.setRawHeader(QByteArrayLiteral("Authorization"), QByteArrayLiteral("Bearer ") + bearer);
	}

	QNetworkAccessManager networkAccessManager;
	auto reply = networkAccessManager.post(request, payload);

	QEventLoop eventLoop;
	QTimer timeoutTimer;
	timeoutTimer.setSingleShot(true);
	timeoutTimer.setInterval(RequestTimeoutMsecs);
	QObject::connect(&timeoutTimer, &QTimer::timeout, reply, &QNetworkReply::abort);
	QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
	timeoutTimer.start();
	eventLoop.exec();

	body = reply->readAll();
	const auto httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	reply->deleteLater();
	return httpStatus;
}
```

- [ ] **Step 4: Jalankan test** — semua lulus.

- [ ] **Step 5: Bite-proof** — tambahkan sementara kunci `"username"` ke `buildCheckInBody` → `checkInBodyCarriesOnlyLicenceFields` harus gagal. Kembalikan.

- [ ] **Step 6: Commit**

```bash
git add core/src/LicenseClient.h core/src/LicenseClient.cpp tests/unit/LicenseTest.cpp
git commit -m "feat: add the licence backend HTTP client with a sterile check-in body"
```

---

### Task 4: `LicenseService` — penyimpanan dan fasad

**Files:**
- Create: `core/src/LicenseStorage.h` (objek `LicenseActivation` dan `LicenseCache`)
- Create: `core/src/LicenseStorage.cpp`
- Create: `core/src/LicenseService.h`
- Create: `core/src/LicenseService.cpp`
- Modify: `tests/unit/LicenseTest.cpp`

**Interfaces:**
- Consumes: `LicenseToken`, `LicenseEvaluator`, `LicenseSelector`, `LicenseClient`
- Produces:
  - `class LicenseActivation : public Configuration::Object` — properti `masterId`, `secret`, `activationToken`, `serverUrl` (default `https://license.khwarizmi.co.id`); store JSON, scope System, nama `KhwarizmiLicense`
  - `class LicenseCache : public Configuration::Object` — properti `refreshedToken`, `lastServerTime` (string ISO); store JSON, scope User, nama `KhwarizmiLicense`
  - `enum class LicenseActionResult { Ok, InvalidCode, NetworkError, ServerError, InvalidResponse, Unauthorized, NotActivated, ClockSkew, StaleToken, NotWritable, TokenRejected }`
  - `LicenseService::acceptToken(const QString& candidate, const QString& currentToken, const QString& masterId, const QDateTime& now, const QMap<QString, CryptoCore::PublicKey>& keys) -> LicenseActionResult` — **murni**, aturan penerimaan token baru
  - `LicenseService::snapshot() -> LicenseSnapshot` — `{ LicenseState state; std::optional<LicenseClaims> claims; QString masterId; }`
  - `LicenseService::activate(const QString& code) -> LicenseActionResult`
  - `LicenseService::checkIn(const QList<LicenseDevice>& devices) -> LicenseActionResult`
  - `LicenseService::describe(const LicenseState&) -> QString` — pesan untuk manusia

**Aturan `acceptToken` (murni, diuji):**
- tidak lolos `verify()` atau `sub != masterId` → `TokenRejected`
- `iat > now + 3600 detik` → `ClockSkew`
- token saat ini sah dan `iat` kandidat lebih tua → `StaleToken`
- selain itu → `Ok`

**Perilaku `checkIn`:** Unauthorized dan NetworkError **tidak** menghapus atau mengubah apa pun — token lama tetap berlaku (spec §11). Token hanya disimpan bila `acceptToken` → `Ok`; saat itu `lastServerTime` dinaikkan ke `iat` token bila lebih besar.

**Perilaku `activate`:** menulis ke `LicenseActivation` (butuh hak admin). Bila store tidak bisa ditulis → `NotWritable` **sebelum** memanggil jaringan, agar kode sekali-pakai tidak hangus sia-sia. Token dari respons harus lolos `acceptToken` (dengan `currentToken` kosong) dan `sub`-nya sama dengan `master_id` respons; kalau tidak → `TokenRejected`, tidak ada yang disimpan.

- [ ] **Step 1: Tulis test yang gagal untuk `acceptToken`**

```cpp
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
	}
```

- [ ] **Step 2: Jalankan untuk memastikan gagal** — gagal compile.

- [ ] **Step 3: Implementasi penyimpanan**

`core/src/LicenseStorage.h`:

```cpp
/*
 * LicenseStorage.h - where licence data lives
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#pragma once

#include "Configuration/Object.h"
#include "Configuration/Property.h"

// Deliberately NOT part of VeyonConfiguration: "Save settings to file" exports
// VeyonCore::config() wholesale, which would put the master secret in every
// exported file and deployment template. These stores are never exported.

#define FOREACH_LICENSE_ACTIVATION_PROPERTY(OP) \
	OP(LicenseActivation, LicenseActivation(), QString, masterId, setMasterId, "MasterId", "License", QString(), Configuration::Property::Flag::Hidden) \
	OP(LicenseActivation, LicenseActivation(), QString, secret, setSecret, "Secret", "License", QString(), Configuration::Property::Flag::Hidden) \
	OP(LicenseActivation, LicenseActivation(), QString, activationToken, setActivationToken, "ActivationToken", "License", QString(), Configuration::Property::Flag::Hidden) \
	OP(LicenseActivation, LicenseActivation(), QString, serverUrl, setServerUrl, "ServerUrl", "License", QStringLiteral("https://license.khwarizmi.co.id"), Configuration::Property::Flag::Hidden)

#define FOREACH_LICENSE_CACHE_PROPERTY(OP) \
	OP(LicenseCache, LicenseCache(), QString, refreshedToken, setRefreshedToken, "RefreshedToken", "License", QString(), Configuration::Property::Flag::Hidden) \
	OP(LicenseCache, LicenseCache(), QString, lastServerTime, setLastServerTime, "LastServerTime", "License", QString(), Configuration::Property::Flag::Hidden)

// clazy:excludeall=ctor-missing-parent-argument,copyable-polymorphic

// System scope: written by the Configurator (administrator) at activation.
class VEYON_CORE_EXPORT LicenseActivation : public Configuration::Object
{
	Q_OBJECT
public:
	LicenseActivation();
	FOREACH_LICENSE_ACTIVATION_PROPERTY(DECLARE_CONFIG_PROPERTY)
};

// User scope: the master runs as the teacher and cannot write system config,
// so refreshed tokens from daily check-ins live here.
class VEYON_CORE_EXPORT LicenseCache : public Configuration::Object
{
	Q_OBJECT
public:
	LicenseCache();
	FOREACH_LICENSE_CACHE_PROPERTY(DECLARE_CONFIG_PROPERTY)
};
```

`core/src/LicenseStorage.cpp`:

```cpp
/*
 * LicenseStorage.cpp - where licence data lives
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#include "LicenseStorage.h"

LicenseActivation::LicenseActivation() :
	Configuration::Object(Configuration::Store::Backend::JsonFile, Configuration::Store::Scope::System,
						  QStringLiteral("KhwarizmiLicense"))
{
}

LicenseCache::LicenseCache() :
	Configuration::Object(Configuration::Store::Backend::JsonFile, Configuration::Store::Scope::User,
						  QStringLiteral("KhwarizmiLicense"))
{
}
```

Bila `Configuration/Object.h` atau `Configuration/Property.h` memerlukan include tambahan untuk `DECLARE_CONFIG_PROPERTY`, ikuti pola `master/src/UserConfig.h` persis. Argumen kedua makro `OP` tidak dipakai oleh `DECLARE_CONFIG_PROPERTY`.

**Semantik `isStoreWritable()` sudah diperiksa:** `JsonStore::isWritable()` membuka file dengan `WriteOnly | Append`, sehingga file yang belum ada **dibuat** bila direktorinya bisa ditulis — admin mendapat `true`, user biasa `false`. Itu yang dibutuhkan pemeriksaan sebelum aktivasi. Efek sampingnya: file kosong tercipta. Pastikan memuat `KhwarizmiLicense.json` yang kosong tidak crash dan menghasilkan properti default (sehingga `snapshot()` melaporkan `NotActivated`) — periksa `JsonStore::load` untuk file kosong, dan laporkan bila perilakunya lain.

- [ ] **Step 4: Implementasi fasad**

`core/src/LicenseService.h`:

```cpp
/*
 * LicenseService.h - licence activation, check-in and status
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#pragma once

#include <QCoreApplication>

#include "LicenseClient.h"
#include "LicenseEvaluator.h"

enum class LicenseActionResult
{
	Ok,
	InvalidCode,
	NetworkError,
	ServerError,
	InvalidResponse,
	Unauthorized,
	NotActivated,
	ClockSkew,
	StaleToken,
	NotWritable,
	TokenRejected,
};

struct VEYON_CORE_EXPORT LicenseSnapshot
{
	LicenseState state;
	std::optional<LicenseClaims> claims;
	QString masterId;
};

// The impure edge: files, network and the clock are only touched here.
class VEYON_CORE_EXPORT LicenseService
{
	Q_DECLARE_TR_FUNCTIONS(LicenseService)
public:
	static constexpr int MaxFutureIssueSecs = 3600;

	static LicenseSnapshot snapshot();
	static LicenseActionResult activate(const QString& code);
	static LicenseActionResult checkIn(const QList<LicenseDevice>& devices);

	static QString describe(const LicenseState& state);
	static QString describe(LicenseActionResult result);

	// Pure, unit-tested.
	static LicenseActionResult acceptToken(const QString& candidate, const QString& currentToken,
										   const QString& masterId, const QDateTime& now,
										   const QMap<QString, CryptoCore::PublicKey>& keys);
};
```

`core/src/LicenseService.cpp`:

```cpp
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
		return LicenseServiceDetail::fromCallStatus(response.status);
	}

	const auto accepted = acceptToken(response.token, {}, response.masterId, QDateTime::currentDateTimeUtc(),
									  LicenseToken::productionKeys());
	if (accepted != LicenseActionResult::Ok)
	{
		return accepted;
	}

	activation.setMasterId(response.masterId);
	activation.setSecret(response.secret);
	activation.setActivationToken(response.token);
	activation.flushStore();

	LicenseCache cache;
	cache.setRefreshedToken({});
	cache.setLastServerTime(LicenseToken::verify(response.token, LicenseToken::productionKeys())
								->issuedAt.toString(Qt::ISODateWithMs));
	cache.flushStore();

	return LicenseActionResult::Ok;
}

LicenseActionResult LicenseService::checkIn(const QList<LicenseDevice>& devices)
{
	using namespace LicenseServiceDetail;

	const LicenseActivation activation;
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

	const auto issuedAt = LicenseToken::verify(response.token, keys)->issuedAt;
	const auto stored = parseStoredTime(cache.lastServerTime());
	cache.setRefreshedToken(response.token);
	if (!stored.isValid() || issuedAt > stored)
	{
		cache.setLastServerTime(issuedAt.toString(Qt::ISODateWithMs));
	}
	cache.flushStore();
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
		return tr("This computer has not been activated. Enter an activation code in the Veyon Configurator.");
	case LicenseReason::ClockRolledBack:
		return tr("This computer's clock is behind the licence server's time. Correct the date and time, then check again.");
	case LicenseReason::Subscription:
		return state.level == LicenseLevel::Suspended
				   ? tr("The subscription has ended and the service is suspended. Please contact your school administrator.")
				   : tr("The subscription has ended. The service will be suspended on %1 unless it is renewed.").arg(deadline);
	case LicenseReason::SubscriptionUnverified:
		return state.level == LicenseLevel::Suspended
				   ? tr("The payment could not be verified and the service is suspended. Check the internet connection, then check again.")
				   : tr("The payment could not be verified. Check the internet connection. The service will be suspended on %1 if this continues.").arg(deadline);
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
	case LicenseActionResult::NotWritable: return tr("The licence could not be saved. Run the Veyon Configurator as an administrator.");
	case LicenseActionResult::TokenRejected: return tr("The licence returned by the server could not be verified.");
	}
	return {};
}
```

- [ ] **Step 5: Jalankan test** — semua lulus.

- [ ] **Step 6: Bite-proof** — hapus pengecekan `ClockSkew` → `refusesTokenFromTheFuture` harus gagal; hapus pengecekan `StaleToken` → `rejectsReplayOfOlderToken` harus gagal. Kembalikan.

- [ ] **Step 7: Commit**

```bash
git add core/src/LicenseStorage.h core/src/LicenseStorage.cpp core/src/LicenseService.h core/src/LicenseService.cpp tests/unit/LicenseTest.cpp
git commit -m "feat: add licence storage outside VeyonConfiguration and the licence service"
```

---

### Task 5: Halaman Lisensi di Configurator

**Files:**
- Create: `configurator/src/LicensePage.h`
- Create: `configurator/src/LicensePage.cpp`
- Modify: `configurator/src/MainWindow.cpp`
- Modify: `translations/veyon_id.ts`

**Interfaces:**
- Consumes: `LicenseService::snapshot`, `activate`, `checkIn`, `describe`

Halaman dibangun **dalam kode** (tanpa `.ui`) dan ditambahkan lewat jalur yang sama dengan halaman plugin, sehingga `MainWindow.ui` tidak perlu diedit.

Isi halaman:
- Status: pesan `describe(state)`, nama sekolah, kuota device, langganan s/d, token berlaku s/d, ID master. **Secret tidak pernah ditampilkan.**
- Kolom kode aktivasi + tombol **Activate** (nonaktif selama request berjalan).
- Tombol **Check now** — `checkIn({})`: check-in tanpa daftar device, hanya memperbarui token. Daftar device dari master adalah 2c.
- Hasil aksi ditampilkan dengan `describe(result)`.

Aktivasi adalah aksi langsung, bukan pengaturan yang menunggu tombol **Apply**; `applyConfiguration()`/`resetWidgets()`/`connectWidgetsToProperties()` hanya menyegarkan tampilan.

- [ ] **Step 1: Implementasi halaman**

`configurator/src/LicensePage.h`:

```cpp
/*
 * LicensePage.h - licence activation and status page
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#pragma once

#include "ConfigurationPage.h"

class QLabel;
class QLineEdit;
class QPushButton;

class LicensePage : public ConfigurationPage
{
	Q_OBJECT
public:
	explicit LicensePage(QWidget* parent = nullptr);

	void resetWidgets() override;
	void connectWidgetsToProperties() override;
	void applyConfiguration() override;

private:
	void refresh();
	void activate();
	void checkNow();
	void setBusy(bool busy);

	QLabel* m_statusLabel{nullptr};
	QLabel* m_detailsLabel{nullptr};
	QLabel* m_resultLabel{nullptr};
	QLineEdit* m_codeEdit{nullptr};
	QPushButton* m_activateButton{nullptr};
	QPushButton* m_checkButton{nullptr};
};
```

`configurator/src/LicensePage.cpp`:

```cpp
/*
 * LicensePage.cpp - licence activation and status page
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "LicensePage.h"
#include "LicenseService.h"

LicensePage::LicensePage(QWidget* parent) :
	ConfigurationPage(parent)
{
	setWindowTitle(tr("Licence"));
	setWindowIcon(QIcon(QStringLiteral(":/core/license.png")));

	auto layout = new QVBoxLayout(this);

	auto statusBox = new QGroupBox(tr("Status"), this);
	auto statusLayout = new QVBoxLayout(statusBox);
	m_statusLabel = new QLabel(statusBox);
	m_statusLabel->setWordWrap(true);
	m_detailsLabel = new QLabel(statusBox);
	m_detailsLabel->setWordWrap(true);
	m_detailsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	m_checkButton = new QPushButton(tr("Check now"), statusBox);
	statusLayout->addWidget(m_statusLabel);
	statusLayout->addWidget(m_detailsLabel);
	statusLayout->addWidget(m_checkButton, 0, Qt::AlignLeft);

	auto activationBox = new QGroupBox(tr("Activation"), this);
	auto activationLayout = new QFormLayout(activationBox);
	m_codeEdit = new QLineEdit(activationBox);
	m_codeEdit->setPlaceholderText(QStringLiteral("KHW-..."));
	m_activateButton = new QPushButton(tr("Activate"), activationBox);
	activationLayout->addRow(tr("Activation code"), m_codeEdit);
	activationLayout->addRow(QString(), m_activateButton);

	m_resultLabel = new QLabel(this);
	m_resultLabel->setWordWrap(true);

	layout->addWidget(statusBox);
	layout->addWidget(activationBox);
	layout->addWidget(m_resultLabel);
	layout->addStretch();

	connect(m_activateButton, &QPushButton::clicked, this, &LicensePage::activate);
	connect(m_checkButton, &QPushButton::clicked, this, &LicensePage::checkNow);

	refresh();
}

void LicensePage::resetWidgets()
{
	refresh();
}

void LicensePage::connectWidgetsToProperties()
{
}

void LicensePage::applyConfiguration()
{
}

void LicensePage::refresh()
{
	const auto snapshot = LicenseService::snapshot();
	m_statusLabel->setText(LicenseService::describe(snapshot.state));

	if (!snapshot.claims)
	{
		m_detailsLabel->clear();
		return;
	}

	const auto& c = *snapshot.claims;
	const auto format = [](const QDateTime& dateTime) -> QString {
		return QLocale().toString(dateTime.toLocalTime(), QLocale::ShortFormat);
	};
	const QString details = tr("School: %1").arg(c.tenantName) + QLatin1Char('\n') +
							tr("Device quota: %1").arg(c.maxDevices) + QLatin1Char('\n') +
							tr("Subscription until: %1").arg(format(c.subscriptionEnd)) + QLatin1Char('\n') +
							tr("Licence renewal due: %1").arg(format(c.expiresAt)) + QLatin1Char('\n') +
							tr("Computer ID: %1").arg(snapshot.masterId);
	m_detailsLabel->setText(details);
}

void LicensePage::activate()
{
	const QString code = m_codeEdit->text().trimmed();
	if (code.isEmpty())
	{
		return;
	}
	setBusy(true);
	const auto result = LicenseService::activate(code);
	setBusy(false);
	m_resultLabel->setText(LicenseService::describe(result));
	if (result == LicenseActionResult::Ok)
	{
		m_codeEdit->clear();
	}
	refresh();
}

void LicensePage::checkNow()
{
	setBusy(true);
	const auto result = LicenseService::checkIn({});
	setBusy(false);
	m_resultLabel->setText(LicenseService::describe(result));
	refresh();
}

void LicensePage::setBusy(bool busy)
{
	m_activateButton->setEnabled(!busy);
	m_checkButton->setEnabled(!busy);
	m_codeEdit->setEnabled(!busy);
}
```

Periksa dulu bahwa `:/core/license.png` memang ada di `core/resources/core.qrc` (terlihat di daftar resource). Bila tidak, pakai ikon lain yang sudah ada.

- [ ] **Step 2: Daftarkan halaman di `MainWindow`**

Di `configurator/src/MainWindow.cpp`, `#include "LicensePage.h"`, lalu di akhir `loadConfigurationPagePlugins()` sebelum baris "adjust minimum size", tambahkan halaman dengan pola yang sama seperti halaman plugin:

```cpp
	// The licence page is built in, not a plugin: deleting a plugin must not
	// be a way to hide the licence state.
	auto licensePage = new LicensePage;
	ui->configPages->addWidget(licensePage);
	auto licenseItem = new QListWidgetItem(licensePage->windowIcon(), licensePage->windowTitle());
	licenseItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	licenseItem->setData(Qt::UserRole, QVariant::fromValue<ConfigurationPage*>(licensePage));
	ui->pageSelector->addItem(licenseItem);
```

Samakan argumen `setData` dengan yang dipakai halaman plugin di fungsi yang sama.

- [ ] **Step 3: Terjemahan Indonesia**

Di `translations/veyon_id.ts`, tambahkan `<context>` untuk `LicenseService` dan `LicensePage` berisi setiap string `tr()` dari Task 4–5 dengan terjemahan Indonesia. Pesan `SubscriptionUnverified` harus berbunyi "tidak dapat memverifikasi pembayaran", tidak pernah "belum membayar". Ikuti format `<message><source>…</source><translation>…</translation></message>` yang sudah ada di file itu.

- [ ] **Step 4: Build seluruh Configurator**

```bash
docker exec veyon-dev bash -lc 'cd /veyon/build-tests && cmake .. >/dev/null && make -j10 veyon-configurator license-test && ./tests/unit/license-test'
```

Expected: Configurator ter-build tanpa error; seluruh test tetap lulus.

- [ ] **Step 5: Commit**

```bash
git add configurator/src/LicensePage.h configurator/src/LicensePage.cpp configurator/src/MainWindow.cpp translations/veyon_id.ts
git commit -m "feat: add the licence page to the Veyon Configurator"
```

---

## Verifikasi end-to-end (controller)

Dilakukan controller setelah Task 5, dengan izin pemilik untuk membuat tenant uji di produksi:

1. Buat tenant uji dan satu kode aktivasi di produksi.
2. Jalankan Configurator di container, buka lewat noVNC (`http://localhost:6080`), masukkan kode, klik **Activate**.
3. Pastikan: status menampilkan nama sekolah uji; `KhwarizmiLicense.json` sistem berisi `masterId`/`secret`/`activationToken`; file ekspor "Save settings to file" **tidak** memuat `Secret` maupun `MasterId`.
4. Klik **Check now**; pastikan `lastServerTime` di cache user naik dan token cache diperbarui.
5. Coba aktivasi ulang dengan kode yang sama → pesan "kode tidak valid, kedaluwarsa, atau sudah dipakai".
6. Hapus tenant uji dari produksi.

## Setelah rencana ini selesai

Configurator bisa mengaktivasi dan menampilkan status. Belum ada yang memblokir apa pun, dan master belum check-in.

- **2c:** gerbang saat master start (`master/src/main.cpp`), check-in harian dari master dengan daftar device dan backoff, banner peringatan, kuota lokal di `ComputerControlListModel.cpp:347`.
- Verifikasi di Windows sungguhan: master (user biasa) harus bisa **membaca** `KhwarizmiLicense.json` di scope System. Bila ACL `ProgramData` mencegahnya, master tidak bisa check-in dan akan tersuspend diam-diam setelah 21 hari. Ini hanya bisa dibuktikan di Windows.
