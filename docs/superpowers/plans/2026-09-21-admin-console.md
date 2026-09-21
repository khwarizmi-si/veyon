# Admin Console Khwarizmi — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Admin console internal untuk mengelola sekolah, kuota, dan kode aktivasi — menggantikan SQL manual di `scripts/admin.sql.md`.

**Architecture:** Rute `/admin/*` di Worker lisensi yang sudah ada, dilindungi Cloudflare Access dan diverifikasi ulang di dalam Worker terhadap JWKS tim (fail closed). UI adalah HTML server-rendered dari Hono dengan form biasa — tanpa framework klien, tanpa bundler, tanpa JavaScript klien.

**Tech Stack:** TypeScript, Hono, Cloudflare Workers, D1, Cloudflare Access (Zero Trust), Vitest + `@cloudflare/vitest-pool-workers`.

**Repo:** `/Volumes/rherdians/programming/alkhwarizmi/khwarizmi-license`, branch `feat/license-backend` (lanjutan; PR #1 masih terbuka).

**Spec sistem:** `docs/superpowers/specs/2026-09-19-khwarizmi-saas-licensing-design.md` (repo veyon).

---

## Global Constraints

Berlaku untuk **setiap** task. Nilai disalin verbatim dari sistem yang sudah berjalan.

- **Fail closed.** Tanpa `Cf-Access-Jwt-Assertion` yang valid, `/admin/*` membalas **403**. Tidak ada flag bypass untuk development, tidak ada mode "izinkan kalau env X". Test memakai JWT yang ditandatangani keypair uji.
- **Every D1 query uses a prepared statement with `.bind()`.** Tidak ada SQL dirangkai dari input.
- Semua timestamp disimpan sebagai string **ISO-8601 UTC** (mis. `2026-10-19T00:00:00.000Z`).
- Jendela device aktif **30 hari** — pakai `ACTIVE_WINDOW_DAYS` dari `src/devices.ts`, jangan hardcode.
- Kode aktivasi dibangkitkan dengan **CSPRNG minimal 128 bit** (`crypto.getRandomValues`), tidak pernah berpola atau berurutan.
- **INVARIANT — jalur lisensi tetap steril.** Console tidak boleh menambah kolom atau endpoint yang membawa data pemantauan (screenshot, aktivitas murid, nama pengguna).
- **INVARIANT — kegagalan condong membiarkan sekolah bekerja.** Console tidak boleh mengubah perilaku `/v1/activate` dan `/v1/checkin`.
- Endpoint publik yang sudah ada (`/v1/activate`, `/v1/checkin`, `/health`) **tidak boleh berubah perilakunya**. 58 test yang ada harus tetap hijau.
- Token lisensi tidak disentuh sama sekali: algoritma RS512, klaim tetap sepuluh, tanpa `status`.

## Prasyarat yang dikerjakan pemilik repo

Tidak bisa diotomatiskan dari sini — butuh dashboard Cloudflare Zero Trust:

1. Buat Access application untuk `license.khwarizmi.co.id/admin/*`, policy: email pemilik.
2. Catat **team domain** (mis. `khwarizmi.cloudflareaccess.com`) dan **Audience (AUD) tag**.
3. Set keduanya sebagai vars Worker: `ACCESS_TEAM_DOMAIN`, `ACCESS_AUD`.

Task 1–3 dapat dikerjakan dan diuji penuh tanpa langkah ini; hanya penggunaan nyata yang menunggu.

---

### Task 1: Verifikasi JWT Cloudflare Access

**Files:**
- Create: `src/admin/access.ts`
- Create: `test/admin-access.test.ts`
- Modify: `src/types.ts` (tambah dua var ke `Env`)

**Interfaces:**
- Consumes: —
- Produces:
  - `interface AccessClaims { email: string; sub: string; aud: string[]; iss: string; exp: number; iat: number }`
  - `verifyAccessJwt(token: string, opts: { teamDomain: string; aud: string; now: Date; jwks: JsonWebKey[] }): Promise<AccessClaims | null>`
  - `fetchAccessJwks(teamDomain: string, cache: Map<string, { keys: JsonWebKey[]; fetchedAt: number }>): Promise<JsonWebKey[]>`
  - `JWKS_TTL_MS = 3_600_000`

- [ ] **Step 1: Tambah var ke `Env`**

Di `src/types.ts`, ubah `Env` menjadi:

```typescript
export interface Env {
  DB: D1Database;
  LICENSE_PRIVATE_KEY: string;
  LICENSE_KID: string;
  LICENSE_ISSUER: string;
  ACCESS_TEAM_DOMAIN: string;
  ACCESS_AUD: string;
}
```

- [ ] **Step 2: Tulis test yang gagal**

`test/admin-access.test.ts`:

```typescript
import { describe, it, expect, beforeAll } from 'vitest';
import { verifyAccessJwt, fetchAccessJwks, JWKS_TTL_MS } from '../src/admin/access';

const TEAM = 'khwarizmi.cloudflareaccess.com';
const AUD = 'aud-tag-under-test';
const NOW = new Date('2026-09-21T00:00:00.000Z');
const ALG = { name: 'RSASSA-PKCS1-v1_5', hash: 'SHA-256' } as const;

let privateKey: CryptoKey;
let jwks: JsonWebKey[];
let otherPrivateKey: CryptoKey;

function b64url(bytes: Uint8Array): string {
  let s = '';
  for (const b of bytes) s += String.fromCharCode(b);
  return btoa(s).replace(/\+/g, '-').replace(/\//g, '_').replace(/=+$/, '');
}

async function makeJwt(
  claims: Record<string, unknown>,
  key: CryptoKey = privateKey,
  kid = 'test-kid'
): Promise<string> {
  const enc = new TextEncoder();
  const header = b64url(enc.encode(JSON.stringify({ alg: 'RS256', typ: 'JWT', kid })));
  const body = b64url(enc.encode(JSON.stringify(claims)));
  const sig = await crypto.subtle.sign(ALG, key, enc.encode(`${header}.${body}`));
  return `${header}.${body}.${b64url(new Uint8Array(sig))}`;
}

function validClaims(overrides: Record<string, unknown> = {}) {
  const iat = Math.floor(NOW.getTime() / 1000);
  return {
    iss: `https://${TEAM}`,
    aud: [AUD],
    email: 'owner@khwarizmi.co.id',
    sub: 'user-1',
    iat,
    exp: iat + 3600,
    ...overrides,
  };
}

beforeAll(async () => {
  const pair = (await crypto.subtle.generateKey(
    { ...ALG, modulusLength: 2048, publicExponent: new Uint8Array([1, 0, 1]) },
    true,
    ['sign', 'verify']
  )) as CryptoKeyPair;
  privateKey = pair.privateKey;
  const jwk = await crypto.subtle.exportKey('jwk', pair.publicKey);
  jwks = [{ ...jwk, kid: 'test-kid', alg: 'RS256', use: 'sig' }];

  const other = (await crypto.subtle.generateKey(
    { ...ALG, modulusLength: 2048, publicExponent: new Uint8Array([1, 0, 1]) },
    true,
    ['sign', 'verify']
  )) as CryptoKeyPair;
  otherPrivateKey = other.privateKey;
});

