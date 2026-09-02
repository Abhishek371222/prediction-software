#include "RadiationPatternComponent.h"
#include "ColourMaps.h"
#include "BrandTheme.h"
#include "AppSettings.h"
#include "MicRingSnap.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

RadiationPatternComponent::RadiationPatternComponent()
{
    setOpaque (true);
    // Need focus after drawing so Ctrl/Cmd+Z/Y reach this component (then MainComponent).
    setWantsKeyboardFocus (true);
    updateMouseCursorForTool();
}

void RadiationPatternComponent::setTool (Tool t)
{
    if (tool_ == t)
    {
        updateDrawPrompt();
        return;
    }
    tool_ = t;
    resetDrawSession();
    pendingAnchor_ = false;
    hoverValid_ = false;
    splProbeValid_ = false;
    if (t != Tool::Select && addMicArmed_)
    {
        addMicArmed_ = false;
        if (onAddMicArmedChanged) onAddMicArmedChanged();
    }
    drag_ = Drag::None;
    updateMouseCursorForTool();
    if (onToolChanged) onToolChanged (tool_);
    updateDrawPrompt();
    repaint();
}

void RadiationPatternComponent::setDrawShape (DrawShape s, Construction c)
{
    drawShape_ = s;
    construction_ = c;
    tool_ = Tool::Shape;
    resetDrawSession();
    pendingAnchor_ = false;
    updateMouseCursorForTool();
    if (onToolChanged) onToolChanged (tool_);
    updateDrawPrompt();
    repaint();
}

void RadiationPatternComponent::setOrtho (bool on)
{
    if (ortho_ == on) return;
    ortho_ = on;
    if (ortho_ && selectedSpeakers_.size() >= 2)
    {
        const float measured = measureOrthoSpacingM();
        if (measured > 1.0e-3f)
            orthoSpacingM_ = measured;
        applyOrthoSpeakerLayout();
    }
    updateDrawPrompt();
    repaint();
}

void RadiationPatternComponent::setOrthoAlign (OrthoAlign a)
{
    if (orthoAlign_ == a) return;
    orthoAlign_ = a;
    if (ortho_)
        applyOrthoSpeakerLayout();
    updateDrawPrompt();
    repaint();
}

void RadiationPatternComponent::setOrthoSpacingM (float metres)
{
    metres = juce::jlimit (0.1f, 50.0f, metres);
    if (std::abs (orthoSpacingM_ - metres) < 1.0e-4f) return;
    orthoSpacingM_ = metres;
    if (ortho_)
        applyOrthoSpeakerLayout();
    repaint();
}

float RadiationPatternComponent::measureOrthoSpacingM() const
{
    if (selectedSpeakers_.size() < 2)
        return 0.0f;

    std::vector<int> idxs = selectedSpeakers_;
    std::sort (idxs.begin(), idxs.end(), [&] (int a, int b)
    {
        const auto& sa = speakers_[(size_t) a];
        const auto& sb = speakers_[(size_t) b];
        if (orthoAlign_ == OrthoAlign::Horizontal)
            return sa.x < sb.x || (sa.x == sb.x && sa.y < sb.y);
        return sa.y < sb.y || (sa.y == sb.y && sa.x < sb.x);
    });

    double sum = 0.0;
    int n = 0;
    for (size_t i = 1; i < idxs.size(); ++i)
    {
        const auto& a = speakers_[(size_t) idxs[i - 1]];
        const auto& b = speakers_[(size_t) idxs[i]];
        const float d = (orthoAlign_ == OrthoAlign::Horizontal)
                            ? std::abs (b.x - a.x)
                            : std::abs (b.y - a.y);
        sum += (double) d;
        ++n;
    }
    return n > 0 ? (float) (sum / (double) n) : 0.0f;
}

void RadiationPatternComponent::syncOrthoSpacingFromSelection()
{
    if (! ortho_) return;
    const float m = measureOrthoSpacingM();
    if (m > 1.0e-3f)
        orthoSpacingM_ = m;
    updateDrawPrompt();
    repaint();
}

bool RadiationPatternComponent::applyOrthoSpeakerLayout()
{
    if (! ortho_ || selectedSpeakers_.size() < 2)
        return false;

    std::vector<int> idxs = selectedSpeakers_;
    // Unique, valid indices
    std::sort (idxs.begin(), idxs.end());
    idxs.erase (std::unique (idxs.begin(), idxs.end()), idxs.end());
    idxs.erase (std::remove_if (idxs.begin(), idxs.end(),
                                [&] (int i) { return i < 0 || i >= (int) speakers_.size(); }),
                idxs.end());
    if (idxs.size() < 2)
        return false;

    std::sort (idxs.begin(), idxs.end(), [&] (int a, int b)
    {
        const auto& sa = speakers_[(size_t) a];
        const auto& sb = speakers_[(size_t) b];
        if (orthoAlign_ == OrthoAlign::Horizontal)
            return sa.x < sb.x || (sa.x == sb.x && sa.y < sb.y);
        return sa.y < sb.y || (sa.y == sb.y && sa.x < sb.x);
    });

    if (onWillEdit) onWillEdit();

    const float gap = juce::jmax (0.1f, orthoSpacingM_);
    const float worldW = hasData_ ? (float) result_.worldW : 100.0f;
    const float worldH = hasData_ ? (float) result_.worldH : 100.0f;

    if (orthoAlign_ == OrthoAlign::Horizontal)
    {
        float sumY = 0.0f;
        for (int i : idxs)
            sumY += speakers_[(size_t) i].y;
        const float y = sumY / (float) idxs.size();
        const float x0 = speakers_[(size_t) idxs.front()].x;

        for (size_t k = 0; k < idxs.size(); ++k)
        {
            auto& s = speakers_[(size_t) idxs[k]];
            s.x = juce::jlimit (0.0f, worldW, x0 + (float) k * gap);
            s.y = juce::jlimit (0.0f, worldH, y);
            if (onSpeakerMoved)
                onSpeakerMoved (idxs[k], s.x, s.y);
        }
    }
    else
    {
        float sumX = 0.0f;
        for (int i : idxs)
            sumX += speakers_[(size_t) i].x;
        const float x = sumX / (float) idxs.size();
        const float y0 = speakers_[(size_t) idxs.front()].y;

        for (size_t k = 0; k < idxs.size(); ++k)
        {
            auto& s = speakers_[(size_t) idxs[k]];
            s.x = juce::jlimit (0.0f, worldW, x);
            s.y = juce::jlimit (0.0f, worldH, y0 + (float) k * gap);
            if (onSpeakerMoved)
                onSpeakerMoved (idxs[k], s.x, s.y);
        }
    }

    if (onEditCommitted) onEditCommitted();
    repaint();
    return true;
}

void RadiationPatternComponent::setDrawGridSnap (bool on)
{
    if (drawGridSnap_ == on) return;
    drawGridSnap_ = on;
    resetSnapSoundState();
    updateDrawPrompt();
}

void RadiationPatternComponent::setShowSplProbe (bool on)
{
    if (showSplProbe_ == on) return;
    showSplProbe_ = on;
    if (! showSplProbe_)
        splProbeValid_ = false;
    repaint();
}

void RadiationPatternComponent::setDrawColour (juce::Colour c)
{
    drawColour_ = c;

    // Recolour every selected drawing.
    bool any = false;
    for (int idx : selectedAnnots_)
    {
        if (idx < 0 || idx >= (int) annotations_.size()) continue;
        auto& a = annotations_[(size_t) idx];
        if (a.colour.getARGB() == c.getARGB()) continue;
        if (! any)
        {
            if (onWillEdit) onWillEdit();
            any = true;
        }
        a.colour = c;
    }
    if (any && onEditCommitted) onEditCommitted();
    repaint();
}

juce::Colour RadiationPatternComponent::getActiveDrawColour() const noexcept
{
    if (selectedAnnot_ >= 0 && selectedAnnot_ < (int) annotations_.size())
        return annotations_[(size_t) selectedAnnot_].colour;
    return drawColour_;
}

void RadiationPatternComponent::setDrawFillAlpha (float a01)
{
    drawFillAlpha_ = juce::jlimit (0.0f, 1.0f, a01);

    // Opacity applies to selected filled shapes only.
    for (int idx : selectedAnnots_)
    {
        if (idx < 0 || idx >= (int) annotations_.size()) continue;
        auto& a = annotations_[(size_t) idx];
        if (isFilledShapeKind (a.kind))
            a.fillAlpha = drawFillAlpha_;
    }
    repaint();
}

float RadiationPatternComponent::getActiveFillAlpha() const noexcept
{
    if (selectedAnnot_ >= 0 && selectedAnnot_ < (int) annotations_.size())
    {
        const auto& a = annotations_[(size_t) selectedAnnot_];
        if (isFilledShapeKind (a.kind))
            return a.fillAlpha;
    }
    return drawFillAlpha_;
}

void RadiationPatternComponent::clearAnnotations()
{
    annotations_.clear();
    selectedAnnots_.clear();
    setSelectedAnnotation (-1);
    resetDrawSession();
    pendingAnchor_ = false;
    hoverValid_ = false;
    numericBuffer_.clear();
    updateDrawPrompt();
    drag_ = Drag::None;
    repaint();
}

void RadiationPatternComponent::clearMics()
{
    mics_.clear();
    selectedMic_ = -1;
    selectedMics_.clear();
    addMicArmed_ = false;
    nextMicId_ = 1;
    if (onAddMicArmedChanged) onAddMicArmedChanged();
    if (onMicsChanged) onMicsChanged();
    updateMouseCursorForTool();
    repaint();
}

void RadiationPatternComponent::setAddMicArmed (bool armed)
{
    if (addMicArmed_ == armed) return;
    addMicArmed_ = armed;
    if (armed)
    {
        setTool (Tool::Select);
        setSelectedAnnotation (-1);
    }
    updateMouseCursorForTool();
    if (onAddMicArmedChanged) onAddMicArmedChanged();
    updateDrawPrompt();
    repaint();
}

void RadiationPatternComponent::setMics (std::vector<MicReceiver> m)
{
    mics_ = std::move (m);
    selectedMic_ = -1;
    selectedMics_.clear();
    nextMicId_ = 1;
    for (const auto& mic : mics_)
        nextMicId_ = juce::jmax (nextMicId_, mic.id + 1);
    refreshMicLevels();
    if (onMicsChanged) onMicsChanged();
    repaint();
}

void RadiationPatternComponent::setSelectedMic (int index)
{
    if (index < 0 || index >= (int) mics_.size())
        index = -1;
    selectedMics_.clear();
    if (index >= 0)
        selectedMics_.push_back (index);
    if (selectedMic_ == index) { repaint(); return; }
    selectedMic_ = index;
    if (onMicsChanged) onMicsChanged();
    repaint();
}

void RadiationPatternComponent::clearPlotSelection()
{
    selectedAnnots_.clear();
    selectedMics_.clear();
    selectedSpeakers_.clear();
    selectedAnnot_ = -1;
    selectedMic_ = -1;
    if (onAnnotSelectionChanged) onAnnotSelectionChanged();
    if (onMicsChanged) onMicsChanged();
    repaint();
}

bool RadiationPatternComponent::isAnnotationSelected (int index) const
{
    return std::find (selectedAnnots_.begin(), selectedAnnots_.end(), index)
        != selectedAnnots_.end();
}

bool RadiationPatternComponent::isMicSelected (int index) const
{
    return std::find (selectedMics_.begin(), selectedMics_.end(), index)
        != selectedMics_.end();
}

bool RadiationPatternComponent::isSpeakerSelected (int index) const
{
    return std::find (selectedSpeakers_.begin(), selectedSpeakers_.end(), index)
        != selectedSpeakers_.end();
}

void RadiationPatternComponent::syncPrimarySelectionFromSets()
{
    selectedAnnot_ = selectedAnnots_.empty() ? -1 : selectedAnnots_.back();
    selectedMic_ = selectedMics_.empty() ? -1 : selectedMics_.back();
    if (selectedAnnot_ >= 0 && selectedAnnot_ < (int) annotations_.size()
        && isFilledShapeKind (annotations_[(size_t) selectedAnnot_].kind))
        drawFillAlpha_ = annotations_[(size_t) selectedAnnot_].fillAlpha;
    if (! selectedSpeakers_.empty())
        selected_ = selectedSpeakers_.back();
}

void RadiationPatternComponent::refreshMicLevels()
{
    for (auto& m : mics_)
    {
        float absDb = 0.0f, relDb = 0.0f;
        m.levelOk = sampleSplAtWorld (m.x, m.y, absDb, relDb);
        if (m.levelOk)
            m.relDb = relDb;
    }
}

void RadiationPatternComponent::snapMicWorld (float& wx, float& wy, bool playSoundIfNewClip)
{
    // Original ring snap: pull onto 1/2/4/8 m; tak only when newly latching.
    const auto snap = MicRingSnap::snapToRing (wx, wy, speakers_);
    const bool nowSnapped = snap.snapped;
    if (nowSnapped)
    {
        wx = snap.x;
        wy = snap.y;
        if (playSoundIfNewClip && ! micWasSnapped_)
            SnapClick::playTak();
    }
    micWasSnapped_ = nowSnapped;
}

void RadiationPatternComponent::beginMicDrag (int micIndex, juce::Point<float> screenPos)
{
    if (micIndex < 0 || micIndex >= (int) mics_.size()) return;
    if (onWillEdit) onWillEdit();
    selectedAnnots_.clear();
    selectedSpeakers_.clear();
    selectedMics_.clear();
    selectedMics_.push_back (micIndex);
    syncPrimarySelectionFromSets();
    drag_ = Drag::Mic;
    auto w = screenToWorld (screenPos.x, screenPos.y);
    lastMicDragWorld_ = w;
    lastMouse_ = screenPos;
    micDragMoved_ = false;
    micWasSnapped_ = mics_[(size_t) micIndex].ringLocked;
    resetSnapSoundState();
}

bool RadiationPatternComponent::placeMicAtWorld (float wx, float wy)
{
    if (! hasData_) return false;
    wx = juce::jlimit (0.0f, (float) result_.worldW, wx);
    wy = juce::jlimit (0.0f, (float) result_.worldH, wy);

    micWasSnapped_ = false;
    float sx = wx, sy = wy;
    snapMicWorld (sx, sy, true);

    if (onWillEdit) onWillEdit();
    MicReceiver m;
    m.id = nextMicId_++;
    m.x = sx;
    m.y = sy;
    if (micWasSnapped_)
    {
        const auto snap = MicRingSnap::snapToRing (sx, sy, speakers_);
        m.ringLocked = snap.snapped;
        m.ringRadiusM = snap.radiusM;
        m.ringSpeaker = snap.speakerIndex;
    }
    mics_.push_back (m);
    selectedMic_ = (int) mics_.size() - 1;
    selectedMics_.clear();
    selectedMics_.push_back (selectedMic_);
    refreshMicLevels();
    if (onEditCommitted) onEditCommitted();
    if (onMicsChanged) onMicsChanged();
    repaint();
    return true;
}

bool RadiationPatternComponent::placeMicOnRing (int micIndex, int speakerIndex, float radiusM)
{
    if (micIndex < 0 || micIndex >= (int) mics_.size()) return false;
    if (speakerIndex < 0 || speakerIndex >= (int) speakers_.size()) return false;
    const auto& spk = speakers_[(size_t) speakerIndex];
    auto& mic = mics_[(size_t) micIndex];
    if (onWillEdit) onWillEdit();
    auto placed = MicRingSnap::placeOnRing (spk, radiusM, mic.x, mic.y);
    mic.x = juce::jlimit (0.0f, (float) result_.worldW, placed.x);
    mic.y = juce::jlimit (0.0f, (float) result_.worldH, placed.y);
    mic.ringLocked = true;
    mic.ringRadiusM = radiusM;
    mic.ringSpeaker = speakerIndex;
    SnapClick::playTak();
    selectedMic_ = micIndex;
    refreshMicLevels();
    if (onEditCommitted) onEditCommitted();
    if (onMicsChanged) onMicsChanged();
    repaint();
    return true;
}

int RadiationPatternComponent::micHitTest (juce::Point<float> worldPt, float radiusM) const
{
    for (int i = (int) mics_.size() - 1; i >= 0; --i)
    {
        const auto& m = mics_[(size_t) i];
        if (worldPt.getDistanceFrom ({ m.x, m.y }) <= radiusM)
            return i;
    }
    return -1;
}

juce::String RadiationPatternComponent::micLabelText (const MicReceiver& m) const
{
    juce::String label = micDisplayName (m);
    if (showMicDegrees_)
    {
        const int si = MicRingSnap::referenceSpeakerIndex (
            m.x, m.y, speakers_, m.ringSpeaker);
        if (si >= 0)
        {
            const int deg = (int) std::lround (
                MicRingSnap::angleDegFromSpeaker (m.x, m.y, speakers_[(size_t) si]));
            label += "  " + juce::String (deg)
                   + juce::String::fromUTF8 ("\xc2\xb0");
        }
    }
    if (m.levelOk)
        label += "  " + juce::String (m.relDb, 1) + " dB";
    return label;
}

int RadiationPatternComponent::micHitTestScreen (juce::Point<float> screenPt) const
{
    // Hit the drawn glyph + label (screen px), not a tiny world-metre radius.
    for (int i = (int) mics_.size() - 1; i >= 0; --i)
    {
        const auto& m = mics_[(size_t) i];
        const auto s = worldToScreen (m.x, m.y);
        const auto glyph = juce::Rectangle<float> (s.x - 14.0f, s.y - 16.0f, 28.0f, 32.0f);
        if (glyph.contains (screenPt))
            return i;

        const juce::String label = micLabelText (m);
        const float tw = juce::jmax (40.0f, (float) label.length() * 7.0f + 12.0f);
        const auto box = juce::Rectangle<float> (s.x + 8.0f, s.y - 12.0f, tw, 20.0f);
        if (box.contains (screenPt))
            return i;
    }
    return -1;
}

void RadiationPatternComponent::drawMics (juce::Graphics& g)
{
    if (mics_.empty()) return;
    for (int i = 0; i < (int) mics_.size(); ++i)
    {
        const auto& m = mics_[(size_t) i];
        auto s = worldToScreen (m.x, m.y);
        const bool sel = isMicSelected (i);
        g.setColour (sel ? Brand::accent() : Brand::white());
        // Simple mic glyph: capsule + stand
        g.fillEllipse (s.x - 5.0f, s.y - 8.0f, 10.0f, 12.0f);
        g.drawLine (s.x, s.y + 4.0f, s.x, s.y + 10.0f, 1.5f);
        g.drawLine (s.x - 4.0f, s.y + 10.0f, s.x + 4.0f, s.y + 10.0f, 1.5f);
        if (sel)
            g.drawEllipse (s.x - 9.0f, s.y - 11.0f, 18.0f, 18.0f, 1.4f);

        const juce::String label = micLabelText (m);
        g.setFont (Brand::tech (Brand::UI::scaledFont (11.0f), true));
        g.setColour (Brand::panelDark().withAlpha (0.85f));
        const float tw = (float) g.getCurrentFont().getStringWidth (label) + 10.0f;
        auto box = juce::Rectangle<float> (s.x + 10.0f, s.y - 10.0f, tw, 16.0f);
        g.fillRoundedRectangle (box, 3.0f);
        g.setColour (sel ? Brand::accent() : Brand::white());
        g.drawText (label, box.toNearestInt(), juce::Justification::centred, false);
    }
}

void RadiationPatternComponent::cancelDrawSession()
{
    resetDrawSession();
    pendingAnchor_ = false;
    hoverValid_ = false;
    numericBuffer_.clear();
    updateDrawPrompt();
    repaint();
}

void RadiationPatternComponent::resetDrawSession()
{
    sessionActive_ = false;
    sessionPts_.clear();
    hoverValid_ = false;
    numericBuffer_.clear();
}

void RadiationPatternComponent::beginDrawSession()
{
    sessionActive_ = true;
    sessionPts_.clear();
    numericBuffer_.clear();
    updateDrawPrompt();
}

juce::String RadiationPatternComponent::getDrawPrompt() const
{
    return drawPrompt_;
}

void RadiationPatternComponent::updateDrawPrompt()
{
    juce::String p;
    if (addMicArmed_)
        p = "MIC: click to place (Esc cancels). Snaps to 1 / 2 / 4 / 8 m rings.";
    else if (tool_ == Tool::Ruler)
        p = pendingAnchor_ ? "RULER: specify end point" : "RULER: specify start point";
    else if (tool_ == Tool::Shape)
    {
        const char* shapeName =
            drawShape_ == DrawShape::Line      ? "LINE" :
            drawShape_ == DrawShape::Polyline  ? "POLYLINE" :
            drawShape_ == DrawShape::Circle    ? "CIRCLE" :
            drawShape_ == DrawShape::Arc       ? "ARC" :
            drawShape_ == DrawShape::Rectangle ? "RECTANGLE" :
            drawShape_ == DrawShape::Square    ? "SQUARE" : "TEXT BOX";

        const char* method =
            construction_ == Construction::LineTwoPoints      ? "2 Points" :
            construction_ == Construction::LineOrtho          ? "Horizontal/Vertical" :
            construction_ == Construction::PolylinePoints     ? "Point-to-Point" :
            construction_ == Construction::PolylineClosed     ? "Closed" :
            construction_ == Construction::CircleCenterRadius ? "Center + Radius" :
            construction_ == Construction::CircleTwoPoints    ? "2 Points (diameter)" :
            construction_ == Construction::ArcThreePoints     ? "3 Points" :
            construction_ == Construction::TextBoxTwoCorners  ? "2 Corners" :
                                                                "2 Corners";

        const int n = (int) sessionPts_.size();
        const int need = pointsNeeded();
        juce::String step;
        if (drawShape_ == DrawShape::Polyline)
            step = sessionActive_ ? ("point " + juce::String (n + 1) + "  (Enter=finish, Esc=cancel)")
                                  : "specify first point";
        else if (n == 0)
            step = "specify first point";
        else if (drawShape_ == DrawShape::Circle && construction_ == Construction::CircleCenterRadius)
            step = "specify radius  (or type value + Enter)";
        else if (drawShape_ == DrawShape::Circle && construction_ == Construction::CircleTwoPoints)
            step = "specify second diameter point";
        else if (drawShape_ == DrawShape::Arc)
            step = (n == 1) ? "specify point on arc" : "specify end point";
        else
            step = "specify next point  (" + juce::String (n) + "/" + juce::String (need) + ")";

        p = juce::String (shapeName) + "  " + method + ":  " + step;
        if (construction_ == Construction::LineOrtho
            || juce::ModifierKeys::getCurrentModifiers().isShiftDown())
            p += "  [SHIFT ORTHO]";
        if (drawGridSnap_)
            p += "  [SNAP]";
        if (numericBuffer_.isNotEmpty())
            p += "  <" + numericBuffer_ + ">";
    }
    else if (tool_ == Tool::Pencil)
        p = "PENCIL: drag to draw";
    else if (tool_ == Tool::Eraser)
        p = "ERASER: drag to erase";
    else if (tool_ == Tool::Select && ortho_)
    {
        p = "ORTHO ";
        p += (orthoAlign_ == OrthoAlign::Horizontal) ? "Horizontal" : "Vertical";
        p += ": select 2+ speakers — linked gap ";
        p += juce::String (orthoSpacingM_, 2) + " m";
    }

    if (p != drawPrompt_)
    {
        drawPrompt_ = p;
        if (onDrawPromptChanged) onDrawPromptChanged();
    }
}

int RadiationPatternComponent::pointsNeeded() const noexcept
{
    switch (construction_)
    {
        case Construction::ArcThreePoints: return 3;
        case Construction::PolylinePoints:
        case Construction::PolylineClosed: return 2; // min; more allowed
        default: return 2;
    }
}

