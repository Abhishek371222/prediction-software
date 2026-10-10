#include "RadiationPatternComponent.h"
#include "ColourMaps.h"
#include "BrandTheme.h"
#include "AppSettings.h"
#include "MicRingSnap.h"
#include "SpeakerPropertiesDialog.h"
#include <cmath>
#include <algorithm>
#include <limits>

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
    if (isEditingTextBox())
        endTextBoxEdit (true);
    tool_ = t;
    resetDrawSession();
    pendingAnchor_ = false;
    hoverValid_ = false;
    splProbeValid_ = false;
    if (t != Tool::Select)
    {
        if (addMicArmed_)
        {
            addMicArmed_ = false;
            if (onAddMicArmedChanged) onAddMicArmedChanged();
        }
        if (addSpeakerArmed_)
        {
            addSpeakerArmed_ = false;
            if (onAddSpeakerArmedChanged) onAddSpeakerArmedChanged();
        }
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

bool RadiationPatternComponent::hasPlaneTarget() const noexcept
{
    // Unlike the text controls this does NOT arm the next shape: a plane is a
    // decision you make about something you have drawn and can see, not a mode
    // you enter beforehand.
    return selectedAnnot_ >= 0 && selectedAnnot_ < (int) annotations_.size()
        && Annotation::canBePlane (annotations_[(size_t) selectedAnnot_].kind);
}

bool RadiationPatternComponent::getPlaneOn() const noexcept
{
    return hasPlaneTarget() && annotations_[(size_t) selectedAnnot_].isPlane;
}

int RadiationPatternComponent::getPlaneType() const noexcept
{
    if (! hasPlaneTarget()) return 0;
    return (int) annotations_[(size_t) selectedAnnot_].planeType;
}

int RadiationPatternComponent::getPlaneListenHeight() const noexcept
{
    if (! hasPlaneTarget()) return 0;
    return (int) annotations_[(size_t) selectedAnnot_].listenHgt;
}

void RadiationPatternComponent::setPlaneListenHeight (int which)
{
    const auto hgt = (Annotation::ListenHeight)
        juce::jlimit (0, (int) Annotation::ListenHeight::Standing, which);
    for (int idx : selectedAnnots_)
    {
        if (idx < 0 || idx >= (int) annotations_.size()) continue;
        auto& a = annotations_[(size_t) idx];
        // Only a listening plane has ears on it.
        if (a.isPlane && a.planeType == Annotation::PlaneType::Listening)
            a.listenHgt = hgt;
    }
    repaint();
}

void RadiationPatternComponent::setPlaneOn (bool on)
{
    for (int idx : selectedAnnots_)
    {
        if (idx < 0 || idx >= (int) annotations_.size()) continue;
        auto& a = annotations_[(size_t) idx];
        if (Annotation::canBePlane (a.kind)) a.isPlane = on;
    }
    repaint();
}

void RadiationPatternComponent::setPlaneType (int type)
{
    const auto t = (Annotation::PlaneType)
        juce::jlimit (0, (int) Annotation::PlaneType::Architectural, type);
    for (int idx : selectedAnnots_)
    {
        if (idx < 0 || idx >= (int) annotations_.size()) continue;
        auto& a = annotations_[(size_t) idx];
        if (! Annotation::canBePlane (a.kind)) continue;
        a.planeType = t;
        // Choosing a type is choosing to be a plane - otherwise the buttons
        // look live but the shape never changes.
        a.isPlane = true;
    }
    repaint();
}

bool RadiationPatternComponent::hasTextTarget() const noexcept
{
    if (selectedAnnot_ >= 0 && selectedAnnot_ < (int) annotations_.size()
        && annotations_[(size_t) selectedAnnot_].kind == Annotation::Kind::TextBox)
        return true;

    // Nothing selected: the controls still set up the NEXT text box.
    return tool_ == Tool::Shape && drawShape_ == DrawShape::TextBox;
}

int RadiationPatternComponent::getActiveTextAlign() const noexcept
{
    if (selectedAnnot_ >= 0 && selectedAnnot_ < (int) annotations_.size())
    {
        const auto& a = annotations_[(size_t) selectedAnnot_];
        if (a.kind == Annotation::Kind::TextBox)
            return a.align;
    }
    return drawTextAlign_;
}

float RadiationPatternComponent::getActiveTextSize() const noexcept
{
    if (selectedAnnot_ >= 0 && selectedAnnot_ < (int) annotations_.size())
    {
        const auto& a = annotations_[(size_t) selectedAnnot_];
        if (a.kind == Annotation::Kind::TextBox)
            return a.fontPx;
    }
    return drawTextSize_;
}

void RadiationPatternComponent::setTextAlign (int align)
{
    drawTextAlign_ = juce::jlimit (0, 2, align);
    for (int idx : selectedAnnots_)
    {
        if (idx < 0 || idx >= (int) annotations_.size()) continue;
        auto& a = annotations_[(size_t) idx];
        if (a.kind == Annotation::Kind::TextBox)
            a.align = drawTextAlign_;
    }
    // A box being edited has to follow too, or the change only appears once
    // you click away. Relaying out reads the box's own alignment, so the
    // vertical setting is not dropped on the way through.
    if (isEditingTextBox()) layoutTextBoxEditor();
    repaint();
}

int RadiationPatternComponent::getActiveTextVAlign() const noexcept
{
    if (selectedAnnot_ >= 0 && selectedAnnot_ < (int) annotations_.size())
    {
        const auto& a = annotations_[(size_t) selectedAnnot_];
        if (a.kind == Annotation::Kind::TextBox) return a.valign;
    }
    return drawTextVAlign_;
}

bool RadiationPatternComponent::getActiveTextBold() const noexcept
{
    if (selectedAnnot_ >= 0 && selectedAnnot_ < (int) annotations_.size())
    {
        const auto& a = annotations_[(size_t) selectedAnnot_];
        if (a.kind == Annotation::Kind::TextBox) return a.bold;
    }
    return drawTextBold_;
}

bool RadiationPatternComponent::getActiveTextItalic() const noexcept
{
    if (selectedAnnot_ >= 0 && selectedAnnot_ < (int) annotations_.size())
    {
        const auto& a = annotations_[(size_t) selectedAnnot_];
        if (a.kind == Annotation::Kind::TextBox) return a.italic;
    }
    return drawTextItalic_;
}

float RadiationPatternComponent::getActiveTextLineHeight() const noexcept
{
    if (selectedAnnot_ >= 0 && selectedAnnot_ < (int) annotations_.size())
    {
        const auto& a = annotations_[(size_t) selectedAnnot_];
        if (a.kind == Annotation::Kind::TextBox) return a.lineHeight;
    }
    return drawTextLineHeight_;
}

void RadiationPatternComponent::setTextLineHeight (float mult)
{
    drawTextLineHeight_ = juce::jlimit (1.0f, 3.0f, mult);
    for (int idx : selectedAnnots_)
    {
        if (idx < 0 || idx >= (int) annotations_.size()) continue;
        auto& a = annotations_[(size_t) idx];
        if (a.kind == Annotation::Kind::TextBox)
        {
            a.lineHeight = drawTextLineHeight_;
            growTextBoxToFit (idx);       // taller lines need a taller box
        }
    }
    if (isEditingTextBox()) layoutTextBoxEditor();
    repaint();
}

void RadiationPatternComponent::setTextVAlign (int valign)
{
    drawTextVAlign_ = juce::jlimit (0, 2, valign);
    for (int idx : selectedAnnots_)
    {
        if (idx < 0 || idx >= (int) annotations_.size()) continue;
        auto& a = annotations_[(size_t) idx];
        if (a.kind == Annotation::Kind::TextBox) a.valign = drawTextVAlign_;
    }
    if (isEditingTextBox()) layoutTextBoxEditor();
    repaint();
}

void RadiationPatternComponent::setTextBold (bool on)
{
    drawTextBold_ = on;
    for (int idx : selectedAnnots_)
    {
        if (idx < 0 || idx >= (int) annotations_.size()) continue;
        auto& a = annotations_[(size_t) idx];
        if (a.kind == Annotation::Kind::TextBox) { a.bold = on; growTextBoxToFit (idx); }
    }
    if (isEditingTextBox()) layoutTextBoxEditor();
    repaint();
}

void RadiationPatternComponent::setTextItalic (bool on)
{
    drawTextItalic_ = on;
    for (int idx : selectedAnnots_)
    {
        if (idx < 0 || idx >= (int) annotations_.size()) continue;
        auto& a = annotations_[(size_t) idx];
        if (a.kind == Annotation::Kind::TextBox) { a.italic = on; growTextBoxToFit (idx); }
    }
    if (isEditingTextBox()) layoutTextBoxEditor();
    repaint();
}

void RadiationPatternComponent::growTextBoxToFit (int index)
{
    if (index < 0 || index >= (int) annotations_.size()) return;
    auto& a = annotations_[(size_t) index];
    if (a.kind != Annotation::Kind::TextBox || a.pts.size() < 2) return;

    const auto local = textBoxLocalRect (a);
    if (local.getWidth() < 1.0e-6f || local.getHeight() < 1.0e-6f) return;

    const auto sTL = annotateToScreen (a, { local.getX(), local.getY() });
    const auto sTR = annotateToScreen (a, { local.getRight(), local.getY() });
    const auto sBL = annotateToScreen (a, { local.getX(), local.getBottom() });
    const float wPx = sTL.getDistanceFrom (sTR);
    const float hPx = sTL.getDistanceFrom (sBL);
    if (wPx < 6.0f || hPx < 1.0f) return;

    // Measure the wrapped text at the width the box already has.
    const juce::String label =
        (isEditingTextBox() && textEditIndex_ == index && textEdit_ != nullptr)
            ? textEdit_->getText() : a.text;
    if (label.isEmpty()) return;

    const float pad = kTextBoxPad;
    juce::GlyphArrangement ga;
    const auto font = textBoxFont (a, wPx, hPx, label);
    // Baseline at the ascent, exactly as the painter lays it out. Starting at
    // zero put the first line's ascenders ABOVE the origin, so the height came
    // back one line short and the last line was clipped.
    ga.addJustifiedText (font, label, 0.0f, font.getAscent(),
                         juce::jmax (10.0f, wPx - pad * 2.0f),
                         juce::Justification::topLeft,
                         textBoxLeading (a, font));
    const float needPx = ga.getBoundingBox (0, -1, true).getBottom() + pad * 2.0f;
    if (needPx <= hPx + 0.5f) return;          // grow only, never shrink

    const float pxPerLocal = hPx / local.getHeight();
    if (pxPerLocal < 1.0e-6f) return;
    const float needLocal = needPx / pxPerLocal;

    // Grow DOWNWARDS on screen. Which local edge that is depends on the space,
    // so ask the transform rather than assuming world Y runs one way.
    const bool bottomEdgeIsLower = (annotateToScreen (a, { local.getX(), local.getBottom() }).y
                                    > annotateToScreen (a, { local.getX(), local.getY() }).y);
    const float keepY = bottomEdgeIsLower ? local.getY() : local.getBottom();
    const float newOther = bottomEdgeIsLower ? (keepY + needLocal) : (keepY - needLocal);

    a.pts[0] = { local.getX(),     keepY };
    a.pts[1] = { local.getRight(), newOther };
}

void RadiationPatternComponent::setTextSize (float px)
{
    drawTextSize_ = (px <= 0.5f) ? 0.0f : juce::jlimit (5.0f, 200.0f, px);
    for (int idx : selectedAnnots_)
    {
        if (idx < 0 || idx >= (int) annotations_.size()) continue;
        auto& a = annotations_[(size_t) idx];
        if (a.kind == Annotation::Kind::TextBox)
        {
            a.fontPx = drawTextSize_;
            growTextBoxToFit (idx);       // bigger letters need a bigger box
        }
    }
    if (isEditingTextBox() && textEdit_ != nullptr)
        layoutTextBoxEditor();
    repaint();
}

bool RadiationPatternComponent::hasFillTarget() const noexcept
{
    if (selectedAnnot_ >= 0 && selectedAnnot_ < (int) annotations_.size()
        && isFilledShapeKind (annotations_[(size_t) selectedAnnot_].kind))
        return true;

    // Nothing selected: the slider still sets the fill for the NEXT shape, but
    // only if that shape actually has a fill.
    if (tool_ == Tool::Shape)
        return drawShape_ == DrawShape::Circle
            || drawShape_ == DrawShape::Rectangle
            || drawShape_ == DrawShape::Square;

    return false;
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
        if (addSpeakerArmed_)
        {
            addSpeakerArmed_ = false;
            if (onAddSpeakerArmedChanged) onAddSpeakerArmedChanged();
        }
        setTool (Tool::Select);
        setSelectedAnnotation (-1);
    }
    updateMouseCursorForTool();
    if (onAddMicArmedChanged) onAddMicArmedChanged();
    updateDrawPrompt();
    repaint();
}

