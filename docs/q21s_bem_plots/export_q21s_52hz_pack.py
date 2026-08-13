#!/usr/bin/env python3
"""Export Q21S 52 Hz pack: polar CSVs + Q21F absolute-field heatmap (Heatmap.m)."""
from __future__ import annotations

import math
import struct
from pathlib import Path

import numpy as np
import openpyxl
from scipy.interpolate import NearestNDInterpolator, griddata

ROOT = Path(__file__).resolve().parents[2]
XLSX = ROOT / "BEM_Data_10m" / "52Hz.xlsx"
OUT = ROOT / "ShyamGui/prediction software/MeasurementIntegrationPack/Data"
P_REF = 20e-6
HZ = 52
DISTANCES = [0.5, 1.0, 2.0]


def spl_db(pr: float, pi: float) -> float:
    m = math.hypot(pr, pi)
    return -200.0 if m <= 0 else 20.0 * math.log10(m / P_REF)


def main() -> None:
    if not XLSX.exists():
        raise SystemExit(f"missing {XLSX}")
    OUT.mkdir(parents=True, exist_ok=True)

    wb = openpyxl.load_workbook(XLSX, data_only=True, read_only=True)
    ws = wb.active
    xs, zs, ss = [], [], []
    for row in ws.iter_rows(min_row=2, values_only=True):
        if not row or row[1] is None:
            continue
        try:
            x, z = float(row[1]), float(row[3])
            pr, pi = float(row[5]), float(row[6])
        except Exception:
            continue
        xs.append(x)
        zs.append(z)
        ss.append(spl_db(pr, pi))
    wb.close()

    xs = np.asarray(xs, float)
    zs = np.asarray(zs, float)
    ss = np.asarray(ss, float)
    print(f"loaded {len(xs)} pts  SPL {ss.min():.2f}..{ss.max():.2f} dB")

    xn = np.unique(xs)
    zn = np.unique(zs)
    Xm, Zm = np.meshgrid(xn, zn)
    SPLm = griddata((xs, zs), ss, (Xm, Zm), method="linear")
    miss = np.isnan(SPLm)
    if miss.any():
        nn = NearestNDInterpolator(np.column_stack([xs, zs]), ss)
        SPLm[miss] = nn(Xm[miss], Zm[miss])

    # --- Q21F absolute field (peak-normalised rel dB) ----------------------
    rel = SPLm - np.nanmax(SPLm)
    nz, nx = rel.shape
    q21f = OUT / f"Q21S_Field_{HZ}Hz.q21f"
    with q21f.open("wb") as f:
        f.write(b"Q21F")
        f.write(struct.pack("<iiii", 1, HZ, nx, nz))
        f.write(struct.pack("<ffff", float(xn.min()), float(xn.max()),
                                      float(zn.min()), float(zn.max())))
        for iz in range(nz):
            for ix in range(nx):
                f.write(struct.pack("<f", float(rel[iz, ix])))
    print(f"wrote {q21f.name}  {nx}x{nz}  rel {rel.min():.2f}..0 dB")

    # --- Polar arcs at catalogue distances ---------------------------------
    angles = np.arange(0, 360, dtype=float)
    body_extra = []
    for dist in DISTANCES:
        qx = dist * np.sin(np.deg2rad(angles))
        qz = dist * np.cos(np.deg2rad(angles))
        vals = griddata((xs, zs), ss, (qx, qz), method="linear")
        miss = np.isnan(vals)
        if miss.any():
            nn = NearestNDInterpolator(np.column_stack([xs, zs]), ss)
            vals[miss] = nn(qx[miss], qz[miss])
        tag = {0.5: "0p5m", 1.0: "1p0m", 2.0: "2p0m"}[dist]
        fname = f"Q21S_{HZ}Hz_{tag}.csv"
        path = OUT / fname
        with path.open("w") as f:
            f.write("degree,dBSPL\n")
            for a, s in zip(angles, vals):
                f.write(f"{a:.1f},{s:.6f}\n")
        body_extra.append(f"Q21S,{HZ},{dist:.1f},{fname},{XLSX.name},360")
        print(f"  {fname} on={vals[0]:.1f} rear={vals[180]:.1f} dB")

    # Update manifest: drop 50 Hz Q21S rows, upsert 52 Hz rows
    man = OUT / "manifest.csv"
    lines = []
    if man.exists():
        for line in man.read_text().splitlines():
            if not line.strip():
                continue
            if line.startswith("set,"):
                lines.append(line)
                continue
            parts = line.split(",")
            if len(parts) >= 2 and parts[0] == "Q21S" and parts[1] in ("50", "52"):
                continue
            lines.append(line)
    else:
        lines = ["set,freq_hz,distance_m,file,source_xlsx,n_points"]

    # Keep header first
    if not lines or not lines[0].startswith("set,"):
        lines.insert(0, "set,freq_hz,distance_m,file,source_xlsx,n_points")
    lines.extend(body_extra)
    man.write_text("\n".join(lines) + "\n")
    print("updated", man)

    # Remove obsolete 50 Hz pack CSVs if present
    for dist_tag in ("0p5m", "1p0m", "2p0m"):
        old = OUT / f"Q21S_50Hz_{dist_tag}.csv"
        if old.exists():
            old.unlink()
            print("removed", old.name)
    old_field = OUT / "Q21S_Field_50Hz.q21f"
    if old_field.exists():
        old_field.unlink()
        print("removed", old_field.name)


if __name__ == "__main__":
    main()
