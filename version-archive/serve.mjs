#!/usr/bin/env node
/**
 * Local archive server — serves index.html and streams release files from GridFS.
 * Not part of the JUCE app. Run: npm start
 */
import 'dotenv/config';
import http from 'http';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';
import { MongoClient, GridFSBucket, ObjectId } from 'mongodb';
import { fetchVersionsFromDb } from './lib/sync.mjs';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const PORT = Number(process.env.PORT || 8787);

if (!process.env.MONGODB_URI) {
  console.error('Missing MONGODB_URI in .env');
  process.exit(1);
}

const client = new MongoClient(process.env.MONGODB_URI);
await client.connect();
const db = client.db('atomik_releases');
const bucket = new GridFSBucket(db, { bucketName: 'release_files' });

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

const server = http.createServer(async (req, res) => {
  try {
    const url = new URL(req.url, `http://127.0.0.1:${PORT}`);

    if (url.pathname === '/api/versions') {
      const versions = await fetchVersionsFromDb();
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
      const doc = await db.collection('versions').findOne({ version });
      const meta = doc?.files?.[kind];
      if (!meta?.fileId) return send(res, 404, `No ${kind} artifact for v${version}`);

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
  console.log(`Version archive: http://127.0.0.1:${PORT}`);
});