RadiationPatternComponent::AnnotSpace
RadiationPatternComponent::currentAnnotSpace() const noexcept
{
    if (params_.viewMode == ViewMode::Directivity
        || params_.viewMode == ViewMode::MeasuredPolar)
        return AnnotSpace::PolarPlot;
    return AnnotSpace::World;
}

juce::Point<float> RadiationPatternComponent::polarNormToScreen (float nx, float ny) const
{
    return { polarCx_ + nx * polarRadius_, polarCy_ - ny * polarRadius_ };
}

juce::Point<float> RadiationPatternComponent::screenToPolarNorm (float sx, float sy) const
{
    if (polarRadius_ < 1.0e-3f) return {};
    return { (sx - polarCx_) / polarRadius_,
             (polarCy_ - sy) / polarRadius_ };
}

juce::Point<float> RadiationPatternComponent::annotateToScreen (const Annotation& a,
                                                                juce::Point<float> p) const
{
    if (a.space == AnnotSpace::PolarPlot)
        return polarNormToScreen (p.x, p.y);
    return worldToScreen (p.x, p.y);
}

juce::Point<float> RadiationPatternComponent::screenToAnnot (float sx, float sy) const
{
    if (currentAnnotSpace() == AnnotSpace::PolarPlot)
        return screenToPolarNorm (sx, sy);
    auto w = screenToWorld (sx, sy);
    w.x = juce::jlimit (0.0f, (float) juce::jmax (1.0, result_.worldW), w.x);
    w.y = juce::jlimit (0.0f, (float) juce::jmax (1.0, result_.worldH), w.y);
    return w;
}

juce::Point<float> RadiationPatternComponent::snapAnnotPoint (juce::Point<float> p) const
{
    return snapAnnotPointFull (p, false);
}

float RadiationPatternComponent::snapScalar (float v, const std::vector<float>& candidates,
                                             float tol, bool& hit) noexcept
{
    hit = false;
    float best = v;
    float bestD = tol;
    for (float c : candidates)
    {
        const float d = std::abs (c - v);
        if (d <= bestD)
        {
            bestD = d;
            best = c;
            hit = true;
        }
    }
    return best;
}

juce::Rectangle<float> RadiationPatternComponent::annotationAnnotBounds (
    const Annotation& a) const
{
    if (a.pts.empty()) return {};

    if ((a.kind == Annotation::Kind::Rectangle || a.kind == Annotation::Kind::Square
         || a.kind == Annotation::Kind::Circle) && a.pts.size() >= 2)
        return normalisedShapeRect (a.pts[0], a.pts[1], a.kind);

    if (a.kind == Annotation::Kind::TextBox && a.pts.size() >= 2)
    {
        const auto local = textBoxLocalRect (a);
        const auto c = local.getCentre();
        const juce::Point<float> corners[4] = {
            rotateAround ({ local.getX(), local.getY() }, c, a.rotationDeg),
            rotateAround ({ local.getRight(), local.getY() }, c, a.rotationDeg),
            rotateAround ({ local.getRight(), local.getBottom() }, c, a.rotationDeg),
            rotateAround ({ local.getX(), local.getBottom() }, c, a.rotationDeg)
        };
        float minX = corners[0].x, maxX = corners[0].x;
        float minY = corners[0].y, maxY = corners[0].y;
        for (const auto& p : corners)
        {
            minX = juce::jmin (minX, p.x);
            maxX = juce::jmax (maxX, p.x);
            minY = juce::jmin (minY, p.y);
            maxY = juce::jmax (maxY, p.y);
        }
        return juce::Rectangle<float>::leftTopRightBottom (minX, minY, maxX, maxY);
    }

    float minX = a.pts.front().x, maxX = minX;
    float minY = a.pts.front().y, maxY = minY;
    for (const auto& p : a.pts)
    {
        minX = juce::jmin (minX, p.x);
        maxX = juce::jmax (maxX, p.x);
        minY = juce::jmin (minY, p.y);
        maxY = juce::jmax (maxY, p.y);
    }
    return juce::Rectangle<float>::leftTopRightBottom (minX, minY, maxX, maxY);
}

juce::Rectangle<float> RadiationPatternComponent::selectionAnnotBounds() const
{
    bool any = false;
    float minX = 0, minY = 0, maxX = 0, maxY = 0;
    for (int idx : selectedAnnots_)
    {
        if (idx < 0 || idx >= (int) annotations_.size()) continue;
        const auto b = annotationAnnotBounds (annotations_[(size_t) idx]);
        if (b.isEmpty()) continue;
        if (! any)
        {
            minX = b.getX(); minY = b.getY();
            maxX = b.getRight(); maxY = b.getBottom();
            any = true;
        }
        else
        {
            minX = juce::jmin (minX, b.getX());
            minY = juce::jmin (minY, b.getY());
            maxX = juce::jmax (maxX, b.getRight());
            maxY = juce::jmax (maxY, b.getBottom());
        }
    }
    if (! any) return {};
    return juce::Rectangle<float>::leftTopRightBottom (minX, minY, maxX, maxY);
}

juce::Rectangle<float> RadiationPatternComponent::selectionMoveBounds() const
{
    bool any = false;
    float minX = 0, minY = 0, maxX = 0, maxY = 0;
    auto grow = [&] (juce::Rectangle<float> b)
    {
        if (b.isEmpty()) return;
        if (! any)
        {
            minX = b.getX(); minY = b.getY();
            maxX = b.getRight(); maxY = b.getBottom();
            any = true;
        }
        else
        {
            minX = juce::jmin (minX, b.getX());
            minY = juce::jmin (minY, b.getY());
            maxX = juce::jmax (maxX, b.getRight());
            maxY = juce::jmax (maxY, b.getBottom());
        }
    };

    for (int idx : selectedAnnots_)
    {
        if (idx < 0 || idx >= (int) annotations_.size()) continue;
        grow (annotationAnnotBounds (annotations_[(size_t) idx]));
    }
    for (int idx : selectedSpeakers_)
    {
        if (idx < 0 || idx >= (int) speakers_.size()) continue;
        grow (speakerFootprintWorld (speakers_[(size_t) idx]));
    }

    if (! any) return {};
    return juce::Rectangle<float>::leftTopRightBottom (minX, minY, maxX, maxY);
}

juce::Point<float> RadiationPatternComponent::selectionSnapReference() const
{
    const auto b = selectionMoveBounds();
    if (! b.isEmpty())
        return { b.getX(), b.getY() };

    if (! selectedMics_.empty() && selectedMics_.front() >= 0
        && selectedMics_.front() < (int) mics_.size())
    {
        const auto& m = mics_[(size_t) selectedMics_.front()];
        return { m.x, m.y };
    }
    return {};
}

void RadiationPatternComponent::collectObjectSnapAxes (std::vector<float>& xs,
                                                       std::vector<float>& ys,
                                                       bool ignoreSelected) const
{
    xs.clear();
    ys.clear();
    const auto space = currentAnnotSpace();

    auto addX = [&] (float v) { xs.push_back (v); };
    auto addY = [&] (float v) { ys.push_back (v); };
    auto addBounds = [&] (juce::Rectangle<float> b)
    {
        if (b.isEmpty()) return;
        addX (b.getX());
        addX (b.getRight());
        addX (b.getCentreX());
        addY (b.getY());
        addY (b.getBottom());
        addY (b.getCentreY());
    };

    for (int i = 0; i < (int) annotations_.size(); ++i)
    {
        if (ignoreSelected && isAnnotationSelected (i)) continue;
        const auto& a = annotations_[(size_t) i];
        if (a.space != space) continue;

        if ((a.kind == Annotation::Kind::Rectangle || a.kind == Annotation::Kind::Square
             || a.kind == Annotation::Kind::Circle) && a.pts.size() >= 2)
        {
            addBounds (annotationAnnotBounds (a));
        }
        else
        {
            for (const auto& p : a.pts)
            {
                addX (p.x);
                addY (p.y);
            }
            // Midpoints of segments help lining up strokes.
            for (size_t k = 1; k < a.pts.size(); ++k)
            {
                addX (0.5f * (a.pts[k - 1].x + a.pts[k].x));
                addY (0.5f * (a.pts[k - 1].y + a.pts[k].y));
            }
        }
    }

    if (space == AnnotSpace::World)
    {
        for (int i = 0; i < (int) mics_.size(); ++i)
        {
            if (ignoreSelected && isMicSelected (i)) continue;
            addX (mics_[(size_t) i].x);
            addY (mics_[(size_t) i].y);
        }
        for (int i = 0; i < (int) speakers_.size(); ++i)
        {
            if (ignoreSelected && isSpeakerSelected (i)) continue;
            const auto& spk = speakers_[(size_t) i];
            addBounds (speakerFootprintWorld (spk));
        }
    }
}

void RadiationPatternComponent::collectAnnotationEdgeAxes (std::vector<float>& xs,
                                                           std::vector<float>& ys,
                                                           bool ignoreSelected) const
{
    xs.clear();
    ys.clear();
    const auto space = currentAnnotSpace();

    for (int i = 0; i < (int) annotations_.size(); ++i)
    {
        if (ignoreSelected && isAnnotationSelected (i)) continue;
        const auto& a = annotations_[(size_t) i];
        if (a.space != space) continue;

        if ((a.kind == Annotation::Kind::Rectangle || a.kind == Annotation::Kind::Square
             || a.kind == Annotation::Kind::Circle) && a.pts.size() >= 2)
        {
            const auto b = annotationAnnotBounds (a);
            if (b.isEmpty()) continue;
            xs.push_back (b.getX());
            xs.push_back (b.getRight());
            ys.push_back (b.getY());
            ys.push_back (b.getBottom());
        }
        else
        {
            for (const auto& p : a.pts)
            {
                xs.push_back (p.x);
                ys.push_back (p.y);
            }
        }
    }
}

bool RadiationPatternComponent::selectionMeetsOtherAnnotation (juce::Rectangle<float> sel) const
{
    if (sel.isEmpty()) return false;

    const float tol = juce::jmax (0.02f, 2.5f / juce::jmax (1.0f, worldScaleX()));
    const auto space = currentAnnotSpace();

    for (int i = 0; i < (int) annotations_.size(); ++i)
    {
        if (isAnnotationSelected (i)) continue;
        const auto& a = annotations_[(size_t) i];
        if (a.space != space) continue;

        const auto o = annotationAnnotBounds (a);
        if (o.isEmpty()) continue;

        const bool yOverlap = sel.getY() <= o.getBottom() + tol
                           && sel.getBottom() >= o.getY() - tol;
        const bool xOverlap = sel.getX() <= o.getRight() + tol
                           && sel.getRight() >= o.getX() - tol;

        if (yOverlap)
        {
            if (std::abs (sel.getRight() - o.getX())      <= tol) return true; // A right | B left
            if (std::abs (sel.getX()     - o.getRight())  <= tol) return true; // A left  | B right
            if (std::abs (sel.getX()     - o.getX())      <= tol) return true; // lefts aligned
            if (std::abs (sel.getRight() - o.getRight())  <= tol) return true; // rights aligned
        }
        if (xOverlap)
        {
            if (std::abs (sel.getBottom() - o.getY())       <= tol) return true;
            if (std::abs (sel.getY()      - o.getBottom())  <= tol) return true;
            if (std::abs (sel.getY()      - o.getY())       <= tol) return true;
            if (std::abs (sel.getBottom() - o.getBottom())  <= tol) return true;
        }
    }
    return false;
}

bool RadiationPatternComponent::selectionMeetsOtherSpeaker (juce::Rectangle<float> sel) const
{
    if (sel.isEmpty()) return false;

    const float tol = juce::jmax (0.02f, 2.5f / juce::jmax (1.0f, worldScaleX()));

    for (int i = 0; i < (int) speakers_.size(); ++i)
    {
        if (isSpeakerSelected (i)) continue;
        const auto o = speakerFootprintWorld (speakers_[(size_t) i]);
        if (o.isEmpty()) continue;

        const bool yOverlap = sel.getY() <= o.getBottom() + tol
                           && sel.getBottom() >= o.getY() - tol;
        const bool xOverlap = sel.getX() <= o.getRight() + tol
                           && sel.getRight() >= o.getX() - tol;

        if (yOverlap)
        {
            if (std::abs (sel.getRight() - o.getX())      <= tol) return true;
            if (std::abs (sel.getX()     - o.getRight())  <= tol) return true;
            if (std::abs (sel.getX()     - o.getX())      <= tol) return true;
            if (std::abs (sel.getRight() - o.getRight())  <= tol) return true;
        }
        if (xOverlap)
        {
            if (std::abs (sel.getBottom() - o.getY())       <= tol) return true;
            if (std::abs (sel.getY()      - o.getBottom())  <= tol) return true;
            if (std::abs (sel.getY()      - o.getY())       <= tol) return true;
            if (std::abs (sel.getBottom() - o.getBottom())  <= tol) return true;
        }
    }
    return false;
}

juce::Point<float> RadiationPatternComponent::snapAnnotPointFull (juce::Point<float> p,
                                                                  bool ignoreSelected,
                                                                  bool* objectHit) const
{
    if (objectHit != nullptr)
        *objectHit = false;

    if (! drawGridSnap_) return p;

    if (currentAnnotSpace() == AnnotSpace::PolarPlot)
    {
        const float r = std::sqrt (p.x * p.x + p.y * p.y);
        float ang = std::atan2 (p.y, p.x);
        const float stepR = 0.05f;
        const float stepA = (float) (15.0 * M_PI / 180.0);
        const float rs = std::round (r / stepR) * stepR;
        const float as = std::round (ang / stepA) * stepA;
        return { rs * std::cos (as), rs * std::sin (as) };
    }

    // 1) Snap to the same minor grid the heatmap draws (visible lines).
    const auto gm = currentGridMetrics();
    const float step = (float) gm.minor;
    float x = p.x;
    float y = p.y;
    if (step > 1.0e-9f)
    {
        x = std::round (p.x / step) * step;
        y = std::round (p.y / step) * step;
    }

    // 2) Object snap (edges / corners / centres) — wins when closer than ~10 px.
    //    Applied per-axis so a left edge can lock while Y still follows the grid.
    const float tolX = 10.0f / juce::jmax (1.0f, worldScaleX());
    const float tolY = 10.0f / juce::jmax (1.0f, worldScaleY());
    std::vector<float> xs, ys;
    collectObjectSnapAxes (xs, ys, ignoreSelected);
    bool hitX = false, hitY = false;
    const float ox = snapScalar (p.x, xs, tolX, hitX);
    const float oy = snapScalar (p.y, ys, tolY, hitY);
    if (hitX) x = ox;
    if (hitY) y = oy;

    // Tak / objectHit: only other drawing edges — not speakers, mics, or centres
    // (those caused random clicks and armed the sound before a real shape meet).
    if (objectHit != nullptr)
    {
        std::vector<float> ex, ey;
        collectAnnotationEdgeAxes (ex, ey, ignoreSelected);
        bool edgeX = false, edgeY = false;
        snapScalar (p.x, ex, tolX, edgeX);
        snapScalar (p.y, ey, tolY, edgeY);
        *objectHit = edgeX || edgeY;
    }

    return { x, y };
}

void RadiationPatternComponent::resetSnapSoundState() noexcept
{
    snapSoundArmed_ = false;
    snapSoundOffFrames_ = 0;
}

void RadiationPatternComponent::noteSnapSound (bool objectSnapEngaged,
                                               juce::Point<float> raw,
                                               juce::Point<float> snapped)
{
    juce::ignoreUnused (raw, snapped);

    if (! drawGridSnap_)
    {
        snapSoundArmed_ = false;
        snapSoundOffFrames_ = 0;
        return;
    }

    // Tak on rising edge of object/edge/corner latch only (not grid steps).
    // Do not require a large raw→snapped pull: selection moves often apply a
    // small correction on the meeting edge while the opposite corner is already
    // on-grid, which previously silenced the click when two shapes met.
    if (! objectSnapEngaged)
    {
        if (++snapSoundOffFrames_ >= 4)
            snapSoundArmed_ = false;
        return;
    }

    snapSoundOffFrames_ = 0;

    if (! snapSoundArmed_)
    {
        SnapClick::playTak();
        lastSnapSoundPos_ = snapped;
        snapSoundArmed_ = true;
    }
}

juce::Point<float> RadiationPatternComponent::applyOrtho (juce::Point<float> from,
                                                          juce::Point<float> to) const
{
    // Drawing constraint: Line Ortho construction or hold Shift — not the Ortho align tool.
    const bool force = construction_ == Construction::LineOrtho
                    || juce::ModifierKeys::getCurrentModifiers().isShiftDown();
    if (! force)
        return to;
    const float dx = std::abs (to.x - from.x);
    const float dy = std::abs (to.y - from.y);
    if (dx >= dy)
        return { to.x, from.y };
    return { from.x, to.y };
}

bool RadiationPatternComponent::circleFrom3Points (juce::Point<float> p1,
                                                   juce::Point<float> p2,
                                                   juce::Point<float> p3,
                                                   juce::Point<float>& centre,
                                                   float& radius) noexcept
{
    const float ax = p1.x, ay = p1.y;
    const float bx = p2.x, by = p2.y;
    const float cx = p3.x, cy = p3.y;
    const float d = 2.0f * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));
    if (std::abs (d) < 1.0e-10f) return false;
    const float ux = ((ax * ax + ay * ay) * (by - cy)
                    + (bx * bx + by * by) * (cy - ay)
                    + (cx * cx + cy * cy) * (ay - by)) / d;
    const float uy = ((ax * ax + ay * ay) * (cx - bx)
                    + (bx * bx + by * by) * (ax - cx)
                    + (cx * cx + cy * cy) * (bx - ax)) / d;
    centre = { ux, uy };
    radius = centre.getDistanceFrom (p1);
    return radius > 1.0e-6f;
}

void RadiationPatternComponent::commitAnnotation (Annotation a)
{
    if (onWillEdit) onWillEdit();
    a.space = currentAnnotSpace();
    a.colour = drawColour_;
    a.fillAlpha = drawFillAlpha_;
    a.construction = construction_;
    annotations_.push_back (std::move (a));
    // Newly placed shape becomes the selection so opacity/move apply to it.
    if (annotations_.back().kind != Annotation::Kind::Freehand)
        setSelectedAnnotation ((int) annotations_.size() - 1);
    if (onEditCommitted) onEditCommitted();
    resetDrawSession();
    updateDrawPrompt();
    repaint();
}

void RadiationPatternComponent::setAnnotations (std::vector<Annotation> a)
{
    annotations_ = std::move (a);
    setSelectedAnnotation (-1);
    resetDrawSession();
    pendingAnchor_ = false;
    drag_ = Drag::None;
    updateDrawPrompt();
    repaint();
}

bool RadiationPatternComponent::finishPolyline (bool forceClose)
{
    if (sessionPts_.size() < 2) return false;
    Annotation a;
    a.kind = Annotation::Kind::Polyline;
    a.pts = sessionPts_;
    a.closed = forceClose || construction_ == Construction::PolylineClosed;
    a.thicknessPx = 2.2f;
    if (a.closed && a.pts.size() >= 2
        && a.pts.front().getDistanceFrom (a.pts.back()) > 1.0e-4f)
        a.pts.push_back (a.pts.front());
    commitAnnotation (std::move (a));
    return true;
}

bool RadiationPatternComponent::acceptAnnotPoint (juce::Point<float> raw)
{
    bool objHit = false;
    auto p = snapAnnotPointFull (raw, false, &objHit);
    noteSnapSound (objHit, raw, p);
    if (! sessionPts_.empty())
        p = applyOrtho (sessionPts_.back(), p);

    if (drawShape_ == DrawShape::Polyline)
    {
        if (! sessionActive_)
            beginDrawSession();

        // Close if click lands near the start vertex.
        const float closeTol = (currentAnnotSpace() == AnnotSpace::PolarPlot) ? 0.08f : 0.12f;
        if (sessionPts_.size() >= 2
            && sessionPts_.front().getDistanceFrom (p) < closeTol)
            return finishPolyline (true);

        sessionPts_.push_back (p);
        updateDrawPrompt();
        repaint();
        return true;
    }

    if (! sessionActive_)
        beginDrawSession();

    sessionPts_.push_back (p);
    const int need = pointsNeeded();

    if ((int) sessionPts_.size() < need)
    {
        updateDrawPrompt();
        repaint();
        return true;
    }

    // Complete shape
    Annotation a;
    a.thicknessPx = 2.0f;

    if (drawShape_ == DrawShape::Line)
    {
        a.kind = Annotation::Kind::Line;
        a.pts = { sessionPts_[0], sessionPts_[1] };
        a.thicknessPx = 2.2f;
        // Reject near-zero segments (e.g. accidental double-click).
        const float minLen = (currentAnnotSpace() == AnnotSpace::PolarPlot) ? 1.0e-4f : 1.0e-3f;
        if (a.pts[0].getDistanceFrom (a.pts[1]) <= minLen)
        {
            sessionPts_.pop_back();
            updateDrawPrompt();
            repaint();
            return true;
        }
        commitAnnotation (std::move (a));
        return true;
    }

    if (drawShape_ == DrawShape::Circle)
    {
        a.kind = Annotation::Kind::Circle;
        if (construction_ == Construction::CircleTwoPoints)
        {
            // Diameter endpoints → store as center + rim point
            const auto mid = (sessionPts_[0] + sessionPts_[1]) * 0.5f;
            a.pts = { mid, sessionPts_[1] };
        }
        else
            a.pts = { sessionPts_[0], sessionPts_[1] };
        commitAnnotation (std::move (a));
        return true;
    }

    if (drawShape_ == DrawShape::Arc && sessionPts_.size() >= 3)
    {
        juce::Point<float> c;
        float r = 0;
        if (! circleFrom3Points (sessionPts_[0], sessionPts_[1], sessionPts_[2], c, r))
        {
            resetDrawSession();
            updateDrawPrompt();
            repaint();
            return false;
        }
        a.kind = Annotation::Kind::Arc;
        // Store: center, start, end, mid-on-arc (for sweep sense)
        a.pts = { c, sessionPts_[0], sessionPts_[2], sessionPts_[1] };
        commitAnnotation (std::move (a));
        return true;
    }

    if (drawShape_ == DrawShape::Rectangle || drawShape_ == DrawShape::Square)
    {
        a.kind = (drawShape_ == DrawShape::Square) ? Annotation::Kind::Square
                                                   : Annotation::Kind::Rectangle;
        a.pts = { sessionPts_[0], sessionPts_[1] };
        commitAnnotation (std::move (a));
        return true;
    }

    if (drawShape_ == DrawShape::TextBox)
    {
        a.kind = Annotation::Kind::TextBox;
        a.pts = { sessionPts_[0], sessionPts_[1] };
        a.text = "Text";
        a.rotationDeg = 0.0f;
        a.thicknessPx = 1.5f;
        commitAnnotation (std::move (a));
        promptEditTextBox ((int) annotations_.size() - 1);
        return true;
    }

    resetDrawSession();
    updateDrawPrompt();
    return false;
}

bool RadiationPatternComponent::commitNumericValue (double value)
{
    if (tool_ != Tool::Shape || ! sessionActive_ || sessionPts_.empty())
        return false;
    if (value <= 0.0) return false;

    const auto& from = sessionPts_.back();
    juce::Point<float> dir = hoverValid_ ? (hoverAnnot_ - from)
                                         : juce::Point<float> (1.0f, 0.0f);
    const float len = dir.getDistanceFromOrigin();
    if (len < 1.0e-8f)
        dir = { 1.0f, 0.0f };
    else
        dir *= (1.0f / len);

    // In polar space, numeric is in normalised units; in world, metres.
    const float dist = (float) value;
    auto to = from + dir * dist;
    to = applyOrtho (from, to);
    return acceptAnnotPoint (to);
}

