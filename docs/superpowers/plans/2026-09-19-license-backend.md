# Backend Lisensi Khwarizmi — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Membangun backend lisensi yang menerbitkan token JWT RS512 bertanda tangan kepada instalasi Veyon Master, dengan dedup device per sekolah dan deteksi kelebihan kuota.

**Architecture:** Hono di Cloudflare Workers dengan D1 (SQLite) sebagai penyimpanan. Token ditandatangani memakai WebCrypto (`RSASSA-PKCS1-v1_5` + SHA-512 = RS512). Dua endpoint publik (`/v1/activate`, `/v1/checkin`) plus skrip admin lokal untuk operasi pilot manual. Logika murni (token, dedup device) dipisah dari lapis akses data supaya dapat diuji tanpa runtime Workers.

**Tech Stack:** TypeScript, Hono, Cloudflare Workers, D1, Vitest + `@cloudflare/vitest-pool-workers`, Wrangler.

**Spec:** `docs/superpowers/specs/2026-09-19-khwarizmi-saas-licensing-design.md` (di repo veyon)

---

## Lokasi repo

Semua path dalam rencana ini relatif terhadap **repo baru yang terpisah dari repo veyon**:

```
/Volumes/rherdians/programming/alkhwarizmi/khwarizmi-license/
```

Backend sengaja **tidak** ditaruh di dalam repo veyon. Veyon berlisensi GPLv2;
backend yang berkomunikasi lewat network bukan derivative dan boleh proprietary
(spec §3). Menaruhnya di dalam repo GPL mengaburkan batas itu. Alasan tambahan:
toolchain, siklus deploy, dan bahasa berbeda, dan repo veyon akan menarik
perubahan dari upstream Veyon.

Bila lokasi ini ingin diubah, cukup ganti root di atas — tidak ada path dalam
rencana ini yang bergantung pada repo veyon.

## Global Constraints

Diambil verbatim dari spec. Berlaku untuk **setiap** task.

- **Issuer token:** `license.khwarizmi.co.id`
- **Algoritma token:** RS512 (`RSASSA-PKCS1-v1_5` + SHA-512). Header membawa `kid`.
- **Masa berlaku token (`exp`):** 7 hari sejak diterbitkan
- **Jendela device aktif:** 30 hari terakhir (`last_seen_at`)
- **Tenggat kelebihan kuota:** 14 hari sejak `overage_since`
- **Semua timestamp disimpan sebagai string ISO-8601 UTC** (mis. `2026-10-19T00:00:00.000Z`)
- **INVARIANT — jalur lisensi steril.** Endpoint hanya menerima `master_id`,
  identitas device (hostname/MAC), hostname & versi master. **Tidak pernah**
  screenshot, aktivitas murid, nama pengguna, atau data pemantauan apa pun.
  Menambah field semacam itu adalah pelanggaran spec §10.
- **INVARIANT — kegagalan condong membiarkan sekolah bekerja.** Backend tidak
  pernah membalas 401/403 untuk kondisi yang bisa disebabkan bug server. Hanya
  secret yang benar-benar tidak cocok yang menghasilkan 401.
- **Semua kueri D1 memakai prepared statement dengan `.bind()`.** Tidak ada
  string SQL yang dirangkai dari input.

## Deviasi dari spec (disengaja, dicatat)

Dua hal berbeda dari spec dan keduanya menyederhanakan:

1. **Tidak ada Cron Trigger di subsistem ini.** Spec §14 mengasumsikan pekerjaan
   harian untuk luruh device dan deteksi kelebihan kuota. Ternyata keduanya tidak
   membutuhkan cron: luruh 30 hari adalah **klausa `WHERE` pada kueri**, bukan
   mutasi, dan kelebihan kuota dihitung ulang **inline saat check-in** ketika
   angkanya memang berubah. Menghapus cron sekaligus menghapus risiko eksekusi
   ganda (at-least-once) yang dicatat spec §14. Cron dapat ditambahkan di
   subsistem 3 bila muncul kebutuhan proaktif seperti pengingat tagihan.
2. **Tabel `invoice` ditunda ke subsistem 3.** Tidak ada yang menulisnya di
   subsistem ini karena webhook pembayaran adalah bagian subsistem 3 dan gateway
   belum ditentukan (spec §15). Pilot memakai skrip admin di Task 7.

---

### Task 1: Scaffold proyek, skema D1, dan harness pengujian

**Files:**
- Create: `package.json`
- Create: `wrangler.jsonc`
- Create: `tsconfig.json`
- Create: `vitest.config.ts`
- Create: `migrations/0001_initial.sql`
- Create: `src/types.ts`
- Create: `src/index.ts`
- Create: `test/env.d.ts`
- Create: `test/schema.test.ts`
- Create: `.gitignore`

**Interfaces:**
- Consumes: —
- Produces: `Env` (binding D1 `DB`, variabel `LICENSE_PRIVATE_KEY`, `LICENSE_KID`, `LICENSE_ISSUER`), tabel `tenant`, `master`, `device`, `activation_code`.

- [ ] **Step 1: Inisialisasi repo dan dependensi**

```bash
mkdir -p /Volumes/rherdians/programming/alkhwarizmi/khwarizmi-license
cd /Volumes/rherdians/programming/alkhwarizmi/khwarizmi-license
git init
npm init -y
npm install hono
npm install -D wrangler typescript vitest @cloudflare/vitest-pool-workers @cloudflare/workers-types
```

- [ ] **Step 2: Tulis file konfigurasi**

`package.json` — tambahkan `"type": "module"` dan ganti bagian `scripts`.
`"type": "module"` wajib: `scripts/keygen.ts` di Task 7 memakai top-level `await`,
yang gagal di bawah CommonJS default dari `npm init -y`.

```json
{
  "type": "module",
  "scripts": {
    "dev": "wrangler dev",
    "deploy": "wrangler deploy",
    "test": "vitest run",
    "migrate:local": "wrangler d1 migrations apply khwarizmi-license --local",
    "migrate:remote": "wrangler d1 migrations apply khwarizmi-license --remote"
  }
}
```

`tsconfig.json`:

```json
{
  "compilerOptions": {
    "target": "ES2022",
    "module": "ES2022",
    "moduleResolution": "bundler",
    "lib": ["ES2022"],
    "types": ["@cloudflare/workers-types", "vitest/globals"],
    "strict": true,
    "noEmit": true,
    "skipLibCheck": true
  },
  "include": ["src/**/*.ts", "test/**/*.ts"]
}
```

`wrangler.jsonc`:

```jsonc
{
  "name": "khwarizmi-license",
  "main": "src/index.ts",
  "compatibility_date": "2026-09-01",
  "vars": {
    "LICENSE_ISSUER": "license.khwarizmi.co.id",
    "LICENSE_KID": "k1"
  },
  "d1_databases": [
    {
      "binding": "DB",
      "database_name": "khwarizmi-license",
      "database_id": "PLACEHOLDER_ISI_SETELAH_WRANGLER_D1_CREATE",
      "migrations_dir": "migrations"
    }
  ]
}
```

`vitest.config.ts`:

```typescript
import { defineWorkersConfig, readD1Migrations } from '@cloudflare/vitest-pool-workers/config';

export default defineWorkersConfig(async () => ({
  test: {
    globals: true,
    poolOptions: {
      workers: {
        wrangler: { configPath: './wrangler.jsonc' },
        miniflare: {
          d1Databases: ['DB'],
          bindings: {
            TEST_MIGRATIONS: await readD1Migrations('./migrations'),
          },
        },
      },
    },
  },
}));
```

`test/env.d.ts` — membuat `env.TEST_MIGRATIONS` bertipe di seluruh test:

```typescript
declare module 'cloudflare:test' {
  interface ProvidedEnv extends Env {
    TEST_MIGRATIONS: D1Migration[];
  }
}

export {};
```

`.gitignore`:

```
node_modules/
.wrangler/
dist/
*.pem
.dev.vars
```

