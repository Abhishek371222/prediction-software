#pragma once
#include <JuceHeader.h>
#include <vector>

// ---------------------------------------------------------------------------
// CommandRegistry — single source of truth for canonical CAD commands + aliases.
// Aliases resolve to the same canonical verb the host already handles.
// Unsupported AutoCAD commands are intentionally omitted (not registered).
// ---------------------------------------------------------------------------
namespace CommandRegistry
{
    struct Entry
    {
        const char* canonical;          // uppercase display / lowercase match key
        const char* const* aliases;     // nullptr-terminated list (may be empty)
        const char* description;
        bool enterRepeats = false;      // safe to re-run on bare Enter after completion
    };

    inline juce::String normalize (juce::String s)
    {
        return s.trim().toUpperCase();
    }

    // Canonical keys are stored UPPERCASE; host handlers use lowercase.
    inline juce::String toHandlerKey (const juce::String& canonicalUpper)
    {
        return canonicalUpper.toLowerCase();
    }

    inline const Entry* table (int& count)
    {
        static const char* aLine[]    = { "L", nullptr };
        static const char* aPline[]   = { "PL", nullptr };
        static const char* aCircle[]  = { "C", nullptr };
        static const char* aArc[]     = { "A", nullptr };
        static const char* aRect[]    = { "REC", "RECT", "RECTANGLE", nullptr };
        static const char* aText[]    = { "T", "TEXTBOX", nullptr };
        static const char* aSquare[]  = { "SQ", nullptr };
        static const char* aErase[]   = { "E", "DELETE", nullptr };
        static const char* aMove[]    = { "M", nullptr };
        static const char* aCut[]     = { "CT", nullptr };
        static const char* aPaste[]   = { "PA", nullptr };
        static const char* aDist[]    = { "DI", "RULER", "MEASURE", nullptr };
        static const char* aPan[]     = { "P", nullptr };
        static const char* aUndo[]    = { "U", nullptr };
        static const char* aZoom[]    = { "Z", nullptr };
        static const char* aFit[]     = { "EXTENTS", nullptr };
        static const char* aZoomIn[]  = { "ZI", nullptr };
        static const char* aZoomOut[] = { "ZO", nullptr };
        static const char* aSelect[]  = { "SE", nullptr };
        static const char* aSave[]    = { "QSAVE", nullptr };
        static const char* aCls[]     = { "CLEAR", nullptr };
        static const char* aHelp[]    = { "?", "H", nullptr };
        static const char* aGrid[]    = { "G", nullptr };
        static const char* aSnap[]    = { "SN", nullptr };
        static const char* aCoords[]  = { "COORD", "COORDINATES", "SPL", nullptr };
        static const char* aOrtho[]   = { "OR", nullptr };
        static const char* aRun[]     = { "SIM", nullptr };
        static const char* aViewSpl[] = { "VIEW", "VS", nullptr };
        static const char* aViewDir[] = { "VD", nullptr };
        static const char* aViewMeas[]= { "VM", nullptr };
        static const char* aPencil[]  = { "PE", "PEN", nullptr };
        static const char* aEraser[]  = { "ER", nullptr };
        static const char* aColor[]   = { "COL", nullptr };
        static const char* aOpacity[] = { "OP", nullptr };
        static const char* aAddMic[]  = { "MIC", nullptr };
        static const char* aAddSpk[]  = { "SPK", "SP", "SPEAKER", nullptr };
        static const char* aCopy[]    = { nullptr }; // clipboard COPY — no CO (AutoCAD displace COPY N/A)
        static const char* aNone[]    = { nullptr };

        static const Entry kTable[] = {
            // DRAW (AutoCAD)
            { "LINE",    aLine,   "Draw a line (2 points)", true },
            { "PLINE",   aPline,  "Draw a polyline", true },
            { "CIRCLE",  aCircle, "Draw a circle (center + rim)", true },
            { "ARC",     aArc,    "Draw an arc (3 points)", true },
            { "RECTANG", aRect,   "Draw a rectangle (2 corners)", true },
            { "TEXT",    aText,   "Place a text box", true },
            { "SQUARE",  aSquare, "Draw a square (custom)", true },

            // MODIFY
            { "ERASE",   aErase,  "Delete selection", false },
            { "MOVE",    aMove,   "Move selection by two points", true },
            { "COPY",    aCopy,   "Copy selection to clipboard", false },
            { "CUT",     aCut,    "Cut selection to clipboard", false },
            { "PASTE",   aPaste,  "Paste clipboard", false },

            // MEASURE / VIEW
            { "DIST",    aDist,   "Measure distance (ruler)", true },
            { "PAN",     aPan,    "Pan tool", false },
            { "ZOOM",    aZoom,   "Zoom [Extents/In/Out]", false },
            { "FIT",     aFit,    "Fit / zoom extents", false },
            { "ZOOMIN",  aZoomIn, "Zoom in", false },
            { "ZOOMOUT", aZoomOut,"Zoom out", false },
            { "SELECT",  aSelect, "Select tool", false },

            // UNDO / FILE
            { "UNDO",    aUndo,   "Undo last edit", false },
            { "REDO",    aNone,   "Redo", false },
            { "OPEN",    aNone,   "Open project", false },
            { "SAVE",    aSave,   "Save project", false },
            { "SAVEAS",  aNone,   "Save project as", false },

            // APP
            { "CLS",     aCls,    "Clear terminal", false },
            { "HELP",    aHelp,   "List commands", false },
            { "CANCEL",  aNone,   "Cancel current command", false },
            { "GRID",    aGrid,   "Toggle grid", false },
            { "SNAP",    aSnap,   "Toggle snap", false },
            { "COORDS",  aCoords, "Toggle coordinate / SPL probe", false },
            { "ORTHO",   aOrtho,  "Toggle ortho", false },
            { "RUN",     aRun,    "Run simulation", false },
            { "VIEWSPL", aViewSpl,"SPL gradient plot view", false },
            { "VIEWDIR", aViewDir,"Directivity view", false },
            { "VIEWMEAS",aViewMeas,"Measured polar view", false },
            { "PENCIL",  aPencil, "Freehand pencil", false },
            { "ERASER",  aEraser, "Eraser tool", false },
            { "COLOR",   aColor,  "Draw colour picker", false },
            { "COLOUR",  aColor,  "Draw colour picker", false },
            { "OPACITY", aOpacity,"Set fill opacity 0–100", false },
            { "ADDMIC",  aAddMic, "Arm mic place (or ADDMIC x,y)", false },
            { "ADDSPEAKER", aAddSpk,"Arm speaker place (or SPK x,y)", false },
        };

        count = (int) (sizeof (kTable) / sizeof (kTable[0]));
        return kTable;
    }

