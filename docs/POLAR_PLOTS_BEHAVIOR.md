# Directivity & Measured Polar — Behaviour Guide

A sound-engineering / maths description of how the two polar views behave in
**Atomik Simulation Engine**, what each drawn element means, and where the
underlying data comes from.

This note is **not** a coding guide. It is for people who think in **dB, angles,
arrays, and measurement arcs**.

---

## 1. The two views at a glance

| View | Question it answers | What you see |
|------|---------------------|--------------|
| **Measured Polar** | “What does **one** Q21S look like, from the lab / BEM polar data?” | One **black** curve = measured (or BEM-derived) horizontal polar at a chosen distance |
| **Directivity** | “What does **this layout of enabled cabinets** look like in the far field?” | One **red** curve = **simulated** array pattern from the acoustic engine |

They share the same **plot frame** (rings, spokes, dB scale, angle convention),
but they do **not** plot the same physical quantity.

- **Measured Polar** → data about a **single unit**.
- **Directivity** → prediction of the **current array** (1, 2, or 3 enabled Q21S units, with gain / delay / polarity / facing).

---

## 2. Shared polar frame (how to read the chart)

Both views use a CLIO-style polar sheet:

### Angle

- **0°** = forward = **right** on the screen (+X in the plan / heatmap).
- **90°** = up (+Y).
- Angles increase **counter-clockwise** (mathematical / heatmap sense).

So a lobe pointing **right** is on-axis forward. A lobe pointing **left** is rear.

### Level (radius)

Concentric rings are **relative level in dB**:

| Ring label | Meaning |
|------------|---------|
| **0** | Reference (outer working ring for 0 dB relative) |
| **−6, −12, −18, −24** | Steps of 6 dB down |
| Scale also allows up to **+6** on the outer chrome | Headroom above the 0 dB ring |

For **Measured Polar**, relative level is:

\[
L_{\mathrm{rel}}(\theta) = \mathrm{SPL}(\theta) - \mathrm{SPL}(0^\circ)
\]

so **on-axis is always 0 dB** on that plot (CLIO convention).

For **Directivity**, the engine builds a complex far-field magnitude \(M(\theta)\),
normalises it to its own peak, then plots:

\[
L_{\mathrm{rel}}(\theta) = 20\log_{10}\!\big(M(\theta)\big)
\quad\text{with } \max_\theta M = 1
\]

so the **peak of the simulated pattern** sits at 0 dB (wherever that peak points).

---

## 3. Measured Polar — every component

### 3.1 The black curve

**What it is**  
Horizontal directivity of **one** Q21S at the selected **frequency** and
**measurement distance**, drawn as a smooth closed polar.

**What it is not**  
It is **not** the sum of two or three cabinets. Moving / enabling / delaying
speakers on the plan does **not** reshape this black curve. Only frequency and
distance (and which measurement set is loaded) change it.

**Math**

1. Raw samples: absolute SPL vs angle, \(\mathrm{SPL}(\theta)\) in dB.
2. Find on-axis level \(\mathrm{SPL}(0^\circ)\).
3. Plot relative:

\[
L_{\mathrm{rel}}(\theta)=\mathrm{SPL}(\theta)-\mathrm{SPL}(0^\circ)
\]

4. For drawing, the curve is interpolated / smoothed in the **dB domain** and
   sampled densely (about every ½°) so the stroke looks like a continuous polar,
   not a jagged polyline.

### 3.2 Beamwidth (BW) and the orange markers

When beamwidth can be computed:

- The software walks left and right from **0°** until relative level falls below
  **−6 dB**.
- **BW** = full angle between those two crossings (degrees).
- A faint **−6 dB ring** may be sketched.
- Two **orange/brown dots** mark the ± half-beamwidth points on that −6 dB ring.
- Text like **“BW 40° (−6 dB)”** is that full beamwidth.

This is standard loudspeaker reporting: **−6 dB horizontal beamwidth** relative
to on-axis.

### 3.3 Legend strip (“0.5 m” / “1.0 m” / “2.0 m”)

That is the **polar distance** of the laboratory / BEM arc you are looking at —
not the world size of the heatmap.

Typical Q21S pack distances:

| Distance | Role |
|----------|------|
| **0.5 m** | Near-field arc (cabinet geometry still “imprinted”) |
| **1.0 m** | Mid |
| **2.0 m** | Preferred **far-field** arc for prediction |

SPL heatmaps and array physics prefer the **largest available radius ≥ 1 m**
(usually **2.0 m**) so near-field cabinet lobes are not mistaken for far-field
directivity.

### 3.4 Where the numbers come from (files)

Product data set: **Q21S** (Ground Plane / open-field style BEM polars).

Canonical pack files look like:

```text
Q21S_<frequency>Hz_<distance>m.csv
```

Examples:

- `Q21S_52Hz_2p0m.csv`
- `Q21S_81Hz_0p5m.csv`
- `Q21S_198Hz_1p0m.csv`

Each CSV is essentially:

| Column | Content |
|--------|---------|
| Angle (°) | Horizontal angle |
| SPL (dB) | Absolute level on that arc |

**Origin of those CSVs**  
They are exported from the BEM / polar workbook pipeline
(historically under folders such as `BEM_Data_10m` / per-Hz workbooks, then
packed as MeasurementIntegrationPack CSVs). In a shipped install they may be
**embedded inside the application** so Excel folders need not travel with the
Setup.

**Frequencies in the UI catalogue** (native BEM bands):

```text
20, 29, 52, 81, 98, 153, 198, 256, 309, 352, 400, 401 Hz
```

If you pick a frequency that has no exact file, the app can **blend** the
nearest measured anchors in log-frequency space at the same distance — still
one unit curve, still black on Measured Polar.

**Legacy note**  
Older workflows also used Factory / room `.xlsx` sets. The live product path
for Q21S is the **CSV pack** (or embedded copy of that pack). Room / Guild
frequencies (e.g. 30 / 80 / 200 / 500 Hz) are a separate legacy set and are not
what the current Q21S Ground Plane UI emphasises.

---

## 4. Directivity — every component

### 4.1 The red curve (what you keep)

**What it is**  
Far-field **simulated** radiation pattern of **all enabled** Q21S cabinets in
the current project, at the selected frequency.

**What it is not**  
It is **not** a second measured Excel curve. It is not a pasted CLIO screenshot.
It is computed when you **RUN** the simulation.

**Why it often looks “flower-like” with several lobes**  
With **two or more** cabinets (spacing, delay, polarity reverse, rear-facing),
you get **coherent interference** in the far field: lobes and nulls. Classic
cardioid / end-fire style presets are a special case of that.

With **one** cabinet, the red curve is essentially that unit’s predicted
far-field shape (using measured directivity \(D(\theta)\) inside the engine),
peak-normalised.

### 4.2 How the red curve is formed (math)

For each direction \(\theta\) around the array centre:

1. Place a virtual listener far away (many times the world size) at angle
   \(\theta\).
2. For each **enabled** speaker \(s\):

\[
A_s(\theta)
  = G_s \cdot D_s(\theta_s) \cdot \frac{1}{r_{\mathrm{spread},s}}
  \cdot e^{-j\big(k\,r_s + \omega\,\tau_s\big)}
  \cdot e^{j\phi_s}
\]

where roughly:

| Symbol | Meaning |
|--------|---------|
| \(G_s\) | Linear gain from the cabinet gain (dB) |
| \(D_s\) | Unit directivity factor toward the listener (from measured / BEM polar tables) |
| \(r_s\) | Geometric path length |
| \(r_{\mathrm{spread}}\) | Path used for \(1/r\) spreading (floored near the cabinet so the singularity is physical) |
| \(\tau_s\) | Delay (seconds) from delay ms |
| \(\phi_s\) | Extra phase (e.g. polarity invert = \(\pi\)) |
| Facing | Forward (+X) or reverse (−X) rotates which way “0°” of the unit points |

3. Coherent sum:

\[
P(\theta)=\sum_s A_s(\theta)
\quad,\quad
M(\theta)=\lvert P(\theta)\rvert
\]

4. Normalise \(\max M = 1\), then plot \(20\log_{10} M(\theta)\).

So the **red** plot answers: *given these cabinets, this drive, this geometry —
what is the far-field pattern?*

