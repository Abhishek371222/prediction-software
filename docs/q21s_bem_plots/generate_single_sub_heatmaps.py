#!/usr/bin/env python3
"""Generate single-sub Q21S SPL heatmaps matching AcousticEngine (BEM polar × 1/r).

Same physics as the app for one cabinet:
  D(θ) = 10^((SPL_BEM(θ) − SPL_BEM(0°)) / 20)   at R_ref = 2 m
  amp  = D / r_spread
  I    = amp²
  SPL  = SPL_onaxis(2 m) + 10·log10(I / (1/R_ref²))

Writes PNGs under docs/single_sub_heatmap/figures/ for the native BEM catalogue.
"""
from __future__ import annotations

from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np

ROOT = Path(__file__).resolve().parents[2]
PACK = ROOT / "ShyamGui/prediction software/MeasurementIntegrationPack/Data"
OUT = ROOT / "docs/single_sub_heatmap/figures"
NATIVE_HZ = [20, 29, 52, 81, 98, 153, 198, 256, 309, 352, 400, 401]

WORLD = 100.0
N = 401                  # 0.25 m cells
R_REF = 2.0
CAB_HALF = 0.917 * 0.5   # Q21S depth / 2
DB_FLOOR = -36.0
CX = CY = 50.0


def load_polar(hz: int) -> tuple[np.ndarray, float]:
    path = PACK / f"Q21S_{hz}Hz_2p0m.csv"
    deg, spl = [], []
    with path.open() as f:
        next(f)
        for line in f:
            a, s = line.strip().split(",")
            deg.append(float(a))
            spl.append(float(s))
    deg = np.asarray(deg)
    spl = np.asarray(spl)
    # Integer-degree table, on-axis ≈ 0°
    on_axis = float(spl[np.argmin(np.abs(((deg + 180) % 360) - 180))])
    gain = np.ones(360, dtype=float)
    for d, s in zip(deg, spl):
        i = int(round(d)) % 360
        gain[i] = 10.0 ** ((s - on_axis) / 20.0)
    # L/R symmetrise + light smooth (matches sanitizeDirectivityGain intent)
    for i in range(1, 180):
        a = 0.5 * (gain[i] + gain[360 - i])
        gain[i] = a
        gain[360 - i] = a
    gain = np.clip(gain / max(gain[0], 1e-6), 0.0, 2.0)
    return gain, on_axis


def dir_factor(gain: np.ndarray, theta_rad: np.ndarray) -> np.ndarray:
    # Facing +X (0 rad). Engine averages ±0.5°; skip for raster speed.
    deg = np.mod(np.degrees(theta_rad), 360.0)
    d0 = np.floor(deg).astype(int) % 360
    d1 = (d0 + 1) % 360
    frac = deg - np.floor(deg)
    return (1.0 - frac) * gain[d0] + frac * gain[d1]


def simulate(gain: np.ndarray, on_axis: float) -> tuple[np.ndarray, np.ndarray, float]:
    xs = np.linspace(0.0, WORLD, N)
    ys = np.linspace(0.0, WORLD, N)
    X, Y = np.meshgrid(xs, ys)
    dx = X - CX
    dy = Y - CY
    r = np.hypot(dx, dy)
    r_spread = np.maximum(r, CAB_HALF)
    theta = np.arctan2(dy, dx)
    D = dir_factor(gain, theta)
    amp = D / r_spread
    I = amp * amp
    I_ref = 1.0 / (R_REF * R_REF)
    spl_abs = on_axis + 10.0 * np.log10(np.maximum(I, 1e-300) / I_ref)
    peak = float(np.max(spl_abs))
    rel = spl_abs - peak
    rel_disp = np.maximum(rel, DB_FLOOR)
    return spl_abs, rel_disp, peak


