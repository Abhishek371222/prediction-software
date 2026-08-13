"""Extract CLIO polar curves from GyltReadings_3july PNGs into clean .xlsx
files that the app loads for the Room measurement set.

Why: runtime PNG pixel-scanning mis-reads axis labels / grid as the curve and
draws spikes. Pre-extracted 1-degree curves match CLIO's smooth scallops.
"""
from __future__ import annotations

import glob
import math
import os
import re
import sys

import numpy as np
from PIL import Image
from openpyxl import Workbook

SRC = os.path.join(os.path.dirname(__file__), "..", "GyltReadings_3july")
SRC = os.path.abspath(SRC)
OUT = os.path.join(SRC, "curves")

CX, CY = 512.0, 395.0
R_DB_MIN, R_DB_MAX = 58.0, 278.0  # -24 dB .. +6 dB on CLIO plots


def r_to_db(r: float) -> float:
    return -24.0 + (r - R_DB_MIN) * 30.0 / (R_DB_MAX - R_DB_MIN)


def extract_rel_db(path: str) -> np.ndarray:
    arr = np.array(Image.open(path).convert("RGB"))
    h, w = arr.shape[:2]
    gray = arr.mean(axis=2)
    mask = (gray < 30) & (arr[:, :, 0] < 40)

    rs = np.full(360, np.nan)
    for deg in range(360):
        d = deg % 360
        # Vertical axis is solid black from dB *labels*   never the curve.
        if d <= 10 or d >= 350 or (170 <= d <= 190):
            continue
        rad = math.radians(deg - 90.0)
        best = None
        for r in range(255, 169, -1):
            x = int(round(CX + r * math.cos(rad)))
            y = int(round(CY + r * math.sin(rad)))
            if not (0 <= x < w and 0 <= y < h):
                continue
            x0, x1 = max(0, x - 1), min(w, x + 2)
            y0, y1 = max(0, y - 1), min(h, y + 2)
            if not mask[y0:y1, x0:x1].any():
                continue
            xo = int(round(CX + (r + 5) * math.cos(rad)))
            yo = int(round(CY + (r + 5) * math.sin(rad)))
            if 0 <= xo < w and 0 <= yo < h and gray[yo, xo] > 200:
                best = float(r)
                break
        rs[deg] = best if best is not None else np.nan

    # Fill label-axis / gaps.
    rs2 = rs.copy()
    for i in range(360):
        if not np.isnan(rs[i]):
            continue
        prev = nxt = None
        dp = dn = 0
        for k in range(1, 180):
            if not np.isnan(rs[(i - k) % 360]):
                prev = rs[(i - k) % 360]
                dp = k
                break
        for k in range(1, 180):
            if not np.isnan(rs[(i + k) % 360]):
                nxt = rs[(i + k) % 360]
                dn = k
                break
        if prev is not None and nxt is not None:
            t = dp / (dp + dn)
            rs2[i] = prev * (1.0 - t) + nxt * t
        elif prev is not None:
            rs2[i] = prev
        elif nxt is not None:
            rs2[i] = nxt
        else:
            rs2[i] = 220.0

    # Light median + smooth (keeps CLIO lobes, kills pixel jaggies).
    rs3 = rs2.copy()
    for i in range(360):
        vals = sorted(rs2[(i + k) % 360] for k in range(-3, 4))
        rs3[i] = vals[3]
    rs4 = np.array(
        [sum(rs3[(i + k) % 360] for k in range(-3, 4)) / 7.0 for i in range(360)]
    )

    dbs = np.array([r_to_db(r) for r in rs4])
    dbs -= dbs.max()  # peak = 0 dB (CLIO convention)
    return dbs


def write_xlsx(path: str, dbs: np.ndarray) -> None:
    wb = Workbook()
    ws = wb.active
    ws.append(["Degree", "dBSPL"])
    # Store as absolute-looking levels (90 + rel) so on-axis ~90 dB.
    for deg in range(360):
        ws.append([deg, float(90.0 + dbs[deg])])
    wb.save(path)


def main() -> int:
    os.makedirs(OUT, exist_ok=True)
    files = sorted(glob.glob(os.path.join(SRC, "*.png")))
    if not files:
        print("No PNGs in", SRC, file=sys.stderr)
        return 1

    for path in files:
        name = os.path.basename(path)
        m = re.search(r"(\d+)\s*Hz", name, re.I)
        if not m:
            continue
        hz = int(m.group(1))
        dbs = extract_rel_db(path)
        out = os.path.join(OUT, f"Frequency_{hz}_1Horizantal.xlsx")
        write_xlsx(out, dbs)
        print(f"{hz:3d} Hz  rel dB [{dbs.min():.2f}, {dbs.max():.2f}]  -> {out}")

    print("Done. Room curves in", OUT)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
