You are working on my prediction/simulation software UI.

TASK:
Make the ENTIRE APPLICATION properly responsive and adaptive across different window sizes and screen resolutions.

IMPORTANT:
Do NOT redesign the application.
Do NOT change the existing brand identity.
Do NOT change functionality, calculations, simulation logic, data structures, or workflows.

The goal is ONLY to make the existing UI responsive, consistent, usable, and professional at every screen size.

==================================================
1. RESPONSIVENESS MUST APPLY TO EVERY SCREEN
==================================================

Audit the ENTIRE application.

Do not only fix the current/main screen.

Check every:
- Page
- Screen
- Modal
- Dialog
- Drawer
- Sidebar
- Toolbar
- Ribbon
- Header
- Footer
- Settings screen
- Project screen
- Statistics screen
- Simulation screen
- Prediction screen
- Configuration panel
- Device panel
- Chart
- Graph
- Table
- Form
- Dropdown
- Context menu
- Tooltip
- Empty state
- Loading state
- Error state
- Confirmation dialog
- Popover
- Any dynamically generated UI

Every screen must remain usable when the application window is resized.

==================================================
2. DESKTOP-FIRST APPLICATION
==================================================

This is a professional desktop engineering/prediction application.

Do NOT treat it like a normal mobile website.

The application should prioritize:

1. Large desktop
2. Standard desktop/laptop
3. Smaller laptop
4. Minimum supported desktop window

If mobile/tablet support already exists, preserve it, but do not sacrifice desktop usability.

The application must NEVER depend on a fixed 1920×1080 layout.

==================================================
3. TEST THESE WINDOW SIZES
==================================================

Explicitly test the UI at:

3840 × 2160
2560 × 1440
1920 × 1080
1600 × 900
1440 × 900
1366 × 768
1280 × 800
1280 × 720
1024 × 768

Also test intermediate widths by manually resizing the application.

The UI must not:
- overlap
- clip
- disappear unexpectedly
- create horizontal scrolling unnecessarily
- push important controls outside the viewport
- create unusably small controls
- break charts
- break tables
- break the ribbon
- break sidebars
- cause text to overlap
- cause buttons to collide
- create excessive empty space

==================================================
4. RIBBON RESPONSIVENESS
==================================================

The application uses a RIBBON-style toolbar.

Treat the ribbon as a major responsive component.

Do NOT allow ribbon controls to simply shrink until they become unreadable.

At large widths:

Show:
- Full tool groups
- Icons
- Labels
- Group names
- Dropdown indicators
- Separators

At smaller widths:

Progressively adapt the ribbon.

Example strategy:

LARGE:
Full ribbon

MEDIUM:
Reduce spacing and icon size slightly.
Keep important labels.

SMALL:
Collapse less frequently used tools into dropdown/overflow menus.
Keep primary tools visible.

VERY SMALL:
Use compact ribbon groups / overflow menu.

Never:
- wrap ribbon buttons awkwardly onto random lines
- clip buttons
- overlap groups
- make labels unreadable
- create horizontal page scrolling

The active tab must remain clearly visible.

The ribbon must preserve a logical hierarchy.

==================================================
5. TOP HEADER RESPONSIVENESS
==================================================

The top application header must adapt.

It contains elements such as:

- ATOMIK branding
- Application name
- Version
- Statistics
- Autosave
- Project
- Help
- Settings
- More menu

At large widths:
Show all important controls.

At smaller widths:
Prioritize:

ATOMIK / application identity
Project
Autosave status
Settings
More

Secondary information can move into overflow menus.

Never allow:
- header controls to overlap
- application title to collide with buttons
- icons to be pushed outside the window
- buttons to become microscopic

==================================================
6. LEFT SIDEBAR RESPONSIVENESS
==================================================

The configuration/sidebar area must be responsive.

Do NOT allow it to consume most of the screen on smaller windows.

Implement sensible behavior:

Large screen:
Normal sidebar width.

Medium screen:
Slightly narrower sidebar.

Small desktop:
Allow sidebar collapse.

Collapsed state:
Show a clean compact rail or button to reopen it.

Important:
The user must always be able to access all controls.

Do NOT simply hide controls permanently.

Sections inside the sidebar must also resize correctly.

==================================================
7. MAIN CANVAS / SIMULATION AREA
==================================================

The simulation/prediction canvas is the most important part of the application.

It must receive the majority of available space.

The canvas should automatically resize when:

- Sidebar opens
- Sidebar closes
- Ribbon changes height
- Window resizes
- Panels open
- Panels close

Charts, heatmaps, graphs and drawing areas must use the actual available container dimensions.

Do NOT use hardcoded canvas dimensions such as:

width: 1200px
height: 700px

Instead use responsive container dimensions.

The canvas must never be cut off because of the sidebar or ribbon.

==================================================
8. CHARTS / GRAPHS / HEATMAPS
==================================================

Every visualization must resize correctly.

