#!/usr/bin/env python3
"""Export 2-inch HF horn native BEM bands -> polar CSVs + Q21F-style field.

Mirror of docs/q21s_bem_plots/export_q21s_native_hz_pack.py, retargeted at the
2" HF horn raw BEM workbooks. Writes into the SAME
MeasurementIntegrationPack/Data/ folder as Q21S, but every output file is
prefixed "BEM2inch_" (vs "Q21S_") so the two measurement sets never collide or
mix -- see MeasurementData::packSetName() in the app.

Two things differ from the Q21S / 15W750 workbooks and are handled here:

1. COLUMN LAYOUT. Q21S and 15W750 export 7 columns
   (Index, X1, X2, X3, Freq, re, im) so pressure sits at index 5/6. The 2-inch
   export has 6 (Param, x, y, z, re_<f>, im_<f>) with pressure at index 4/5 and
   the frequency encoded in the header instead of a column.

2. HALF-PLANE DOMAIN. Q21S and 15W750 span z = -5..+5 m with the source at the
   CENTRE, so a full 360 deg polar can be sampled straight off the mesh. The
   2-inch export spans z = 0..+10 m with the source at the EDGE -- there is no
   data behind the horn at all. Sampling a circle naively would hand the whole
   rear hemisphere to nearest-neighbour extrapolation, i.e. invented radiation.
   Instead the rear is synthesised explicitly by REAR POLICY below.

Default bands: 64, 135, 243, 507, 1057, 1904, 3971, 8280, 17266 Hz -- the
native BEM_Data_2inch_10m/<Hz>*.xlsx bands actually supplied.
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
BEM = ROOT / "BEM_Data_2inch_10m"
OUT = ROOT / "ShyamGui/prediction software/MeasurementIntegrationPack/Data"
SET_NAME = "BEM2inch"
P_REF = 20e-6
DISTANCES = [0.5, 1.0, 2.0]
DEFAULT_HZ = [64, 135, 243, 507, 1057, 1904, 3971, 8280, 17266]

# The BEM run is unit-drive normalised: on-axis at 1 m lands around 46 dB,
# where Q21S measures ~123 dB and 15W750 ~105 dB. Until a real sensitivity
# figure (dB SPL @ 1 W / 1 m) is supplied for this horn, absolute levels from
# this set are NOT comparable with Q21S in a mixed scene. Set this once that
# number is known and re-run; every exported SPL shifts by it.
SENSITIVITY_OFFSET_DB = 0.0  # keep 0: calibration now lives in MeasurementData.h

# --- REAR POLICY ----------------------------------------------------------
# There is no measured data for |theta| > 90 deg. Rather than let the circle
# sampler extrapolate, the rear is built from the measured grazing value:
# taper linearly (in dB) from the real +/-90 deg level down to a rear target
# at 180 deg. The target is min(grazing, -REAR_FLOOR_DB) relative to on-axis,
# so the taper is always downward -- at HF the grazing level is already far
# below the floor and the rear stays flat at that lower value rather than
# being lifted up to it.
REAR_FLOOR_DB = 25.0


def spl_db(pr: float, pi: float) -> float:
    m = math.hypot(pr, pi)
    if m <= 0:
        return -200.0
    return 20.0 * math.log10(m / P_REF) + SENSITIVITY_OFFSET_DB


def find_xlsx(hz: int) -> Path:
    """The supplied files are named inconsistently (64Hz.xlsx, '135 Hz.xlsx',
    17266.xlsx), so match on the leading band number rather than a template."""
    for p in sorted(BEM.glob("*.xlsx")):
        head = p.stem.split("Hz")[0].strip()
        try:
            if int(float(head)) == hz:
                return p
        except ValueError:
            continue
    raise SystemExit(f"no workbook for {hz} Hz under {BEM}")


def load_plane(xlsx: Path) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    wb = openpyxl.load_workbook(xlsx, data_only=True, read_only=True)
    ws = wb.active
    xs, zs, ss = [], [], []
    for row in ws.iter_rows(min_row=2, values_only=True):
        if not row or row[1] is None:
            continue
        try:
            x, z = float(row[1]), float(row[3])
            pr, pi = float(row[4]), float(row[5])
        except Exception:
            continue
        xs.append(x)
        zs.append(z)
        ss.append(spl_db(pr, pi))
    wb.close()
    return np.asarray(xs, float), np.asarray(zs, float), np.asarray(ss, float)


def front_polar(xs, zs, ss, dist: float) -> np.ndarray:
    """360-entry dB SPL polar, front half measured and rear half synthesised.

    Angle convention matches the Q21S exporter: 0 deg = on-axis (+z),
    increasing clockwise via +x, so 90 deg is grazing and 180 deg is behind."""
    ang = np.arange(0, 360, dtype=float)
    out = np.full(360, np.nan)

    # --- measured front hemisphere (|theta| <= 90) ---
    front = (ang <= 90.0) | (ang >= 270.0)
    fa = np.deg2rad(ang[front])
    qx, qz = dist * np.sin(fa), dist * np.cos(fa)
    vals = griddata((xs, zs), ss, (qx, qz), method="linear")
    miss = np.isnan(vals)
    if miss.any():
        nn = NearestNDInterpolator(np.column_stack([xs, zs]), ss)
        vals[miss] = nn(qx[miss], qz[miss])
    out[front] = vals

    # --- synthesised rear hemisphere ---
    on_axis = out[0]
    graze_r = out[90]    # +90 deg, +x side
    graze_l = out[270]   # -90 deg, -x side
    for side_graze, lo, hi in ((graze_r, 90.0, 180.0), (graze_l, 270.0, 180.0)):
        target = min(side_graze, on_axis - REAR_FLOOR_DB)
        step = 1.0 if hi > lo else -1.0
        for a in np.arange(lo + step, hi + step / 2.0, step):
            t = abs(a - lo) / abs(hi - lo)          # 0 at grazing, 1 at rear
            out[int(a) % 360] = side_graze + t * (target - side_graze)
    return out


def export_one(hz: int) -> list[str]:
    xlsx = find_xlsx(hz)
    OUT.mkdir(parents=True, exist_ok=True)

    xs, zs, ss = load_plane(xlsx)
    print(f"[{hz} Hz] loaded {len(xs)} pts  SPL {ss.min():.2f}..{ss.max():.2f} dB  <- {xlsx.name}")
    if zs.min() < -0.5:
        print("  NOTE: workbook spans z < 0 -- rear is measured, policy not applied")

    # --- absolute field, same Heatmap.m recipe as Q21S ---
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
    field = OUT / f"{SET_NAME}_Field_{hz}Hz.q21f"
    with field.open("wb") as f:
        f.write(b"Q21F")
        f.write(struct.pack("<iiii", 1, hz, nx, nz))
        f.write(struct.pack("<ffff", float(xn.min()), float(xn.max()),
                                     float(zn.min()), float(zn.max())))
        for iz in range(nz):
            for ix in range(nx):
                f.write(struct.pack("<f", float(rel[iz, ix])))
    print(f"  wrote {field.name}  {nx}x{nz}  rel {rel.min():.2f}..0 dB")

    # --- polar CSVs ---
    body: list[str] = []
    angles = np.arange(0, 360, dtype=float)
    for dist in DISTANCES:
        vals = front_polar(xs, zs, ss, dist)
        tag = {0.5: "0p5m", 1.0: "1p0m", 2.0: "2p0m"}[dist]
        fname = f"{SET_NAME}_{hz}Hz_{tag}.csv"
        with (OUT / fname).open("w") as f:
            f.write("degree,dBSPL\n")
            for a, s in zip(angles, vals):
                f.write(f"{a:.1f},{s:.6f}\n")
        body.append(f"{SET_NAME},{hz},{dist:.1f},{fname},{xlsx.name},360")
        print(f"  {fname} on={vals[0]:.1f} graze={vals[90]:.1f} "
              f"rear={vals[180]:.1f}  F/B={vals[0]-vals[180]:.1f} dB")
    return body


def update_manifest(body_rows: list[str], hz_list: list[int]) -> None:
    man = OUT / "manifest.csv"
    drop = {str(h) for h in hz_list}
    lines: list[str] = []
    if man.exists():
        for line in man.read_text().splitlines():
            if not line.strip():
                continue
            if line.startswith("set,"):
                lines.append(line)
                continue
            parts = line.split(",")
            if len(parts) >= 2 and parts[0] == SET_NAME and parts[1] in drop:
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
                    help=f"BEM bands to export (default: {DEFAULT_HZ})")
    args = ap.parse_args()
    body: list[str] = []
    for hz in args.hz:
        body.extend(export_one(hz))
    update_manifest(body, list(args.hz))


if __name__ == "__main__":
    main()
