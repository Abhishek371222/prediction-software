#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"
#include "AcousticEngine.h"

// =============================================================================
// SourcePanel - the side panel, as the Figma draws it, bound to the scene.
//
// The panel owns NO state. It is told the speakers and which one is selected,
// and hands every edit straight back out through its callbacks, so the old
// sidebar, the plan and the solver stay in step. A second copy of the truth
// here would go stale the first time a speaker was dragged on the plan.
//
// Two things in the Figma have nothing behind them yet, and are shown as
// unavailable rather than as numbers that would be believed:
//   * Rigging Points  - loads need cabinet weights, which the model lacks.
//
// A SOURCE IS AN ARRAY. The Sources list shows one row per array, not per
// cabinet; No. of Cabinets is that array's count; and Rigging Angles lists
// that array's own boxes, each Splay measured from the box above it. The
// grouping lives on Speaker::arrayId - see AcousticEngine.h.
// =============================================================================
class SourcePanel : public juce::Component
{
public:
    //--------------------------------------------------------------- look
    //
    // The SAME values the Venue tab uses. The two panels sit under one strip
    // and are read as one piece of furniture, so they cannot be two families
    // of grey - this one had its own #DBDBDB / #BEBEBE / #E8E8E8 set, a few
    // levels off Venue's at every step, which is exactly the kind of mismatch
    // that reads as sloppiness rather than as a deliberate difference.
    static juce::Colour fieldFill()  { return juce::Colour (0xffd9d9d9); }   // value boxes
    static juce::Colour headFill()   { return juce::Colour (0xffdfdede); }   // section headers
    static juce::Colour headRule()   { return juce::Colour (0xffc8c8c8); }
    static juce::Colour stripeFill() { return juce::Colour (0xfff0f0f0); }   // list rows
    static juce::Colour listRule()   { return juce::Colour (0xffc8c8c8); }
    static juce::Colour rowRule()    { return juce::Colour (0xffc8c8c8); }
    /** The chosen row, as the Venue tab paints it. */
    static juce::Colour rowSelect()  { return Brand::selection(); }

    /** 20 not 20.0, 2.5 as 2.5 - the Figma reads "20 ft" and "5 Deg". */
    static juce::String num (double v)
    {
        return std::abs (v - std::round (v)) < 0.05 ? juce::String ((int) std::lround (v))
                                                    : juce::String (v, 1);
    }

    /** The combo's arrow: 12 x 7, centred 14 px in from the right end. */
    static void drawDropArrow (juce::Graphics& g, int w, int h, bool enabled)
    {
        const float s = UiConfig::Scale::figf (12.0f);
        const float cx = (float) w - UiConfig::Scale::figf (14.0f), cy = (float) h * 0.5f;
        juce::Path t;
        t.addTriangle (cx - s * 0.5f, cy - s * 0.29f,
                       cx + s * 0.5f, cy - s * 0.29f,
                       cx,            cy + s * 0.29f);
        g.setColour (Brand::text().withAlpha (enabled ? 0.85f : 0.35f));
        g.fillPath (t);
    }

    //---------------------------------------------- open dropdown list
    // Figma: a white list in a #CCCCCC hairline, a grey rule under every
    // item, the current choice in the selection blue and no tick; the item
    // under the pointer takes a paler blue. Shared with the Venue panel.
    static juce::Colour listEdge() { return juce::Colour (0xffcccccc); }
    static juce::Colour listSep()  { return juce::Colour (0xffc8c8c8); }

    static void drawListBackground (juce::Graphics& g, int w, int h)
    {
        g.fillAll (Brand::white());
        g.setColour (listEdge());
        g.drawRect (0, 0, w, h, 1);
    }

    static void drawListItem (juce::Graphics& g, juce::Rectangle<int> area, bool isSeparator,
                              bool isActive, bool isHighlighted, bool isTicked,
                              const juce::String& text, const juce::Font& font)
    {
        using UiConfig::Scale::fig;
        if (isSeparator)
        {
            g.setColour (listSep());
            g.fillRect (area.getX(), area.getCentreY(), area.getWidth(), 1);
            return;
        }
        if (isActive && (isTicked || isHighlighted))
        {
            g.setColour (isTicked ? Brand::selection()
                                  : Brand::selection().interpolatedWith (Brand::white(), 0.5f));
            g.fillRect (area);
        }
        g.setColour (listSep());
        g.fillRect (area.getX(), area.getBottom() - 1, area.getWidth(), 1);
        g.setColour (Brand::text().withAlpha (isActive ? 1.0f : 0.4f));
        g.setFont (font);
        g.drawText (text, area.withTrimmedLeft (fig (8.0f)).withTrimmedRight (fig (6.0f)),
                    juce::Justification::centredLeft, true);
    }

    static void listItemSize (const juce::String& text, bool isSeparator, const juce::Font& font,
                              int& idealWidth, int& idealHeight)
    {
        using UiConfig::Scale::fig;
        idealHeight = isSeparator ? fig (7.0f) : fig (26.0f);
        idealWidth  = font.getStringWidth (text) + fig (28.0f);
    }

    class PanelLnF : public juce::LookAndFeel_V4
    {
    public:
        juce::Font getPopupMenuFont() override { return Brand::figMed (12.0f); }

        void drawPopupMenuBackground (juce::Graphics& g, int w, int h) override
        {
            drawListBackground (g, w, h);
        }

        void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                bool isSeparator, bool isActive, bool isHighlighted,
                                bool isTicked, bool, const juce::String& text,
                                const juce::String&, const juce::Drawable*,
                                const juce::Colour*) override
        {
            drawListItem (g, area, isSeparator, isActive, isHighlighted, isTicked,
                          text, getPopupMenuFont());
        }

        void getIdealPopupMenuItemSize (const juce::String& text, bool isSeparator, int,
                                        int& idealWidth, int& idealHeight) override
        {
            listItemSize (text, isSeparator, getPopupMenuFont(), idealWidth, idealHeight);
        }

        void drawComboBox (juce::Graphics& g, int w, int h, bool,
                           int, int, int, int, juce::ComboBox& box) override
        {
            // Figma: a flat #D9D9D9 box, no outline. A row can tint its own
            // combo via backgroundColourId.
            const auto fill = box.isColourSpecified (juce::ComboBox::backgroundColourId)
                                ? box.findColour (juce::ComboBox::backgroundColourId)
                                : fieldFill();
            g.setColour (box.isEnabled() ? fill : fill.withAlpha (0.55f));
            g.fillRect (0, 0, w, h);
            if (box.isColourSpecified (juce::ComboBox::outlineColourId))
            {
                g.setColour (box.findColour (juce::ComboBox::outlineColourId));
                g.drawRect (0, 0, w, h, 1);
            }

            drawDropArrow (g, w, h, box.isEnabled());
        }