describe('verifyAccessJwt', () => {
  const opts = () => ({ teamDomain: TEAM, aud: AUD, now: NOW, jwks });

  it('accepts a valid token and returns its claims', async () => {
    const claims = await verifyAccessJwt(await makeJwt(validClaims()), opts());
    expect(claims).not.toBeNull();
    expect(claims!.email).toBe('owner@khwarizmi.co.id');
  });

  it('rejects a token signed by a different key', async () => {
    const token = await makeJwt(validClaims(), otherPrivateKey);
    expect(await verifyAccessJwt(token, opts())).toBeNull();
  });

  it('rejects a token whose kid is not in the JWKS', async () => {
    const token = await makeJwt(validClaims(), privateKey, 'unknown-kid');
    expect(await verifyAccessJwt(token, opts())).toBeNull();
  });

  it('rejects a token for a different audience', async () => {
    const token = await makeJwt(validClaims({ aud: ['someone-elses-aud'] }));
    expect(await verifyAccessJwt(token, opts())).toBeNull();
  });

  it('rejects a token from a different team domain', async () => {
    const token = await makeJwt(validClaims({ iss: 'https://evil.cloudflareaccess.com' }));
    expect(await verifyAccessJwt(token, opts())).toBeNull();
  });

  it('rejects an expired token', async () => {
    const iat = Math.floor(NOW.getTime() / 1000) - 7200;
    const token = await makeJwt(validClaims({ iat, exp: iat + 3600 }));
    expect(await verifyAccessJwt(token, opts())).toBeNull();
  });

  it('rejects a token that is not yet valid', async () => {
    const iat = Math.floor(NOW.getTime() / 1000) + 3600;
    const token = await makeJwt(validClaims({ iat, exp: iat + 3600, nbf: iat }));
    expect(await verifyAccessJwt(token, opts())).toBeNull();
  });

  it('rejects malformed input without throwing', async () => {
    expect(await verifyAccessJwt('', opts())).toBeNull();
    expect(await verifyAccessJwt('a.b', opts())).toBeNull();
    expect(await verifyAccessJwt('not-a-token', opts())).toBeNull();
    expect(await verifyAccessJwt('!!!.!!!.!!!', opts())).toBeNull();
  });

  it('rejects when the JWKS is empty', async () => {
    const token = await makeJwt(validClaims());
    expect(await verifyAccessJwt(token, { ...opts(), jwks: [] })).toBeNull();
  });
});

describe('fetchAccessJwks', () => {
  it('fetches, caches, and reuses within the TTL', async () => {
    const cache = new Map();
    let calls = 0;
    const originalFetch = globalThis.fetch;
    globalThis.fetch = (async () => {
      calls++;
      return new Response(JSON.stringify({ keys: jwks }), {
        headers: { 'content-type': 'application/json' },
      });
    }) as typeof fetch;

    try {
      expect(await fetchAccessJwks(TEAM, cache)).toHaveLength(1);
      await fetchAccessJwks(TEAM, cache);
      expect(calls).toBe(1);

      const entry = cache.get(TEAM)!;
      entry.fetchedAt = Date.now() - JWKS_TTL_MS - 1;
      await fetchAccessJwks(TEAM, cache);
      expect(calls).toBe(2);
    } finally {
      globalThis.fetch = originalFetch;
    }
  });

  it('returns an empty array when the endpoint fails', async () => {
    const originalFetch = globalThis.fetch;
    globalThis.fetch = (async () => new Response('nope', { status: 500 })) as typeof fetch;
    try {
      expect(await fetchAccessJwks(TEAM, new Map())).toEqual([]);
    } finally {
      globalThis.fetch = originalFetch;
    }
  });
});
```

- [ ] **Step 3: Jalankan test untuk memastikan gagal**

Run: `npm test -- admin-access`
Expected: FAIL — `Failed to resolve import "../src/admin/access"`

- [ ] **Step 4: Implementasi**

`src/admin/access.ts`:

```typescript
/**
 * Verifikasi JWT Cloudflare Access.
 *
 * Access sudah menahan permintaan di edge, tetapi Worker memverifikasi ulang:
 * bila policy Access salah konfigurasi, dihapus, atau rute tidak tercakup,
 * /admin/* tetap menolak. Tidak ada jalur bypass — tanpa JWT valid, 403.
 */

export const JWKS_TTL_MS = 3_600_000;

const ALG = { name: 'RSASSA-PKCS1-v1_5', hash: 'SHA-256' } as const;

export interface AccessClaims {
  email: string;
  sub: string;
  aud: string[];
  iss: string;
  exp: number;
  iat: number;
}

export interface JwksCacheEntry {
  keys: JsonWebKey[];
  fetchedAt: number;
}

function base64UrlDecode(input: string): Uint8Array {
  const padded = input.replace(/-/g, '+').replace(/_/g, '/');
  const str = atob(padded);
  const bytes = new Uint8Array(str.length);
  for (let i = 0; i < str.length; i++) bytes[i] = str.charCodeAt(i);
  return bytes;
}

export async function fetchAccessJwks(
  teamDomain: string,
  cache: Map<string, JwksCacheEntry>
): Promise<JsonWebKey[]> {
  const cached = cache.get(teamDomain);
  if (cached && Date.now() - cached.fetchedAt < JWKS_TTL_MS) return cached.keys;

  try {
    const res = await fetch(`https://${teamDomain}/cdn-cgi/access/certs`);
    if (!res.ok) return [];
    const body = (await res.json()) as { keys?: JsonWebKey[] };
    const keys = Array.isArray(body.keys) ? body.keys : [];
    cache.set(teamDomain, { keys, fetchedAt: Date.now() });
    return keys;
  } catch {
    return [];
  }
}

