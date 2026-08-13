"""

CLIO-style 2D Directivity plots for the ShyamGuild horizontal measurements.



This reproduces the look + methodology of the reference 2d_*_clio.png files so the

output can be visually verified against them:



  * normalized to 0 dB on-axis peak (each frequency curve scaled to its own max)

  * radial axis in dB: outer ring +6, then 0, -6, -12, -18, -24 (center ~ -30)

  * 0 deg at top, clockwise, angles labelled 0..+-180 (negative on the left)

  * solid 30 deg spokes + dashed 15 deg spokes, solid 6 dB rings + dashed 3 dB rings

  * ATOMIK / title / date header and a per-frequency colour legend

  * one plot per distance, all frequencies overlaid (CLIO overlays frequencies)



Only the 0.5 m and 1 m sets are used; the "2" m files are ignored.

"""



import os

import datetime

import numpy as np

import openpyxl

import matplotlib.pyplot as plt

import matplotlib.image as mpimg



HERE = os.path.dirname(os.path.abspath(__file__))

FREQS = [30, 80, 200, 500]

DISTANCES = ["0.5", "1"]



# colours echoing the CLIO legend (red / grey / green / yellow / black)

COLORS = {30: "red", 80: "0.6", 200: "green", 500: "#d4c200"}



R_MIN, R_MAX = -30, 6

SOLID_RINGS = [6, 0, -6, -12, -18, -24]

DASH_RINGS = [3, -3, -9, -15, -21, -27]





def load(freq, dist):

    fn = os.path.join(HERE, f"Frequency_{freq}_{dist}Horizantal.xlsx")

    ws = openpyxl.load_workbook(fn, data_only=True).active

    ang, spl = [], []

    for i, row in enumerate(ws.iter_rows(values_only=True)):

        if i == 0 or row[0] is None or row[1] is None:

            continue

        ang.append(float(row[0]))

        spl.append(float(row[1]))

    ang, spl = np.array(ang), np.array(spl)

    o = np.argsort(ang)

    return ang[o], spl[o]





def draw_grid(ax):

    ax.grid(False)

    ax.set_yticklabels([])

    ax.set_ylim(R_MIN, R_MAX)

    th = np.linspace(0, 2 * np.pi, 400)



    for r in SOLID_RINGS:

        ax.plot(th, np.full_like(th, r), color="k", lw=0.7, zorder=1)

    for r in DASH_RINGS:

        ax.plot(th, np.full_like(th, r), color="k", lw=0.5, ls=(0, (3, 3)),

                zorder=1)

    # spokes

    for a in range(0, 360, 30):

        ax.plot([np.deg2rad(a)] * 2, [R_MIN, R_MAX], color="k", lw=0.7, zorder=1)

    for a in range(15, 360, 30):

        ax.plot([np.deg2rad(a)] * 2, [R_MIN, R_MAX], color="k", lw=0.5,

                ls=(0, (3, 3)), zorder=1)



    # radial dB labels along the top spoke (theta = 0 after N/clockwise mapping)

    for r in SOLID_RINGS:

        ax.text(np.deg2rad(2), r, f"{r}", ha="left", va="center",

                fontsize=8, zorder=5)

    ax.text(np.deg2rad(0), R_MAX + 2.2, "dB", ha="center", va="bottom",

            fontsize=8)



    # angle labels

    labels = ["0", "30", "60", "90", "120", "150", "180",

              "-150", "-120", "-90", "-60", "-30"]

    ax.set_xticks(np.deg2rad(np.arange(0, 360, 30)))

    ax.set_xticklabels([f"{l}\u00b0" for l in labels], fontsize=9)

    ax.tick_params(pad=6)





def clio_axes(ax, center_label="ShyamGuild"):

    ax.set_theta_zero_location("N")

    ax.set_theta_direction(-1)

    ax.set_facecolor("white")

    for s in ax.spines.values():

        s.set_visible(False)

    draw_grid(ax)

    ax.text(0.5, 0.5, center_label, transform=ax.transAxes, ha="center",

            va="center", fontweight="bold", fontsize=12, zorder=6)





def header(fig, ax, plane, dist):

    fig.text(0.07, 0.95, "ATOMIK", color="navy", fontsize=11, fontweight="bold")

    ax.set_title("2D Directivity Analysis", color="navy", fontsize=12, pad=26)

    stamp = datetime.datetime.now().strftime("%d-%m-%Y %H.%M.%S")

    fig.text(0.93, 0.95, stamp, color="k", fontsize=9, ha="right")

    fig.text(0.93, 0.92, f"{plane} @ {dist} m", color="k", fontsize=9, ha="right")

    # frequency colour legend (top-left)

    y = 0.90

    for f in FREQS:

        fig.text(0.07, y, f"{f}Hz", color=COLORS[f], fontsize=10,

                 fontweight="bold")

        y -= 0.028





def plot_distance(dist, plane="Horizontal", save=True):

    fig = plt.figure(figsize=(10, 7.5))

    fig.patch.set_facecolor("white")

    ax = fig.add_axes([0.18, 0.05, 0.64, 0.85], projection="polar")

    for f in FREQS:

        ang, spl = load(f, dist)

        i0 = np.argmin(np.abs(((ang + 180) % 360) - 180))   # index of 0 deg

        spl_n = spl - spl[i0]                    # CLIO: normalize to on-axis (0 deg)

        ax.plot(np.deg2rad(ang), spl_n, color=COLORS[f], lw=1.3, zorder=3)

    clio_axes(ax)

    header(fig, ax, plane, dist)

    if save:

        out = os.path.join(HERE, f"ClioStyle_{plane}_{dist}m.png")

        fig.savefig(out, dpi=140, facecolor="white")

        print(f"saved: {out}")

    return fig





def verification_sheet():

    """Side-by-side: reference CLIO image vs our 0.5 m CLIO-style plot."""

    ref_path = os.path.join(HERE, "2d_HorizontalPlot_clio.png")

    our_path = os.path.join(HERE, "ClioStyle_Horizontal_0.5m.png")

    if not (os.path.exists(ref_path) and os.path.exists(our_path)):

        return

    fig, axs = plt.subplots(1, 2, figsize=(18, 7))

    for a, p, t in zip(axs, [ref_path, our_path],

                       ["CLIO reference (template)", "Ours - 0.5 m"]):

        a.imshow(mpimg.imread(p))

        a.set_title(t, fontsize=13)

        a.axis("off")

    out = os.path.join(HERE, "CLIO_verification_sidebyside.png")

    fig.savefig(out, dpi=110, bbox_inches="tight")

    print(f"saved: {out}")





def main():

    for d in DISTANCES:

        plot_distance(d)

    verification_sheet()





if __name__ == "__main__":

    main()