> **Catatan verifikasi:** bentuk persis konfigurasi `@cloudflare/vitest-pool-workers`
> dan `compatibility_date` yang tepat dapat berubah. Bila Step 6 gagal karena
> konfigurasi, periksa `node_modules/wrangler/config-schema.json` dan dokumentasi
> Cloudflare terkini sebelum mengubah pendekatan. Jangan mengganti strategi
> pengujian tanpa memeriksa dokumen dulu.

- [ ] **Step 3: Buat database D1**

```bash
npx wrangler d1 create khwarizmi-license
```

Salin `database_id` dari keluaran perintah ke `wrangler.jsonc`, menggantikan
`PLACEHOLDER_ISI_SETELAH_WRANGLER_D1_CREATE`.

- [ ] **Step 4: Tulis migrasi skema**

`migrations/0001_initial.sql`:

```sql
CREATE TABLE tenant (
  id            TEXT PRIMARY KEY,
  name          TEXT NOT NULL,
  plan          TEXT NOT NULL CHECK (plan IN ('trial', 'paid')),
  max_devices   INTEGER NOT NULL,
  sub_end       TEXT NOT NULL,
  overage_since TEXT,
  created_at    TEXT NOT NULL
);

CREATE TABLE master (
  id           TEXT PRIMARY KEY,
  tenant_id    TEXT NOT NULL REFERENCES tenant(id),
  secret_hash  TEXT NOT NULL,
  hostname     TEXT,
  version      TEXT,
  last_seen_at TEXT,
  created_at   TEXT NOT NULL
);

CREATE INDEX idx_master_tenant ON master(tenant_id);

CREATE TABLE device (
  id            TEXT PRIMARY KEY,
  tenant_id     TEXT NOT NULL REFERENCES tenant(id),
  mac           TEXT,
  hostname      TEXT NOT NULL,
  first_seen_at TEXT NOT NULL,
  last_seen_at  TEXT NOT NULL
);

CREATE INDEX idx_device_tenant_mac ON device(tenant_id, mac);
CREATE INDEX idx_device_tenant_hostname ON device(tenant_id, hostname);
CREATE INDEX idx_device_tenant_seen ON device(tenant_id, last_seen_at);

CREATE TABLE activation_code (
  code       TEXT PRIMARY KEY,
  tenant_id  TEXT NOT NULL REFERENCES tenant(id),
  used_at    TEXT,
  expires_at TEXT NOT NULL,
  created_at TEXT NOT NULL
);
```

- [ ] **Step 5: Tulis tipe bersama dan entry point**

`src/types.ts`:

```typescript
export type Plan = 'trial' | 'paid';

export interface Env {
  DB: D1Database;
  LICENSE_PRIVATE_KEY: string;
  LICENSE_KID: string;
  LICENSE_ISSUER: string;
}

export interface TokenPayload {
  iss: string;
  sub: string;
  tenant: string;
  tenant_name: string;
  plan: Plan;
  max_devices: number;
  sub_end: string;
  overage_since: string | null;
  iat: number;
  exp: number;
}

export interface TenantRow {
  id: string;
  name: string;
  plan: Plan;
  max_devices: number;
  sub_end: string;
  overage_since: string | null;
  created_at: string;
}

export interface MasterRow {
  id: string;
  tenant_id: string;
  secret_hash: string;
  hostname: string | null;
  version: string | null;
  last_seen_at: string | null;
  created_at: string;
}

export interface DeviceRow {
  id: string;
  tenant_id: string;
  mac: string | null;
  hostname: string;
  first_seen_at: string;
  last_seen_at: string;
}
```

`src/index.ts`:

```typescript
import { Hono } from 'hono';
import type { Env } from './types';

const app = new Hono<{ Bindings: Env }>();

app.get('/health', (c) => c.json({ ok: true }));

app.onError((err, c) => {
  console.error(err);
  return c.json({ error: 'internal_error' }, 500);
});

app.notFound((c) => c.json({ error: 'not_found' }, 404));

export default app;
```

- [ ] **Step 6: Tulis test skema dan jalankan**

`test/schema.test.ts`:

```typescript
import { env, applyD1Migrations } from 'cloudflare:test';
import { describe, it, expect, beforeAll } from 'vitest';
import app from '../src/index';

beforeAll(async () => {
  await applyD1Migrations(env.DB, env.TEST_MIGRATIONS);
});

describe('scaffold', () => {
  it('health endpoint responds', async () => {
    const res = await app.request('/health', {}, env);
    expect(res.status).toBe(200);
    expect(await res.json()).toEqual({ ok: true });
  });

  it('all four tables exist', async () => {
    const { results } = await env.DB.prepare(
      "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name"
    ).all<{ name: string }>();
    const names = results.map((r) => r.name);
    expect(names).toContain('tenant');
    expect(names).toContain('master');
    expect(names).toContain('device');
    expect(names).toContain('activation_code');
  });
});
```

Run: `npm test`
Expected: PASS, dua test hijau.

- [ ] **Step 7: Commit**

```bash
git add -A
git commit -m "chore: scaffold license backend with D1 schema and test harness"
```

---

### Task 2: Penandatanganan dan verifikasi token RS512

**Files:**
- Create: `src/token.ts`
- Create: `test/token.test.ts`

**Interfaces:**
- Consumes: `TokenPayload` dari `src/types.ts` (Task 1)
- Produces:
  - `importPrivateKey(pem: string): Promise<CryptoKey>`
  - `importPublicKey(pem: string): Promise<CryptoKey>`
  - `signToken(payload: TokenPayload, key: CryptoKey, kid: string): Promise<string>`
  - `verifyToken(token: string, key: CryptoKey): Promise<TokenPayload | null>`
  - `generateTestKeyPair(): Promise<{ privatePem: string; publicPem: string }>`

- [ ] **Step 1: Tulis test yang gagal**

`test/token.test.ts`:

```typescript
import { describe, it, expect, beforeAll } from 'vitest';
import {
  generateTestKeyPair,
  importPrivateKey,
  importPublicKey,
  signToken,
  verifyToken,
} from '../src/token';
import type { TokenPayload } from '../src/types';

const payload: TokenPayload = {
  iss: 'license.khwarizmi.co.id',
  sub: 'mst_test',
  tenant: 'sch_test',
  tenant_name: 'SMAN 1 Bandung',
  plan: 'paid',
  max_devices: 40,
  sub_end: '2026-10-19T00:00:00.000Z',
  overage_since: null,
  iat: 1760832000,
  exp: 1761436800,
};

let privateKey: CryptoKey;
let publicKey: CryptoKey;

beforeAll(async () => {
  const pair = await generateTestKeyPair();
  privateKey = await importPrivateKey(pair.privatePem);
  publicKey = await importPublicKey(pair.publicPem);
});

describe('token', () => {
  it('round-trips a signed payload', async () => {
    const token = await signToken(payload, privateKey, 'k1');
    expect(await verifyToken(token, publicKey)).toEqual(payload);
  });

  it('produces a three-part JWT with RS512 and kid in the header', async () => {
    const token = await signToken(payload, privateKey, 'k1');
    const parts = token.split('.');
    expect(parts).toHaveLength(3);
    const header = JSON.parse(atob(parts[0].replace(/-/g, '+').replace(/_/g, '/')));
    expect(header).toEqual({ alg: 'RS512', typ: 'JWT', kid: 'k1' });
  });

  it('rejects a token whose payload was tampered with', async () => {
    const token = await signToken(payload, privateKey, 'k1');
    const [h, , s] = token.split('.');
    const forged = { ...payload, max_devices: 9999 };
    const bin = new TextEncoder().encode(JSON.stringify(forged));
    let str = '';
    for (const b of bin) str += String.fromCharCode(b);
    const p = btoa(str).replace(/\+/g, '-').replace(/\//g, '_').replace(/=+$/, '');
    expect(await verifyToken(`${h}.${p}.${s}`, publicKey)).toBeNull();
  });

  it('rejects a malformed token', async () => {
    expect(await verifyToken('not-a-token', publicKey)).toBeNull();
    expect(await verifyToken('a.b', publicKey)).toBeNull();
  });

  it('rejects a token signed by a different key', async () => {
    const other = await generateTestKeyPair();
    const otherPrivate = await importPrivateKey(other.privatePem);
    const token = await signToken(payload, otherPrivate, 'k1');
    expect(await verifyToken(token, publicKey)).toBeNull();
  });
});
```

