#!/usr/bin/env node
/**
 * Upload every version listed in versions.json to MongoDB: each local
 * artifact goes to GridFS (replacing any copy of the same name) and the
 * version document is upserted with its notes and dates. Then versions.json
 * is refreshed from the database.
 *
 * Usage: node publish-all.mjs            (all versions)
 *        node publish-all.mjs 1.4.4 1.4.2 (just these)
 *
 * Unlike publish-version.mjs this packages nothing; it ships the files that
 * are already under artifacts/. A kind whose file is missing locally is left
 * as it is in the database.
 */
import 'dotenv/config';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';
import { MongoClient, GridFSBucket } from 'mongodb';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const ARTIFACTS = path.join(__dirname, 'artifacts');
const VERSIONS_JSON = path.join(__dirname, 'versions.json');

if (!process.env.MONGODB_URI) {
  console.error('Missing MONGODB_URI in version-archive/.env');
  process.exit(1);
}

const only = process.argv.slice(2).map((v) => v.replace(/^v/i, ''));
const local = JSON.parse(fs.readFileSync(VERSIONS_JSON, 'utf8')).versions || [];
const todo = only.length ? local.filter((v) => only.includes(v.version)) : local;
if (!todo.length) {
  console.error('No matching versions in versions.json');
  process.exit(1);
}

const client = new MongoClient(process.env.MONGODB_URI);
await client.connect();
const db = client.db('atomik_releases');
const versions = db.collection('versions');
const bucket = new GridFSBucket(db, { bucketName: 'release_files' });

async function replaceFile(version, kind, filename, localPath) {
  for (const doc of await bucket.find({ filename }).toArray())
    await bucket.delete(doc._id);
  await new Promise((resolve, reject) => {
    fs.createReadStream(localPath)
      .pipe(bucket.openUploadStream(filename, { metadata: { version, kind } }))
      .on('error', reject)
      .on('finish', resolve);
  });
  const [meta] = await bucket.find({ filename }).toArray();
  return { filename, fileId: meta._id.toString(), size: meta.length };
}

let uploaded = 0;
try {
  for (const v of todo) {
    const set = {
      version: v.version,
      notes: v.notes || '',
      updatedAt: v.updatedAt ? new Date(v.updatedAt) : new Date(),
    };
    const parts = [];
    for (const kind of ['source', 'mac', 'windows']) {
      const dl = v.downloads?.[kind];
      if (!dl?.available || !dl.filename) continue;
      const full = path.join(ARTIFACTS, dl.filename);
      if (!fs.existsSync(full)) {
        console.warn(`  v${v.version} ${kind}: ${dl.filename} not found locally - left as is`);
        continue;
      }
      set[`files.${kind}`] = await replaceFile(v.version, kind, dl.filename, full);
      parts.push(`${kind} ${(fs.statSync(full).size / 1048576).toFixed(1)} MB`);
      ++uploaded;
    }
    await versions.updateOne(
      { version: v.version },
      {
        $set: set,
        $setOnInsert: { createdAt: v.createdAt ? new Date(v.createdAt) : new Date() },
      },
      { upsert: true }
    );
    console.log(`v${v.version}: ${parts.length ? parts.join(', ') : 'metadata only'}`);
  }
} finally {
  await client.close();
}

console.log(`\n${todo.length} version(s), ${uploaded} file(s) uploaded to atomik_releases.`);

const { syncVersionsJson } = await import('./lib/sync.mjs');
await syncVersionsJson();
