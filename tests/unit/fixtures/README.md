# Fixture token lisensi

Dihasilkan oleh `scripts/generate-client-fixtures.ts` di repo `khwarizmi-license`,
memakai `signToken()` yang sama dengan backend produksi dan **keypair uji
sekali-pakai** — bukan kunci produksi. Private key uji tidak disimpan.

Regenerasi hanya bila format token berubah:

    npx tsx scripts/generate-client-fixtures.ts <path-ke>/veyon/tests/unit/fixtures

Semua token memakai `iat = 1790000000` (2026-09-21 UTC); test menyuntikkan waktu
sendiri, jadi fixture tidak pernah "kedaluwarsa".