export async function verifyAccessJwt(
  token: string,
  opts: { teamDomain: string; aud: string; now: Date; jwks: JsonWebKey[] }
): Promise<AccessClaims | null> {
  const parts = token.split('.');
  if (parts.length !== 3) return null;
  const [headerPart, bodyPart, signaturePart] = parts;

  try {
    const header = JSON.parse(new TextDecoder().decode(base64UrlDecode(headerPart))) as {
      kid?: string;
      alg?: string;
    };
    if (header.alg !== 'RS256' || !header.kid) return null;

    const jwk = opts.jwks.find((k) => (k as { kid?: string }).kid === header.kid);
    if (!jwk) return null;

    const key = await crypto.subtle.importKey('jwk', jwk, ALG, false, ['verify']);
    const ok = await crypto.subtle.verify(
      ALG,
      key,
      base64UrlDecode(signaturePart),
      new TextEncoder().encode(`${headerPart}.${bodyPart}`)
    );
    if (!ok) return null;

    const claims = JSON.parse(new TextDecoder().decode(base64UrlDecode(bodyPart))) as Record<
      string,
      unknown
    >;

    if (claims.iss !== `https://${opts.teamDomain}`) return null;

    const aud = Array.isArray(claims.aud) ? claims.aud : [claims.aud];
    if (!aud.includes(opts.aud)) return null;

    const nowSec = Math.floor(opts.now.getTime() / 1000);
    if (typeof claims.exp !== 'number' || claims.exp <= nowSec) return null;
    if (typeof claims.nbf === 'number' && claims.nbf > nowSec) return null;
    if (typeof claims.iat === 'number' && claims.iat > nowSec + 60) return null;
    if (typeof claims.email !== 'string' || typeof claims.sub !== 'string') return null;

    return {
      email: claims.email,
      sub: claims.sub,
      aud: aud as string[],
      iss: claims.iss as string,
      exp: claims.exp,
      iat: claims.iat as number,
    };
  } catch {
    return null;
  }
}
```

- [ ] **Step 5: Jalankan test**

Run: `npm test -- admin-access`
Expected: PASS, sebelas test hijau.

- [ ] **Step 6: Jalankan seluruh suite**

Run: `npm test`
Expected: PASS, 58 test lama + 11 baru.

- [ ] **Step 7: Commit**

```bash
git add src/admin/access.ts src/types.ts test/admin-access.test.ts
git commit -m "feat: verify Cloudflare Access JWTs for the admin console"
```

---

### Task 2: Kueri admin

**Files:**
- Create: `src/admin/queries.ts`
- Create: `test/admin-queries.test.ts`

**Interfaces:**
- Consumes: `TenantRow` (`src/types.ts`), `ACTIVE_WINDOW_DAYS` (`src/devices.ts`), `randomId` (`src/db.ts`)
- Produces:
  - `interface SchoolSummary extends TenantRow { active_devices: number; master_count: number }`
  - `listSchools(db: D1Database, now: Date): Promise<SchoolSummary[]>`
  - `createSchool(db, input: { name: string; maxDevices: number; trialDays: number }, now: Date): Promise<string>`
  - `setQuota(db, tenantId: string, maxDevices: number): Promise<boolean>`
  - `setSubscription(db, tenantId: string, plan: Plan, subEnd: Date): Promise<boolean>`
  - `issueActivationCode(db, tenantId: string, validDays: number, now: Date): Promise<string | null>`
  - `listActivationCodes(db, tenantId: string): Promise<ActivationCodeRow[]>`
  - `revokeActivationCode(db, code: string): Promise<boolean>`
  - `generateActivationCode(): string`

- [ ] **Step 1: Tulis test yang gagal**

`test/admin-queries.test.ts`:

```typescript
import { env, applyD1Migrations } from 'cloudflare:test';
import { describe, it, expect, beforeAll, beforeEach } from 'vitest';
import {
  listSchools,
  createSchool,
  setQuota,
  setSubscription,
  issueActivationCode,
  listActivationCodes,
  revokeActivationCode,
  generateActivationCode,
} from '../src/admin/queries';

const NOW = new Date('2026-09-21T00:00:00.000Z');

beforeAll(async () => {
  await applyD1Migrations(env.DB, env.TEST_MIGRATIONS);
});

beforeEach(async () => {
  await env.DB.batch([
    env.DB.prepare('DELETE FROM device'),
    env.DB.prepare('DELETE FROM master'),
    env.DB.prepare('DELETE FROM activation_code'),
    env.DB.prepare('DELETE FROM tenant'),
  ]);
});

describe('generateActivationCode', () => {
  it('produces a prefixed, high-entropy, non-sequential code', () => {
    const a = generateActivationCode();
    const b = generateActivationCode();
    expect(a).toMatch(/^KHW-[0-9A-HJ-NP-Z]{26}$/);
    expect(a).not.toBe(b);
  });
});

describe('createSchool and listSchools', () => {
  it('creates a school on trial and lists it with zero usage', async () => {
    const id = await createSchool(env.DB, { name: 'SMAN 1 Bandung', maxDevices: 40, trialDays: 30 }, NOW);
    const schools = await listSchools(env.DB, NOW);
    expect(schools).toHaveLength(1);
    expect(schools[0].id).toBe(id);
    expect(schools[0].name).toBe('SMAN 1 Bandung');
    expect(schools[0].plan).toBe('trial');
    expect(schools[0].max_devices).toBe(40);
    expect(schools[0].active_devices).toBe(0);
    expect(schools[0].master_count).toBe(0);
    expect(new Date(schools[0].sub_end).getTime()).toBe(NOW.getTime() + 30 * 86_400_000);
  });

  it('counts only devices seen inside the 30 day window', async () => {
    const id = await createSchool(env.DB, { name: 'A', maxDevices: 10, trialDays: 30 }, NOW);
    const fresh = new Date(NOW.getTime() - 86_400_000).toISOString();
    const stale = new Date(NOW.getTime() - 31 * 86_400_000).toISOString();
    await env.DB.batch([
      env.DB.prepare(
        `INSERT INTO device (id,tenant_id,mac,hostname,first_seen_at,last_seen_at)
         VALUES ('d1',?1,NULL,'fresh',?2,?2)`
      ).bind(id, fresh),
      env.DB.prepare(
        `INSERT INTO device (id,tenant_id,mac,hostname,first_seen_at,last_seen_at)
         VALUES ('d2',?1,NULL,'stale',?2,?2)`
      ).bind(id, stale),
    ]);
    const schools = await listSchools(env.DB, NOW);
    expect(schools[0].active_devices).toBe(1);
  });

  it('counts masters per school and keeps schools separate', async () => {
    const a = await createSchool(env.DB, { name: 'A', maxDevices: 10, trialDays: 30 }, NOW);
    const b = await createSchool(env.DB, { name: 'B', maxDevices: 10, trialDays: 30 }, NOW);
    await env.DB.prepare(
      `INSERT INTO master (id,tenant_id,secret_hash,hostname,version,last_seen_at,created_at)
       VALUES ('m1',?1,'hash',NULL,NULL,?2,?2)`
    )
      .bind(a, NOW.toISOString())
      .run();
    const schools = await listSchools(env.DB, NOW);
    const byId = Object.fromEntries(schools.map((s) => [s.id, s]));
    expect(byId[a].master_count).toBe(1);
    expect(byId[b].master_count).toBe(0);
  });
});