def atomik_cmap():
    # Approximate ColourMaps::sevenColor (black→blue→cyan→green→yellow→orange→red)
    from matplotlib.colors import LinearSegmentedColormap
    colors = [
        "#000000", "#0000AA", "#00AAAA", "#00AA00",
        "#EEEE00", "#FF8800", "#CC0000",
    ]
    return LinearSegmentedColormap.from_list("atomik7", colors, N=256)


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    cmap = atomik_cmap()
    summary: list[tuple[int, float, float, float]] = []

    for hz in NATIVE_HZ:
        gain, on_axis = load_polar(hz)
        spl_abs, rel, peak = simulate(gain, on_axis)
        # Sample on-axis at 2 m and 4 m for inverse-square check
        i2 = int(round((CX + 2.0) / WORLD * (N - 1)))
        i4 = int(round((CX + 4.0) / WORLD * (N - 1)))
        j = int(round(CY / WORLD * (N - 1)))
        at2 = float(spl_abs[j, i2])
        at4 = float(spl_abs[j, i4])
        summary.append((hz, on_axis, at2, at4))

        fig, ax = plt.subplots(figsize=(7.2, 6.4), dpi=140)
        im = ax.imshow(
            rel,
            origin="lower",
            extent=[0, WORLD, 0, WORLD],
            cmap=cmap,
            vmin=DB_FLOOR,
            vmax=0.0,
            interpolation="bilinear",
        )
        ax.plot(CX, CY, "ws", ms=5, mew=0.8, mec="k")
        ax.set_aspect("equal")
        ax.set_xlabel("X (m)")
        ax.set_ylabel("Y (m)")
        ax.set_title(
            f"Q21S single-sub SPL — {hz} Hz\n"
            f"BEM 2 m polar × 1/r | peak {peak:.1f} dB SPL | floor {DB_FLOOR:.0f} dB"
        )
        cb = fig.colorbar(im, ax=ax, fraction=0.046, pad=0.04)
        cb.set_label("Rel. SPL (dB)")
        ax.set_xlim(0, WORLD)
        ax.set_ylim(0, WORLD)
        out = OUT / f"Q21S_single_sub_{hz}Hz.png"
        fig.tight_layout()
        fig.savefig(out)
        plt.close(fig)
        print(f"wrote {out.name}  on-axis BEM={on_axis:.2f}  peak={peak:.2f}  "
              f"@2m={at2:.2f}  @4m={at4:.2f}  drop={at2-at4:.2f}")

    # Summary strip
    fig, axes = plt.subplots(3, 4, figsize=(14, 10), dpi=120)
    for ax, hz in zip(axes.ravel(), NATIVE_HZ):
        gain, on_axis = load_polar(hz)
        _, rel, peak = simulate(gain, on_axis)
        ax.imshow(rel, origin="lower", extent=[0, WORLD, 0, WORLD],
                  cmap=cmap, vmin=DB_FLOOR, vmax=0.0, interpolation="bilinear")
        ax.plot(CX, CY, "ws", ms=3, mew=0.5, mec="k")
        ax.set_title(f"{hz} Hz  peak {peak:.0f} dB", fontsize=9)
        ax.set_xticks([])
        ax.set_yticks([])
        ax.set_aspect("equal")
    fig.suptitle("Q21S single-sub heatmaps — native BEM bands (100×100 m)", fontsize=13)
    fig.tight_layout()
    strip = OUT / "Q21S_single_sub_all_freqs.png"
    fig.savefig(strip)
    plt.close(fig)
    print("wrote", strip.name)

    sum_path = OUT / "summary.csv"
    with sum_path.open("w") as f:
        f.write("hz,bem_onaxis_2m_dB,sim_at_2m_dB,sim_at_4m_dB,drop_2_to_4_dB\n")
        for hz, on_axis, at2, at4 in summary:
            f.write(f"{hz},{on_axis:.4f},{at2:.4f},{at4:.4f},{at2-at4:.4f}\n")
    print("wrote", sum_path.name)


if __name__ == "__main__":
    main()
