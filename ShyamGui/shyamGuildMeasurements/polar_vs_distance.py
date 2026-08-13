"""
SPL Radiation Pattern vs Distance - one polar plot per frequency.

Matches the ShyamGuild reference style:
  * MATLAB-like polar axes: 0 deg at right, counter-clockwise, 0..360 labels
  * linear radial axis R = 10^((SPL - SPL_ref)/20)  (pressure ratio)
  * blue  = 0.5 m (Reference),  orange = 1 m (~6 dB loss)
  * one figure per frequency, light-gray figure / white plot disc

Only the 0.5 m and 1 m measurement sets are used (the "2" m files are ignored).
"""

import os
import numpy as np
import openpyxl
import matplotlib.pyplot as plt

HERE = os.path.dirname(os.path.abspath(__file__))
FREQS = [30, 80, 200, 500]
DISTANCES = ["0.5", "1"]

# MATLAB default line colours
C_REF = "#0072BD"   # 0.5 m
C_1M = "#D95319"    # 1 m

# Per-frequency reference SPL (dB) reverse-engineered from the reference plots.
# 80 Hz has no reference plot -> fall back to energy-average of the data.
REF_SPL = {30: 82.70, 200: 95.89, 500: 96.748}


def load(freq, dist):
    fn = os.path.join(HERE, f"Frequency_{freq}_{dist}Horizantal.xlsx")
    wb = openpyxl.load_workbook(fn, data_only=True)
    ws = wb.active
    ang, spl = [], []
    for i, row in enumerate(ws.iter_rows(values_only=True)):
        if i == 0 or row[0] is None or row[1] is None:
            continue
        ang.append(float(row[0]))
        spl.append(float(row[1]))
    ang = np.array(ang)
    spl = np.array(spl)
    order = np.argsort(ang)
    return ang[order], spl[order]


def energy_avg(*spl_arrays):
    allv = np.concatenate(spl_arrays)
    return 10 * np.log10(np.mean(10 ** (allv / 10.0)))


def ref_for(freq):
    if freq in REF_SPL:
        return REF_SPL[freq]
    _, s05 = load(freq, "0.5")
    _, s1 = load(freq, "1")
    return energy_avg(s05, s1)


def style_axes(ax, rmax):
    ax.set_theta_zero_location("E")   # 0 deg to the right (MATLAB)
    ax.set_theta_direction(1)         # counter-clockwise
    ax.set_thetagrids(range(0, 360, 30))
    ax.set_facecolor("white")
    rticks = np.arange(0, rmax + 1e-9, 1.0)
    ax.set_rlim(0, rmax)
    ax.set_rticks(rticks)
    ax.set_rlabel_position(80)
    ax.grid(True, color="0.8", lw=0.8)


def plot_freq(freq, ax=None, save=True):
    ref = ref_for(freq)
    a05, s05 = load(freq, "0.5")
    a1, s1 = load(freq, "1")
    r05 = 10 ** ((s05 - ref) / 20.0)
    r1 = 10 ** ((s1 - ref) / 20.0)

    own = ax is not None
    if ax is None:
        fig = plt.figure(figsize=(8.5, 8))
        fig.patch.set_facecolor("#ececec")
        ax = fig.add_subplot(111, projection="polar")

    ax.plot(np.deg2rad(a05), r05, color=C_REF, lw=2, label="0.5 m (Reference)")
    ax.plot(np.deg2rad(a1), r1, color=C_1M, lw=2, label="1 m (~6 dB loss)")
    # on-axis (0 deg) markers like the reference data-tips
    i0 = np.argmin(np.abs(((a05 + 180) % 360) - 180))
    ax.plot(np.deg2rad(a05[i0]), r05[i0], "o", color="k", ms=4)
    ax.plot(np.deg2rad(a1[i0]), r1[i0], "o", color="k", ms=4)

    rmax = np.ceil(max(r05.max(), r1.max()))
    style_axes(ax, rmax)
    ax.set_title(f"{freq} Hz SPL Radiation Pattern vs Distance",
                 fontweight="bold", pad=22)

    if not own and save:
        ax.figure.legend(loc="lower center", ncol=1,
                         bbox_to_anchor=(0.5, -0.02), frameon=True)
        out = os.path.join(HERE, f"PolarVsDistance_{freq}Hz.png")
        ax.figure.savefig(out, dpi=130, bbox_inches="tight",
                          facecolor=ax.figure.get_facecolor())
        print(f"saved: {out}  (R_ref SPL = {ref:.3f} dB, "
              f"R@0deg 0.5m={r05[i0]:.5f} 1m={r1[i0]:.5f})")
    return ref


def main():
    for f in FREQS:
        plot_freq(f)

    fig, axes = plt.subplots(2, 2, figsize=(13, 13),
                             subplot_kw=dict(projection="polar"))
    fig.patch.set_facecolor("#ececec")
    for ax, f in zip(axes.ravel(), FREQS):
        plot_freq(f, ax=ax, save=False)
    handles, labels = axes.ravel()[0].get_legend_handles_labels()
    fig.legend(handles, labels, loc="lower center", ncol=2, frameon=True)
    fig.suptitle("SPL Radiation Pattern vs Distance (Horizontal)",
                 fontweight="bold", fontsize=15)
    out = os.path.join(HERE, "PolarVsDistance_AllFreqs.png")
    fig.savefig(out, dpi=120, bbox_inches="tight", facecolor=fig.get_facecolor())
    print(f"saved: {out}")


if __name__ == "__main__":
    main()