describe('setQuota and setSubscription', () => {
  it('updates the quota', async () => {
    const id = await createSchool(env.DB, { name: 'A', maxDevices: 10, trialDays: 30 }, NOW);
    expect(await setQuota(env.DB, id, 60)).toBe(true);
    expect((await listSchools(env.DB, NOW))[0].max_devices).toBe(60);
  });

  it('reports false for an unknown school', async () => {
    expect(await setQuota(env.DB, 'sch_nope', 60)).toBe(false);
    expect(await setSubscription(env.DB, 'sch_nope', 'paid', NOW)).toBe(false);
  });

  it('moves a school to paid with a new end date', async () => {
    const id = await createSchool(env.DB, { name: 'A', maxDevices: 10, trialDays: 30 }, NOW);
    const end = new Date('2027-09-21T00:00:00.000Z');
    expect(await setSubscription(env.DB, id, 'paid', end)).toBe(true);
    const school = (await listSchools(env.DB, NOW))[0];
    expect(school.plan).toBe('paid');
    expect(school.sub_end).toBe(end.toISOString());
  });
});

describe('activation codes', () => {
  it('issues a code tied to the school', async () => {
    const id = await createSchool(env.DB, { name: 'A', maxDevices: 10, trialDays: 30 }, NOW);
    const code = await issueActivationCode(env.DB, id, 30, NOW);
    expect(code).toMatch(/^KHW-/);
    const codes = await listActivationCodes(env.DB, id);
    expect(codes).toHaveLength(1);
    expect(codes[0].code).toBe(code);
    expect(codes[0].used_at).toBeNull();
    expect(new Date(codes[0].expires_at).getTime()).toBe(NOW.getTime() + 30 * 86_400_000);
  });

  it('refuses to issue a code for an unknown school', async () => {
    expect(await issueActivationCode(env.DB, 'sch_nope', 30, NOW)).toBeNull();
  });

  it('revokes an unused code', async () => {
    const id = await createSchool(env.DB, { name: 'A', maxDevices: 10, trialDays: 30 }, NOW);
    const code = (await issueActivationCode(env.DB, id, 30, NOW))!;
    expect(await revokeActivationCode(env.DB, code)).toBe(true);
    expect(await listActivationCodes(env.DB, id)).toHaveLength(0);
  });

  it('refuses to revoke a code that has already been used', async () => {
    const id = await createSchool(env.DB, { name: 'A', maxDevices: 10, trialDays: 30 }, NOW);
    const code = (await issueActivationCode(env.DB, id, 30, NOW))!;
    await env.DB.prepare('UPDATE activation_code SET used_at = ?1 WHERE code = ?2')
      .bind(NOW.toISOString(), code)
      .run();
    expect(await revokeActivationCode(env.DB, code)).toBe(false);
    expect(await listActivationCodes(env.DB, id)).toHaveLength(1);
  });
});
```

- [ ] **Step 2: Jalankan test untuk memastikan gagal**

Run: `npm test -- admin-queries`
Expected: FAIL — `Failed to resolve import "../src/admin/queries"`

- [ ] **Step 3: Implementasi**

`src/admin/queries.ts`:

```typescript
import type { Plan, TenantRow } from '../types';
import { ACTIVE_WINDOW_DAYS } from '../devices';
import { randomId } from '../db';

/** Crockford base32 tanpa I, L, O, U — tidak ada karakter yang tertukar saat dibaca lewat telepon. */
const CODE_ALPHABET = '0123456789ABCDEFGHJKMNPQRSTVWXYZ';
const CODE_LENGTH = 26; // 26 * 5 bit = 130 bit entropi

export interface SchoolSummary extends TenantRow {
  active_devices: number;
  master_count: number;
}

export interface ActivationCodeRow {
  code: string;
  tenant_id: string;
  used_at: string | null;
  expires_at: string;
  created_at: string;
}

export function generateActivationCode(): string {
  const bytes = new Uint8Array(CODE_LENGTH);
  crypto.getRandomValues(bytes);
  let out = '';
  for (const b of bytes) out += CODE_ALPHABET[b % CODE_ALPHABET.length];
  return `KHW-${out}`;
}

export async function listSchools(db: D1Database, now: Date): Promise<SchoolSummary[]> {
  const cutoff = new Date(now.getTime() - ACTIVE_WINDOW_DAYS * 86_400_000).toISOString();
  const { results } = await db
    .prepare(
      `SELECT t.*,
              (SELECT COUNT(*) FROM device d
                WHERE d.tenant_id = t.id AND d.last_seen_at >= ?1) AS active_devices,
              (SELECT COUNT(*) FROM master m WHERE m.tenant_id = t.id) AS master_count
         FROM tenant t
        ORDER BY t.name`
    )
    .bind(cutoff)
    .all<SchoolSummary>();
  return results;
}

export async function createSchool(
  db: D1Database,
  input: { name: string; maxDevices: number; trialDays: number },
  now: Date
): Promise<string> {
  const id = randomId('sch');
  const nowIso = now.toISOString();
  const subEnd = new Date(now.getTime() + input.trialDays * 86_400_000).toISOString();
  await db
    .prepare(
      `INSERT INTO tenant (id, name, plan, max_devices, sub_end, overage_since, created_at)
       VALUES (?1, ?2, 'trial', ?3, ?4, NULL, ?5)`
    )
    .bind(id, input.name, input.maxDevices, subEnd, nowIso)
    .run();
  return id;
}

export async function setQuota(
  db: D1Database,
  tenantId: string,
  maxDevices: number
): Promise<boolean> {
  const res = await db
    .prepare('UPDATE tenant SET max_devices = ?1 WHERE id = ?2')
    .bind(maxDevices, tenantId)
    .run();
  return res.meta.changes === 1;
}

export async function setSubscription(
  db: D1Database,
  tenantId: string,
  plan: Plan,
  subEnd: Date
): Promise<boolean> {
  const res = await db
    .prepare('UPDATE tenant SET plan = ?1, sub_end = ?2 WHERE id = ?3')
    .bind(plan, subEnd.toISOString(), tenantId)
    .run();
  return res.meta.changes === 1;
}

export async function issueActivationCode(
  db: D1Database,
  tenantId: string,
  validDays: number,
  now: Date
): Promise<string | null> {
  const tenant = await db
    .prepare('SELECT id FROM tenant WHERE id = ?1')
    .bind(tenantId)
    .first<{ id: string }>();
  if (!tenant) return null;

  const code = generateActivationCode();
  const nowIso = now.toISOString();
  const expires = new Date(now.getTime() + validDays * 86_400_000).toISOString();
  await db
    .prepare(
      `INSERT INTO activation_code (code, tenant_id, used_at, expires_at, created_at)
       VALUES (?1, ?2, NULL, ?3, ?4)`
    )
    .bind(code, tenantId, expires, nowIso)
    .run();
  return code;
}

export async function listActivationCodes(
  db: D1Database,
  tenantId: string
): Promise<ActivationCodeRow[]> {
  const { results } = await db
    .prepare('SELECT * FROM activation_code WHERE tenant_id = ?1 ORDER BY created_at DESC')
    .bind(tenantId)
    .all<ActivationCodeRow>();
  return results;
}