        juce::Font getComboBoxFont (juce::ComboBox&) override
        {
            return Brand::figMed (12.0f);
        }

        void positionComboBoxText (juce::ComboBox& box, juce::Label& l) override
        {
            // Figma: the value is 12 px in, clear of the 28 px arrow cell.
            using UiConfig::Scale::fig;
            l.setBorderSize ({});
            l.setBounds (fig (12.0f), 0, box.getWidth() - fig (12.0f) - fig (28.0f), box.getHeight());
            l.setFont (getComboBoxFont (box));
            l.setColour (juce::Label::textColourId,
                         Brand::text().withMultipliedAlpha (box.isEnabled() ? 1.0f : 0.45f));
        }
    };

    //-------------------------------------------------------- small pieces
    class SectionHead : public juce::Component
    {
    public:
        SectionHead (const juce::String& title, bool withPlus)
            : title_ (title), hasPlus_ (withPlus) {}

        void setTitle (const juce::String& t)
        {
            if (title_ != t) { title_ = t; repaint(); }
        }

        std::function<void()> onPlus;
        /** Set to make the arrow fold the section; it is told the new state. */
        std::function<void (bool folded)> onFold;

        bool isFolded() const { return folded_; }

        // Figma: the plus is centred in the 44 px cell at x 308..352, the fold
        // arrow in 352..396 - 66 and 22 px in from the right edge.
        void mouseUp (const juce::MouseEvent& e) override
        {
            using UiConfig::Scale::fig;
            if (hasPlus_ && onPlus != nullptr
                && e.x > getWidth() - fig (88.0f)
                && e.x < getWidth() - fig (44.0f))
                onPlus();
            else if (onFold != nullptr && e.x >= getWidth() - fig (44.0f))
            {
                folded_ = ! folded_;
                repaint();
                onFold (folded_);
            }
        }

        void paint (juce::Graphics& g) override
        {
            using UiConfig::Scale::fig;
            using UiConfig::Scale::figf;
            g.fillAll (headFill());
            g.setColour (headRule());
            g.drawHorizontalLine (0, 0.0f, (float) getWidth());
            g.drawHorizontalLine (getHeight() - 1, 0.0f, (float) getWidth());
            g.setColour (Brand::text());
            g.setFont (Brand::figMed (16.0f));
            g.drawText (title_, getLocalBounds().withTrimmedLeft (fig (14.0f))
                                                .withTrimmedRight (fig (92.0f)),
                        juce::Justification::centredLeft, true);

            // Down when open; a folded section points right, at what it hides.
            const float h = (float) getHeight();
            float x = (float) getWidth() - figf (22.0f);
            const float tw = figf (14.0f), th = figf (8.0f);
            juce::Path p;
            p.addTriangle (x - tw * 0.5f, h * 0.5f - th * 0.5f,
                           x + tw * 0.5f, h * 0.5f - th * 0.5f,
                           x,             h * 0.5f + th * 0.5f);
            if (folded_)
                p.applyTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi,
                                                                   x, h * 0.5f));
            g.fillPath (p);
            if (hasPlus_)
            {
                x = (float) getWidth() - figf (66.0f);
                const float a = figf (15.0f), t = juce::jmax (1.2f, figf (2.0f));
                g.fillRect (x - t * 0.5f, h * 0.5f - a * 0.5f, t, a);
                g.fillRect (x - a * 0.5f, h * 0.5f - t * 0.5f, a, t);
            }
        }

    private:
        juce::String title_;
        bool         hasPlus_;
        bool         folded_ = false;
    };

    // A flat grey field with its unit inside, right-aligned. Click, type, Enter.
    class FieldBox : public juce::Label
    {
    public:
        FieldBox (const juce::String& value, const juce::String& unit = {})
            : unit_ (unit)
        {
            setText (value, juce::dontSendNotification);
            setEditable (true, true, false);
            setColour (backgroundWhenEditingColourId, juce::Colours::transparentBlack);
            setColour (outlineWhenEditingColourId,    juce::Colours::transparentBlack);
            onTextChange = [this] { if (onCommit) onCommit (getText().getDoubleValue()); };
        }

        std::function<void (double)> onCommit;

        /** Figma type size and left inset for the value. Pos. fields are 12 px
            at 10 in; the narrow rotation boxes carry 8 px at 6 in. */
        void setStyle (float figFontPx, float figInset)
        {
            fontPx_ = figFontPx;
            inset_  = figInset;
            repaint();
        }

        /** The suffix was captured once when the box was built, so switching
            to Imperial left every field still reading "m". It is settable. */
        void setUnit (const juce::String& u)
        {
            if (unit_ == u) return;
            unit_ = u;
            repaint();
        }

        void setValue (const juce::String& v)
        {
            if (! isBeingEdited() && getText() != v)
                setText (v, juce::dontSendNotification);
        }

        /** "2 Deg." read as one string after the value, rather than the unit
            pinned to the right edge; with @p arrow the box wears the combo's
            arrow, as the Figma's aim and angle boxes do. */
        void setInlineUnit (bool inlineUnit, bool arrow)
        {
            inline_ = inlineUnit; arrow_ = arrow;
            repaint();
        }

        void paint (juce::Graphics& g) override
        {
            g.setColour (isEnabled() ? fieldFill() : fieldFill().withAlpha (0.55f));
            g.fillRect (getLocalBounds());
            if (isBeingEdited()) return;

            using UiConfig::Scale::fig;
            const auto f = Brand::figMed (fontPx_);
            g.setFont (f);
            g.setColour (Brand::text().withMultipliedAlpha (isEnabled() ? 1.0f : 0.45f));
            if (inline_)
            {
                const int arrowW = arrow_ ? fig (24.0f) : 0;
                g.drawText (getText() + (unit_.isEmpty() ? juce::String() : " " + unit_),
                            getLocalBounds().withTrimmedLeft (fig (inset_)).withTrimmedRight (arrowW),
                            juce::Justification::centredLeft, true);
                if (arrow_) drawDropArrow (g, getWidth(), getHeight(), isEnabled());
                return;
            }

            // Figma: value and unit in the same Medium black; the unit's right
            // edge is 10 px in from the box's.
            const int unitW = unit_.isEmpty() ? 0 : f.getStringWidth (unit_) + fig (14.0f);
            g.drawText (getText(), getLocalBounds().withTrimmedLeft (fig (inset_))
                                                   .withTrimmedRight (juce::jmax (fig (2.0f), unitW)),
                        juce::Justification::centredLeft, true);
            if (unit_.isNotEmpty())
                g.drawText (unit_, getLocalBounds().withTrimmedRight (fig (10.0f)),
                            juce::Justification::centredRight, false);
        }

        void editorShown (juce::TextEditor* ed) override
        {
            // setFont / textColourId only reach text typed afterwards; the value
            // already in the box needs the apply*ToAllText calls.
            ed->setFont (Brand::figMed (fontPx_));
            ed->applyFontToAllText (Brand::figMed (fontPx_));
            ed->setColour (juce::TextEditor::textColourId, Brand::text());
            ed->applyColourToAllText (Brand::text());
            ed->setColour (juce::TextEditor::highlightColourId, Brand::accent().withAlpha (0.30f));
            ed->setColour (juce::TextEditor::highlightedTextColourId, Brand::text());
            ed->setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
            ed->setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
            ed->setBounds (getLocalBounds().withTrimmedLeft (juce::jmax (0, UiConfig::Scale::fig (inset_) - 2)));
        }

    private:
        juce::String unit_;
        float fontPx_ = 12.0f;
        float inset_  = 10.0f;
        bool  inline_ = false, arrow_ = false;
    };

    // M and O-slash. They latch, and they show it - a marker that does
    // something has to look different once it has, or a muted source reads
    // exactly like a live one.
    class SquareMark : public juce::Button
    {
    public:
        explicit SquareMark (const juce::String& glyph)
            : juce::Button (glyph), glyph_ (glyph) { setClickingTogglesState (true); }

        void paintButton (juce::Graphics& g, bool hl, bool) override
        {
            // Figma: a flat grey square like the value boxes; latched, red.
            const auto r = getLocalBounds().toFloat();
            const bool on = getToggleState();
            g.setColour (on ? Brand::accent()
                            : (hl ? fieldFill().darker (0.06f) : fieldFill()));
            g.fillRect (r);
            g.setColour (on ? Brand::onAccent()
                            : Brand::text().withMultipliedAlpha (isEnabled() ? 1.0f : 0.45f));
            g.setFont (Brand::figMed (12.0f));
            g.drawText (glyph_, getLocalBounds(), juce::Justification::centred, false);
        }

    private:
        juce::String glyph_;
    };

    class LoadBar : public juce::Component
    {
    public:
        /** @p share of the rated load, 1 = full; negative when no figure is
            available (cabinets carry no weights yet) - the rig is then taken
            as okay and no percentage is shown. */
        void setLoad (float share)
        {
            share_ = share;
            if (share < 0.0f)      { state_ = "LOAD OKAY"; shareText_ = {}; }
            else
            {
                state_ = share > 1.0f ? "LOAD NOT OKAY" : "LOAD OKAY";
                shareText_ = juce::String (juce::roundToInt (share * 100.0f)) + "% of Total Load";
            }
            repaint();
        }

        void paint (juce::Graphics& g) override
        {
            // Figma: black while the rig is within its rating, red once it
            // is over.
            g.fillAll (share_ > 1.0f ? juce::Colour (0xffed2227) : juce::Colour (0xff000000));
            // Figma: the state in Medium 16 at x 14, the share in Medium 12
            // from x 285.
            using UiConfig::Scale::fig;
            g.setColour (Brand::white());
            g.setFont (Brand::figMed (16.0f));
            g.drawText (state_, getLocalBounds().withTrimmedLeft (fig (14.0f))
                                                .withWidth (fig (265.0f)),
                        juce::Justification::centredLeft, true);
            const auto shareFont = Brand::figMed (12.0f);
            g.setFont (shareFont);
            const int right = getWidth() - fig (8.0f);
            const int left  = juce::jmin (fig (285.0f), right - shareFont.getStringWidth (shareText_));
            g.drawText (shareText_, juce::Rectangle<int> (left, 0, right - left, getHeight()),
                        juce::Justification::centredLeft, true);
        }

    private:
        juce::String state_     = "LOAD OKAY";
        juce::String shareText_;
        float        share_     = -1.0f;
    };

    //-------------------------------------------------------------- panel
    SourcePanel() { setLookAndFeel (&lnf_); buildFixed(); }
    ~SourcePanel() override { setLookAndFeel (nullptr); }

    // --- what the panel tells the scene -----------------------------------
    std::function<void (int index)>                   onSelect;
    std::function<void (int index, float x, float y)> onPosition;
    std::function<void (int index, float deg)>        onRotation;
    std::function<void (int index, float deg)>        onTilt;
    std::function<void (int index, float metres)>     onHeight;
    std::function<void (int index, int model)>        onModel;
    /** The selected source's cabinet count changed. */
    std::function<void (int sourceIndex, int count)>  onCabinetCount;
    std::function<void (int index, bool on)>          onEnabled;
    std::function<void (int index, bool inverted)>    onPolarity;
    std::function<void()>                             onAddSource;
    std::function<void (int index)>                   onDeleteSource;
    // Listening planes share the Sources list with the cabinets: a
    // plane is a thing you add from the same "+", so it belongs in the
    // same list rather than in a second one nobody thinks to look at.
    std::function<void (int planeIndex)>              onSelectPlane;
    std::function<void (int planeIndex)>              onDeletePlane;

    /** Rebuild from the scene. Cheap enough for every change. */
    void setScene (const std::vector<Speaker>& sp, int selected)
    {
        scene_ = sp;
        wireCabinetCount();
        if (! rowsMatch (sp))
            buildRows (sp);
        selected_ = juce::jlimit (-1, (int) sp.size() - 1, selected);
        pushValues (sp);
        resized();
        repaint();
    }

    /** The listening planes, newest last, and which one is selected.

        Passed separately from the cabinets because they are a different kind
        of thing with a different index space - plane 0 is not speaker 0. */
    void setPlanes (const juce::StringArray& names, int selectedPlane)
    {
        if (! planesMatch (names))
            buildPlaneRows (names);
        selPlane_ = juce::jlimit (-1, names.size() - 1, selectedPlane);
        for (size_t i = 0; i < planes_.size(); ++i)
            planes_[i]->del->setVisible ((int) i == selPlane_);
        resized();
        repaint();
    }

    /** Re-stamp every unit suffix and re-read the values in the new unit.
        Called when AppSettings broadcasts a change. */
    void refreshUnits()
    {
        const juce::String u = Units::lengthUnit();
        for (auto& f : posXYZ_)
            if (f != nullptr) f->setUnit (u);
        resized();
        repaint();
    }

    int getContentHeight() const { return contentH_; }

    void mouseDown (const juce::MouseEvent& ev) override
    {
        const auto e = ev.getEventRelativeTo (this);

        // A click on a row's own dropdown or M selects that row as well, so
        // the sections below follow whichever source was just touched.
        if (ev.eventComponent != this)
        {
            if (! e.mods.isPopupMenu())
                for (size_t i = 0; i < sourceRowY_.size(); ++i)
                    if (sourceRowY_[i].contains (e.y) && (int) i != selected_)
                    {
                        if (onSelect) onSelect ((int) i);
                        break;
                    }
            return;
        }

        if (! e.mods.isPopupMenu()) return;

        // Right-click a row: select it, so the menu plainly names what goes.
        auto rowAt = [&e] (const std::vector<juce::Range<int>>& rows)
        {
            for (size_t i = 0; i < rows.size(); ++i)
                if (rows[i].contains (e.y)) return (int) i;
            return -1;
        };

        if (const int i = rowAt (sourceRowY_); i >= 0)
        {
            if (onSelect) onSelect (i);
            juce::PopupMenu m;
            m.addItem (1, "Delete " + sources_[(size_t) i]->name, onDeleteSource != nullptr);
            m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea ({ e.getScreenX(), e.getScreenY(), 1, 1 }),
                             [safe = juce::Component::SafePointer<SourcePanel> (this), i] (int r)
                             {
                                 if (r == 1 && safe != nullptr && safe->onDeleteSource)
                                     safe->onDeleteSource (i);
                             });
        }
        else if (const int p = rowAt (planeRowY_); p >= 0)
        {
            if (onSelectPlane) onSelectPlane (p);
            juce::PopupMenu m;
            m.addItem (1, "Delete " + planes_[(size_t) p]->name, onDeletePlane != nullptr);
            m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea ({ e.getScreenX(), e.getScreenY(), 1, 1 }),
                             [safe = juce::Component::SafePointer<SourcePanel> (this), p] (int r)
                             {
                                 if (r == 1 && safe != nullptr && safe->onDeletePlane)
                                     safe->onDeletePlane (p);
                             });
        }
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu() || e.eventComponent != this) return;

        // Clicking a source row selects it, as clicking a unit on the plan does.
        for (size_t i = 0; i < sourceRowY_.size(); ++i)
            if (e.y >= sourceRowY_[i].getStart() && e.y < sourceRowY_[i].getEnd())
            {
                if (onSelect) onSelect ((int) i);
                return;
            }

        // A plane row answers to the same click. Selecting it is what puts its
        // Delete within reach - the button only appears on the selected row,
        // so a plane cannot be removed by a stray click on a row you were only
        // pointing at.
        for (size_t i = 0; i < planeRowY_.size(); ++i)
            if (e.y >= planeRowY_[i].getStart() && e.y < planeRowY_[i].getEnd())
            {
                if (onSelectPlane) onSelectPlane ((int) i);
                return;
            }
    }

    void paint (juce::Graphics& g) override
    {
        // Figma: a white sheet; the source list and alternate positioning
        // rows are banded #E8E8E8.
        g.fillAll (Brand::white());
        g.setColour (stripeFill());
        for (const auto& b : bands_)
            g.fillRect (0, b.getStart(), getWidth(), b.getLength());

        // The selected source carries a tint and a red edge, so the two
        // sections headed "(name)" below have something to point back at.
        //
        // Suppressed while a plane is selected. The cabinet selection is still
        // live underneath - the positioning and rigging sections go on editing
        // it - but showing both marks at once reads as two selected rows, and
        // only one of them is what the click just chose.
        if (selPlane_ < 0 && selected_ >= 0 && (size_t) selected_ < sourceRowY_.size())
        {
            const auto r = sourceRowY_[(size_t) selected_];
            // The selection blue, flat across the row, as the Venue list
            // draws it. No red edge.
            g.setColour (rowSelect());
            g.fillRect (0, r.getStart(), getWidth(), r.getLength());
        }

        // A selected plane is marked the same way a selected cabinet is, so
        // the one list reads as one list.
        if (selPlane_ >= 0 && (size_t) selPlane_ < planeRowY_.size())
        {
            const auto r = planeRowY_[(size_t) selPlane_];
            // The selection blue, flat across the row, as the Venue list
            // draws it. No red edge.
            g.setColour (rowSelect());
            g.fillRect (0, r.getStart(), getWidth(), r.getLength());
        }

        g.setColour (listRule());
        for (int y : listRules_)
            g.drawHorizontalLine (y, 0.0f, (float) getWidth());
        g.setColour (rowRule());
        for (int y : rowRules_)
            g.drawHorizontalLine (y, 0.0f, (float) getWidth());
        for (const auto& v : vRules_)
            g.drawVerticalLine (v.getX(), (float) v.getY(), (float) v.getBottom());

        // Sizes are Figma px; a "\n" in a caption is the Figma's own line break.
        for (const auto& t : texts_)
        {
            g.setFont (t.bold ? Brand::figSemi (t.size) : Brand::figMed (t.size));
            g.setColour (t.dim ? Brand::text().withAlpha (0.62f) : Brand::text());
            g.drawFittedText (t.text, t.area, t.just, 2, 1.0f);
        }

        // Only when the list is genuinely empty - a plane is a row in it too.
        if (sources_.empty() && planes_.empty() && ! headSrc_->isFolded())
        {
            g.setColour (Brand::text().withAlpha (0.55f));
            g.setFont (Brand::figMed (12.0f));
            g.drawText ("No sources yet - add a cabinet to begin.",
                        emptyArea_, juce::Justification::centred, true);
        }
    }

    void resized() override
    {
        texts_.clear();
        rowRules_.clear();
        listRules_.clear();
        bands_.clear();
        sourceRowY_.clear();
        planeRowY_.clear();
        vRules_.clear();

        // Every number below is a Figma pixel from the SOURCE frame (panel
        // x 0..396), mapped onto the window by fig(). Figma V1.1: 30 px
        // section heads, rows of 42 with 26 px controls 8 px down them, and
        // a 27 px white gap closing each section.
        using UiConfig::Scale::fig;
        const int W    = getWidth();
        const int hdrH = fig (30.0f);
        const int rowH = fig (42.0f);
        const int ctlH = fig (26.0f);
        const int ctlY = fig (8.0f);
        const int gapH = fig (27.0f);
        int y = 0;

        auto label = [this] (const juce::String& t, juce::Rectangle<int> a,
                             float size = 12.0f, bool bold = false, bool dim = false)
        { texts_.push_back ({ t, a, size, bold, dim, juce::Justification::centredLeft }); };
        auto caption = [this] (const juce::String& t, juce::Rectangle<int> a)
        { texts_.push_back ({ t, a, 10.0f, false, false, juce::Justification::centred }); };
        auto ctl = [&] (juce::Component& c, float x, float w)
        { c.setBounds (fig (x), y + ctlY, fig (w), ctlH); };
        auto head = [&] (SectionHead& h)
        { h.setBounds (0, y, W, hdrH); y += hdrH; return ! h.isFolded(); };
        auto row = [&] (bool striped)
        {
            if (striped) bands_.push_back ({ y, y + rowH });
            y += rowH;
            rowRules_.push_back (y - 1);
        };
        auto gap = [&] { y += gapH; };
        auto show = [] (bool on, std::initializer_list<juce::Component*> cs)
        { for (auto* c : cs) if (c != nullptr) c->setVisible (on); };

        // ------------------------------------------------------- Sources
        // Name at 12, "System" at 178, the combo 242..351, the mute 362..389.
        //
        // No header of its own. The view strip directly above this panel IS
        // the Sources header - same word, same plus, same chevron - so
        // drawing one here put "Sources" on the screen twice, one bar under
        // the other. The Venue panel already works this way; this is the same
        // fix applied to the tab it was copied from.
        //
        // headSrc_ stays alive and keeps carrying the fold state and the
        // onPlus callback; it is simply given no room and no visibility.
        headSrc_->setBounds (0, y, 0, 0);
        headSrc_->setVisible (false);
        const bool srcOpen = ! headSrc_->isFolded();
        // One row per ARRAY. A cabinet that is not the head of its array is a
        // box inside a source, not a source, and belongs in Rigging Angles.
        for (size_t i = 0; i < sources_.size(); ++i)
            show (srcOpen && isHead ((int) i),
                  { sources_[i]->system.get(), sources_[i]->mute.get() });
        for (size_t i = 0; i < planes_.size(); ++i)
            planes_[i]->del->setVisible (srcOpen && (int) i == selPlane_);
        if (srcOpen)
        {
            const int listTop = y;
            for (size_t i = 0; i < sources_.size(); ++i)
            {
                if (! isHead ((int) i)) { sourceRowY_.push_back ({ y, y }); continue; }
                auto& s = sources_[i];
                const int n = (int) arrayMembers (scene_, (int) i).size();
                // A source carries its cabinet count, as ArrayCalc's list does.
                label (s->name + (n > 1 ? "  (" + juce::String (n) + ")" : juce::String()),
                       { fig (12.0f), y, fig (160.0f), rowH });
                label ("System", { fig (178.0f), y, fig (60.0f), rowH });
                ctl (*s->system, 242.0f, 109.0f);
                ctl (*s->mute,   362.0f, 27.0f);
                sourceRowY_.push_back ({ y, y + rowH });
                y += rowH;
                listRules_.push_back (y - 1);
            }
            for (auto& p : planes_)
            {
                label (p->name, { fig (12.0f), y, fig (160.0f), rowH });
                label ("Listening plane", { fig (178.0f), y, fig (120.0f), rowH });
                ctl (*p->del, 300.0f, 84.0f);
                planeRowY_.push_back ({ y, y + rowH });
                y += rowH;
                listRules_.push_back (y - 1);
            }
            if (sources_.empty() && planes_.empty())
            {
                emptyArea_ = { 0, y, W, rowH };
                y += rowH;
            }
            bands_.push_back ({ listTop, y });
            if (! listRules_.empty())
            {
                listRules_.pop_back();          // the list's last edge is a row rule
                rowRules_.push_back (y - 1);
            }
            gap();
        }

        // --------------------------------------------- Source Positioning
        const bool posOpen = head (*headPos_);
        show (posOpen, { posSystem_.get(), posMounting_.get(), posCabinets_.get(),
                         posHorAim_.get(), posVerAim_.get(),
                         posXYZ_[0].get(), posXYZ_[1].get(), posXYZ_[2].get() });
        if (posOpen)
        {
            // Labels at 12 and 202; controls 79..180 and 281..384.
            label ("SYSTEM",   { fig (12.0f),  y, fig (66.0f), rowH });
            ctl (*posSystem_,   79.0f, 101.0f);
            label ("MOUNTING", { fig (202.0f), y, fig (78.0f), rowH });
            ctl (*posMounting_, 281.0f, 103.0f);
            row (true);

            label ("No. of\nCABINETS", { fig (12.0f), y, fig (66.0f), rowH }, 10.0f);
            ctl (*posCabinets_, 79.0f, 101.0f);
            row (false);

            label ("Hor.Aim", { fig (12.0f),  y, fig (66.0f), rowH });
            ctl (*posHorAim_, 79.0f, 101.0f);
            label ("Ver.Aim", { fig (202.0f), y, fig (78.0f), rowH });
            ctl (*posVerAim_, 281.0f, 103.0f);
            row (true);

            // Three 132-wide cells, 48 tall: caption 2 px down, field 18 down.
            const int posH = fig (48.0f);
            const char* caps[] = { "Pos. X", "Pos. Y", "Pos. Z" };
            for (int i = 0; i < 3; ++i)
            {
                const float cx = 132.0f * (float) i;
                label (caps[i], { fig (cx + 12.0f), y + fig (2.0f), fig (106.0f), fig (15.0f) }, 10.0f);
                posXYZ_[i]->setBounds (fig (cx + 12.0f), y + fig (18.0f), fig (106.0f), ctlH);
            }
            vRules_.push_back ({ fig (132.0f), y, 1, posH });
            vRules_.push_back ({ fig (264.0f), y, 1, posH });
            y += posH;
            rowRules_.push_back (y - 1);
            gap();
        }

        // ------------------------------------------------- Rigging Points
        const bool rigOpen = head (*headRig_);
        show (rigOpen, { loadBar_.get(), frontPick_.get(), rearPick_.get(), singlePick_.get() });
        if (rigOpen)
        {
            loadBar_->setBounds (0, y, W, rowH); y += rowH;

            label ("FRONT PICK", { fig (12.0f),  y, fig (82.0f), rowH });
            ctl (*frontPick_,  96.0f, 100.0f);
            label ("REAR PICK",  { fig (210.0f), y, fig (74.0f), rowH });
            ctl (*rearPick_,  286.0f, 98.0f);
            row (false);

            label ("SINGLE PICK", { fig (12.0f), y, fig (82.0f), rowH });
            ctl (*singlePick_, 96.0f, 100.0f);
            row (false);
            gap();
        }

        // ------------------------------------------------- Rigging Angles
        const bool angOpen = head (*headAng_);
        // Only the selected source's own boxes. The header names one source, so
        // listing the whole scene under it was the table contradicting its own
        // title.
        for (size_t i = 0; i < angles_.size(); ++i)
            show (angOpen && inSelectedArray ((int) i),
                  { angles_[i]->ver.get(), angles_[i]->hor.get(), angles_[i]->pol.get(),
                    angles_[i]->mute.get(), angles_[i]->splay.get() });
        if (angOpen)
        {
            // Each caption centred over its column: Ver. 121..175, Hor.
            // 186..239, Pol 249..277, Mute 286..311, Angles 322..384.
            label ("Cab no.", { fig (12.0f), y, fig (80.0f), rowH }, 10.0f);
            const struct { const char* text; float x, w; } caps[] = {
                { "Ver. Rot.", 121.0f, 54.0f }, { "Hor. Rot.", 186.0f, 53.0f },
                { "Pol inv.",  249.0f, 28.0f }, { "Mute", 286.0f, 25.0f },
                { "Angles",    322.0f, 62.0f } };
            for (const auto& c : caps)
            {
                const float cx = c.x + c.w * 0.5f;
                caption (c.text, { fig (cx - 30.0f), y, fig (60.0f), rowH });
            }
            row (false);

            int cabNo = 0;
            for (size_t i = 0; i < angles_.size(); ++i)
            {
                if (! inSelectedArray ((int) i)) continue;
                auto& a = angles_[i];
                // Numbered within the ARRAY - cabinet 1 is its top box.
                a->num = juce::String (++cabNo);
                label (a->num + ". " + a->cab, { fig (18.0f), y, fig (98.0f), rowH });
                ctl (*a->ver,   121.0f, 54.0f);
                ctl (*a->hor,   186.0f, 53.0f);
                ctl (*a->pol,   249.0f, 28.0f);
                ctl (*a->mute,  286.0f, 25.0f);
                ctl (*a->splay, 322.0f, 62.0f);
                row (false);
            }
            gap();
        }

        contentH_ = y;
    }