- [ ] **Step 2: Jalankan test untuk memastikan gagal**

Run: `npm test -- token`
Expected: FAIL — `Failed to resolve import "../src/token"`

- [ ] **Step 3: Implementasi**

`src/token.ts`:

```typescript
import type { TokenPayload } from './types';

const ALG = { name: 'RSASSA-PKCS1-v1_5', hash: 'SHA-512' } as const;

function base64UrlEncode(bytes: Uint8Array): string {
  let str = '';
  for (const b of bytes) str += String.fromCharCode(b);
  return btoa(str).replace(/\+/g, '-').replace(/\//g, '_').replace(/=+$/, '');
}

function base64UrlDecode(input: string): Uint8Array {
  const padded = input.replace(/-/g, '+').replace(/_/g, '/');
  const str = atob(padded);
  const bytes = new Uint8Array(str.length);
  for (let i = 0; i < str.length; i++) bytes[i] = str.charCodeAt(i);
  return bytes;
}

function pemToBytes(pem: string): Uint8Array {
  const body = pem.replace(/-----[A-Z ]+-----/g, '').replace(/\s+/g, '');
  const str = atob(body);
  const bytes = new Uint8Array(str.length);
  for (let i = 0; i < str.length; i++) bytes[i] = str.charCodeAt(i);
  return bytes;
}

function bytesToPem(bytes: ArrayBuffer, label: string): string {
  let str = '';
  for (const b of new Uint8Array(bytes)) str += String.fromCharCode(b);
  const b64 = btoa(str).replace(/(.{64})/g, '$1\n');
  return `-----BEGIN ${label}-----\n${b64}\n-----END ${label}-----`;
}

export async function importPrivateKey(pem: string): Promise<CryptoKey> {
  return crypto.subtle.importKey('pkcs8', pemToBytes(pem), ALG, false, ['sign']);
}

export async function importPublicKey(pem: string): Promise<CryptoKey> {
  return crypto.subtle.importKey('spki', pemToBytes(pem), ALG, false, ['verify']);
}

export async function signToken(
  payload: TokenPayload,
  key: CryptoKey,
  kid: string
): Promise<string> {
  const enc = new TextEncoder();
  const header = base64UrlEncode(enc.encode(JSON.stringify({ alg: 'RS512', typ: 'JWT', kid })));
  const body = base64UrlEncode(enc.encode(JSON.stringify(payload)));
  const signingInput = `${header}.${body}`;
  const signature = await crypto.subtle.sign(ALG, key, enc.encode(signingInput));
  return `${signingInput}.${base64UrlEncode(new Uint8Array(signature))}`;
}

export async function verifyToken(
  token: string,
  key: CryptoKey
): Promise<TokenPayload | null> {
  const parts = token.split('.');
  if (parts.length !== 3) return null;
  const [header, body, signature] = parts;
  try {
    const ok = await crypto.subtle.verify(
      ALG,
      key,
      base64UrlDecode(signature),
      new TextEncoder().encode(`${header}.${body}`)
    );
    if (!ok) return null;
    return JSON.parse(new TextDecoder().decode(base64UrlDecode(body))) as TokenPayload;
  } catch {
    return null;
  }
}

/** Hanya untuk pengujian dan skrip admin — produksi memakai kunci dari Workers Secrets. */
export async function generateTestKeyPair(): Promise<{ privatePem: string; publicPem: string }> {
  const pair = (await crypto.subtle.generateKey(
    { ...ALG, modulusLength: 4096, publicExponent: new Uint8Array([1, 0, 1]) },
    true,
    ['sign', 'verify']
  )) as CryptoKeyPair;
  const priv = await crypto.subtle.exportKey('pkcs8', pair.privateKey);
  const pub = await crypto.subtle.exportKey('spki', pair.publicKey);
  return {
    privatePem: bytesToPem(priv, 'PRIVATE KEY'),
    publicPem: bytesToPem(pub, 'PUBLIC KEY'),
  };
}
```

- [ ] **Step 4: Jalankan test untuk memastikan lulus**

Run: `npm test -- token`
Expected: PASS, lima test hijau.

- [ ] **Step 5: Commit**

```bash
git add src/token.ts test/token.test.ts
git commit -m "feat: add RS512 token signing and verification"
```

---

### Task 3: Normalisasi identitas dan dedup device

**Files:**
- Create: `src/devices.ts`
- Create: `test/devices.test.ts`

**Interfaces:**
- Consumes: —
- Produces:
  - `normalizeMac(mac: string | null | undefined): string | null`
  - `normalizeHostname(hostname: string): string`
  - `DeviceIdentity { mac: string | null; hostname: string }`
  - `StoredDevice { id: string; mac: string | null; hostname: string }`
  - `matchDevice(reported: DeviceIdentity, existing: StoredDevice[]): StoredDevice | null`
  - `ACTIVE_WINDOW_DAYS = 30`

- [ ] **Step 1: Tulis test yang gagal**

`test/devices.test.ts`:

```typescript
import { describe, it, expect } from 'vitest';
import { normalizeMac, normalizeHostname, matchDevice } from '../src/devices';
import type { StoredDevice } from '../src/devices';

describe('normalizeMac', () => {
  it('strips separators and lowercases', () => {
    expect(normalizeMac('AA:BB:CC:DD:EE:FF')).toBe('aabbccddeeff');
    expect(normalizeMac('aa-bb-cc-dd-ee-ff')).toBe('aabbccddeeff');
    expect(normalizeMac('aabb.ccdd.eeff')).toBe('aabbccddeeff');
  });

  it('returns null for empty or malformed input', () => {
    expect(normalizeMac('')).toBeNull();
    expect(normalizeMac(null)).toBeNull();
    expect(normalizeMac(undefined)).toBeNull();
    expect(normalizeMac('not-a-mac')).toBeNull();
    expect(normalizeMac('aabbccddee')).toBeNull();
  });
});

describe('normalizeHostname', () => {
  it('lowercases, trims, and drops the domain suffix', () => {
    expect(normalizeHostname('  LAB1-PC01  ')).toBe('lab1-pc01');
    expect(normalizeHostname('LAB1-PC01.sekolah.sch.id')).toBe('lab1-pc01');
  });
});

describe('matchDevice', () => {
  const existing: StoredDevice[] = [
    { id: 'dev_a', mac: 'aabbccddeeff', hostname: 'lab1-pc01' },
    { id: 'dev_b', mac: null, hostname: 'lab1-pc02' },
  ];

  it('matches on MAC first', () => {
    const hit = matchDevice({ mac: 'AA:BB:CC:DD:EE:FF', hostname: 'renamed-pc' }, existing);
    expect(hit?.id).toBe('dev_a');
  });

  it('falls back to hostname when MAC is absent', () => {
    const hit = matchDevice({ mac: null, hostname: 'LAB1-PC02' }, existing);
    expect(hit?.id).toBe('dev_b');
  });

  it('falls back to hostname when the MAC is unknown', () => {
    const hit = matchDevice({ mac: '11:22:33:44:55:66', hostname: 'lab1-pc02' }, existing);
    expect(hit?.id).toBe('dev_b');
  });

  it('falls back to hostname when the MAC is malformed', () => {
    const hit = matchDevice({ mac: 'garbage', hostname: 'lab1-pc01' }, existing);
    expect(hit?.id).toBe('dev_a');
  });

  it('returns null for a genuinely new device', () => {
    expect(matchDevice({ mac: '11:22:33:44:55:66', hostname: 'lab2-pc09' }, existing)).toBeNull();
  });

  it('does not match a MAC-less report against a different host', () => {
    expect(matchDevice({ mac: null, hostname: 'lab9-pc99' }, existing)).toBeNull();
  });
});
```

- [ ] **Step 2: Jalankan test untuk memastikan gagal**

