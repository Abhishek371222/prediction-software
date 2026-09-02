#pragma once
#include <JuceHeader.h>
#include "AcousticEngine.h"
#include "MeasurementData.h"
#include "LayoutLayer.h"
#include "MicReceiver.h"
#include <vector>
#include <functional>

// ---------------------------------------------------------------------------
// RadiationPatternComponent — 2D world renderer + CAD-style draw overlays.
//
// Annotations use Shape → Construction → Modifiers (AutoCAD-like). Stored in
// world metres (SPL) or normalised polar-plot space (Directivity / Measured).
// ---------------------------------------------------------------------------
class RadiationPatternComponent : public juce::Component
{
public:
    RadiationPatternComponent();

    void updateData (const SimResult& result, const SimParams& params);
    void setSpeakers (const std::vector<Speaker>& speakers, int selectedIndex);
    void selectOnlySpeaker (int index);
    void setMeasuredData (const MeasuredSet& measured);
    void setMeasuredFrequency (int hz);
    void setMeasuredDistance (float distanceM);

    void setShowGrid (bool b) { showGrid_ = b; repaint(); }
    bool showGrid() const noexcept { return showGrid_; }

    void setShowDistanceRings (bool b) { showDistanceRings_ = b; repaint(); }
    bool showDistanceRings() const noexcept { return showDistanceRings_; }
    void setShowMicDegrees (bool b) { showMicDegrees_ = b; repaint(); }
    bool showMicDegrees() const noexcept { return showMicDegrees_; }

    void setLayoutLayer (LayoutLayer* layer) { layout_ = layer; repaint(); }
    void setLayoutEditMode (bool b) { layoutEditMode_ = b; repaint(); }
    void setLayoutSnap (bool b) { layoutSnap_ = b; }

    std::function<void()> onLayoutMoved;

    void resetView();
    void zoomIn();
    void zoomOut();
    void refreshView() { repaint(); }

    // Plot toolbar tools (Shape is a family; construction chosen separately).
    enum class Tool { Select, Pan, Pencil, Eraser, Ruler, Shape };
    enum class DrawShape { Line, Polyline, Circle, Arc, Rectangle, Square, TextBox };
    enum class Construction
    {
        LineTwoPoints,
        LineOrtho,
        PolylinePoints,
        PolylineClosed,
        CircleCenterRadius,
        CircleTwoPoints,
        ArcThreePoints,
        RectTwoCorners,
        SquareTwoCorners,
        TextBoxTwoCorners
    };

    void setTool (Tool t);
    Tool getTool() const noexcept { return tool_; }

    void setDrawShape (DrawShape s, Construction c);
    DrawShape getDrawShape() const noexcept { return drawShape_; }
    Construction getConstruction() const noexcept { return construction_; }

    void setOrtho (bool on);
    bool getOrtho() const noexcept { return ortho_; }
    enum class OrthoAlign { Horizontal, Vertical };
    void setOrthoAlign (OrthoAlign a);
    OrthoAlign getOrthoAlign() const noexcept { return orthoAlign_; }
    void setOrthoSpacingM (float metres);
    float getOrthoSpacingM() const noexcept { return orthoSpacingM_; }
    /** Average centre-to-centre gap of the current speaker selection (0 if < 2). */
    float measureOrthoSpacingM() const;
    /** Update stored gap from selection without moving speakers. */
    void syncOrthoSpacingFromSelection();
    /** Align + distribute selected speakers when Ortho is on (≥ 2 selected). */
    bool applyOrthoSpeakerLayout();
    void setDrawGridSnap (bool on);
    bool getDrawGridSnap() const noexcept { return drawGridSnap_; }
    void setShowSplProbe (bool on);
    bool getShowSplProbe() const noexcept { return showSplProbe_; }

    /** Commit numeric length/radius/angle for the current rubber-band step. */
    bool commitNumericValue (double value);

    juce::String getDrawPrompt() const;

