#!/usr/bin/env python3
"""Export 15W750 native BEM bands -> polar CSVs + Q21F-style field (Heatmap.m recipe).

Mirror of docs/q21s_bem_plots/export_q21s_native_hz_pack.py, retargeted at the
15W750 (15" sealed) raw BEM workbooks. Writes into the SAME
MeasurementIntegrationPack/Data/ folder as Q21S, but every output file is
prefixed "15W750_" (vs "Q21S_") so the two measurement sets never collide or
mix -- see MeasurementData::packSetName() in the app.

Default bands: 64, 135, 243, 507, 1057, 1904, 3971, 8280, 17200 Hz -- the
native BEM_Data_15W750_10m/<Hz>Hz.xlsx bands actually supplied.
"""
from __future__ import annotations

import argparse
import math
import struct
from pathlib import Path

import numpy as np
import openpyxl
from scipy.interpolate import NearestNDInterpolator, griddata

ROOT = Path(__file__).resolve().parents[2]
BEM = ROOT / "BEM_Data_15W750_10m"
OUT = ROOT / "ShyamGui/prediction software/MeasurementIntegrationPack/Data"
SET_NAME = "15W750"
P_REF = 20e-6
DISTANCES = [0.5, 1.0, 2.0]
DEFAULT_HZ = [64, 135, 243, 507, 1057, 1904, 3971, 8280, 17200]


def spl_db(pr: float, pi: float) -> float:
    m = math.hypot(pr, pi)
    return -200.0 if m <= 0 else 20.0 * math.log10(m / P_REF)


def export_one(hz: int) -> list[str]:
    xlsx = BEM / f"{hz}Hz.xlsx"
    if not xlsx.exists():
        raise SystemExit(f"missing {xlsx}")
    OUT.mkdir(parents=True, exist_ok=True)

    wb = openpyxl.load_workbook(xlsx, data_only=True, read_only=True)
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
    print(f"[{hz} Hz] loaded {len(xs)} pts  SPL {ss.min():.2f}..{ss.max():.2f} dB  <- {xlsx.name}")

    xn = np.unique(xs)
    zn = np.unique(zs)
    Xm, Zm = np.meshgrid(xn, zn)
    SPLm = griddata((xs, zs), ss, (Xm, Zm), method="linear")
    miss = np.isnan(SPLm)
    if miss.any():
        nn = NearestNDInterpolator(np.column_stack([xs, zs]), ss)
        SPLm[miss] = nn(Xm[miss], Zm[miss])

    rel = SPLm - np.nanmax(SPLm)
    nz, nx = rel.shape
    q21f = OUT / f"{SET_NAME}_Field_{hz}Hz.q21f"
    with q21f.open("wb") as f:
        f.write(b"Q21F")
        f.write(struct.pack("<iiii", 1, hz, nx, nz))
        f.write(struct.pack("<ffff", float(xn.min()), float(xn.max()),
                                      float(zn.min()), float(zn.max())))
        for iz in range(nz):
            for ix in range(nx):
                f.write(struct.pack("<f", float(rel[iz, ix])))
    print(f"  wrote {q21f.name}  {nx}x{nz}  rel {rel.min():.2f}..0 dB")

    body: list[str] = []
    angles = np.arange(0, 360, dtype=float)
    for dist in DISTANCES:
        qx = dist * np.sin(np.deg2rad(angles))
        qz = dist * np.cos(np.deg2rad(angles))
        vals = griddata((xs, zs), ss, (qx, qz), method="linear")
        miss = np.isnan(vals)
        if miss.any():
            nn = NearestNDInterpolator(np.column_stack([xs, zs]), ss)
            vals[miss] = nn(qx[miss], qz[miss])
        tag = {0.5: "0p5m", 1.0: "1p0m", 2.0: "2p0m"}[dist]
        fname = f"{SET_NAME}_{hz}Hz_{tag}.csv"
        path = OUT / fname
        with path.open("w") as f:
            f.write("degree,dBSPL\n")
            for a, s in zip(angles, vals):
                f.write(f"{a:.1f},{s:.6f}\n")
        body.append(f"{SET_NAME},{hz},{dist:.1f},{fname},{xlsx.name},360")
        print(f"  {fname} on={vals[0]:.1f} rear={vals[180]:.1f}  F/B={vals[0]-vals[180]:.1f} dB")
    return body


def update_manifest(hz_list: list[int], body_rows: list[str]) -> None:
    man = OUT / "manifest.csv"
    lines: list[str] = []
    if man.exists():
        for line in man.read_text().splitlines():
            if not line.strip():
                continue
            if line.startswith("set,"):
                lines.append(line)
                continue
            parts = line.split(",")
            # Drop any stale 15W750 rows for the bands we're about to rewrite.
            if len(parts) >= 2 and parts[0] == SET_NAME and int(parts[1]) in set(hz_list):
                continue
            lines.append(line)
    else:
        lines = ["set,freq_hz,distance_m,file,source_xlsx,n_points"]

    if not lines or not lines[0].startswith("set,"):
        lines.insert(0, "set,freq_hz,distance_m,file,source_xlsx,n_points")
    lines.extend(body_rows)
    man.write_text("\n".join(lines) + "\n")
    print("updated", man)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("hz", nargs="*", type=int, default=DEFAULT_HZ,
                    help="Native 15W750 BEM bands to export")
    args = ap.parse_args()
    body: list[str] = []
    for hz in args.hz:
        body.extend(export_one(hz))
    update_manifest(list(args.hz), body)


if __name__ == "__main__":
    main()