### 4.3 Red squares on the chart

Small **red rounded markers** are the **enabled speakers**, drawn in a
schematic polar inset (scaled positions relative to the array centre). They are
**not** measurement microphones and not data points of the polar sample set.

### 4.4 What Directivity no longer shows

Previously, Directivity also drew a **black** “unit measured” polar underneath
the red array overlay. That black trace was the **same family of data** as
Measured Polar (one-cabinet CSV / BEM arc). It was easy to confuse with the
array result.

**Current behaviour:** Directivity shows **only the red simulated pattern**
(+ speaker markers). Measured unit polars live exclusively under **Measured Polar**.

---

## 5. How Measured Polar feeds the heatmap (and the red Directivity)

Even though Measured Polar is a **display** of unit data, that same data family
drives prediction:

1. Take the **far-field** unit polar (prefer **2 m** when available).
2. Convert to linear directivity:

\[
D(\theta)=10^{\big(\mathrm{SPL}(\theta)-\mathrm{SPL}(0^\circ)\big)/20}
\quad\Rightarrow\quad D(0^\circ)=1
\]

3. On the **100 × 100 m** plan, each enabled cabinet radiates with that \(D(\theta)\)
   and spherical spreading \(\sim 1/r\) (intensity \(\sim 1/r^2\)).
4. Multiple cabinets: **complex coherent sum** (same idea as the red Directivity
   curve, but sampled on a grid instead of on a far circle).

So:

| Surface | Uses unit \(D(\theta)\)? | Shows multi-cab interference? |
|---------|--------------------------|-------------------------------|
| Measured Polar (black) | Displays the unit curve itself | No |
| Directivity (red) | Yes, inside each cabinet’s contribution | Yes (when ≥2 enabled) |
| SPL heatmap | Yes | Yes |

---

## 6. Colour cheat-sheet (current product)

| Colour / mark | View | Meaning |
|---------------|------|---------|
| **Black closed curve** | Measured Polar | One-cabinet measured / BEM horizontal polar at chosen Hz & distance |
| **BW text + −6 dB cues + orange dots** | Measured Polar | −6 dB beamwidth of that unit curve |
| **Red closed curve** | Directivity | Simulated far-field pattern of the **enabled array** |
| **Red squares** | Directivity | Schematic positions of enabled Q21S cabinets |
| Grid rings 0 / −6 / … / −24 | Both | Relative dB radius |
| Angle labels | Both | 0° forward (right), CCW |

---

## 7. Practical checklist for sound / maths review

1. **Want lab / BEM shape of one box?** → Measured Polar. Expect **black**. Change
   distance (0.5 / 1 / 2 m) and frequency; speakers on the plan should **not**
   morph that curve.
2. **Want array lobes / cardioid / cancellation?** → Directivity after **RUN**.
   Expect **red**. Change spacing, delay, polarity, facing, enable/disable —
   the red curve should respond.
3. **0° is forward (right).** Rear is 180° (left).
4. **Levels on both polars are relative**, not absolute dB SPL of a venue.
5. **Heatmap absolute calibration** still uses on-axis BEM level at the far-field
   reference distance; the polar charts themselves are relative patterns.

---

## 8. Data inventory (Q21S)

| Item | Typical form |
|------|----------------|
| Unit polars | `Q21S_<Hz>Hz_0p5m.csv`, `_1p0m.csv`, `_2p0m.csv` |
| Hz catalogue | 20, 29, 52, 81, 98, 153, 198, 256, 309, 352, 400, 401 |
| Preferred prediction arc | **2.0 m** when present |
| On-axis convention | \(\mathrm{SPL}(0^\circ)\) → 0 dB relative; \(D(0^\circ)=1\) |
| Beamwidth | Full angle where \(L_{\mathrm{rel}}\) stays above −6 dB, both sides of 0° |
| Shipped app | Same CSV content may be **embedded** (no Excel folder required on the PC) |

---

## 9. One-sentence summary

**Measured Polar (black)** shows *what one Q21S measures as* on a polar arc;
**Directivity (red)** shows *what the current array predicts in the far field*
after coherent summation — using those unit polars as \(D(\theta)\) inside the
engine, not as a second black overlay on the same chart.
