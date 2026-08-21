#!/usr/bin/env node
/**
 * Package a release, upload to MongoDB GridFS, upsert metadata, refresh versions.json.
 *
 * Usage: node publish-version.mjs <version> ["optional notes"]
 * Example: node publish-version.mjs 1.3.0 "Baseline Q21S BEM heatmap release"
 */
import 'dotenv/config';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';
import { spawnSync } from 'child_process';
import { MongoClient, GridFSBucket } from 'mongodb';
import { createReadStream } from 'fs';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const ROOT = path.resolve(__dirname, '..');
const ARTIFACTS = path.join(__dirname, 'artifacts');

const version = (process.argv[2] || '').replace(/^v/i, '');
const notes = process.argv[3] || '';

if (!/^\d+\.\d+\.\d+$/.test(version)) {
  console.error('Usage: node publish-version.mjs <x.y.z> ["notes"]');
  process.exit(1);
}

if (!process.env.MONGODB_URI) {
  console.error('Missing MONGODB_URI in version-archive/.env');
  process.exit(1);
}

const outDir = path.join(ARTIFACTS, `v${version}`);
fs.mkdirSync(outDir, { recursive: true });

function run(cmd, args, opts = {}) {
  const r = spawnSync(cmd, args, { stdio: 'inherit', ...opts });
  if (r.status !== 0) throw new Error(`${cmd} ${args.join(' ')} failed`);
}

function fileExists(p) {
  try { return fs.statSync(p).isFile(); } catch { return false; }
}

function dirExists(p) {
  try { return fs.statSync(p).isDirectory(); } catch { return false; }
}

// --- 1) Source zip (app sources + Q21S pack + docs; no JUCE / build junk) ---
const sourceZip = path.join(outDir, `Atomik-Source-v${version}.zip`);
const sourceList = path.join(outDir, '_source_paths.txt');
const includePaths = [
  'ShyamGui/Source',
  'ShyamGui/Builds/MacManual/build_macos15.sh',
  'ShyamGui/Builds/MacManual/AppConfig.h',
  'ShyamGui/Installer',
  'ShyamGui/Tools',
  'ShyamGui/prediction software/MeasurementIntegrationPack',
  'ShyamGui/CHANGELOG.md',
  'docs/SINGLE_SUB_HEATMAP.md',
  'docs/single_sub_heatmap',
  'docs/q21s_bem_plots/generate_single_sub_heatmaps.py',
  'BEM_Data_10m/Heatmap.m',
  'version-archive/README.md',
].filter((rel) => {
  const abs = path.join(ROOT, rel);
  return fileExists(abs) || dirExists(abs);
});

fs.writeFileSync(sourceList, includePaths.join('\n') + '\n');
if (fileExists(sourceZip)) fs.unlinkSync(sourceZip);
run('zip', ['-r', '-q', sourceZip, ...includePaths], { cwd: ROOT });
console.log('Source zip:', sourceZip);

// --- 2) macOS DMG (from built .app if present) ---
const appPath = path.join(
  ROOT,
  'ShyamGui/Builds/MacManual/build/Atomik Acoustic Simulation Engine.app'
);
let macDmg = path.join(outDir, `Atomik-macOS-v${version}.dmg`);
let macAvailable = false;

if (dirExists(appPath)) {
  const stage = path.join(outDir, '_dmg_stage');
  fs.rmSync(stage, { recursive: true, force: true });
  fs.mkdirSync(stage, { recursive: true });
  run('cp', ['-R', appPath, path.join(stage, 'Atomik Acoustic Simulation Engine.app')]);
  if (fileExists(macDmg)) fs.unlinkSync(macDmg);
  run('hdiutil', [
    'create',
    '-volname', `Atomik v${version}`,
    '-srcfolder', stage,
    '-ov',
    '-format', 'UDZO',
    macDmg,
  ]);
  fs.rmSync(stage, { recursive: true, force: true });
  macAvailable = true;
  console.log('macOS DMG:', macDmg);
} else {
  console.warn('No macOS .app found — mac download will be empty until you build.');
  macDmg = null;
}

// --- 3) Windows EXE (optional; often built on Windows) ---
const winCandidates = [
  path.join(ROOT, 'ShyamGui/Builds/Release/Atomik Simulation Engine.exe'),
  path.join(ROOT, 'ShyamGui/Builds/VisualStudio2022/x64/Release/Atomik Simulation Engine.exe'),
];
let winExe = path.join(outDir, `Atomik-Windows-v${version}.exe`);
let winAvailable = false;
for (const c of winCandidates) {
  if (fileExists(c)) {
    fs.copyFileSync(c, winExe);
    winAvailable = true;
    console.log('Windows EXE:', winExe);
    break;
  }
}
if (!winAvailable) {
  console.warn('No Windows EXE found — Windows download left empty for this version.');
  winExe = null;
}

// --- 4) Upload to MongoDB ---
const client = new MongoClient(process.env.MONGODB_URI);
await client.connect();
const db = client.db('atomik_releases');
const versions = db.collection('versions');
const bucket = new GridFSBucket(db, { bucketName: 'release_files' });

async function replaceFile(filename, localPath) {
  const existing = await bucket.find({ filename }).toArray();
  for (const doc of existing) await bucket.delete(doc._id);
  await new Promise((resolve, reject) => {
    createReadStream(localPath)
      .pipe(bucket.openUploadStream(filename, {
        metadata: { version, kind: filename.includes('Source') ? 'source'
          : filename.includes('macOS') ? 'mac'
          : filename.includes('Windows') ? 'windows' : 'other' },
      }))
      .on('error', reject)
      .on('finish', resolve);
  });
  const [meta] = await bucket.find({ filename }).toArray();
  return {
    filename,
    fileId: meta._id.toString(),
    size: meta.length,
  };
}

const files = {};
files.source = await replaceFile(`v${version}/Atomik-Source-v${version}.zip`, sourceZip);
if (macAvailable) files.mac = await replaceFile(`v${version}/Atomik-macOS-v${version}.dmg`, macDmg);
if (winAvailable) files.windows = await replaceFile(`v${version}/Atomik-Windows-v${version}.exe`, winExe);

const now = new Date();
await versions.updateOne(
  { version },
  {
    $set: {
      version,
      notes,
      updatedAt: now,
      files: {
        source: files.source || null,
        mac: files.mac || null,
        windows: files.windows || null,
      },
    },
    $setOnInsert: { createdAt: now },
  },
  { upsert: true }
);

await client.close();

// Refresh local JSON cache used by HTML / server
const { syncVersionsJson } = await import('./lib/sync.mjs');
await syncVersionsJson();

console.log(`\nPublished v${version} to MongoDB (atomik_releases.versions).`);
console.log('Open archive: cd version-archive && npm start');