/** Hanya kode yang belum terpakai yang boleh dicabut — kode terpakai adalah catatan riwayat. */
export async function revokeActivationCode(db: D1Database, code: string): Promise<boolean> {
  const res = await db
    .prepare('DELETE FROM activation_code WHERE code = ?1 AND used_at IS NULL')
    .bind(code)
    .run();
  return res.meta.changes === 1;
}
```

- [ ] **Step 4: Jalankan test**

Run: `npm test -- admin-queries`
Expected: PASS, sebelas test hijau.

- [ ] **Step 5: Commit**

```bash
git add src/admin/queries.ts test/admin-queries.test.ts
git commit -m "feat: add admin queries for schools, quota, and activation codes"
```

---

### Task 3: Rute admin, middleware auth, dan tampilan HTML

**Files:**
- Create: `src/admin/views.ts`
- Create: `src/admin/routes.ts`
- Modify: `src/index.ts`
- Create: `test/admin-routes.test.ts`

**Interfaces:**
- Consumes: `verifyAccessJwt`, `fetchAccessJwks` (Task 1); semua fungsi Task 2
- Produces: Hono app `admin` yang dipasang di `/admin`

- [ ] **Step 1: Tulis test yang gagal**

`test/admin-routes.test.ts`:

```typescript
import { env, applyD1Migrations } from 'cloudflare:test';
import { describe, it, expect, beforeAll, beforeEach } from 'vitest';
import app from '../src/index';
import { createSchool } from '../src/admin/queries';

const TEAM = 'khwarizmi.cloudflareaccess.com';
const AUD = 'aud-tag-under-test';
const ORIGIN = 'https://license.khwarizmi.co.id';
const ALG = { name: 'RSASSA-PKCS1-v1_5', hash: 'SHA-256' } as const;

let jwt: string;
let adminEnv: Record<string, unknown>;

