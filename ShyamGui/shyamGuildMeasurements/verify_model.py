"""
Verification of the radiation-pattern model used by the prediction software.

1) Audit the MATLAB piston model: piston directivity D = 2 J1(ka sin th)/(ka sin th)
   with a sigmoid rear taper.
2) Compare that MODEL directivity against the MEASURED directivity (the .xlsx
   readings) at 30/80/200/500 Hz.
3) Predict 1/2/3-device interference heat maps at 200 Hz using BOTH the piston
   model and the measured directivity, so the app's heat maps can be checked.

Conventions match the app: 0 deg = on-axis (+x), sources face +x, 2D horizontal
plane, complex-pressure summation, normalize to max, dB.
"""

import os
import numpy as np
import openpyxl
import matplotlib.pyplot as plt
from scipy.special import j1, struve

HERE = os.path.dirname(os.path.abspath(__file__))

C0 = 343.0
A  = 0.13          # piston radius (m) from the MATLAB script
FREQS = [30, 80, 200, 500]


# ---------------------------------------------------------------------------
# Measured directivity (normalized to on-axis), from the 0.5 m readings.
# ---------------------------------------------------------------------------
def load_measured(hz):
    fn = os.path.join(HERE, f"Frequency_{hz}_0.5Horizantal.xlsx")
    ws = openpyxl.load_workbook(fn, data_only=True).active
    deg, spl = [], []
    for i, row in enumerate(ws.iter_rows(values_only=True)):
        if i == 0 or row[0] is None or row[1] is None:
            continue
        deg.append(float(row[0]))
        spl.append(float(row[1]))
    deg = np.array(deg); spl = np.array(spl)
    o = np.argsort(deg); deg, spl = deg[o], spl[o]
    on_axis = spl[np.argmin(np.abs(((deg + 180) % 360) - 180))]
    return deg, spl - on_axis   # dB relative to on-axis


# ---------------------------------------------------------------------------
# Piston + rear-taper model directivity (linear), angle phi from +x axis (rad).
# ---------------------------------------------------------------------------
def piston_directivity(phi, hz):
    k = 2 * np.pi * hz / C0
    ka = k * A
    x = ka * np.sin(phi)
    with np.errstate(divide="ignore", invalid="ignore"):
        D = 2 * j1(x) / x
    D = np.where(np.abs(x) < 1e-10, 1.0, D)
    wrapped = (phi + np.pi) % (2 * np.pi) - np.pi      # -> [-pi, pi]
    sigma = max(1.0, ka)
    taper = 1.0 / (1.0 + np.exp(sigma * (np.abs(wrapped) - np.pi / 2)))
    return np.abs(D) * taper


def radiation_impedance_check(hz):
    """Return (R1, X1_script, X1_correct) to audit FIX 1."""
    k = 2 * np.pi * hz / C0
    ka = k * A
    x = 2 * ka
    R1 = 1 - 2 * j1(x) / x if abs(x) > 1e-10 else 0.0
    H1 = struve(1, x)
    X1_script  = (2 / np.pi) * (H1 / x)   # the script's FIX 1
    X1_correct = 2 * H1 / x               # textbook  X1 = 2 H1(2ka)/(2ka)
    return R1, X1_script, X1_correct


# ---------------------------------------------------------------------------
# Field computation (shared physics: complex sum of D * exp(-jkr)/r).
# ---------------------------------------------------------------------------
def measured_interp_factory(hz):
    deg, dB = load_measured(hz)
    lin = 10 ** (dB / 20.0)              # linear, on-axis = 1
    # ensure wrap coverage 0..360
    def f(phi):
        a = np.degrees(phi) % 360.0
        return np.interp(a, deg, lin, period=360.0)
    return f


def field(sources, hz, dirfun, x, y):
    X, Y = np.meshgrid(x, y)
    k = 2 * np.pi * hz / C0
    P = np.zeros_like(X, dtype=complex)
    for (sx, sy) in sources:
        r = np.sqrt((X - sx) ** 2 + (Y - sy) ** 2)
        r = np.maximum(r, 0.05)
        phi = np.arctan2(Y - sy, X - sx)        # facing +x => relative angle = phi
        D = dirfun(phi, hz) if dirfun.__code__.co_argcount == 2 else dirfun(phi)
        P += D * np.exp(-1j * k * r) / r
    I = np.abs(P) ** 2
    I = I / I.max()
    IdB = 10 * np.log10(np.maximum(I, 1e-12))
    return X, Y, np.clip(IdB, -40, 0)


