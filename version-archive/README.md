# Atomik version archive

Standalone HTML + MongoDB backup of app releases (**not** wired into the JUCE software).

Starts at **v1.3.0**. Current product cut: **v1.3.6**. Each entry can offer:

- Source code zip
- macOS DMG
- Windows Setup / ZIP

## Quick use

```bash
cd version-archive
cp .env.example .env   # once — put MONGODB_URI
npm install
node publish-version.mjs 1.3.6 "Mic ring snap + shape-edge Snap tak"
npm start              # http://127.0.0.1:8787
```

Local-only (no Mongo): put files under `artifacts/vX.Y.Z/` and list them in `versions.json` — `npm start` serves downloads from disk.

## Next releases

**Only when you explicitly ask to bump/buff the version** — do not publish to the HTML archive on ordinary changes.

1. Update `ShyamGui/CHANGELOG.md` and `ShyamGui/Installer/make_changelog_xlsx.py`, then:
   ```bash
   python3 ShyamGui/Installer/make_changelog_xlsx.py
   ```
2. Publish the archive entry:
   ```bash
   node publish-version.mjs 1.3.7 "Short notes about the change"
   ```

Cursor rule: `.cursor/rules/version-archive.mdc` (loads when working on releases / this folder).
