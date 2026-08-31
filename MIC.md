# Mic option

Virtual receivers (EASE Focus–style probes) on the SPL heatmap. They do **not** change field computation — they only sample the existing map / engine at a point.

## Open the tool

On the plot toolbar, click the **mic** button. A menu appears:

| Item | What it does |
|------|----------------|
| **Add Mic** | Arms placement mode (crosshair). Click the field to place. Stay armed to place more. **Esc** cancels. |
| **Place on ring** | Per existing mic: open the ring / reference dialog (1 / 2 / 4 / 8 m around a speaker). Disabled until at least one mic exists. |
| **Show Degrees** | Toggle: show each mic’s angle (°) vs the Q21S facing, before the dB on the label (e.g. `Mic 1  45°  −12.3 dB`). 0° = front of the speaker; angles increase CCW. |

Toolbar highlight / tooltip show when Add Mic is armed.

## Place and edit

- **Add Mic armed** — click places a mic (named `Mic 1`, `Mic 2`, …). Position snaps to the nearest **1 / 2 / 4 / 8 m** ring around an enabled speaker when within ~0.45 m of that ring (tak on latch).
- **Select** — click a mic to select; drag follows the cursor with the same ring snap. **Delete** / **Backspace** removes the selected mic.
- Drawn as the mic glyph + live relative SPL label (e.g. `Mic 1  −12.3 dB`) when the heatmap has data. With **Show Degrees**, angle vs the reference Q21S (ring speaker, else nearest enabled) is inserted before dB.
- Drawn as the mic glyph + live relative SPL label (e.g. `Mic 1  −12.3 dB`) when the heatmap has data. With **Show Degrees**, angle vs the reference Q21S (ring speaker, else nearest enabled) is inserted before dB.

## Live level

After each simulation (or when mics move), relative SPL is bilinear-sampled from the current heatmap grid at `(x, y)`. Same relative scale as the colour map (peak ≈ 0 dB).

## Frequency Response panel

When mics exist, a **small separate Frequency Response window** opens (does not shrink the heatmap). It shows one curve per mic across supported Hz. Curves come from point probes (`AcousticEngine::sampleIntensityAt`), not a full re-grid. Legend: left-click sets the **reference** mic (`*`); others are plotted relative to it. The window closes when all mics are removed.

## Snap sound (“tak”)

There is **no separate `.wav` asset on disk**. The click is a short PCM WAV built and played in memory when a mic **newly clips** onto a ring.

| What | Where |
|------|--------|
| Sound generation + play | [`Source/MicRingSnap.h`](Source/MicRingSnap.h) — namespace `SnapClick`, function `playTak()` |
| Called from | [`Source/RadiationPatternComponent.cpp`](Source/RadiationPatternComponent.cpp) — `snapMicWorld(..., playSoundIfNewClip)` |
| Format | In-memory RIFF/WAV, 8-bit mono PCM, 22 050 Hz, ~18 ms (400 samples) |
| Playback (Windows) | `PlaySoundA(..., SND_MEMORY \| SND_ASYNC \| SND_NODEFAULT)` via `winmm` |

Open `Source/MicRingSnap.h` and search for `playTak` to see the WAV bytes / tone synthesis (≈1850 Hz + 3100 Hz with fast decay).

## Main source files (mic only)

| File | Role |
|------|------|
| [`Source/MicReceiver.h`](Source/MicReceiver.h) | Mic data (position, live SPL, optional reference lock) |
| [`Source/MicRingSnap.h`](Source/MicRingSnap.h) | Ring snap (1/2/4/8 m) + **tak** sound |
| [`Source/MicRefLockDialog.h`](Source/MicRefLockDialog.h) | Place-on-ring / dB dialog |
| [`Source/MicRefLevelEditor.h`](Source/MicRefLevelEditor.h) | Reference level editor UI |
| [`Source/RadiationPatternComponent.*`](Source/RadiationPatternComponent.h) | Place / select / draw / snap / keys |
| [`Source/MainComponent.*`](Source/MainComponent.h) | Toolbar menu, mic list, live levels, FR updates |
| [`Source/AcousticEngine.*`](Source/AcousticEngine.h) | `sampleIntensityAt` point probe for FR curves |