Check:

- SPL heatmaps
- Prediction graphs
- Frequency graphs
- Response curves
- Tables
- Axes
- Legends
- Labels
- Grid
- Measurement overlays
- Tooltips

Charts must recalculate their drawing dimensions when their parent container changes size.

Use ResizeObserver where appropriate.

Do NOT rely only on window.resize.

==================================================
9. TABLE RESPONSIVENESS
==================================================

Tables must remain usable.

For wide screens:
Show full columns.

For smaller screens:
Use one of these approaches depending on the table:

- Horizontal scrolling inside the table container
- Column prioritization
- Expandable rows
- Details panel
- Compact columns

Do NOT allow the entire application to horizontally scroll just because one table is wide.

The table itself should scroll.

Headers must remain aligned with data.

==================================================
10. FORMS AND INPUTS
==================================================

All inputs must resize appropriately.

Check:

- Text inputs
- Number inputs
- Sliders
- Dropdowns
- Checkboxes
- Radio buttons
- Toggle switches
- Buttons
- Search fields

Do not make controls unnecessarily huge on small screens.

Do not make controls too small to interact with.

Labels must never overlap inputs.

Numeric engineering values must remain readable.

==================================================
11. MODALS / DIALOGS
==================================================

Every modal must be responsive.

Large screen:
Use appropriate fixed/max width.

Small screen:
Modal should fit inside viewport.

Use:

max-width
max-height
width: calc(...)
overflow handling

Content should scroll INSIDE the modal when required.

Do not let the entire application become unusable because a modal is larger than the viewport.

Buttons must remain visible.

For long forms:
Header and footer/actions should remain accessible while content scrolls.

==================================================
12. OVERFLOW RULES
==================================================

Avoid global horizontal scrolling.

Do NOT blindly use:

overflow-x: hidden

as a solution.

First identify the actual component causing overflow.

Fix the component itself.

Use overflow only where it makes sense:

- Tables → internal horizontal scrolling
- Long lists → internal scrolling
- Ribbon → controlled overflow/collapse
- Canvas → controlled viewport behavior
- Modal content → internal scrolling

==================================================
13. NO FIXED PIXEL LAYOUT
==================================================

Audit the existing code for excessive fixed dimensions.

Look for:

width: xxxpx
height: xxxpx
margin-left: xxxpx
left: xxxpx
top: xxxpx

Replace inappropriate fixed sizing with responsive techniques.

Use where appropriate:

flex
grid
min-width
max-width
min-height
max-height
clamp()
calc()
1fr
auto
percentage sizing
viewport/container sizing

Fixed dimensions are acceptable only where they are intentionally required, such as:
- icon buttons
- minimum control sizes
- engineering-specific canvas elements
- compact toolbar controls

==================================================
14. USE FLEXBOX / CSS GRID PROPERLY
==================================================

Prefer:

CSS Grid for major application layout.

Flexbox for:

- toolbars
- ribbon groups
- headers
- button groups
- form rows

The application structure should behave like:

┌──────────────────────────────────────┐
│ Application Header                   │
├──────────────────────────────────────┤
│ Ribbon / Toolbar                     │
├──────────────┬───────────────────────┤
│              │                       │
│ Sidebar      │ Main Simulation Area  │
│              │                       │
│              │                       │
├──────────────┴───────────────────────┤
│ Status Bar                            │
└──────────────────────────────────────┘

All regions must adapt to available space.

==================================================
15. SIDEBAR + CANVAS INTERACTION
==================================================

This is extremely important.

When sidebar width changes:

Main canvas must automatically resize.

When sidebar collapses:

Main canvas must expand.

When sidebar opens:

Main canvas must shrink gracefully.

Never let the canvas remain at its old fixed width.

The same principle applies to:

- Properties panels
- Device panels
- Statistics panels
- Settings panels
- Any dockable UI

==================================================
16. HEIGHT RESPONSIVENESS
==================================================

Do not only test width.

Test vertical space too.

Especially:

1366 × 768
1280 × 720
1024 × 768

The ribbon + header + sidebar + status bar must not consume the entire viewport.

The main workspace should remain usable.

If content becomes too tall:

Make the relevant panel scroll internally.

Do NOT make the entire application page scroll unnecessarily.

==================================================
17. TYPOGRAPHY
==================================================

Typography must scale sensibly.

Do not allow:

- clipped text
- overlapping labels
- unreadable text
- excessive line wrapping

Use sensible:

font-size
line-height
letter-spacing
white-space
text-overflow

For labels that cannot fit:

Use ellipsis + tooltip where appropriate.

Do NOT randomly reduce font sizes just to make everything fit.

==================================================
18. ICONS
==================================================

Icons must remain visually consistent.

Do not distort icons.

Maintain consistent:

- size
- alignment
- spacing
- stroke weight
- visual hierarchy

Ribbon icons should scale only within reasonable limits.

==================================================
19. ATOMIK BRANDING
==================================================