    void setDrawColour (juce::Colour c);
    juce::Colour getDrawColour() const noexcept { return drawColour_; }
    /** Colour of the selected annotation, or the draw brush if none selected. */
    juce::Colour getActiveDrawColour() const noexcept;
    void setDrawFillAlpha (float a01);
    float getDrawFillAlpha() const noexcept { return drawFillAlpha_; }
    int  getSelectedAnnotation() const noexcept { return selectedAnnot_; }
    /** Fill alpha of the selected filled shape, or draw brush alpha if none. */
    float getActiveFillAlpha() const noexcept;
    void clearAnnotations();
    void cancelDrawSession();
    void clearMics();

    // Virtual mics (MIC.md)
    void setAddMicArmed (bool armed);
    bool isAddMicArmed() const noexcept { return addMicArmed_; }
    std::vector<MicReceiver> getMics() const { return mics_; }
    void setMics (std::vector<MicReceiver> m);
    int  getSelectedMic() const noexcept { return selectedMic_; }
    void setSelectedMic (int index);
    bool placeMicAtWorld (float wx, float wy);
    /** Indices of speakers currently selected on the plot (may be empty). */
    std::vector<int> getSelectedSpeakers() const { return selectedSpeakers_; }
    bool placeMicOnRing (int micIndex, int speakerIndex, float radiusM);
    void refreshMicLevels();

    enum class AnnotSpace { World, PolarPlot };

    struct Annotation
    {
        enum class Kind { Freehand, Line, Measure, Rectangle, Square, Circle, Polyline, Arc, TextBox };
        Kind kind = Kind::Freehand;
        Construction construction = Construction::LineTwoPoints;
        AnnotSpace space = AnnotSpace::World;
        juce::Colour colour { 0xffffcc00 };
        float fillAlpha = 0.35f;
        float thicknessPx = 2.2f;
        bool closed = false;                          // polyline
        std::vector<juce::Point<float>> pts;          // world m or polar-normalised
        juce::String text;                            // TextBox label
        float rotationDeg = 0.0f;                     // TextBox rotation (CCW degrees)
    };

    std::vector<Annotation> getAnnotations() const { return annotations_; }
    void setAnnotations (std::vector<Annotation> a);

    bool hasCopyableSelection() const noexcept;
    bool hasClipboardContent() const noexcept;
    bool copySelection();
    bool pasteClipboard();
    bool deleteSelection();
    void showSelectionContextMenu (juce::Point<int> screenPos);

    std::function<void(int)>               onSpeakerSelected;
    std::function<void(int, float, float)> onSpeakerMoved;
    /** Paste speakers into the scene; return their new indices. */
    std::function<std::vector<int>(std::vector<Speaker>)> onPasteSpeakers;
    /** Remove speakers by index (e.g. Delete / context menu). */
    std::function<void(std::vector<int>)> onDeleteSpeakers;
    std::function<void(Tool)>              onToolChanged;
    std::function<void()>                  onDrawPromptChanged;  // toolbar status
    std::function<void()>                  onWillEdit;
    std::function<void()>                  onEditCommitted;
    std::function<void()>                  onAnnotSelectionChanged; // opacity slider sync
    std::function<void()>                  onMicsChanged;
    std::function<void()>                  onAddMicArmedChanged;
    std::function<bool(const juce::KeyPress&)> onKeyPressed;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

