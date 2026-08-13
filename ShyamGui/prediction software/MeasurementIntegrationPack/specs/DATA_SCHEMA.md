# Measurement data schema

## manifest.csv

| Column | Type | Example | Notes |
|---|---|---|---|
| set | string | ShyamGuild | Set ID: ShyamGuild, Factory, 3inch, XN18 |
| freq_hz | int | 200 | Frequency in Hz |
| distance_m | float | 0.5 | Mic distance in metres |
| file | string | ShyamGuild_200Hz_0p5m.csv | CSV filename in `Data/` |
| source_xlsx | string | Frequency_200_0.5Horizantal.xlsx | Original Excel name |
| n_points | int | 36 | Number of angle samples |

Filename distance encoding: `0p5` = 0.5, `1p0` = 1.0, `2p0` = 2.0.

## Sweep CSV

```text
degree,dBSPL
0.0,105.310
10.0,104.880
```

| Column | Unit | Notes |
|---|---|---|
| degree | degrees | 0 = on-axis; increasing clockwise in CLIO plots |
| dBSPL | dB SPL | Absolute level as recorded |

Rules:

- No 360° row (0° only).
- Sorted ascending by degree.
- ShyamGuild / Factory / 3inch: typically 0,10,...,350 (36 points).
- XN18: 0,1,...,359 (360 points).

## Lookup key

Primary key: `(set, freq_hz, distance_m)`.

## Derived quantities

```text
on_axis = SPL at angle nearest 0°
rel_dB(θ) = SPL(θ) - on_axis
D_linear(θ) = 10^(rel_dB(θ) / 20)     # on-axis = 1
```

For field interpolation, use periodic interpolation over degree ∈ [0, 360).

## Source Excel mirror

`Data/source/<set>/<original.xlsx>` — originals used to generate CSVs. CSVs are verified exact for ShyamGuild (max delta 0.000 dB).
