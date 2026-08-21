#pragma once
#include <JuceHeader.h>
#include "AcousticEngine.h"
#include "MeasurementData.h"
#include "LayoutLayer.h"
#include <vector>
#include <functional>

// ---------------------------------------------------------------------------
// RadiationPatternComponent — 2D world renderer.
//
// Draws the simulated field (SPL / pressure / interference) or the far-field
// directivity polar plot over a 30 x 30 m world, with zoom & pan, a grid
// overlay, and interactive speaker markers (click to select, drag to move).
// Field recomputation is owned by MainComponent; this component only scales
// the cached image, so zoom/pan never trigger a recompute.
// ---------------------------------------------------------------------------
class RadiationPatternComponent : public juce::Component
{
public:
    RadiationPatternComponent();

    void updateData (const SimResult& result, const SimParams& params);
    void setSpeakers (const std::vector<Speaker>& speakers, int selectedIndex);
    void setMeasuredData (const MeasuredSet& measured);
    void setMeasuredFrequency (int hz);
    void setMeasuredDistance (float distanceM);

    void setShowGrid (bool b) { showGrid_ = b; repaint(); }
    bool showGrid() const noexcept { return showGrid_; }

    void setShowDistanceRings (bool b) { showDistanceRings_ = b; repaint(); }
    bool showDistanceRings() const noexcept { return showDistanceRings_; }

    // Imported reference layout (image / DXF) drawn under the field. The
    // component holds a pointer to the shared layer owned by MainComponent.
    void setLayoutLayer (LayoutLayer* layer) { layout_ = layer; repaint(); }
    void setLayoutEditMode (bool b) { layoutEditMode_ = b; repaint(); }
    void setLayoutSnap (bool b) { layoutSnap_ = b; }

    std::function<void()> onLayoutMoved;   // fired after the layer is dragged

    void resetView();
    void zoomIn();
    void zoomOut();
    void refreshView() { repaint(); }

    // Plot toolbar tools.
    enum class Tool { Select, Pan, Pencil, Eraser, Ruler, Line, Rectangle, Square, Circle };
    void setTool (Tool t);
    Tool getTool() const noexcept { return tool_; }

    void setDrawColour (juce::Colour c);
    juce::Colour getDrawColour() const noexcept { return drawColour_; }
    void setDrawFillAlpha (float a01);   // 0 = invisible fill … 1 = opaque
    float getDrawFillAlpha() const noexcept { return drawFillAlpha_; }
    void clearAnnotations();

    struct Annotation
    {
        enum class Kind { Freehand, Line, Measure, Rectangle, Square, Circle };
        Kind kind = Kind::Freehand;
        juce::Colour colour { 0xffffcc00 };
        float fillAlpha = 0.35f;          // shape fill opacity (0…1)
        float thicknessPx = 2.2f;
        std::vector<juce::Point<float>> worldPts;   // metres; locked to map
    };

    std::vector<Annotation> getAnnotations() const { return annotations_; }
    void setAnnotations (std::vector<Annotation> a);

    std::function<void(int)>               onSpeakerSelected;   // index (-1 = none)
    std::function<void(int, float, float)> onSpeakerMoved;      // index, x, y (m)
    std::function<void(Tool)>              onToolChanged;       // toolbar sync
    std::function<void()>                  onWillEdit;          // before draw / erase / drag
    std::function<void()>                  onEditCommitted;     // after gesture completes
    std::function<bool(const juce::KeyPress&)> onKeyPressed;    // forward shortcuts (undo/redo)

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

