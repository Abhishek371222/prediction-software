"""



Generate ShyamGuild-style polar plots (SPL Radiation Pattern vs Distance) for the



requested frequencies and save them into the user's Downloads folder.







Style matches ShyamGuild24.6.26_polarPlots reference:



  * one figure per frequency, 0.5 m (blue) and 1 m (orange) overlaid



  * linear radial axis R = 10^((SPL - SPL_ref)/20)



  * MATLAB-like polar axes (0 deg right, counter-clockwise, 0..360 labels)







Only 0.5 m and 1 m sets are used (the "2" m files are ignored).



"""







import os



import numpy as np



import openpyxl



import matplotlib.pyplot as plt







HERE = os.path.dirname(os.path.abspath(__file__))



OUT = os.path.join(HERE, "..", "Downloads")   # D:\shayam gui\Downloads







REQUESTED = [30, 80, 200, 500]       # what the user asked for



DISTANCES = ["0.5", "1"]







C_REF = "#0072BD"   # 0.5 m



C_1M = "#D95319"    # 1 m







# Per-frequency reference SPL (dB), reverse-engineered from the reference plots.



REF_SPL = {30: 82.70, 200: 95.89, 500: 96.748}











def data_path(freq, dist):



    return os.path.join(HERE, f"Frequency_{freq}_{dist}Horizantal.xlsx")











def available(freq):



    return all(os.path.exists(data_path(freq, d)) for d in DISTANCES)











def load(freq, dist):



    ws = openpyxl.load_workbook(data_path(freq, dist), data_only=True).active



    ang, spl = [], []



    for i, row in enumerate(ws.iter_rows(values_only=True)):



        if i == 0 or row[0] is None or row[1] is None:



            continue



        ang.append(float(row[0]))



        spl.append(float(row[1]))



    ang, spl = np.array(ang), np.array(spl)



    o = np.argsort(ang)



    return ang[o], spl[o]











def ref_for(freq):



    if freq in REF_SPL:



        return REF_SPL[freq]



    _, a = load(freq, "0.5")



    _, b = load(freq, "1")



    both = np.concatenate([a, b])



    return 10 * np.log10(np.mean(10 ** (both / 10.0)))











def plot_freq(freq):



    ref = ref_for(freq)



    a05, s05 = load(freq, "0.5")



    a1, s1 = load(freq, "1")



    r05 = 10 ** ((s05 - ref) / 20.0)



    r1 = 10 ** ((s1 - ref) / 20.0)







    fig = plt.figure(figsize=(8.5, 8))



    fig.patch.set_facecolor("#ececec")



    ax = fig.add_subplot(111, projection="polar")



    ax.plot(np.deg2rad(a05), r05, color=C_REF, lw=2, label="0.5 m (Reference)")



    ax.plot(np.deg2rad(a1), r1, color=C_1M, lw=2, label="1 m (~6 dB loss)")







    i0 = np.argmin(np.abs(((a05 + 180) % 360) - 180))



    ax.plot(np.deg2rad(a05[i0]), r05[i0], "o", color="k", ms=4)



    ax.plot(np.deg2rad(a1[i0]), r1[i0], "o", color="k", ms=4)







    ax.set_theta_zero_location("E")



    ax.set_theta_direction(1)



    ax.set_thetagrids(range(0, 360, 30))



    ax.set_facecolor("white")



    rmax = float(np.ceil(max(r05.max(), r1.max())))



    ax.set_rlim(0, rmax)



    ax.set_rticks(np.arange(0, rmax + 1e-9, 1.0))



    ax.set_rlabel_position(80)



    ax.grid(True, color="0.8", lw=0.8)



    ax.set_title(f"{freq} Hz SPL Radiation Pattern vs Distance",



                 fontweight="bold", pad=22)



    fig.legend(loc="lower center", ncol=1, bbox_to_anchor=(0.5, -0.02),



               frameon=True)







    out = os.path.join(OUT, f"PolarVsDistance_{freq}Hz.png")



    fig.savefig(out, dpi=140, bbox_inches="tight", facecolor=fig.get_facecolor())



    plt.close(fig)



    print(f"saved: {out}  (SPL_ref={ref:.3f} dB)")











def main():



    os.makedirs(OUT, exist_ok=True)



    print(f"output folder: {OUT}")



    made, missing = [], []



    for f in REQUESTED:



        if available(f):



            plot_freq(f)



            made.append(f)



        else:



            missing.append(f)



    if missing:



        print(f"\nNo measurement data for: {missing} Hz "



              f"(available frequencies: 30, 80, 200, 500 Hz)")



    print(f"generated {len(made)} plots: {made} Hz")











if __name__ == "__main__":



    main()



