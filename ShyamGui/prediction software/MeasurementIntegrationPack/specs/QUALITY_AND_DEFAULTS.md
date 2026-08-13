# Quality flags and recommended defaults

## Defaults for “Use measured directivity”

```text
set        = ShyamGuild
distance_m = 0.5
freqs      = 80, 200, 500   # Hz
normalize  = on-axis (0°)
```

Prefer 0.5 m, then 1.0 m. Avoid 2.0 m for free-field / DI work.

## Trust matrix

| Sweep | Trust for model? | Notes |
|---|---|---|
| ShyamGuild 80/200/500 @ 0.5 m | **Yes — best** | Clean lobes; used by prior verify_model.py |
| ShyamGuild 80/200/500 @ 1.0 m | **Yes** | Good; ~6–8 dB down from 0.5 m |
| ShyamGuild 80/200/500 @ 2.0 m | No (room) | Only 1–3 dB down from 1 m (expect −6) |
| ShyamGuild 30 Hz any distance | No | Room modes; peak not on-axis |
| Factory 60–200 @ 1 m | Yes (cross-check) | 200 Hz shape matches ShyamGuild (corr 0.93) |
| Factory 30 @ 1 m | **Discard** | Corrupt / jagged |
| 3inch 300–16k @ 1 m | HF product only | Different device; near-omni below ~1 kHz |
| XN18 30–200 @ 1 m | Display / CLIO compare | 1° resolution; not same numbers as ShyamGuild |

## Cross-set facts (do not merge blindly)

- ShyamGuild vs Factory @ 200 Hz, 1 m: shape corr **0.93**, Factory ~**7 dB** louder.
- ShyamGuild vs XN18 @ 30 Hz: shape corr **~0.26** — different files.
- ShyamGuild vs 3inch @ 500 Hz: different devices (directional vs near-omni).

## Distance law (ShyamGuild on-axis)

Expected free-field for distance doubling: **−6.02 dB**.

| Freq | 0.5→1 m actual | excess vs −6.02 | 1→2 m actual | excess |
|---|---|---|---|---|
| 30 | −4.97 | +1.05 | −2.94 | +3.08 |
| 80 | −8.75 | −2.73 | −1.30 | +4.72 |
| 200 | −7.62 | −1.60 | −2.16 | +3.86 |
| 500 | −6.43 | −0.41 | −1.06 | +4.96 |

500 Hz @ 0.5→1 m is closest to free-field. All 1→2 m steps show strong room gain.

## Sanity check values (ShyamGuild)

| File | On-axis dBSPL | Notes |
|---|---|---|
| 200 Hz 0.5 m | 105.310 | Peak on-axis |
| 200 Hz 1.0 m | 97.690 | |
| 500 Hz 0.5 m | 104.240 | |
| 80 Hz 0.5 m | 106.280 | |
| 30 Hz 0.5 m | 85.840 | Peak at 320° = 88.750 — do not use |

At 200 Hz 0.5 m, relative to on-axis: 90° ≈ −7.76 dB, 180° ≈ −14.42 dB.