IMPORTANT:

Preserve the existing ATOMIK visual identity exactly.

Do NOT introduce:
- blue UI accents
- purple
- green
- gradients
- random colors

Use the existing ATOMIK palette.

Primary visual language:

- Black / near-black
- Dark charcoal
- White
- Light gray
- ATOMIK red accent

Red should be used strategically for:
- active states
- important actions
- selected tools
- alerts
- key controls

Do not flood the entire UI with red.

Keep the professional engineering-software appearance.

==================================================
20. COMPONENT CONSISTENCY
==================================================

Create/reuse responsive layout primitives where appropriate.

For example:

ResponsiveShell
ResponsivePanel
ResponsiveToolbar
ResponsiveRibbon
ResponsiveModal
ResponsiveTable
ResponsiveCanvasContainer

Do NOT duplicate responsive CSS unnecessarily.

If the project already has a design system, use it instead of creating another one.

==================================================
21. RESPONSIVE BREAKPOINTS
==================================================

Do not blindly use generic web breakpoints.

Choose breakpoints based on actual application layout requirements.

Example starting points:

≥ 1600px
Full desktop ribbon

1280–1599px
Compact desktop ribbon

1024–1279px
Condensed ribbon + collapsible panels

< 1024px
Compact/overflow toolbar strategy

Adjust these after testing the actual UI.

Do not force these values if the application's actual layout requires different thresholds.

==================================================
22. IMPORTANT: NO FUNCTIONAL REGRESSIONS
==================================================

Do NOT modify:

- Simulation calculations
- Prediction algorithms
- Audio calculations
- Frequency calculations
- Device data
- Physics
- API behavior
- State management
- File formats
- Project saving
- Autosave
- Import/export
- Keyboard shortcuts
- Existing commands

unless absolutely necessary to make layout resizing work.

This is a UI responsiveness task.

==================================================
23. KEYBOARD / ENGINEERING WORKFLOW
==================================================

This is a professional prediction/simulation tool.

Do not destroy desktop workflows.

Existing keyboard shortcuts must continue working.

Mouse interactions must continue working.

Canvas interactions must continue working.

Ribbon commands must continue working.

Dragging devices/components must continue working.

Zoom/pan must continue working.

==================================================
24. IMPLEMENTATION PROCESS
==================================================

Before changing code:

1. Inspect the complete project structure.
2. Identify the main application shell.
3. Identify routing/navigation.
4. Identify every major screen.
5. Identify shared UI components.
6. Identify ribbon/header components.
7. Identify sidebar/panel components.
8. Identify chart/canvas components.
9. Identify modal/dialog components.
10. Identify existing responsive CSS.

Then create a responsiveness audit.

For each screen/component record:

- Current problem
- Cause
- Recommended fix
- Component/file
- Breakpoint behavior

Then implement the fixes.

==================================================
25. DO NOT REWRITE THE APPLICATION
==================================================

Do NOT rebuild the UI from scratch.

Do NOT replace working components just because they are not perfectly responsive.

Make the smallest clean architectural changes necessary.

Preserve the existing visual design and functionality.

==================================================
26. TESTING REQUIREMENT
==================================================

After implementation, test every major screen at:

1920×1080
1600×900
1440×900
1366×768
1280×800
1280×720
1024×768

Also resize continuously between these sizes.

Check specifically:

[ ] Header
[ ] Ribbon
[ ] Ribbon groups
[ ] Sidebar
[ ] Main canvas
[ ] Charts
[ ] Heatmaps
[ ] Tables
[ ] Forms
[ ] Modals
[ ] Dropdowns
[ ] Tooltips
[ ] Status bar
[ ] Settings
[ ] Statistics
[ ] Project screens
[ ] Prediction screens
[ ] Simulation screens

==================================================
27. ACCEPTANCE CRITERIA
==================================================

The task is NOT complete until:

1. Every screen can resize without visual breakage.
2. No important control is clipped.
3. No unintended global horizontal scrollbar exists.
4. Ribbon remains usable at every supported width.
5. Sidebar behaves correctly at smaller widths.
6. Main canvas always receives available space.
7. Charts resize correctly.
8. Tables remain usable.
9. Modals fit within the viewport.
10. Typography remains readable.
11. ATOMIK branding remains unchanged.
12. Existing functionality remains unchanged.
13. Keyboard shortcuts continue working.
14. Mouse/canvas interactions continue working.
15. No console errors are introduced.
16. No unnecessary duplicate responsive CSS is created.

==================================================
FINAL REQUIREMENT
==================================================

Do not simply tell me what should be done.

Actually inspect the existing implementation and implement the responsiveness.

After implementation, give me a concise report:

1. Screens audited
2. Components changed
3. Responsive breakpoints used
4. Major responsiveness fixes
5. Any remaining limitations
6. Files modified

Do not modify simulation/prediction logic.
This task is strictly about making the entire UI responsive and professional.