Run: `npm test -- devices`
Expected: FAIL — `Failed to resolve import "../src/devices"`

- [ ] **Step 3: Implementasi**

`src/devices.ts`:

```typescript
/** Jendela device dianggap aktif. Device yang lebih lama tidak dihitung terhadap kuota. */
export const ACTIVE_WINDOW_DAYS = 30;

export interface DeviceIdentity {
  mac: string | null;
  hostname: string;
}

export interface StoredDevice {
  id: string;
  mac: string | null;
  hostname: string;
}

export function normalizeMac(mac: string | null | undefined): string | null {
  if (!mac) return null;
  const normalized = mac.replace(/[:\-.\s]/g, '').toLowerCase();
  return /^[0-9a-f]{12}$/.test(normalized) ? normalized : null;
}

export function normalizeHostname(hostname: string): string {
  return hostname.trim().toLowerCase().split('.')[0];
}

/**
 * Aturan dedup spec §6, dalam lingkup satu tenant:
 *   1. MAC cocok  -> device yang sama
 *   2. hostname cocok -> device yang sama
 *   3. selain itu -> device baru
 * MAC yang malformed diperlakukan seperti tidak ada, sehingga jatuh ke hostname.
 */
export function matchDevice(
  reported: DeviceIdentity,
  existing: StoredDevice[]
): StoredDevice | null {
  const mac = normalizeMac(reported.mac);
  if (mac) {
    const byMac = existing.find((d) => normalizeMac(d.mac) === mac);
    if (byMac) return byMac;
  }
  const hostname = normalizeHostname(reported.hostname);
  return existing.find((d) => normalizeHostname(d.hostname) === hostname) ?? null;
}
```

- [ ] **Step 4: Jalankan test untuk memastikan lulus**

Run: `npm test -- devices`
Expected: PASS, sepuluh test hijau.

- [ ] **Step 5: Commit**

```bash
git add src/devices.ts test/devices.test.ts
git commit -m "feat: add device identity normalization and dedup rules"
```

---

### Task 4: Lapis akses data

**Files:**
- Create: `src/db.ts`
- Create: `test/db.test.ts`

**Interfaces:**
- Consumes: `TenantRow`, `MasterRow`, `DeviceRow` (Task 1); `matchDevice`, `normalizeMac`, `normalizeHostname`, `ACTIVE_WINDOW_DAYS` (Task 3)
- Produces:
  - `getTenant(db: D1Database, tenantId: string): Promise<TenantRow | null>`
  - `getMaster(db: D1Database, masterId: string): Promise<MasterRow | null>`
  - `listDevices(db: D1Database, tenantId: string): Promise<DeviceRow[]>`
  - `countActiveDevices(db: D1Database, tenantId: string, now: Date): Promise<number>`
  - `upsertDevices(db: D1Database, tenantId: string, reported: DeviceIdentity[], now: Date): Promise<void>`
  - `recomputeOverage(db: D1Database, tenantId: string, now: Date): Promise<string | null>`
  - `randomId(prefix: string): string`
  - `randomSecret(): string`
  - `sha256Hex(input: string): Promise<string>`
  - `timingSafeEqual(a: string, b: string): boolean`

- [ ] **Step 1: Tulis test yang gagal**

`test/db.test.ts`:

```typescript
import { env, applyD1Migrations } from 'cloudflare:test';
import { describe, it, expect, beforeAll, beforeEach } from 'vitest';
import {
  getTenant,
  listDevices,
  countActiveDevices,
  upsertDevices,
  recomputeOverage,
  sha256Hex,
  timingSafeEqual,
} from '../src/db';

const NOW = new Date('2026-09-19T00:00:00.000Z');

function daysAgo(days: number): string {
  return new Date(NOW.getTime() - days * 86_400_000).toISOString();
}

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
  await env.DB.prepare(
    `INSERT INTO tenant (id, name, plan, max_devices, sub_end, overage_since, created_at)
     VALUES ('sch_1', 'SMAN 1', 'paid', 2, '2026-12-01T00:00:00.000Z', NULL, ?1)`
  )
    .bind(daysAgo(60))
    .run();
});

describe('upsertDevices', () => {
  it('inserts new devices', async () => {
    await upsertDevices(env.DB, 'sch_1', [{ mac: 'AA:BB:CC:DD:EE:FF', hostname: 'lab1-pc01' }], NOW);
    const devices = await listDevices(env.DB, 'sch_1');
    expect(devices).toHaveLength(1);
    expect(devices[0].mac).toBe('aabbccddeeff');
    expect(devices[0].hostname).toBe('lab1-pc01');
  });

  it('is idempotent for the same device', async () => {
    const d = [{ mac: 'AA:BB:CC:DD:EE:FF', hostname: 'lab1-pc01' }];
    await upsertDevices(env.DB, 'sch_1', d, NOW);
    await upsertDevices(env.DB, 'sch_1', d, NOW);
    expect(await listDevices(env.DB, 'sch_1')).toHaveLength(1);
  });

  it('merges a hostname-only device when a MAC is later reported', async () => {
    await upsertDevices(env.DB, 'sch_1', [{ mac: null, hostname: 'lab1-pc01' }], NOW);
    await upsertDevices(env.DB, 'sch_1', [{ mac: 'AA:BB:CC:DD:EE:FF', hostname: 'lab1-pc01' }], NOW);
    const devices = await listDevices(env.DB, 'sch_1');
    expect(devices).toHaveLength(1);
    expect(devices[0].mac).toBe('aabbccddeeff');
  });
});

describe('countActiveDevices', () => {
  it('ignores devices not seen within the 30 day window', async () => {
    await env.DB.batch([
      env.DB.prepare(
        `INSERT INTO device (id, tenant_id, mac, hostname, first_seen_at, last_seen_at)
         VALUES ('d1', 'sch_1', NULL, 'fresh', ?1, ?1)`
      ).bind(daysAgo(1)),
      env.DB.prepare(
        `INSERT INTO device (id, tenant_id, mac, hostname, first_seen_at, last_seen_at)
         VALUES ('d2', 'sch_1', NULL, 'stale', ?1, ?1)`
      ).bind(daysAgo(31)),
    ]);
    expect(await countActiveDevices(env.DB, 'sch_1', NOW)).toBe(1);
  });
});

describe('recomputeOverage', () => {
  async function seedDevices(count: number) {
    for (let i = 0; i < count; i++) {
      await upsertDevices(env.DB, 'sch_1', [{ mac: null, hostname: `pc-${i}` }], NOW);
    }
  }

  it('returns null while within quota', async () => {
    await seedDevices(2);
    expect(await recomputeOverage(env.DB, 'sch_1', NOW)).toBeNull();
  });

  it('sets overage_since when the quota is exceeded', async () => {
    await seedDevices(3);
    expect(await recomputeOverage(env.DB, 'sch_1', NOW)).toBe(NOW.toISOString());
  });

  it('is idempotent and preserves the original overage clock', async () => {
    await seedDevices(3);
    const first = await recomputeOverage(env.DB, 'sch_1', NOW);
    const later = new Date(NOW.getTime() + 5 * 86_400_000);
    const second = await recomputeOverage(env.DB, 'sch_1', later);
    expect(second).toBe(first);
  });

  it('clears overage_since once back within quota', async () => {
    await seedDevices(3);
    await recomputeOverage(env.DB, 'sch_1', NOW);
    await env.DB.prepare("DELETE FROM device WHERE hostname = 'pc-2'").run();
    expect(await recomputeOverage(env.DB, 'sch_1', NOW)).toBeNull();
    const tenant = await getTenant(env.DB, 'sch_1');
    expect(tenant?.overage_since).toBeNull();
  });
});

describe('timingSafeEqual', () => {
  it('compares correctly', async () => {
    const h = await sha256Hex('secret');
    expect(timingSafeEqual(h, h)).toBe(true);
    expect(timingSafeEqual(h, await sha256Hex('other'))).toBe(false);
    expect(timingSafeEqual(h, 'short')).toBe(false);
  });
});
```

- [ ] **Step 2: Jalankan test untuk memastikan gagal**