    void mouseDown      (const juce::MouseEvent&) override;
    void mouseDrag      (const juce::MouseEvent&) override;
    void mouseUp        (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMove      (const juce::MouseEvent&) override;
    void mouseExit      (const juce::MouseEvent&) override;

private:
    void buildImage();
    void fitView();
    void clampViewToField();   // never show empty space around the heatmap

    juce::Rectangle<int> plotArea() const;     // component minus colour bar
    float worldScaleX() const;                 // pixels per metre (X)
    float worldScaleY() const;                 // pixels per metre (Y)
    float worldScale() const;                  // average — for marker / hit sizes
    juce::Point<float> worldToScreen (float wx, float wy) const;
    juce::Point<float> screenToWorld (float sx, float sy) const;
    int  speakerHitTest (juce::Point<float> screenPt) const;
    bool canAnnotate() const noexcept;
    void eraseNear (juce::Point<float> worldPt, float radiusM);
    static float distPointToSegment (juce::Point<float> p,
                                     juce::Point<float> a,
                                     juce::Point<float> b) noexcept;

    void drawField     (juce::Graphics&, juce::Rectangle<int> bounds);
    void drawLayout    (juce::Graphics&, juce::Rectangle<int> bounds);
    void drawGrid      (juce::Graphics&, juce::Rectangle<int> bounds);
    static double niceStep (double rawMetres);
    struct GridMetrics { double minor = 1.0; double major = 10.0; };
    GridMetrics currentGridMetrics() const;   // adaptive; finest minor = 1 mm
    static juce::String formatGridLabel (double metres);
    void drawSpeakers  (juce::Graphics&, juce::Rectangle<int> bounds);
    void drawColourbar (juce::Graphics&, juce::Rectangle<int> bounds);
    void drawPolarPlot (juce::Graphics&, juce::Rectangle<int> bounds);
    void drawMeasuredPolar (juce::Graphics&, juce::Rectangle<int> bounds);
    void drawAnnotations (juce::Graphics&, juce::Rectangle<int> bounds);
    const MeasuredFreq* measuredForHz (int hz) const;
    bool showingBemHeatmap() const noexcept;

    SimResult           result_;
    SimParams           params_;
    std::vector<Speaker> speakers_;
    int                 selected_ = -1;
    juce::Image         fieldImage_;
    bool                hasData_ = false;
    MeasuredSet         measured_;
    int                 measuredHz_ = 80;
    float               measuredDistanceM_ = 0.5f;
    bool                showGrid_ = true;
    bool                showDistanceRings_ = true;
    LayoutLayer*        layout_ = nullptr;   // not owned
    bool                layoutEditMode_ = false;
    bool                layoutSnap_ = false;

    // View transform — uniform px/m (X == Y) so metre grid cells stay square.
    // Fit View covers the plot (may crop one axis); zoom keeps aspect locked.
    float              baseScaleX_ = 1.0f;      // px/m (same as Y)
    float              baseScaleY_ = 1.0f;      // px/m (same as X)
    float              zoom_       = 1.0f;
    juce::Point<float> origin_     { 0.0f, 0.0f }; // px offset of world top-left
    bool               viewInit_   = false;

    // Interaction state
    enum class Drag { None, Pan, Speaker, Layer, Pencil, Erase, Shape } drag_ = Drag::None;
    Tool               tool_ = Tool::Select;
    int                draggedSpeaker_ = -1;
    juce::Point<float> lastMouse_;
    void updateMouseCursorForTool();

    // User annotations (pencil / line / ruler / shapes) in world metres.
    std::vector<Annotation> annotations_;
    juce::Colour            drawColour_ { 0xffffcc00 };
    float                   drawFillAlpha_ = 0.35f;
    bool                    pendingAnchor_ = false;   // line/ruler: waiting for 2nd click
    juce::Point<float>      pendingStartWorld_ { 0, 0 };
    juce::Point<float>      hoverWorld_ { 0, 0 };
    bool                    hoverValid_ = false;
    juce::Point<float>      shapeStartWorld_ { 0, 0 };
    juce::Point<float>      shapeEndWorld_ { 0, 0 };
    bool                    shapeDragging_ = false;

    static juce::Rectangle<float> normalisedShapeRect (juce::Point<float> a,
                                                       juce::Point<float> b,
                                                       Annotation::Kind kind) noexcept;
    static bool pointHitsShape (juce::Point<float> worldPt,
                                const Annotation& a,
                                float radiusM) noexcept;
    void drawShapeAnnotation (juce::Graphics& g, const Annotation& a, float alphaMul = 1.0f);

    static constexpr int   kColourbarW = 86;
    static constexpr float kMaxZoom = 2000.0f;     // enough to resolve 1 mm cells
    static constexpr double kMinGridM = 0.001;     // 1 mm — finest grid box

    // Q21S plan footprint helpers (true product size W×D).
    juce::Rectangle<float> speakerFootprintScreen (const Speaker& spk) const;
    juce::Rectangle<float> speakerFootprintWorld  (const Speaker& spk) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RadiationPatternComponent)
};
