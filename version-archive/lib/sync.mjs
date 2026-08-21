import 'dotenv/config';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';
import { MongoClient } from 'mongodb';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const ARCHIVE_ROOT = path.resolve(__dirname, '..');

export async function fetchVersionsFromDb() {
  if (!process.env.MONGODB_URI) throw new Error('Missing MONGODB_URI');
  const client = new MongoClient(process.env.MONGODB_URI);
  await client.connect();
  try {
    const docs = await client
      .db('atomik_releases')
      .collection('versions')
      .find({})
      .sort({ version: -1 })
      .toArray();

    return docs.map((d) => ({
      version: d.version,
      notes: d.notes || '',
      createdAt: d.createdAt ? new Date(d.createdAt).toISOString() : null,
      updatedAt: d.updatedAt ? new Date(d.updatedAt).toISOString() : null,
      downloads: {
        source: d.files?.source
          ? { available: true, filename: d.files.source.filename, size: d.files.source.size }
          : { available: false },
        mac: d.files?.mac
          ? { available: true, filename: d.files.mac.filename, size: d.files.mac.size }
          : { available: false },
        windows: d.files?.windows
          ? { available: true, filename: d.files.windows.filename, size: d.files.windows.size }
          : { available: false },
      },
    }));
  } finally {
    await client.close();
  }
}

export async function syncVersionsJson() {
  const versions = await fetchVersionsFromDb();
  // Semver-ish sort newest first
  versions.sort((a, b) => {
    const pa = a.version.split('.').map(Number);
    const pb = b.version.split('.').map(Number);
    for (let i = 0; i < 3; i++) if ((pb[i] || 0) !== (pa[i] || 0)) return (pb[i] || 0) - (pa[i] || 0);
    return 0;
  });
  const out = path.join(ARCHIVE_ROOT, 'versions.json');
  fs.writeFileSync(out, JSON.stringify({ generatedAt: new Date().toISOString(), versions }, null, 2) + '\n');
  console.log('Wrote', out, `(${versions.length} versions)`);
  return versions;
}