Run: `npm test -- db`
Expected: FAIL — `Failed to resolve import "../src/db"`

- [ ] **Step 3: Implementasi**

`src/db.ts`:

```typescript
import type { TenantRow, MasterRow, DeviceRow } from './types';
import {
  ACTIVE_WINDOW_DAYS,
  matchDevice,
  normalizeHostname,
  normalizeMac,
  type DeviceIdentity,
  type StoredDevice,
} from './devices';

export function randomId(prefix: string): string {
  const bytes = new Uint8Array(8);
  crypto.getRandomValues(bytes);
  const hex = [...bytes].map((b) => b.toString(16).padStart(2, '0')).join('');
  return `${prefix}_${hex}`;
}

export function randomSecret(): string {
  const bytes = new Uint8Array(32);
  crypto.getRandomValues(bytes);
  return [...bytes].map((b) => b.toString(16).padStart(2, '0')).join('');
}

export async function sha256Hex(input: string): Promise<string> {
  const digest = await crypto.subtle.digest('SHA-256', new TextEncoder().encode(input));
  return [...new Uint8Array(digest)].map((b) => b.toString(16).padStart(2, '0')).join('');
}

/** Perbandingan waktu-tetap untuk hash secret. */
export function timingSafeEqual(a: string, b: string): boolean {
  if (a.length !== b.length) return false;
  let diff = 0;
  for (let i = 0; i < a.length; i++) diff |= a.charCodeAt(i) ^ b.charCodeAt(i);
  return diff === 0;
}

export async function getTenant(db: D1Database, tenantId: string): Promise<TenantRow | null> {
  return db.prepare('SELECT * FROM tenant WHERE id = ?1').bind(tenantId).first<TenantRow>();
}

export async function getMaster(db: D1Database, masterId: string): Promise<MasterRow | null> {
  return db.prepare('SELECT * FROM master WHERE id = ?1').bind(masterId).first<MasterRow>();
}

export async function listDevices(db: D1Database, tenantId: string): Promise<DeviceRow[]> {
  const { results } = await db
    .prepare('SELECT * FROM device WHERE tenant_id = ?1')
    .bind(tenantId)
    .all<DeviceRow>();
  return results;
}

export async function countActiveDevices(
  db: D1Database,
  tenantId: string,
  now: Date
): Promise<number> {
  const cutoff = new Date(now.getTime() - ACTIVE_WINDOW_DAYS * 86_400_000).toISOString();
  const row = await db
    .prepare('SELECT COUNT(*) AS n FROM device WHERE tenant_id = ?1 AND last_seen_at >= ?2')
    .bind(tenantId, cutoff)
    .first<{ n: number }>();
  return row?.n ?? 0;
}

export async function upsertDevices(
  db: D1Database,
  tenantId: string,
  reported: DeviceIdentity[],
  now: Date
): Promise<void> {
  const nowIso = now.toISOString();
  const existing = await listDevices(db, tenantId);
  const pool: StoredDevice[] = existing.map((d) => ({
    id: d.id,
    mac: d.mac,
    hostname: d.hostname,
  }));
  const statements: D1PreparedStatement[] = [];

  for (const device of reported) {
    const mac = normalizeMac(device.mac);
    const hostname = normalizeHostname(device.hostname);
    if (!hostname) continue;

    const hit = matchDevice(device, pool);
    if (hit) {
      statements.push(
        db
          .prepare(
            'UPDATE device SET last_seen_at = ?1, hostname = ?2, mac = COALESCE(?3, mac) WHERE id = ?4'
          )
          .bind(nowIso, hostname, mac, hit.id)
      );
      if (mac && !hit.mac) hit.mac = mac;
      hit.hostname = hostname;
    } else {
      const id = randomId('dev');
      statements.push(
        db
          .prepare(
            `INSERT INTO device (id, tenant_id, mac, hostname, first_seen_at, last_seen_at)
             VALUES (?1, ?2, ?3, ?4, ?5, ?5)`
          )
          .bind(id, tenantId, mac, hostname, nowIso)
      );
      pool.push({ id, mac, hostname });
    }
  }

  if (statements.length > 0) await db.batch(statements);
}

/**
 * Menghitung ULANG status kelebihan kuota dari keadaan saat ini.
 * Tidak pernah menambah atau menggeser penghitung, sehingga aman dijalankan
 * berkali-kali (spec §14).
 */
export async function recomputeOverage(
  db: D1Database,
  tenantId: string,
  now: Date
): Promise<string | null> {
  const tenant = await getTenant(db, tenantId);
  if (!tenant) return null;

  const active = await countActiveDevices(db, tenantId, now);

  if (active > tenant.max_devices) {
    if (tenant.overage_since) return tenant.overage_since;
    const since = now.toISOString();
    await db
      .prepare('UPDATE tenant SET overage_since = ?1 WHERE id = ?2 AND overage_since IS NULL')
      .bind(since, tenantId)
      .run();
    return since;
  }

  if (tenant.overage_since) {
    await db.prepare('UPDATE tenant SET overage_since = NULL WHERE id = ?1').bind(tenantId).run();
  }
  return null;
}
```

- [ ] **Step 4: Jalankan test untuk memastikan lulus**

Run: `npm test -- db`
Expected: PASS, sembilan test hijau.

- [ ] **Step 5: Commit**

```bash
git add src/db.ts test/db.test.ts
git commit -m "feat: add data access layer with device upsert and overage recompute"
```

---

### Task 5: Endpoint `POST /v1/activate`

**Files:**
- Create: `src/license.ts`
- Create: `src/routes/activate.ts`
- Modify: `src/index.ts`
- Create: `test/activate.test.ts`

**Interfaces:**
- Consumes: `signToken`, `importPrivateKey` (Task 2); `getTenant`, `randomId`, `randomSecret`, `sha256Hex` (Task 4)
- Produces:
  - `issueToken(env: Env, tenant: TenantRow, masterId: string, now: Date): Promise<string>` di `src/license.ts`
  - `TOKEN_TTL_DAYS = 7`
  - Route `POST /v1/activate`

- [ ] **Step 1: Tulis test yang gagal**

`test/activate.test.ts`:

