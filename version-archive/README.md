# Atomik version archive

Standalone HTML + MongoDB backup of app releases (**not** wired into the JUCE software).

Starts at **v1.3.0**. Each entry can offer:

- Source code zip
- macOS DMG
- Windows EXE

## Quick use

```bash
cd version-archive
cp .env.example .env   # once — put MONGODB_URI
npm install
node publish-version.mjs 1.3.0 "Baseline Q21S BEM heatmap release"
npm start              # http://127.0.0.1:8787
```

## Next releases

**Only when you explicitly ask to bump/buff the version** — do not publish to the HTML archive on ordinary changes.

1. Update `ShyamGui/CHANGELOG.md` and `ShyamGui/Installer/make_changelog_xlsx.py`, then:
   ```bash
   python3 ShyamGui/Installer/make_changelog_xlsx.py
   ```
2. Publish the archive entry:
   ```bash
   node publish-version.mjs 1.3.1 "Short notes about the change"
   ```

Cursor rule: `.cursor/rules/version-archive.mdc` (loads when working on releases / this folder).