function b64url(bytes: Uint8Array): string {
  let s = '';
  for (const b of bytes) s += String.fromCharCode(b);
  return btoa(s).replace(/\+/g, '-').replace(/\//g, '_').replace(/=+$/, '');
}

beforeAll(async () => {
  await applyD1Migrations(env.DB, env.TEST_MIGRATIONS);

  const pair = (await crypto.subtle.generateKey(
    { ...ALG, modulusLength: 2048, publicExponent: new Uint8Array([1, 0, 1]) },
    true,
    ['sign', 'verify']
  )) as CryptoKeyPair;
  const jwk = await crypto.subtle.exportKey('jwk', pair.publicKey);
  const keys = [{ ...jwk, kid: 'test-kid', alg: 'RS256', use: 'sig' }];

  const iat = Math.floor(Date.now() / 1000);
  const enc = new TextEncoder();
  const header = b64url(enc.encode(JSON.stringify({ alg: 'RS256', typ: 'JWT', kid: 'test-kid' })));
  const body = b64url(
    enc.encode(
      JSON.stringify({
        iss: `https://${TEAM}`,
        aud: [AUD],
        email: 'owner@khwarizmi.co.id',
        sub: 'user-1',
        iat,
        exp: iat + 3600,
      })
    )
  );
  const sig = await crypto.subtle.sign(ALG, pair.privateKey, enc.encode(`${header}.${body}`));
  jwt = `${header}.${body}.${b64url(new Uint8Array(sig))}`;

  // Serve the test JWKS in place of the real Access certs endpoint.
  const originalFetch = globalThis.fetch;
  globalThis.fetch = (async (input: RequestInfo | URL) => {
    const url = String(input);
    if (url.includes('/cdn-cgi/access/certs')) {
      return new Response(JSON.stringify({ keys }), {
        headers: { 'content-type': 'application/json' },
      });
    }
    return originalFetch(input as RequestInfo);
  }) as typeof fetch;

  adminEnv = { ...env, ACCESS_TEAM_DOMAIN: TEAM, ACCESS_AUD: AUD };
});

beforeEach(async () => {
  await env.DB.batch([
    env.DB.prepare('DELETE FROM device'),
    env.DB.prepare('DELETE FROM master'),
    env.DB.prepare('DELETE FROM activation_code'),
    env.DB.prepare('DELETE FROM tenant'),
  ]);
});

function get(path: string, headers: Record<string, string> = {}) {
  return app.request(`${ORIGIN}${path}`, { headers }, adminEnv);
}

function post(path: string, form: Record<string, string>, headers: Record<string, string> = {}) {
  return app.request(
    `${ORIGIN}${path}`,
    {
      method: 'POST',
      headers: { 'content-type': 'application/x-www-form-urlencoded', origin: ORIGIN, ...headers },
      body: new URLSearchParams(form).toString(),
    },
    adminEnv
  );
}

const auth = () => ({ 'cf-access-jwt-assertion': jwt });

describe('admin auth', () => {
  it('refuses without a JWT', async () => {
    expect((await get('/admin')).status).toBe(403);
  });

  it('refuses a garbage JWT', async () => {
    expect((await get('/admin', { 'cf-access-jwt-assertion': 'nope' })).status).toBe(403);
  });

  it('refuses when the team domain and audience are not configured', async () => {
    const res = await app.request(`${ORIGIN}/admin`, { headers: auth() }, {
      ...env,
      ACCESS_TEAM_DOMAIN: '',
      ACCESS_AUD: '',
    });
    expect(res.status).toBe(403);
  });

  it('allows a valid JWT', async () => {
    const res = await get('/admin', auth());
    expect(res.status).toBe(200);
    expect(await res.text()).toContain('owner@khwarizmi.co.id');
  });

  it('leaves the public licence endpoints unauthenticated', async () => {
    const res = await app.request(`${ORIGIN}/health`, {}, adminEnv);
    expect(res.status).toBe(200);
  });
});

describe('admin CSRF', () => {
  it('refuses a POST from a foreign origin', async () => {
    const res = await post('/admin/schools', { name: 'X', max_devices: '10', trial_days: '30' }, {
      ...auth(),
      origin: 'https://evil.example',
    });
    expect(res.status).toBe(403);
  });

  it('refuses a POST with no origin header', async () => {
    const res = await app.request(
      `${ORIGIN}/admin/schools`,
      {
        method: 'POST',
        headers: { 'content-type': 'application/x-www-form-urlencoded', ...auth() },
        body: new URLSearchParams({ name: 'X', max_devices: '10', trial_days: '30' }).toString(),
      },
      adminEnv
    );
    expect(res.status).toBe(403);
  });
});

describe('admin flows', () => {
  it('creates a school and shows it in the list', async () => {
    const res = await post(
      '/admin/schools',
      { name: 'SMAN 1 Bandung', max_devices: '40', trial_days: '30' },
      auth()
    );
    expect(res.status).toBe(303);

    const page = await (await get('/admin', auth())).text();
    expect(page).toContain('SMAN 1 Bandung');
    expect(page).toContain('40');
  });

  it('rejects an empty school name', async () => {
    const res = await post('/admin/schools', { name: '   ', max_devices: '40', trial_days: '30' }, auth());
    expect(res.status).toBe(400);
  });

  it('rejects a non-numeric or negative quota', async () => {
    expect(
      (await post('/admin/schools', { name: 'A', max_devices: 'abc', trial_days: '30' }, auth())).status
    ).toBe(400);
    expect(
      (await post('/admin/schools', { name: 'A', max_devices: '-5', trial_days: '30' }, auth())).status
    ).toBe(400);
  });

  it('updates the quota', async () => {
    const id = await createSchool(env.DB, { name: 'A', maxDevices: 10, trialDays: 30 }, new Date());
    const res = await post(`/admin/schools/${id}/quota`, { max_devices: '60' }, auth());
    expect(res.status).toBe(303);
    expect(await (await get('/admin', auth())).text()).toContain('60');
  });

  it('issues and then revokes an activation code', async () => {
    const id = await createSchool(env.DB, { name: 'A', maxDevices: 10, trialDays: 30 }, new Date());
    expect((await post(`/admin/schools/${id}/codes`, { valid_days: '30' }, auth())).status).toBe(303);

    const detail = await (await get(`/admin/schools/${id}`, auth())).text();
    const match = detail.match(/KHW-[0-9A-HJ-NP-Z]{26}/);
    expect(match).not.toBeNull();

    expect((await post(`/admin/codes/${match![0]}/revoke`, {}, auth())).status).toBe(303);
    expect(await (await get(`/admin/schools/${id}`, auth())).text()).not.toContain(match![0]);
  });

  it('escapes HTML in a school name', async () => {
    await post(
      '/admin/schools',
      { name: '<script>alert(1)</script>', max_devices: '10', trial_days: '30' },
      auth()
    );
    const page = await (await get('/admin', auth())).text();
    expect(page).not.toContain('<script>alert(1)</script>');
    expect(page).toContain('&lt;script&gt;');
  });

  it('returns 404 for an unknown school', async () => {
    expect((await get('/admin/schools/sch_nope', auth())).status).toBe(404);
  });
});
```

- [ ] **Step 2: Jalankan test untuk memastikan gagal**

Run: `npm test -- admin-routes`
Expected: FAIL — rute belum ada, balasan 404 bukan 403.

- [ ] **Step 3: Implementasi tampilan**

`src/admin/views.ts`:

```typescript
import type { SchoolSummary, ActivationCodeRow } from './queries';

export function escapeHtml(value: string): string {
  return value
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#39;');
}

const STYLES = `
:root {
  --bg: #f7f7f5;
  --surface: #ffffff;
  --ink: #1b1b18;
  --ink-soft: #5f5f58;
  --line: #e2e2dc;
  --accent: #1f5f4e;
  --warn: #8a5a00;
  --danger: #9b2c2c;
  --radius: 10px;
  --gap: 1.25rem;
}
* { box-sizing: border-box; }
body {
  margin: 0;
  background: var(--bg);
  color: var(--ink);
  font: 16px/1.55 ui-sans-serif, system-ui, -apple-system, "Segoe UI", sans-serif;
}
header {
  background: var(--surface);
  border-bottom: 1px solid var(--line);
  padding: 1rem clamp(1rem, 4vw, 2.5rem);
  display: flex;
  justify-content: space-between;
  align-items: baseline;
  gap: 1rem;
  flex-wrap: wrap;
}
header h1 { font-size: 1.1rem; margin: 0; letter-spacing: -0.01em; }
header .who { color: var(--ink-soft); font-size: 0.85rem; }
main { padding: clamp(1rem, 4vw, 2.5rem); max-width: 68rem; margin: 0 auto; }
h2 { font-size: 0.95rem; text-transform: uppercase; letter-spacing: 0.06em; color: var(--ink-soft); margin: 0 0 0.75rem; }
section { margin-bottom: 2.5rem; }
table { width: 100%; border-collapse: collapse; background: var(--surface); border: 1px solid var(--line); border-radius: var(--radius); overflow: hidden; }
th, td { text-align: left; padding: 0.7rem 0.9rem; border-bottom: 1px solid var(--line); font-size: 0.9rem; }
th { font-weight: 600; color: var(--ink-soft); font-size: 0.78rem; text-transform: uppercase; letter-spacing: 0.05em; }
tr:last-child td { border-bottom: 0; }
td.num { font-variant-numeric: tabular-nums; }
a { color: var(--accent); }
.pill { display: inline-block; padding: 0.1rem 0.5rem; border-radius: 999px; font-size: 0.75rem; border: 1px solid var(--line); }
.pill.trial { color: var(--warn); border-color: currentColor; }
.pill.over { color: var(--danger); border-color: currentColor; }
.card { background: var(--surface); border: 1px solid var(--line); border-radius: var(--radius); padding: var(--gap); }
form.inline { display: flex; gap: 0.6rem; flex-wrap: wrap; align-items: flex-end; }
label { display: flex; flex-direction: column; gap: 0.25rem; font-size: 0.78rem; color: var(--ink-soft); }
input, select { font: inherit; padding: 0.45rem 0.6rem; border: 1px solid var(--line); border-radius: 6px; background: #fff; color: var(--ink); }
button { font: inherit; padding: 0.5rem 0.9rem; border-radius: 6px; border: 1px solid var(--accent); background: var(--accent); color: #fff; cursor: pointer; }
button.quiet { background: transparent; color: var(--danger); border-color: var(--line); }
code { font-family: ui-monospace, SFMono-Regular, Menlo, monospace; font-size: 0.85rem; }
.empty { color: var(--ink-soft); font-size: 0.9rem; }
@media (prefers-color-scheme: dark) {
  :root { --bg: #16161a; --surface: #1e1e23; --ink: #ecece8; --ink-soft: #a0a099; --line: #30303a; --accent: #4bbf9a; --warn: #d9a441; --danger: #e07a7a; }
  input, select { background: #16161a; }
  button { color: #10221c; }
}
`;

function layout(title: string, email: string, body: string): string {
  return `<!doctype html>
<html lang="id">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>${escapeHtml(title)}</title>
<style>${STYLES}</style>
</head>
<body>
<header>
  <h1><a href="/admin" style="text-decoration:none;color:inherit">Khwarizmi · Lisensi</a></h1>
  <span class="who">${escapeHtml(email)}</span>
</header>
<main>${body}</main>
</body>
</html>`;
}

function dateOnly(iso: string): string {
  return iso.slice(0, 10);
}

export function renderSchools(schools: SchoolSummary[], email: string, now: Date): string {
  const rows = schools
    .map((s) => {
      const over = s.active_devices > s.max_devices;
      const lapsed = new Date(s.sub_end).getTime() <= now.getTime();
      return `<tr>
  <td><a href="/admin/schools/${encodeURIComponent(s.id)}">${escapeHtml(s.name)}</a></td>
  <td><span class="pill ${s.plan === 'trial' ? 'trial' : ''}">${escapeHtml(s.plan)}</span></td>
  <td class="num">${s.active_devices} / ${s.max_devices}${over ? ' <span class="pill over">lewat kuota</span>' : ''}</td>
  <td class="num">${s.master_count}</td>
  <td class="num">${dateOnly(s.sub_end)}${lapsed ? ' <span class="pill over">habis</span>' : ''}</td>
</tr>`;
    })
    .join('\n');

  const table = schools.length
    ? `<table>
<thead><tr><th>Sekolah</th><th>Paket</th><th>Device</th><th>Master</th><th>Langganan s/d</th></tr></thead>
<tbody>${rows}</tbody>
</table>`
    : `<p class="empty">Belum ada sekolah.</p>`;

  return layout(
    'Sekolah',
    email,
    `<section>
  <h2>Sekolah</h2>
  ${table}
</section>
<section>
  <h2>Tambah sekolah</h2>
  <div class="card">
    <form class="inline" method="post" action="/admin/schools">
      <label>Nama<input name="name" required maxlength="120" placeholder="SMAN 1 Bandung"></label>
      <label>Slot device<input name="max_devices" type="number" min="1" max="100000" value="40" required></label>
      <label>Trial (hari)<input name="trial_days" type="number" min="1" max="365" value="30" required></label>
      <button type="submit">Buat</button>
    </form>
  </div>
</section>`
  );
}

export function renderSchool(
  school: SchoolSummary,
  codes: ActivationCodeRow[],
  email: string,
  now: Date
): string {
  const codeRows = codes
    .map((c) => {
      const used = c.used_at !== null;
      const expired = new Date(c.expires_at).getTime() <= now.getTime();
      const status = used
        ? `<span class="pill">terpakai ${dateOnly(c.used_at!)}</span>`
        : expired
          ? `<span class="pill over">kedaluwarsa</span>`
          : `<span class="pill">belum dipakai</span>`;
      const revoke = used
        ? ''
        : `<form method="post" action="/admin/codes/${encodeURIComponent(c.code)}/revoke" style="margin:0">
             <button class="quiet" type="submit">Cabut</button>
           </form>`;
      return `<tr>
  <td><code>${escapeHtml(c.code)}</code></td>
  <td>${status}</td>
  <td class="num">${dateOnly(c.expires_at)}</td>
  <td>${revoke}</td>
</tr>`;
    })
    .join('\n');

  const codeTable = codes.length
    ? `<table>
<thead><tr><th>Kode</th><th>Status</th><th>Berlaku s/d</th><th></th></tr></thead>
<tbody>${codeRows}</tbody>
</table>`
    : `<p class="empty">Belum ada kode aktivasi.</p>`;

  return layout(
    school.name,
    email,
    `<section>
  <h2><a href="/admin">&larr; Semua sekolah</a></h2>
  <div class="card">
    <strong>${escapeHtml(school.name)}</strong><br>
    <span class="empty">${escapeHtml(school.id)} · ${school.active_devices} dari ${school.max_devices} slot terpakai · ${school.master_count} master</span>
  </div>
</section>
<section>
  <h2>Kuota</h2>
  <div class="card">
    <form class="inline" method="post" action="/admin/schools/${encodeURIComponent(school.id)}/quota">
      <label>Slot device<input name="max_devices" type="number" min="1" max="100000" value="${school.max_devices}" required></label>
      <button type="submit">Simpan</button>
    </form>
  </div>
</section>
<section>
  <h2>Langganan</h2>
  <div class="card">
    <form class="inline" method="post" action="/admin/schools/${encodeURIComponent(school.id)}/subscription">
      <label>Paket<select name="plan"><option value="trial"${school.plan === 'trial' ? ' selected' : ''}>trial</option><option value="paid"${school.plan === 'paid' ? ' selected' : ''}>paid</option></select></label>
      <label>Berlaku s/d<input name="sub_end" type="date" value="${dateOnly(school.sub_end)}" required></label>
      <button type="submit">Simpan</button>
    </form>
  </div>
</section>
<section>
  <h2>Kode aktivasi</h2>
  ${codeTable}
  <div class="card" style="margin-top:1rem">
    <form class="inline" method="post" action="/admin/schools/${encodeURIComponent(school.id)}/codes">
      <label>Berlaku (hari)<input name="valid_days" type="number" min="1" max="365" value="30" required></label>
      <button type="submit">Terbitkan kode</button>
    </form>
  </div>
</section>`
  );
}
```

- [ ] **Step 4: Implementasi rute**

`src/admin/routes.ts`:

```typescript
import { Hono } from 'hono';
import type { Env, Plan } from '../types';
import { fetchAccessJwks, verifyAccessJwt, type JwksCacheEntry } from './access';
import {
  createSchool,
  issueActivationCode,
  listActivationCodes,
  listSchools,
  revokeActivationCode,
  setQuota,
  setSubscription,
} from './queries';
import { renderSchool, renderSchools } from './views';

type Vars = { adminEmail: string };

export const admin = new Hono<{ Bindings: Env; Variables: Vars }>();

const jwksCache = new Map<string, JwksCacheEntry>();

/**
 * Fail closed. Cloudflare Access already stops requests at the edge, but a
 * misconfigured or removed Access policy must not expose these routes, so the
 * Worker verifies the assertion itself. There is deliberately no bypass flag.
 */
admin.use('*', async (c, next) => {
  const token = c.req.header('cf-access-jwt-assertion');
  const teamDomain = c.env.ACCESS_TEAM_DOMAIN;
  const aud = c.env.ACCESS_AUD;
  if (!token || !teamDomain || !aud) return c.text('Forbidden', 403);

  const jwks = await fetchAccessJwks(teamDomain, jwksCache);
  const claims = await verifyAccessJwt(token, { teamDomain, aud, now: new Date(), jwks });
  if (!claims) return c.text('Forbidden', 403);

  c.set('adminEmail', claims.email);
  await next();
});

/**
 * CSRF: the Access cookie rides along on cross-site form posts, so an origin
 * check is what actually stops them. Same-origin posts always send Origin.
 */
admin.use('*', async (c, next) => {
  if (c.req.method !== 'GET' && c.req.method !== 'HEAD') {
    const origin = c.req.header('origin');
    const expected = new URL(c.req.url).origin;
    if (origin !== expected) return c.text('Forbidden', 403);
  }
  await next();
});

function positiveInt(value: FormDataEntryValue | null, max: number): number | null {
  if (typeof value !== 'string' || !/^\d+$/.test(value.trim())) return null;
  const n = Number(value.trim());
  return n >= 1 && n <= max ? n : null;
}

admin.get('/', async (c) => {
  const now = new Date();
  const schools = await listSchools(c.env.DB, now);
  return c.html(renderSchools(schools, c.get('adminEmail'), now));
});

admin.post('/schools', async (c) => {
  const form = await c.req.formData();
  const name = String(form.get('name') ?? '').trim();
  const maxDevices = positiveInt(form.get('max_devices'), 100_000);
  const trialDays = positiveInt(form.get('trial_days'), 365);
  if (!name || name.length > 120 || maxDevices === null || trialDays === null) {
    return c.text('Bad Request', 400);
  }
  await createSchool(c.env.DB, { name, maxDevices, trialDays }, new Date());
  return c.redirect('/admin', 303);
});

admin.get('/schools/:id', async (c) => {
  const now = new Date();
  const id = c.req.param('id');
  const school = (await listSchools(c.env.DB, now)).find((s) => s.id === id);
  if (!school) return c.text('Not Found', 404);
  const codes = await listActivationCodes(c.env.DB, id);
  return c.html(renderSchool(school, codes, c.get('adminEmail'), now));
});

admin.post('/schools/:id/quota', async (c) => {
  const form = await c.req.formData();
  const maxDevices = positiveInt(form.get('max_devices'), 100_000);
  if (maxDevices === null) return c.text('Bad Request', 400);
  const ok = await setQuota(c.env.DB, c.req.param('id'), maxDevices);
  if (!ok) return c.text('Not Found', 404);
  return c.redirect(`/admin/schools/${encodeURIComponent(c.req.param('id'))}`, 303);
});

admin.post('/schools/:id/subscription', async (c) => {
  const form = await c.req.formData();
  const plan = String(form.get('plan') ?? '');
  const subEnd = String(form.get('sub_end') ?? '');
  if (plan !== 'trial' && plan !== 'paid') return c.text('Bad Request', 400);
  if (!/^\d{4}-\d{2}-\d{2}$/.test(subEnd)) return c.text('Bad Request', 400);
  const parsed = new Date(`${subEnd}T00:00:00.000Z`);
  if (Number.isNaN(parsed.getTime())) return c.text('Bad Request', 400);

  const ok = await setSubscription(c.env.DB, c.req.param('id'), plan as Plan, parsed);
  if (!ok) return c.text('Not Found', 404);
  return c.redirect(`/admin/schools/${encodeURIComponent(c.req.param('id'))}`, 303);
});

admin.post('/schools/:id/codes', async (c) => {
  const form = await c.req.formData();
  const validDays = positiveInt(form.get('valid_days'), 365);
  if (validDays === null) return c.text('Bad Request', 400);
  const code = await issueActivationCode(c.env.DB, c.req.param('id'), validDays, new Date());
  if (!code) return c.text('Not Found', 404);
  return c.redirect(`/admin/schools/${encodeURIComponent(c.req.param('id'))}`, 303);
});

admin.post('/codes/:code/revoke', async (c) => {
  const code = c.req.param('code');
  const row = await c.env.DB.prepare('SELECT tenant_id FROM activation_code WHERE code = ?1')
    .bind(code)
    .first<{ tenant_id: string }>();
  if (!row) return c.text('Not Found', 404);
  await revokeActivationCode(c.env.DB, code);
  return c.redirect(`/admin/schools/${encodeURIComponent(row.tenant_id)}`, 303);
});
```

- [ ] **Step 5: Pasang di `src/index.ts`**

Tambahkan import:

```typescript
import { admin } from './admin/routes';
```

dan setelah rute publik dipasang:

```typescript
app.route('/admin', admin);
```

- [ ] **Step 6: Jalankan test**

Run: `npm test -- admin-routes`
Expected: PASS.

- [ ] **Step 7: Jalankan seluruh suite**

Run: `npm test`
Expected: PASS — 58 test lama tetap hijau, plus test Task 1–3.

- [ ] **Step 8: Commit**

```bash
git add src/admin/routes.ts src/admin/views.ts src/index.ts test/admin-routes.test.ts
git commit -m "feat: add admin console routes and views behind Cloudflare Access"
```

---

### Task 4: Custom domain dan dokumentasi

**Files:**
- Modify: `wrangler.jsonc`
- Modify: `README.md`
- Modify: `scripts/admin.sql.md`

- [ ] **Step 1: Tambahkan custom domain dan vars**

Di `wrangler.jsonc`, tambahkan route custom domain dan dua var baru:

```jsonc
  "routes": [
    { "pattern": "license.khwarizmi.co.id", "custom_domain": true }
  ],
  "vars": {
    "LICENSE_ISSUER": "license.khwarizmi.co.id",
    "LICENSE_KID": "k1",
    "ACCESS_TEAM_DOMAIN": "",
    "ACCESS_AUD": ""
  },
```

Biarkan kosong sampai pemilik repo mengisinya — `/admin/*` fail closed selama
nilai-nilai itu kosong, dan itu memang perilaku yang diinginkan.

- [ ] **Step 2: Dokumentasikan penyiapan Access di `README.md`**

Tambahkan bagian yang menjelaskan:

- Console ada di `https://license.khwarizmi.co.id/admin`, dilindungi Cloudflare Access.
- Langkah membuat Access application: Zero Trust → Access → Applications → Self-hosted, domain `license.khwarizmi.co.id`, path `/admin`, policy email pemilik.
- Salin **team domain** dan **AUD tag** ke `ACCESS_TEAM_DOMAIN` dan `ACCESS_AUD` di `wrangler.jsonc`, lalu `npx wrangler deploy`.
- Worker memverifikasi ulang JWT Access; selama dua var itu kosong, `/admin/*` membalas 403. Ini disengaja — bukan bug.
- Peringatan: endpoint publik `/v1/activate` dan `/v1/checkin` **tidak** boleh dimasukkan ke Access application, karena klien Veyon memanggilnya tanpa sesi browser. Cakup hanya path `/admin`.

- [ ] **Step 3: Tandai operasi yang pindah ke console di `scripts/admin.sql.md`**

Tambahkan catatan di bagian atas: membuat sekolah, mengubah kuota, memperpanjang
langganan, dan menerbitkan atau mencabut kode aktivasi sekarang dilakukan lewat
console di `/admin`. SQL untuk operasi tersebut tetap ada sebagai jalur darurat
bila console tidak dapat diakses. Operasi yang **belum** ada di console dan masih
harus lewat SQL: melihat dan mencabut master, membersihkan device yang luruh.

- [ ] **Step 4: Jalankan seluruh suite**

Run: `npm test`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add wrangler.jsonc README.md scripts/admin.sql.md
git commit -m "docs: document the admin console and move the Worker to its own domain"
```

---

## Setelah rencana ini selesai

Pemilik repo membuat Access application, mengisi `ACCESS_TEAM_DOMAIN` dan
`ACCESS_AUD`, lalu deploy. Sebelum itu `/admin/*` membalas 403 dan endpoint
publik tetap berjalan normal.

Tidak termasuk dan memang bukan bagian tugas ini: manajemen master dan device di
console (masih lewat SQL), pencatatan tagihan (subsistem 3), dan rate limiting.