```typescript
import { env, applyD1Migrations } from 'cloudflare:test';
import { describe, it, expect, beforeAll, beforeEach } from 'vitest';
import app from '../src/index';
import { importPublicKey, verifyToken, generateTestKeyPair } from '../src/token';

const NOW = new Date('2026-09-19T00:00:00.000Z');
let testEnv: typeof env & { LICENSE_PRIVATE_KEY: string };
let publicKey: CryptoKey;

beforeAll(async () => {
  await applyD1Migrations(env.DB, env.TEST_MIGRATIONS);
  const pair = await generateTestKeyPair();
  publicKey = await importPublicKey(pair.publicPem);
  testEnv = { ...env, LICENSE_PRIVATE_KEY: pair.privatePem };
});

beforeEach(async () => {
  await env.DB.batch([
    env.DB.prepare('DELETE FROM device'),
    env.DB.prepare('DELETE FROM master'),
    env.DB.prepare('DELETE FROM activation_code'),
    env.DB.prepare('DELETE FROM tenant'),
  ]);
  await env.DB.batch([
    env.DB.prepare(
      `INSERT INTO tenant (id, name, plan, max_devices, sub_end, overage_since, created_at)
       VALUES ('sch_1', 'SMAN 1 Bandung', 'trial', 40, '2026-10-19T00:00:00.000Z', NULL, '2026-09-19T00:00:00.000Z')`
    ),
    env.DB.prepare(
      `INSERT INTO activation_code (code, tenant_id, used_at, expires_at, created_at)
       VALUES ('KHW-TEST-0001', 'sch_1', NULL, '2027-01-01T00:00:00.000Z', '2026-09-19T00:00:00.000Z')`
    ),
    env.DB.prepare(
      `INSERT INTO activation_code (code, tenant_id, used_at, expires_at, created_at)
       VALUES ('KHW-EXPIRED-01', 'sch_1', NULL, '2026-01-01T00:00:00.000Z', '2025-09-19T00:00:00.000Z')`
    ),
  ]);
});

function activate(body: unknown) {
  return app.request(
    '/v1/activate',
    { method: 'POST', body: JSON.stringify(body), headers: { 'content-type': 'application/json' } },
    testEnv
  );
}

describe('POST /v1/activate', () => {
  it('exchanges a valid code for credentials and a token', async () => {
    const res = await activate({ code: 'KHW-TEST-0001', hostname: 'guru-pc', version: '4.11.0' });
    expect(res.status).toBe(200);
    const body = await res.json<{ master_id: string; secret: string; token: string }>();
    expect(body.master_id).toMatch(/^mst_/);
    expect(body.secret).toHaveLength(64);

    const payload = await verifyToken(body.token, publicKey);
    expect(payload).not.toBeNull();
    expect(payload!.iss).toBe('license.khwarizmi.co.id');
    expect(payload!.tenant).toBe('sch_1');
    expect(payload!.tenant_name).toBe('SMAN 1 Bandung');
    expect(payload!.plan).toBe('trial');
    expect(payload!.max_devices).toBe(40);
    expect(payload!.sub_end).toBe('2026-10-19T00:00:00.000Z');
    expect(payload!.overage_since).toBeNull();
    expect(payload!.sub).toBe(body.master_id);
    expect(payload!.exp - payload!.iat).toBe(7 * 86_400);
  });

  it('rejects a code that was already used', async () => {
    await activate({ code: 'KHW-TEST-0001', hostname: 'guru-pc', version: '4.11.0' });
    const res = await activate({ code: 'KHW-TEST-0001', hostname: 'lain', version: '4.11.0' });
    expect(res.status).toBe(400);
    expect(await res.json()).toEqual({ error: 'invalid_or_used_code' });
  });

  it('rejects an expired code', async () => {
    const res = await activate({ code: 'KHW-EXPIRED-01', hostname: 'guru-pc', version: '4.11.0' });
    expect(res.status).toBe(400);
  });

  it('rejects an unknown code', async () => {
    const res = await activate({ code: 'KHW-NOPE-9999', hostname: 'guru-pc', version: '4.11.0' });
    expect(res.status).toBe(400);
  });

  it('rejects a malformed body', async () => {
    const res = await activate({ hostname: 'guru-pc' });
    expect(res.status).toBe(400);
  });

  it('stores the secret hashed, never in plaintext', async () => {
    const res = await activate({ code: 'KHW-TEST-0001', hostname: 'guru-pc', version: '4.11.0' });
    const body = await res.json<{ master_id: string; secret: string }>();
    const row = await env.DB.prepare('SELECT secret_hash FROM master WHERE id = ?1')
      .bind(body.master_id)
      .first<{ secret_hash: string }>();
    expect(row!.secret_hash).not.toBe(body.secret);
    expect(row!.secret_hash).toHaveLength(64);
  });
});
```

- [ ] **Step 2: Jalankan test untuk memastikan gagal**

Run: `npm test -- activate`
Expected: FAIL — route belum ada, balasan 404.

- [ ] **Step 3: Implementasi penerbit token**

`src/license.ts`:

```typescript
import { importPrivateKey, signToken } from './token';
import type { Env, TenantRow } from './types';

export const TOKEN_TTL_DAYS = 7;

export async function issueToken(
  env: Env,
  tenant: TenantRow,
  masterId: string,
  now: Date
): Promise<string> {
  const iat = Math.floor(now.getTime() / 1000);
  const key = await importPrivateKey(env.LICENSE_PRIVATE_KEY);
  return signToken(
    {
      iss: env.LICENSE_ISSUER,
      sub: masterId,
      tenant: tenant.id,
      tenant_name: tenant.name,
      plan: tenant.plan,
      max_devices: tenant.max_devices,
      sub_end: tenant.sub_end,
      overage_since: tenant.overage_since,
      iat,
      exp: iat + TOKEN_TTL_DAYS * 86_400,
    },
    key,
    env.LICENSE_KID
  );
}
```

- [ ] **Step 4: Implementasi route**

`src/routes/activate.ts`:

```typescript
import { Hono } from 'hono';
import type { Env } from '../types';
import { getTenant, randomId, randomSecret, sha256Hex } from '../db';
import { issueToken } from '../license';

export const activate = new Hono<{ Bindings: Env }>();

activate.post('/v1/activate', async (c) => {
  const body = await c.req.json().catch(() => null);
  const code = typeof body?.code === 'string' ? body.code : null;
  if (!code) return c.json({ error: 'invalid_request' }, 400);

  const hostname = typeof body?.hostname === 'string' ? body.hostname : null;
  const version = typeof body?.version === 'string' ? body.version : null;

  const now = new Date();
  const nowIso = now.toISOString();

  // Klaim atomik: hanya satu permintaan yang bisa menang, bahkan bila bersamaan.
  const claim = await c.env.DB.prepare(
    `UPDATE activation_code SET used_at = ?1
     WHERE code = ?2 AND used_at IS NULL AND expires_at > ?1`
  )
    .bind(nowIso, code)
    .run();

  if (claim.meta.changes !== 1) return c.json({ error: 'invalid_or_used_code' }, 400);

  const row = await c.env.DB.prepare('SELECT tenant_id FROM activation_code WHERE code = ?1')
    .bind(code)
    .first<{ tenant_id: string }>();
  const tenant = row ? await getTenant(c.env.DB, row.tenant_id) : null;
  if (!tenant) return c.json({ error: 'invalid_or_used_code' }, 400);

  const masterId = randomId('mst');
  const secret = randomSecret();

  await c.env.DB.prepare(
    `INSERT INTO master (id, tenant_id, secret_hash, hostname, version, last_seen_at, created_at)
     VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?6)`
  )
    .bind(masterId, tenant.id, await sha256Hex(secret), hostname, version, nowIso)
    .run();

  return c.json({
    master_id: masterId,
    secret,
    token: await issueToken(c.env, tenant, masterId, now),
  });
});
```

- [ ] **Step 5: Pasang route di `src/index.ts`**

Ganti isi `src/index.ts` menjadi:

```typescript
import { Hono } from 'hono';
import type { Env } from './types';
import { activate } from './routes/activate';

const app = new Hono<{ Bindings: Env }>();

app.get('/health', (c) => c.json({ ok: true }));
app.route('/', activate);

app.onError((err, c) => {
  console.error(err);
  return c.json({ error: 'internal_error' }, 500);
});

app.notFound((c) => c.json({ error: 'not_found' }, 404));

export default app;
```

- [ ] **Step 6: Jalankan test untuk memastikan lulus**

Run: `npm test -- activate`
Expected: PASS, enam test hijau.

- [ ] **Step 7: Commit**

```bash
git add src/license.ts src/routes/activate.ts src/index.ts test/activate.test.ts
git commit -m "feat: add activation endpoint issuing master credentials and first token"
```

---

### Task 6: Endpoint `POST /v1/checkin`

**Files:**
- Create: `src/routes/checkin.ts`
- Modify: `src/index.ts`
- Create: `test/checkin.test.ts`

**Interfaces:**
- Consumes: `getMaster`, `getTenant`, `upsertDevices`, `recomputeOverage`, `sha256Hex`, `timingSafeEqual` (Task 4); `issueToken` (Task 5)
- Produces: Route `POST /v1/checkin`

- [ ] **Step 1: Tulis test yang gagal**

`test/checkin.test.ts`:

```typescript
import { env, applyD1Migrations } from 'cloudflare:test';
import { describe, it, expect, beforeAll, beforeEach } from 'vitest';
import app from '../src/index';
import { importPublicKey, verifyToken, generateTestKeyPair } from '../src/token';

let testEnv: typeof env & { LICENSE_PRIVATE_KEY: string };
let publicKey: CryptoKey;
let masterId: string;
let secret: string;

beforeAll(async () => {
  await applyD1Migrations(env.DB, env.TEST_MIGRATIONS);
  const pair = await generateTestKeyPair();
  publicKey = await importPublicKey(pair.publicPem);
  testEnv = { ...env, LICENSE_PRIVATE_KEY: pair.privatePem };
});

beforeEach(async () => {
  await env.DB.batch([
    env.DB.prepare('DELETE FROM device'),
    env.DB.prepare('DELETE FROM master'),
    env.DB.prepare('DELETE FROM activation_code'),
    env.DB.prepare('DELETE FROM tenant'),
  ]);
  await env.DB.batch([
    env.DB.prepare(
      `INSERT INTO tenant (id, name, plan, max_devices, sub_end, overage_since, created_at)
       VALUES ('sch_1', 'SMAN 1 Bandung', 'paid', 2, '2026-12-01T00:00:00.000Z', NULL, '2026-09-19T00:00:00.000Z')`
    ),
    env.DB.prepare(
      `INSERT INTO activation_code (code, tenant_id, used_at, expires_at, created_at)
       VALUES ('KHW-TEST-0001', 'sch_1', NULL, '2027-01-01T00:00:00.000Z', '2026-09-19T00:00:00.000Z')`
    ),
  ]);

  const res = await app.request(
    '/v1/activate',
    {
      method: 'POST',
      body: JSON.stringify({ code: 'KHW-TEST-0001', hostname: 'guru-pc', version: '4.11.0' }),
      headers: { 'content-type': 'application/json' },
    },
    testEnv
  );
  const body = await res.json<{ master_id: string; secret: string }>();
  masterId = body.master_id;
  secret = body.secret;
});

function checkin(body: unknown, bearer: string = secret) {
  return app.request(
    '/v1/checkin',
    {
      method: 'POST',
      body: JSON.stringify(body),
      headers: { 'content-type': 'application/json', authorization: `Bearer ${bearer}` },
    },
    testEnv
  );
}

describe('POST /v1/checkin', () => {
  it('records devices and returns a fresh token', async () => {
    const res = await checkin({
      master_id: masterId,
      devices: [
        { mac: 'AA:BB:CC:DD:EE:FF', hostname: 'lab1-pc01' },
        { mac: null, hostname: 'lab1-pc02' },
      ],
    });
    expect(res.status).toBe(200);
    const body = await res.json<{ token: string }>();
    const payload = await verifyToken(body.token, publicKey);
    expect(payload!.sub).toBe(masterId);
    expect(payload!.max_devices).toBe(2);
    expect(payload!.overage_since).toBeNull();

    const { results } = await env.DB.prepare('SELECT * FROM device WHERE tenant_id = ?1')
      .bind('sch_1')
      .all();
    expect(results).toHaveLength(2);
  });

  it('flags overage when unique devices exceed the quota', async () => {
    const res = await checkin({
      master_id: masterId,
      devices: [
        { mac: null, hostname: 'pc-1' },
        { mac: null, hostname: 'pc-2' },
        { mac: null, hostname: 'pc-3' },
      ],
    });
    const payload = await verifyToken((await res.json<{ token: string }>()).token, publicKey);
    expect(payload!.overage_since).not.toBeNull();
  });

  it('deduplicates devices reported by two different masters', async () => {
    await env.DB.prepare(
      `INSERT INTO activation_code (code, tenant_id, used_at, expires_at, created_at)
       VALUES ('KHW-TEST-0002', 'sch_1', NULL, '2027-01-01T00:00:00.000Z', '2026-09-19T00:00:00.000Z')`
    ).run();
    const second = await app.request(
      '/v1/activate',
      {
        method: 'POST',
        body: JSON.stringify({ code: 'KHW-TEST-0002', hostname: 'guru2-pc', version: '4.11.0' }),
        headers: { 'content-type': 'application/json' },
      },
      testEnv
    );
    const other = await second.json<{ master_id: string; secret: string }>();

    await checkin({ master_id: masterId, devices: [{ mac: null, hostname: 'shared-pc' }] });
    await checkin(
      { master_id: other.master_id, devices: [{ mac: null, hostname: 'SHARED-PC' }] },
      other.secret
    );

    const row = await env.DB.prepare('SELECT COUNT(*) AS n FROM device WHERE tenant_id = ?1')
      .bind('sch_1')
      .first<{ n: number }>();
    expect(row!.n).toBe(1);
  });

  it('rejects a wrong secret with 401', async () => {
    const res = await checkin({ master_id: masterId, devices: [] }, 'f'.repeat(64));
    expect(res.status).toBe(401);
  });

  it('rejects a missing authorization header', async () => {
    const res = await app.request(
      '/v1/checkin',
      {
        method: 'POST',
        body: JSON.stringify({ master_id: masterId, devices: [] }),
        headers: { 'content-type': 'application/json' },
      },
      testEnv
    );
    expect(res.status).toBe(401);
  });

  it('rejects an unknown master with 401', async () => {
    const res = await checkin({ master_id: 'mst_nope', devices: [] });
    expect(res.status).toBe(401);
  });

  it('updates last_seen_at on the master', async () => {
    await checkin({ master_id: masterId, devices: [] });
    const row = await env.DB.prepare('SELECT last_seen_at FROM master WHERE id = ?1')
      .bind(masterId)
      .first<{ last_seen_at: string }>();
    expect(row!.last_seen_at).not.toBeNull();
  });
});
```

- [ ] **Step 2: Jalankan test untuk memastikan gagal**

Run: `npm test -- checkin`
Expected: FAIL — route belum ada, balasan 404.

- [ ] **Step 3: Implementasi route**

`src/routes/checkin.ts`:

```typescript
import { Hono } from 'hono';
import type { Env } from '../types';
import {
  getMaster,
  getTenant,
  recomputeOverage,
  sha256Hex,
  timingSafeEqual,
  upsertDevices,
} from '../db';
import { issueToken } from '../license';
import type { DeviceIdentity } from '../devices';

export const checkin = new Hono<{ Bindings: Env }>();

checkin.post('/v1/checkin', async (c) => {
  const auth = c.req.header('authorization') ?? '';
  const secret = auth.startsWith('Bearer ') ? auth.slice(7) : null;
  if (!secret) return c.json({ error: 'unauthorized' }, 401);

  const body = await c.req.json().catch(() => null);
  const masterId = typeof body?.master_id === 'string' ? body.master_id : null;
  if (!masterId) return c.json({ error: 'invalid_request' }, 400);

  const master = await getMaster(c.env.DB, masterId);
  if (!master) return c.json({ error: 'unauthorized' }, 401);
  if (!timingSafeEqual(master.secret_hash, await sha256Hex(secret))) {
    return c.json({ error: 'unauthorized' }, 401);
  }

  const reported: DeviceIdentity[] = Array.isArray(body?.devices)
    ? body.devices
        .filter((d: unknown): d is { mac?: unknown; hostname?: unknown } => typeof d === 'object' && d !== null)
        .filter((d) => typeof d.hostname === 'string')
        .map((d) => ({
          mac: typeof d.mac === 'string' ? d.mac : null,
          hostname: d.hostname as string,
        }))
    : [];

  const now = new Date();
  const nowIso = now.toISOString();

  await upsertDevices(c.env.DB, master.tenant_id, reported, now);
  await c.env.DB.prepare('UPDATE master SET last_seen_at = ?1 WHERE id = ?2')
    .bind(nowIso, masterId)
    .run();

  // Dihitung ulang dari keadaan saat ini, bukan digeser — aman bila terulang.
  await recomputeOverage(c.env.DB, master.tenant_id, now);

  const tenant = await getTenant(c.env.DB, master.tenant_id);
  if (!tenant) return c.json({ error: 'internal_error' }, 500);

  return c.json({ token: await issueToken(c.env, tenant, masterId, now) });
});
```

- [ ] **Step 4: Pasang route di `src/index.ts`**

Tambahkan import dan pemasangan route:

```typescript
import { checkin } from './routes/checkin';
```

dan setelah `app.route('/', activate);`:

```typescript
app.route('/', checkin);
```

- [ ] **Step 5: Jalankan seluruh test**

Run: `npm test`
Expected: PASS, semua test dari Task 1–6 hijau.

- [ ] **Step 6: Commit**