    /** Resolve user token (any case) → canonical UPPERCASE, or empty if unknown. */
    inline juce::String resolve (juce::String token)
    {
        token = normalize (token);
        if (token.isEmpty())
            return {};

        int n = 0;
        const Entry* t = table (n);
        for (int i = 0; i < n; ++i)
        {
            if (token == t[i].canonical)
                return t[i].canonical;
            if (t[i].aliases != nullptr)
                for (auto* a = t[i].aliases; *a != nullptr; ++a)
                    if (token == *a)
                        return t[i].canonical;
        }
        return {};
    }

    inline bool isKnown (const juce::String& token)
    {
        return resolve (token).isNotEmpty();
    }

    inline bool enterRepeats (const juce::String& canonicalUpper)
    {
        int n = 0;
        const Entry* t = table (n);
        for (int i = 0; i < n; ++i)
            if (canonicalUpper == t[i].canonical)
                return t[i].enterRepeats;
        return false;
    }

    inline juce::String helpText()
    {
        juce::String s;
        s << "Atomik Acoustic Simulation Engine — command line\n"
          << "Short form and full name run the same command.\n"
          << "\n"
          << "DRAW\n"
          << "  L    LINE       PL   PLINE      C    CIRCLE\n"
          << "  A    ARC        REC  RECTANG    T    TEXT\n"
          << "  SQ   SQUARE     PE   PENCIL     ER   ERASER\n"
          << "\n"
          << "MODIFY\n"
          << "  E    ERASE      M    MOVE\n"
          << "  CT   CUT        PA   PASTE      COPY  (clipboard)\n"
          << "\n"
          << "MEASURE / VIEW\n"
          << "  DI   DIST       P    PAN        Z    ZOOM  [E|I|O]\n"
          << "  FIT             ZI   ZOOMIN     ZO   ZOOMOUT\n"
          << "  SE   SELECT     VS   VIEWSPL    VD   VIEWDIR    VM   VIEWMEAS\n"
          << "\n"
          << "UNDO / FILE\n"
          << "  U    UNDO       REDO\n"
          << "  OPEN            SAVE            SAVEAS\n"
          << "\n"
          << "APP\n"
          << "  ?    HELP       CLS  CLEAR      CANCEL\n"
          << "  G    GRID       SN   SNAP       OR   ORTHO      SIM  RUN\n"
          << "  COL  COLOR      OP   OPACITY\n"
          << "  MIC  ADDMIC     SPK  ADDSPEAKER   (no args = click to place)\n"
          << "\n"
          << "Tips: points as x,y (metres). Esc cancels. Enter finishes or repeats when safe.\n";
        return s;
    }
}
