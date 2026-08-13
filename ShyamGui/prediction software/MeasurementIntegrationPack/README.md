# Measurement Integration Pack

Give this **entire folder** to your other Atomik / prediction project (copy it into that project root), then open `CURSOR_PROMPT.md` and paste its contents into Cursor in that project.

## Contents

```text
MeasurementIntegrationPack/
  CURSOR_PROMPT.md          <-- paste this into Cursor in the other project
  README.md                 <-- you are here
  Data/
    manifest.csv            <-- index of all 42 sweeps
    ShyamGuild_*.csv        <-- reference real data (use these curves)
    Factory_*.csv
    3inch_*.csv
    XN18_*.csv
    source/                 <-- original Excel files
      ShyamGuild/
      Factory/
      3inch/
      XN18/
  specs/
    DATA_SCHEMA.md
    QUALITY_AND_DEFAULTS.md
    metrics_summary.json
```

## Quick start in the other project

1. Copy `MeasurementIntegrationPack` into the other project root.
2. Open that project in Cursor.
3. Open `MeasurementIntegrationPack/CURSOR_PROMPT.md`.
4. Copy everything under the horizontal line (`---`) into the chat.
5. Tell Cursor: *“Follow CURSOR_PROMPT.md and integrate MeasurementIntegrationPack/Data into this existing project.”*

## Reference set

**ShyamGuild** at **0.5 m**, frequencies **80 / 200 / 500 Hz**, is the trusted measured-directivity source. Curves in the CSVs match the Excel files exactly.
