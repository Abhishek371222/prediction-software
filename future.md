# Future work — terminal / AutoCAD command line

Deferred items. Do not treat as committed scope until picked up.

## Point entry (beyond absolute `x,y`)

Today the terminal accepts absolute Cartesian points only (e.g. `10,20`).

| Feature | AutoCAD-style example | Notes |
|---------|----------------------|--------|
| Relative polar | `@5<45` | Distance + angle from last point (CCW from +X unless units say otherwise) |
| Relative Cartesian | `@3,4` | Offset from last point |
| Direct distance entry | Aim cursor, type `5`, Enter | Length along current rubber-band / cursor direction |

Wire into the same draw/session path as existing `parseAnnotPoint` / `feedAnnotPoint` — no second geometry stack.

## Unsupported AutoCAD commands (need new geometry/APIs first)

Do not register aliases until implemented (keep `Unknown command`):

- `CO` — AutoCAD displace **COPY** (clipboard COPY stays separate)
- `RO` / `SC` / `MI` — ROTATE / SCALE / MIRROR (general selection)
- `O` / `TR` / `EX` / `F` / `CHA` — OFFSET / TRIM / EXTEND / FILLET / CHAMFER
- `X` / `J` — EXPLODE / JOIN
- `MT` — MTEXT
- `AA` / `ID` / `LA` — AREA / ID / LAYER

## Docs polish

- Sync `cmdref.md` with terminal aliases and point-entry syntax when the above ships.