    void mouseDown      (const juce::MouseEvent&) override;
    void mouseDrag      (const juce::MouseEvent&) override;
    void mouseUp        (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMove      (const juce::MouseEvent&) override;
    void mouseExit      (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    void buildImage();
    void fitView();
    void clampViewToField();

    juce::Rectangle<int> plotArea() const;
    float worldScaleX() const;
    float worldScaleY() const;
    float worldScale() const;
    juce::Point<float> worldToScreen (float wx, float wy) const;
    juce::Point<float> screenToWorld (float sx, float sy) const;

    // Polar-plot normalised space: (0,0)=centre, radius 1 = outer ring.
    juce::Point<float> polarNormToScreen (float nx, float ny) const;
    juce::Point<float> screenToPolarNorm (float sx, float sy) const;
    juce::Point<float> annotateToScreen (const Annotation& a, juce::Point<float> p) const;
    AnnotSpace currentAnnotSpace() const noexcept;
    juce::Point<float> screenToAnnot (float sx, float sy) const;
    juce::Point<float> snapAnnotPoint (juce::Point<float> p) const;
    /** Grid + object snap. ignoreSelected skips currently selected items (for move).
        If objectHit != nullptr, set true when an edge/corner/centre object axis latched. */
    juce::Point<float> snapAnnotPointFull (juce::Point<float> p, bool ignoreSelected,
                                           bool* objectHit = nullptr) const;
    /** Play tak only when object/shape/line snap newly latches (not on grid steps). */
    void noteSnapSound (bool objectSnapEngaged,
                        juce::Point<float> raw,
                        juce::Point<float> snappedPos);
    void resetSnapSoundState() noexcept;
    static float snapScalar (float v, const std::vector<float>& candidates, float tol,
                             bool& hit) noexcept;
    void collectObjectSnapAxes (std::vector<float>& xs, std::vector<float>& ys,
                                bool ignoreSelected) const;
    /** Shape L/R/T/B only (no centres / speakers / mics) — used for snap tak. */
    void collectAnnotationEdgeAxes (std::vector<float>& xs, std::vector<float>& ys,
                                    bool ignoreSelected) const;
    /** True when sel shares an edge (or aligned edge) with another annotation. */
    bool selectionMeetsOtherAnnotation (juce::Rectangle<float> sel) const;
    /** True when sel footprint shares an edge with another speaker cabinet. */
    bool selectionMeetsOtherSpeaker (juce::Rectangle<float> sel) const;
    juce::Rectangle<float> annotationAnnotBounds (const Annotation& a) const;
    juce::Rectangle<float> selectionAnnotBounds() const;
    /** Union of selected annotation + speaker footprints (for edge snap while moving). */
    juce::Rectangle<float> selectionMoveBounds() const;
    juce::Point<float> selectionSnapReference() const;
    juce::Point<float> applyOrtho (juce::Point<float> from, juce::Point<float> to) const;

    int  speakerHitTest (juce::Point<float> screenPt) const;
    bool canAnnotate() const noexcept;
    void eraseNear (juce::Point<float> annotPt, float radius);
    static float distPointToSegment (juce::Point<float> p,
                                     juce::Point<float> a,
                                     juce::Point<float> b) noexcept;

    void drawField     (juce::Graphics&, juce::Rectangle<int> bounds);
    void drawLayout    (juce::Graphics&, juce::Rectangle<int> bounds);
    void drawGrid      (juce::Graphics&, juce::Rectangle<int> bounds);
    static double niceStep (double rawMetres);
    struct GridMetrics { double minor = 1.0; double major = 10.0; };
    GridMetrics currentGridMetrics() const;
    static juce::String formatGridLabel (double metres);
    void drawSpeakers  (juce::Graphics&, juce::Rectangle<int> bounds);
    void drawOrthoSpacingOverlay (juce::Graphics&);
    void drawColourbar (juce::Graphics&, juce::Rectangle<int> bounds);
    void drawPolarPlot (juce::Graphics&, juce::Rectangle<int> bounds);
    void drawMeasuredPolar (juce::Graphics&, juce::Rectangle<int> bounds);
    void drawAnnotations (juce::Graphics&, juce::Rectangle<int> bounds);
    void drawSplProbe (juce::Graphics& g);
    bool updateSplProbeAt (juce::Point<float> screenPt);
    bool sampleSplAtWorld (float wx, float wy, float& absDb, float& relDb) const noexcept;
    void updateRubberBandAt (juce::Point<float> screenPt);
    bool tryFinishRubberBandAt (juce::Point<float> screenPt);
    const MeasuredFreq* measuredForHz (int hz) const;
    bool showingBemHeatmap() const noexcept;

    // --- Draw session (Shape / Construction) -----------------------------
    void beginDrawSession();
    void resetDrawSession();
    void updateDrawPrompt();
    int  pointsNeeded() const noexcept;
    bool acceptAnnotPoint (juce::Point<float> p);
    bool finishPolyline (bool forceClose);
    void commitAnnotation (Annotation a);
    static bool circleFrom3Points (juce::Point<float> p1, juce::Point<float> p2,
                                   juce::Point<float> p3,
                                   juce::Point<float>& centre, float& radius) noexcept;

    static juce::Rectangle<float> normalisedShapeRect (juce::Point<float> a,
                                                       juce::Point<float> b,
                                                       Annotation::Kind kind) noexcept;
    static bool pointHitsShape (juce::Point<float> pt,
                                const Annotation& a,
                                float radius) noexcept;
    static bool isFilledShapeKind (Annotation::Kind k) noexcept;
    int  annotationHitTest (juce::Point<float> annotPt, float radius) const;
    void setSelectedAnnotation (int index);
    void clearPlotSelection();
    bool isAnnotationSelected (int index) const;
    bool isMicSelected (int index) const;
    bool isSpeakerSelected (int index) const;
    void syncPrimarySelectionFromSets();
    void moveSelectedAnnotationBy (juce::Point<float> deltaAnnot);
    void moveSelectionBy (juce::Point<float> deltaAnnot, juce::Point<float> deltaWorld);
    void drawSelectionOverlay (juce::Graphics& g, const Annotation& a);
    static std::vector<juce::Point<float>> resizeHandlesFor (const Annotation& a);
    int  resizeHandleHitTest (const Annotation& a, juce::Point<float> annotPt,
                              float radius) const;
    void applyAnnotationResize (Annotation& a, int handleIndex,
                                juce::Point<float> annotPt);
    juce::Rectangle<float> annotationScreenBounds (const Annotation& a) const;
    juce::Rectangle<float> currentMarqueeScreen() const;
    void applyMarqueeSelection (bool addToExisting);
    void drawMarqueeOverlay (juce::Graphics& g);
    void drawMics (juce::Graphics& g);
    int  micHitTest (juce::Point<float> worldPt, float radiusM) const;
    int  micHitTestScreen (juce::Point<float> screenPt) const;
    juce::String micLabelText (const MicReceiver& m) const;
    void snapMicWorld (float& wx, float& wy, bool playSoundIfNewClip);
    void beginMicDrag (int micIndex, juce::Point<float> screenPos);
    void drawShapeAnnotation (juce::Graphics& g, const Annotation& a, float alphaMul = 1.0f);
    void drawArcAnnotation (juce::Graphics& g, const Annotation& a, float alphaMul = 1.0f);
    void drawTextBoxAnnotation (juce::Graphics& g, const Annotation& a, float alphaMul = 1.0f);
    void promptEditTextBox (int index);
    static juce::Point<float> rotateAround (juce::Point<float> p, juce::Point<float> c, float deg) noexcept;
    static juce::Rectangle<float> textBoxLocalRect (const Annotation& a) noexcept;
    bool pointHitsTextBox (juce::Point<float> pt, const Annotation& a, float radius) const noexcept;
    static juce::String formatLengthLabel (float metres);
    void drawPendingDimLabel (juce::Graphics& g,
                              juce::Point<float> screenMid,
                              const juce::String& text);
    void updateMouseCursorForTool();

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
    bool                showDistanceRings_ = false;
    bool                showMicDegrees_ = false;
    LayoutLayer*        layout_ = nullptr;
    bool                layoutEditMode_ = false;
    bool                layoutSnap_ = false;

    float              baseScaleX_ = 1.0f;
    float              baseScaleY_ = 1.0f;
    float              zoom_       = 1.0f;
    juce::Point<float> origin_     { 0.0f, 0.0f };
    bool               viewInit_   = false;

    // Cached polar frame for annotation mapping (updated each polar paint).
    float polarCx_ = 0.0f, polarCy_ = 0.0f, polarRadius_ = 1.0f;
    bool  polarFrameValid_ = false;

    enum class Drag { None, Pan, Speaker, Layer, Pencil, Erase, RubberBand,
                      Annot, AnnotResize, AnnotRotate, SelectionMove, Marquee, Mic } drag_ = Drag::None;
    Tool               tool_ = Tool::Select;
    DrawShape          drawShape_ = DrawShape::Line;
    Construction       construction_ = Construction::LineTwoPoints;
    bool               ortho_ = false;
    OrthoAlign         orthoAlign_ = OrthoAlign::Horizontal;
    float              orthoSpacingM_ = 3.0f;
    bool               drawGridSnap_ = false;
    int                draggedSpeaker_ = -1;
    juce::Point<float> lastMouse_;
    juce::Point<float> lastAnnotDrag_ { 0, 0 };
    bool               annotDragMoved_ = false;
    int                resizeHandleIndex_ = -1;
    float              rotateDragStartDeg_ = 0.0f;
    float              rotateDragStartMouseDeg_ = 0.0f;
    juce::Point<float> rotateDragCentre_ { 0, 0 };
    juce::Point<float> marqueeStartScreen_ { 0, 0 };
    juce::Point<float> marqueeEndScreen_ { 0, 0 };
    juce::Point<float> selMoveStartMouse_ { 0, 0 };
    juce::Point<float> selMoveStartRef_ { 0, 0 };
    juce::Rectangle<float> selMoveStartBounds_;
    bool               selMoveRefValid_ = false;
    bool               selMoveHasBounds_ = false;
    bool               snapSoundArmed_ = false;
    int                snapSoundOffFrames_ = 0;
    juce::Point<float> lastSnapSoundPos_ { 0, 0 };

    struct Clipboard
    {
        std::vector<Annotation> annots;
        std::vector<MicReceiver> mics;
        std::vector<Speaker> speakers;
        bool empty() const noexcept
        {
            return annots.empty() && mics.empty() && speakers.empty();
        }
    };
    Clipboard clipboard_;
    int       pasteGeneration_ = 0;

    std::vector<Annotation> annotations_;
    int                     selectedAnnot_ = -1;          // primary (last) for colour/opacity
    std::vector<int>        selectedAnnots_;
    std::vector<int>        selectedMics_;
    std::vector<int>        selectedSpeakers_;
    juce::Colour            drawColour_ { 0xffffcc00 };
    float                   drawFillAlpha_ = 0.35f;

    // Virtual mics
    std::vector<MicReceiver> mics_;
    int                      selectedMic_ = -1;           // primary
    bool                     addMicArmed_ = false;
    int                      nextMicId_ = 1;
    juce::Point<float>       lastMicDragWorld_ { 0, 0 };
    bool                     micDragMoved_ = false;
    bool                     micWasSnapped_ = false;

    // Active multi-click draw session
    bool                    sessionActive_ = false;
    std::vector<juce::Point<float>> sessionPts_;
    juce::Point<float>      hoverAnnot_ { 0, 0 };
    bool                    hoverValid_ = false;
    juce::String            drawPrompt_;
    juce::String            numericBuffer_;

    // Legacy two-click (ruler) still uses these
    bool                    pendingAnchor_ = false;
    juce::Point<float>      pendingStartWorld_ { 0, 0 };
    /** Screen position when a rubber-band (line / shape / ruler) drag began. */
    juce::Point<float>      rubberBandStartScreen_ { 0, 0 };

    // Select-tool SPL readout under the cursor (heatmap probe)
    bool                    showSplProbe_ = false;
    bool                    splProbeValid_ = false;
    juce::Point<float>      splProbeScreen_ { 0, 0 };
    juce::Point<float>      splProbeWorld_ { 0, 0 };
    float                   splProbeAbsDb_ = 0.0f;
    float                   splProbeRelDb_ = 0.0f;

    static constexpr int   kColourbarW = 86;
    static constexpr float kMaxZoom = 2000.0f;
    static constexpr double kMinGridM = 0.001;

    juce::Rectangle<float> speakerFootprintScreen (const Speaker& spk) const;
    juce::Rectangle<float> speakerFootprintWorld  (const Speaker& spk) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RadiationPatternComponent)
};