private:
    struct SourceRow
    {
        juce::String name;
        int          model = 0;
        std::unique_ptr<juce::ComboBox> system;
        std::unique_ptr<SquareMark>     mute;
    };
    struct PlaneRow
    {
        juce::String name;
        std::unique_ptr<juce::TextButton> del;
    };
    struct AngleRow
    {
        int spIndex = -1;              // the cabinet this row edits
        juce::String num, cab;
        std::unique_ptr<FieldBox>   ver, hor, splay;
        std::unique_ptr<SquareMark> pol, mute;
    };
    struct Text
    {
        juce::String text; juce::Rectangle<int> area;
        float size; bool bold; bool dim; juce::Justification just;
    };

    static juce::String modelName (int m)
    { return m == 0 ? "Q21S" : m == 3 ? "Q212P" : m == 2 ? "BEM 2inch" : "Unit"; }
    static int modelFromItem (int id) { return id == 1 ? 3 : id == 2 ? 0 : 2; }
    static int itemFromModel (int m)  { return m == 3 ? 1 : m == 0 ? 2 : 3; }

    void buildFixed()
    {
        headSrc_ = std::make_unique<SectionHead> ("Sources", true);
        // No plus: this section EDITS the selected source, it does not make
        // one. Adding is the Sources header's job, one strip above.
        headPos_ = std::make_unique<SectionHead> ("Source Positioning", false);
        headRig_ = std::make_unique<SectionHead> ("Rigging Points", false);
        headAng_ = std::make_unique<SectionHead> ("Rigging Angles", false);
        headSrc_->onPlus = [this] { if (onAddSource) onAddSource(); };
        headPos_->onPlus = [this] { if (onAddSource) onAddSource(); };
        for (auto* h : { headSrc_.get(), headPos_.get(), headRig_.get(), headAng_.get() })
        {
            h->onFold = [this] (bool) { refold(); };
            addAndMakeVisible (*h);
        }

        auto combo = [this] (std::unique_ptr<juce::ComboBox>& c,
                             std::initializer_list<const char*> items)
        {
            c = std::make_unique<juce::ComboBox>();
            int id = 1;
            for (auto* it : items) c->addItem (it, id++);
            addAndMakeVisible (*c);
        };
        combo (posSystem_,   { "Q212P", "Q21S", "BEM 2inch" });
        combo (posMounting_, { "Ground", "Flown" });
        combo (posCabinets_, { "1" });
        combo (frontPick_,   { "1", "2", "3" });
        combo (rearPick_,    { "1", "2", "3" });
        combo (singlePick_,  { "0 Deg.", "2 Deg.", "5 Deg." });

        posSystem_->onChange = [this]
        {
            if (! building_ && onModel && selected_ >= 0)
                onModel (selected_, modelFromItem (posSystem_->getSelectedId()));
        };
        posMounting_->onChange = [this]
        {
            if (building_ || onHeight == nullptr || selected_ < 0) return;
            // "Flown" needs a height to be flown AT, so it takes the last one
            // this source had, or a sensible default - rather than staying 0
            // and quietly meaning "ground" anyway.
            onHeight (selected_, posMounting_->getSelectedId() == 1
                                     ? 0.0f : (lastHeight_ > 0.01f ? lastHeight_ : 6.0f));
        };

        posCabinets_->setSelectedId (1, juce::dontSendNotification);
        posCabinets_->setEnabled (false);
        posCabinets_->setTooltip ("Grouped arrays are not modelled yet - one source is one cabinet.");

        for (auto* c : { frontPick_.get(), rearPick_.get(), singlePick_.get() })
        {
            c->setEnabled (false);
            c->setTooltip ("Rigging loads need cabinet weights, which the model does not carry yet.");
        }
        frontPick_->setSelectedId (1, juce::dontSendNotification);
        rearPick_->setSelectedId (2, juce::dontSendNotification);
        singlePick_->setSelectedId (1, juce::dontSendNotification);

        loadBar_ = std::make_unique<LoadBar>();
        addAndMakeVisible (*loadBar_);

        for (int i = 0; i < 3; ++i)
        {
            posXYZ_[i] = std::make_unique<FieldBox> ("0", Units::lengthUnit());
            posXYZ_[i]->onCommit = [this, i] (double v) { commitPosition (i, v); };
            addAndMakeVisible (*posXYZ_[i]);
        }
        posHorAim_ = std::make_unique<FieldBox> ("0", "Deg.");
        posVerAim_ = std::make_unique<FieldBox> ("0", "Deg.");
        posHorAim_->setInlineUnit (true, true);
        posVerAim_->setInlineUnit (true, true);
        posHorAim_->onCommit = [this] (double v)
        { if (! building_ && onRotation && selected_ >= 0) onRotation (selected_, (float) v); };
        posVerAim_->onCommit = [this] (double v)
        { if (! building_ && onTilt && selected_ >= 0) onTilt (selected_, (float) v); };
        addAndMakeVisible (*posHorAim_);
        addAndMakeVisible (*posVerAim_);
    }

    /** The cabinet-count box, filled from the selected array and able to
        change it. It used to be greyed at 1 because a source was one box. */
    void wireCabinetCount()
    {
        if (posCabinets_ == nullptr) return;
        if (posCabinets_->getNumItems() == 0)
            for (int n = 1; n <= 24; ++n) posCabinets_->addItem (juce::String (n), n);

        if (posCabinets_->onChange == nullptr)
            posCabinets_->onChange = [this]
            {
                if (building_ || onCabinetCount == nullptr || selected_ < 0) return;
                onCabinetCount (selected_, posCabinets_->getSelectedId());
            };

        const bool has = selected_ >= 0 && selected_ < (int) scene_.size();
        posCabinets_->setEnabled (has);
        if (has)
        {
            building_ = true;
            posCabinets_->setSelectedId (
                juce::jlimit (1, 24, (int) arrayMembers (scene_, selected_).size()),
                juce::dontSendNotification);
            building_ = false;
        }
    }

    /** Is this row's cabinet the HEAD of its array? Only heads are sources. */
    bool isHead (int i) const
    {
        if (i < 0 || i >= (int) scene_.size()) return false;
        const int id = scene_[(size_t) i].arrayId;
        if (id < 0) return true;                       // ungrouped stands alone
        for (int k = 0; k < i; ++k)
            if (scene_[(size_t) k].arrayId == id) return false;
        return true;
    }

    /** Does this cabinet belong to the selected source's array? */
    bool inSelectedArray (int i) const
    {
        if (selected_ < 0 || i < 0
            || selected_ >= (int) scene_.size() || i >= (int) scene_.size())
            return false;
        const int a = scene_[(size_t) selected_].arrayId;
        const int b = scene_[(size_t) i].arrayId;
        if (a < 0 || b < 0) return i == selected_;
        return a == b;
    }

    bool rowsMatch (const std::vector<Speaker>& sp) const
    {
        if (sp.size() != sources_.size() || sp.size() != angles_.size()) return false;
        for (size_t i = 0; i < sp.size(); ++i)
            if (sources_[i]->model != sp[i].model) return false;
        return true;
    }

    bool planesMatch (const juce::StringArray& names) const
    {
        if ((int) planes_.size() != names.size()) return false;
        for (int i = 0; i < names.size(); ++i)
            if (planes_[(size_t) i]->name != names[i]) return false;
        return true;
    }

    void buildPlaneRows (const juce::StringArray& names)
    {
        planes_.clear();
        for (int i = 0; i < names.size(); ++i)
        {
            auto row = std::make_unique<PlaneRow>();
            row->name = names[i];
            row->del  = std::make_unique<juce::TextButton> ("Delete");
            row->del->setColour (juce::TextButton::buttonColourId, Brand::panel());
            row->del->setColour (juce::TextButton::textColourOffId, Brand::accent());
            row->del->setVisible (false);
            row->del->onClick = [this, i]
            {
                if (! building_ && onDeletePlane) onDeletePlane (i);
            };
            addAndMakeVisible (*row->del);
            planes_.push_back (std::move (row));
        }
    }

    void buildRows (const std::vector<Speaker>& sp)
    {
        sources_.clear();
        angles_.clear();

        for (size_t i = 0; i < sp.size(); ++i)
        {
            const int idx = (int) i;

            auto row = std::make_unique<SourceRow>();
            row->model = sp[i].model;
            row->name  = modelName (sp[i].model) + "_" + juce::String (idx + 1);
            row->system = std::make_unique<juce::ComboBox>();
            row->system->addItem ("Q212P", 1);
            row->system->addItem ("Q21S", 2);
            row->system->addItem ("BEM 2inch", 3);
            row->system->onChange = [this, idx]
            {
                if (building_ || onModel == nullptr) return;
                onModel (idx, modelFromItem (sources_[(size_t) idx]->system->getSelectedId()));
            };
            row->mute = std::make_unique<SquareMark> ("M");
            row->mute->onClick = [this, idx]
            {
                if (building_ || onEnabled == nullptr) return;
                onEnabled (idx, ! sources_[(size_t) idx]->mute->getToggleState());
            };
            addAndMakeVisible (*row->system);
            addAndMakeVisible (*row->mute);
            row->system->addMouseListener (this, false);
            row->mute->addMouseListener (this, false);
            sources_.push_back (std::move (row));

            auto a = std::make_unique<AngleRow>();
            a->num   = juce::String (idx + 1);
            a->cab   = modelName (sp[i].model);
            a->ver   = std::make_unique<FieldBox> ("0", "Deg");
            a->hor   = std::make_unique<FieldBox> ("0", "Deg");
            a->splay = std::make_unique<FieldBox> ("0", "Deg");
            a->ver->setStyle (10.0f, 9.0f);
            a->hor->setStyle (10.0f, 9.0f);
            a->splay->setStyle (10.0f, 8.0f);
            a->ver->setInlineUnit (true, false);
            a->hor->setInlineUnit (true, false);
            a->splay->setInlineUnit (true, true);
            a->ver->onCommit = [this, idx] (double v)
            { if (! building_ && onTilt)     onTilt (idx, (float) v); };
            a->hor->onCommit = [this, idx] (double v)
            { if (! building_ && onRotation) onRotation (idx, (float) v); };
            a->splay->onCommit = [this, idx] (double v)
            {
                // Splay is measured from the cabinet above, so setting it turns
                // THIS cabinet and leaves the one above where it was.
                if (! building_ && onRotation && idx > 0 && (size_t) idx < prevRot_.size())
                    onRotation (idx, prevRot_[(size_t) idx] + (float) v);
            };
            a->pol  = std::make_unique<SquareMark> (juce::String::charToString ((juce_wchar) 0x00d8));
            a->mute = std::make_unique<SquareMark> ("M");
            a->pol->onClick = [this, idx]
            {
                if (building_ || onPolarity == nullptr) return;
                onPolarity (idx, angles_[(size_t) idx]->pol->getToggleState());
            };
            a->mute->onClick = [this, idx]
            {
                if (building_ || onEnabled == nullptr) return;
                onEnabled (idx, ! angles_[(size_t) idx]->mute->getToggleState());
            };
            for (juce::Component* c : { (juce::Component*) a->ver.get(),
                                        (juce::Component*) a->hor.get(),
                                        (juce::Component*) a->pol.get(),
                                        (juce::Component*) a->mute.get(),
                                        (juce::Component*) a->splay.get() })
                addAndMakeVisible (*c);
            angles_.push_back (std::move (a));
        }
    }

    /** A section folded or opened: re-lay, and fit the panel to what is now
        showing, never shorter than the viewport it scrolls in. */
    void refold()
    {
        resized();
        int h = contentH_;
        if (auto* vp = findParentComponentOfClass<juce::Viewport>())
            h = juce::jmax (h, vp->getMaximumVisibleHeight());
        if (h != getHeight()) setSize (getWidth(), h);
        repaint();
    }

    void commitPosition (int axis, double v)
    {
        if (building_ || selected_ < 0) return;
        const float m = (float) Units::displayToMetres (v);
        if (axis == 2) { if (onHeight) onHeight (selected_, m); return; }
        if (onPosition) onPosition (selected_, axis == 0 ? m : lastX_, axis == 1 ? m : lastY_);
    }

    void pushValues (const std::vector<Speaker>& sp)
    {
        building_ = true;
        prevRot_.assign (sp.size(), 0.0f);

        for (size_t i = 0; i < sources_.size() && i < sp.size(); ++i)
        {
            sources_[i]->system->setSelectedId (itemFromModel (sp[i].model), juce::dontSendNotification);
            sources_[i]->mute->setToggleState (! sp[i].enabled, juce::dontSendNotification);

            angles_[i]->ver->setValue (num (sp[i].tiltDeg));
            angles_[i]->hor->setValue (num (sp[i].rotationDeg));
            angles_[i]->pol->setToggleState (sp[i].polarityInverted, juce::dontSendNotification);
            angles_[i]->mute->setToggleState (! sp[i].enabled, juce::dontSendNotification);
            // Splay is the angle from the box ABOVE THIS ONE IN THE SAME
            // ARRAY. Taken across the scene it measured the gap between two
            // unrelated sources, and the first box of every array but the
            // first carried a splay it had no business having.
            int above = -1;
            const int myId = sp[i].arrayId;
            for (int k = (int) i - 1; k >= 0; --k)
                if (myId >= 0 ? sp[(size_t) k].arrayId == myId : false) { above = k; break; }

            prevRot_[i] = above < 0 ? 0.0f : sp[(size_t) above].rotationDeg;
            const float splay = above < 0 ? 0.0f
                              : sp[i].rotationDeg - sp[(size_t) above].rotationDeg;
            angles_[i]->splay->setValue (num (splay));
            // The top box of an array has nothing to splay from.
            angles_[i]->splay->setEnabled (above >= 0);
        }

        const bool has = selected_ >= 0 && (size_t) selected_ < sp.size();
        if (has)
        {
            const auto& sel = sp[(size_t) selected_];
            lastX_ = sel.x; lastY_ = sel.y; lastHeight_ = sel.baseHeightM;
            posSystem_->setSelectedId (itemFromModel (sel.model), juce::dontSendNotification);
            posMounting_->setSelectedId (sel.baseHeightM > 0.01f ? 2 : 1, juce::dontSendNotification);
            posXYZ_[0]->setValue (num (Units::metresToDisplay (sel.x)));
            posXYZ_[1]->setValue (num (Units::metresToDisplay (sel.y)));
            posXYZ_[2]->setValue (num (Units::metresToDisplay (sel.baseHeightM)));
            posHorAim_->setValue (num (sel.rotationDeg));
            posVerAim_->setValue (num (sel.tiltDeg));
            headPos_->setTitle ("Source Positioning (" + sources_[(size_t) selected_]->name + ")");
            headAng_->setTitle ("Rigging Angles (" + sources_[(size_t) selected_]->name + ")");
        }
        else
        {
            headPos_->setTitle ("Source Positioning");
            headAng_->setTitle ("Rigging Angles");
        }

        for (juce::Component* c : { (juce::Component*) posSystem_.get(),
                                    (juce::Component*) posMounting_.get(),
                                    (juce::Component*) posXYZ_[0].get(),
                                    (juce::Component*) posXYZ_[1].get(),
                                    (juce::Component*) posXYZ_[2].get(),
                                    (juce::Component*) posHorAim_.get(),
                                    (juce::Component*) posVerAim_.get() })
            if (c != nullptr) c->setEnabled (has);

        building_ = false;
    }

    PanelLnF lnf_;
    // The scene as last handed in. Needed because which rows belong to the
    // selected SOURCE is a question about arrays, not about row order.
    std::vector<Speaker> scene_;

    std::vector<std::unique_ptr<SourceRow>> sources_;
    std::vector<std::unique_ptr<AngleRow>>  angles_;
    std::unique_ptr<SectionHead> headSrc_, headPos_, headRig_, headAng_;
    std::vector<std::unique_ptr<PlaneRow>> planes_;
    std::vector<juce::Range<int>>          planeRowY_;
    int                                    selPlane_ = -1;
    std::unique_ptr<juce::ComboBox> posSystem_, posMounting_, posCabinets_,
                                    frontPick_, rearPick_, singlePick_;
    std::unique_ptr<FieldBox> posXYZ_[3], posHorAim_, posVerAim_;
    std::unique_ptr<LoadBar>  loadBar_;

    std::vector<Text>  texts_;
    std::vector<int>   rowRules_, listRules_;
    std::vector<juce::Range<int>> bands_;
    juce::Rectangle<int> emptyArea_;
    std::vector<juce::Range<int>> sourceRowY_;
    std::vector<float> prevRot_;
    std::vector<juce::Rectangle<int>> vRules_;
    int   contentH_  = 400;
    int   selected_  = -1;
    bool  building_  = false;
    float lastX_ = 0, lastY_ = 0, lastHeight_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SourcePanel)
};
