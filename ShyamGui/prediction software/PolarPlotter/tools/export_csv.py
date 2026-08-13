"""
Export all polar Excel readings into PolarPlotter/Data.

ShyamGuild is the reference set for the app (same curves as shyamGuildMeasurements).
Also exports Factory, 3inch, and Gylt/XN18 so every folder is available in one place.
"""
import re
import shutil
from pathlib import Path

import openpyxl

ROOT = Path(r"d:\prediction software")
OUT = Path(r"d:\prediction software\PolarPlotter\Data")
SOURCE_MIRROR = OUT / "source"

# Display name -> (folder, recursive?)
SETS = {
    "ShyamGuild": (ROOT / "shyamGuildMeasurements", False),
    "Factory": (ROOT / "Factory Readings", True),
    "3inch": (ROOT / "3 inch frequency sweep", False),
    "XN18": (ROOT / "GyltReadings_3july" / "curves", False),
}


def parse_name(name: str):
    m = re.match(
        r"Freq(?:uency|unecy)_(\d+(?:\.\d+)?)_(\d+(?:\.\d+)?)Horizantal\.xlsx",
        name,
        re.I,
    )
    if not m:
        return None, None
    return int(float(m.group(1))), float(m.group(2))


def export_file(path: Path, set_name: str):
    freq, dist = parse_name(path.name)
    if freq is None:
        return None

    ws = openpyxl.load_workbook(path, data_only=True).active
    rows = []
    for i, row in enumerate(ws.iter_rows(values_only=True)):
        if i == 0 or row[0] is None or row[1] is None:
            continue
        deg = float(row[0])
        spl = float(row[1])
        # Drop duplicate 360 deg — keep 0 deg only
        if abs(deg - 360.0) < 1e-9:
            continue
        rows.append((deg, spl))
    rows.sort(key=lambda r: r[0])
    if not rows:
        return None

    dist_tag = str(dist).replace(".", "p")
    out_name = f"{set_name}_{freq}Hz_{dist_tag}m.csv"
    out_path = OUT / out_name
    with open(out_path, "w", encoding="utf-8", newline="\n") as f:
        f.write("degree,dBSPL\n")
        # Keep full precision for 1-deg sets; 3 decimals for 10-deg sets
        decimals = 6 if len(rows) > 100 else 3
        for deg, spl in rows:
            f.write(f"{deg:.{1 if decimals == 3 else 1}f},{spl:.{decimals}f}\n")

    # Mirror original xlsx under Data/source/<set>/
    mirror_dir = SOURCE_MIRROR / set_name
    mirror_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(path, mirror_dir / path.name)

    return {
        "set": set_name,
        "freq": freq,
        "dist": dist,
        "file": out_name,
        "source": path.name,
        "n": len(rows),
    }


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    if SOURCE_MIRROR.exists():
        shutil.rmtree(SOURCE_MIRROR)
    SOURCE_MIRROR.mkdir(parents=True, exist_ok=True)

    # Remove old CSVs (keep folder)
    for old in OUT.glob("*.csv"):
        if old.name != "manifest.csv":
            old.unlink()

    manifest = []
    for set_name, (folder, recursive) in SETS.items():
        if not folder.is_dir():
            print(f"  SKIP missing folder: {folder}")
            continue
        files = sorted(folder.rglob("*.xlsx")) if recursive else sorted(folder.glob("*.xlsx"))
        print(f"\n[{set_name}] {folder}")
        for path in files:
            rec = export_file(path, set_name)
            if rec:
                manifest.append(rec)
                print(f"  {rec['file']}  ({rec['n']} pts)  <- {rec['source']}")

    manifest.sort(key=lambda r: (r["set"] != "ShyamGuild", r["set"], r["freq"], r["dist"]))
    with open(OUT / "manifest.csv", "w", encoding="utf-8", newline="\n") as f:
        f.write("set,freq_hz,distance_m,file,source_xlsx,n_points\n")
        for r in manifest:
            f.write(
                f"{r['set']},{r['freq']},{r['dist']},{r['file']},{r['source']},{r['n']}\n"
            )

    print(f"\nExported {len(manifest)} sweeps to {OUT}")
    print("ShyamGuild is first in manifest (reference set).")
    print(f"Original Excel mirrored under {SOURCE_MIRROR}")


if __name__ == "__main__":
    main()
