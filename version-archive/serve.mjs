#!/usr/bin/env node
/**
 * Local archive server — serves index.html and streams release files from
 * local artifacts/ (preferred) or MongoDB GridFS when configured.
 * Not part of the JUCE app. Run: npm start
 */
import 'dotenv/config';
import http from 'http';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';
import { MongoClient, GridFSBucket, ObjectId } from 'mongodb';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const PORT = Number(process.env.PORT || 8787);
const ARTIFACTS = path.join(__dirname, 'artifacts');
const VERSIONS_JSON = path.join(__dirname, 'versions.json');

const hasMongo = Boolean(process.env.MONGODB_URI);
let client = null;
let db = null;
let bucket = null;

if (hasMongo) {
  client = new MongoClient(process.env.MONGODB_URI);
  await client.connect();
  db = client.db('atomik_releases');
  bucket = new GridFSBucket(db, { bucketName: 'release_files' });
}

function send(res, code, body, type = 'text/plain') {
  res.writeHead(code, { 'Content-Type': type, 'Cache-Control': 'no-store' });
  res.end(body);
}

function contentType(name) {
  if (name.endsWith('.html')) return 'text/html; charset=utf-8';
  if (name.endsWith('.json')) return 'application/json';
  if (name.endsWith('.css')) return 'text/css';
  if (name.endsWith('.js')) return 'text/javascript';
  if (name.endsWith('.dmg')) return 'application/x-apple-diskimage';
  if (name.endsWith('.zip')) return 'application/zip';
  if (name.endsWith('.exe')) return 'application/vnd.microsoft.portable-executable';
  return 'application/octet-stream';
}

function readLocalVersions() {
  const raw = JSON.parse(fs.readFileSync(VERSIONS_JSON, 'utf8'));
  return raw.versions || [];
}

async function fetchVersions() {
  if (!hasMongo) return readLocalVersions();
  try {
    const { fetchVersionsFromDb } = await import('./lib/sync.mjs');
    const versions = await fetchVersionsFromDb();
    // Prefer local Windows artifact flag if GridFS has not been updated yet.
    const local = readLocalVersions();
    const byVer = Object.fromEntries(local.map((v) => [v.version, v]));
    return versions.map((v) => {
      const loc = byVer[v.version];
      if (!loc) return v;
      const merged = { ...v, downloads: { ...v.downloads } };
      for (const kind of ['source', 'mac', 'windows']) {
        const localDl = loc.downloads?.[kind];
        if (localDl?.available && !merged.downloads?.[kind]?.available) {
          merged.downloads[kind] = localDl;
        }
      }
      return merged;
    });
  } catch {
    return readLocalVersions();
  }
}

function localArtifactPath(version, kind) {
  const versions = readLocalVersions();
  const doc = versions.find((v) => v.version === version);
  const meta = doc?.downloads?.[kind];
  if (!meta?.available || !meta.filename) return null;
  const full = path.join(ARTIFACTS, meta.filename);
  if (!full.startsWith(ARTIFACTS)) return null;
  if (!fs.existsSync(full) || !fs.statSync(full).isFile()) return null;
  return { full, filename: path.basename(meta.filename), size: meta.size || fs.statSync(full).size };
}

function streamLocal(res, info) {
  res.writeHead(200, {
    'Content-Type': contentType(info.filename),
    'Content-Disposition': `attachment; filename="${info.filename}"`,
    'Content-Length': info.size,
  });
  fs.createReadStream(info.full).pipe(res);
}

const server = http.createServer(async (req, res) => {
  try {
    const url = new URL(req.url, `http://127.0.0.1:${PORT}`);

    if (url.pathname === '/api/versions') {
      const versions = await fetchVersions();
      versions.sort((a, b) => {
        const pa = a.version.split('.').map(Number);
        const pb = b.version.split('.').map(Number);
        for (let i = 0; i < 3; i++) if ((pb[i] || 0) !== (pa[i] || 0)) return (pb[i] || 0) - (pa[i] || 0);
        return 0;
      });
      return send(res, 200, JSON.stringify({ versions }), 'application/json');
    }

    const dl = url.pathname.match(/^\/download\/([^/]+)\/(source|mac|windows)$/);
    if (dl) {
      const version = dl[1].replace(/^v/i, '');
      const kind = dl[2];

      const local = localArtifactPath(version, kind);
      if (local) {
        streamLocal(res, local);
        return;
      }

      if (hasMongo && db) {
        const doc = await db.collection('versions').findOne({ version });
        const meta = doc?.files?.[kind];
        if (meta?.fileId) {
          const id = new ObjectId(meta.fileId);
          const filename = meta.filename.split('/').pop();
          res.writeHead(200, {
            'Content-Type': contentType(filename),
            'Content-Disposition': `attachment; filename="${filename}"`,
            'Content-Length': meta.size,
          });
          bucket.openDownloadStream(id).on('error', () => {
            if (!res.headersSent) send(res, 500, 'Download failed');
            else res.destroy();
          }).pipe(res);
          return;
        }
      }

      return send(res, 404, `No ${kind} artifact for v${version}`);
    }

    let filePath = path.join(__dirname, url.pathname === '/' ? 'index.html' : url.pathname);
    if (!filePath.startsWith(__dirname)) return send(res, 403, 'Forbidden');
    if (!fs.existsSync(filePath) || fs.statSync(filePath).isDirectory()) {
      return send(res, 404, 'Not found');
    }
    const data = fs.readFileSync(filePath);
    return send(res, 200, data, contentType(filePath));
  } catch (err) {
    console.error(err);
    if (!res.headersSent) send(res, 500, String(err.message || err));
  }
});

server.listen(PORT, '127.0.0.1', () => {
  const mode = hasMongo ? 'MongoDB + local artifacts' : 'local artifacts (no .env)';
  console.log(`Version archive: http://127.0.0.1:${PORT}  [${mode}]`);
});
