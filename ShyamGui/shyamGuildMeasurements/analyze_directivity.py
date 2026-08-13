"""
Directivity analysis of horizontal speaker measurements.

Files are named  Frequency_<Hz>_<distance-m>Horizantal.xlsx  and contain a full
0..360 deg polar sweep (10 deg steps) of dB-SPL at a fixed frequency / distance.

Per user request only the 0.5 m and 1 m sets are used (the "2" m files are ignored).
For each distance the four frequencies (30, 80, 200, 500 Hz) are overlaid on a
single normalized polar plot, in the CLIO 2D-Directivity style.
"""

import os
import numpy as np
import openpyxl
import matplotlib.pyplot as plt

HERE = os.path.dirname(os.path.abspath(__file__))

FREQS = [30, 80, 200, 500]          # Hz
DISTANCES = ["0.5", "1"]            # metres (the "2" set is deliberately skipped)

# CLIO-like colour per frequency
COLORS = {30: "red", 80: "0.6", 200: "green", 500: "darkgoldenrod"}


def load(freq, dist):
    fn = os.path.join(HERE, f"Frequency_{freq}_{dist}Horizantal.xlsx")
    wb = openpyxl.load_workbook(fn, data_only=True)
    ws = wb.active
    ang, spl = [], []
    for i, row in enumerate(ws.iter_rows(values_only=True)):
        if i == 0:
            continue
        a, s = row[0], row[1]
        if a is None or s is None:
            continue
        ang.append(float(a))
        spl.append(float(s))
    ang = np.array(ang)
    spl = np.array(spl)
    order = np.argsort(ang)
    return ang[order], spl[order]


def directivity_stats(ang, spl):
    """Return useful directivity descriptors from a normalized sweep."""
    spl_n = spl - spl.max()
    # wrap angles to -180..180 for front/back reasoning
    a = ((ang + 180) % 360) - 180
    on_axis = spl_n[np.argmin(np.abs(a))]              # value nearest 0 deg

    # front (|angle|<=90) average vs rear average
    front = spl_n[np.abs(a) <= 90]
    rear = spl_n[np.abs(a) > 90]
    fb_ratio = front.mean() - rear.mean()

    # -6 dB beamwidth (front lobe): widest contiguous span around 0 deg >= -6 dB
    idx = np.argsort(a)
    a_s, s_s = a[idx], spl_n[idx]
    zero = np.argmin(np.abs(a_s))
    lo = a_s[zero]
    for k in range(zero, -1, -1):
        if s_s[k] >= -6:
            lo = a_s[k]
        else:
            break
    hi = a_s[zero]
    for k in range(zero, len(a_s)):
        if s_s[k] >= -6:
            hi = a_s[k]
        else:
            break
    beamwidth = hi - lo

    # Directivity Index estimate from horizontal sweep (2-D approximation)
    p2 = 10 ** (spl_n / 10.0)
    di = -10 * np.log10(p2.mean())     # relative to omnidirectional, dB
    return dict(on_axis=on_axis, fb_ratio=fb_ratio, beamwidth=beamwidth,
                di=di, min_db=spl_n.min())


def make_polar(ax, freq, dist):
    ang, spl = load(freq, dist)
    spl_n = spl - spl.max()
    theta = np.deg2rad(ang)
    ax.plot(theta, spl_n, color=COLORS[freq], lw=2, label=f"{freq} Hz")
    return spl_n


def style_axes(ax, title):
    ax.set_theta_zero_location("N")     # 0 deg at top  (CLIO style)
    ax.set_theta_direction(-1)          # clockwise
    ax.set_rlim(-30, 0)
    ax.set_rticks([-24, -18, -12, -6, 0])
    ax.set_rlabel_position(90)
    ax.set_thetagrids(range(0, 360, 30),
                      labels=["0", "30", "60", "90", "120", "150",
                              "180", "-150", "-120", "-90", "-60", "-30"])
    ax.grid(True, which="both", color="0.6", lw=0.6)
    ax.set_title(title, pad=18, color="navy")


def main():
    # ---- text report ----
    print("=" * 74)
    print("HORIZONTAL DIRECTIVITY ANALYSIS  (0.5 m and 1 m sets, normalized)")
    print("=" * 74)
    header = f"{'dist':>5} {'freq':>6} {'on-axis':>8} {'-6dB BW':>9} {'F/B':>7} {'DI~':>6} {'min':>7}"
    for dist in DISTANCES:
        print(f"\n--- {dist} m ---")
        print(header)
        for freq in FREQS:
            ang, spl = load(freq, dist)
            st = directivity_stats(ang, spl)
            print(f"{dist:>5} {freq:>5}H {st['on_axis']:>8.1f} "
                  f"{st['beamwidth']:>7.0f}\u00b0 {st['fb_ratio']:>6.1f} "
                  f"{st['di']:>6.1f} {st['min_db']:>7.1f}")

    # ---- plots ----
    for dist in DISTANCES:
        fig = plt.figure(figsize=(8, 8))
        ax = fig.add_subplot(111, projection="polar")
        for freq in FREQS:
            make_polar(ax, freq, dist)
        style_axes(ax, f"2D Directivity Analysis - Horizontal @ {dist} m")
        ax.legend(loc="upper left", bbox_to_anchor=(-0.12, 1.12), frameon=False)
        fig.text(0.5, 0.04, "dB (normalized to on-axis peak)",
                 ha="center", color="navy")
        out = os.path.join(HERE, f"Directivity_Horizontal_{dist}m.png")
        fig.savefig(out, dpi=130, bbox_inches="tight")
        print(f"\nsaved: {out}")

    # ---- combined 0.5 vs 1 m per frequency (overlay distances) ----
    fig, axes = plt.subplots(2, 2, figsize=(12, 12),
                             subplot_kw=dict(projection="polar"))
    dist_color = {"0.5": "tab:blue", "1": "tab:red"}
    for ax, freq in zip(axes.ravel(), FREQS):
        for dist in DISTANCES:
            ang, spl = load(freq, dist)
            spl_n = spl - spl.max()
            ax.plot(np.deg2rad(ang), spl_n, color=dist_color[dist],
                    lw=2, label=f"{dist} m")
        style_axes(ax, f"{freq} Hz")
        ax.legend(loc="upper right", bbox_to_anchor=(1.15, 1.12), frameon=False)
    fig.suptitle("Horizontal Directivity - distance comparison (0.5 m vs 1 m)",
                 color="navy", fontsize=14)
    out = os.path.join(HERE, "Directivity_DistanceComparison.png")
    fig.savefig(out, dpi=120, bbox_inches="tight")
    print(f"saved: {out}")


if __name__ == "__main__":
    main()