bool RadiationPatternComponent::canAnnotate() const noexcept
{
    // SPL heatmap or polar plot overlays.
    if (params_.viewMode == ViewMode::Directivity
        || params_.viewMode == ViewMode::MeasuredPolar)
        return true;
    return hasData_;
}

float RadiationPatternComponent::distPointToSegment (juce::Point<float> p,
                                                     juce::Point<float> a,
                                                     juce::Point<float> b) noexcept
{
    const auto ab = b - a;
    const float len2 = ab.x * ab.x + ab.y * ab.y;
    if (len2 < 1.0e-12f)
        return p.getDistanceFrom (a);
    float t = ((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / len2;
    t = juce::jlimit (0.0f, 1.0f, t);
    return p.getDistanceFrom (a + ab * t);
}

void RadiationPatternComponent::eraseNear (juce::Point<float> annotPt, float radius)
{
    annotations_.erase (std::remove_if (annotations_.begin(), annotations_.end(),
        [&] (const Annotation& a)
        {
            if (a.space != currentAnnotSpace()) return false;

            if (a.kind == Annotation::Kind::Rectangle
                || a.kind == Annotation::Kind::Square
                || a.kind == Annotation::Kind::Circle
                || a.kind == Annotation::Kind::Arc
                || a.kind == Annotation::Kind::Polyline
                || a.kind == Annotation::Kind::TextBox)
                return a.kind == Annotation::Kind::TextBox
                    ? pointHitsTextBox (annotPt, a, radius)
                    : pointHitsShape (annotPt, a, radius);

            if (a.pts.empty()) return true;
            if (a.pts.size() == 1)
                return annotPt.getDistanceFrom (a.pts.front()) <= radius;

            for (size_t i = 1; i < a.pts.size(); ++i)
                if (distPointToSegment (annotPt, a.pts[i - 1], a.pts[i]) <= radius)
                    return true;
            return false;
        }), annotations_.end());

    // Indices shift after erase — clear selection to avoid pointing at the wrong shape.
    setSelectedAnnotation (-1);
}

bool RadiationPatternComponent::isFilledShapeKind (Annotation::Kind k) noexcept
{
    return k == Annotation::Kind::Rectangle
        || k == Annotation::Kind::Square
        || k == Annotation::Kind::Circle
        || k == Annotation::Kind::TextBox;
}

int RadiationPatternComponent::annotationHitTest (juce::Point<float> annotPt,
                                                  float radius) const
{
    const auto space = currentAnnotSpace();
    for (int i = (int) annotations_.size() - 1; i >= 0; --i)
    {
        const auto& a = annotations_[(size_t) i];
        if (a.space != space) continue;

        if (a.kind == Annotation::Kind::Rectangle
            || a.kind == Annotation::Kind::Square
            || a.kind == Annotation::Kind::Circle
            || a.kind == Annotation::Kind::Arc
            || a.kind == Annotation::Kind::Polyline
            || a.kind == Annotation::Kind::TextBox)
        {
            if (a.kind == Annotation::Kind::TextBox
                ? pointHitsTextBox (annotPt, a, radius)
                : pointHitsShape (annotPt, a, radius))
                return i;
            continue;
        }

        if (a.pts.empty()) continue;
        if (a.pts.size() == 1)
        {
            if (annotPt.getDistanceFrom (a.pts.front()) <= radius)
                return i;
            continue;
        }
        for (size_t j = 1; j < a.pts.size(); ++j)
            if (distPointToSegment (annotPt, a.pts[j - 1], a.pts[j]) <= radius)
                return i;
    }
    return -1;
}

void RadiationPatternComponent::setSelectedAnnotation (int index)
{
    if (index < 0 || index >= (int) annotations_.size())
        index = -1;

    selectedAnnots_.clear();
    selectedMics_.clear();
    selectedSpeakers_.clear();
    selectedMic_ = -1;
    if (index >= 0)
        selectedAnnots_.push_back (index);

    if (selectedAnnot_ == index && index >= 0)
    {
        syncPrimarySelectionFromSets();
        if (onAnnotSelectionChanged) onAnnotSelectionChanged();
        repaint();
        return;
    }

    selectedAnnot_ = index;
    syncPrimarySelectionFromSets();
    if (onAnnotSelectionChanged) onAnnotSelectionChanged();
    repaint();
}

void RadiationPatternComponent::moveSelectedAnnotationBy (juce::Point<float> deltaAnnot)
{
    if (std::abs (deltaAnnot.x) < 1.0e-12f && std::abs (deltaAnnot.y) < 1.0e-12f)
        return;
    for (int idx : selectedAnnots_)
    {
        if (idx < 0 || idx >= (int) annotations_.size()) continue;
        auto& a = annotations_[(size_t) idx];
        for (auto& p : a.pts)
            p += deltaAnnot;
    }
}

void RadiationPatternComponent::moveSelectionBy (juce::Point<float> deltaAnnot,
                                                 juce::Point<float> deltaWorld)
{
    if (std::abs (deltaAnnot.x) > 1.0e-12f || std::abs (deltaAnnot.y) > 1.0e-12f)
        moveSelectedAnnotationBy (deltaAnnot);

    const bool moveWorld = (std::abs (deltaWorld.x) > 1.0e-12f
                            || std::abs (deltaWorld.y) > 1.0e-12f);
    if (! moveWorld) return;

    for (int idx : selectedMics_)
    {
        // Group / mixed selection move: free translate only — ring snap is
        // handled by the dedicated Drag::Mic path (original first snap pattern).
        if (idx < 0 || idx >= (int) mics_.size()) continue;
        auto& mic = mics_[(size_t) idx];
        mic.x = juce::jlimit (0.0f, (float) result_.worldW, mic.x + deltaWorld.x);
        mic.y = juce::jlimit (0.0f, (float) result_.worldH, mic.y + deltaWorld.y);
        mic.ringLocked = false;
        mic.ringSpeaker = -1;
    }

    for (int idx : selectedSpeakers_)
    {
        if (idx < 0 || idx >= (int) speakers_.size()) continue;
        auto& spk = speakers_[(size_t) idx];
        spk.x = juce::jlimit (0.0f, (float) result_.worldW, spk.x + deltaWorld.x);
        spk.y = juce::jlimit (0.0f, (float) result_.worldH, spk.y + deltaWorld.y);
        if (onSpeakerMoved)
            onSpeakerMoved (idx, spk.x, spk.y);
    }

    if (! selectedMics_.empty())
    {
        refreshMicLevels();
        if (onMicsChanged) onMicsChanged();
    }
}

std::vector<juce::Point<float>> RadiationPatternComponent::resizeHandlesFor (
    const Annotation& a)
{
    std::vector<juce::Point<float>> h;
    if ((a.kind == Annotation::Kind::Rectangle || a.kind == Annotation::Kind::Square)
        && a.pts.size() >= 2)
    {
        const auto r = normalisedShapeRect (a.pts[0], a.pts[1], a.kind);
        h.push_back ({ r.getX(),      r.getY() });
        h.push_back ({ r.getRight(),  r.getY() });
        h.push_back ({ r.getRight(),  r.getBottom() });
        h.push_back ({ r.getX(),      r.getBottom() });
        return h;
    }

    if (a.kind == Annotation::Kind::TextBox && a.pts.size() >= 2)
    {
        const auto r = textBoxLocalRect (a);
        const auto c = r.getCentre();
        // 0–3: corners (rotated). 4: rotate grip above top-centre.
        h.push_back (rotateAround ({ r.getX(), r.getY() }, c, a.rotationDeg));
        h.push_back (rotateAround ({ r.getRight(), r.getY() }, c, a.rotationDeg));
        h.push_back (rotateAround ({ r.getRight(), r.getBottom() }, c, a.rotationDeg));
        h.push_back (rotateAround ({ r.getX(), r.getBottom() }, c, a.rotationDeg));
        const float lift = juce::jmax (r.getHeight() * 0.35f,
                                       (a.space == AnnotSpace::PolarPlot) ? 0.06f : 0.8f);
        h.push_back (rotateAround ({ r.getCentreX(), r.getBottom() + lift }, c, a.rotationDeg));
        return h;
    }

    if (a.kind == Annotation::Kind::Circle && a.pts.size() >= 2)
    {
        const auto c = a.pts[0];
        const float rad = juce::jmax (1.0e-4f, c.getDistanceFrom (a.pts[1]));
        h.push_back (c);                          // 0 = centre (move)
        h.push_back ({ c.x + rad, c.y });         // E / N / W / S radius grips
        h.push_back ({ c.x,       c.y + rad });
        h.push_back ({ c.x - rad, c.y });
        h.push_back ({ c.x,       c.y - rad });
        return h;
    }

    return a.pts; // line / polyline / arc / freehand vertices
}

int RadiationPatternComponent::resizeHandleHitTest (const Annotation& a,
                                                    juce::Point<float> annotPt,
                                                    float radius) const
{
    const auto handles = resizeHandlesFor (a);
    for (int i = (int) handles.size() - 1; i >= 0; --i)
        if (annotPt.getDistanceFrom (handles[(size_t) i]) <= radius)
            return i;
    return -1;
}

void RadiationPatternComponent::applyAnnotationResize (Annotation& a, int handleIndex,
                                                       juce::Point<float> annotPt)
{
    if (handleIndex < 0) return;
    auto pt = annotPt;
    if (drawGridSnap_)
    {
        bool objHit = false;
        pt = snapAnnotPointFull (annotPt, true, &objHit);
        noteSnapSound (objHit, annotPt, pt);
    }

    if ((a.kind == Annotation::Kind::Rectangle || a.kind == Annotation::Kind::Square)
        && a.pts.size() >= 2 && handleIndex < 4)
    {
        const auto r = normalisedShapeRect (a.pts[0], a.pts[1], a.kind);
        const juce::Point<float> corners[4] = {
            { r.getX(),     r.getY() },
            { r.getRight(), r.getY() },
            { r.getRight(), r.getBottom() },
            { r.getX(),     r.getBottom() }
        };
        const auto fixed = corners[(handleIndex + 2) % 4];
        a.pts.resize (2);
        a.pts[0] = fixed;
        a.pts[1] = pt;
        return;
    }

    if (a.kind == Annotation::Kind::TextBox && a.pts.size() >= 2 && handleIndex < 4)
    {
        const auto r = textBoxLocalRect (a);
        const auto c = r.getCentre();
        const auto localPt = rotateAround (pt, c, -a.rotationDeg);
        const juce::Point<float> corners[4] = {
            { r.getX(),     r.getY() },
            { r.getRight(), r.getY() },
            { r.getRight(), r.getBottom() },
            { r.getX(),     r.getBottom() }
        };
        const auto fixed = corners[(handleIndex + 2) % 4];
        a.pts.resize (2);
        a.pts[0] = fixed;
        a.pts[1] = localPt;
        return;
    }

    if (a.kind == Annotation::Kind::Circle && a.pts.size() >= 2)
    {
        if (handleIndex == 0)
        {
            const auto d = pt - a.pts[0];
            a.pts[0] = pt;
            a.pts[1] += d;
        }
        else
        {
            float rad = pt.getDistanceFrom (a.pts[0]);
            if (rad < 1.0e-4f) rad = 1.0e-4f;
            auto dir = pt - a.pts[0];
            const float len = dir.getDistanceFromOrigin();
            if (len < 1.0e-8f)
                dir = { 1.0f, 0.0f };
            else
                dir *= (1.0f / len);
            a.pts[1] = a.pts[0] + dir * rad;
        }
        return;
    }

    if (handleIndex < (int) a.pts.size())
        a.pts[(size_t) handleIndex] = pt;
}

void RadiationPatternComponent::drawSelectionOverlay (juce::Graphics& g, const Annotation& a)
{
    g.setColour (Brand::accent());

    if (a.kind == Annotation::Kind::Circle && a.pts.size() >= 2)
    {
        const auto& c = a.pts[0];
        const float r = c.getDistanceFrom (a.pts[1]);
        auto sc = annotateToScreen (a, c);
        float rx, ry;
        if (a.space == AnnotSpace::PolarPlot)
            rx = ry = r * polarRadius_;
        else
        {
            rx = r * worldScaleX();
            ry = r * worldScaleY();
        }
        g.drawEllipse (sc.x - rx, sc.y - ry, rx * 2.0f, ry * 2.0f, 1.5f);
    }
    else if ((a.kind == Annotation::Kind::Rectangle || a.kind == Annotation::Kind::Square)
             && a.pts.size() >= 2)
    {
        const auto wr = normalisedShapeRect (a.pts[0], a.pts[1], a.kind);
        auto s0 = annotateToScreen (a, { wr.getX(), wr.getY() });
        auto s1 = annotateToScreen (a, { wr.getRight(), wr.getBottom() });
        auto sr = juce::Rectangle<float>::leftTopRightBottom (
            juce::jmin (s0.x, s1.x), juce::jmin (s0.y, s1.y),
            juce::jmax (s0.x, s1.x), juce::jmax (s0.y, s1.y));
        g.drawRect (sr, 1.5f);
    }
    else if (a.kind == Annotation::Kind::TextBox && a.pts.size() >= 2)
    {
        const auto handles = resizeHandlesFor (a);
        if (handles.size() >= 4)
        {
            juce::Path outline;
            auto s0 = annotateToScreen (a, handles[0]);
            outline.startNewSubPath (s0);
            for (int i = 1; i < 4; ++i)
                outline.lineTo (annotateToScreen (a, handles[(size_t) i]));
            outline.closeSubPath();
            g.strokePath (outline, juce::PathStrokeType (1.5f));
            if (handles.size() >= 5)
            {
                auto topMid = annotateToScreen (a, (handles[0] + handles[1]) * 0.5f);
                auto rot = annotateToScreen (a, handles[4]);
                g.drawLine (topMid.x, topMid.y, rot.x, rot.y, 1.2f);
            }
        }
    }

    // Resize grips only when this is the sole selected drawing.
    const bool showGrips = (selectedAnnots_.size() == 1
                            && selectedMics_.empty()
                            && selectedSpeakers_.empty());
    const auto handles = resizeHandlesFor (a);
    for (size_t hi = 0; hi < handles.size(); ++hi)
    {
        const auto& p = handles[hi];
        auto s = annotateToScreen (a, p);
        const bool isRotate = (a.kind == Annotation::Kind::TextBox && hi == 4);
        g.setColour (isRotate ? Brand::white() : Brand::accent());
        if (showGrips)
            g.fillEllipse (s.x - 4.5f, s.y - 4.5f, 9.0f, 9.0f);
        else
            g.drawEllipse (s.x - 3.5f, s.y - 3.5f, 7.0f, 7.0f, 1.4f);
        g.setColour (isRotate ? Brand::accent() : Brand::panelDark());
        g.drawEllipse (s.x - 4.5f, s.y - 4.5f, 9.0f, 9.0f, 1.2f);
    }
}

juce::Rectangle<float> RadiationPatternComponent::annotationScreenBounds (
    const Annotation& a) const
{
    if (a.pts.empty()) return {};

    if ((a.kind == Annotation::Kind::Rectangle || a.kind == Annotation::Kind::Square
         || a.kind == Annotation::Kind::Circle) && a.pts.size() >= 2)
    {
        const auto wr = normalisedShapeRect (a.pts[0], a.pts[1], a.kind);
        auto s0 = annotateToScreen (a, { wr.getX(), wr.getY() });
        auto s1 = annotateToScreen (a, { wr.getRight(), wr.getBottom() });
        return juce::Rectangle<float>::leftTopRightBottom (
            juce::jmin (s0.x, s1.x), juce::jmin (s0.y, s1.y),
            juce::jmax (s0.x, s1.x), juce::jmax (s0.y, s1.y)).expanded (3.0f);
    }

    if (a.kind == Annotation::Kind::TextBox && a.pts.size() >= 2)
    {
        const auto ab = annotationAnnotBounds (a);
        auto s0 = annotateToScreen (a, { ab.getX(), ab.getY() });
        auto s1 = annotateToScreen (a, { ab.getRight(), ab.getBottom() });
        return juce::Rectangle<float>::leftTopRightBottom (
            juce::jmin (s0.x, s1.x), juce::jmin (s0.y, s1.y),
            juce::jmax (s0.x, s1.x), juce::jmax (s0.y, s1.y)).expanded (8.0f);
    }

    float minX = 1.0e9f, minY = 1.0e9f, maxX = -1.0e9f, maxY = -1.0e9f;
    for (const auto& p : a.pts)
    {
        auto s = annotateToScreen (a, p);
        minX = juce::jmin (minX, s.x);
        minY = juce::jmin (minY, s.y);
        maxX = juce::jmax (maxX, s.x);
        maxY = juce::jmax (maxY, s.y);
    }
    if (maxX < minX) return {};
    // Thin strokes (lines) get a hit padding so marquee can catch them.
    return juce::Rectangle<float>::leftTopRightBottom (minX, minY, maxX, maxY)
        .expanded (6.0f);
}

juce::Rectangle<float> RadiationPatternComponent::currentMarqueeScreen() const
{
    return juce::Rectangle<float>::leftTopRightBottom (
        juce::jmin (marqueeStartScreen_.x, marqueeEndScreen_.x),
        juce::jmin (marqueeStartScreen_.y, marqueeEndScreen_.y),
        juce::jmax (marqueeStartScreen_.x, marqueeEndScreen_.x),
        juce::jmax (marqueeStartScreen_.y, marqueeEndScreen_.y));
}

void RadiationPatternComponent::applyMarqueeSelection (bool addToExisting)
{
    const auto box = currentMarqueeScreen();
    if (box.getWidth() < 3.0f && box.getHeight() < 3.0f)
        return;

    if (! addToExisting)
    {
        selectedAnnots_.clear();
        selectedMics_.clear();
        selectedSpeakers_.clear();
    }

    const auto space = currentAnnotSpace();
    for (int i = 0; i < (int) annotations_.size(); ++i)
    {
        const auto& a = annotations_[(size_t) i];
        if (a.space != space) continue;
        if (box.intersects (annotationScreenBounds (a)) && ! isAnnotationSelected (i))
            selectedAnnots_.push_back (i);
    }

    if (space == AnnotSpace::World)
    {
        for (int i = 0; i < (int) mics_.size(); ++i)
        {
            auto s = worldToScreen (mics_[(size_t) i].x, mics_[(size_t) i].y);
            if (box.contains (s) && ! isMicSelected (i))
                selectedMics_.push_back (i);
        }
        for (int i = 0; i < (int) speakers_.size(); ++i)
        {
            if (box.intersects (speakerFootprintScreen (speakers_[(size_t) i]))
                && ! isSpeakerSelected (i))
                selectedSpeakers_.push_back (i);
        }
    }

    syncPrimarySelectionFromSets();
    if (onAnnotSelectionChanged) onAnnotSelectionChanged();
    if (onMicsChanged) onMicsChanged();
    if (! selectedSpeakers_.empty() && onSpeakerSelected)
        onSpeakerSelected (selectedSpeakers_.back());
}

void RadiationPatternComponent::drawMarqueeOverlay (juce::Graphics& g)
{
    if (drag_ != Drag::Marquee) return;
    auto r = currentMarqueeScreen();
    if (r.getWidth() < 1.0f && r.getHeight() < 1.0f) return;
    g.setColour (Brand::accent().withAlpha (0.15f));
    g.fillRect (r);
    g.setColour (Brand::accent().withAlpha (0.9f));
    g.drawRect (r, 1.2f);
}

juce::Rectangle<float> RadiationPatternComponent::normalisedShapeRect (
    juce::Point<float> a, juce::Point<float> b, Annotation::Kind kind) noexcept
{
    // Circle (AutoCAD CIRCLE default): a = center, b = point on circumference.
    if (kind == Annotation::Kind::Circle)
    {
        const float r = a.getDistanceFrom (b);
        return juce::Rectangle<float> (a.x - r, a.y - r, r * 2.0f, r * 2.0f);
    }

    // Square: first corner a, opposite corner constrained to equal sides.
    if (kind == Annotation::Kind::Square)
    {
        const float sx = (b.x >= a.x) ? 1.0f : -1.0f;
        const float sy = (b.y >= a.y) ? 1.0f : -1.0f;
        const float side = juce::jmax (std::abs (b.x - a.x), std::abs (b.y - a.y));
        const float x1 = a.x + sx * side;
        const float y1 = a.y + sy * side;
        return juce::Rectangle<float>::leftTopRightBottom (
            juce::jmin (a.x, x1), juce::jmin (a.y, y1),
            juce::jmax (a.x, x1), juce::jmax (a.y, y1));
    }

    // Rectangle (AutoCAD RECTANG): diagonally opposite corners.
    return juce::Rectangle<float>::leftTopRightBottom (
        juce::jmin (a.x, b.x), juce::jmin (a.y, b.y),
        juce::jmax (a.x, b.x), juce::jmax (a.y, b.y));
}

juce::Point<float> RadiationPatternComponent::rotateAround (juce::Point<float> p,
                                                            juce::Point<float> c,
                                                            float deg) noexcept
{
    const float rad = deg * (float) M_PI / 180.0f;
    const float cs = std::cos (rad), sn = std::sin (rad);
    const float dx = p.x - c.x, dy = p.y - c.y;
    return { c.x + dx * cs - dy * sn, c.y + dx * sn + dy * cs };
}

juce::Rectangle<float> RadiationPatternComponent::textBoxLocalRect (const Annotation& a) noexcept
{
    if (a.pts.size() < 2) return {};
    return normalisedShapeRect (a.pts[0], a.pts[1], Annotation::Kind::Rectangle);
}

bool RadiationPatternComponent::pointHitsTextBox (juce::Point<float> pt,
                                                  const Annotation& a,
                                                  float radius) const noexcept
{
    const auto local = textBoxLocalRect (a);
    if (local.isEmpty()) return false;
    const auto c = local.getCentre();
    const auto unrot = rotateAround (pt, c, -a.rotationDeg);
    return local.expanded (radius).contains (unrot);
}

void RadiationPatternComponent::promptEditTextBox (int index)
{
    if (index < 0 || index >= (int) annotations_.size()) return;
    if (annotations_[(size_t) index].kind != Annotation::Kind::TextBox) return;

    auto aw = std::make_shared<juce::AlertWindow> (
        "Text Box", "Enter label text:", juce::AlertWindow::NoIcon);
    aw->addTextEditor ("txt", annotations_[(size_t) index].text, "Text:");
    aw->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    aw->enterModalState (true,
        juce::ModalCallbackFunction::create (
            [safe = juce::Component::SafePointer<RadiationPatternComponent> (this),
             index, aw] (int result)
            {
                if (safe == nullptr || result != 1) return;
                if (index < 0 || index >= (int) safe->annotations_.size()) return;
                auto& a = safe->annotations_[(size_t) index];
                if (a.kind != Annotation::Kind::TextBox) return;
                const auto next = aw->getTextEditorContents ("txt").trim();
                if (next == a.text) return;
                if (safe->onWillEdit) safe->onWillEdit();
                a.text = next.isNotEmpty() ? next : juce::String ("Text");
                if (safe->onEditCommitted) safe->onEditCommitted();
                safe->repaint();
            }),
        false);
}


juce::String RadiationPatternComponent::formatLengthLabel (float metres)
{
    const float m = std::abs (metres);
    if (m < 1.0f)
        return juce::String (m * 100.0f, 0) + " cm";
    return juce::String (m, 2) + " m";
}

void RadiationPatternComponent::drawPendingDimLabel (juce::Graphics& g,
                                                     juce::Point<float> screenMid,
                                                     const juce::String& text)
{
    g.setFont (Brand::tech (Brand::UI::scaledFont (11.0f), true));
    const float tw = (float) juce::jmax (64, text.length() * 7 + 16);
    const float th = 18.0f;
    auto labelBox = juce::Rectangle<float> (screenMid.x - tw * 0.5f, screenMid.y - th - 6.0f,
                                            tw, th);
    g.setColour (Brand::panelDark().withAlpha (0.85f));
    g.fillRoundedRectangle (labelBox, 4.0f);
    g.setColour (drawColour_);
    g.drawText (text, labelBox.toNearestInt(), juce::Justification::centred, false);
}

bool RadiationPatternComponent::sampleSplAtWorld (float wx, float wy,
                                                  float& absDb, float& relDb) const noexcept
{
    const int W = result_.width;
    const int H = result_.height;
    if (W < 2 || H < 2 || result_.worldW < 1.0e-6 || result_.worldH < 1.0e-6)
        return false;

    const float fx = (float) ((wx / (float) result_.worldW) * (double) (W - 1));
    const float fy = (float) ((wy / (float) result_.worldH) * (double) (H - 1));
    if (fx < 0.0f || fy < 0.0f || fx > (float) (W - 1) || fy > (float) (H - 1))
        return false;

    const int c0 = juce::jlimit (0, W - 2, (int) std::floor (fx));
    const int r0 = juce::jlimit (0, H - 2, (int) std::floor (fy));
    const int c1 = c0 + 1;
    const int r1 = r0 + 1;
    const float tx = fx - (float) c0;
    const float ty = fy - (float) r0;

    const bool hasRel = result_.splRelDB.size() == (size_t) W * (size_t) H;
    const bool hasAbs = result_.hasAbsoluteSpl
                     && result_.splAbsDB.size() == (size_t) W * (size_t) H;
    const bool hasDisp = result_.splDB.size() == (size_t) W * (size_t) H;
    if (! hasRel && ! hasAbs && ! hasDisp)
        return false;

    auto sample = [&] (const std::vector<float>& grid) -> float
    {
        const float v00 = grid[(size_t) r0 * (size_t) W + (size_t) c0];
        const float v10 = grid[(size_t) r0 * (size_t) W + (size_t) c1];
        const float v01 = grid[(size_t) r1 * (size_t) W + (size_t) c0];
        const float v11 = grid[(size_t) r1 * (size_t) W + (size_t) c1];
        const float a = v00 + (v10 - v00) * tx;
        const float b = v01 + (v11 - v01) * tx;
        return a + (b - a) * ty;
    };

    relDb = hasRel ? sample (result_.splRelDB)
                   : (hasDisp ? sample (result_.splDB) : 0.0f);
    if (hasAbs)
        absDb = sample (result_.splAbsDB);
    else if (result_.hasAbsoluteSpl)
        absDb = (float) result_.peakAbsDb + relDb;
    else
        absDb = relDb;

    return true;
}

bool RadiationPatternComponent::updateSplProbeAt (juce::Point<float> screenPt)
{
    splProbeValid_ = false;
    if (! showSplProbe_)
        return false;
    if (tool_ != Tool::Select)
        return false;
    if (! hasData_ || params_.viewMode == ViewMode::Directivity
                   || params_.viewMode == ViewMode::MeasuredPolar)
        return false;

    const auto pb = plotArea();
    if (! pb.toFloat().contains (screenPt))
        return false;

    auto w = screenToWorld (screenPt.x, screenPt.y);
    if (w.x < 0.0f || w.y < 0.0f
        || w.x > (float) result_.worldW || w.y > (float) result_.worldH)
        return false;

    float absDb = 0.0f, relDb = 0.0f;
    if (! sampleSplAtWorld (w.x, w.y, absDb, relDb))
        return false;

    splProbeValid_ = true;
    splProbeScreen_ = screenPt;
    splProbeWorld_ = w;
    splProbeAbsDb_ = absDb;
    splProbeRelDb_ = relDb;
    return true;
}

void RadiationPatternComponent::drawSplProbe (juce::Graphics& g)
{
    if (! showSplProbe_ || ! splProbeValid_)
        return;

    // Crosshair at sample point
    g.setColour (Brand::white().withAlpha (0.85f));
    g.drawLine (splProbeScreen_.x - 7.0f, splProbeScreen_.y,
                splProbeScreen_.x + 7.0f, splProbeScreen_.y, 1.2f);
    g.drawLine (splProbeScreen_.x, splProbeScreen_.y - 7.0f,
                splProbeScreen_.x, splProbeScreen_.y + 7.0f, 1.2f);
    g.drawEllipse (splProbeScreen_.x - 4.0f, splProbeScreen_.y - 4.0f, 8.0f, 8.0f, 1.2f);

    juce::String line1;
    if (result_.hasAbsoluteSpl)
        line1 = juce::String (splProbeAbsDb_, 1) + " dB SPL";
    else
        line1 = juce::String (splProbeRelDb_, 1) + " dB (rel.)";

    const juce::String line2 = "(" + juce::String (splProbeWorld_.x, 1) + ", "
                             + juce::String (splProbeWorld_.y, 1) + ") m"
                             + (result_.hasAbsoluteSpl
                                    ? ("   " + juce::String (splProbeRelDb_, 1) + " dB rel")
                                    : juce::String());

    g.setFont (Brand::tech (Brand::UI::scaledFont (11.0f), true));
    const float pad = 8.0f;
    const float tw = (float) juce::jmax (line1.length(), line2.length()) * 7.2f + pad * 2.0f;
    const float th = 36.0f;
    float lx = splProbeScreen_.x + 14.0f;
    float ly = splProbeScreen_.y - th - 8.0f;
    const auto pb = plotArea().toFloat();
    if (lx + tw > pb.getRight() - 4.0f)
        lx = splProbeScreen_.x - tw - 14.0f;
    if (ly < pb.getY() + 4.0f)
        ly = splProbeScreen_.y + 14.0f;

    auto box = juce::Rectangle<float> (lx, ly, tw, th);
    g.setColour (Brand::panelDark().withAlpha (0.92f));
    g.fillRoundedRectangle (box, 5.0f);
    g.setColour (Brand::accent().withAlpha (0.7f));
    g.drawRoundedRectangle (box, 5.0f, 1.0f);
    g.setColour (Brand::white());
    g.drawText (line1, box.withTrimmedTop (2.0f).withTrimmedBottom (th * 0.45f).toNearestInt(),
                juce::Justification::centred, false);
    g.setColour (Brand::muted());
    g.setFont (Brand::tech (Brand::UI::scaledFont (10.0f), false));
    g.drawText (line2, box.withTrimmedTop (th * 0.48f).withTrimmedBottom (2.0f).toNearestInt(),
                juce::Justification::centred, false);
}

void RadiationPatternComponent::updateRubberBandAt (juce::Point<float> screenPt)
{
    const bool tracking = (tool_ == Tool::Ruler && pendingAnchor_)
                       || (tool_ == Tool::Shape && sessionActive_ && ! sessionPts_.empty());
    if (! tracking)
        return;

    const bool inPlot = (currentAnnotSpace() == AnnotSpace::PolarPlot)
                            ? getLocalBounds().toFloat().contains (screenPt)
                            : plotArea().toFloat().contains (screenPt);
    if (! canAnnotate() || ! inPlot)
    {
        hoverValid_ = false;
        return;
    }

    hoverAnnot_ = screenToAnnot (screenPt.x, screenPt.y);
    hoverValid_ = true;
}

bool RadiationPatternComponent::tryFinishRubberBandAt (juce::Point<float> screenPt)
{
    // Click-drag-release: finish 2-point shapes / ruler when the drag travelled enough.
    // Use screen pixels — world minDist (~5 cm) is sub-pixel at Fit View, so a plain
    // click's mouseUp used to commit an invisible micro-line and eat the first stroke.
    constexpr float kMinDragPx = 6.0f;
    if (screenPt.getDistanceFrom (rubberBandStartScreen_) < kMinDragPx)
        return false;

    updateRubberBandAt (screenPt);
    if (! hoverValid_)
        return false;

    if (tool_ == Tool::Ruler && pendingAnchor_)
    {
        auto annot = applyOrtho (pendingStartWorld_, hoverAnnot_);
        annot = snapAnnotPoint (annot);
        if (pendingStartWorld_.getDistanceFrom (annot) <= 1.0e-6f)
            return false;
        if (onWillEdit) onWillEdit();
        Annotation a;
        a.kind = Annotation::Kind::Measure;
        a.space = currentAnnotSpace();
        a.colour = drawColour_;
        a.thicknessPx = 1.8f;
        a.pts = { pendingStartWorld_, annot };
        annotations_.push_back (std::move (a));
        if (onEditCommitted) onEditCommitted();
        pendingAnchor_ = false;
        hoverValid_ = false;
        updateDrawPrompt();
        repaint();
        return true;
    }

    if (tool_ == Tool::Shape && sessionActive_
        && drawShape_ != DrawShape::Polyline
        && drawShape_ != DrawShape::Arc
        && sessionPts_.size() == 1
        && pointsNeeded() == 2)
    {
        auto p = applyOrtho (sessionPts_.back(), hoverAnnot_);
        p = snapAnnotPoint (p);
        if (sessionPts_.front().getDistanceFrom (p) <= 1.0e-6f)
            return false;
        acceptAnnotPoint (p);
        return true;
    }

    return false;
}

bool RadiationPatternComponent::pointHitsShape (juce::Point<float> pt,
                                                const Annotation& a,
                                                float radius) noexcept
{
    if (a.pts.size() < 2) return false;

    if (a.kind == Annotation::Kind::Circle)
    {
        const auto& c = a.pts[0];
        const float r = c.getDistanceFrom (a.pts[1]);
        return pt.getDistanceFrom (c) <= r + radius;
    }

    if (a.kind == Annotation::Kind::Arc && a.pts.size() >= 3)
    {
        const auto& c = a.pts[0];
        const float r = c.getDistanceFrom (a.pts[1]);
        return std::abs (pt.getDistanceFrom (c) - r) <= radius;
    }

    if (a.kind == Annotation::Kind::Polyline)
    {
        for (size_t i = 1; i < a.pts.size(); ++i)
            if (distPointToSegment (pt, a.pts[i - 1], a.pts[i]) <= radius)
                return true;
        return false;
    }

    const auto r = normalisedShapeRect (a.pts[0], a.pts[1], a.kind);
    if (r.getWidth() < 1.0e-6f || r.getHeight() < 1.0e-6f)
        return pt.getDistanceFrom (r.getCentre()) <= radius;

    return r.expanded (radius).contains (pt);
}

void RadiationPatternComponent::drawShapeAnnotation (juce::Graphics& g,
                                                     const Annotation& a,
                                                     float alphaMul)
{
    if (a.pts.size() < 2) return;

    // Isolate from any leftover Graphics::setOpacity (e.g. heatmap / layout).
    juce::Graphics::ScopedSaveState ss (g);
    g.setOpacity (1.0f);

    const float fillA = juce::jlimit (0.0f, 1.0f, a.fillAlpha) * alphaMul;
    const float strokeA = juce::jlimit (0.15f, 1.0f, 0.55f + 0.45f * a.fillAlpha) * alphaMul;
    // Rebuild from RGB so we never inherit a stale alpha channel on colour.
    const auto base = juce::Colour::fromFloatRGBA (a.colour.getFloatRed(),
                                                   a.colour.getFloatGreen(),
                                                   a.colour.getFloatBlue(),
                                                   1.0f);

    if (a.kind == Annotation::Kind::Circle)
    {
        const auto& c = a.pts[0];
        const float r = c.getDistanceFrom (a.pts[1]);
        if (r < 1.0e-6f) return;
        auto sc = annotateToScreen (a, c);
        float rx, ry;
        if (a.space == AnnotSpace::PolarPlot)
        {
            rx = r * polarRadius_;
            ry = r * polarRadius_;
        }
        else
        {
            rx = r * worldScaleX();
            ry = r * worldScaleY();
        }
        auto sr = juce::Rectangle<float> (sc.x - rx, sc.y - ry, rx * 2.0f, ry * 2.0f);
        g.setColour (base.withAlpha (fillA));
        g.fillEllipse (sr);
        g.setColour (base.withAlpha (strokeA));
        g.drawEllipse (sr, a.thicknessPx);
        return;
    }

    const auto wr = normalisedShapeRect (a.pts[0], a.pts[1], a.kind);
    if (wr.getWidth() < 1.0e-6f && wr.getHeight() < 1.0e-6f) return;

    auto s0 = annotateToScreen (a, { wr.getX(), wr.getY() });
    auto s1 = annotateToScreen (a, { wr.getRight(), wr.getBottom() });
    auto sr = juce::Rectangle<float>::leftTopRightBottom (
        juce::jmin (s0.x, s1.x), juce::jmin (s0.y, s1.y),
        juce::jmax (s0.x, s1.x), juce::jmax (s0.y, s1.y));

    g.setColour (base.withAlpha (fillA));
    g.fillRect (sr);
    g.setColour (base.withAlpha (strokeA));
    g.drawRect (sr, a.thicknessPx);
}

void RadiationPatternComponent::drawTextBoxAnnotation (juce::Graphics& g,
                                                       const Annotation& a,
                                                       float alphaMul)
{
    if (a.kind != Annotation::Kind::TextBox || a.pts.size() < 2) return;

    juce::Graphics::ScopedSaveState ss (g);
    g.setOpacity (1.0f);

    const auto local = textBoxLocalRect (a);
    if (local.getWidth() < 1.0e-6f || local.getHeight() < 1.0e-6f) return;

    const auto c = local.getCentre();
    const juce::Point<float> corners[4] = {
        rotateAround ({ local.getX(), local.getY() }, c, a.rotationDeg),
        rotateAround ({ local.getRight(), local.getY() }, c, a.rotationDeg),
        rotateAround ({ local.getRight(), local.getBottom() }, c, a.rotationDeg),
        rotateAround ({ local.getX(), local.getBottom() }, c, a.rotationDeg)
    };

    juce::Path path;
    auto s0 = annotateToScreen (a, corners[0]);
    path.startNewSubPath (s0);
    for (int i = 1; i < 4; ++i)
        path.lineTo (annotateToScreen (a, corners[i]));
    path.closeSubPath();

    const float fillA = juce::jlimit (0.0f, 1.0f, a.fillAlpha) * alphaMul;
    const float strokeA = juce::jlimit (0.2f, 1.0f, 0.55f + 0.45f * a.fillAlpha) * alphaMul;
    const auto base = juce::Colour::fromFloatRGBA (a.colour.getFloatRed(),
                                                   a.colour.getFloatGreen(),
                                                   a.colour.getFloatBlue(),
                                                   1.0f);

    g.setColour (base.withAlpha (fillA));
    g.fillPath (path);
    g.setColour (base.withAlpha (strokeA));
    g.strokePath (path, juce::PathStrokeType (juce::jmax (1.0f, a.thicknessPx)));

    const auto label = a.text.isNotEmpty() ? a.text : juce::String ("Text");
    auto sc = annotateToScreen (a, c);
    // Screen-space rotation: world +Y is up, screen +Y is down → negate.
    const float screenDeg = -a.rotationDeg;

    auto sA = annotateToScreen (a, corners[0]);
    auto sB = annotateToScreen (a, corners[1]);
    auto sD = annotateToScreen (a, corners[3]);
    const float wPx = sA.getDistanceFrom (sB);
    const float hPx = sA.getDistanceFrom (sD);
    if (wPx < 8.0f || hPx < 8.0f) return;

    const float pad = 4.0f;
    const float fontH = juce::jlimit (9.0f, 28.0f, hPx * 0.45f);
    g.setFont (Brand::tech (fontH, false));

    const bool lightBg = base.getPerceivedBrightness() > 0.55f;
    const auto ink = (lightBg ? juce::Colours::black : juce::Colours::white)
                         .withAlpha (juce::jlimit (0.35f, 1.0f, alphaMul));

    {
        juce::Graphics::ScopedSaveState textSs (g);
        g.addTransform (juce::AffineTransform::rotation (
            screenDeg * (float) M_PI / 180.0f, sc.x, sc.y));
        auto box = juce::Rectangle<float> (sc.x - wPx * 0.5f + pad,
                                           sc.y - hPx * 0.5f + pad,
                                           juce::jmax (1.0f, wPx - pad * 2.0f),
                                           juce::jmax (1.0f, hPx - pad * 2.0f));
        g.setColour (ink);
        g.drawFittedText (label, box.toNearestInt(),
                          juce::Justification::centred, 4, 0.8f);
    }
}

void RadiationPatternComponent::drawArcAnnotation (juce::Graphics& g,
                                                   const Annotation& a,
                                                   float alphaMul)
{
    // pts: center, start, end, mid-on-arc
    if (a.pts.size() < 4) return;
    const auto& c = a.pts[0];
    const float r = c.getDistanceFrom (a.pts[1]);
    if (r < 1.0e-6f) return;

    auto angOf = [&] (juce::Point<float> p) -> float
    {
        return std::atan2 (p.y - c.y, p.x - c.x);
    };
    float a0 = angOf (a.pts[1]);
    float a1 = angOf (a.pts[2]);
    float am = angOf (a.pts[3]);

    // Sweep from a0 to a1 through am
    auto norm = [] (float x)
    {
        while (x < 0.0f) x += (float) (2.0 * M_PI);
        while (x >= (float) (2.0 * M_PI)) x -= (float) (2.0 * M_PI);
        return x;
    };
    a0 = norm (a0); a1 = norm (a1); am = norm (am);

    auto betweenCCW = [] (float s, float e, float m)
    {
        float se = e - s; if (se < 0) se += (float) (2.0 * M_PI);
        float sm = m - s; if (sm < 0) sm += (float) (2.0 * M_PI);
        return sm <= se + 1.0e-4f;
    };
    const bool ccw = betweenCCW (a0, a1, am);
    float sweep = a1 - a0;
    if (ccw) { if (sweep < 0) sweep += (float) (2.0 * M_PI); }
    else     { if (sweep > 0) sweep -= (float) (2.0 * M_PI); }

    juce::Path path;
    const int n = juce::jmax (8, (int) std::ceil (std::abs (sweep) / (float) (M_PI / 36.0)));
    for (int i = 0; i <= n; ++i)
    {
        const float t = (float) i / (float) n;
        const float ang = a0 + sweep * t;
        const juce::Point<float> wp { c.x + r * std::cos (ang), c.y + r * std::sin (ang) };
        auto sp = annotateToScreen (a, wp);
        if (i == 0) path.startNewSubPath (sp);
        else        path.lineTo (sp);
    }
    g.setColour (a.colour.withMultipliedAlpha (alphaMul));
    g.strokePath (path, juce::PathStrokeType (a.thicknessPx,
                                               juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
}

void RadiationPatternComponent::updateMouseCursorForTool()
{
    if (addMicArmed_)
    {
        setMouseCursor (juce::MouseCursor::CrosshairCursor);
        return;
    }
    switch (tool_)
    {
        case Tool::Select: setMouseCursor (juce::MouseCursor::NormalCursor); break;
        case Tool::Pan:    setMouseCursor (juce::MouseCursor::DraggingHandCursor); break;
        case Tool::Pencil:
        case Tool::Eraser:
        case Tool::Ruler:
        case Tool::Shape:  setMouseCursor (juce::MouseCursor::CrosshairCursor); break;
    }
}

// ---------------------------------------------------------------------------
void RadiationPatternComponent::updateData (const SimResult& result, const SimParams& params)
{
    result_ = result;
    params_ = params;
    // No active units → keep the plot empty ("Add a Q21S unit…") instead of a blank heatmap.
    hasData_ = (result_.width > 0 && result_.activeSpeakers > 0);
    // First load, or still at default Fit View: fill the whole plot pane.
    if (! viewInit_ || std::abs (zoom_ - 1.0f) < 0.02f)
        fitView();
    buildImage();
    refreshMicLevels();
    if (onMicsChanged) onMicsChanged();
    repaint();
}

void RadiationPatternComponent::setSpeakers (const std::vector<Speaker>& speakers, int selectedIndex)
{
    speakers_ = speakers;
    selected_ = selectedIndex;
    repaint();
}

void RadiationPatternComponent::selectOnlySpeaker (int index)
{
    selected_ = index;
    selectedSpeakers_.clear();
    if (index >= 0 && index < (int) speakers_.size())
        selectedSpeakers_.push_back (index);
    repaint();
}

bool RadiationPatternComponent::hasCopyableSelection() const noexcept
{
    return ! selectedAnnots_.empty()
        || ! selectedMics_.empty()
        || ! selectedSpeakers_.empty();
}

bool RadiationPatternComponent::hasClipboardContent() const noexcept
{
    return ! clipboard_.empty();
}

bool RadiationPatternComponent::copySelection()
{
    clipboard_ = {};
    pasteGeneration_ = 0;

    for (int idx : selectedAnnots_)
        if (idx >= 0 && idx < (int) annotations_.size())
            clipboard_.annots.push_back (annotations_[(size_t) idx]);

    for (int idx : selectedMics_)
        if (idx >= 0 && idx < (int) mics_.size())
            clipboard_.mics.push_back (mics_[(size_t) idx]);

    for (int idx : selectedSpeakers_)
        if (idx >= 0 && idx < (int) speakers_.size())
            clipboard_.speakers.push_back (speakers_[(size_t) idx]);

    return ! clipboard_.empty();
}

bool RadiationPatternComponent::pasteClipboard()
{
    if (clipboard_.empty())
        return false;

    ++pasteGeneration_;
    const float worldNudge = 1.0f * (float) pasteGeneration_;
    const float polarNudge = 0.05f * (float) pasteGeneration_;

    if (onWillEdit) onWillEdit();

    clearPlotSelection();

    std::vector<int> newAnnots, newMics, newSpeakers;

    for (auto a : clipboard_.annots)
    {
        const float dx = (a.space == AnnotSpace::PolarPlot) ? polarNudge : worldNudge;
        const float dy = dx;
        for (auto& p : a.pts)
        {
            p.x += dx;
            p.y += dy;
        }
        newAnnots.push_back ((int) annotations_.size());
        annotations_.push_back (std::move (a));
    }

    for (auto m : clipboard_.mics)
    {
        m.id = nextMicId_++;
        m.x = juce::jlimit (0.0f, (float) result_.worldW, m.x + worldNudge);
        m.y = juce::jlimit (0.0f, (float) result_.worldH, m.y + worldNudge);
        m.ringLocked = false;
        m.ringSpeaker = -1;
        m.levelOk = false;
        newMics.push_back ((int) mics_.size());
        mics_.push_back (std::move (m));
    }

    if (! clipboard_.speakers.empty() && onPasteSpeakers != nullptr)
    {
        std::vector<Speaker> toAdd;
        toAdd.reserve (clipboard_.speakers.size());
        for (auto s : clipboard_.speakers)
        {
            s.x = juce::jlimit (0.0f, (float) result_.worldW, s.x + worldNudge);
            s.y = juce::jlimit (0.0f, (float) result_.worldH, s.y + worldNudge);
            toAdd.push_back (s);
        }
        newSpeakers = onPasteSpeakers (std::move (toAdd));
        // Control panel notify refreshes speakers_ via syncRenderer — re-read selection.
    }

    selectedAnnots_ = std::move (newAnnots);
    selectedMics_ = std::move (newMics);
    selectedSpeakers_ = std::move (newSpeakers);
    syncPrimarySelectionFromSets();

    if (! selectedMics_.empty())
    {
        refreshMicLevels();
        if (onMicsChanged) onMicsChanged();
    }
    if (! selectedSpeakers_.empty() && onSpeakerSelected)
        onSpeakerSelected (selectedSpeakers_.back());
    if (onAnnotSelectionChanged) onAnnotSelectionChanged();
    if (onEditCommitted) onEditCommitted();
    repaint();
    return true;
}

void RadiationPatternComponent::showSelectionContextMenu (juce::Point<int> screenPos)
{
    juce::PopupMenu m;
    const bool canEdit = hasCopyableSelection();
    m.addItem (1, "Copy",   canEdit);
    m.addItem (2, "Paste",  hasClipboardContent()); // off when clipboard empty
    m.addSeparator();
    m.addItem (3, "Delete", canEdit);
    m.showMenuAsync (juce::PopupMenu::Options()
                         .withTargetScreenArea ({ screenPos.x, screenPos.y, 1, 1 }),
                     [safe = juce::Component::SafePointer<RadiationPatternComponent> (this)] (int result)
                     {
                         if (safe == nullptr || result <= 0) return;
                         if (result == 1)
                             safe->copySelection();
                         else if (result == 2)
                             safe->pasteClipboard();
                         else if (result == 3)
                             safe->deleteSelection();
                     });
}

bool RadiationPatternComponent::deleteSelection()
{
    if (! hasCopyableSelection())
        return false;
    if (sessionActive_ || pendingAnchor_)
        return false;

    if (onWillEdit) onWillEdit();

    if (! selectedAnnots_.empty())
    {
        std::vector<int> order = selectedAnnots_;
        std::sort (order.begin(), order.end(), std::greater<int>());
        for (int idx : order)
            if (idx >= 0 && idx < (int) annotations_.size())
                annotations_.erase (annotations_.begin() + idx);
        selectedAnnots_.clear();
        selectedAnnot_ = -1;
    }

    if (! selectedMics_.empty())
    {
        std::vector<int> order = selectedMics_;
        std::sort (order.begin(), order.end(), std::greater<int>());
        for (int idx : order)
            if (idx >= 0 && idx < (int) mics_.size())
                mics_.erase (mics_.begin() + idx);
        selectedMics_.clear();
        selectedMic_ = -1;
        if (onMicsChanged) onMicsChanged();
    }

    if (! selectedSpeakers_.empty() && onDeleteSpeakers != nullptr)
    {
        auto toRemove = selectedSpeakers_;
        selectedSpeakers_.clear();
        selected_ = -1;
        onDeleteSpeakers (std::move (toRemove));
    }
    else
    {
        selectedSpeakers_.clear();
    }

    syncPrimarySelectionFromSets();
    if (onEditCommitted) onEditCommitted();
    if (onAnnotSelectionChanged) onAnnotSelectionChanged();
    repaint();
    return true;
}

void RadiationPatternComponent::setMeasuredData (const MeasuredSet& measured)
{
    measured_ = measured;
    if (params_.viewMode == ViewMode::MeasuredPolar) repaint();
}

void RadiationPatternComponent::setMeasuredFrequency (int hz)
{
    if (measuredHz_ == hz) return;
    measuredHz_ = hz;
    if (params_.viewMode == ViewMode::MeasuredPolar
        || params_.viewMode == ViewMode::Directivity) repaint();
}

void RadiationPatternComponent::setMeasuredDistance (float distanceM)
{
    if (std::abs (measuredDistanceM_ - distanceM) < 1.0e-4f) return;
    measuredDistanceM_ = distanceM;
    if (params_.viewMode == ViewMode::MeasuredPolar
        || params_.viewMode == ViewMode::Directivity) repaint();
}

const MeasuredFreq* RadiationPatternComponent::measuredForHz (int hz) const
{
    for (const auto& mf : measured_.freqs)
        if (mf.hz == hz) return &mf;
    return nullptr;
}

bool RadiationPatternComponent::showingBemHeatmap() const noexcept
{
    return params_.viewMode == ViewMode::MeasuredPolar
        && hasData_
        && result_.usedBemField
        && result_.measuredDirectivityHz == measuredHz_
        && ! result_.splDB.empty();
}

// ---------------------------------------------------------------------------
juce::Rectangle<int> RadiationPatternComponent::plotArea() const
{
    return getLocalBounds().withTrimmedRight (kColourbarW + 8);
}

float RadiationPatternComponent::worldScaleX() const
{
    return baseScaleX_ * zoom_;
}

float RadiationPatternComponent::worldScaleY() const
{
    return baseScaleY_ * zoom_;
}

float RadiationPatternComponent::worldScale() const
{
    return 0.5f * (worldScaleX() + worldScaleY());
}

juce::Point<float> RadiationPatternComponent::worldToScreen (float wx, float wy) const
{
    const auto pb = plotArea();
    return { (float) pb.getX() + origin_.x + wx * worldScaleX(),
             (float) pb.getY() + origin_.y + ((float) result_.worldH - wy) * worldScaleY() };
}

juce::Point<float> RadiationPatternComponent::screenToWorld (float sx, float sy) const
{
    const auto pb = plotArea();
    const float sxScale = worldScaleX();
    const float syScale = worldScaleY();
    return { (sx - (float) pb.getX() - origin_.x) / sxScale,
             (float) result_.worldH - (sy - (float) pb.getY() - origin_.y) / syScale };
}

void RadiationPatternComponent::fitView()
{
    const auto pb = plotArea();
    const double ww = (result_.worldW > 0 ? result_.worldW : params_.worldW);
    const double wh = (result_.worldH > 0 ? result_.worldH : params_.worldH);
    if (pb.getWidth() <= 0 || pb.getHeight() <= 0 || ww <= 0 || wh <= 0) return;

    // Uniform px/m so metre grid cells stay square. Cover the plot (may crop
    // one axis); origin (0, 0) is world bottom-left on screen.
    const float sx = (float) (pb.getWidth()  / ww);
    const float sy = (float) (pb.getHeight() / wh);
    const float s  = juce::jmax (sx, sy);
    baseScaleX_ = s;
    baseScaleY_ = s;
    zoom_       = 1.0f;

    const float worldPxW = (float) ww * s;
    const float worldPxH = (float) wh * s;
    origin_ = { 0.5f * ((float) pb.getWidth()  - worldPxW),
                0.5f * ((float) pb.getHeight() - worldPxH) };
    viewInit_   = true;
    clampViewToField();
}

void RadiationPatternComponent::clampViewToField()
{
    const auto pb = plotArea();
    const double ww = (result_.worldW > 0 ? result_.worldW : params_.worldW);
    const double wh = (result_.worldH > 0 ? result_.worldH : params_.worldH);
    if (pb.getWidth() <= 0 || pb.getHeight() <= 0 || ww <= 0 || wh <= 0) return;

    if (zoom_ < 1.0f)
        zoom_ = 1.0f;

    const float worldPxW = (float) ww * worldScaleX();
    const float worldPxH = (float) wh * worldScaleY();
    const float viewW = (float) pb.getWidth();
    const float viewH = (float) pb.getHeight();

    if (worldPxW >= viewW)
        origin_.x = juce::jlimit (viewW - worldPxW, 0.0f, origin_.x);
    else
        origin_.x = 0.0f;

    if (worldPxH >= viewH)
        origin_.y = juce::jlimit (viewH - worldPxH, 0.0f, origin_.y);
    else
        origin_.y = 0.0f;
}

void RadiationPatternComponent::resetView()
{
    viewInit_ = false;
    fitView();
    repaint();
}

void RadiationPatternComponent::zoomIn()
{
    if (! hasData_ || params_.viewMode == ViewMode::Directivity
                   || (params_.viewMode == ViewMode::MeasuredPolar && ! showingBemHeatmap())) return;
    const auto pb = plotArea();
    const float cx = (float) pb.getCentreX();
    const float cy = (float) pb.getCentreY();
    const float newZoom = juce::jlimit (1.0f, kMaxZoom, zoom_ * 1.2f);
    if (std::abs (newZoom - zoom_) < 1e-6f) return;
    auto worldUnder = screenToWorld (cx, cy);
    zoom_ = newZoom;
    origin_.x = cx - (float) pb.getX() - worldUnder.x * worldScaleX();
    origin_.y = cy - (float) pb.getY() - ((float) result_.worldH - worldUnder.y) * worldScaleY();
    clampViewToField();
    repaint();
}

void RadiationPatternComponent::zoomOut()
{
    if (! hasData_ || params_.viewMode == ViewMode::Directivity
                   || (params_.viewMode == ViewMode::MeasuredPolar && ! showingBemHeatmap())) return;
    const auto pb = plotArea();
    const float cx = (float) pb.getCentreX();
    const float cy = (float) pb.getCentreY();
    const float newZoom = juce::jlimit (1.0f, kMaxZoom, zoom_ / 1.2f);
    if (std::abs (newZoom - zoom_) < 1e-6f) return;
    auto worldUnder = screenToWorld (cx, cy);
    zoom_ = newZoom;
    origin_.x = cx - (float) pb.getX() - worldUnder.x * worldScaleX();
    origin_.y = cy - (float) pb.getY() - ((float) result_.worldH - worldUnder.y) * worldScaleY();
    clampViewToField();
    repaint();
}

void RadiationPatternComponent::resized()
{
    const auto pb = plotArea();
    const double ww = (result_.worldW > 0 ? result_.worldW : params_.worldW);
    const double wh = (result_.worldH > 0 ? result_.worldH : params_.worldH);
    if (pb.getWidth() <= 0 || pb.getHeight() <= 0 || ww <= 0 || wh <= 0) return;

    if (! viewInit_ || std::abs (zoom_ - 1.0f) < 0.02f)
    {
        fitView();
        return;
    }

    const float cx = (float) pb.getCentreX();
    const float cy = (float) pb.getCentreY();
    const auto worldUnder = screenToWorld (cx, cy);
    const float sx = (float) (pb.getWidth()  / ww);
    const float sy = (float) (pb.getHeight() / wh);
    const float s  = juce::jmax (sx, sy);
    baseScaleX_ = s;
    baseScaleY_ = s;
    origin_.x = cx - (float) pb.getX() - worldUnder.x * worldScaleX();
    origin_.y = cy - (float) pb.getY() - ((float) result_.worldH - worldUnder.y) * worldScaleY();
    clampViewToField();
}

// ---------------------------------------------------------------------------
void RadiationPatternComponent::buildImage()
{
    if (! hasData_) return;

    const int W = result_.width;
    const int H = result_.height;
    if (W <= 0 || H <= 0) return;

    fieldImage_ = juce::Image (juce::Image::RGB, W, H, false);
    juce::Image::BitmapData bm (fieldImage_, juce::Image::BitmapData::writeOnly);

    const auto mode = params_.viewMode;

    for (int row = 0; row < H; ++row)
    {
        // Engine row 0 = world Y = 0 (bottom); image row 0 = top (max Y).
        const int imgRow = H - 1 - row;
        for (int col = 0; col < W; ++col)
        {
            const size_t idx = (size_t) row * W + col;
            juce::Colour c;

            // SPL heatmap — 7-color scale (black→blue→cyan→green→yellow→orange→red).
            {
                const float dB = (result_.splRelDB.size() == (size_t) W * (size_t) H)
                                    ? result_.splRelDB[idx]
                                    : result_.splDB[idx];
                if (params_.bandedSPL)
                {
                    // Contour bands: fixed 3 dB steps. db Floor only clips the bottom.
                    c = ColourMaps::splBandForFloor (dB, (float) params_.dBfloor, 3.0f);
                }
                else
                {
                    // Continuous: fixed 0…−36 colour span (−6 dB always same hue).
                    // db Floor only blacks out levels at/below the floor.
                    const float t = ColourMaps::relDbToColourT (dB, (float) params_.dBfloor);
                    c = ColourMaps::sevenColor (t);
                }
            }

            bm.setPixelColour (col, imgRow, c);
        }
    }
}

// ---------------------------------------------------------------------------
void RadiationPatternComponent::paint (juce::Graphics& g)
{
    // Theme canvas: black in dark mode, warm light in light mode (mockup grid).
    g.fillAll (Brand::plotBg());

    // Measured polar view is independent of the simulation result.
    if (params_.viewMode == ViewMode::MeasuredPolar)
    {
        drawMeasuredPolar (g, getLocalBounds());
        drawAnnotations (g, getLocalBounds());
        drawMarqueeOverlay (g);
        return;
    }

    if (! hasData_)
    {
        g.setColour (Brand::ash());
        g.drawText ("Add a Q21S unit and press RUN", getLocalBounds(),
                    juce::Justification::centred);
        return;
    }

    const auto pb = plotArea();

    if (params_.viewMode == ViewMode::Directivity)
    {
        drawPolarPlot (g, getLocalBounds());
        drawAnnotations (g, getLocalBounds());
        drawMarqueeOverlay (g);
        return;
    }

    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (pb);
        drawField    (g, pb);
        drawLayout   (g, pb);
        drawGrid     (g, pb);
        drawSpeakers (g, pb);
        drawAnnotations (g, pb);
        drawMics (g);
        drawSplProbe (g);
        drawMarqueeOverlay (g);
    }

    drawColourbar (g, getLocalBounds().withTrimmedLeft (pb.getWidth() + 8));
}

// ---------------------------------------------------------------------------
void RadiationPatternComponent::drawField (juce::Graphics& g, juce::Rectangle<int>)
{
    if (! fieldImage_.isValid()) return;

    const auto tl = worldToScreen (0.0f, (float) result_.worldH);   // top-left
    const float w = (float) result_.worldW * worldScaleX();
    const float h = (float) result_.worldH * worldScaleY();

    // Smooth gradient (EASE-style) interpolates; banded contours stay crisp.
    const bool banded = (params_.viewMode == ViewMode::SPL && params_.bandedSPL);
    juce::Graphics::ScopedSaveState ss (g);
    g.setImageResamplingQuality (banded ? juce::Graphics::lowResamplingQuality
                                        : juce::Graphics::highResamplingQuality);
    g.setOpacity (1.0f);
    g.drawImage (fieldImage_,
                 juce::Rectangle<float> (tl.x, tl.y, w, h),
                 juce::RectanglePlacement::stretchToFit);
}

// Pick a "nice" 1 / 2 / 5 x 10^n step (in metres) closest to the requested
// raw spacing, so grid density stays readable at any zoom level.
double RadiationPatternComponent::niceStep (double raw)
{
    if (raw <= 0.0) return 1.0;
    const double mag  = std::pow (10.0, std::floor (std::log10 (raw)));
    const double norm = raw / mag;                       // 1 .. 10
    const double n = (norm < 1.5) ? 1.0 : (norm < 3.5) ? 2.0 : (norm < 7.5) ? 5.0 : 10.0;
    return n * mag;
}

RadiationPatternComponent::GridMetrics RadiationPatternComponent::currentGridMetrics() const
{
    // Aim for ~20 px minor / ~90 px major so cells stay readable; floor at 1 mm.
    const double pxPerM = (double) worldScaleX();
    GridMetrics m;
    if (pxPerM <= 1.0e-6)
        return m;

    m.minor = niceStep (20.0 / pxPerM);
    if (m.minor < kMinGridM)
        m.minor = kMinGridM;

    m.major = niceStep (90.0 / pxPerM);
    if (m.major < m.minor)
        m.major = m.minor;

    // Keep majors on a clean multiple of minors (5× or 10× preferred).
    const double ratio = m.major / m.minor;
    if (ratio < 2.5)
        m.major = m.minor * 5.0;
    else if (ratio < 7.5)
        m.major = m.minor * 5.0;
    else
        m.major = m.minor * 10.0;

    return m;
}

juce::String RadiationPatternComponent::formatGridLabel (double metres)
{
    const double a = std::abs (metres);
    if (a < 1.0e-12)
        return "0";

    if (a + 1.0e-12 < 0.01)
        return juce::String ((int) std::lround (metres * 1000.0)) + " mm";

    if (a + 1.0e-12 < 1.0)
    {
        const double cm = metres * 100.0;
        if (std::abs (cm - std::round (cm)) < 1.0e-6)
            return juce::String ((int) std::lround (cm)) + " cm";
        return juce::String (metres, 3) + " m";
    }

    if (std::abs (metres - std::round (metres)) < 1.0e-6)
        return juce::String ((int) std::lround (metres)) + " m";

    return juce::String (metres, 2) + " m";
}

void RadiationPatternComponent::drawGrid (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    const float ww = (float) result_.worldW;
    const float wh = (float) result_.worldH;

    if (! showGrid_) return;

    const auto metrics = currentGridMetrics();
    const double minorStep = metrics.minor;
    const double majorStep = metrics.major;
    if (minorStep <= 0.0 || majorStep <= 0.0) return;

    // Only draw lines that intersect the visible plot (needed at 1 mm density).
    const auto tl = screenToWorld ((float) bounds.getX(),      (float) bounds.getY());
    const auto br = screenToWorld ((float) bounds.getRight(),  (float) bounds.getBottom());
    const double visX0 = juce::jlimit (0.0, (double) ww, (double) std::min (tl.x, br.x));
    const double visX1 = juce::jlimit (0.0, (double) ww, (double) std::max (tl.x, br.x));
    const double visY0 = juce::jlimit (0.0, (double) wh, (double) std::min (tl.y, br.y));
    const double visY1 = juce::jlimit (0.0, (double) wh, (double) std::max (tl.y, br.y));

    const double xMin = 0.0;
    const double xMax = (double) ww;
    const double yMin = 0.0;
    const double yMax = (double) wh;

    const juce::Colour minorCol = Brand::plotGrid().withMultipliedAlpha (0.45f);
    const juce::Colour majorCol = Brand::plotGrid();
    const juce::Colour labelInk = Brand::axisLabel();

    auto isMajor = [&] (double v)
    {
        return std::abs (std::remainder (v, majorStep)) < minorStep * 0.25;
    };

    auto forTicks = [] (double lo, double hi, double step, const std::function<void(double)>& fn)
    {
        if (step <= 0.0) return;
        const long i0 = (long) std::ceil  ((lo - 1.0e-12) / step);
        const long i1 = (long) std::floor ((hi + 1.0e-12) / step);
        for (long i = i0; i <= i1; ++i)
            fn ((double) i * step);
    };

    auto vline = [&] (double xm, bool major)
    {
        auto a = worldToScreen ((float) xm, (float) yMin);
        auto b = worldToScreen ((float) xm, (float) yMax);
        g.setColour (major ? majorCol : minorCol);
        g.drawLine (a.x, a.y, b.x, b.y, major ? 1.0f : 0.6f);
    };
    auto hline = [&] (double ym, bool major)
    {
        auto a = worldToScreen ((float) xMin, (float) ym);
        auto b = worldToScreen ((float) xMax, (float) ym);
        g.setColour (major ? majorCol : minorCol);
        g.drawLine (a.x, a.y, b.x, b.y, major ? 1.0f : 0.6f);
    };

    forTicks (visX0, visX1, minorStep, [&] (double x) { if (! isMajor (x)) vline (x, false); });
    forTicks (visY0, visY1, minorStep, [&] (double y) { if (! isMajor (y)) hline (y, false); });
    forTicks (visX0, visX1, majorStep, [&] (double x) { vline (x, true); });
    forTicks (visY0, visY1, majorStep, [&] (double y) { hline (y, true); });

    // Axis labels follow major spacing so zoom reveals 62, 63, 64… then cm/mm.
    g.setFont (Brand::techMed (Brand::Type::gridNum));
    g.setColour (labelInk);
    constexpr int labelW = 56;
    forTicks (visX0, visX1, majorStep, [&] (double x)
    {
        auto a = worldToScreen ((float) x, 0.0f);
        if (a.x < (float) bounds.getX() - 4.0f || a.x > (float) bounds.getRight() + 4.0f)
            return;
        const int chipX = juce::jlimit (bounds.getX() + 2, bounds.getRight() - labelW, (int) a.x + 2);
        const int chipY = bounds.getBottom() - 16;
        g.drawText (formatGridLabel (x),
                    juce::Rectangle<int> (chipX, chipY, labelW, 14),
                    juce::Justification::left);
    });
    forTicks (visY0, visY1, majorStep, [&] (double y)
    {
        auto a = worldToScreen (0.0f, (float) y);
        if (a.y < (float) bounds.getY() - 4.0f || a.y > (float) bounds.getBottom() + 4.0f)
            return;
        const int chipY = juce::jlimit (bounds.getY() + 2, bounds.getBottom() - 16, (int) a.y - 7);
        g.drawText (formatGridLabel (y),
                    juce::Rectangle<int> (bounds.getX() + 2, chipY, labelW, 14),
                    juce::Justification::left);
    });
}

// ---------------------------------------------------------------------------
void RadiationPatternComponent::drawLayout (juce::Graphics& g, juce::Rectangle<int>)
{
    if (layout_ == nullptr || ! layout_->valid() || ! layout_->visible) return;

    const float sx  = worldScaleX();
    const float sy  = worldScaleY();
    const float mpu = layout_->metresPerUnit();
    const float kx  = mpu * sx;
    const float ky  = mpu * sy;
    if (kx <= 0.0f || ky <= 0.0f) return;

    const auto& L = *layout_;
    const float x0 = L.srcBounds.getX(), y0 = L.srcBounds.getY();
    const float h  = L.srcBounds.getHeight();
    const float ox = L.originM.x, oy = L.originM.y;

    const auto pb = plotArea();
    const float bx = (float) pb.getX() + origin_.x + (ox - x0 * mpu) * sx;
    const float by = (float) pb.getY() + origin_.y
                   + ((float) result_.worldH - oy - (y0 + h) * mpu) * sy;

    juce::AffineTransform t (kx, 0.0f, bx, 0.0f, ky, by);
    if (std::abs (L.rotationDeg) > 0.01f)
    {
        auto p0 = worldToScreen (ox, oy);
        t = t.followedBy (juce::AffineTransform::rotation (
                juce::degreesToRadians (L.rotationDeg), p0.x, p0.y));
    }

    juce::Graphics::ScopedSaveState ss (g);
    if (L.kind == LayoutLayer::Kind::Image && L.image.isValid())
    {
        g.setOpacity (L.opacity);
        g.drawImageTransformed (L.image, t, false);
    }
    else if (L.kind == LayoutLayer::Kind::Dxf && ! L.path.isEmpty())
    {
        g.setColour (juce::Colour (0xff39c0ff).withAlpha (L.opacity));
        g.strokePath (L.path, juce::PathStrokeType (1.2f), t);
    }

    // Outline + handle when in edit mode so the user can see/grab it.
    if (layoutEditMode_)
    {
        juce::Path box;
        box.addRectangle (L.srcBounds);
        g.setColour (Brand::accent().withAlpha (0.9f));
        g.strokePath (box, juce::PathStrokeType (1.5f), t);
        auto p0 = worldToScreen (ox, oy);
        g.fillEllipse (p0.x - 5.0f, p0.y - 5.0f, 10.0f, 10.0f);
    }
}

juce::Rectangle<float> RadiationPatternComponent::speakerFootprintWorld (const Speaker& spk) const
{
    // Plan view: depth along X (firing), width along Y.
    const float hw = Q21SCabinet::widthM * 0.5f;
    const float hd = Q21SCabinet::depthM * 0.5f;
    return { spk.x - hd, spk.y - hw, Q21SCabinet::depthM, Q21SCabinet::widthM };
}

juce::Rectangle<float> RadiationPatternComponent::speakerFootprintScreen (const Speaker& spk) const
{
    const auto wr = speakerFootprintWorld (spk);
    const auto p0 = worldToScreen (wr.getX(), wr.getY());
    const auto p1 = worldToScreen (wr.getRight(), wr.getBottom());
    const float x = juce::jmin (p0.x, p1.x);
    const float y = juce::jmin (p0.y, p1.y);
    const float w = std::abs (p1.x - p0.x);
    const float h = std::abs (p1.y - p0.y);
    return { x, y, juce::jmax (1.0f, w), juce::jmax (1.0f, h) };
}

void RadiationPatternComponent::drawSpeakers (juce::Graphics& g, juce::Rectangle<int>)
{
    // Distance reference rings (1 / 2 / 4 / 8 m) — toggleable from the plot toolbar.
    if (showDistanceRings_)
    {
        static constexpr float kRingM[] = { 1.0f, 2.0f, 4.0f, 8.0f };
        for (const auto& spk : speakers_)
        {
            if (! spk.enabled) continue;
            const auto c = worldToScreen (spk.x, spk.y);
            for (float rm : kRingM)
            {
                const float rx = rm * worldScaleX();
                const float ry = rm * worldScaleY();
                g.setColour (Brand::white().withAlpha (0.55f));
                g.drawEllipse (c.x - rx, c.y - ry, rx * 2.0f, ry * 2.0f, 1.0f);
                g.setFont (Brand::mono (Brand::Type::gridNum));
                g.setColour (Brand::white().withAlpha (0.75f));
                const juce::String lab = juce::String ((int) rm) + " m";
                g.drawText (lab,
                            (int) (c.x + rx * 0.707f) + 4,
                            (int) (c.y - ry * 0.707f) - 10,
                            36, 14, juce::Justification::centredLeft, false);
            }
        }
    }

    // True Q21S plan footprint (750 mm W × 917 mm D). Selected = thick black boundary.
    for (int i = 0; i < (int) speakers_.size(); ++i)
    {
        const auto& spk = speakers_[i];
        const auto c = worldToScreen (spk.x, spk.y);
        const bool isSel = (i == selected_) || isSpeakerSelected (i);
        const float alpha = spk.enabled ? 1.0f : 0.42f;
        const bool reverse = spk.reverseOrientation;

        auto box = speakerFootprintScreen (spk);

        // Body fill first so selection glow can sit inside the footprint.
        g.setColour (Brand::white().withAlpha (0.92f * alpha));
        g.fillRect (box);

        if (isSel)
        {
            // Vivid magenta-red glow INSIDE the cabinet — keeps the outer border crisp.
            const auto glow = juce::Colour (0xffff3d6e);
            const float maxInset = juce::jmin (box.getWidth(), box.getHeight()) * 0.42f;
            for (int ring = 1; ring <= 5; ++ring)
            {
                const float inset = juce::jmin (maxInset, (float) ring * 2.2f);
                auto inner = box.reduced (inset);
                if (inner.getWidth() < 2.0f || inner.getHeight() < 2.0f)
                    break;
                // Stronger near the border, softer toward the centre.
                const float a = (0.28f - 0.045f * (float) ring) * alpha;
                g.setColour (glow.withAlpha (juce::jmax (0.04f, a)));
                g.drawRect (inner, 2.4f);
            }
            // Soft wash just inside the edge so the glow reads on hot SPL areas.
            g.setColour (glow.withAlpha (0.22f * alpha));
            g.fillRect (box.reduced (1.0f));
            g.setColour (Brand::white().withAlpha (0.88f * alpha));
            const float coreInset = juce::jmin (maxInset, 6.0f);
            auto core = box.reduced (coreInset);
            if (core.getWidth() > 2.0f && core.getHeight() > 2.0f)
                g.fillRect (core);
        }

        // Sharp outline drawn last so speaker borders stay clearly visible.
        g.setColour (isSel ? juce::Colour (0xffff3d6e).withAlpha (0.98f * alpha)
                           : Brand::charcoal().withAlpha (0.85f * alpha));
        g.drawRect (box, isSel ? 2.5f : 1.4f);

        // Facing chevron on the front face (+X = right when not reversed).
        {
            const float cy = box.getCentreY();
            const float inset = juce::jmin (box.getWidth(), box.getHeight()) * 0.18f;
            juce::Path tip;
            if (! reverse)
            {
                const float xFront = box.getRight() - inset;
                tip.addTriangle (xFront, cy,
                                 xFront - inset * 1.6f, cy - inset,
                                 xFront - inset * 1.6f, cy + inset);
            }
            else
            {
                const float xFront = box.getX() + inset;
                tip.addTriangle (xFront, cy,
                                 xFront + inset * 1.6f, cy - inset,
                                 xFront + inset * 1.6f, cy + inset);
            }
            g.setColour (Brand::charcoal().withAlpha (0.9f * alpha));
            g.fillPath (tip);
        }

        if (spk.polarityInverted)
        {
            const float d = juce::jlimit (4.0f, 8.0f, box.getWidth() * 0.12f);
            g.setColour (Brand::red().withAlpha (0.95f * alpha));
            g.fillEllipse (box.getRight() - d - 2.0f, box.getY() + 2.0f, d, d);
        }

        g.setColour (Brand::white().withAlpha (alpha));
        g.setFont (Brand::tech (isSel ? Brand::Type::speakerIdSelected
                                      : Brand::Type::speakerId, true));
        g.drawText ("Q21S_" + juce::String (i + 1),
                    (int) (c.x - 40), (int) (box.getY() - 16.0f), 80, 14,
                    juce::Justification::centred);
    }

    drawOrthoSpacingOverlay (g);
}

void RadiationPatternComponent::drawOrthoSpacingOverlay (juce::Graphics& g)
{
    if (! ortho_ || selectedSpeakers_.size() < 2)
        return;

    std::vector<int> idxs = selectedSpeakers_;
    std::sort (idxs.begin(), idxs.end());
    idxs.erase (std::unique (idxs.begin(), idxs.end()), idxs.end());
    idxs.erase (std::remove_if (idxs.begin(), idxs.end(),
                                [&] (int i) { return i < 0 || i >= (int) speakers_.size(); }),
                idxs.end());
    if (idxs.size() < 2)
        return;

    std::sort (idxs.begin(), idxs.end(), [&] (int a, int b)
    {
        const auto& sa = speakers_[(size_t) a];
        const auto& sb = speakers_[(size_t) b];
        if (orthoAlign_ == OrthoAlign::Horizontal)
            return sa.x < sb.x || (sa.x == sb.x && sa.y < sb.y);
        return sa.y < sb.y || (sa.y == sb.y && sa.x < sb.x);
    });

    const auto glow = juce::Colour (0xffff3d6e);
    g.setFont (Brand::tech (Brand::UI::scaledFont (11.0f), true));

    for (size_t i = 1; i < idxs.size(); ++i)
    {
        const auto& a = speakers_[(size_t) idxs[i - 1]];
        const auto& b = speakers_[(size_t) idxs[i]];
        const auto sa = worldToScreen (a.x, a.y);
        const auto sb = worldToScreen (b.x, b.y);
        const auto mid = (sa + sb) * 0.5f;
        const float d = (orthoAlign_ == OrthoAlign::Horizontal)
                            ? std::abs (b.x - a.x)
                            : std::abs (b.y - a.y);

        g.setColour (glow.withAlpha (0.85f));
        g.drawLine (sa.x, sa.y, sb.x, sb.y, 1.4f);

        // End ticks perpendicular to the gap line
        const float tx = (orthoAlign_ == OrthoAlign::Horizontal) ? 0.0f : 6.0f;
        const float ty = (orthoAlign_ == OrthoAlign::Horizontal) ? 6.0f : 0.0f;
        g.drawLine (sa.x - tx, sa.y - ty, sa.x + tx, sa.y + ty, 1.4f);
        g.drawLine (sb.x - tx, sb.y - ty, sb.x + tx, sb.y + ty, 1.4f);

        const juce::String lab = juce::String (d, 2) + " m";
        const float tw = (float) lab.length() * 7.0f + 14.0f;
        const float th = 18.0f;
        auto box = juce::Rectangle<float> (mid.x - tw * 0.5f, mid.y - th * 0.5f - 10.0f, tw, th);
        g.setColour (Brand::panelDark().withAlpha (0.92f));
        g.fillRoundedRectangle (box, 4.0f);
        g.setColour (glow.withAlpha (0.9f));
        g.drawRoundedRectangle (box, 4.0f, 1.2f);
        g.setColour (Brand::white());
        g.drawText (lab, box.toNearestInt(), juce::Justification::centred, false);
    }
}

// ---------------------------------------------------------------------------
void RadiationPatternComponent::drawColourbar (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // Layout matches mockup: thin vertical Rel. SPL strip + 0/-6/…/-36 db + caption.
    const int barX = bounds.getX() + 4;
    const int barY = bounds.getY() + 16;
    const int barW = 16;
    const int barH = bounds.getHeight() - 46;
    if (barH < 20) return;

    const auto tickCol = Brand::text();                 // white (dark) / charcoal (light)
    const auto outline = AppSettings::get().isDark()
                             ? juce::Colour (0xffc8c8c8)
                             : Brand::border();
    g.setFont (Brand::techBold (Brand::Type::colourBarTick));

    auto drawTick = [&] (int db, int ty)
    {
        g.setColour (tickCol);
        g.drawText (juce::String (db) + " db",
                    barX + barW + 4, ty - 7, 54, 14,
                    juce::Justification::centredLeft);
    };

    if (params_.viewMode == ViewMode::SPL && params_.bandedSPL)
    {
        // Fixed 3 dB contour steps from 0 down to db Floor (never stretch labels).
        const float step = 3.0f;
        const float floor = (float) params_.dBfloor;
        const int bands = juce::jmax (1, (int) std::lround (-floor / step) + 1);
        const float bh = (float) barH / (float) bands;
        for (int i = 0; i < bands; ++i)
        {
            const float db = -step * (float) i;
            g.setColour (ColourMaps::splBandForFloor (db, floor, step));
            g.fillRect ((float) barX, barY + i * bh, (float) barW, bh + 0.5f);
            drawTick ((int) std::lround (db), (int) (barY + i * bh));
        }
        g.setColour (outline);
        g.drawRect (barX, barY, barW, (int) (bh * (float) bands + 0.5f), 1);
    }
    else // continuous — fixed −6 dB legend ticks; floor is the bottom stop only
    {
        const float floor = (float) params_.dBfloor;
        const float span = juce::jmax (ColourMaps::kRelSplDesignSpanDB, -floor);
        for (int yy = 0; yy < barH; ++yy)
        {
            const float frac = (float) yy / (float) juce::jmax (1, barH - 1); // 0 at top
            const float db = -frac * span;
            const float t = ColourMaps::relDbToColourT (db, floor);
            g.setColour (ColourMaps::sevenColor (t));
            g.fillRect (barX, barY + yy, barW, 1);
        }
        g.setColour (outline);
        g.drawRect (barX, barY, barW, barH, 1);

        // Ticks every −6 dB from 0 down to floor (e.g. 0,−6,…,−36) — never −9.
        const float step = ColourMaps::kRelSplStepDB;
        const int nTicks = juce::jmax (1, (int) std::lround (-floor / step));
        for (int i = 0; i <= nTicks; ++i)
        {
            const float db = -step * (float) i;
            if (db < floor - 0.01f) break;
            const float frac = (span > 1.0e-3f) ? (-db / span) : 0.0f;
            const int ty = barY + (int) std::lround (frac * (float) (barH - 1));
            drawTick ((int) std::lround (db), ty);
        }
    }

    g.setColour (tickCol);
    g.setFont (Brand::techBold (Brand::Type::colourBarTitle));
    g.drawText ("Rel. SPL", barX - 2, barY + barH + 6, barW + 72, 14,
                juce::Justification::centredLeft);
}

// ---------------------------------------------------------------------------
// CLIO-style Atomik polar frame (vector only — never pastes graph images).
// Polar frame aligned with SPL heatmap: 0 deg = forward = right (+X),
// 90 deg = up (+Y), angles increase counter-clockwise (math / heatmap sense).
// dB rings 6 / 0 / -6 / -12 / -18 / -24, ATOMIK branding.
// ---------------------------------------------------------------------------
namespace
{
    juce::String utf8Deg()  { return juce::String::fromUTF8 ("\xc2\xb0"); }
    juce::String utf8Dot()  { return juce::String::fromUTF8 (" \xc2\xb7 "); }

    struct ClioFrame
    {
        float cx = 0, cy = 0, radius = 0;
        float dbMin = -24.0f, dbMax = 6.0f;

        float dbToR (float db) const
        {
            return juce::jlimit (0.0f, 1.0f, (db - dbMin) / (dbMax - dbMin)) * radius;
        }

        // 0° right (forward / heatmap +X), 90° top (+Y), CCW.
        juce::Point<float> toXY (float deg, float db) const
        {
            const float a  = deg * (float) M_PI / 180.0f;
            const float rr = dbToR (db);
            return { cx + rr * std::cos (a), cy - rr * std::sin (a) };
        }
    };

    void drawClioChrome (juce::Graphics& g, juce::Rectangle<int> bounds,
                         int hz, const juce::String& subtitle,
                         ClioFrame& out)
    {
        // Light CLIO-like canvas on Atomik dark chrome.
        const juce::Colour paper (0xfff4f5f7);
        const juce::Colour ink   (0xff1a1c20);
        const juce::Colour grid  (0xffb8bec8);
        const juce::Colour axis  (0xff5a6270);
        const juce::Colour brand = Brand::accent();

        g.fillAll (Brand::panelDark());

        auto sheet = bounds.reduced (10, 8);
        g.setColour (paper);
        g.fillRoundedRectangle (sheet.toFloat(), 4.0f);
        g.setColour (Brand::border());
        g.drawRoundedRectangle (sheet.toFloat(), 4.0f, 1.0f);

        // Header: ATOMIK + frequency (left), title (centre).
        g.setColour (ink);
        g.setFont (Brand::tech (Brand::Type::exportBrand, true));
        g.drawText ("ATOMIK", sheet.getX() + 14, sheet.getY() + 8, 120, 18,
                    juce::Justification::centredLeft);

        g.setFont (Brand::tech (Brand::Type::exportFrequency, true));
        g.setColour (brand);
        const juce::String freqLab = juce::String (hz) + " Hz";
        g.drawText (freqLab, sheet.getX() + 14, sheet.getY() + 28, 140, 22,
                    juce::Justification::centredLeft);
        // Underline frequency like CLIO.
        const float fw = g.getCurrentFont().getStringWidthFloat (freqLab);
        g.drawLine ((float) sheet.getX() + 14.0f, (float) sheet.getY() + 50.0f,
                    (float) sheet.getX() + 14.0f + fw, (float) sheet.getY() + 50.0f, 1.5f);

        g.setColour (ink);
        g.setFont (Brand::tech (Brand::Type::exportChartTitle, true));
        g.drawText ("2D Directivity Analysis",
                    sheet.getX(), sheet.getY() + 10, sheet.getWidth(), 22,
                    juce::Justification::centred);

        if (subtitle.isNotEmpty())
        {
            g.setColour (axis);
            g.setFont (Brand::tech (Brand::Type::exportSubtitle));
            g.drawText (subtitle, sheet.getX(), sheet.getY() + 32, sheet.getWidth(), 16,
                        juce::Justification::centred);
        }

        auto plot = sheet.withTrimmedTop (58).withTrimmedBottom (18).reduced (36, 12);
        out.cx = (float) plot.getCentreX();
        out.cy = (float) plot.getCentreY() + 4.0f;
        out.radius = (float) std::min (plot.getWidth(), plot.getHeight()) * 0.44f;
        out.dbMin = -24.0f;
        out.dbMax = 6.0f;

        // Concentric dB rings (CLIO scale).
        g.setFont (Brand::mono (Brand::Type::exportPolarRing));
        for (int db = 6; db >= -24; db -= 6)
        {
            const float rr = out.dbToR ((float) db);
            g.setColour (grid);
            g.drawEllipse (out.cx - rr, out.cy - rr, 2 * rr, 2 * rr,
                           db == 0 ? 1.3f : 0.9f);
            // Labels along the forward (+X / 0°) spoke — right side.
            g.setColour (axis);
            g.drawText (juce::String (db),
                        (int) (out.cx + rr + 4), (int) (out.cy - 7),
                        28, 12, juce::Justification::left);
        }

        // Radial spokes + angle labels every 30 deg (0 right, 90 top).
        for (int deg = 0; deg < 360; deg += 30)
        {
            auto p = out.toXY ((float) deg, out.dbMax);
            g.setColour (grid);
            g.drawLine (out.cx, out.cy, p.x, p.y, 0.9f);

            auto lp = out.toXY ((float) deg, out.dbMax + 3.0f);
            g.setColour (axis);
            g.setFont (Brand::mono (Brand::Type::exportPolarRing));
            juce::String lab;
            if (deg == 0)        lab = "0" + utf8Deg();
            else if (deg == 180) lab = "180" + utf8Deg();
            else if (deg < 180)  lab = juce::String (deg) + utf8Deg();
            else                 lab = juce::String (deg - 360) + utf8Deg();
            g.drawText (lab, (int) lp.x - 18, (int) lp.y - 7, 36, 14,
                        juce::Justification::centred);
        }

        // Centre brand mark (replaces CLIO).
        g.setColour (ink.withAlpha (0.35f));
        g.setFont (Brand::tech (Brand::Type::exportPolarCenter, true));
        g.drawText ("ATOMIK", (int) out.cx - 36, (int) out.cy - 8, 72, 16,
                    juce::Justification::centred);
    }

    // Closed polar stroke in (angle, dB) — dense 0.5° samples, rounded joins.
    // Interpolates in dB (not XY) so lobes stay smooth curves, not hard corners.
    void strokeClioCurve (juce::Graphics& g, const ClioFrame& fr,
                          const std::vector<float>& deg,
                          const std::vector<float>& dbRel,
                          juce::Colour col, float strokeW)
    {
        if (deg.size() < 2 || deg.size() != dbRel.size()) return;

        const int nIn = (int) deg.size();
        auto dbAt = [&] (float a) -> float
        {
            while (a < 0.0f)    a += 360.0f;
            while (a >= 360.0f) a -= 360.0f;
            int i0 = 0;
            for (int i = 1; i < nIn; ++i)
            {
                if (deg[(size_t) i] >= a) { i0 = i - 1; break; }
                i0 = i;
            }
            const int i1 = (i0 + 1) % nIn;
            float a0 = deg[(size_t) i0];
            float a1 = deg[(size_t) i1];
            if (i1 == 0) a1 += 360.0f;
            if (a < a0) a += 360.0f;
            const float t = (a1 - a0) > 1.0e-6f ? (a - a0) / (a1 - a0) : 0.0f;
            const float tt = t * t * (3.0f - 2.0f * t);   // smoothstep
            return dbRel[(size_t) i0] + tt * (dbRel[(size_t) i1] - dbRel[(size_t) i0]);
        };

        constexpr int nOut = 720;
        juce::Path path;
        for (int i = 0; i <= nOut; ++i)
        {
            const float a = (float) i * (360.0f / (float) nOut);
            auto p = fr.toXY (a, juce::jlimit (fr.dbMin, fr.dbMax, dbAt (a)));
            if (i == 0) path.startNewSubPath (p);
            else        path.lineTo (p);
        }
        path.closeSubPath();

        g.setColour (col);
        g.strokePath (path, juce::PathStrokeType (strokeW, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }

    // Dot markers at exact measured angles (data-point traceability).
    void drawMeasuredDots (juce::Graphics& g, const ClioFrame& fr,
                           const std::vector<float>& deg,
                           const std::vector<float>& dbRel,
                           juce::Colour col, float radiusPx = 3.0f)
    {
        if (deg.size() != dbRel.size()) return;
        // Dense 1° sweeps: skip dots (curve already shows every sample).
        if (deg.size() > 90) return;

        g.setColour (col);
        for (size_t i = 0; i < deg.size(); ++i)
        {
            auto p = fr.toXY (deg[i], juce::jlimit (fr.dbMin, fr.dbMax, dbRel[i]));
            g.fillEllipse (p.x - radiusPx, p.y - radiusPx, radiusPx * 2.0f, radiusPx * 2.0f);
        }
    }

    // -6 dB beamwidth ring + label (professional acoustic annotation).
    void drawBeamwidthRing (juce::Graphics& g, const ClioFrame& fr, float beamwidthDeg)
    {
        if (beamwidthDeg < 1.0f || beamwidthDeg > 359.0f) return;

        const float rr = fr.dbToR (-6.0f);
        g.setColour (juce::Colour (0xff5a6270).withAlpha (0.55f));
        // Dashed look: short arcs around the -6 dB ring.
        for (int a = 0; a < 360; a += 8)
        {
            auto p0 = fr.toXY ((float) a,       -6.0f);
            auto p1 = fr.toXY ((float) (a + 4), -6.0f);
            g.drawLine (p0.x, p0.y, p1.x, p1.y, 1.0f);
        }
        juce::ignoreUnused (rr);

        // Arc markers at +/- half beamwidth from on-axis.
        const float half = beamwidthDeg * 0.5f;
        g.setColour (juce::Colour (0xffc45c26));
        auto L = fr.toXY (-half, -6.0f);
        auto R = fr.toXY ( half, -6.0f);
        g.fillEllipse (L.x - 3.5f, L.y - 3.5f, 7.0f, 7.0f);
        g.fillEllipse (R.x - 3.5f, R.y - 3.5f, 7.0f, 7.0f);

        g.setFont (Brand::mono (Brand::Type::exportPolarRing));
        g.setColour (juce::Colour (0xff1a1c20));
        const juce::String lab = "BW " + juce::String (beamwidthDeg, 0)
                                 + juce::String::fromUTF8 ("\xc2\xb0");
        g.drawText (lab, (int) fr.cx - 40, (int) (fr.cy + fr.radius * 0.55f), 80, 14,
                    juce::Justification::centred);
    }
}

// ---------------------------------------------------------------------------
// DIRECTIVITY — CLIO-style Atomik plot of the *simulated* far-field pattern
// (AcousticEngine::polarMag). No measured-unit CSV overlay here — that lives
// on the Measured Polar view only.
// ---------------------------------------------------------------------------
void RadiationPatternComponent::drawPolarPlot (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    int nEnabled = 0;
    for (const auto& s : speakers_) if (s.enabled) ++nEnabled;

    const int hz = (int) (params_.frequency + 0.5);

    juce::String sub = "Simulated far-field";
    if (nEnabled >= 2)
        sub += utf8Dot() + "Array (" + juce::String (nEnabled) + " subs)";
    else if (nEnabled == 1)
        sub += utf8Dot() + "1 sub";
    else
        sub += utf8Dot() + "No active sub";

    ClioFrame fr;
    drawClioChrome (g, bounds, hz, sub, fr);
    polarCx_ = fr.cx;
    polarCy_ = fr.cy;
    polarRadius_ = fr.radius;
    polarFrameValid_ = fr.radius > 1.0f;

    // Simulated polar from the engine (cardioid / coherent array lobes, etc.).
    if (! result_.polarMag.empty() && nEnabled > 0)
    {
        std::vector<float> degs, dbs;
        const int n = (int) result_.polarMag.size();
        degs.reserve ((size_t) n);
        dbs.reserve ((size_t) n);
        for (int i = 0; i < n; ++i)
        {
            const float mag = juce::jmax (1.0e-6f, result_.polarMag[(size_t) i]);
            degs.push_back ((float) i * 360.0f / (float) n);
            dbs.push_back (20.0f * std::log10 (mag));
        }
        strokeClioCurve (g, fr, degs, dbs, Brand::accent().withAlpha (0.95f), 2.2f);
    }
    else
    {
        g.setColour (juce::Colour (0xff5a6270));
        g.setFont (Brand::tech (Brand::Type::colourBarTick));
        g.drawText (nEnabled == 0 ? "Enable a Q21S unit and press RUN"
                                  : "No polar data — press RUN",
                    bounds.withTrimmedTop (bounds.getHeight() / 2),
                    juce::Justification::centred);
    }

    // Speaker markers (live list — reflects delete / enable immediately).
    float mx = 0.0f, my = 0.0f;
    if (nEnabled > 0)
    {
        for (const auto& s : speakers_) if (s.enabled) { mx += s.x; my += s.y; }
        mx /= (float) nEnabled; my /= (float) nEnabled;
        const float sc = fr.radius * 0.08f;
        for (int i = 0; i < (int) speakers_.size(); ++i)
        {
            const auto& s = speakers_[i];
            if (! s.enabled) continue;
            const float px = fr.cx + (s.x - mx) * sc;
            const float py = fr.cy - (s.y - my) * sc;
            juce::Rectangle<float> cab (px - 5.0f, py - 5.0f, 10.0f, 10.0f);
            g.setColour (Brand::accent());
            g.fillRoundedRectangle (cab, 2.0f);
            g.setColour (juce::Colour (0xff1a1c20));
            g.drawRoundedRectangle (cab, 2.0f, (i == selected_) ? 2.0f : 1.0f);
        }
    }

    // Legend
    const int lx = bounds.getX() + 24;
    int ly = bounds.getY() + 58;
    g.setFont (Brand::tech (Brand::Type::axis));
    g.setColour (Brand::accent().withAlpha (0.95f));
    g.fillRect (lx, ly + 4, 16, 3);
    g.setColour (juce::Colour (0xff1a1c20));
    g.drawText (nEnabled >= 2 ? ("Array (" + juce::String (nEnabled) + " subs)")
                              : "Simulated pattern",
                lx + 20, ly, 160, 14, juce::Justification::left);
}

// ---------------------------------------------------------------------------
// Measured polar — CLIO-style Atomik plot from real MeasurementIntegrationPack
// readings (Room = ShyamGuild, Ground Plane = Factory). Exact CSV points only.
// ---------------------------------------------------------------------------
void RadiationPatternComponent::drawMeasuredPolar (juce::Graphics& g,
                                                   juce::Rectangle<int> bounds)
{
    if (! measured_.ok)
    {
        g.fillAll (Brand::panelDark());
        g.setColour (Brand::ash());
        g.setFont (Brand::tech (Brand::Type::polarEmptyMessage));
        g.drawFittedText ("No measurement data found\n"
                          "Place MeasurementIntegrationPack/Data (or shyamGuildMeasurements)\n"
                          "inside the project folder, then rebuild / re-run.",
                          bounds.reduced (24), juce::Justification::centred, 4);
        return;
    }

    juce::StringArray availHz;
    for (const auto& f : measured_.freqs) if (f.ok) availHz.add (juce::String (f.hz));

    // Exact match preferred; else nearest measured frequency.
    const MeasuredFreq* mf = measuredForHz (measuredHz_);
    int displayHz = measuredHz_;
    if (mf == nullptr)
    {
        int best = 100000;
        for (const auto& f : measured_.freqs)
        {
            if (! f.ok) continue;
            const int d = std::abs (f.hz - measuredHz_);
            if (d < best) { best = d; mf = &f; displayHz = f.hz; }
        }
        if (mf == nullptr) displayHz = measuredHz_;
    }
    else
    {
        displayHz = mf->hz;
    }

    const juce::String setName = measured_.sourceName.isNotEmpty()
                                     ? measured_.sourceName : "Measured";

    juce::String sub = setName + utf8Dot() + "Horizontal"
                       + utf8Dot() + juce::String (measuredDistanceM_, 1) + " m";
    ClioFrame fr;
    drawClioChrome (g, bounds, displayHz, sub, fr);
    polarCx_ = fr.cx;
    polarCy_ = fr.cy;
    polarRadius_ = fr.radius;
    polarFrameValid_ = fr.radius > 1.0f;

    if (mf == nullptr || ! mf->ok)
    {
        g.setColour (juce::Colour (0xff5a6270));
        g.setFont (Brand::tech (Brand::Type::colourBarTick));
        g.drawText ("No reading for " + juce::String (measuredHz_)
                    + " Hz  (available: " + availHz.joinIntoString (", ") + " Hz)",
                    bounds.withTrimmedTop (bounds.getHeight() / 2),
                    juce::Justification::centred);
        return;
    }

    std::vector<float> degs, dbs;
    const auto syn = MeasurementData::curveForFrequency (measured_, displayHz, measuredDistanceM_);
    const MeasuredCurve* primary = syn.ok ? &syn.curve : MeasurementData::curveAtDistance (*mf, measuredDistanceM_);
    if (primary != nullptr)
    {
        MeasurementData::polarStrokeSamples (*primary, degs, dbs);
        const juce::Colour ink (0xff1a1c20);
        strokeClioCurve (g, fr, degs, dbs, ink, 2.6f);
        if (primary->beamwidthDeg > 1.0f)
            drawBeamwidthRing (g, fr, primary->beamwidthDeg);
    }

    const int lx = bounds.getX() + 24;
    int ly = bounds.getY() + 58;
    g.setFont (Brand::tech (Brand::Type::polarLegend));
    g.setColour (juce::Colour (0xff1a1c20));
    g.fillRect (lx, ly + 4, 18, 3);
    g.drawText (juce::String (measuredDistanceM_, 1) + " m",
                lx + 24, ly, 80, 14, juce::Justification::left);
    ly += 16;

    if (primary != nullptr && primary->beamwidthDeg > 1.0f)
    {
        g.setColour (juce::Colour (0xff5a6270));
        g.setFont (Brand::mono (Brand::Type::exportPolarRing));
        g.drawText ("BW " + juce::String (primary->beamwidthDeg, 0)
                    + juce::String::fromUTF8 ("\xc2\xb0") + " (-6 dB)",
                    lx, ly, 160, 12, juce::Justification::left);
    }
}

// ---------------------------------------------------------------------------
void RadiationPatternComponent::drawAnnotations (juce::Graphics& g, juce::Rectangle<int>)
{
    const auto space = currentAnnotSpace();

    auto strokePath = [&] (const Annotation& a, float alpha = 1.0f)
    {
        if (a.pts.size() < 2) return;
        juce::Path path;
        auto p0 = annotateToScreen (a, a.pts[0]);
        path.startNewSubPath (p0);
        for (size_t i = 1; i < a.pts.size(); ++i)
            path.lineTo (annotateToScreen (a, a.pts[i]));
        g.setColour (a.colour.withMultipliedAlpha (alpha));
        g.strokePath (path, juce::PathStrokeType (a.thicknessPx,
                                                   juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    };

    auto formatDim = [&] (float dist) -> juce::String
    {
        if (space == AnnotSpace::PolarPlot)
            return juce::String (dist, 3) + " r";
        return formatLengthLabel (dist);
    };

    for (size_t ai = 0; ai < annotations_.size(); ++ai)
    {
        const auto& a = annotations_[ai];
        if (a.space != space) continue;

        if (a.kind == Annotation::Kind::Rectangle
            || a.kind == Annotation::Kind::Square
            || a.kind == Annotation::Kind::Circle)
        {
            drawShapeAnnotation (g, a);
            if (isAnnotationSelected ((int) ai))
                drawSelectionOverlay (g, a);
            continue;
        }
        if (a.kind == Annotation::Kind::TextBox)
        {
            drawTextBoxAnnotation (g, a);
            if (isAnnotationSelected ((int) ai))
                drawSelectionOverlay (g, a);
            continue;
        }
        if (a.kind == Annotation::Kind::Arc)
        {
            drawArcAnnotation (g, a);
            if (isAnnotationSelected ((int) ai))
                drawSelectionOverlay (g, a);
            continue;
        }

        strokePath (a);

        if (a.kind == Annotation::Kind::Measure && a.pts.size() >= 2)
        {
            const auto& w0 = a.pts.front();
            const auto& w1 = a.pts.back();
            const float dist = w0.getDistanceFrom (w1);
            auto s0 = annotateToScreen (a, w0);
            auto s1 = annotateToScreen (a, w1);
            const auto mid = (s0 + s1) * 0.5f;

            g.setColour (a.colour);
            g.fillEllipse (s0.x - 3.5f, s0.y - 3.5f, 7.0f, 7.0f);
            g.fillEllipse (s1.x - 3.5f, s1.y - 3.5f, 7.0f, 7.0f);
            drawPendingDimLabel (g, mid, formatDim (dist));
        }
        else if ((a.kind == Annotation::Kind::Line || a.kind == Annotation::Kind::Polyline)
                 && a.pts.size() >= 2)
        {
            auto s0 = annotateToScreen (a, a.pts.front());
            auto s1 = annotateToScreen (a, a.pts.back());
            g.setColour (a.colour);
            g.fillEllipse (s0.x - 3.0f, s0.y - 3.0f, 6.0f, 6.0f);
            g.fillEllipse (s1.x - 3.0f, s1.y - 3.0f, 6.0f, 6.0f);
        }

        if (isAnnotationSelected ((int) ai))
            drawSelectionOverlay (g, a);
    }

    // Rubber-band: ruler (two-click) ------------------------------------------------
    if (tool_ == Tool::Ruler && pendingAnchor_ && hoverValid_)
    {
        Annotation preview;
        preview.kind = Annotation::Kind::Measure;
        preview.space = space;
        preview.colour = drawColour_;
        preview.thicknessPx = 1.8f;
        auto hover = applyOrtho (pendingStartWorld_, hoverAnnot_);
        hover = snapAnnotPoint (hover);
        preview.pts = { pendingStartWorld_, hover };
        strokePath (preview, 0.75f);

        auto s0 = annotateToScreen (preview, preview.pts[0]);
        auto s1 = annotateToScreen (preview, preview.pts[1]);
        g.setColour (drawColour_.withAlpha (0.9f));
        g.fillEllipse (s0.x - 3.5f, s0.y - 3.5f, 7.0f, 7.0f);
        g.drawEllipse (s1.x - 3.5f, s1.y - 3.5f, 7.0f, 7.0f, 1.2f);
        drawPendingDimLabel (g, (s0 + s1) * 0.5f,
                             formatDim (preview.pts[0].getDistanceFrom (preview.pts[1])));
    }

    // Rubber-band: Shape session ----------------------------------------------------
    if (tool_ == Tool::Shape && sessionActive_ && ! sessionPts_.empty() && hoverValid_)
    {
        Annotation preview;
        preview.space = space;
        preview.colour = drawColour_;
        preview.fillAlpha = drawFillAlpha_;
        preview.thicknessPx = 2.0f;
        auto hover = applyOrtho (sessionPts_.back(), hoverAnnot_);
        hover = snapAnnotPoint (hover);

        auto markPts = [&] (const std::vector<juce::Point<float>>& pts)
        {
            g.setColour (drawColour_.withAlpha (0.9f));
            for (size_t i = 0; i < pts.size(); ++i)
            {
                auto s = annotateToScreen (preview, pts[i]);
                if (i + 1 == pts.size())
                    g.drawEllipse (s.x - 3.5f, s.y - 3.5f, 7.0f, 7.0f, 1.2f);
                else
                    g.fillEllipse (s.x - 3.5f, s.y - 3.5f, 7.0f, 7.0f);
            }
        };

        if (drawShape_ == DrawShape::Polyline)
        {
            preview.kind = Annotation::Kind::Polyline;
            preview.pts = sessionPts_;
            preview.pts.push_back (hover);
            strokePath (preview, 0.75f);
            markPts (preview.pts);
        }
        else if (drawShape_ == DrawShape::Line)
        {
            preview.kind = Annotation::Kind::Line;
            preview.pts = { sessionPts_[0], hover };
            strokePath (preview, 0.75f);
            markPts (preview.pts);
            drawPendingDimLabel (g,
                (annotateToScreen (preview, preview.pts[0])
                 + annotateToScreen (preview, preview.pts[1])) * 0.5f,
                formatDim (preview.pts[0].getDistanceFrom (preview.pts[1])));
        }
        else if (drawShape_ == DrawShape::Circle)
        {
            preview.kind = Annotation::Kind::Circle;
            if (construction_ == Construction::CircleTwoPoints && sessionPts_.size() >= 1)
            {
                const auto mid = (sessionPts_[0] + hover) * 0.5f;
                preview.pts = { mid, hover };
            }
            else
                preview.pts = { sessionPts_[0], hover };
            drawShapeAnnotation (g, preview, 0.85f);
            auto s0 = annotateToScreen (preview, sessionPts_[0]);
            auto s1 = annotateToScreen (preview, hover);
            g.setColour (drawColour_.withAlpha (0.65f));
            g.drawLine (s0.x, s0.y, s1.x, s1.y, 1.2f);
            markPts ({ sessionPts_[0], hover });
            drawPendingDimLabel (g, (s0 + s1) * 0.5f,
                                 "R = " + formatDim (preview.pts[0].getDistanceFrom (preview.pts[1])));
        }
        else if (drawShape_ == DrawShape::Arc)
        {
            preview.kind = Annotation::Kind::Arc;
            if (sessionPts_.size() == 1)
            {
                preview.kind = Annotation::Kind::Line;
                preview.pts = { sessionPts_[0], hover };
                strokePath (preview, 0.75f);
                markPts (preview.pts);
            }
            else if (sessionPts_.size() >= 2)
            {
                juce::Point<float> c;
                float r = 0;
                if (circleFrom3Points (sessionPts_[0], sessionPts_[1], hover, c, r))
                {
                    preview.pts = { c, sessionPts_[0], hover, sessionPts_[1] };
                    drawArcAnnotation (g, preview, 0.85f);
                }
                markPts ({ sessionPts_[0], sessionPts_[1], hover });
            }
        }
        else if (drawShape_ == DrawShape::Rectangle || drawShape_ == DrawShape::Square)
        {
            preview.kind = (drawShape_ == DrawShape::Square) ? Annotation::Kind::Square
                                                            : Annotation::Kind::Rectangle;
            preview.pts = { sessionPts_[0], hover };
            drawShapeAnnotation (g, preview, 0.85f);
            markPts (preview.pts);
            const auto wr = normalisedShapeRect (sessionPts_[0], hover, preview.kind);
            juce::String dim = formatDim (wr.getWidth());
            if (preview.kind == Annotation::Kind::Rectangle)
                dim += "  x  " + formatDim (wr.getHeight());
            auto s0 = annotateToScreen (preview, sessionPts_[0]);
            auto s1 = annotateToScreen (preview, hover);
            drawPendingDimLabel (g, (s0 + s1) * 0.5f, dim);
        }
        else if (drawShape_ == DrawShape::TextBox)
        {
            preview.kind = Annotation::Kind::TextBox;
            preview.pts = { sessionPts_[0], hover };
            preview.text = "Text";
            preview.rotationDeg = 0.0f;
            drawTextBoxAnnotation (g, preview, 0.85f);
            markPts (preview.pts);
        }
    }
}

// ---------------------------------------------------------------------------
int RadiationPatternComponent::speakerHitTest (juce::Point<float> p) const
{
    // True footprint hit, with a small screen pad so zoomed-out cabinets stay selectable.
    constexpr float kPadPx = 6.0f;
    for (int i = (int) speakers_.size() - 1; i >= 0; --i)
    {
        auto box = speakerFootprintScreen (speakers_[(size_t) i]).expanded (kPadPx);
        // Also accept clicks on the label band above the cabinet.
        box.setTop (box.getY() - 16.0f);
        if (box.contains (p))
            return i;
    }
    return -1;
}

void RadiationPatternComponent::mouseDown (const juce::MouseEvent& e)
{
    // Keep shortcuts (undo/redo) working after using pencil/line/etc.
    if (! hasKeyboardFocus (true))
        grabKeyboardFocus();

    lastMouse_ = e.position;
    const auto pb = plotArea();
    // Polar uses full bounds as the plot frame; SPL uses the trimmed plot area.
    const bool inPlot = (currentAnnotSpace() == AnnotSpace::PolarPlot)
                            ? getLocalBounds().contains (e.getPosition())
                            : pb.contains (e.getPosition());
    if (! inPlot) return;

    if (e.mods.isPopupMenu())
    {
        // Select under the cursor first so Copy/Paste/Delete work without a prior left-click.
        // Already-selected hits keep the current multi-selection (Windows-style).
        auto annotPt = screenToAnnot (e.position.x, e.position.y);
        const float radius = (currentAnnotSpace() == AnnotSpace::PolarPlot)
            ? (10.0f / juce::jmax (1.0f, polarRadius_))
            : (10.0f / juce::jmax (1.0f, worldScale()));

        bool hitSomething = false;

        if (currentAnnotSpace() == AnnotSpace::World)
        {
            const int mHit = micHitTestScreen (e.position);
            if (mHit >= 0)
            {
                hitSomething = true;
                if (! isMicSelected (mHit))
                {
                    selectedAnnots_.clear();
                    selectedSpeakers_.clear();
                    selectedMics_.clear();
                    selectedMics_.push_back (mHit);
                    syncPrimarySelectionFromSets();
                    if (onAnnotSelectionChanged) onAnnotSelectionChanged();
                    if (onMicsChanged) onMicsChanged();
                    repaint();
                }
            }
        }

        if (! hitSomething)
        {
            const int aHit = annotationHitTest (annotPt, radius);
            if (aHit >= 0)
            {
                hitSomething = true;
                if (! isAnnotationSelected (aHit))
                {
                    selectedAnnots_.clear();
                    selectedMics_.clear();
                    selectedSpeakers_.clear();
                    selectedAnnots_.push_back (aHit);
                    syncPrimarySelectionFromSets();
                    if (onAnnotSelectionChanged) onAnnotSelectionChanged();
                    if (onMicsChanged) onMicsChanged();
                    repaint();
                }
            }
        }

        if (! hitSomething && currentAnnotSpace() == AnnotSpace::World)
        {
            const int sHit = speakerHitTest (e.position);
            if (sHit >= 0)
            {
                hitSomething = true;
                if (! isSpeakerSelected (sHit))
                {
                    selectedAnnots_.clear();
                    selectedMics_.clear();
                    selectedSpeakers_.clear();
                    selectedSpeakers_.push_back (sHit);
                    syncPrimarySelectionFromSets();
                    if (onAnnotSelectionChanged) onAnnotSelectionChanged();
                    if (onMicsChanged) onMicsChanged();
                    if (onSpeakerSelected) onSpeakerSelected (sHit);
                    repaint();
                }
            }
        }

        juce::ignoreUnused (hitSomething); // empty space keeps current selection (Paste still works)
        showSelectionContextMenu (e.getScreenPosition());
        return;
    }

    auto annot = screenToAnnot (e.position.x, e.position.y);

    // Add Mic armed: place on heatmap — but clicking an existing mic selects it.
    if (addMicArmed_ && currentAnnotSpace() == AnnotSpace::World)
    {
        const int mHit = micHitTestScreen (e.position);
        if (mHit >= 0)
        {
            setAddMicArmed (false);
            selectedAnnots_.clear();
            selectedSpeakers_.clear();
            selectedMics_.clear();
            selectedMics_.push_back (mHit);
            syncPrimarySelectionFromSets();
            if (onAnnotSelectionChanged) onAnnotSelectionChanged();
            if (onMicsChanged) onMicsChanged();
            if (tool_ == Tool::Select)
                beginMicDrag (mHit, e.position);
            repaint();
            return;
        }

        auto w = screenToWorld (e.position.x, e.position.y);
        placeMicAtWorld (w.x, w.y);
        return;
    }

    if (tool_ == Tool::Pencil)
    {
        if (! canAnnotate()) return;
        if (onWillEdit) onWillEdit();
        Annotation stroke;
        stroke.kind = Annotation::Kind::Freehand;
        stroke.space = currentAnnotSpace();
        stroke.colour = drawColour_;
        stroke.thicknessPx = 2.4f;
        stroke.pts.push_back (snapAnnotPoint (annot));
        annotations_.push_back (std::move (stroke));
        drag_ = Drag::Pencil;
        repaint();
        return;
    }

    if (tool_ == Tool::Eraser)
    {
        if (! canAnnotate()) return;
        if (onWillEdit) onWillEdit();
        const float radius = (currentAnnotSpace() == AnnotSpace::PolarPlot)
            ? (8.0f / juce::jmax (1.0f, polarRadius_))
            : (8.0f / juce::jmax (1.0f, worldScale()));
        eraseNear (annot, radius);
        drag_ = Drag::Erase;
        repaint();
        return;
    }

    if (tool_ == Tool::Ruler)
    {
        if (! canAnnotate()) return;
        const auto rawAnnot = annot;
        bool objHit = false;
        annot = snapAnnotPointFull (annot, false, &objHit);
        noteSnapSound (objHit, rawAnnot, annot);
        if (! pendingAnchor_)
        {
            pendingAnchor_ = true;
            pendingStartWorld_ = annot;
            hoverAnnot_ = annot;
            hoverValid_ = true;
            rubberBandStartScreen_ = e.position;
            drag_ = Drag::RubberBand;
            updateDrawPrompt();
        }
        else
        {
            annot = applyOrtho (pendingStartWorld_, annot);
            const auto rawEnd = annot;
            bool endHit = false;
            annot = snapAnnotPointFull (annot, false, &endHit);
            noteSnapSound (endHit, rawEnd, annot);
            if (pendingStartWorld_.getDistanceFrom (annot) > 1.0e-4f)
            {
                if (onWillEdit) onWillEdit();
                Annotation a;
                a.kind = Annotation::Kind::Measure;
                a.space = currentAnnotSpace();
                a.colour = drawColour_;
                a.thicknessPx = 1.8f;
                a.pts = { pendingStartWorld_, annot };
                annotations_.push_back (std::move (a));
                if (onEditCommitted) onEditCommitted();
            }
            pendingAnchor_ = false;
            hoverValid_ = false;
            drag_ = Drag::None;
            updateDrawPrompt();
        }
        repaint();
        return;
    }

    if (tool_ == Tool::Shape)
    {
        if (! canAnnotate()) return;
        acceptAnnotPoint (annot);
        hoverAnnot_ = annot;
        hoverValid_ = sessionActive_ && ! sessionPts_.empty();
        // Keep rubber-band live while the button is held (click-drag feels smooth).
        if (sessionActive_ && ! sessionPts_.empty()
            && (drawShape_ == DrawShape::Polyline
                || (int) sessionPts_.size() < pointsNeeded()))
        {
            rubberBandStartScreen_ = e.position;
            drag_ = Drag::RubberBand;
        }
        else
            drag_ = Drag::None;
        return;
    }

    // Select: Windows-style multi-select (click / Ctrl+click / marquee) + group move.
    if (tool_ == Tool::Select)
    {
        const bool additive = e.mods.isCommandDown() || e.mods.isCtrlDown();
        const float radius = (currentAnnotSpace() == AnnotSpace::PolarPlot)
            ? (10.0f / juce::jmax (1.0f, polarRadius_))
            : (10.0f / juce::jmax (1.0f, worldScale()));
        const float handleR = radius * 1.35f;

        auto beginSelectionMove = [&] ()
        {
            if (onWillEdit) onWillEdit();
            drag_ = Drag::SelectionMove;
            lastAnnotDrag_ = annot;
            lastMouse_ = e.position;
            selMoveStartMouse_ = annot;
            selMoveStartRef_ = selectionSnapReference();
            selMoveStartBounds_ = selectionMoveBounds();
            selMoveHasBounds_ = ! selMoveStartBounds_.isEmpty();
            selMoveRefValid_ = true;
            resetSnapSoundState();
            annotDragMoved_ = false;
            micDragMoved_ = false;
            repaint();
        };

        // Resize / rotate grips only when a single drawing is selected (no group).
        if (selectedAnnots_.size() == 1 && selectedMics_.empty() && selectedSpeakers_.empty())
        {
            const int ai = selectedAnnots_.front();
            if (ai >= 0 && ai < (int) annotations_.size())
            {
                const int h = resizeHandleHitTest (annotations_[(size_t) ai], annot, handleR);
                if (h >= 0)
                {
                    if (onWillEdit) onWillEdit();
                    selectedAnnot_ = ai;
                    annotDragMoved_ = false;
                    lastAnnotDrag_ = annot;
                    lastMouse_ = e.position;
                    auto& a = annotations_[(size_t) ai];
                    if (a.kind == Annotation::Kind::TextBox && h == 4)
                    {
                        drag_ = Drag::AnnotRotate;
                        resizeHandleIndex_ = 4;
                        const auto local = textBoxLocalRect (a);
                        rotateDragCentre_ = local.getCentre();
                        rotateDragStartDeg_ = a.rotationDeg;
                        rotateDragStartMouseDeg_ = std::atan2 (annot.y - rotateDragCentre_.y,
                                                               annot.x - rotateDragCentre_.x)
                                                    * 180.0f / (float) M_PI;
                    }
                    else
                    {
                        drag_ = Drag::AnnotResize;
                        resizeHandleIndex_ = h;
                    }
                    repaint();
                    return;
                }
            }
        }

        // Mic hit (world heatmap only) — glyph + label in screen space.
        if (currentAnnotSpace() == AnnotSpace::World)
        {
            const int mHit = micHitTestScreen (e.position);
            if (mHit >= 0)
            {
                if (additive)
                {
                    if (isMicSelected (mHit))
                        selectedMics_.erase (std::remove (selectedMics_.begin(),
                                                          selectedMics_.end(), mHit),
                                             selectedMics_.end());
                    else
                        selectedMics_.push_back (mHit);
                    syncPrimarySelectionFromSets();
                    if (onMicsChanged) onMicsChanged();
                    repaint();
                    return;
                }
                if (! isMicSelected (mHit))
                {
                    selectedAnnots_.clear();
                    selectedSpeakers_.clear();
                    selectedMics_.clear();
                    selectedMics_.push_back (mHit);
                    syncPrimarySelectionFromSets();
                    if (onAnnotSelectionChanged) onAnnotSelectionChanged();
                    if (onMicsChanged) onMicsChanged();
                }
                // Single mic → original Drag::Mic ring snap (not object/grid SelectionMove).
                if (selectedMics_.size() == 1 && selectedAnnots_.empty()
                    && selectedSpeakers_.empty())
                {
                    beginMicDrag (selectedMics_.front(), e.position);
                    if (onAnnotSelectionChanged) onAnnotSelectionChanged();
                    if (onMicsChanged) onMicsChanged();
                    repaint();
                    return;
                }
                beginSelectionMove();
                return;
            }
        }

        const int aHit = annotationHitTest (annot, radius);
        if (aHit >= 0)
        {
            // Sole selection: allow resize via grips on this hit.
            if (! additive && selectedAnnots_.size() <= 1)
            {
                const int h = resizeHandleHitTest (annotations_[(size_t) aHit], annot, handleR);
                if (h >= 0)
                {
                    selectedAnnots_ = { aHit };
                    selectedMics_.clear();
                    selectedSpeakers_.clear();
                    syncPrimarySelectionFromSets();
                    if (onWillEdit) onWillEdit();
                    annotDragMoved_ = false;
                    lastAnnotDrag_ = annot;
                    lastMouse_ = e.position;
                    auto& a = annotations_[(size_t) aHit];
                    if (a.kind == Annotation::Kind::TextBox && h == 4)
                    {
                        drag_ = Drag::AnnotRotate;
                        resizeHandleIndex_ = 4;
                        const auto local = textBoxLocalRect (a);
                        rotateDragCentre_ = local.getCentre();
                        rotateDragStartDeg_ = a.rotationDeg;
                        rotateDragStartMouseDeg_ = std::atan2 (annot.y - rotateDragCentre_.y,
                                                               annot.x - rotateDragCentre_.x)
                                                    * 180.0f / (float) M_PI;
                    }
                    else
                    {
                        drag_ = Drag::AnnotResize;
                        resizeHandleIndex_ = h;
                    }
                    if (onAnnotSelectionChanged) onAnnotSelectionChanged();
                    repaint();
                    return;
                }
            }

            if (additive)
            {
                if (isAnnotationSelected (aHit))
                    selectedAnnots_.erase (std::remove (selectedAnnots_.begin(),
                                                        selectedAnnots_.end(), aHit),
                                           selectedAnnots_.end());
                else
                    selectedAnnots_.push_back (aHit);
                syncPrimarySelectionFromSets();
                if (onAnnotSelectionChanged) onAnnotSelectionChanged();
                repaint();
                return;
            }

            if (! isAnnotationSelected (aHit))
            {
                selectedAnnots_.clear();
                selectedMics_.clear();
                selectedSpeakers_.clear();
                selectedAnnots_.push_back (aHit);
                syncPrimarySelectionFromSets();
                if (onAnnotSelectionChanged) onAnnotSelectionChanged();
                if (onMicsChanged) onMicsChanged();
            }
            beginSelectionMove();
            return;
        }

        // Speaker hit — world view only (below)
        if (currentAnnotSpace() == AnnotSpace::World)
        {
            const int sHit = speakerHitTest (e.position);
            if (sHit >= 0)
            {
                if (additive)
                {
                    if (isSpeakerSelected (sHit))
                        selectedSpeakers_.erase (std::remove (selectedSpeakers_.begin(),
                                                              selectedSpeakers_.end(), sHit),
                                                 selectedSpeakers_.end());
                    else
                        selectedSpeakers_.push_back (sHit);
                    syncPrimarySelectionFromSets();
                    if (onSpeakerSelected) onSpeakerSelected (selected_ >= 0 ? selected_ : sHit);
                    repaint();
                    return;
                }
                if (! isSpeakerSelected (sHit))
                {
                    selectedAnnots_.clear();
                    selectedMics_.clear();
                    selectedSpeakers_.clear();
                    selectedSpeakers_.push_back (sHit);
                    syncPrimarySelectionFromSets();
                    if (onAnnotSelectionChanged) onAnnotSelectionChanged();
                    if (onMicsChanged) onMicsChanged();
                    if (onSpeakerSelected) onSpeakerSelected (sHit);
                }
                beginSelectionMove();
                return;
            }
        }

        // Empty space → marquee (Windows desktop style). Pan is the Pan tool.
        if (! additive)
            clearPlotSelection();
        drag_ = Drag::Marquee;
        marqueeStartScreen_ = e.position;
        marqueeEndScreen_ = e.position;
        lastMouse_ = e.position;
        repaint();
        return;
    }

    // Pan / layout — SPL world view only
    if (currentAnnotSpace() == AnnotSpace::PolarPlot)
        return;

    if (tool_ == Tool::Select && layoutEditMode_ && layout_ != nullptr
             && layout_->valid() && layout_->visible && ! layout_->locked)
    {
        if (onWillEdit) onWillEdit();
        drag_ = Drag::Layer;
    }
    else if (tool_ == Tool::Pan)
    {
        drag_ = Drag::Pan;
    }
}

void RadiationPatternComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (drag_ == Drag::Pencil && ! annotations_.empty())
    {
        auto annot = snapAnnotPoint (screenToAnnot (e.position.x, e.position.y));
        auto& stroke = annotations_.back();
        const float minPx = 0.4f;
        float pxDist = minPx + 1.0f;
        if (! stroke.pts.empty())
        {
            auto a = annotateToScreen (stroke, stroke.pts.back());
            auto b = annotateToScreen (stroke, annot);
            pxDist = a.getDistanceFrom (b);
        }
        if (stroke.pts.empty() || pxDist > minPx)
            stroke.pts.push_back (annot);
        lastMouse_ = e.position;
        repaint();
        return;
    }

    if (drag_ == Drag::Erase)
    {
        auto annot = screenToAnnot (e.position.x, e.position.y);
        const float radius = (currentAnnotSpace() == AnnotSpace::PolarPlot)
            ? (8.0f / juce::jmax (1.0f, polarRadius_))
            : (8.0f / juce::jmax (1.0f, worldScale()));
        eraseNear (annot, radius);
        lastMouse_ = e.position;
        repaint();
        return;
    }

    if (drag_ == Drag::RubberBand
        || (tool_ == Tool::Shape && sessionActive_ && ! sessionPts_.empty())
        || (tool_ == Tool::Ruler && pendingAnchor_))
    {
        updateRubberBandAt (e.position);
        lastMouse_ = e.position;
        repaint();
        return;
    }

    if (drag_ == Drag::AnnotResize && selectedAnnot_ >= 0
        && selectedAnnot_ < (int) annotations_.size()
        && resizeHandleIndex_ >= 0)
    {
        auto cur = screenToAnnot (e.position.x, e.position.y);
        applyAnnotationResize (annotations_[(size_t) selectedAnnot_],
                               resizeHandleIndex_, cur);
        annotDragMoved_ = true;
        lastAnnotDrag_ = cur;
        lastMouse_ = e.position;
        if (tool_ == Tool::Select)
            updateSplProbeAt (e.position);
        repaint();
        return;
    }

    if (drag_ == Drag::AnnotRotate && selectedAnnot_ >= 0
        && selectedAnnot_ < (int) annotations_.size())
    {
        auto cur = screenToAnnot (e.position.x, e.position.y);
        auto& a = annotations_[(size_t) selectedAnnot_];
        if (a.kind == Annotation::Kind::TextBox)
        {
            const float mouseDeg = std::atan2 (cur.y - rotateDragCentre_.y,
                                               cur.x - rotateDragCentre_.x)
                                     * 180.0f / (float) M_PI;
            float next = rotateDragStartDeg_ + (mouseDeg - rotateDragStartMouseDeg_);
            // Snap to 15° when Shift is held.
            if (e.mods.isShiftDown())
                next = std::round (next / 15.0f) * 15.0f;
            // Keep in (-180, 180]
            while (next > 180.0f) next -= 360.0f;
            while (next <= -180.0f) next += 360.0f;
            a.rotationDeg = next;
            annotDragMoved_ = true;
        }
        lastAnnotDrag_ = cur;
        lastMouse_ = e.position;
        repaint();
        return;
    }

    if (drag_ == Drag::Marquee)
    {
        marqueeEndScreen_ = e.position;
        lastMouse_ = e.position;
        repaint();
        return;
    }

    if (drag_ == Drag::SelectionMove)
    {
        auto curAnnot = screenToAnnot (e.position.x, e.position.y);

        // Absolute snap from drag-start: avoids drift; snaps whichever edge of the
        // selection is closest to the grid / neighbouring geometry.
        // Mic-only moves use Drag::Mic (not this path).
        juce::Point<float> dAnnot { 0, 0 };
        juce::Point<float> dWorld { 0, 0 };

        if (drawGridSnap_ && selMoveRefValid_)
        {
            const auto mouseDelta = curAnnot - selMoveStartMouse_;

            if (selMoveHasBounds_)
            {
                const auto desired = selMoveStartBounds_ + mouseDelta;
                // Snap each corner; pick the smaller correction per axis so either
                // side of a rect can lock to a neighbour or grid line.
                bool hitTL = false, hitBR = false;
                const auto sTL = snapAnnotPointFull ({ desired.getX(), desired.getY() }, true, &hitTL);
                const auto sBR = snapAnnotPointFull ({ desired.getRight(), desired.getBottom() }, true, &hitBR);

                const float dLeft  = sTL.x - desired.getX();
                const float dRight = sBR.x - desired.getRight();
                const float dBot   = sTL.y - desired.getY();
                const float dTop   = sBR.y - desired.getBottom();

                // Prefer the side that latched onto another object; else nearer correction.
                const float dx = hitTL && ! hitBR ? dLeft
                               : hitBR && ! hitTL ? dRight
                               : (std::abs (dLeft) <= std::abs (dRight) ? dLeft : dRight);
                const float dy = hitTL && ! hitBR ? dBot
                               : hitBR && ! hitTL ? dTop
                               : (std::abs (dBot) <= std::abs (dTop) ? dBot : dTop);

                const auto curB = selectionMoveBounds();
                const auto target = desired.translated (dx, dy);
                dAnnot = { target.getX() - curB.getX(), target.getY() - curB.getY() };

                noteSnapSound (selectionMeetsOtherAnnotation (target)
                               || selectionMeetsOtherSpeaker (target),
                               { desired.getX(), desired.getY() },
                               { target.getX(), target.getY() });
            }
            else
            {
                const auto desired = selMoveStartRef_ + mouseDelta;
                bool objHit = false;
                const auto snapped = snapAnnotPointFull (desired, true, &objHit);
                const auto curRef = selectionSnapReference();
                dAnnot = snapped - curRef;
                noteSnapSound (objHit, desired, snapped);
            }

            if (currentAnnotSpace() == AnnotSpace::World)
                dWorld = dAnnot;
        }
        else
        {
            dAnnot = curAnnot - lastAnnotDrag_;
            if (currentAnnotSpace() == AnnotSpace::World)
            {
                auto curW = screenToWorld (e.position.x, e.position.y);
                auto lastW = screenToWorld (lastMouse_.x, lastMouse_.y);
                dWorld = curW - lastW;
            }
        }

        if (std::abs (dAnnot.x) > 1.0e-12f || std::abs (dAnnot.y) > 1.0e-12f
            || std::abs (dWorld.x) > 1.0e-12f || std::abs (dWorld.y) > 1.0e-12f)
        {
            moveSelectionBy (dAnnot, dWorld);
            lastAnnotDrag_ = curAnnot;
            annotDragMoved_ = true;
            micDragMoved_ = true;
        }
        lastMouse_ = e.position;
        if (tool_ == Tool::Select)
            updateSplProbeAt (e.position);
        repaint();
        return;
    }

    if (drag_ == Drag::Mic && selectedMic_ >= 0 && selectedMic_ < (int) mics_.size())
    {
        // Original first snap pattern: mic follows cursor; ring snap + tak on latch.
        auto w = screenToWorld (e.position.x, e.position.y);
        float nx = juce::jlimit (0.0f, (float) result_.worldW, w.x);
        float ny = juce::jlimit (0.0f, (float) result_.worldH, w.y);
        snapMicWorld (nx, ny, true);
        auto& mic = mics_[(size_t) selectedMic_];
        if (std::abs (mic.x - nx) > 1.0e-6f || std::abs (mic.y - ny) > 1.0e-6f)
        {
            mic.x = nx;
            mic.y = ny;
            if (micWasSnapped_)
            {
                const auto snap = MicRingSnap::snapToRing (nx, ny, speakers_);
                mic.ringLocked = snap.snapped;
                mic.ringRadiusM = snap.radiusM;
                mic.ringSpeaker = snap.speakerIndex;
            }
            else
            {
                mic.ringLocked = false;
                mic.ringSpeaker = -1;
            }
            micDragMoved_ = true;
            refreshMicLevels();
            if (onMicsChanged) onMicsChanged();
        }
        lastMouse_ = e.position;
        lastMicDragWorld_ = w;
        if (tool_ == Tool::Select)
            updateSplProbeAt (e.position);
        repaint();
        return;
    }

    if (drag_ == Drag::Pan)
    {
        origin_ += (e.position - lastMouse_);
        lastMouse_ = e.position;
        clampViewToField();
        if (tool_ == Tool::Select)
            updateSplProbeAt (e.position);
        repaint();
    }
    else if (drag_ == Drag::Layer && layout_ != nullptr)
    {
        const auto d = e.position - lastMouse_;
        lastMouse_ = e.position;
        layout_->originM.x += d.x / worldScaleX();
        layout_->originM.y -= d.y / worldScaleY();     // screen y down -> world y up
        if (layoutSnap_)
        {
            layout_->originM.x = std::round (layout_->originM.x);
            layout_->originM.y = std::round (layout_->originM.y);
        }
        if (onLayoutMoved) onLayoutMoved();
        repaint();
    }
    else if (drag_ == Drag::Speaker && draggedSpeaker_ >= 0)
    {
        auto w = screenToWorld (e.position.x, e.position.y);
        const float nx = juce::jlimit (0.0f, (float) result_.worldW, w.x);
        const float ny = juce::jlimit (0.0f, (float) result_.worldH, w.y);
        if (draggedSpeaker_ < (int) speakers_.size())
        {
            speakers_[(size_t) draggedSpeaker_].x = nx;
            speakers_[(size_t) draggedSpeaker_].y = ny;
        }
        if (onSpeakerMoved) onSpeakerMoved (draggedSpeaker_, nx, ny);
        if (tool_ == Tool::Select)
            updateSplProbeAt (e.position);
        repaint();
    }
}

void RadiationPatternComponent::mouseUp (const juce::MouseEvent& e)
{
    if (drag_ == Drag::Marquee)
    {
        marqueeEndScreen_ = e.position;
        const bool additive = e.mods.isCommandDown() || e.mods.isCtrlDown();
        applyMarqueeSelection (additive);
        drag_ = Drag::None;
        resetSnapSoundState();
        repaint();
        return;
    }

    if (drag_ == Drag::RubberBand
        || (tool_ == Tool::Shape && sessionActive_)
        || (tool_ == Tool::Ruler && pendingAnchor_))
    {
        // Smooth click-drag: releasing after a real drag finishes the 2-point entity.
        tryFinishRubberBandAt (e.position);
        drag_ = Drag::None;
        // If still waiting for a second pick (tiny click), keep rubber-band on mouseMove.
        if ((tool_ == Tool::Shape && sessionActive_ && ! sessionPts_.empty())
            || (tool_ == Tool::Ruler && pendingAnchor_))
            updateRubberBandAt (e.position);
        repaint();
        return;
    }

    const bool committed = (drag_ == Drag::Pencil || drag_ == Drag::Erase
                            || drag_ == Drag::Speaker || drag_ == Drag::Layer
                            || (drag_ == Drag::Annot && annotDragMoved_)
                            || (drag_ == Drag::AnnotResize && annotDragMoved_)
                            || (drag_ == Drag::AnnotRotate && annotDragMoved_)
                            || (drag_ == Drag::SelectionMove && annotDragMoved_)
                            || (drag_ == Drag::Mic && micDragMoved_));
    drag_ = Drag::None;
    draggedSpeaker_ = -1;
    resizeHandleIndex_ = -1;
    annotDragMoved_ = false;
    micDragMoved_ = false;
    resetSnapSoundState();
    if (committed && onEditCommitted)
        onEditCommitted();
}

void RadiationPatternComponent::mouseWheelMove (const juce::MouseEvent& e,
                                                const juce::MouseWheelDetails& wheel)
{
    if (! hasData_ || params_.viewMode == ViewMode::Directivity
                   || (params_.viewMode == ViewMode::MeasuredPolar && ! showingBemHeatmap())) return;

    const float factor = (wheel.deltaY > 0 ? 1.1f : 1.0f / 1.1f);
    // Zoom in to inspect sections (down to 1 mm cells); never past full-pane cover.
    const float newZoom = juce::jlimit (1.0f, kMaxZoom, zoom_ * factor);
    if (std::abs (newZoom - zoom_) < 1e-6f) return;

    // Keep the world point under the cursor fixed.
    auto worldUnder = screenToWorld (e.position.x, e.position.y);
    zoom_ = newZoom;
    const auto pb = plotArea();
    origin_.x = e.position.x - (float) pb.getX() - worldUnder.x * worldScaleX();
    origin_.y = e.position.y - (float) pb.getY() - ((float) result_.worldH - worldUnder.y) * worldScaleY();
    clampViewToField();
    repaint();
}

void RadiationPatternComponent::mouseMove (const juce::MouseEvent& e)
{
    const bool tracking = (tool_ == Tool::Ruler && pendingAnchor_)
                       || (tool_ == Tool::Shape && sessionActive_ && ! sessionPts_.empty());
    if (tracking)
    {
        splProbeValid_ = false;
        updateRubberBandAt (e.position);
        repaint();
        return;
    }

    // Select tool: live SPL readout under the cursor on the heatmap.
    if (tool_ == Tool::Select && showSplProbe_)
    {
        const bool was = splProbeValid_;
        updateSplProbeAt (e.position);
        if (splProbeValid_ || was)
            repaint();
        return;
    }

    if (splProbeValid_)
    {
        splProbeValid_ = false;
        repaint();
    }
    hoverValid_ = false;
}

void RadiationPatternComponent::mouseExit (const juce::MouseEvent&)
{
    hoverValid_ = false;
    const bool hadProbe = splProbeValid_;
    splProbeValid_ = false;
    if (pendingAnchor_ || sessionActive_ || hadProbe)
        repaint();
}

void RadiationPatternComponent::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (tool_ == Tool::Shape && drawShape_ == DrawShape::Polyline && sessionPts_.size() >= 2)
    {
        finishPolyline (construction_ == Construction::PolylineClosed);
        return;
    }

    if (tool_ == Tool::Select || tool_ == Tool::Shape)
    {
        const float radius = (currentAnnotSpace() == AnnotSpace::PolarPlot)
            ? (10.0f / juce::jmax (1.0f, polarRadius_))
            : (10.0f / juce::jmax (1.0f, worldScale()));
        const auto annot = screenToAnnot (e.position.x, e.position.y);
        const int hit = annotationHitTest (annot, radius);
        if (hit >= 0 && hit < (int) annotations_.size()
            && annotations_[(size_t) hit].kind == Annotation::Kind::TextBox)
        {
            setSelectedAnnotation (hit);
            promptEditTextBox (hit);
        }
    }
}

bool RadiationPatternComponent::keyPressed (const juce::KeyPress& key)
{
    {
        const auto mods = juce::ModifierKeys::getCurrentModifiersRealtime();
        const bool chord = mods.isCommandDown() || mods.isCtrlDown();
        if (chord && ! mods.isAltDown())
        {
            const auto letter = key.getTextCharacter();
            const int code = key.getKeyCode();
            const bool isC = (letter == 'c' || letter == 'C' || code == 'C' || code == 'c');
            const bool isV = (letter == 'v' || letter == 'V' || code == 'V' || code == 'v');
            if (isC)
            {
                copySelection();
                return true;
            }
            if (isV)
            {
                pasteClipboard();
                return true;
            }
        }
    }

    if (key.isKeyCode (juce::KeyPress::escapeKey))
    {
        if (addMicArmed_)
        {
            setAddMicArmed (false);
            return true;
        }
        if (pendingAnchor_ || sessionActive_ || numericBuffer_.isNotEmpty())
        {
            cancelDrawSession();
            return true;
        }
        if (! selectedAnnots_.empty() || ! selectedMics_.empty() || ! selectedSpeakers_.empty())
        {
            clearPlotSelection();
            return true;
        }
        // Idle on a drawing tool → return to Select (cursor).
        if (tool_ == Tool::Shape || tool_ == Tool::Pencil
            || tool_ == Tool::Eraser || tool_ == Tool::Ruler)
        {
            setTool (Tool::Select);
            return true;
        }
    }

    if ((key.isKeyCode (juce::KeyPress::deleteKey)
         || key.isKeyCode (juce::KeyPress::backspaceKey))
        && ! sessionActive_ && ! pendingAnchor_
        && hasCopyableSelection())
    {
        deleteSelection();
        return true;
    }

    if (tool_ == Tool::Shape)
    {
        if (key.isKeyCode (juce::KeyPress::returnKey))
        {
            if (numericBuffer_.isNotEmpty())
            {
                const double v = numericBuffer_.getDoubleValue();
                numericBuffer_.clear();
                if (commitNumericValue (v))
                    return true;
                updateDrawPrompt();
                return true;
            }
            if (drawShape_ == DrawShape::Polyline && sessionPts_.size() >= 2)
            {
                finishPolyline (construction_ == Construction::PolylineClosed);
                return true;
            }
        }

        const juce::juce_wchar c = key.getTextCharacter();
        if ((c >= '0' && c <= '9') || c == '.' || c == ',')
        {
            if (c == ',')
                numericBuffer_ += '.';
            else
                numericBuffer_ += juce::String::charToString (c);
            updateDrawPrompt();
            return true;
        }
        if (key.isKeyCode (juce::KeyPress::backspaceKey) && numericBuffer_.isNotEmpty())
        {
            numericBuffer_ = numericBuffer_.dropLastCharacters (1);
            updateDrawPrompt();
            return true;
        }
    }

    if (onKeyPressed != nullptr && onKeyPressed (key))
        return true;
    return false;
}