void RadiationPatternComponent::setAddSpeakerArmed (bool armed)
{
    if (addSpeakerArmed_ == armed) return;
    addSpeakerArmed_ = armed;
    if (armed)
    {
        if (addMicArmed_)
        {
            addMicArmed_ = false;
            if (onAddMicArmedChanged) onAddMicArmedChanged();
        }
        setTool (Tool::Select);
        setSelectedAnnotation (-1);
    }
    updateMouseCursorForTool();
    if (onAddSpeakerArmedChanged) onAddSpeakerArmedChanged();
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
    if (isEditingTextBox())
        endTextBoxEdit (true);
    selectedAnnots_.clear();
    selectedMics_.clear();
    selectedSpeakers_.clear();
    selectedAnnot_ = -1;
    selectedMic_ = -1;
    // The speaker has to let go too. It used to be left behind here: the set
    // emptied but selected_ kept pointing at the last unit, so one speaker was
    // always current - highlighted on the plot, its fields live in the
    // sidebar - and clicking empty field could not give you an empty
    // selection. Telling the sidebar is part of it; without that the panel
    // keeps editing a unit the plot no longer shows as chosen.
    const bool hadSpeaker = (selected_ >= 0);
    selected_ = -1;
    if (hadSpeaker && onSpeakerSelected) onSpeakerSelected (-1);
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
    // Empty set means nothing selected, not "keep the last one".
    selected_ = selectedSpeakers_.empty() ? -1 : selectedSpeakers_.back();
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
    // Ring snap: pull onto one of the user's own range rings, and only the
    // ones on screen. Tak only when newly latching.
    const auto snap = MicRingSnap::snapToRing (wx, wy, speakers_, activeRangeRings());
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
    ensureWorldExtents();
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
        const auto snap = MicRingSnap::snapToRing (sx, sy, speakers_, activeRangeRings());
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

juce::String RadiationPatternComponent::micCoordText (const MicReceiver& m)
{
    // Converted through Units, like every other length on the plot, so the
    // mic does not read in metres while the axes beside it read in feet.
    auto one = [] (float metres)
    {
        return juce::String (Units::metresToDisplay ((double) metres), 1);
    };
    return one (m.x) + ", " + one (m.y) + " " + Units::lengthUnit();
}

juce::String RadiationPatternComponent::micLabelText (const MicReceiver& m) const
{
    juce::String label = micDisplayName (m);
    // Where the mic actually stands, in the unit on show - the same reading a
    // speaker carries in its X / Y fields. A level means little without the
    // seat it was taken at, and reading the position off the axes by eye is
    // guesswork at any useful zoom.
    if (showMicCoords_)
        label += "  " + micCoordText (m);
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

        // Same flip the painter applies, or the label you can see beside a
        // mic at the right-hand edge would not be the label you can click.
        const juce::String label = micLabelText (m);
        const float tw = juce::jmax (40.0f, (float) label.length() * 7.0f + 12.0f);
        const auto pb = plotArea().toFloat();
        float boxX = s.x + 8.0f;
        if (boxX + tw > pb.getRight() - 2.0f)
            boxX = juce::jmax (pb.getX() + 2.0f, s.x - 10.0f - tw);
        const auto box = juce::Rectangle<float> (boxX, s.y - 12.0f, tw, 20.0f);
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
        const float fh = juce::jmax (11.0f, Brand::UI::scaledFont (12.5f));
        g.setFont (Brand::tech (fh, true));
        // Charcoal, not panelDark: this pill sits on the canvas, which is
        // always dark, while panelDark follows the THEME and is plain white in
        // the light one - so the white reading below was drawn white on white.
        g.setColour (Brand::charcoal().withAlpha (0.94f));
        const float tw = (float) g.getCurrentFont().getStringWidth (label) + fh;
        const float th = fh * 1.55f;
        // The reading sits to the right of the marker, unless that would push
        // it off the plot - a mic placed near the right-hand edge used to have
        // half its label clipped away, and carrying the coordinates makes the
        // label long enough for that to be the common case rather than a rare
        // one. It flips to the left when it does not fit.
        const auto pb = plotArea().toFloat();
        float boxX = s.x + 10.0f;
        if (boxX + tw > pb.getRight() - 2.0f)
            boxX = juce::jmax (pb.getX() + 2.0f, s.x - 10.0f - tw);
        auto box = juce::Rectangle<float> (boxX, s.y - th * 0.5f, tw, th);
        g.fillRoundedRectangle (box, 3.0f);
        // A hairline in the mic's own colour keeps the reading tied to its
        // marker when several sit close together.
        g.setColour ((sel ? Brand::accent() : Brand::white()).withAlpha (0.55f));
        g.drawRoundedRectangle (box, 3.0f, 1.0f);
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
    if (addSpeakerArmed_)
        p = "Click the plot to place the unit (Esc cancels)";
    else if (addMicArmed_)
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
            construction_ == Construction::TextBoxClick       ? "Click" :
                                                                "2 Corners";

        const int n = (int) sessionPts_.size();
        const int need = pointsNeeded();
        juce::String step;
        if (drawShape_ == DrawShape::TextBox)
            step = "click to place text";
        else if (drawShape_ == DrawShape::Polyline)
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
        p += ": select 2+ speakers - linked gap ";
        p += Units::metres ((double) orthoSpacingM_, 2);
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
        case Construction::TextBoxClick:   return 1; // click → place + type
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

juce::Rectangle<float> RadiationPatternComponent::rotatableLocalRect (const Annotation& a) noexcept
{
    if (a.pts.size() < 2) return {};
    return a.kind == Annotation::Kind::TextBox
             ? textBoxLocalRect (a)
             : normalisedShapeRect (a.pts[0], a.pts[1], a.kind);
}

juce::Rectangle<float> RadiationPatternComponent::annotationAnnotBounds (
    const Annotation& a) const
{
    if (a.pts.empty()) return {};

    if (a.kind == Annotation::Kind::Circle && a.pts.size() >= 2)
        return normalisedShapeRect (a.pts[0], a.pts[1], a.kind);

    if (Annotation::isRotatable (a.kind) && a.pts.size() >= 2)
    {
        const auto local = rotatableLocalRect (a);
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

    // 1) Fixed snap step from unit system (SI: 100 mm, Imperial: 1 ft).
    //    Object-edge snap below still overrides when closer.
    const float step = (float) Units::snapStepMetres();
    float x = p.x;
    float y = p.y;
    if (step > 1.0e-9f)
    {
        x = std::round (p.x / step) * step;
        y = std::round (p.y / step) * step;
    }

    // 2) Object snap (edges / corners / centres) - wins when closer than ~10 px.
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

    // Tak / objectHit: only other drawing edges - not speakers, mics, or centres
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
    // Drawing constraint: Line Ortho construction or hold Shift - not the Ortho align tool.
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

bool RadiationPatternComponent::feedAnnotPoint (juce::Point<float> annotPt)
{
    if (tool_ == Tool::Ruler)
    {
        // Mirror two-click ruler via the same Measure annotation path.
        if (! pendingAnchor_)
        {
            bool objHit = false;
            auto p = snapAnnotPointFull (annotPt, false, &objHit);
            noteSnapSound (objHit, annotPt, p);
            pendingAnchor_ = true;
            pendingStartWorld_ = p;
            hoverAnnot_ = p;
            hoverValid_ = true;
            updateDrawPrompt();
            repaint();
            return true;
        }

        bool objHit = false;
        auto p = snapAnnotPointFull (annotPt, false, &objHit);
        noteSnapSound (objHit, annotPt, p);
        p = applyOrtho (pendingStartWorld_, p);
        if (pendingStartWorld_.getDistanceFrom (p) > 1.0e-4f)
        {
            if (onWillEdit) onWillEdit();
            Annotation a;
            a.kind = Annotation::Kind::Measure;
            a.space = currentAnnotSpace();
            a.colour = drawColour_;
            a.thicknessPx = 1.8f;
            a.pts = { pendingStartWorld_, p };
            annotations_.push_back (std::move (a));
            if (onEditCommitted) onEditCommitted();
        }
        pendingAnchor_ = false;
        hoverValid_ = false;
        updateDrawPrompt();
        repaint();
        return true;
    }

    if (tool_ != Tool::Shape)
        return false;
    return acceptAnnotPoint (annotPt);
}

bool RadiationPatternComponent::finishPolylineCommand (bool forceClose)
{
    if (tool_ != Tool::Shape || drawShape_ != DrawShape::Polyline)
        return false;
    return finishPolyline (forceClose);
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
        // A new box inherits the ribbon's current Text settings, the same way
        // a new shape inherits the opacity slider.
        a.align  = drawTextAlign_;
        a.valign = drawTextVAlign_;
        a.fontPx = drawTextSize_;
        a.bold   = drawTextBold_;
        a.italic = drawTextItalic_;
        a.lineHeight = drawTextLineHeight_;
        a.rotationDeg = 0.0f;
        a.thicknessPx = 1.5f;
        a.text = "Text";

        // One click: default-sized box centred on the click (screen-stable size).
        const float sx = juce::jmax (1.0e-3f, worldScaleX());
        const float sy = juce::jmax (1.0e-3f, worldScaleY());
        float halfW = 70.0f / sx;   // ~140 px wide
        float halfH = 28.0f / sy;   // ~56 px tall
        if (currentAnnotSpace() == AnnotSpace::PolarPlot)
        {
            halfW = 0.18f;
            halfH = 0.08f;
        }
        const auto c = sessionPts_[0];
        a.pts = { { c.x - halfW, c.y - halfH }, { c.x + halfW, c.y + halfH } };

        commitAnnotation (std::move (a));
        setTool (Tool::Select);
        beginTextBoxEdit ((int) annotations_.size() - 1);
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
    // Polar overlays, post-RUN heatmap, or empty world grid before RUN.
    if (params_.viewMode == ViewMode::Directivity
        || params_.viewMode == ViewMode::MeasuredPolar)
        return true;
    return hasData_ || result_.worldW > 0.0 || params_.worldW > 0.0;
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

    // Indices shift after erase - clear selection to avoid pointing at the wrong shape.
    setSelectedAnnotation (-1);
}

bool RadiationPatternComponent::isFilledShapeKind (Annotation::Kind k) noexcept
{
    // TextBox is deliberately NOT here. It has no fill, and emphasis is what
    // Bold is for, so the Opacity control greys out for one instead of
    // offering a slider that only fades the words.
    return k == Annotation::Kind::Rectangle
        || k == Annotation::Kind::Square
        || k == Annotation::Kind::Circle;
}

int RadiationPatternComponent::annotationBorderHitTest (juce::Point<float> annotPt,
                                                        float radius) const
{
    const auto space = currentAnnotSpace();
    for (int i = (int) annotations_.size() - 1; i >= 0; --i)
    {
        const auto& a = annotations_[(size_t) i];
        if (a.space != space) continue;

        if (a.kind == Annotation::Kind::TextBox)
        {
            if (pointHitsTextBoxBorder (annotPt, a, radius))
                return i;
            continue;
        }

        if (a.kind == Annotation::Kind::Rectangle
            || a.kind == Annotation::Kind::Square
            || a.kind == Annotation::Kind::Circle
            || a.kind == Annotation::Kind::Arc
            || a.kind == Annotation::Kind::Polyline)
        {
            if (pointHitsShapeBorder (annotPt, a, radius))
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

int RadiationPatternComponent::annotationFillHitTest (juce::Point<float> annotPt,
                                                      float radius) const
{
    const auto space = currentAnnotSpace();
    for (int i = (int) annotations_.size() - 1; i >= 0; --i)
    {
        const auto& a = annotations_[(size_t) i];
        if (a.space != space) continue;

        if (a.kind == Annotation::Kind::TextBox)
        {
            if (pointHitsTextBoxFill (annotPt, a, radius))
                return i;
            continue;
        }

        if (a.kind == Annotation::Kind::Rectangle
            || a.kind == Annotation::Kind::Square
            || a.kind == Annotation::Kind::Circle)
        {
            if (pointHitsShapeFill (annotPt, a, radius))
                return i;
        }
    }
    return -1;
}

bool RadiationPatternComponent::annotationHitsPoint (const Annotation& a,
                                                     juce::Point<float> annotPt,
                                                     float radius) const
{
    if (a.kind == Annotation::Kind::TextBox)
        return pointHitsTextBox (annotPt, a, radius);

    if (a.kind == Annotation::Kind::Rectangle
        || a.kind == Annotation::Kind::Square
        || a.kind == Annotation::Kind::Circle)
        return pointHitsShapeBorder (annotPt, a, radius)
            || pointHitsShapeFill (annotPt, a, radius);

    if (a.kind == Annotation::Kind::Arc
        || a.kind == Annotation::Kind::Polyline)
        return pointHitsShapeBorder (annotPt, a, radius);

    if (a.pts.empty()) return false;
    if (a.pts.size() == 1)
        return annotPt.getDistanceFrom (a.pts.front()) <= radius;

    for (size_t j = 1; j < a.pts.size(); ++j)
        if (distPointToSegment (annotPt, a.pts[j - 1], a.pts[j]) <= radius)
            return true;
    return false;
}

std::vector<int> RadiationPatternComponent::annotationsUnder (juce::Point<float> annotPt,
                                                              float radius) const
{
    std::vector<int> out;
    const auto space = currentAnnotSpace();
    for (int i = (int) annotations_.size() - 1; i >= 0; --i)
    {
        const auto& a = annotations_[(size_t) i];
        if (a.space != space) continue;
        if (annotationHitsPoint (a, annotPt, radius))
            out.push_back (i);
    }
    return out;
}

int RadiationPatternComponent::annotationHitTest (juce::Point<float> annotPt,
                                                  float radius) const
{
    const int border = annotationBorderHitTest (annotPt, radius);
    if (border >= 0) return border;
    return annotationFillHitTest (annotPt, radius);
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
        // Group / mixed selection move: free translate only - ring snap is
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
    if (Annotation::isRotatable (a.kind) && a.pts.size() >= 2)
    {
        const auto r = rotatableLocalRect (a);
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
    // Larger hit target for the text-box rotate disc.
    const float r = (a.kind == Annotation::Kind::TextBox) ? radius * 1.75f : radius;
    const auto handles = resizeHandlesFor (a);
    for (int i = (int) handles.size() - 1; i >= 0; --i)
        if (annotPt.getDistanceFrom (handles[(size_t) i]) <= r)
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

    if (Annotation::isRotatable (a.kind) && a.pts.size() >= 2 && handleIndex < 4)
    {
        const auto r = rotatableLocalRect (a);
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
    else if (Annotation::isRotatable (a.kind) && a.pts.size() >= 2)
    {
        // Selection: outline + corner resize grips + rotate arrows.
        const auto local = rotatableLocalRect (a);
        const auto c = local.getCentre();
        const juce::Point<float> corners[4] = {
            rotateAround ({ local.getX(), local.getY() }, c, a.rotationDeg),
            rotateAround ({ local.getRight(), local.getY() }, c, a.rotationDeg),
            rotateAround ({ local.getRight(), local.getBottom() }, c, a.rotationDeg),
            rotateAround ({ local.getX(), local.getBottom() }, c, a.rotationDeg)
        };
        juce::Path outline;
        auto s0 = annotateToScreen (a, corners[0]);
        outline.startNewSubPath (s0);
        for (int i = 1; i < 4; ++i)
            outline.lineTo (annotateToScreen (a, corners[i]));
        outline.closeSubPath();
        g.setColour (Brand::accent().withAlpha (0.9f));
        g.strokePath (outline, juce::PathStrokeType (1.4f));

        const bool showGrips = (selectedAnnots_.size() == 1
                                && selectedMics_.empty()
                                && selectedSpeakers_.empty());
        const auto handles = resizeHandlesFor (a);
        for (size_t hi = 0; hi < handles.size(); ++hi)
        {
            auto s = annotateToScreen (a, handles[hi]);
            if (hi == 4)
            {
                if (showGrips)
                    drawTextBoxRotateIcon (g, s, 9.0f);
                continue;
            }
            g.setColour (Brand::accent());
            if (showGrips)
                g.fillEllipse (s.x - 4.5f, s.y - 4.5f, 9.0f, 9.0f);
            else
                g.drawEllipse (s.x - 3.5f, s.y - 3.5f, 7.0f, 7.0f, 1.4f);
            g.setColour (Brand::panelDark());
            g.drawEllipse (s.x - 4.5f, s.y - 4.5f, 9.0f, 9.0f, 1.2f);
        }
        return;
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
        g.setColour (Brand::accent());
        if (showGrips)
            g.fillEllipse (s.x - 4.5f, s.y - 4.5f, 9.0f, 9.0f);
        else
            g.drawEllipse (s.x - 3.5f, s.y - 3.5f, 7.0f, 7.0f, 1.4f);
        g.setColour (Brand::panelDark());
        g.drawEllipse (s.x - 4.5f, s.y - 4.5f, 9.0f, 9.0f, 1.2f);
    }
}

juce::Rectangle<float> RadiationPatternComponent::annotationScreenBounds (
    const Annotation& a) const
{
    if (a.pts.empty()) return {};

    if (a.kind == Annotation::Kind::Circle && a.pts.size() >= 2)
    {
        const auto wr = normalisedShapeRect (a.pts[0], a.pts[1], a.kind);
        auto s0 = annotateToScreen (a, { wr.getX(), wr.getY() });
        auto s1 = annotateToScreen (a, { wr.getRight(), wr.getBottom() });
        return juce::Rectangle<float>::leftTopRightBottom (
            juce::jmin (s0.x, s1.x), juce::jmin (s0.y, s1.y),
            juce::jmax (s0.x, s1.x), juce::jmax (s0.y, s1.y)).expanded (3.0f);
    }

    if (Annotation::isRotatable (a.kind) && a.pts.size() >= 2)
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
    // Plain click (no drag): clear selection when not additive.
    if (box.getWidth() < 3.0f && box.getHeight() < 3.0f)
    {
        if (! addToExisting)
        {
            clearPlotSelection();
            if (onSpeakerSelected) onSpeakerSelected (-1);
        }
        return;
    }

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
    return pointHitsTextBoxFill (pt, a, radius)
        || pointHitsTextBoxBorder (pt, a, radius);
}

bool RadiationPatternComponent::pointHitsTextBoxFill (juce::Point<float> pt,
                                                      const Annotation& a,
                                                      float radius) const noexcept
{
    const auto local = textBoxLocalRect (a);
    if (local.isEmpty()) return false;
    const auto c = local.getCentre();
    const auto unrot = rotateAround (pt, c, -a.rotationDeg);
    return local.expanded (radius).contains (unrot);
}

bool RadiationPatternComponent::pointHitsTextBoxBorder (juce::Point<float> pt,
                                                        const Annotation& a,
                                                        float radius) const noexcept
{
    const auto local = textBoxLocalRect (a);
    if (local.isEmpty()) return false;
    const auto c = local.getCentre();
    const auto unrot = rotateAround (pt, c, -a.rotationDeg);
    const auto expanded = local.expanded (radius);
    if (! expanded.contains (unrot)) return false;
    const auto shrunk = local.reduced (radius);
    if (shrunk.getWidth() <= 0.0f || shrunk.getHeight() <= 0.0f)
        return true; // thin box - whole area is border
    return ! shrunk.contains (unrot);
}

/** The run of word characters surrounding @p caret - what a double click
    selects. An empty range when the caret sits between two separators. */
static juce::Range<int> wordAround (const juce::String& text, int caret)
{
    const int len = text.length();
    caret = juce::jlimit (0, len, caret);
    auto isWord = [&] (int i)
    {
        if (i < 0 || i >= len) return false;
        const auto ch = text[i];
        return juce::CharacterFunctions::isLetterOrDigit (ch) || ch == '_';
    };
    // Landing just past the end of a word counts as being in it, the way it
    // does when you double click at the right edge of one.
    int at = isWord (caret) ? caret : caret - 1;
    if (! isWord (at)) return { caret, caret };

    int start = at, end = at + 1;
    while (isWord (start - 1)) --start;
    while (isWord (end)) ++end;
    return { start, end };
}

void RadiationPatternComponent::beginTextBoxEdit (int index,
                                                  const juce::Point<float>* clickInComponent)
{
    if (index < 0 || index >= (int) annotations_.size()) return;
    if (annotations_[(size_t) index].kind != Annotation::Kind::TextBox) return;

    if (isEditingTextBox() && textEditIndex_ == index)
    {
        if (textEdit_ != nullptr)
            textEdit_->grabKeyboardFocus();
        return;
    }

    endTextBoxEdit (true);

    selectedAnnots_ = { index };
    selectedMics_.clear();
    selectedSpeakers_.clear();
    syncPrimarySelectionFromSets();
    if (onAnnotSelectionChanged) onAnnotSelectionChanged();

    textEditIndex_ = index;
    auto& a = annotations_[(size_t) index];

    textEdit_ = std::make_unique<juce::TextEditor>();
    textEdit_->setMultiLine (true, true);
    textEdit_->setReturnKeyStartsNewLine (true);
    textEdit_->setScrollbarsShown (false);
    textEdit_->setCaretVisible (true);
    textEdit_->setPopupMenuEnabled (true);
    // Brand::muted() follows the theme and disappeared on the dark canvas;
    // a faded copy of the text's own colour reads on either.
    textEdit_->setTextToShowWhenEmpty ("Type here",
        juce::Colour::fromFloatRGBA (a.colour.getFloatRed(), a.colour.getFloatGreen(),
                                     a.colour.getFloatBlue(), 0.45f));
    textEdit_->setText (a.text == "Text" ? juce::String() : a.text, false);
    // Set AFTER the text exists: a colour set beforehand only reaches text
    // added later, which is why the words went dark the moment you clicked in.
    // (The setColour calls below still matter - they catch what you type.)

    const auto base = juce::Colour::fromFloatRGBA (a.colour.getFloatRed(),
                                                   a.colour.getFloatGreen(),
                                                   a.colour.getFloatBlue(),
                                                   1.0f);
    const auto ink = base;

    textEdit_->setColour (juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    textEdit_->setColour (juce::TextEditor::textColourId, ink);
    textEdit_->setColour (juce::TextEditor::highlightColourId, Brand::accent().withAlpha (0.35f));
    // Without this, selected text is redrawn in the look-and-feel's colour and
    // a select-all turned the whole box a different colour.
    textEdit_->setColour (juce::TextEditor::highlightedTextColourId, ink);
    textEdit_->setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    textEdit_->setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    textEdit_->setColour (juce::CaretComponent::caretColourId, ink);
    // The painter insets the text by `pad`. A TextEditor insets by its border
    // PLUS its own indent, which defaults to 4 - set both, so the sum is the
    // pad exactly and the words do not step sideways when you click in.
    textEdit_->setBorder (juce::BorderSize<int> (0));
    textEdit_->setIndents (0, 0);   // the pad lives in the bounds instead
    textEdit_->applyColourToAllText (ink);
    textEdit_->setOpaque (false);

    textEdit_->onTextChange = [this]
    {
        // Figma's auto height: the box follows the text as it wraps, so you
        // never type into a container that silently clips what you wrote.
        if (textEditIndex_ >= 0)
        {
            growTextBoxToFit (textEditIndex_);
            layoutTextBoxEditor();
            repaint();
        }
    };
    textEdit_->onEscapeKey = [this]
    {
        endTextBoxEdit (true);
        return true;
    };
    textEdit_->onFocusLost = [this]
    {
        if (! textEditClosing_)
            endTextBoxEdit (true);
    };

    addAndMakeVisible (*textEdit_);
    layoutTextBoxEditor();
    textEdit_->grabKeyboardFocus();

    // Word puts the caret where you clicked and selects the word you landed
    // on; it only selects everything on a box that has nothing in it yet.
    const auto& typed = textEdit_->getText();
    if (clickInComponent != nullptr && typed.isNotEmpty())
    {
        const auto lp = textEdit_->getLocalPoint (this, *clickInComponent);
        const int caret = textEdit_->getTextIndexAt ((int) lp.x, (int) lp.y);
        textEdit_->setCaretPosition (caret);
        textEdit_->setHighlightedRegion (wordAround (typed, caret));
    }
    else
    {
        textEdit_->selectAll();
    }
    repaint();
}

void RadiationPatternComponent::endTextBoxEdit (bool commit)
{
    if (textEdit_ == nullptr && textEditIndex_ < 0) return;
    if (textEditClosing_) return;
    textEditClosing_ = true;

    const int idx = textEditIndex_;
    juce::String next;
    if (textEdit_ != nullptr)
        next = textEdit_->getText();   // as typed - Word keeps your spaces

    textEdit_.reset();
    textEditIndex_ = -1;
    textEditClosing_ = false;

    if (commit && idx >= 0 && idx < (int) annotations_.size()
        && annotations_[(size_t) idx].kind == Annotation::Kind::TextBox)
    {
        auto& a = annotations_[(size_t) idx];
        const auto finalText = next.trim().isNotEmpty() ? next : juce::String ("Text");
        if (a.text != finalText)
        {
            if (onWillEdit) onWillEdit();
            a.text = finalText;
            if (onEditCommitted) onEditCommitted();
        }
    }

    grabKeyboardFocus();
    repaint();
}

void RadiationPatternComponent::layoutTextBoxEditor()
{
    if (textEdit_ == nullptr || textEditIndex_ < 0
        || textEditIndex_ >= (int) annotations_.size())
        return;

    const auto& a = annotations_[(size_t) textEditIndex_];

    // Lay the editor out in the box's OWN frame - an upright rectangle the
    // size of the box - and turn it with a transform. Measured exactly as the
    // painter measures it, so clicking into a box does not shift the words,
    // and JUCE maps the mouse back through the transform, so the caret lands
    // under the pointer on a turned box as readily as on a straight one.
    const auto local = textBoxLocalRect (a);
    const auto lc = local.getCentre();
    const juce::Point<float> corners[4] = {
        rotateAround ({ local.getX(), local.getY() }, lc, a.rotationDeg),
        rotateAround ({ local.getRight(), local.getY() }, lc, a.rotationDeg),
        rotateAround ({ local.getRight(), local.getBottom() }, lc, a.rotationDeg),
        rotateAround ({ local.getX(), local.getBottom() }, lc, a.rotationDeg)
    };
    const auto sc = annotateToScreen (a, lc);
    const auto sA = annotateToScreen (a, corners[0]);
    const auto sB = annotateToScreen (a, corners[1]);
    const auto sD = annotateToScreen (a, corners[3]);
    const float boxW = juce::jmax (40.0f, sA.getDistanceFrom (sB));
    const float boxH = juce::jmax (24.0f, sA.getDistanceFrom (sD));

    // Give the editor exactly the rectangle the painter lays text into, and
    // no indents of its own. JUCE takes the top indent off the height it
    // centres in but still draws from it, so any indent at all left the words
    // half a pad low; it also keeps 2px spare on the right, so the box is
    // widened by that much for the two to centre on the same middle.
    constexpr int kRightEdgeSpace = 2;              // juce::TextEditor's own
    auto bounds = juce::Rectangle<float> (
                      sc.x - boxW * 0.5f + kTextBoxPad,
                      sc.y - boxH * 0.5f + kTextBoxPad,
                      juce::jmax (1.0f, boxW - 2.0f * kTextBoxPad + kRightEdgeSpace),
                      juce::jmax (1.0f, boxH - 2.0f * kTextBoxPad))
                  .getSmallestIntegerContainer();

    const float hPx = boxH;
    // Was 0.28 of the box height while the painter used 0.22, so the text
    // visibly jumped the moment you stopped editing. One helper now answers
    // for both.
    //
    // applyFontToAllText, NOT setFont: setFont only governs text typed from
    // here on, so the words already in the box kept the look-and-feel's
    // default face and the box changed size and colour the instant you
    // clicked into it.
    // Fitted to what is being typed, so in Auto the words shrink as they
    // fill the box rather than running out of it.
    textEdit_->applyFontToAllText (textBoxFont (a, boxW, hPx, textEdit_->getText()));
    textEdit_->setJustification (textBoxJustification (a.align, a.valign));
    textEdit_->setLineSpacing (juce::jlimit (1.0f, 3.0f, a.lineHeight));
    textEdit_->setBounds (bounds);

    // Lean the editor itself, so the words do not straighten up the moment you
    // click into them. A component takes one transform, applied in the parent's
    // space, so this pivots on the first baseline rather than per line - on a
    // multi-line box the later lines sit a couple of pixels off where they
    // land once committed.
    juce::AffineTransform t;
    if (a.italic)
    {
        // A component takes ONE transform, applied in the parent's space, while
        // the painter leans each glyph about its own baseline. They can only
        // agree on one line, so agree on the middle one: the error then splits
        // either side of centre instead of piling up down the box.
        const float th = (float) textEdit_->getTextHeight();
        float blockTop = (float) bounds.getY();
        if (a.valign == 1)      blockTop += ((float) bounds.getHeight() - th) * 0.5f;
        else if (a.valign == 2) blockTop += (float) bounds.getHeight() - th;
        const float anchor = blockTop + th * 0.5f;
        t = juce::AffineTransform::translation (0.0f, -anchor)
                .sheared (-kItalicShear, 0.0f)
                .translated (0.0f, anchor);
    }
    if (std::abs (a.rotationDeg) > 0.01f)
        t = t.followedBy (juce::AffineTransform::rotation (
                juce::degreesToRadians (-a.rotationDeg), sc.x, sc.y));
    textEdit_->setTransform (t);

    textEdit_->toFront (false);
}

void RadiationPatternComponent::drawTextBoxRotateIcon (juce::Graphics& g,
                                                       juce::Point<float> centre,
                                                       float radius)
{
    // Three curved arrows only - no disc / red ring.
    const float arcR = radius * 0.72f;
    const float stroke = juce::jmax (1.4f, radius * 0.18f);

    for (int i = 0; i < 3; ++i)
    {
        const float startDeg = (float) i * 120.0f - 40.0f;
        const float endDeg   = startDeg + 78.0f;
        const float startRad = juce::degreesToRadians (startDeg);
        const float endRad   = juce::degreesToRadians (endDeg);

        juce::Path arc;
        arc.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startRad, endRad, true);
        g.setColour (Brand::white());
        g.strokePath (arc, juce::PathStrokeType (stroke,
                                                  juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));

        const float tx = centre.x + arcR * std::cos (endRad);
        const float ty = centre.y + arcR * std::sin (endRad);
        const float tang = endRad + 0.5f * (float) M_PI;
        const float dirX = std::cos (tang), dirY = std::sin (tang);
        const float sideX = -dirY, sideY = dirX;
        const float tipLen = radius * 0.36f;
        juce::Path tip;
        tip.addTriangle (tx + tipLen * dirX,
                         ty + tipLen * dirY,
                         tx - tipLen * 0.45f * dirX + tipLen * 0.55f * sideX,
                         ty - tipLen * 0.45f * dirY + tipLen * 0.55f * sideY,
                         tx - tipLen * 0.45f * dirX - tipLen * 0.55f * sideX,
                         ty - tipLen * 0.45f * dirY - tipLen * 0.55f * sideY);
        g.fillPath (tip);
    }
}

juce::Font RadiationPatternComponent::textBoxFont (const Annotation& a,
                                                   float boxWidthPx,
                                                   float boxHeightPx,
                                                   const juce::String& text) const
{
    // Weight only. Slant is NOT set here: see kItalicShear - there is no
    // italic Montserrat to switch to, so asking the Font for one silently
    // returned the upright face. The shear is applied where the glyphs are
    // drawn instead.
    return Brand::tech (textBoxFontScreenPx (a, boxWidthPx, boxHeightPx, text), a.bold);
}

float RadiationPatternComponent::textBoxLeading (const Annotation& a,
                                                 const juce::Font& f) noexcept
{
    // GlyphArrangement advances by (font height + leading), while TextEditor
    // multiplies its line height. Converting here keeps the in-place editor
    // and the painted result on the same baselines.
    return f.getHeight() * (juce::jlimit (1.0f, 3.0f, a.lineHeight) - 1.0f);
}

float RadiationPatternComponent::textBoxFontScreenPx (const Annotation& a,
                                                      float boxWidthPx,
                                                      float boxHeightPx,
                                                      const juce::String& text) const
{
    if (a.fontPx > 0.5f)
    {
        // A set size is a property of the drawing, not of the screen, so it
        // rides the zoom exactly as the box does. Clamped only to keep a
        // deep zoom from asking for a font thousands of pixels tall.
        const float z = (a.space == AnnotSpace::World) ? zoom_ : 1.0f;
        return juce::jlimit (5.0f, 400.0f, a.fontPx * z);
    }
    return textBoxFitPx (a, boxWidthPx, boxHeightPx, text);
}

float RadiationPatternComponent::textBoxFitPx (const Annotation& a,
                                               float boxWidthPx,
                                               float boxHeightPx,
                                               const juce::String& text) const
{
    constexpr float kMinFitPx = 6.0f;
    const float availW = boxWidthPx  - 2.0f * kTextBoxPad;
    const float availH = boxHeightPx - 2.0f * kTextBoxPad;
    if (availW < 1.0f || availH < 1.0f) return kMinFitPx;

    const juce::String label = text.isNotEmpty() ? text : juce::String ("Text");

    // GlyphArrangement breaks a word that is wider than the line mid-word, so
    // the widest word has to fit on its own or a short label is cut in two
    // instead of shrinking. Width is linear in font height, so measure once.
    constexpr float kRefPx = 100.0f;
    const auto refFont = Brand::tech (kRefPx, a.bold);
    float widestWordRef = 0.0f;
    {
        juce::StringArray words;
        words.addTokens (label, " \t\r\n", {});
        for (const auto& w : words)
            if (w.isNotEmpty())
                widestWordRef = juce::jmax (widestWordRef, refFont.getStringWidthFloat (w));
    }

    auto fits = [&] (float px)
    {
        const auto f = Brand::tech (px, a.bold);
        // The synthesised italic leans the top of each glyph to the right.
        const float lean = a.italic ? kItalicShear * f.getAscent() : 0.0f;
        if (widestWordRef * (px / kRefPx) + lean > availW) return false;

        juce::GlyphArrangement ga;
        ga.addJustifiedText (f, label, 0.0f, f.getAscent(), availW,
                             juce::Justification::topLeft, textBoxLeading (a, f));
        const auto bb = ga.getBoundingBox (0, -1, true);
        return bb.getBottom() <= availH && bb.getRight() + lean <= availW;
    };

    float lo = kMinFitPx;
    float hi = juce::jmin (availH, 2000.0f);
    if (hi <= lo || ! fits (lo)) return lo;
    if (fits (hi)) return hi;
    for (int i = 0; i < 14; ++i)
    {
        const float mid = 0.5f * (lo + hi);
        if (fits (mid)) lo = mid; else hi = mid;
    }
    return lo;
}

juce::Justification RadiationPatternComponent::textBoxJustification (int align, int valign)
{
    const int h = (align == 1) ? juce::Justification::horizontallyCentred
                : (align == 2) ? juce::Justification::right
                               : juce::Justification::left;
    // The painter shifts the whole block for vertical alignment; the editor
    // can be told directly, and must be, or the words jump up to the top the
    // moment you click into a middle- or bottom-aligned box.
    const int v = (valign == 1) ? juce::Justification::verticallyCentred
                : (valign == 2) ? juce::Justification::bottom
                                : juce::Justification::top;
    return juce::Justification (h | v);
}

void RadiationPatternComponent::drawTextBoxAnnotation (juce::Graphics& g,
                                                       const Annotation& a,
                                                       float alphaMul,
                                                       bool showBorder)
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

    const auto base = juce::Colour::fromFloatRGBA (a.colour.getFloatRed(),
                                                   a.colour.getFloatGreen(),
                                                   a.colour.getFloatBlue(),
                                                   1.0f);

    // Border only while drawing (preview) or when selected (selection overlay).
    if (showBorder)
    {
        juce::Path path;
        auto s0 = annotateToScreen (a, corners[0]);
        path.startNewSubPath (s0);
        for (int i = 1; i < 4; ++i)
            path.lineTo (annotateToScreen (a, corners[i]));
        path.closeSubPath();
        g.setColour (base.withAlpha (juce::jlimit (0.45f, 1.0f, 0.85f * alphaMul)));
        g.strokePath (path, juce::PathStrokeType (juce::jmax (1.0f, a.thicknessPx)));
    }

    // While editing in-place, the TextEditor shows the text (Word/PPT style).
    if (isEditingTextBox()
        && textEditIndex_ >= 0
        && textEditIndex_ < (int) annotations_.size()
        && &annotations_[(size_t) textEditIndex_] == &a)
        return;

    const auto label = a.text.isNotEmpty() ? a.text : juce::String ("Text");
    auto sc = annotateToScreen (a, c);
    const float screenDeg = -a.rotationDeg;

    auto sA = annotateToScreen (a, corners[0]);
    auto sB = annotateToScreen (a, corners[1]);
    auto sD = annotateToScreen (a, corners[3]);
    const float wPx = sA.getDistanceFrom (sB);
    const float hPx = sA.getDistanceFrom (sD);
    if (wPx < 8.0f || hPx < 8.0f) return;

    const float pad = kTextBoxPad;
    g.setFont (textBoxFont (a, wPx, hPx, label));

    // Fully opaque: opacity no longer applies to a text box (see
    // isFilledShapeKind), so the only thing that can fade the words is a
    // preview's own alphaMul.
    const auto ink = base.withAlpha (juce::jlimit (0.0f, 1.0f, alphaMul));

    {
        juce::Graphics::ScopedSaveState textSs (g);
        g.addTransform (juce::AffineTransform::rotation (
            screenDeg * (float) M_PI / 180.0f, sc.x, sc.y));
        auto box = juce::Rectangle<float> (sc.x - wPx * 0.5f + pad,
                                           sc.y - hPx * 0.5f + pad,
                                           juce::jmax (1.0f, wPx - pad * 2.0f),
                                           juce::jmax (1.0f, hPx - pad * 2.0f));
        g.setColour (ink);

        // drawFittedText has no line-spacing control, so the text is laid out
        // by hand: GlyphArrangement takes the leading directly, and it is the
        // same call growTextBoxToFit measures with, so what fits is what shows.
        const auto font = textBoxFont (a, wPx, hPx, label);
        const auto hJust = (a.align == 1) ? juce::Justification::horizontallyCentred
                         : (a.align == 2) ? juce::Justification::right
                                          : juce::Justification::left;
        juce::GlyphArrangement ga;
        ga.addJustifiedText (font, label,
                             box.getX(), box.getY() + font.getAscent(),
                             box.getWidth(), juce::Justification (hJust),
                             textBoxLeading (a, font));

        // Vertical alignment, measured in LINE BOXES rather than in ink.
        // Centring the ink put a line with no descender in a different place
        // from one with, and sat a few pixels off the in-place editor, which
        // lays out the way every text engine does: by the lines, not by which
        // letters happen to be in them.
        if (a.valign != 0 && ga.getNumGlyphs() > 0)
        {
            int numLines = 0;
            float prevBaseline = -1.0e9f;
            for (int gi = 0; gi < ga.getNumGlyphs(); ++gi)
            {
                const float by = ga.getGlyph (gi).getBaselineY();
                if (std::abs (by - prevBaseline) > 0.5f) { ++numLines; prevBaseline = by; }
            }
            const float pitch = font.getHeight() * juce::jlimit (1.0f, 3.0f, a.lineHeight);
            // Last line carries no trailing leading - same block height the
            // editor measures, so the text does not step when you click in.
            const float blockH = (float) (numLines - 1) * pitch + font.getHeight();
            const float slack = box.getHeight() - blockH;
            const float wantTop = box.getY() + (a.valign == 1 ? slack * 0.5f : slack);
            const float haveTop = ga.getGlyph (0).getBaselineY() - font.getAscent();
            ga.moveRangeOfGlyphs (0, -1, 0.0f, wantTop - haveTop);
        }

        // Keep long text inside the box the user drew, as the old maxLines did.
        g.reduceClipRegion (box.expanded (pad).getSmallestIntegerContainer());

        if (a.italic)
        {
            // Slant each glyph about its OWN baseline, so every line leans the
            // same way. One shear over the whole block would rake multi-line
            // text into a parallelogram.
            for (int i = 0; i < ga.getNumGlyphs(); ++i)
            {
                const auto& pg = ga.getGlyph (i);
                const float by = pg.getBaselineY();
                pg.draw (g, juce::AffineTransform::translation (0.0f, -by)
                                .sheared (-kItalicShear, 0.0f)
                                .translated (0.0f, by));
            }
        }
        else
        {
            ga.draw (g);
        }
    }
}

juce::String RadiationPatternComponent::formatLengthLabel (float metres)
{
    // Ruler / shape dims - always show the active unit system.
    if (Units::imperial())
    {
        const double ft = (double) metres * 3.280839895;
        const double a  = std::abs (ft);
        if (a + 1.0e-9 < 1.0)
            return juce::String (metres * 39.37007874f, 1) + " in";
        if (a < 10.0)
            return juce::String (ft, 2) + " ft";
        return juce::String (ft, 1) + " ft";
    }
    return Units::formatLengthSmart ((double) metres);
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

    const float fx = (float) (((wx - (float) result_.worldX0) / (float) result_.worldW) * (double) (W - 1));
    const float fy = (float) (((wy - (float) result_.worldY0) / (float) result_.worldH) * (double) (H - 1));
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

    const juce::String line2 = "(" + juce::String (Units::metresToDisplay (splProbeWorld_.x), 1) + ", "
                             + juce::String (Units::metresToDisplay (splProbeWorld_.y), 1) + ") "
                             + Units::lengthUnit()
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
    // Use screen pixels - world minDist (~5 cm) is sub-pixel at Fit View, so a plain
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
    return pointHitsShapeBorder (pt, a, radius) || pointHitsShapeFill (pt, a, radius);
}

bool RadiationPatternComponent::pointHitsShapeBorder (juce::Point<float> pt,
                                                      const Annotation& a,
                                                      float radius) noexcept
{
    if (a.pts.size() < 2) return false;

    if (a.kind == Annotation::Kind::Circle)
    {
        const auto& c = a.pts[0];
        const float r = c.getDistanceFrom (a.pts[1]);
        return std::abs (pt.getDistanceFrom (c) - r) <= radius;
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

    if (a.kind != Annotation::Kind::Rectangle && a.kind != Annotation::Kind::Square)
        return false;

    const auto r = normalisedShapeRect (a.pts[0], a.pts[1], a.kind);
    if (r.getWidth() < 1.0e-6f || r.getHeight() < 1.0e-6f)
        return pt.getDistanceFrom (r.getCentre()) <= radius;

    // Test in the shape's OWN frame: un-rotate the point rather than rotating
    // four edges, so a turned rectangle is as clickable as a straight one.
    if (std::abs (a.rotationDeg) > 0.01f)
        pt = rotateAround (pt, r.getCentre(), -a.rotationDeg);

    const auto tl = r.getTopLeft();
    const auto tr = r.getTopRight();
    const auto bl = r.getBottomLeft();
    const auto br = r.getBottomRight();
    return distPointToSegment (pt, tl, tr) <= radius
        || distPointToSegment (pt, tr, br) <= radius
        || distPointToSegment (pt, br, bl) <= radius
        || distPointToSegment (pt, bl, tl) <= radius;
}

bool RadiationPatternComponent::pointHitsShapeFill (juce::Point<float> pt,
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

    if (a.kind != Annotation::Kind::Rectangle && a.kind != Annotation::Kind::Square)
        return false;

    const auto r = normalisedShapeRect (a.pts[0], a.pts[1], a.kind);
    if (r.getWidth() < 1.0e-6f || r.getHeight() < 1.0e-6f)
        return pt.getDistanceFrom (r.getCentre()) <= radius;

    if (std::abs (a.rotationDeg) > 0.01f)
        pt = rotateAround (pt, r.getCentre(), -a.rotationDeg);

    return r.expanded (radius).contains (pt);
}

RadiationPatternComponent::PlaneStats
RadiationPatternComponent::planeStatsFor (const Annotation& a) const
{
    PlaneStats st;
    if (! hasData_ || a.pts.size() < 2 || a.space != AnnotSpace::World)
        return st;

    double sumI = 0.0;        // energy, not dB - see the average below
    float  mn = 1.0e9f, mx = -1.0e9f;
    int    n = 0;
    bool   anyAbs = false;

    auto take = [&] (float wx, float wy)
    {
        float absDb = 0.0f, relDb = 0.0f;
        if (! sampleSplAtWorld (wx, wy, absDb, relDb)) return;
        const float v = result_.hasAbsoluteSpl ? absDb : relDb;
        anyAbs = anyAbs || result_.hasAbsoluteSpl;
        mn = juce::jmin (mn, v);
        mx = juce::jmax (mx, v);
        // Spatial average is an average of ENERGY, then back to dB. Averaging
        // the decibels themselves would quietly understate hot spots.
        sumI += std::pow (10.0, (double) v / 10.0);
        ++n;
    };

    const int kSide = 32;     // 1024 samples over an area is plenty and instant
    const int kLine = 200;

    if (a.kind == Annotation::Kind::Circle)
    {
        const auto c = a.pts[0];
        const float r = c.getDistanceFrom (a.pts[1]);
        if (r < 1.0e-6f) return st;
        for (int iy = 0; iy <= kSide; ++iy)
            for (int ix = 0; ix <= kSide; ++ix)
            {
                const float dx = (ix / (float) kSide * 2.0f - 1.0f) * r;
                const float dy = (iy / (float) kSide * 2.0f - 1.0f) * r;
                if (dx * dx + dy * dy <= r * r) take (c.x + dx, c.y + dy);
            }
    }
    else if (a.kind == Annotation::Kind::Rectangle || a.kind == Annotation::Kind::Square)
    {
        const auto wr = normalisedShapeRect (a.pts[0], a.pts[1], a.kind);
        for (int iy = 0; iy <= kSide; ++iy)
            for (int ix = 0; ix <= kSide; ++ix)
                take (wr.getX() + wr.getWidth()  * (ix / (float) kSide),
                      wr.getY() + wr.getHeight() * (iy / (float) kSide));
    }
    else if (a.kind == Annotation::Kind::Polyline && a.closed && a.pts.size() >= 3)
    {
        // Closed polyline is a real audience area, so it is sampled as one:
        // bounding box, keeping whatever is inside.
        float x0 = a.pts[0].x, x1 = x0, y0 = a.pts[0].y, y1 = y0;
        for (const auto& pt : a.pts)
        {
            x0 = juce::jmin (x0, pt.x); x1 = juce::jmax (x1, pt.x);
            y0 = juce::jmin (y0, pt.y); y1 = juce::jmax (y1, pt.y);
        }
        auto inside = [&] (float px, float py)
        {
            bool in = false;
            for (size_t i = 0, j = a.pts.size() - 1; i < a.pts.size(); j = i++)
            {
                const auto& pi = a.pts[i];
                const auto& pj = a.pts[j];
                if (((pi.y > py) != (pj.y > py))
                    && (px < (pj.x - pi.x) * (py - pi.y) / (pj.y - pi.y) + pi.x))
                    in = ! in;
            }
            return in;
        };
        for (int iy = 0; iy <= kSide; ++iy)
            for (int ix = 0; ix <= kSide; ++ix)
            {
                const float px = x0 + (x1 - x0) * (ix / (float) kSide);
                const float py = y0 + (y1 - y0) * (iy / (float) kSide);
                if (inside (px, py)) take (px, py);
            }
    }
    else
    {
        // Line or open polyline: walk it, which is what a line of seats is.
        for (size_t i = 1; i < a.pts.size(); ++i)
        {
            const auto p0 = a.pts[i - 1];
            const auto p1 = a.pts[i];
            for (int k = 0; k <= kLine; ++k)
            {
                const float t = k / (float) kLine;
                take (p0.x + (p1.x - p0.x) * t, p0.y + (p1.y - p0.y) * t);
            }
        }
    }

    if (n == 0) return st;
    st.valid    = true;
    st.absolute = anyAbs;
    st.minDb    = mn;
    st.maxDb    = mx;
    st.avgDb    = (float) (10.0 * std::log10 (sumI / (double) n));
    st.samples  = n;
    return st;
}

int RadiationPatternComponent::planeDashPattern (const Annotation& a, float strokeW,
                                                 float (&dl)[2]) noexcept
{
    if (! a.isPlane) return 0;

    // Three types, three strokes. Listening and Architectural were both plain
    // solid lines, so on an open plane - where there is no area to fill - they
    // were impossible to tell apart.
    const float w = juce::jmax (1.0f, strokeW);
    switch (a.planeType)
    {
        case Annotation::PlaneType::Listening:
            // Dots: a row of seats, not a barrier.
            dl[0] = w * 0.9f; dl[1] = w * 2.6f; return 2;

        case Annotation::PlaneType::Virtual:
            // Long dashes: a construction line, the way a drawing marks one.
            dl[0] = w * 5.0f; dl[1] = w * 3.4f; return 2;

        case Annotation::PlaneType::Architectural:
        default:
            return 0;                         // solid, and already the heaviest
    }
}

bool RadiationPatternComponent::planeIsDashed (const Annotation& a) noexcept
{
    // Virtual is a reference surface nobody occupies, so it reads as a
    // construction line rather than as something solid.
    return a.isPlane && a.planeType == Annotation::PlaneType::Virtual;
}

bool RadiationPatternComponent::planeBlocksRays (const Annotation& a) noexcept
{
    // Listening is where the sound is meant to land and Architectural is built
    // out of something, so a ray has arrived once it reaches either. Virtual
    // is a construction line - it marks a position without occupying it, so
    // the ray carries straight on through.
    return a.isPlane && a.planeType != Annotation::PlaneType::Virtual;
}

juce::Point<float> RadiationPatternComponent::clipRayAtPlanes (juce::Point<float> from,
                                                               juce::Point<float> to) const
{
    const auto dir = to - from;
    const float len2 = dir.x * dir.x + dir.y * dir.y;
    if (len2 < 1.0e-9f) return to;

    float nearest = 1.0f;        // fraction along from -> to

    // One segment of a plane's outline, in screen pixels. Standard 2D segment
    // crossing: solve from + t*dir == p + u*(q - p) and keep the first hit
    // that lies on both.
    auto cross = [&] (juce::Point<float> p, juce::Point<float> q)
    {
        const auto e = q - p;
        const float den = dir.x * e.y - dir.y * e.x;
        if (std::abs (den) < 1.0e-9f) return;        // parallel
        const auto w = p - from;
        const float t = (w.x * e.y - w.y * e.x) / den;
        const float u = (w.x * dir.y - w.y * dir.x) / den;
        if (t > 1.0e-4f && t < nearest && u >= 0.0f && u <= 1.0f)
            nearest = t;
    };

    for (const auto& a : annotations_)
    {
        if (! planeBlocksRays (a)) continue;
        if (a.space != AnnotSpace::World) continue;   // rays live on the field
        if (a.pts.size() < 2) continue;

        switch (a.kind)
        {
            case Annotation::Kind::Rectangle:
            case Annotation::Kind::Square:
            {
                const auto r = normalisedShapeRect (a.pts[0], a.pts[1], a.kind);
                const auto c = r.getCentre();
                juce::Point<float> sc[4];
                for (int i = 0; i < 4; ++i)
                {
                    const juce::Point<float> corner {
                        (i == 0 || i == 3) ? r.getX() : r.getRight(),
                        (i < 2)            ? r.getY() : r.getBottom()
                    };
                    sc[i] = annotateToScreen (a, rotateAround (corner, c, a.rotationDeg));
                }
                for (int i = 0; i < 4; ++i)
                    cross (sc[i], sc[(i + 1) % 4]);
                break;
            }

            case Annotation::Kind::Circle:
            {
                // Tessellated rather than solved: the two world axes can be
                // scaled differently, so on screen this is an ellipse.
                const auto& cw = a.pts[0];
                const float rad = cw.getDistanceFrom (a.pts[1]);
                if (rad < 1.0e-6f) break;
                constexpr int kSteps = 72;
                juce::Point<float> prev;
                for (int i = 0; i <= kSteps; ++i)
                {
                    const float ang = (float) i / kSteps * 2.0f * (float) M_PI;
                    const auto sp = annotateToScreen (a, { cw.x + rad * std::cos (ang),
                                                           cw.y + rad * std::sin (ang) });
                    if (i > 0) cross (prev, sp);
                    prev = sp;
                }
                break;
            }

            default:   // Line, Polyline - the points are the surface
            {
                auto prev = annotateToScreen (a, a.pts[0]);
                for (size_t j = 1; j < a.pts.size(); ++j)
                {
                    const auto cur = annotateToScreen (a, a.pts[j]);
                    cross (prev, cur);
                    prev = cur;
                }
                break;
            }
        }
    }

    return from + dir * nearest;
}

float RadiationPatternComponent::planeStrokeW (const Annotation& a) noexcept
{
    // Architectural is the building: heavier than anything drawn on it.
    return (a.isPlane && a.planeType == Annotation::PlaneType::Architectural)
             ? a.thicknessPx * 1.8f : a.thicknessPx;
}

float RadiationPatternComponent::planeFillAlpha (const Annotation& a, float base) noexcept
{
    if (! a.isPlane) return base;
    // Listening is the only one people stand on, so it is the only one that
    // reads as an AREA. The other two are outlines.
    return (a.planeType == Annotation::PlaneType::Listening)
             ? juce::jmax (base, 0.22f) : 0.0f;
}

bool RadiationPatternComponent::planeTagStrings (const Annotation& a,
                                                 juce::String& name,
                                                 juce::String& stats) const
{
    if (! a.isPlane) return false;

    // Without this a Listening plane and an ordinary filled rectangle look
    // identical. The tag is what says "this is a surface, not a note".
    name = a.planeType == Annotation::PlaneType::Listening ? "LISTENING"
         : a.planeType == Annotation::PlaneType::Virtual   ? "VIRTUAL"
                                                           : "ARCHITECTURAL";
    // The ear height belongs on the drawing, not buried in a panel: it is the
    // one thing that says WHICH listening plane this is.
    if (a.planeType == Annotation::PlaneType::Listening)
        name << "  -  "
             << (a.listenHgt == Annotation::ListenHeight::Standing ? "STANDING" : "SEATED")
             << " " << Units::metres ((double) Annotation::listenHeightM (a.listenHgt), 1);

    // Coverage, for the two types that are about measuring something. A wall
    // is not a measurement, so Architectural carries no numbers.
    stats.clear();
    if (a.planeType != Annotation::PlaneType::Architectural)
    {
        const auto st = planeStatsFor (a);
        if (! st.valid)
        {
            stats = "no solve yet";
        }
        else
        {
            // The frequency is stated because the map is ONE frequency - these
            // are not broadband numbers and must not be read as such.
            const double f = result_.frequency;
            stats = "avg " + juce::String (st.avgDb, 1)
                  + (st.absolute ? " dB" : " dB rel")
                  + "   min " + juce::String (st.minDb, 1)
                  + "   max " + juce::String (st.maxDb, 1)
                  + "   spread " + juce::String (st.spreadDb(), 1)
                  + "   @ " + (f >= 100.0 ? juce::String ((int) (f + 0.5))
                                          : juce::String (f, 1)) + " Hz";
        }
    }
    return true;
}

juce::Rectangle<int> RadiationPatternComponent::planeTagBounds (const Annotation& a) const
{
    juce::String name, stats;
    if (! planeTagStrings (a, name, stats)) return {};

    const auto box = annotationScreenBounds (a);
    const float h = juce::jmax (11.0f, 11.5f * Brand::UI::scale);
    const auto font = Brand::tech (h, true);
    int w = font.getStringWidth (name) + (int) h;
    if (stats.isNotEmpty())
        w = juce::jmax (w, font.getStringWidth (stats) + (int) h);
    w = juce::jmax (40, w);

    // Above the shape by preference, but tucked inside it when that would put
    // the tag off the top of the plot - a plane drawn near the far edge of the
    // world would otherwise be the one plane you cannot identify.
    const auto plot = plotArea();
    const int pillH = (int) (h * (stats.isNotEmpty() ? 2.75f : 1.45f));
    int pillY = (int) box.getY() - (int) (h * 1.7f);
    if (pillY < plot.getY() + 2)
        pillY = (int) box.getY() + 2;      // tuck inside the shape instead
    // ...and the shape itself can be clipped by the plot edge, so the final
    // say belongs to the plot, not the shape.
    pillY = juce::jlimit (plot.getY() + 2, plot.getBottom() - pillH - 2, pillY);
    return { juce::jmax ((int) box.getX(), plot.getX() + 2), pillY, w, pillH };
}

int RadiationPatternComponent::planeAtScreen (juce::Point<float> p) const
{
    // Tags first and topmost-first: the label is what you are pointing at when
    // it is sitting on top of a shape.
    for (int i = (int) annotations_.size(); --i >= 0;)
    {
        const auto& a = annotations_[(size_t) i];
        if (! a.isPlane || ! a.showTag) continue;
        if (planeTagBounds (a).toFloat().contains (p)) return i;
    }

    // Then the plane itself, so a hidden tag can still be switched back on by
    // right-clicking the shape it belongs to.
    const auto annot = screenToAnnot (p.x, p.y);
    const float radius = (currentAnnotSpace() == AnnotSpace::PolarPlot)
        ? (10.0f / juce::jmax (1.0f, polarRadius_))
        : (10.0f / juce::jmax (1.0f, worldScale()));
    for (int idx : annotationsUnder (annot, radius))
        if (annotations_[(size_t) idx].isPlane) return idx;

    return -1;
}

void RadiationPatternComponent::drawPlaneTag (juce::Graphics& g, const Annotation& a)
{
    if (! a.isPlane || ! a.showTag) return;

    juce::String name, stats;
    if (! planeTagStrings (a, name, stats)) return;

    const float h = juce::jmax (11.0f, 11.5f * Brand::UI::scale);
    const auto pill = planeTagBounds (a);
    g.setFont (Brand::tech (h, true));
    g.setColour (a.colour.withAlpha (0.92f));
    g.fillRoundedRectangle (pill.toFloat(), 3.0f);
    g.setColour (Brand::white());
    if (stats.isEmpty())
    {
        g.drawText (name, pill, juce::Justification::centred);
    }
    else
    {
        auto top = pill.withHeight (pill.getHeight() / 2);
        g.drawText (name, top, juce::Justification::centred);
        g.setFont (Brand::tech (h * 0.95f, false));
        g.drawText (stats, pill.withTrimmedTop (pill.getHeight() / 2),
                    juce::Justification::centred);
    }
}

void RadiationPatternComponent::drawShapeAnnotation (juce::Graphics& g,
                                                     const Annotation& a,
                                                     float alphaMul)
{
    if (a.pts.size() < 2) return;

    // Isolate from any leftover Graphics::setOpacity (e.g. heatmap / layout).
    juce::Graphics::ScopedSaveState ss (g);
    g.setOpacity (1.0f);

    const float fillA = planeFillAlpha (a, juce::jlimit (0.0f, 1.0f, a.fillAlpha)) * alphaMul;
    const float strokeA = a.isPlane
        ? alphaMul
        : juce::jlimit (0.15f, 1.0f, 0.55f + 0.45f * a.fillAlpha) * alphaMul;
    const float strokeW = planeStrokeW (a);
    // Rebuild from RGB so we never inherit a stale alpha channel on colour.
    const auto base = juce::Colour::fromFloatRGBA (a.colour.getFloatRed(),
                                                   a.colour.getFloatGreen(),
                                                   a.colour.getFloatBlue(),
                                                   1.0f);

    // One dashed stroker for both shapes below, so Virtual looks the same
    // whether it is a rectangle or a circle.
    auto strokeShape = [&] (const juce::Path& path)
    {
        g.setColour (base.withAlpha (strokeA));
        float dl[2];
        const int n = planeDashPattern (a, strokeW, dl);
        // Round caps turn the short Listening segments into dots rather than
        // stubby dashes.
        const juce::PathStrokeType stroke (strokeW, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded);
        if (n == 0) { g.strokePath (path, stroke); return; }
        juce::Path dashed;
        stroke.createDashedStroke (dashed, path, dl, n);
        g.fillPath (dashed);
    };

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
        juce::Path ring;
        ring.addEllipse (sr);
        strokeShape (ring);
        return;
    }

    const auto wr = normalisedShapeRect (a.pts[0], a.pts[1], a.kind);
    if (wr.getWidth() < 1.0e-6f && wr.getHeight() < 1.0e-6f) return;

    // Turn the corners in annotation space, then map each one, so the shape
    // still lands correctly when the two world axes are scaled differently.
    const auto c = wr.getCentre();
    juce::Path outline;
    for (int i = 0; i < 4; ++i)
    {
        const juce::Point<float> corner {
            (i == 0 || i == 3) ? wr.getX() : wr.getRight(),
            (i < 2)            ? wr.getY() : wr.getBottom()
        };
        auto sp = annotateToScreen (a, rotateAround (corner, c, a.rotationDeg));
        if (i == 0) outline.startNewSubPath (sp);
        else        outline.lineTo (sp);
    }
    outline.closeSubPath();

    g.setColour (base.withAlpha (fillA));
    g.fillPath (outline);
    strokeShape (outline);
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
    if (addMicArmed_ || addSpeakerArmed_)
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
    // Heatmap only when a RUN produced a field with active units.
    hasData_ = (result_.width > 0 && result_.activeSpeakers > 0);
    // Empty / pre-RUN scenes still get a world grid (no SPL field).
    ensureWorldExtents();
    // First load, or a view the user has not touched: fill the plot pane.
    // This used to key off the ZOOM being near 1, which is exactly where the
    // default fit leaves it - so panning without zooming was undone by the
    // next solve, and every edit triggers a solve.
    if (mayAutoFitView())
        fitView();
    buildImage();
    refreshMicLevels();
    if (onMicsChanged) onMicsChanged();
    repaint();
}

void RadiationPatternComponent::ensureWorldExtents() noexcept
{
    if (result_.worldW <= 0.0)
        result_.worldW = params_.worldW > 0.0 ? params_.worldW : 100.0;
    if (result_.worldH <= 0.0)
        result_.worldH = params_.worldH > 0.0 ? params_.worldH : 100.0;
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

bool RadiationPatternComponent::cutSelection()
{
    if (! copySelection())
        return false;
    return deleteSelection();
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
        // Control panel notify refreshes speakers_ via syncRenderer - re-read selection.
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
    showSelectionContextMenu (screenPos, -1);
}

int RadiationPatternComponent::micAtScreen (juce::Point<float> p) const noexcept
{
    // Matches the drawn glyph, which is about 10 px across with a stand below.
    for (int i = (int) mics_.size(); --i >= 0;)
        if (worldToScreen (mics_[(size_t) i].x, mics_[(size_t) i].y)
                .getDistanceFrom (p) <= 12.0f)
            return i;
    return -1;
}

void RadiationPatternComponent::showSelectionContextMenu (juce::Point<int> screenPos,
                                                          int speakerUnderCursor)
{
    juce::PopupMenu m;
    const bool canEdit = hasCopyableSelection();
    const bool hasSpeakerProps = speakerUnderCursor >= 0
                              && speakerUnderCursor < (int) speakers_.size();

    const int micIdx = micAtScreen (getMouseXYRelative().toFloat());
    if (micIdx >= 0)
    {
        m.addItem (6, "Listen here");
        m.addSeparator();
    }

    // A ring is a thin line that can land anywhere, so deleting it from where
    // you can see it beats hunting for it in a list.
    const float ringHit = rangeRingAtScreen (getMouseXYRelative().toFloat());
    if (ringHit > 0.0f)
    {
        m.addItem (8, "Delete range " + rangeRingLabel (ringHit));
        m.addSeparator();
    }

    // The tag sits on top of the drawing, so it has to be possible to put it
    // away - and to get it back, which is why the item is also offered when
    // the pointer is over a plane whose tag is already hidden.
    const int planeIdx = planeAtScreen (getMouseXYRelative().toFloat());
    if (planeIdx >= 0)
    {
        m.addItem (7, annotations_[(size_t) planeIdx].showTag
                        ? "Hide plane label" : "Show plane label");
        m.addSeparator();
    }

    // A sub's stalk is its only aiming handle, so putting it away has to be
    // reversible from the same place - which is why the item is offered on a
    // sub whose line is already hidden too, reading "Show" instead.
    const bool subUnderCursor = hasSpeakerProps
        && isSubwooferModel (speakers_[(size_t) speakerUnderCursor].model);
    if (subUnderCursor)
    {
        m.addItem (10, speakers_[(size_t) speakerUnderCursor].showAimLine
                         ? "Hide aim line" : "Show aim line");
        m.addSeparator();
    }

    if (hasSpeakerProps)
    {
        m.addItem (4, "Properties");
        m.addSeparator();
    }
    m.addItem (1, "Copy",   canEdit);
    m.addItem (5, "Cut",    canEdit);
    m.addItem (2, "Paste",  hasClipboardContent()); // off when clipboard empty
    m.addSeparator();
    m.addItem (3, "Delete", canEdit);
    // Its own group, well away from Copy: that one copies the SELECTION,
    // this one copies the picture, and the two sitting side by side would
    // be a coin toss every time.
    m.addSeparator();
    m.addItem (9, "Copy image to clipboard");
    m.showMenuAsync (juce::PopupMenu::Options()
                         .withTargetScreenArea ({ screenPos.x, screenPos.y, 1, 1 }),
                     [safe = juce::Component::SafePointer<RadiationPatternComponent> (this),
                      speakerUnderCursor, micIdx, planeIdx, ringHit] (int result)
                     {
                         if (safe == nullptr || result <= 0) return;
                         if (result == 6)
                         {
                             if (safe->onListenAtMic) safe->onListenAtMic (micIdx);
                             return;
                         }
                         if (result == 8)
                         {
                             if (safe->onWillEdit) safe->onWillEdit();
                             safe->removeRangeRing (ringHit);
                             if (safe->onEditCommitted) safe->onEditCommitted();
                             return;
                         }
                         if (result == 7)
                         {
                             if (planeIdx >= 0 && planeIdx < (int) safe->annotations_.size())
                             {
                                 if (safe->onWillEdit) safe->onWillEdit();
                                 auto& pa = safe->annotations_[(size_t) planeIdx];
                                 pa.showTag = ! pa.showTag;
                                 if (safe->onEditCommitted) safe->onEditCommitted();
                                 safe->repaint();
                             }
                             return;
                         }
                         if (result == 9)
                         {
                             if (safe->onCopyImage) safe->onCopyImage();
                             return;
                         }
                         if (result == 10)
                         {
                             if (speakerUnderCursor >= 0
                                 && speakerUnderCursor < (int) safe->speakers_.size())
                             {
                                 if (safe->onWillEdit) safe->onWillEdit();
                                 auto& sp = safe->speakers_[(size_t) speakerUnderCursor];
                                 sp.showAimLine = ! sp.showAimLine;
                                 if (safe->onSpeakerAimLineChanged)
                                     safe->onSpeakerAimLineChanged (speakerUnderCursor,
                                                                    sp.showAimLine);
                                 if (safe->onEditCommitted) safe->onEditCommitted();
                                 safe->repaint();
                             }
                             return;
                         }
                         if (result == 1)
                             safe->copySelection();
                         else if (result == 5)
                             safe->cutSelection();
                         else if (result == 2)
                             safe->pasteClipboard();
                         else if (result == 3)
                             safe->deleteSelection();
                         else if (result == 4)
                             safe->showSpeakerProperties (speakerUnderCursor);
                     });
}

void RadiationPatternComponent::showSpeakerProperties (int speakerIndex)
{
    if (speakerIndex < 0 || speakerIndex >= (int) speakers_.size())
        return;

    auto* body = new SpeakerPropertiesDialog (speakers_[(size_t) speakerIndex],
                                             speakerModelOrdinal (speakers_, speakerIndex));

    juce::DialogWindow::LaunchOptions opts;
    opts.content.setOwned (body);
    opts.dialogTitle = juce::String (speakerModelTag (speakers_[(size_t) speakerIndex].model))
                      + "-" + juce::String (speakerModelOrdinal (speakers_, speakerIndex))
                      + " Properties";
    opts.dialogBackgroundColour = Brand::panel();
    opts.escapeKeyTriggersCloseButton = true;
    opts.useNativeTitleBar = true;
    opts.resizable = false;
    opts.launchAsync();
}

bool RadiationPatternComponent::deleteSelection()
{
    if (! hasCopyableSelection())
        return false;
    if (sessionActive_ || pendingAnchor_)
        return false;
    if (isEditingTextBox())
        endTextBoxEdit (true);

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
    // Relative to the solved region's origin, not absolute (0, 0): the region
    // moves when you pan, so a world point's screen position depends on where
    // that region currently starts.
    const auto pb = plotArea();
    const float rx = wx - (float) result_.worldX0;
    const float ry = (float) (result_.worldY0 + result_.worldH) - wy;
    return { (float) pb.getX() + origin_.x + rx * worldScaleX(),
             (float) pb.getY() + origin_.y + ry * worldScaleY() };
}

juce::Point<float> RadiationPatternComponent::screenToWorld (float sx, float sy) const
{
    const auto pb = plotArea();
    const float sxScale = worldScaleX();
    const float syScale = worldScaleY();
    return { (float) result_.worldX0 + (sx - (float) pb.getX() - origin_.x) / sxScale,
             (float) (result_.worldY0 + result_.worldH)
                 - (sy - (float) pb.getY() - origin_.y) / syScale };
}

void RadiationPatternComponent::fitView()
{
    const auto pb = plotArea();
    const double ww = (result_.worldW > 0 ? result_.worldW : params_.worldW);
    const double wh = (result_.worldH > 0 ? result_.worldH : params_.worldH);
    if (pb.getWidth() <= 0 || pb.getHeight() <= 0 || ww <= 0 || wh <= 0) return;

    // ONE px/m for both axes, so grid cells are square and the cabinets are
    // drawn to scale. The LARGER ratio, so the plot is filled edge to edge
    // with no margins; on a non-square canvas that crops the fixed field on
    // the shorter axis, and zooming out past this fit (see minZoomForFit)
    // brings the whole field back into view.
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
    // An explicit fit is the user asking for the default framing back.
    viewUserAdjusted_ = false;
    clampViewToField();
}

void RadiationPatternComponent::zoomAboutCentre (float factor)
{
    const auto pb = plotArea();
    const float newZoom = juce::jlimit (minZoomForFit(), kMaxZoom, zoom_ * factor);
    if (std::abs (newZoom - zoom_) < 1e-6f) return;

    const float cx = (float) pb.getCentreX();
    const float cy = (float) pb.getCentreY();
    const auto worldUnder = screenToWorld (cx, cy);
    zoom_ = newZoom;
    origin_.x = cx - (float) pb.getX() - worldUnder.x * worldScaleX();
    origin_.y = cy - (float) pb.getY()
                  - ((float) result_.worldH - worldUnder.y) * worldScaleY();
    noteViewAdjusted();
    clampViewToField();
    layoutTextBoxEditor();
    repaint();
}

void RadiationPatternComponent::zoomToSelection()
{
    const auto pb = plotArea();
    if (pb.getWidth() <= 0 || pb.getHeight() <= 0 || speakers_.empty()) return;
    if (params_.viewMode == ViewMode::Directivity) return;

    // Nothing selected reads as "show me everything", which is more useful
    // than doing nothing at all.
    std::vector<int> idx = selectedSpeakers_;
    if (idx.empty())
        for (int i = 0; i < (int) speakers_.size(); ++i)
            idx.push_back (i);

    juce::Rectangle<float> box;
    bool any = false;
    for (int i : idx)
    {
        if (i < 0 || i >= (int) speakers_.size()) continue;
        const auto fp = speakerFootprintWorld (speakers_[(size_t) i]);
        box = any ? box.getUnion (fp) : fp;
        any = true;
    }
    if (! any) return;

    // A single cabinet is barely a metre across; framed tight it would fill
    // the screen at useless magnification and show none of its surroundings.
    const float pad = juce::jmax (2.0f, 0.35f * juce::jmax (box.getWidth(), box.getHeight()));
    box = box.expanded (pad);

    // Contain, not cover: the whole selection has to be on screen.
    const float wantX = (float) pb.getWidth()  / juce::jmax (0.01f, box.getWidth());
    const float wantY = (float) pb.getHeight() / juce::jmax (0.01f, box.getHeight());
    const float want  = juce::jmin (wantX, wantY);
    if (baseScaleX_ > 1.0e-6f)
        zoom_ = juce::jlimit (minZoomForFit(), kMaxZoom, want / baseScaleX_);

    const auto c = box.getCentre();
    origin_.x = 0.5f * (float) pb.getWidth()
              - (c.x - (float) result_.worldX0) * worldScaleX();
    origin_.y = 0.5f * (float) pb.getHeight()
              - ((float) (result_.worldY0 + result_.worldH) - c.y) * worldScaleY();
    noteViewAdjusted();

    clampViewToField();
    layoutTextBoxEditor();
    repaint();
}

float RadiationPatternComponent::minZoomForFit() const
{
    // zoom 1.0 IS the floor: it is the fill-the-canvas fit, so the plot is
    // never letterboxed. An earlier cut let you zoom out past it to see the
    // whole square field at once, which meant pale margins down both sides --
    // the thing the full-bleed layout exists to avoid. The field stays
    // 100 x 100 m either way; you just pan to reach the cropped band instead
    // of shrinking everything to fit it on screen.
    return 1.0f;
}

void RadiationPatternComponent::clampViewToField()
{
    const auto pb = plotArea();
    const double ww = (result_.worldW > 0 ? result_.worldW : params_.worldW);
    const double wh = (result_.worldH > 0 ? result_.worldH : params_.worldH);
    if (pb.getWidth() <= 0 || pb.getHeight() <= 0 || ww <= 0 || wh <= 0) return;

    if (zoom_ < minZoomForFit())
        zoom_ = minZoomForFit();

    const float worldPxW = (float) ww * worldScaleX();
    const float worldPxH = (float) wh * worldScaleY();
    const float viewW = (float) pb.getWidth();
    const float viewH = (float) pb.getHeight();

    // Fitted to contain, the world is smaller than the viewport on one axis.
    // Centre it there rather than pinning it into a corner.
    if (worldPxW >= viewW)
        origin_.x = juce::jlimit (viewW - worldPxW, 0.0f, origin_.x);
    else
        origin_.x = 0.5f * (viewW - worldPxW);

    if (worldPxH >= viewH)
        origin_.y = juce::jlimit (viewH - worldPxH, 0.0f, origin_.y);
    else
        origin_.y = 0.5f * (viewH - worldPxH);
}

void RadiationPatternComponent::resetView()
{
    viewInit_ = false;
    fitView();
    repaint();
}

void RadiationPatternComponent::zoomIn()
{
    // World-plane zoom - any tool; works with or without a finished RUN.
    if (params_.viewMode == ViewMode::Directivity) return;
    if (params_.viewMode == ViewMode::MeasuredPolar && ! showingBemHeatmap()) return;

    zoomAboutCentre (1.2f);
}

void RadiationPatternComponent::zoomOut()
{
    if (params_.viewMode == ViewMode::Directivity) return;
    if (params_.viewMode == ViewMode::MeasuredPolar && ! showingBemHeatmap()) return;

    zoomAboutCentre (1.0f / 1.2f);
}

void RadiationPatternComponent::resized()
{
    const auto pb = plotArea();
    const double ww = (result_.worldW > 0 ? result_.worldW : params_.worldW);
    const double wh = (result_.worldH > 0 ? result_.worldH : params_.worldH);
    if (pb.getWidth() <= 0 || pb.getHeight() <= 0 || ww <= 0 || wh <= 0) return;

    if (mayAutoFitView())
    {
        fitView();
        layoutTextBoxEditor();
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
    layoutTextBoxEditor();
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

            // SPL heatmap - 7-color scale (black→blue→cyan→green→yellow→orange→red).
            {
                const float dB = (result_.splRelDB.size() == (size_t) W * (size_t) H)
                                    ? result_.splRelDB[idx]
                                    : result_.splDB[idx];
                if (params_.bandedSPL)
                {
                    // Contour bands at the fixed step. db Floor only clips the bottom.
                    c = ColourMaps::splBandForFloor (dB, (float) params_.dBfloor,
                                                     ColourMaps::kContourStepDB);
                }
                else
                {
                    // Continuous: fixed 0...-36 colour span (-6 dB always same hue).
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

    if (params_.viewMode == ViewMode::ElevationFront
        || params_.viewMode == ViewMode::ElevationSide)
    {
        // Same canvas as the plan with mapping off, so switching views does
        // not switch the drawing from a dark field to a white page - and the
        // white labels and grid keep the contrast they were designed for.
        g.fillAll (ColourMaps::sevenColor (0.0f));
        drawElevation (g, getLocalBounds());
        drawMarqueeOverlay (g);
        return;
    }

    ensureWorldExtents();
    if (! viewInit_)
        fitView();

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
        if (hasData_ && showField_)
            drawField (g, pb);   // stretched to fill pb exactly
        else if (! showField_)
            // Mapping Off still needs a canvas. Painting the colour map's own
            // floor - the shade a silent field would be - keeps every overlay
            // (white labels, rays, grid) at the contrast it was designed for;
            // the bare component background is white in the light theme, which
            // made them vanish.
            g.fillAll (ColourMaps::sevenColor (0.0f));
        drawLayout   (g, pb);
        drawGrid     (g, pb);
        drawSplValues (g, pb);     // over the map, under everything placed on it
        drawSpeakerRays (g, pb);   // under the markers, over the grid
        drawSpeakers (g, pb);
        drawAnnotations (g, pb);
        drawMics (g);
        drawSplProbe (g);
        drawMarqueeOverlay (g);
    }

    // No scale without a map to read it against.
    if (hasData_ && showField_)
        drawColourbar (g, getLocalBounds().withTrimmedLeft (pb.getWidth() + 8));

    // The glass goes over everything, including the colourbar - it magnifies
    // what is on screen, and the scale is part of that. Skipped while its own
    // snapshot is being taken, or it would appear inside itself.
    if (magnifierOn_ && magValid_ && ! magBusy_)
        drawMagnifier (g);
}

// ---------------------------------------------------------------------------
// Elevation: the rig seen from the front or the side, drawn to scale.
//
// This is a DRAWING, not a prediction. Every device's measured data is a
// single horizontal plane, so there is no vertical directivity to solve with;
// showing a heatmap here would be inventing coverage. What it does show is the
// thing a plan view structurally cannot: how high each cabinet sits and how
// far it is tilted.
// ---------------------------------------------------------------------------
void RadiationPatternComponent::drawElevation (juce::Graphics& g, juce::Rectangle<int> area)
{
    const bool front = (params_.viewMode == ViewMode::ElevationFront);
    const auto pb = area.reduced (juce::roundToInt (area.getWidth() * 0.06f),
                                  juce::roundToInt (area.getHeight() * 0.09f));
    if (pb.getWidth() < 40 || pb.getHeight() < 40) return;

    // --- what to frame -----------------------------------------------------
    // Horizontal axis is the world axis we are NOT looking along: the front
    // view looks down the firing axis (+x), so it spreads the rig across y.
    float lo = 0.0f, hi = 0.0f, topM = 0.0f;
    bool any = false;
    for (const auto& s : speakers_)
    {
        const auto cab = cabinetFor (s.model);
        const float c    = front ? s.y : s.x;
        const float half = 0.5f * (front ? cab.widthM : cab.depthM);
        const float t    = s.baseHeightM + cab.heightM;
        lo   = any ? juce::jmin (lo, c - half) : c - half;
        hi   = any ? juce::jmax (hi, c + half) : c + half;
        topM = any ? juce::jmax (topM, t) : t;
        any  = true;
    }
    if (! any) { lo = 0.0f; hi = 10.0f; topM = 3.0f; }

    const float spanH = juce::jmax (4.0f, (hi - lo) * 1.35f);
    const float spanV = juce::jmax (3.0f, topM * 1.45f);
    const float cH    = 0.5f * (lo + hi);

    // One scale for both axes, or the cabinets stop being to scale.
    const float sc = juce::jmin ((float) pb.getWidth()  / spanH,
                                 (float) pb.getHeight() / spanV);
    const float groundY = (float) pb.getBottom();
    const float midX    = (float) pb.getCentreX();
    auto hx = [&] (float world) { return midX + (world - cH) * sc; };
    auto vy = [&] (float metres) { return groundY - metres * sc; };

    // --- grid + height scale ----------------------------------------------
    const juce::String u = Units::lengthUnit();
    g.setColour (Brand::plotGrid().withAlpha (0.35f));
    const float stepM = spanV > 12.0f ? 5.0f : (spanV > 6.0f ? 2.0f : 1.0f);
    for (float m = stepM; m <= spanV; m += stepM)
    {
        const float yy = vy (m);
        if (yy < pb.getY()) break;
        g.drawHorizontalLine (juce::roundToInt (yy), (float) pb.getX(), (float) pb.getRight());
    }
    g.setColour (Brand::ash());
    g.setFont (Brand::tech (juce::jmax (9.0f, 11.0f * Brand::UI::scale)));
    for (float m = stepM; m <= spanV; m += stepM)
    {
        const float yy = vy (m);
        if (yy < pb.getY()) break;
        g.drawText (juce::String (Units::metresToDisplay (m), 1) + " " + u,
                    pb.getX() - 4, juce::roundToInt (yy) - 8, 60, 16,
                    juce::Justification::centredRight);
    }

    // --- ground ------------------------------------------------------------
    g.setColour (Brand::text().withAlpha (0.85f));
    g.drawLine ((float) pb.getX() - 8.0f, groundY, (float) pb.getRight() + 8.0f, groundY, 2.0f);
    g.setFont (Brand::tech (juce::jmax (9.0f, 11.0f * Brand::UI::scale)));
    g.setColour (Brand::ash());
    g.drawText (front ? "FRONT ELEVATION  -  looking along the firing axis"
                      : "SIDE ELEVATION  -  cabinets fire to the right",
                pb.getX(), pb.getY() - 22, pb.getWidth(), 18,
                juce::Justification::centredLeft);

    // Horizontal scale: which way along the rig you are looking, and where.
    {
        const float halfSpan = 0.5f * spanH;
        const float stepH = halfSpan > 25.0f ? 10.0f : (halfSpan > 10.0f ? 5.0f : 2.0f);
        const float first = std::ceil ((cH - halfSpan) / stepH) * stepH;
        g.setFont (Brand::tech (juce::jmax (8.0f, 10.0f * Brand::UI::scale)));
        for (float m = first; m <= cH + halfSpan; m += stepH)
        {
            const float xx = hx (m);
            if (xx < pb.getX() || xx > pb.getRight()) continue;
            g.setColour (Brand::plotGrid().withAlpha (0.25f));
            g.drawVerticalLine (juce::roundToInt (xx), (float) pb.getY(), groundY);
            g.setColour (Brand::ash());
            g.drawText (juce::String (Units::metresToDisplay (m), 0) + " " + u,
                        juce::roundToInt (xx) - 30, juce::roundToInt (groundY) + 4, 60, 14,
                        juce::Justification::centred);
        }
        g.setColour (Brand::ash());
        g.drawText (front ? "position across the rig (world Y)"
                          : "position along the firing axis (world X)",
                    pb.getX(), juce::roundToInt (groundY) + 20, pb.getWidth(), 14,
                    juce::Justification::centredLeft);
    }

    // --- cabinets ----------------------------------------------------------
    for (int i = 0; i < (int) speakers_.size(); ++i)
    {
        const auto& s = speakers_[(size_t) i];
        const auto cab = cabinetFor (s.model);
        const float c    = front ? s.y : s.x;
        const float wM   = front ? cab.widthM : cab.depthM;
        const float halfW = 0.5f * wM * sc;
        const float hPx   = cab.heightM * sc;
        const float cy    = vy (s.baseHeightM + 0.5f * cab.heightM);

        juce::Rectangle<float> box (hx (c) - halfW, cy - 0.5f * hPx, 2.0f * halfW, hPx);

        const bool sel = (std::find (selectedSpeakers_.begin(), selectedSpeakers_.end(), i)
                          != selectedSpeakers_.end());
        juce::Graphics::ScopedSaveState ss (g);

        // Tilt is a rotation about the cabinet's own centre. Edge-on in the
        // front view, so it is only applied from the side -- drawing it
        // rotated head-on would show a tilt that is not really visible there.
        if (! front && std::abs (s.tiltDeg) > 0.01f)
            g.addTransform (juce::AffineTransform::rotation (
                juce::degreesToRadians (s.tiltDeg), box.getCentreX(), box.getCentreY()));

        g.setColour (s.enabled ? Brand::white() : Brand::white().withAlpha (0.35f));
        g.fillRect (box);
        g.setColour (sel ? Brand::accent() : Brand::charcoal());
        g.drawRect (box, sel ? 2.5f : 1.2f);

        // Which way it fires, so a tilted box reads unambiguously.
        if (! front)
        {
            const float arrow = juce::jmin (box.getHeight() * 0.45f, box.getWidth() * 0.8f);
            juce::Path tri;
            tri.addTriangle (box.getCentreX() - arrow * 0.3f, box.getCentreY() - arrow * 0.5f,
                             box.getCentreX() - arrow * 0.3f, box.getCentreY() + arrow * 0.5f,
                             box.getCentreX() + arrow * 0.5f, box.getCentreY());
            g.setColour (Brand::charcoal().withAlpha (0.75f));
            g.fillPath (tri);
        }
    }

    // Labels go on last, unrotated, so they stay readable whatever the tilt.
    g.setFont (Brand::tech (juce::jmax (8.0f, 10.0f * Brand::UI::scale), true));
    for (int i = 0; i < (int) speakers_.size(); ++i)
    {
        const auto& s = speakers_[(size_t) i];
        const auto cab = cabinetFor (s.model);
        const float c  = front ? s.y : s.x;
        const float ty = vy (s.baseHeightM + cab.heightM) - 15.0f;
        juce::String tag = juce::String (speakerModelTag (s.model))
                         + "_" + juce::String (speakerModelOrdinal (speakers_, i));
        if (! front && std::abs (s.tiltDeg) > 0.01f)
            tag += "  " + juce::String (s.tiltDeg, 1) + " deg";
        g.setColour (Brand::text());
        g.drawText (tag, juce::roundToInt (hx (c)) - 60, juce::roundToInt (ty), 120, 14,
                    juce::Justification::centred);
    }

    if (! any)
    {
        g.setColour (Brand::ash());
        g.setFont (Brand::tech (juce::jmax (11.0f, 13.0f * Brand::UI::scale)));
        g.drawText ("Add a speaker to see it in elevation",
                    pb, juce::Justification::centred);
    }
}

// ---------------------------------------------------------------------------
// Annotations <-> JSON. Kept on the component rather than in ProjectData so
// the project format does not have to know what an Annotation is - it stores
// the result verbatim.
// ---------------------------------------------------------------------------
juce::var RadiationPatternComponent::annotationsToVar (const std::vector<Annotation>& a)
{
    juce::Array<juce::var> out;
    for (const auto& an : a)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("kind",         (int) an.kind);
        o->setProperty ("construction", (int) an.construction);
        o->setProperty ("space",        (int) an.space);
        o->setProperty ("colour",       (int) (juce::int64) an.colour.getARGB());
        o->setProperty ("fillAlpha",    an.fillAlpha);
        o->setProperty ("thicknessPx",  an.thicknessPx);
        o->setProperty ("closed",       an.closed);
        o->setProperty ("text",         an.text);
        o->setProperty ("rotationDeg",  an.rotationDeg);
        o->setProperty ("fontPx",       an.fontPx);
        o->setProperty ("align",        an.align);
        o->setProperty ("valign",       an.valign);
        o->setProperty ("bold",         an.bold);
        o->setProperty ("italic",       an.italic);
        o->setProperty ("lineHeight",   an.lineHeight);
        o->setProperty ("isPlane",      an.isPlane);
        o->setProperty ("planeType",    (int) an.planeType);
        o->setProperty ("listenHgt",    (int) an.listenHgt);
        o->setProperty ("showTag",      an.showTag);

        // Points go in as a flat x,y,x,y list - half the JSON of an array of
        // objects, and the order is the only thing that carries meaning.
        juce::Array<juce::var> pts;
        for (const auto& pt : an.pts) { pts.add (pt.x); pts.add (pt.y); }
        o->setProperty ("pts", pts);

        out.add (juce::var (o));
    }
    return out;
}

std::vector<RadiationPatternComponent::Annotation>
RadiationPatternComponent::annotationsFromVar (const juce::var& v)
{
    std::vector<Annotation> out;
    const auto* arr = v.getArray();
    if (arr == nullptr) return out;

    for (const auto& e : *arr)
    {
        auto* o = e.getDynamicObject();
        if (o == nullptr) continue;

        Annotation an;
        auto geti = [o] (const char* k, int fallback)
        { return o->hasProperty (k) ? (int) o->getProperty (k) : fallback; };
        auto getf = [o] (const char* k, float fallback)
        { return o->hasProperty (k) ? (float) (double) o->getProperty (k) : fallback; };
        auto getb = [o] (const char* k, bool fallback)
        { return o->hasProperty (k) ? (bool) o->getProperty (k) : fallback; };

        // Clamped on the way in: a corrupt or hand-edited file must not be able
        // to hand the painter an out-of-range enum.
        an.kind         = (Annotation::Kind) juce::jlimit (0, (int) Annotation::Kind::TextBox,
                                                           geti ("kind", 0));
        an.construction = (Construction) juce::jmax (0, geti ("construction", 0));
        an.space        = (AnnotSpace) juce::jmax (0, geti ("space", 0));
        an.colour       = juce::Colour ((juce::uint32) geti ("colour",
                                        (int) (juce::int64) an.colour.getARGB()));
        an.fillAlpha    = juce::jlimit (0.0f, 1.0f, getf ("fillAlpha", an.fillAlpha));
        an.thicknessPx  = juce::jlimit (0.1f, 64.0f, getf ("thicknessPx", an.thicknessPx));
        an.closed       = getb ("closed", false);
        an.text         = o->getProperty ("text").toString();
        an.rotationDeg  = getf ("rotationDeg", 0.0f);
        an.fontPx       = getf ("fontPx", 0.0f);
        an.align        = juce::jlimit (0, 2, geti ("align", 0));
        an.valign       = juce::jlimit (0, 2, geti ("valign", 0));
        an.bold         = getb ("bold", false);
        an.italic       = getb ("italic", false);
        an.lineHeight   = juce::jlimit (1.0f, 3.0f, getf ("lineHeight", 1.0f));
        an.isPlane      = getb ("isPlane", false)
                            && Annotation::canBePlane (an.kind);
        an.planeType    = (Annotation::PlaneType) juce::jlimit (
                              0, (int) Annotation::PlaneType::Architectural,
                              geti ("planeType", 0));
        an.listenHgt    = (Annotation::ListenHeight) juce::jlimit (
                              0, (int) Annotation::ListenHeight::Standing,
                              geti ("listenHgt", 0));
        // Older files have no such property, and a plane in one was drawn with
        // its tag showing, so that is what they keep.
        an.showTag      = getb ("showTag", true);

        if (const auto* pts = o->getProperty ("pts").getArray())
            for (int i = 0; i + 1 < pts->size(); i += 2)
                an.pts.push_back ({ (float) (double) (*pts)[i],
                                    (float) (double) (*pts)[i + 1] });

        // A shape with no points cannot be drawn or selected, so it would be an
        // invisible item in every list it appears in.
        if (! an.pts.empty())
            out.push_back (std::move (an));
    }
    return out;
}

// ---------------------------------------------------------------------------
bool RadiationPatternComponent::listeningClipPath (juce::Path& out) const
{
    const auto pb = plotArea();
    // Long enough to run clear of the plot from anywhere inside it.
    const float big = 2.0f * (float) juce::jmax (pb.getWidth(), pb.getHeight());

    // Which side of an open plane the audience is on: the side the sound
    // arrives from. With nothing enabled, up the screen - a stage faces the
    // room, and that is how a section is drawn.
    juce::Point<float> sourceCentre { (float) pb.getCentreX(), (float) pb.getY() };
    {
        double sx = 0.0, sy = 0.0; int n = 0;
        for (const auto& spk : speakers_)
        {
            if (! spk.enabled) continue;
            const auto sc = worldToScreen (spk.x, spk.y);
            sx += sc.x; sy += sc.y; ++n;
        }
        if (n > 0) sourceCentre = { (float) (sx / n), (float) (sy / n) };
    }

    bool any = false;
    for (const auto& a : annotations_)
    {
        if (! a.isPlane || a.planeType != Annotation::PlaneType::Listening) continue;
        if (a.space != AnnotSpace::World || a.pts.size() < 2) continue;

        switch (a.kind)
        {
            case Annotation::Kind::Rectangle:
            case Annotation::Kind::Square:
            {
                // A closed plane IS the audience area: the seating is inside it.
                const auto r = normalisedShapeRect (a.pts[0], a.pts[1], a.kind);
                const auto c = r.getCentre();
                for (int i = 0; i < 4; ++i)
                {
                    const juce::Point<float> corner {
                        (i == 0 || i == 3) ? r.getX() : r.getRight(),
                        (i < 2)            ? r.getY() : r.getBottom()
                    };
                    const auto sp = annotateToScreen (a, rotateAround (corner, c, a.rotationDeg));
                    if (i == 0) out.startNewSubPath (sp);
                    else        out.lineTo (sp);
                }
                out.closeSubPath();
                any = true;
                break;
            }

            case Annotation::Kind::Circle:
            {
                const auto& cw = a.pts[0];
                const float rad = cw.getDistanceFrom (a.pts[1]);
                if (rad < 1.0e-6f) break;
                constexpr int kSteps = 96;
                for (int i = 0; i < kSteps; ++i)
                {
                    const float ang = (float) i / kSteps * 2.0f * (float) M_PI;
                    const auto sp = annotateToScreen (a, { cw.x + rad * std::cos (ang),
                                                           cw.y + rad * std::sin (ang) });
                    if (i == 0) out.startNewSubPath (sp);
                    else        out.lineTo (sp);
                }
                out.closeSubPath();
                any = true;
                break;
            }

            default:
            {
                // An open plane has no inside, so the audience is the ground it
                // runs along: everything on the side the sound comes from. The
                // band is swept off the plane far enough to leave the plot.
                std::vector<juce::Point<float>> line;
                line.reserve (a.pts.size());
                for (const auto& wp : a.pts)
                    line.push_back (annotateToScreen (a, wp));
                if (line.size() < 2) break;

                // Run the ends out past the edge, so the audience does not stop
                // where the drawn line happens to stop.
                auto extend = [big] (juce::Point<float> from, juce::Point<float> towards)
                {
                    auto d = towards - from;
                    const float len = std::sqrt (d.x * d.x + d.y * d.y);
                    if (len < 1.0e-6f) return towards;
                    return towards + d * (big / len);
                };
                line.front() = extend (line[1], line.front());
                line.back()  = extend (line[line.size() - 2], line.back());

                auto axis = line.back() - line.front();
                const float alen = std::sqrt (axis.x * axis.x + axis.y * axis.y);
                if (alen < 1.0e-6f) break;
                juce::Point<float> nrm { -axis.y / alen, axis.x / alen };
                const auto toSource = sourceCentre - line.front();
                if (nrm.x * toSource.x + nrm.y * toSource.y < 0.0f)
                    nrm = { -nrm.x, -nrm.y };

                out.startNewSubPath (line.front());
                for (size_t i = 1; i < line.size(); ++i)
                    out.lineTo (line[i]);
                for (size_t i = line.size(); i-- > 0;)
                    out.lineTo (line[i] + nrm * big);
                out.closeSubPath();
                any = true;
                break;
            }
        }
    }
    return any;
}

void RadiationPatternComponent::drawField (juce::Graphics& g, juce::Rectangle<int>)
{
    if (! fieldImage_.isValid()) return;

    const auto tl = worldToScreen ((float) result_.worldX0,
                                   (float) (result_.worldY0 + result_.worldH));   // top-left
    const float w = (float) result_.worldW * worldScaleX();
    const float h = (float) result_.worldH * worldScaleY();

    // Smooth gradient (EASE-style) interpolates; banded contours stay crisp.
    const bool banded = (params_.viewMode == ViewMode::SPL && params_.bandedSPL);
    juce::Graphics::ScopedSaveState ss (g);

    // Mark out an audience and the map belongs to it: paint the level where
    // people are, not into the air above them. No listening plane means
    // nobody has said where the audience is, so the whole field shows.
    juce::Path audience;
    if (listeningClipPath (audience))
    {
        // Off the audience, the colour map's own floor - the shade a silent
        // field would be. The bare component background is white in the light
        // theme, which left a glaring slab where the map stopped.
        g.fillAll (ColourMaps::sevenColor (0.0f));
        g.reduceClipRegion (audience);
    }

    g.setImageResamplingQuality (banded ? juce::Graphics::lowResamplingQuality
                                        : juce::Graphics::highResamplingQuality);
    g.setOpacity (1.0f);
    g.drawImage (fieldImage_,
                 juce::Rectangle<float> (tl.x, tl.y, w, h),
                 juce::RectanglePlacement::stretchToFit);
}

void RadiationPatternComponent::drawSplValues (juce::Graphics& g,
                                               juce::Rectangle<int> plotBounds)
{
    if (! showSplValues_ || ! showField_ || ! hasData_) return;

    // One number per major grid cell, doubled up until they are far enough
    // apart on screen to read. Zoom out and the readout thins instead of
    // turning into a smear of overlapping digits.
    double stepM = currentGridMetrics().major;
    const double pxPerM = (double) worldScaleX();
    if (stepM <= 0.0 || pxPerM <= 1.0e-6) return;
    while (stepM * pxPerM < 78.0) stepM *= 2.0;

    juce::Graphics::ScopedSaveState ss (g);
    // Where the map is painted is where the numbers belong, so they follow the
    // audience in exactly the same way.
    if (juce::Path audience; listeningClipPath (audience))
        g.reduceClipRegion (audience);

    const float fh = juce::jmax (9.0f, Brand::UI::scaledFont (10.0f));
    g.setFont (Brand::techSemi (fh));
    const float boxW = fh * 3.4f;
    const float boxH = fh * 1.45f;

    const double xEnd = result_.worldX0 + result_.worldW;
    const double yEnd = result_.worldY0 + result_.worldH;
    // Keep clear of the strips that already carry writing: the banner along
    // the top, the axis numbers along the bottom and down the left. A reading
    // printed over them helps nobody and hides both.
    const auto pbF = plotBounds.toFloat()
                         .withTrimmedTop    (boxH + 10.0f)
                         .withTrimmedBottom (boxH + 12.0f)
                         .withTrimmedLeft   (boxW * 0.75f);

    for (double wy = std::ceil (result_.worldY0 / stepM) * stepM; wy <= yEnd; wy += stepM)
    {
        for (double wx = std::ceil (result_.worldX0 / stepM) * stepM; wx <= xEnd; wx += stepM)
        {
            const auto sc = worldToScreen ((float) wx, (float) wy);
            if (! pbF.contains (sc)) continue;

            float absDb = 0.0f, relDb = 0.0f;
            if (! sampleSplAtWorld ((float) wx, (float) wy, absDb, relDb)) continue;
            const float v = result_.hasAbsoluteSpl ? absDb : relDb;

            // A plate behind each number, because the map runs from near-black
            // to bright red and no single ink colour reads on both.
            const juce::Rectangle<float> plate (sc.x - boxW * 0.5f, sc.y - boxH * 0.5f,
                                                boxW, boxH);
            g.setColour (Brand::charcoal().withAlpha (0.72f));
            g.fillRoundedRectangle (plate, 3.0f);
            g.setColour (Brand::white());
            g.drawText (juce::String (v, 1), plate, juce::Justification::centred, false);
        }
    }
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
    // Aim for ~20 px minor / ~90 px major so cells stay readable.
    // Steps are chosen in the *display* unit (m or ft), then stored as metres
    // so axis labels read as clean 25 / 50 / 75 ft (or m) rather than ugly conversions.
    const double pxPerM = (double) worldScaleX();
    GridMetrics m;
    if (pxPerM <= 1.0e-6)
        return m;

    const bool imperial = Units::imperial();
    constexpr double kFtPerM = 3.280839895;
    const double toDisp = imperial ? kFtPerM : 1.0;
    const double pxPerDisp = pxPerM / toDisp;          // px per display-unit
    const double minDisp = imperial ? (0.001 * kFtPerM) : kMinGridM; // ≥ 1 mm

    const double minorDisp = juce::jmax (minDisp, niceStep (20.0 / pxPerDisp));
    double majorDisp = juce::jmax (minorDisp, niceStep (90.0 / pxPerDisp));

    const double ratio = majorDisp / minorDisp;
    if (ratio < 2.5)
        majorDisp = minorDisp * 5.0;
    else if (ratio < 7.5)
        majorDisp = minorDisp * 5.0;
    else
        majorDisp = minorDisp * 10.0;

    m.minor = minorDisp / toDisp;   // back to metres for world-space ticks
    m.major = majorDisp / toDisp;
    return m;
}

juce::String RadiationPatternComponent::formatGridLabel (double metres)
{
    // Tick positions land on nice display steps - show them in the active unit.
    if (Units::imperial())
    {
        const double ft = metres * 3.280839895;
        const double a  = std::abs (ft);
        if (a < 1.0e-9)
            return "0";
        if (a + 1.0e-9 < 1.0)
        {
            const double inches = metres * 39.37007874;
            if (std::abs (inches - std::round (inches)) < 1.0e-4)
                return juce::String ((int) std::lround (inches)) + " in";
            return juce::String (inches, 1) + " in";
        }
        if (std::abs (ft - std::round (ft)) < 1.0e-4)
            return juce::String ((int) std::lround (ft)) + " ft";
        return juce::String (ft, 1) + " ft";
    }
    return Units::formatLengthSmart (metres);
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
    // The solved region, wherever panning has moved it to.
    const double xMin = (double) result_.worldX0;
    const double xMax = xMin + (double) ww;
    const double yMin = (double) result_.worldY0;
    const double yMax = yMin + (double) wh;

    // Clipped to that region, so the grid never advertises area outside it --
    // placement clamps to the region, so ruled space beyond its edge would be
    // space you cannot actually click into.
    const double visX0 = juce::jlimit (xMin, xMax, (double) std::min (tl.x, br.x));
    const double visX1 = juce::jlimit (xMin, xMax, (double) std::max (tl.x, br.x));
    const double visY0 = juce::jlimit (yMin, yMax, (double) std::min (tl.y, br.y));
    const double visY1 = juce::jlimit (yMin, yMax, (double) std::max (tl.y, br.y));

    const juce::Colour minorCol = Brand::plotGrid().withMultipliedAlpha (UiConfig::PlotGrid::minorAlpha);
    const juce::Colour majorCol = Brand::plotGrid().withMultipliedAlpha (UiConfig::PlotGrid::majorAlpha);
    // Axis numbers sit ON the plot surface, so their colour has to follow what
    // is actually behind them rather than the app theme. With a result on
    // screen that surface is the SPL heatmap -- near-black at the dB floor --
    // and the light theme's black ink disappeared into it entirely. White over
    // the heatmap, the theme ink over the bare plot background.
    const bool         overField = hasData_ && fieldImage_.isValid();
    const juce::Colour labelInk  = overField ? Brand::white() : Brand::axisLabel();
    const juce::Colour labelHalo = (overField ? juce::Colours::black : Brand::plotBg())
                                       .withAlpha (0.55f);

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
        g.drawLine (a.x, a.y, b.x, b.y, major ? UiConfig::PlotGrid::majorThickness
                                             : UiConfig::PlotGrid::minorThickness);
    };
    auto hline = [&] (double ym, bool major)
    {
        auto a = worldToScreen ((float) xMin, (float) ym);
        auto b = worldToScreen ((float) xMax, (float) ym);
        g.setColour (major ? majorCol : minorCol);
        g.drawLine (a.x, a.y, b.x, b.y, major ? UiConfig::PlotGrid::majorThickness
                                             : UiConfig::PlotGrid::minorThickness);
    };

    forTicks (visX0, visX1, minorStep, [&] (double x) { if (! isMajor (x)) vline (x, false); });
    forTicks (visY0, visY1, minorStep, [&] (double y) { if (! isMajor (y)) hline (y, false); });
    forTicks (visX0, visX1, majorStep, [&] (double x) { vline (x, true); });
    forTicks (visY0, visY1, majorStep, [&] (double y) { hline (y, true); });

    // Axis labels follow major spacing so zoom reveals 62, 63, 64... then cm/mm.
    g.setFont (Brand::techMed (Brand::Type::gridNum));
    const int labelW = Units::imperial() ? 72 : 56;

    // Anchor the numbers to the WORLD's edges, not the component's. Now that
    // the view fits the whole field, the world is usually narrower than the
    // plot area and centred in it -- placing labels at the component's left
    // edge stranded the whole vertical axis out in the empty margin, detached
    // from the grid it annotates (and sitting on the page background, where
    // the over-the-heatmap white ink was unreadable).
    // Anchored to the WORLD's edges, not the component's. Fitted to contain,
    // the field is centred with margins on one axis, and placing numbers at
    // the component's edge stranded the whole vertical axis out in the empty
    // margin -- detached from the grid it annotates, and on a pale background
    // where the over-the-field white ink is unreadable.
    const auto wtl = worldToScreen ((float) result_.worldX0,
                                    (float) (result_.worldY0 + wh));   // top-left
    const auto wbr = worldToScreen ((float) (result_.worldX0 + ww),
                                    (float) result_.worldY0);          // bottom-right
    const auto axisBox = bounds.getIntersection (
        juce::Rectangle<int> (juce::roundToInt (wtl.x), juce::roundToInt (wtl.y),
                              juce::jmax (1, juce::roundToInt (wbr.x - wtl.x)),
                              juce::jmax (1, juce::roundToInt (wbr.y - wtl.y))));

    // The heatmap runs from a saturated red at 0 dB to near-black at the
    // floor, so a single flat ink colour cannot stay legible across all of it.
    // A one-pixel halo in the opposite tone keeps the digits readable over
    // every band without putting an opaque chip on top of the data.
    // Labels are drawn left-justified from their tick, so when the window gets
    // small enough that major ticks fall closer together than a label is wide
    // they run into each other ("90 m100 m"). Track the last one placed on each
    // axis and skip any that would collide.
    static constexpr int kNoTick = std::numeric_limits<int>::min();
    int lastTickPos = kNoTick;
    auto drawTick = [&] (const juce::String& txt, juce::Rectangle<int> r)
    {
        g.setColour (labelHalo);
        for (auto d : { juce::Point<int> (-1, 0), juce::Point<int> (1, 0),
                        juce::Point<int> (0, -1), juce::Point<int> (0, 1) })
            g.drawText (txt, r.translated (d.x, d.y), juce::Justification::left);
        g.setColour (labelInk);
        g.drawText (txt, r, juce::Justification::left);
    };

    // Distance from the last label placed, NOT a directional "past the right
    // edge" test: world y increases upward while screen y increases downward,
    // so the vertical run walks backwards and a one-sided comparison rejects
    // every label after the first.
    auto tickFits = [&] (int pos, int need)
    {
        if (lastTickPos != kNoTick && std::abs (pos - lastTickPos) < need)
            return false;
        lastTickPos = pos;
        return true;
    };
    forTicks (visX0, visX1, majorStep, [&] (double x)
    {
        auto a = worldToScreen ((float) x, 0.0f);
        if (a.x < (float) bounds.getX() - 4.0f || a.x > (float) bounds.getRight() + 4.0f)
            return;
        // Clamp against the label's real glyph width, not the roomy chip it is
        // drawn into: using the chip width shoved the final tick ("100 m")
        // far enough left to collide with its neighbour and be dropped, which
        // lost exactly the label that confirms the field's full extent.
        const auto txt  = formatGridLabel (x);
        const int  inkW = juce::roundToInt (g.getCurrentFont().getStringWidthFloat (txt));
        const int chipX = juce::jlimit (axisBox.getX() + 2,
                                        juce::jmax (axisBox.getX() + 2, axisBox.getRight() - inkW - 2),
                                        (int) a.x + 2);
        const int chipY = axisBox.getBottom() - 16;
        if (tickFits (chipX, inkW + 6))
            drawTick (txt, { chipX, chipY, labelW, 14 });
    });
    lastTickPos = kNoTick;   // vertical axis: its own run
    forTicks (visY0, visY1, majorStep, [&] (double y)
    {
        auto a = worldToScreen (0.0f, (float) y);
        if (a.y < (float) bounds.getY() - 4.0f || a.y > (float) bounds.getBottom() + 4.0f)
            return;
        const int chipY = juce::jlimit (axisBox.getY() + 2, juce::jmax (axisBox.getY() + 2,
                                        axisBox.getBottom() - 16), (int) a.y - 7);
        // Stacked vertically, so the collision test is on y, not x.
        if (! tickFits (chipY, 16))
            return;
        drawTick (formatGridLabel (y), { axisBox.getX() + 2, chipY, labelW, 14 });
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

juce::Rectangle<int> RadiationPatternComponent::fieldScreenBounds() const
{
    const auto pb = plotArea();
    const double ww = (result_.worldW > 0 ? result_.worldW : params_.worldW);
    const double wh = (result_.worldH > 0 ? result_.worldH : params_.worldH);
    if (ww <= 0 || wh <= 0) return pb;

    const auto tl = worldToScreen ((float) result_.worldX0,
                                   (float) (result_.worldY0 + wh));
    const auto br = worldToScreen ((float) (result_.worldX0 + ww),
                                   (float) result_.worldY0);
    return pb.getIntersection (
        juce::Rectangle<int> (juce::roundToInt (tl.x), juce::roundToInt (tl.y),
                              juce::jmax (1, juce::roundToInt (br.x - tl.x)),
                              juce::jmax (1, juce::roundToInt (br.y - tl.y))));
}

juce::Rectangle<float> RadiationPatternComponent::speakerFootprintWorld (const Speaker& spk) const
{
    // Plan view: depth along X (firing), width along Y. Each model has its
    // own enclosure, so a 2" horn draws at its true 459 x 150 mm footprint
    // rather than borrowing the Q21S's 1546 x 1024 mm.
    const auto cab = cabinetFor (spk.model);
    const float hw = cab.widthM * 0.5f;
    const float hd = cab.depthM * 0.5f;
    return { spk.x - hd, spk.y - hw, cab.depthM, cab.widthM };
}

// ---------------------------------------------------------------------------
// Dimension one plan marker the way a drawing would: extension lines off the
// cabinet's corners, a dimension line between them with arrowheads, and the
// value on it. A single "1546 x 679 x 1024 mm" string said nothing about
// WHICH edge was which; this puts each number against the edge it measures.
//
// Height has no edge to sit against -- the plan looks straight down, so the
// cabinet's height is into the page. It is called out separately rather than
// implied against an edge it does not belong to.
// ---------------------------------------------------------------------------
void RadiationPatternComponent::drawSpeakerDimensions (juce::Graphics& g,
                                                       juce::Rectangle<float> box,
                                                       const Speaker& spk,
                                                       float alpha)
{
    // Below roughly 26 px the arrowheads and text collide into a smudge, so
    // the marker gets the compact one-line label instead of unreadable
    // decoration. Zooming in swaps it for the full dimensioning.
    const bool roomy = (box.getWidth() >= 26.0f && box.getHeight() >= 26.0f);
    const auto cab = cabinetFor (spk.model);

    const juce::Colour ink  = Brand::white().withAlpha (0.92f * alpha);
    const juce::Colour thin = Brand::white().withAlpha (0.55f * alpha);

    if (! roomy)
    {
        g.setFont (Brand::tech (juce::jmax (8.0f, 9.5f * Brand::UI::scale)));
        g.setColour (Brand::white().withAlpha (0.78f * alpha));
        g.drawText (Units::dims3 (cab.widthM * 1000.0, cab.heightM * 1000.0,
                                  cab.depthM * 1000.0),
                    (int) (box.getCentreX() - 80.0f), (int) (box.getBottom() + 3.0f),
                    160, 13, juce::Justification::centred);
        return;
    }

    const float off  = 13.0f;   // gap from the cabinet to its dimension line
    const float over = 4.0f;    // extension line overshoot past it
    const float ah   = 6.0f;    // arrowhead length
    const float aw   = 2.8f;    // arrowhead half-width

    auto arrow = [&] (juce::Point<float> tip, juce::Point<float> from)
    {
        const auto d = (tip - from);
        const float len = juce::jmax (0.001f, d.getDistanceFromOrigin());
        const auto u = juce::Point<float> (d.x / len, d.y / len);
        const auto n = juce::Point<float> (-u.y, u.x);
        juce::Path t;
        t.addTriangle (tip.x, tip.y,
                       tip.x - u.x * ah + n.x * aw, tip.y - u.y * ah + n.y * aw,
                       tip.x - u.x * ah - n.x * aw, tip.y - u.y * ah - n.y * aw);
        g.fillPath (t);
    };

    g.setFont (Brand::tech (juce::jmax (8.5f, 10.0f * Brand::UI::scale), true));

    // --- DEPTH: the firing axis, horizontal on screen. Dimension below. ----
    {
        const float yLine = box.getBottom() + off;
        g.setColour (thin);
        g.drawLine (box.getX(), box.getBottom() + 2.0f, box.getX(), yLine + over, 1.0f);
        g.drawLine (box.getRight(), box.getBottom() + 2.0f, box.getRight(), yLine + over, 1.0f);
        g.setColour (ink);
        g.drawLine (box.getX(), yLine, box.getRight(), yLine, 1.2f);
        arrow ({ box.getX(),     yLine }, { box.getX() + ah,     yLine });
        arrow ({ box.getRight(), yLine }, { box.getRight() - ah, yLine });
        g.drawText ("D " + Units::dim (cab.depthM * 1000.0),
                    (int) (box.getCentreX() - 60.0f), (int) (yLine + 2.0f), 120, 13,
                    juce::Justification::centred);
    }

    // --- WIDTH: across the baffle, vertical on screen. Dimension to the right.
    {
        const float xLine = box.getRight() + off;
        g.setColour (thin);
        g.drawLine (box.getRight() + 2.0f, box.getY(), xLine + over, box.getY(), 1.0f);
        g.drawLine (box.getRight() + 2.0f, box.getBottom(), xLine + over, box.getBottom(), 1.0f);
        g.setColour (ink);
        g.drawLine (xLine, box.getY(), xLine, box.getBottom(), 1.2f);
        arrow ({ xLine, box.getY() },      { xLine, box.getY() + ah });
        arrow ({ xLine, box.getBottom() }, { xLine, box.getBottom() - ah });

        // Rotated so it runs along the edge it measures, like a drawing.
        const float textX = xLine + 9.0f;
        const float ty = box.getCentreY();

        juce::Graphics::ScopedSaveState ss (g);
        g.addTransform (juce::AffineTransform::rotation (
            -juce::MathConstants<float>::halfPi, textX, ty));
        g.drawText ("W " + Units::dim (cab.widthM * 1000.0),
                    (int) (textX - 60.0f), (int) (ty - 7.0f), 120, 14,
                    juce::Justification::centred);
    }

    // No HEIGHT callout here. This is a top-down plan: height goes into the
    // page, so it could only ever be stated rather than drawn, and it cluttered
    // the view without showing anything. The 3D view carries it instead.
}

void RadiationPatternComponent::drawSpeakerRays (juce::Graphics& g,
                                                 juce::Rectangle<int> plotBounds)
{
    // Long enough to leave the plot from anywhere inside it, so the rays end
    // at the edge of the canvas rather than stopping somewhere in the field.
    // The caller has already clipped to the plot area.
    const float reach = 2.0f * (float) juce::jmax (plotBounds.getWidth(),
                                                   plotBounds.getHeight());
    const float dashes[] = { 5.0f, 4.0f };

    for (int i = 0; i < (int) speakers_.size(); ++i)
    {
        const auto& spk = speakers_[(size_t) i];
        if (! spk.enabled || ! rayVisibleFor (i)) continue;

        const auto c = worldToScreen (spk.x, spk.y);
        // Degrees counter-clockwise from +x, and screen y runs the other way.
        const float th = juce::degreesToRadians (
            spk.rotationDeg + (spk.reverseOrientation ? 180.0f : 0.0f));
        // Not `far`: windows.h still defines that as a keyword leftover.
        const juce::Point<float> offPlot { c.x + std::cos (th) * reach,
                                           c.y - std::sin (th) * reach };
        // A listening or architectural plane is where the ray arrives, so the
        // line ends there rather than running on across the field.
        const auto end = clipRayAtPlanes (c, offPlot);

        // The ray you are holding, or have selected, is the accent colour, so
        // it is obvious which one a drag is about to swing. Locked, nothing is
        // grabbable, so nothing wears the accent and the whole set draws back
        // a little - the cabinet's own border still shows what is selected.
        const bool live = ! lockRays_
                       && ((drag_ == Drag::SpeakerRotate && rotatingSpeaker_ == i)
                           || (i == selected_) || isSpeakerSelected (i));
        g.setColour (live ? Brand::accent().withAlpha (0.95f)
                          : Brand::white().withAlpha (lockRays_ ? 0.40f : 0.60f));
        g.drawDashedLine ({ c, end }, dashes, 2, live ? 1.8f : 1.4f);
    }
}

bool RadiationPatternComponent::rayVisibleFor (int i) const noexcept
{
    if (i < 0 || i >= (int) speakers_.size()) return false;
    // A sub has no aiming ray - see isSubwooferModel(). It carries the short
    // stalk and knob instead (aimHandleVisibleFor), so it can still be turned;
    // what it does not get is a line drawn across the whole field claiming a
    // pattern it does not have.
    if (isSubwooferModel (speakers_[(size_t) i].model)) return false;
    return showRays_ || (i == selected_) || isSpeakerSelected (i);
}

bool RadiationPatternComponent::aimHandleVisibleFor (int i) const noexcept
{
    if (i < 0 || i >= (int) speakers_.size()) return false;
    const auto& spk = speakers_[(size_t) i];
    // Only where there is no ray to do the job - otherwise it is the same
    // control drawn twice.
    if (! isSubwooferModel (spk.model)) return false;
    if (! spk.showAimLine) return false;
    return showRays_ || (i == selected_) || isSpeakerSelected (i);
}

int RadiationPatternComponent::aimHandleHitTest (juce::Point<float> p) const
{
    // Lock Rays locks every aiming control, stalk included: a lock that only
    // held half of them would be worse than none.
    if (lockRays_) return -1;

    int   best  = -1;
    float bestD = 11.0f;          // a little past the 8 px knob, in px
    for (int i = 0; i < (int) speakers_.size(); ++i)
    {
        if (! aimHandleVisibleFor (i)) continue;
        const float d = speakerRotateHandle (speakers_[(size_t) i]).getDistanceFrom (p);
        if (d < bestD) { bestD = d; best = i; }
    }
    return best;
}

int RadiationPatternComponent::rayHitTest (juce::Point<float> p) const
{
    if (lockRays_) return -1;

    int   best  = -1;
    float bestD = 7.0f;          // grab tolerance either side of the line, px

    for (int i = 0; i < (int) speakers_.size(); ++i)
    {
        const auto& spk = speakers_[(size_t) i];
        if (! spk.enabled || ! rayVisibleFor (i)) continue;

        const auto c = worldToScreen (spk.x, spk.y);
        const float th = juce::degreesToRadians (
            spk.rotationDeg + (spk.reverseOrientation ? 180.0f : 0.0f));
        const juce::Point<float> dir { std::cos (th), -std::sin (th) };

        const auto v = p - c;
        const float along = v.x * dir.x + v.y * dir.y;

        // Only out along the ray, and clear of the cabinet, so the marker
        // keeps its own click - dragging a speaker must still move it.
        const auto box = speakerFootprintScreen (spk);
        const float start = 0.5f * juce::jmax (box.getWidth(), box.getHeight()) + 12.0f;
        if (along < start) continue;

        // Only the drawn part is grabbable: past a plane the ray is not there
        // to be taken hold of.
        const float reach = 2.0f * (float) juce::jmax (plotArea().getWidth(),
                                                       plotArea().getHeight());
        const auto stop = clipRayAtPlanes (c, c + dir * reach);
        if (along > c.getDistanceFrom (stop)) continue;

        const float perp = std::abs (v.x * -dir.y + v.y * dir.x);
        if (perp < bestD) { bestD = perp; best = i; }
    }
    return best;
}

juce::Point<float> RadiationPatternComponent::speakerRotateHandle (const Speaker& spk) const
{
    // No longer a grip - the ray took that job. This is just where the angle
    // readout is pinned while a rotation drag is running: off the FRONT face,
    // so the number sits where the cabinet is pointing.
    const auto c = worldToScreen (spk.x, spk.y);
    const auto box = speakerFootprintScreen (spk);
    const float reach = 0.5f * juce::jmax (box.getWidth(), box.getHeight()) + 18.0f;
    const float th = juce::degreesToRadians (spk.rotationDeg);
    return { c.x + std::cos (th) * reach, c.y - std::sin (th) * reach };
}

juce::Rectangle<float> RadiationPatternComponent::speakerFootprintScreen (const Speaker& spk) const
{
    // Straight through the view transform: with an isotropic view (see
    // fitView) that renders the cabinet's true 1546 x 1024 mm plan footprint at
    // the same px/m as the field around it, so the marker is to scale rather
    // than a fixed-size icon. Hit-testing shares this rectangle, so clicks
    // match what is drawn.
    const auto wr = speakerFootprintWorld (spk);
    const auto p0 = worldToScreen (wr.getX(), wr.getY());
    const auto p1 = worldToScreen (wr.getRight(), wr.getBottom());
    const float x = juce::jmin (p0.x, p1.x);
    const float y = juce::jmin (p0.y, p1.y);
    const float w = std::abs (p1.x - p0.x);
    const float h = std::abs (p1.y - p0.y);
    return { x, y, juce::jmax (1.0f, w), juce::jmax (1.0f, h) };
}

juce::String RadiationPatternComponent::rangeRingLabel (float metres)
{
    // One decimal only when it says something: 2 m rather than 2.0 m.
    const double d = Units::metresToDisplay ((double) metres);
    const bool whole = std::abs (d - std::round (d)) < 0.05;
    return juce::String (d, whole ? 0 : 1) + " " + Units::lengthUnit();
}

bool RadiationPatternComponent::addRangeRing (float metres)
{
    if (! std::isfinite (metres) || metres <= 0.0f || metres > 10000.0f)
        return false;
    // Two rings a centimetre apart would just be a thick line, and you could
    // never hit the one you meant to delete.
    for (float r : rangeRings_)
        if (std::abs (r - metres) < 0.01f) return false;

    rangeRings_.push_back (metres);
    std::sort (rangeRings_.begin(), rangeRings_.end());
    if (onRangeRingsChanged) onRangeRingsChanged();
    repaint();
    return true;
}

void RadiationPatternComponent::removeRangeRing (float metres)
{
    const auto before = rangeRings_.size();
    rangeRings_.erase (std::remove_if (rangeRings_.begin(), rangeRings_.end(),
                                       [metres] (float r)
                                       { return std::abs (r - metres) < 0.01f; }),
                       rangeRings_.end());
    if (rangeRings_.size() != before)
    {
        if (onRangeRingsChanged) onRangeRingsChanged();
        repaint();
    }
}

void RadiationPatternComponent::setRangeRings (std::vector<float> metres)
{
    std::sort (metres.begin(), metres.end());
    metres.erase (std::unique (metres.begin(), metres.end(),
                               [] (float a, float b) { return std::abs (a - b) < 0.01f; }),
                  metres.end());
    rangeRings_ = std::move (metres);
    if (onRangeRingsChanged) onRangeRingsChanged();
    repaint();
}

void RadiationPatternComponent::setRangesVisible (bool on)
{
    if (rangesVisible_ == on) return;
    rangesVisible_ = on;
    if (onRangeRingsChanged) onRangeRingsChanged();
    repaint();
}

void RadiationPatternComponent::clearRangeRings()
{
    if (rangeRings_.empty()) return;
    rangeRings_.clear();
    if (onRangeRingsChanged) onRangeRingsChanged();
    repaint();
}

float RadiationPatternComponent::rangeRingAtScreen (juce::Point<float> p) const
{
    if (rangeRings_.empty() || ! rangesVisible_) return -1.0f;

    // Rings are drawn round every enabled speaker, so the one you are pointing
    // at is whichever ring of whichever speaker passes closest to the cursor.
    float bestDist = 7.0f;          // grab tolerance, px
    float bestRing = -1.0f;
    for (const auto& spk : speakers_)
    {
        if (! spk.enabled) continue;
        const auto c = worldToScreen (spk.x, spk.y);
        for (float rm : rangeRings_)
        {
            const float rx = rm * worldScaleX();
            const float ry = rm * worldScaleY();
            if (rx < 1.0f || ry < 1.0f) continue;
            // Distance to the ellipse, in the circle's own normalised frame.
            const float nx = (p.x - c.x) / rx;
            const float ny = (p.y - c.y) / ry;
            const float n  = std::sqrt (nx * nx + ny * ny);
            if (n < 1.0e-4f) continue;
            const auto nearest = juce::Point<float> (c.x + nx / n * rx, c.y + ny / n * ry);
            const float d = p.getDistanceFrom (nearest);
            if (d < bestDist) { bestDist = d; bestRing = rm; }
        }
    }
    return bestRing;
}

void RadiationPatternComponent::drawSpeakers (juce::Graphics& g, juce::Rectangle<int>)
{
    // Range rings at the distances the user asked for - nothing is drawn
    // until they add one.
    if (! rangeRings_.empty() && rangesVisible_)
    {
        for (const auto& spk : speakers_)
        {
            if (! spk.enabled) continue;
            const auto c = worldToScreen (spk.x, spk.y);
            for (float rm : rangeRings_)
            {
                const float rx = rm * worldScaleX();
                const float ry = rm * worldScaleY();
                g.setColour (Brand::white().withAlpha (0.55f));
                g.drawEllipse (c.x - rx, c.y - ry, rx * 2.0f, ry * 2.0f, 1.0f);
                g.setFont (Brand::mono (Brand::Type::gridNum));
                g.setColour (Brand::white().withAlpha (0.75f));
                const juce::String lab = rangeRingLabel (rm);
                g.drawText (lab,
                            (int) (c.x + rx * 0.707f) + 4,
                            (int) (c.y - ry * 0.707f) - 10,
                            36, 14, juce::Justification::centredLeft, false);
            }
        }
    }

    // True Q21S plan footprint (750 mm W x 917 mm D). Selected = thick black boundary.
    for (int i = 0; i < (int) speakers_.size(); ++i)
    {
        const auto& spk = speakers_[i];
        const auto c = worldToScreen (spk.x, spk.y);
        const bool isSel = (i == selected_) || isSpeakerSelected (i);
        const float alpha = spk.enabled ? 1.0f : 0.42f;
        const bool reverse = spk.reverseOrientation;

        auto box = speakerFootprintScreen (spk);

        // Everything that belongs to the cabinet turns with it. Screen Y runs
        // downwards, so a counter-clockwise world rotation is a negative one
        // here. The name label is drawn outside this block, upright, because a
        // label you have to tilt your head to read is worse than no label.
        juce::Graphics::ScopedSaveState spkState (g);
        if (std::abs (spk.rotationDeg) > 0.01f)
            g.addTransform (juce::AffineTransform::rotation (
                -juce::degreesToRadians (spk.rotationDeg), c.x, c.y));

        // Body fill first so selection glow can sit inside the footprint.
        g.setColour (Brand::white().withAlpha (0.92f * alpha));
        g.fillRect (box);

        if (isSel)
        {
            // Vivid magenta-red glow INSIDE the cabinet - keeps the outer border crisp.
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
        g.drawText (juce::String (speakerModelTag (spk.model))
                        + "_" + juce::String (speakerModelOrdinal (speakers_, i)),
                    (int) (c.x - 40), (int) (box.getY() - 16.0f), 80, 14,
                    juce::Justification::centred);

        if (showSpeakerDims_)
            drawSpeakerDimensions (g, box, spk, alpha);
    }

    // Aiming handles last and unrotated, so they sit on top of every cabinet.
    // A top is aimed by its ray; a sub has none, so it keeps the original
    // stalk and knob - the only handle it can be turned by, and the reason
    // subs stopped being aimable at all when the ray replaced it.
    for (int i = 0; i < (int) speakers_.size(); ++i)
    {
        if (! aimHandleVisibleFor (i)) continue;

        const auto& spk = speakers_[i];
        const auto  c   = worldToScreen (spk.x, spk.y);
        const auto  h   = speakerRotateHandle (spk);
        const float alpha = spk.enabled ? 1.0f : 0.42f;

        g.setColour (Brand::charcoal().withAlpha (0.55f * alpha));
        g.drawLine (c.x, c.y, h.x, h.y, 1.0f);
        g.setColour (juce::Colour (0xffff3d6e).withAlpha (0.95f * alpha));
        g.fillEllipse (h.x - 4.0f, h.y - 4.0f, 8.0f, 8.0f);
        g.setColour (Brand::white().withAlpha (0.9f * alpha));
        g.drawEllipse (h.x - 4.0f, h.y - 4.0f, 8.0f, 8.0f, 1.0f);
    }

    // The angle readout. On the selected unit it stays up, so the heading can
    // simply be read off; while turning it is the only way to see the 5-degree
    // snap land.
    for (int i = 0; i < (int) speakers_.size(); ++i)
    {
        const auto& spk = speakers_[i];
        const bool turning = (drag_ == Drag::SpeakerRotate && rotatingSpeaker_ == i);
        if (! (turning || ((i == selected_) || isSpeakerSelected (i)))) continue;

        const auto h = speakerRotateHandle (spk);

        if (turning || aimHandleVisibleFor (i))
        {
            g.setFont (Brand::tech (juce::jmax (9.0f, 10.5f * Brand::UI::scale), true));
            const juce::String txt = juce::String (juce::roundToInt (spk.rotationDeg)) + " deg";
            juce::Rectangle<float> pill (h.x - 26.0f, h.y - 26.0f, 52.0f, 16.0f);
            g.setColour (Brand::charcoal().withAlpha (0.85f));
            g.fillRoundedRectangle (pill, 3.0f);
            g.setColour (Brand::white());
            g.drawText (txt, pill.toNearestInt(), juce::Justification::centred);
        }
    }

    drawOrthoSpacingOverlay (g);
}

void RadiationPatternComponent::drawOrthoSpacingOverlay (juce::Graphics& g)
{
    // Ortho's own read-out, and only that: the gap it reports is the gap
    // Ortho is linking, so with Ortho off there is nothing to report.
    if (! ortho_ || ! showInterdistance_)
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
        // The axis Ortho aligns on, because that is the number it sets.
        const float d = (orthoAlign_ == OrthoAlign::Horizontal)
                            ? std::abs (b.x - a.x)
                            : std::abs (b.y - a.y);

        g.setColour (glow.withAlpha (0.85f));
        g.drawLine (sa.x, sa.y, sb.x, sb.y, 1.4f);

        // End ticks square to the line itself, so they read as ticks on any
        // bearing rather than only on the two ortho axes.
        const auto vv = sb - sa;
        const float vlen = juce::jmax (0.001f, vv.getDistanceFromOrigin());
        const juce::Point<float> nn { -vv.y / vlen, vv.x / vlen };
        const float tk = 6.0f;
        g.drawLine (sa.x - nn.x * tk, sa.y - nn.y * tk, sa.x + nn.x * tk, sa.y + nn.y * tk, 1.4f);
        g.drawLine (sb.x - nn.x * tk, sb.y - nn.y * tk, sb.x + nn.x * tk, sb.y + nn.y * tk, 1.4f);

        const juce::String lab = Units::metres ((double) d, 2);
        const float tw = (float) lab.length() * 7.0f + 14.0f;
        const float th = 18.0f;
        auto box = juce::Rectangle<float> (mid.x - tw * 0.5f, mid.y - th * 0.5f - 10.0f, tw, th);
        // Charcoal, not panelDark: this pill sits on the canvas, which is
        // always dark, while panelDark follows the THEME and is plain white in
        // the light one - so the white label below was drawn white on white.
        g.setColour (Brand::charcoal().withAlpha (0.94f));
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
    // Figma "Home Screen 1.1": the Rel. SPL bar is 18x750.66 at x=1813,y=163.65
    // inside a canvas of x=340..1920, y=132..979 - i.e. 31.65px below the canvas
    // top, 89px in from its right edge, with 96px of the canvas height left over
    // for the tick above and the caption below. Expressed here through the
    // window scale so it holds at other window sizes.
    // `bounds` is the gutter to the right of the plot, so anchor the bar to its
    // LEFT edge. Anchoring from the right instead pushed the bar back over the
    // heatmap whenever the gutter was narrower than that offset - which is what
    // happens full-screen, where the plot takes more of the width.
    const int barW = UiConfig::Scale::px (14);                       // -> 18px
    const int barX = bounds.getX() + UiConfig::Scale::px (4);
    const int barY = bounds.getY() + UiConfig::Scale::px (24);       // -> 32px
    const int barH = bounds.getHeight() - UiConfig::Scale::px (73);  // -> 96px
    if (barH < 20) return;

    const auto tickCol = Brand::text();                 // white (dark) / charcoal (light)
    const auto outline = AppSettings::get().isDark()
                             ? juce::Colour (0xffc8c8c8)
                             : Brand::border();
    g.setFont (Brand::techBold (Brand::UI::scaledFont (Brand::Type::legendTickOnScreen)));

    const int tickH = UiConfig::Scale::px (14);
    auto drawTick = [&] (int db, int ty)
    {
        g.setColour (tickCol);
        g.drawText (juce::String (db) + " db",
                    barX + barW + 4, ty - tickH / 2, UiConfig::Scale::px (54), tickH,
                    juce::Justification::centredLeft);
    };

    if (params_.viewMode == ViewMode::SPL && params_.bandedSPL)
    {
        // Fixed contour steps from 0 down to db Floor (never stretch labels).
        const float step = ColourMaps::kContourStepDB;
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
    else // continuous - fixed -6 dB legend ticks; floor is the bottom stop only
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

        // Ticks every -6 dB from 0 down to floor (e.g. 0,-6,...,-36) - never -9.
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
    g.setFont (Brand::techBold (Brand::UI::scaledFont (Brand::Type::legendTickOnScreen)));
    // Figma puts the caption just below the bar, starting ~13px right of its
    // left edge (x=1826 for a bar at 1813) and running to the canvas edge.
    const int capX = barX + UiConfig::Scale::px (10);
    g.drawText ("Rel. SPL", capX, barY + barH + UiConfig::Scale::px (6),
                juce::jmax (10, bounds.getRight() - capX), tickH,
                juce::Justification::centredLeft);
}

// ---------------------------------------------------------------------------
// CLIO-style Atomik polar frame (vector only - never pastes graph images).
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
            // Labels along the forward (+X / 0°) spoke - right side.
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

    // Closed polar stroke in (angle, dB) - dense 0.5° samples, rounded joins.
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
// DIRECTIVITY - CLIO-style Atomik plot of the *simulated* far-field pattern
// (AcousticEngine::polarMag). No measured-unit CSV overlay here - that lives
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
        g.drawText (nEnabled == 0 ? "Enable a unit and press RUN"
                                  : "No polar data - press RUN",
                    bounds.withTrimmedTop (bounds.getHeight() / 2),
                    juce::Justification::centred);
    }

    // Speaker markers (live list - reflects delete / enable immediately).
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
// Measured polar - CLIO-style Atomik plot from real MeasurementIntegrationPack
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
                       + utf8Dot() + Units::metres ((double) measuredDistanceM_, 1);
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
    g.drawText (Units::metres ((double) measuredDistanceM_, 1),
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
        const juce::PathStrokeType stroke (planeStrokeW (a),
                                           juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded);
        float dl[2];
        if (const int n = planeDashPattern (a, planeStrokeW (a), dl); n > 0)
        {
            juce::Path dashed;
            stroke.createDashedStroke (dashed, path, dl, n);
            g.fillPath (dashed);
            return;
        }
        g.strokePath (path, stroke);
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

    // Text Box: ghost under cursor (click to place - no corner rubber-band).
    if (tool_ == Tool::Shape && drawShape_ == DrawShape::TextBox && hoverValid_
        && ! sessionActive_)
    {
        Annotation preview;
        preview.kind = Annotation::Kind::TextBox;
        preview.space = space;
        preview.colour = drawColour_;
        preview.fillAlpha = drawFillAlpha_;
        preview.thicknessPx = 1.5f;
        preview.text = "Text";
        preview.rotationDeg = 0.0f;
        const float sx = juce::jmax (1.0e-3f, worldScaleX());
        const float sy = juce::jmax (1.0e-3f, worldScaleY());
        float halfW = 70.0f / sx;
        float halfH = 28.0f / sy;
        if (currentAnnotSpace() == AnnotSpace::PolarPlot)
        {
            halfW = 0.18f;
            halfH = 0.08f;
        }
        const auto c = snapAnnotPoint (hoverAnnot_);
        preview.pts = { { c.x - halfW, c.y - halfH }, { c.x + halfW, c.y + halfH } };
        drawTextBoxAnnotation (g, preview, 0.75f, true);
    }

    // Rubber-band: Shape session ----------------------------------------------------
    if (tool_ == Tool::Shape && sessionActive_ && ! sessionPts_.empty() && hoverValid_
        && drawShape_ != DrawShape::TextBox)
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
    }

    // Plane tags last, over every shape: a plane's own fill would otherwise
    // paint across the reading that belongs to it.
    for (const auto& a : annotations_)
        if (a.space == space)
            drawPlaneTag (g, a);
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

    // Click away to commit. The editor commits on losing focus, but a click on
    // the plot does not always move focus off it, so a press outside the box
    // being edited ends the edit first and then falls through to do whatever
    // that click would normally have done.
    if (isEditingTextBox() && textEditIndex_ >= 0
        && textEditIndex_ < (int) annotations_.size())
    {
        const auto& edited = annotations_[(size_t) textEditIndex_];
        const float radius = 10.0f / juce::jmax (1.0f, worldScale());
        if (! pointHitsTextBox (screenToAnnot (e.position.x, e.position.y), edited, radius))
            endTextBoxEdit (true);
    }

    // Middle button pans from ANY tool, the way every CAD app behaves -- you
    // should not have to leave the tool you are drawing with to move the view.
    if (e.mods.isMiddleButtonDown())
    {
        drag_ = Drag::Pan;
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        return;
    }

    const auto pb = plotArea();
    // Polar uses full bounds as the plot frame; SPL uses the trimmed plot area.
    const bool inPlot = (currentAnnotSpace() == AnnotSpace::PolarPlot)
                            ? getLocalBounds().contains (e.getPosition())
                            : pb.contains (e.getPosition());
    if (! inPlot) return;

    // In-place text edit: clicks inside the editor stay there; outside commits (Word/PPT).
    if (isEditingTextBox())
    {
        if (textEdit_ != nullptr && textEdit_->getBounds().contains (e.getPosition()))
            return;
        endTextBoxEdit (true);
    }

    if (e.mods.isPopupMenu())
    {
        // Outside the plain cursor tool (Pencil/Ruler/Shapes/Add Mic/Add Q21S,
        // mid-draw or not), right-click mirrors Esc: cancel and return to
        // Select. Normal Select-mode right-click keeps its context menu below.
        const bool inSelectMode = tool_ == Tool::Select && ! addMicArmed_ && ! addSpeakerArmed_;
        if (! inSelectMode)
        {
            if (onRequestCancelCurrentTool)
                onRequestCancelCurrentTool();
            return;
        }

        // Select under the cursor first so Copy/Paste/Delete work without a prior left-click.
        // Already-selected hits keep the current multi-selection (Windows-style).
        auto annotPt = screenToAnnot (e.position.x, e.position.y);
        const float radius = (currentAnnotSpace() == AnnotSpace::PolarPlot)
            ? (10.0f / juce::jmax (1.0f, polarRadius_))
            : (10.0f / juce::jmax (1.0f, worldScale()));

        bool hitSomething = false;
        int speakerUnderCursor = -1;

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

        if (! hitSomething && currentAnnotSpace() == AnnotSpace::World)
        {
            const int sHit = speakerHitTest (e.position);
            if (sHit >= 0)
            {
                hitSomething = true;
                speakerUnderCursor = sHit;
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

        if (! hitSomething)
        {
            int aHit = annotationBorderHitTest (annotPt, radius);
            if (aHit < 0)
                aHit = annotationFillHitTest (annotPt, radius);
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

        juce::ignoreUnused (hitSomething); // empty space keeps current selection (Paste still works)
        showSelectionContextMenu (e.getScreenPosition(), speakerUnderCursor);
        return;
    }

    auto annot = screenToAnnot (e.position.x, e.position.y);

    // Add Q21S armed: place at the click (world heatmap only).
    if (addSpeakerArmed_ && currentAnnotSpace() == AnnotSpace::World)
    {
        const int sHit = speakerHitTest (e.position);
        if (sHit >= 0)
        {
            setAddSpeakerArmed (false);
            selectedAnnots_.clear();
            selectedMics_.clear();
            selectedSpeakers_ = { sHit };
            syncPrimarySelectionFromSets();
            if (onSpeakerSelected) onSpeakerSelected (sHit);
            if (onAnnotSelectionChanged) onAnnotSelectionChanged();
            repaint();
            return;
        }

        ensureWorldExtents();
        auto w = screenToWorld (e.position.x, e.position.y);
        w.x = juce::jlimit (0.0f, (float) result_.worldW, w.x);
        w.y = juce::jlimit (0.0f, (float) result_.worldH, w.y);
        if (drawGridSnap_)
        {
            const float step = (float) Units::snapStepMetres();
            if (step > 1.0e-9f)
            {
                w.x = std::round (w.x / step) * step;
                w.y = std::round (w.y / step) * step;
                w.x = juce::jlimit (0.0f, (float) result_.worldW, w.x);
                w.y = juce::jlimit (0.0f, (float) result_.worldH, w.y);
            }
        }
        if (onPlaceSpeakerAt)
            onPlaceSpeakerAt (w.x, w.y);
        // Stay armed so the next click places another unit (Esc cancels).
        return;
    }

    // Add Mic armed: place on heatmap - but clicking an existing mic selects it.
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
        // A ray is the rotation knob with a target you cannot miss: clicking
        // anywhere along it takes that speaker and starts swinging it. Tested
        // before the normal selection, which would otherwise read the click as
        // empty field and drop the selection instead.
        //
        // A mic wins, though. A ray runs the whole width of the field and will
        // sooner or later lie across something you placed deliberately; when
        // it crosses a mic, the thing under the pointer is the mic, and trying
        // to drag it would silently aim a speaker instead.
        const bool micUnderCursor = micHitTestScreen (e.position) >= 0;
        // A sub's knob is tested first and on the same terms: it is a small
        // target sitting right beside the cabinet, so whatever is under the
        // pointer there is meant to be the knob.
        const int aimIdx = micUnderCursor ? -1 : aimHandleHitTest (e.position);
        if (const int rayIdx = (aimIdx >= 0) ? aimIdx
                                             : (micUnderCursor ? -1 : rayHitTest (e.position));
            rayIdx >= 0)
        {
            if (onWillEdit) onWillEdit();

            // Only reduce the selection when the ray grabbed is OUTSIDE it.
            // Grabbing one of several selected speakers has to keep the group,
            // or aiming a whole array would collapse to aiming one box.
            if (! isSpeakerSelected (rayIdx) && selected_ != rayIdx)
            {
                selectedAnnots_.clear();
                selectedMics_.clear();
                selectedSpeakers_.clear();
                selectedSpeakers_.push_back (rayIdx);
                syncPrimarySelectionFromSets();
                if (onAnnotSelectionChanged) onAnnotSelectionChanged();
                if (onSpeakerSelected) onSpeakerSelected (rayIdx);
            }

            drag_ = Drag::SpeakerRotate;
            rotatingSpeaker_ = rayIdx;
            lastMouse_ = e.position;
            repaint();
            return;
        }

        const bool additive = e.mods.isCommandDown() || e.mods.isCtrlDown();
        const float radius = (currentAnnotSpace() == AnnotSpace::PolarPlot)
            ? (10.0f / juce::jmax (1.0f, polarRadius_))
            : (10.0f / juce::jmax (1.0f, worldScale()));
        const float handleR = radius * 1.35f;

        cycleCandidate_ = -1;

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
                    if (Annotation::isRotatable (a.kind) && h == 4)
                    {
                        drag_ = Drag::AnnotRotate;
                        resizeHandleIndex_ = 4;
                        const auto local = rotatableLocalRect (a);
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

        // Mic hit (world heatmap only) - glyph + label in screen space.
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

        // Speakers win over filled shape interiors (still under covering rectangles).
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

        auto tryAnnotationResize = [&] (int aHit) -> bool
        {
            if (aHit < 0 || additive || selectedAnnots_.size() > 1) return false;
            const int h = resizeHandleHitTest (annotations_[(size_t) aHit], annot, handleR);
            if (h < 0) return false;

            selectedAnnots_ = { aHit };
            selectedMics_.clear();
            selectedSpeakers_.clear();
            syncPrimarySelectionFromSets();
            if (onWillEdit) onWillEdit();
            annotDragMoved_ = false;
            lastAnnotDrag_ = annot;
            lastMouse_ = e.position;
            auto& a = annotations_[(size_t) aHit];
            if (Annotation::isRotatable (a.kind) && h == 4)
            {
                drag_ = Drag::AnnotRotate;
                resizeHandleIndex_ = 4;
                const auto local = rotatableLocalRect (a);
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
            return true;
        };

        auto applyAnnotationHit = [&] (int aHit) -> bool
        {
            if (aHit < 0) return false;
            if (tryAnnotationResize (aHit)) return true;

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
                return true;
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
            else if (selectedAnnots_.size() == 1 && selectedMics_.empty()
                     && selectedSpeakers_.empty())
            {
                // Already the selection: if this click does not turn into a
                // drag, mouseUp steps to whatever is underneath.
                cycleCandidate_ = aHit;
                cycleAnnotPt_   = annot;
                cycleRadius_    = radius;
            }
            beginSelectionMove();
            return true;
        };

        // Shape border / stroke (lines, arcs, rect edges) - pickable even near speakers.
        if (applyAnnotationHit (annotationBorderHitTest (annot, radius)))
            return;

        // Filled interior only when no speaker/mic under the click.
        {
            const int fillHit = annotationFillHitTest (annot, radius);
            // Edit text boxes via double-click only - single click selects / drags.
            if (applyAnnotationHit (fillHit))
                return;
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

    // Pan / layout - SPL world view only
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
        layoutTextBoxEditor();
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
        if (Annotation::isRotatable (a.kind))
        {
            const float mouseDeg = std::atan2 (cur.y - rotateDragCentre_.y,
                                               cur.x - rotateDragCentre_.x)
                                     * 180.0f / (float) M_PI;
            float next = rotateDragStartDeg_ + (mouseDeg - rotateDragStartMouseDeg_);
            // Snap to 15° when Shift is held.
            if (e.mods.isShiftDown())
                next = std::round (next / 15.0f) * 15.0f;
            // Keep continuous 360° rotation (no wrap snap while dragging).
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
            layoutTextBoxEditor();
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
                const auto snap = MicRingSnap::snapToRing (nx, ny, speakers_, activeRangeRings());
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
        // View-only: slides the window over the fixed field, no re-solve, so
        // dragging stays smooth and the prediction underneath never changes.
        // The clamp keeps the view inside the field, which is possible again
        // now that the fit crops rather than exactly filling.
        origin_ += (e.position - lastMouse_);
        noteViewAdjusted();
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
            const float step = (float) Units::snapStepMetres();
            if (step > 1.0e-9f)
            {
                layout_->originM.x = std::round (layout_->originM.x / step) * step;
                layout_->originM.y = std::round (layout_->originM.y / step) * step;
            }
        }
        if (onLayoutMoved) onLayoutMoved();
        repaint();
    }
    else if (drag_ == Drag::SpeakerRotate && rotatingSpeaker_ >= 0
             && rotatingSpeaker_ < (int) speakers_.size())
    {
        auto& spk = speakers_[(size_t) rotatingSpeaker_];
        const auto c = worldToScreen (spk.x, spk.y);
        // Screen Y is inverted, so negate it to get a world-space angle.
        float deg = juce::radiansToDegrees (std::atan2 (-(e.position.y - c.y),
                                                          e.position.x - c.x));
        // Snap to 5 degrees: the value stored is always a multiple, never an
        // in-between angle that merely looks like one.
        deg = 5.0f * std::round (deg / 5.0f);
        while (deg < 0.0f)     deg += 360.0f;
        while (deg >= 360.0f)  deg -= 360.0f;

        if (std::abs (deg - spk.rotationDeg) > 0.01f)
        {
            // The ray under the cursor lands exactly where you point; everything
            // else selected turns by the SAME amount rather than snapping to the
            // same bearing, so a fan keeps its shape instead of collapsing flat.
            const float delta = deg - spk.rotationDeg;
            spk.rotationDeg = deg;
            if (onSpeakerRotated) onSpeakerRotated (rotatingSpeaker_, deg);

            for (int idx : selectedSpeakers_)
            {
                if (idx == rotatingSpeaker_
                    || idx < 0 || idx >= (int) speakers_.size())
                    continue;

                auto& other = speakers_[(size_t) idx];
                float nd = other.rotationDeg + delta;
                while (nd < 0.0f)     nd += 360.0f;
                while (nd >= 360.0f)  nd -= 360.0f;
                other.rotationDeg = nd;
                if (onSpeakerRotated) onSpeakerRotated (idx, nd);
            }
        }
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
    if (drag_ == Drag::Pan)
    {
        drag_ = Drag::None;
        updateMouseCursorForTool();
        repaint();
        return;
    }

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

    // A click on an already-selected drawing that never became a drag means
    // "show me what is under this one". Step one down the stack, wrapping at
    // the bottom, so repeated clicks walk through everything at that point.
    if (cycleCandidate_ >= 0 && drag_ == Drag::SelectionMove && ! annotDragMoved_)
    {
        const auto stack = annotationsUnder (cycleAnnotPt_, cycleRadius_);
        if (stack.size() > 1)
        {
            const auto at = std::find (stack.begin(), stack.end(), cycleCandidate_);
            const size_t next = (at == stack.end())
                                  ? 0
                                  : (size_t) ((at - stack.begin()) + 1) % stack.size();
            selectedAnnots_ = { stack[next] };
            selectedMics_.clear();
            selectedSpeakers_.clear();
            syncPrimarySelectionFromSets();
            if (onAnnotSelectionChanged) onAnnotSelectionChanged();
            if (onMicsChanged) onMicsChanged();
            repaint();
        }
    }
    cycleCandidate_ = -1;

    const bool committed = (drag_ == Drag::Pencil || drag_ == Drag::Erase
                            || drag_ == Drag::Speaker || drag_ == Drag::Layer
                            || (drag_ == Drag::Annot && annotDragMoved_)
                            || (drag_ == Drag::AnnotResize && annotDragMoved_)
                            || (drag_ == Drag::AnnotRotate && annotDragMoved_)
                            || (drag_ == Drag::SelectionMove && annotDragMoved_)
                            || (drag_ == Drag::Mic && micDragMoved_));
    drag_ = Drag::None;
    draggedSpeaker_ = -1;
    rotatingSpeaker_ = -1;
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
    // Scroll-wheel zoom in every tool mode (Select / Shape / Pencil / ...).
    // Skip polar-only views that do not use the world zoom transform.
    if (params_.viewMode == ViewMode::Directivity) return;
    if (params_.viewMode == ViewMode::MeasuredPolar && ! showingBemHeatmap()) return;

    ensureWorldExtents();

    const float factor = (wheel.deltaY > 0 ? 1.1f : 1.0f / 1.1f);
    // Zoom about the pointer so the point under the cursor stays put.
    const float newZoom = juce::jlimit (minZoomForFit(), kMaxZoom, zoom_ * factor);
    if (std::abs (newZoom - zoom_) < 1e-6f) return;
    const auto worldUnder = screenToWorld (e.position.x, e.position.y);
    zoom_ = newZoom;
    const auto pb = plotArea();
    origin_.x = e.position.x - (float) pb.getX() - worldUnder.x * worldScaleX();
    origin_.y = e.position.y - (float) pb.getY()
                  - ((float) result_.worldH - worldUnder.y) * worldScaleY();
    noteViewAdjusted();
    clampViewToField();
    layoutTextBoxEditor();
    repaint();
}

void RadiationPatternComponent::setMagnifier (bool on)
{
    if (magnifierOn_ == on) return;
    magnifierOn_ = on;
    if (! on) { magImage_ = juce::Image(); magValid_ = false; }
    repaint();
}

float RadiationPatternComponent::magnifierRadiusPx() const noexcept
{
    // Big enough to be worth looking through, never so big it covers the plot
    // it is meant to help you read.
    const float want = 78.0f * juce::jmax (0.5f, Brand::UI::scale);
    return juce::jlimit (36.0f, want,
                         0.22f * (float) juce::jmin (getWidth(), getHeight()));
}

void RadiationPatternComponent::updateMagnifierImage()
{
    if (! magnifierOn_ || ! magValid_ || magBusy_) return;
    if (getWidth() < 8 || getHeight() < 8) return;

    const float r = magnifierRadiusPx();
    const int   src = juce::jmax (8, juce::roundToInt (2.0f * r / kMagnifyFactor));

    // Clamped into the component so the glass always has something under it;
    // right at an edge that means the view stops following the pointer rather
    // than filling half the circle with nothing.
    auto area = juce::Rectangle<int> (0, 0, src, src)
                    .withCentre (magPoint_.roundToInt())
                    .constrainedWithin (getLocalBounds());

    magBusy_ = true;      // suppresses the loupe inside its own snapshot
    magImage_ = createComponentSnapshot (area, false, kMagnifyFactor);
    magBusy_ = false;
}

void RadiationPatternComponent::drawMagnifier (juce::Graphics& g)
{
    if (! magImage_.isValid()) return;

    const float r = magnifierRadiusPx();
    const auto  c = magPoint_;

    juce::Path circle;
    circle.addEllipse (c.x - r, c.y - r, 2.0f * r, 2.0f * r);

    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (circle);
        // The snapshot is already at the magnified scale, so it is drawn 1:1
        // and simply centred - no second resample to soften it.
        const float iw = (float) magImage_.getWidth();
        const float ih = (float) magImage_.getHeight();
        g.drawImageTransformed (magImage_,
            juce::AffineTransform::translation (c.x - iw * 0.5f, c.y - ih * 0.5f),
            false);
    }

    // A rim, so the glass reads as an instrument held over the plot rather
    // than as part of the prediction.
    g.setColour (Brand::charcoal().withAlpha (0.85f));
    g.drawEllipse (c.x - r, c.y - r, 2.0f * r, 2.0f * r, 3.0f);
    g.setColour (Brand::white().withAlpha (0.9f));
    g.drawEllipse (c.x - r, c.y - r, 2.0f * r, 2.0f * r, 1.2f);

    // Say how much it is enlarging, or the reading means nothing.
    const float fh = juce::jmax (10.0f, 11.5f * juce::jmax (0.5f, Brand::UI::scale));
    g.setFont (Brand::tech (fh, true));
    const juce::String tag = juce::String (kMagnifyFactor, 1) + "x";
    juce::Rectangle<float> pill (c.x - 18.0f, c.y + r + 4.0f, 36.0f, fh * 1.5f);
    g.setColour (Brand::charcoal().withAlpha (0.85f));
    g.fillRoundedRectangle (pill, 3.0f);
    g.setColour (Brand::white());
    g.drawText (tag, pill.toNearestInt(), juce::Justification::centred, false);
}

void RadiationPatternComponent::mouseMove (const juce::MouseEvent& e)
{
    if (magnifierOn_)
    {
        magPoint_ = e.position;
        magValid_ = true;
        updateMagnifierImage();
        repaint();
    }

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
    if (magValid_) { magValid_ = false; magImage_ = juce::Image(); repaint(); }
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

        // Speakers / mics under a covering text box win over edit-on-double-click.
        if (currentAnnotSpace() == AnnotSpace::World)
        {
            // Double click a mic to listen there. Right-click works too, but
            // this is the gesture you reach for when you are moving a mic
            // around and want to hear each spot.
            if (const int mHit = micHitTestScreen (e.position); mHit >= 0)
            {
                if (onListenAtMic) onListenAtMic (mHit);
                return;
            }
            if (speakerHitTest (e.position) >= 0)
                return;
        }

        int hit = annotationBorderHitTest (annot, radius);
        if (hit < 0)
            hit = annotationFillHitTest (annot, radius);
        if (hit >= 0 && hit < (int) annotations_.size()
            && annotations_[(size_t) hit].kind == Annotation::Kind::TextBox)
        {
            setSelectedAnnotation (hit);
            beginTextBoxEdit (hit, &e.position);
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
            const bool isX = (letter == 'x' || letter == 'X' || code == 'X' || code == 'x');
            const bool isV = (letter == 'v' || letter == 'V' || code == 'V' || code == 'v');
            if (isC)
            {
                copySelection();
                return true;
            }
            if (isX)
            {
                cutSelection();
                return true;
            }
            if (isV)
            {
                pasteClipboard();
                return true;
            }
        }
    }

    // F fits the whole field, Z frames what is selected. Plain letters, so
    // they are dead while a text box is being edited or a draw session is up.
    if (UiConfig::showViewSwitcher && ! isEditingTextBox() && ! sessionActive_)
    {
        const auto mods = juce::ModifierKeys::getCurrentModifiersRealtime();
        if (! mods.isCommandDown() && ! mods.isCtrlDown() && ! mods.isAltDown())
        {
            const auto ch = key.getTextCharacter();
            if (ch == 'f' || ch == 'F') { resetView();       return true; }
            if (ch == 'z' || ch == 'Z') { zoomToSelection(); return true; }
        }
    }

    if (key.isKeyCode (juce::KeyPress::escapeKey))
    {
        // Word/PPT: Esc leaves text edit first; next Esc returns to cursor.
        if (isEditingTextBox())
        {
            endTextBoxEdit (true);
            return true;
        }
        if (addMicArmed_)
            setAddMicArmed (false);
        if (addSpeakerArmed_)
            setAddSpeakerArmed (false);
        cancelDrawSession();
        clearPlotSelection();
        if (tool_ != Tool::Select)
            setTool (Tool::Select);
        return true;
    }

    // Arrow keys nudge whatever is selected - a shape, a mic, a cabinet, or a
    // mixed group - the same way dragging it would, which is why it goes
    // through moveSelectionBy and not a second mover of its own. A drag is
    // fine for putting something roughly in place and hopeless for putting it
    // exactly one step over.
    if (! isEditingTextBox() && ! sessionActive_ && ! pendingAnchor_
        && hasCopyableSelection())
    {
        const bool left  = key.isKeyCode (juce::KeyPress::leftKey);
        const bool right = key.isKeyCode (juce::KeyPress::rightKey);
        const bool up    = key.isKeyCode (juce::KeyPress::upKey);
        const bool down  = key.isKeyCode (juce::KeyPress::downKey);
        if (left || right || up || down)
        {
            // One step is the unit system's own small step - 100 mm, or a foot
            // in Imperial - so a nudge lands on the same grid the snaps use.
            // Shift takes ten of them for crossing the field.
            const float base = Units::imperial() ? 0.3048f : 0.1f;
            const float stepM = base * (key.getModifiers().isShiftDown() ? 10.0f : 1.0f);

            // World Y runs up the plan, screen Y runs down it: Up means away
            // from the viewer, which is +y.
            juce::Point<float> d (left ? -stepM : (right ? stepM : 0.0f),
                                  down ? -stepM : (up    ? stepM : 0.0f));

            if (onWillEdit) onWillEdit();
            const bool world = (currentAnnotSpace() == AnnotSpace::World);
            moveSelectionBy (d, world ? d : juce::Point<float> (0.0f, 0.0f));
            layoutTextBoxEditor();
            if (onEditCommitted) onEditCommitted();
            repaint();
            return true;
        }
    }

    if ((key.isKeyCode (juce::KeyPress::deleteKey)
         || key.isKeyCode (juce::KeyPress::backspaceKey))
        && ! sessionActive_ && ! pendingAnchor_
        && ! isEditingTextBox()
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