```bash
git add src/routes/checkin.ts src/index.ts test/checkin.test.ts
git commit -m "feat: add checkin endpoint with device dedup and overage detection"
```

---

### Task 7: Skrip admin untuk pilot

**Files:**
- Create: `scripts/keygen.ts`
- Create: `scripts/admin.sql.md`
- Create: `README.md`

**Interfaces:**
- Consumes: —
- Produces: Prosedur manual untuk membuat tenant, menerbitkan kode aktivasi, dan memperpanjang langganan.

Untuk pilot beberapa sekolah, operasi admin dilakukan lewat `wrangler d1 execute`.
Membangun API admin berikut autentikasinya untuk tiga operasi manual tidak
sebanding; itu masuk subsistem 3 bersama dashboard.

- [ ] **Step 1: Tulis skrip pembuat kunci**

`scripts/keygen.ts`:

```typescript
/**
 * Membuat pasangan kunci RSA-4096 untuk penandatanganan token.
 * Jalankan: npx tsx scripts/keygen.ts
 *
 * Private key -> Workers Secret LICENSE_PRIVATE_KEY (jangan pernah di-commit)
 * Public key   -> ditanam di binary Veyon Master (subsistem 2)
 */
import { webcrypto } from 'node:crypto';
import { writeFileSync } from 'node:fs';

const ALG = { name: 'RSASSA-PKCS1-v1_5', hash: 'SHA-512' } as const;

function toPem(buffer: ArrayBuffer, label: string): string {
  const b64 = Buffer.from(buffer).toString('base64').replace(/(.{64})/g, '$1\n');
  return `-----BEGIN ${label}-----\n${b64}\n-----END ${label}-----\n`;
}

const pair = (await webcrypto.subtle.generateKey(
  { ...ALG, modulusLength: 4096, publicExponent: new Uint8Array([1, 0, 1]) },
  true,
  ['sign', 'verify']
)) as CryptoKeyPair;

writeFileSync('license-private.pem', toPem(await webcrypto.subtle.exportKey('pkcs8', pair.privateKey), 'PRIVATE KEY'), { mode: 0o600 });
writeFileSync('license-public.pem', toPem(await webcrypto.subtle.exportKey('spki', pair.publicKey), 'PUBLIC KEY'));

console.log('Wrote license-private.pem (mode 600) and license-public.pem');
console.log('Upload the private key:  npx wrangler secret put LICENSE_PRIVATE_KEY < license-private.pem');
```

- [ ] **Step 2: Tulis prosedur admin**

`scripts/admin.sql.md`:

````markdown
# Operasi admin pilot

Semua timestamp ISO-8601 UTC. Ganti nilai contoh sesuai kebutuhan.

## Membuat sekolah baru dengan trial 30 hari

```bash
npx wrangler d1 execute khwarizmi-license --remote --command \
"INSERT INTO tenant (id, name, plan, max_devices, sub_end, overage_since, created_at)
 VALUES ('sch_001', 'SMAN 1 Bandung', 'trial', 40, '2026-10-19T00:00:00.000Z', NULL, '2026-09-19T00:00:00.000Z')"
```

## Menerbitkan kode aktivasi

```bash
npx wrangler d1 execute khwarizmi-license --remote --command \
"INSERT INTO activation_code (code, tenant_id, used_at, expires_at, created_at)
 VALUES ('KHW-7F3A-9B2C', 'sch_001', NULL, '2026-12-31T00:00:00.000Z', '2026-09-19T00:00:00.000Z')"
```

## Memperpanjang langganan setelah pembayaran diterima

```bash
npx wrangler d1 execute khwarizmi-license --remote --command \
"UPDATE tenant SET plan = 'paid', sub_end = '2027-09-19T00:00:00.000Z' WHERE id = 'sch_001'"
```

## Mengubah kuota device

```bash
npx wrangler d1 execute khwarizmi-license --remote --command \
"UPDATE tenant SET max_devices = 60 WHERE id = 'sch_001'"
```

Kelebihan kuota dihitung ulang otomatis pada check-in berikutnya; `overage_since`
tidak perlu disentuh manual.

## Melihat pemakaian kuota sebuah sekolah

```bash
npx wrangler d1 execute khwarizmi-license --remote --command \
"SELECT t.name, t.max_devices, t.sub_end, t.overage_since,
        (SELECT COUNT(*) FROM device d
          WHERE d.tenant_id = t.id
            AND d.last_seen_at >= datetime('now', '-30 days')) AS active_devices
   FROM tenant t WHERE t.id = 'sch_001'"
```
````

- [ ] **Step 3: Tulis README**

`README.md`:

```markdown
# Khwarizmi License Backend

Backend lisensi untuk Khwarizmi (fork Veyon). Menerbitkan token JWT RS512
bertanda tangan kepada instalasi Veyon Master.

Desain: `docs/superpowers/specs/2026-09-19-khwarizmi-saas-licensing-design.md`
di repo veyon.

## Setup

```bash
npm install
npx wrangler d1 create khwarizmi-license   # salin database_id ke wrangler.jsonc
npm run migrate:local
npx tsx scripts/keygen.ts
npx wrangler secret put LICENSE_PRIVATE_KEY < license-private.pem
```

`license-public.pem` ditanam di binary Veyon Master (subsistem 2).
`license-private.pem` tidak pernah masuk repo — sudah ada di `.gitignore`.

## Perintah

| Perintah | Kegunaan |
|---|---|
| `npm test` | Jalankan seluruh test |
| `npm run dev` | Server lokal |
| `npm run migrate:remote` | Terapkan migrasi ke D1 produksi |
| `npm run deploy` | Deploy ke Workers |

## Endpoint

| Endpoint | Auth | Kegunaan |
|---|---|---|
| `POST /v1/activate` | kode aktivasi | Tukar kode dengan kredensial master + token pertama |
| `POST /v1/checkin` | `Bearer <secret>` | Laporkan device, terima token baru |

Webhook pembayaran adalah bagian subsistem 3.

## Invariant

Endpoint hanya menerima identitas master dan device. **Tidak pernah** data
pemantauan: screenshot, aktivitas murid, atau nama pengguna. Lihat spec §10.

Operasi admin pilot: `scripts/admin.sql.md`.
```

- [ ] **Step 4: Verifikasi skrip kunci berjalan**

```bash
npm install -D tsx
npx tsx scripts/keygen.ts
```

Expected: mencetak dua baris konfirmasi, menghasilkan `license-private.pem` dan
`license-public.pem`. Pastikan keduanya **tidak** muncul di `git status` (sudah
tercakup `*.pem` di `.gitignore`).

- [ ] **Step 5: Jalankan seluruh test sekali lagi**

Run: `npm test`
Expected: PASS, seluruh test hijau.

- [ ] **Step 6: Commit**

```bash
git add scripts/ README.md package.json package-lock.json
git commit -m "docs: add keygen script, pilot admin procedures, and README"
```

---

## Setelah rencana ini selesai

Backend siap menerima aktivasi dan check-in. Yang belum ada dan memang bukan
bagian subsistem ini:

- **Subsistem 2 (klien Veyon):** `LicenseManager` di `core`, penegakan di
  `master`, halaman Lisensi di Configurator. Membutuhkan `license-public.pem`
  dari Task 7.
- **Subsistem 3 (dashboard + gateway):** webhook pembayaran, tabel `invoice`,
  dashboard web. Menunggu keputusan payment gateway (spec §15).

Satu hal dari spec yang sengaja belum ada di mana pun, dicatat agar tidak hilang:
**penanda `tenant_terminated`** (spec §11). Klien memperlakukan 401/403 sebagai
kegagalan koneksi kecuali penanda ini muncul, tetapi backend belum punya operasi
"menghentikan tenant" sama sekali — belum ada yang menerbitkannya. Ini benar
untuk sekarang: pilot tidak membutuhkannya, dan penegakan berjalan lewat
`sub_end` yang tidak diperpanjang. Tambahkan bersama dashboard di subsistem 3,
saat penghentian tenant menjadi operasi nyata.