def layouts(d=1.0):
    return {
        1: [(0.0, 0.0)],
        2: [(0.0, -d / 2), (0.0, d / 2)],
        3: [(0.0, -d), (0.0, 0.0), (0.0, d)],
    }


# ---------------------------------------------------------------------------
def plot_directivity_comparison():
    fig, axes = plt.subplots(2, 2, figsize=(11, 11),
                             subplot_kw=dict(projection="polar"))
    phi = np.deg2rad(np.arange(0, 361, 2))
    for ax, hz in zip(axes.ravel(), FREQS):
        deg_m, dB_m = load_measured(hz)
        ax.plot(np.deg2rad(deg_m), np.clip(dB_m, -30, 5),
                color="#1a9eff", lw=2, label="measured (0.5 m)")
        Dp = piston_directivity(phi, hz)
        dB_p = 20 * np.log10(np.maximum(Dp, 1e-3))
        ax.plot(phi, np.clip(dB_p, -30, 5), color="#ff5050", lw=2,
                label="piston+taper model")
        ax.set_theta_zero_location("E"); ax.set_theta_direction(1)
        ax.set_rlim(-30, 5); ax.set_rticks([-24, -18, -12, -6, 0])
        ka = 2 * np.pi * hz / C0 * A
        ax.set_title(f"{hz} Hz   (ka = {ka:.3f})", pad=14)
        ax.legend(loc="upper right", bbox_to_anchor=(1.18, 1.12), fontsize=8,
                  frameon=False)
    fig.suptitle("Directivity: measured vs piston+taper model (normalized to on-axis)",
                 fontsize=14)
    out = os.path.join(HERE, "verify_directivity_model_vs_measured.png")
    fig.savefig(out, dpi=120, bbox_inches="tight"); print("saved:", out)


def plot_heatmaps(hz=200, d=1.0):
    x = np.linspace(-1, 4, 500)
    y = np.linspace(-2.5, 2.5, 500)
    lay = layouts(d)
    meas = measured_interp_factory(hz)

    fig, axes = plt.subplots(2, 3, figsize=(16, 10))
    for col, n in enumerate([1, 2, 3]):
        for row, (name, dirfun) in enumerate(
                [("piston+taper model", piston_directivity),
                 ("measured directivity", meas)]):
            X, Y, IdB = field(lay[n], hz, dirfun, x, y)
            ax = axes[row, col]
            pc = ax.contourf(X, Y, IdB, levels=50, cmap="jet")
            for (sx, sy) in lay[n]:
                ax.plot(sx, sy, "ko", mfc="w", ms=9)
            ax.set_aspect("equal"); ax.set_xlim(-1, 4); ax.set_ylim(-2.5, 2.5)
            ax.set_title(f"{n} device(s) - {name}")
            ax.set_xlabel("x (m)"); ax.set_ylabel("y (m)")
            fig.colorbar(pc, ax=ax, fraction=0.046, pad=0.04)
    fig.suptitle(f"Predicted interference heat maps @ {hz} Hz  (spacing d = {d} m)",
                 fontsize=15)
    out = os.path.join(HERE, f"verify_heatmaps_1_2_3_devices_{hz}Hz.png")
    fig.savefig(out, dpi=110, bbox_inches="tight"); print("saved:", out)


def main():
    print("=== Radiation-impedance audit (FIX 1) ===")
    print(f"{'freq':>5} {'ka':>7} {'R1':>9} {'X1(script)':>12} {'X1(textbook)':>13}")
    for hz in FREQS:
        ka = 2 * np.pi * hz / C0 * A
        R1, Xs, Xc = radiation_impedance_check(hz)
        print(f"{hz:>5} {ka:>7.3f} {R1:>9.5f} {Xs:>12.5f} {Xc:>13.5f}")
    print("\n=== Piston off-axis level (should be ~0 dB = near-omni at low f) ===")
    for hz in FREQS:
        D90 = piston_directivity(np.array([np.pi / 2]), hz)[0]
        # use the raw piston term (without taper) for the physical statement
        ka = 2 * np.pi * hz / C0 * A
        raw90 = abs(2 * j1(ka) / ka) if ka > 1e-10 else 1.0
        print(f"{hz:>3} Hz: piston |D(90)| raw = {raw90:.3f} "
              f"({20*np.log10(raw90):+.2f} dB)")
    print()
    plot_directivity_comparison()
    plot_heatmaps(200, 1.0)


if __name__ == "__main__":
    main()
