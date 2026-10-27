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
//   * No. of Cabinets - a source is one cabinet today; arrays are not modelled.
//   * Rigging Points  - loads need cabinet weights, which the model lacks.
// =============================================================================
class SourcePanel : public juce::Component
{
public:
    //--------------------------------------------------------------- look
    static juce::Colour fieldFill() { return juce::Colour (0xffd9d9d9); }

    class PanelLnF : public juce::LookAndFeel_V4
    {
    public:
        void drawComboBox (juce::Graphics& g, int w, int h, bool,
                           int, int, int, int, juce::ComboBox& box) override
        {
            // Figma: a flat #D9D9D9 box, no outline. A row can tint its own
            // combo (the selected element's is pink) via backgroundColourId.
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

            // Figma: a 12 x 7 arrow centred 14 px in from the box's right end.
            const float s = UiConfig::Scale::figf (12.0f);
            const float cx = (float) w - UiConfig::Scale::figf (14.0f), cy = (float) h * 0.5f;
            juce::Path t;
            t.addTriangle (cx - s * 0.5f, cy - s * 0.29f,
                           cx + s * 0.5f, cy - s * 0.29f,
                           cx,            cy + s * 0.29f);
            g.setColour (Brand::text().withAlpha (box.isEnabled() ? 0.85f : 0.35f));
            g.fillPath (t);
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

        // Figma: the plus is centred in the 44 px cell at x 308..352, the fold
        // arrow in 352..396 - 66 and 22 px in from the right edge.
        void mouseUp (const juce::MouseEvent& e) override
        {
            using UiConfig::Scale::fig;
            if (hasPlus_ && onPlus != nullptr
                && e.x > getWidth() - fig (88.0f)
                && e.x < getWidth() - fig (44.0f))
                onPlus();
        }

        void paint (juce::Graphics& g) override
        {
            using UiConfig::Scale::fig;
            using UiConfig::Scale::figf;
            g.fillAll (juce::Colour (0xffdfdede));
            g.setColour (Brand::text());
            g.setFont (Brand::figMed (16.0f));
            g.drawText (title_, getLocalBounds().withTrimmedLeft (fig (14.0f))
                                                .withTrimmedRight (fig (92.0f)),
                        juce::Justification::centredLeft, true);

            const float h = (float) getHeight();
            float x = (float) getWidth() - figf (22.0f);
            const float tw = figf (14.0f), th = figf (8.0f);
            juce::Path p;
            p.addTriangle (x - tw * 0.5f, h * 0.5f - th * 0.5f,
                           x + tw * 0.5f, h * 0.5f - th * 0.5f,
                           x,             h * 0.5f + th * 0.5f);
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

        void paint (juce::Graphics& g) override
        {
            g.setColour (isEnabled() ? fieldFill() : fieldFill().withAlpha (0.55f));
            g.fillRect (getLocalBounds());
            if (isBeingEdited()) return;

            // Figma: value and unit in the same Medium black; the unit's right
            // edge is 10 px in from the box's.
            using UiConfig::Scale::fig;
            const auto f = Brand::figMed (fontPx_);
            g.setFont (f);
            g.setColour (Brand::text().withMultipliedAlpha (isEnabled() ? 1.0f : 0.45f));
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
            ed->setFont (Brand::figMed (fontPx_));
            ed->setColour (juce::TextEditor::textColourId, Brand::text());
            ed->setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
            ed->setBounds (getLocalBounds().withTrimmedLeft (juce::jmax (0, UiConfig::Scale::fig (inset_) - 2)));
        }

    private:
        juce::String unit_;
        float fontPx_ = 12.0f;
        float inset_  = 10.0f;
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
            auto r = getLocalBounds().toFloat().reduced (0.5f);
            const bool on = getToggleState();
            g.setColour (on ? Brand::accent() : Brand::white());
            g.fillRect (r);
            g.setColour (on ? Brand::accent()
                            : Brand::border().withMultipliedAlpha (hl ? 1.6f : 1.0f));
            g.drawRect (r, 1.0f);
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
        void paint (juce::Graphics& g) override
        {
            // Red says "this is the load state". Grey says "there is no load
            // state to show" - the truth until cabinets carry weights.
            g.fillAll (unavailable_ ? juce::Colour (0xff9a9a9a) : juce::Colour (0xffe8202a));
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
            const int left  = juce::jmin (fig (285.0f), right - shareFont.getStringWidth (share_));
            g.drawText (share_, juce::Rectangle<int> (left, 0, right - left, getHeight()),
                        juce::Justification::centredLeft, true);
        }

        juce::String state_ = "LOAD NOT CALCULATED";
        juce::String share_ = "needs cabinet weights";
        bool         unavailable_ = true;
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
    std::function<void (int index, bool on)>          onEnabled;
    std::function<void (int index, bool inverted)>    onPolarity;
    std::function<void()>                             onAddSource;
    // Listening planes share the Sources list with the cabinets: a
    // plane is a thing you add from the same "+", so it belongs in the
    // same list rather than in a second one nobody thinks to look at.
    std::function<void (int planeIndex)>              onSelectPlane;
    std::function<void (int planeIndex)>              onDeletePlane;

    /** Rebuild from the scene. Cheap enough for every change. */
    void setScene (const std::vector<Speaker>& sp, int selected)
    {
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

    void mouseUp (const juce::MouseEvent& e) override
    {
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
        g.fillAll (juce::Colour (0xfff2f2f2));

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
            g.setColour (Brand::accent().withAlpha (0.07f));
            g.fillRect (0, r.getStart(), getWidth(), r.getLength());
            g.setColour (Brand::accent());
            g.fillRect (0, r.getStart(), UiConfig::Scale::px (3), r.getLength());
        }

        // A selected plane is marked the same way a selected cabinet is, so
        // the one list reads as one list.
        if (selPlane_ >= 0 && (size_t) selPlane_ < planeRowY_.size())
        {
            const auto r = planeRowY_[(size_t) selPlane_];
            g.setColour (Brand::accent().withAlpha (0.07f));
            g.fillRect (0, r.getStart(), getWidth(), r.getLength());
            g.setColour (Brand::accent());
            g.fillRect (0, r.getStart(), UiConfig::Scale::px (3), r.getLength());
        }

        g.setColour (Brand::border().withAlpha (0.8f));
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

        // Only when the list is genuinely empty. A plane is a row in this
        // list too, and the message is drawn at a fixed height from the top -
        // so with a plane present and no cabinet it was painted straight
        // through the plane's own row.
        if (sources_.empty() && planes_.empty())
        {
            g.setColour (Brand::text().withAlpha (0.55f));
            g.setFont (Brand::figMed (12.0f));
            g.drawText ("No sources yet - add a cabinet to begin.",
                        getLocalBounds().withHeight (UiConfig::Scale::px (80)),
                        juce::Justification::centred, true);
        }
    }

    void resized() override
    {
        texts_.clear();
        rowRules_.clear();
        sourceRowY_.clear();
        planeRowY_.clear();

        vRules_.clear();

        // Every number below is a Figma pixel from the SOURCE frame (panel
        // x 0..396, its top at frame y 102), mapped onto the window by fig().
        // The Figma grid is rows of 44 with 28 px controls 8 px down them.
        using UiConfig::Scale::fig;
        const int W    = getWidth();
        const int rowH = fig (44.0f);
        const int ctlH = fig (28.0f);
        const int ctlY = fig (8.0f);
        int y = 0;

        auto label = [this] (const juce::String& t, juce::Rectangle<int> a,
                             float size = 12.0f, bool bold = false, bool dim = false)
        { texts_.push_back ({ t, a, size, bold, dim, juce::Justification::centredLeft }); };
        auto ctl = [&] (juce::Component& c, float x, float w)
        { c.setBounds (fig (x), y + ctlY, fig (w), ctlH); };
        auto nextRow = [&] { y += rowH; rowRules_.push_back (y); };

        // ---------------------------------------------------- Sources rows
        // Name at 14, "System" at 178, the combo 235..352, the mute 360..388.
        for (auto& s : sources_)
        {
            label (s->name, { fig (14.0f), y, fig (160.0f), rowH });
            label ("System", { fig (178.0f), y, fig (56.0f), rowH });
            ctl (*s->system, 235.0f, 117.0f);
            ctl (*s->mute,   360.0f, 28.0f);
            sourceRowY_.push_back ({ y, y + rowH });
            nextRow();
        }
        // ----------------------------------------------------- Plane rows
        for (auto& p : planes_)
        {
            label (p->name, { fig (14.0f), y, fig (160.0f), rowH });
            label ("Listening plane", { fig (178.0f), y, fig (120.0f), rowH });
            ctl (*p->del, 300.0f, 84.0f);
            planeRowY_.push_back ({ y, y + rowH });
            nextRow();
        }

        // The Figma's list is a four-row well - three sources and a spare -
        // and it keeps one spare row as it grows.
        const int listRows = (int) (sources_.size() + planes_.size());
        while (y < rowH * juce::jmax (4, listRows + 1))
            nextRow();

        // --------------------------------------------- Source Positioning
        headPos_->setBounds (0, y, W, rowH); y += rowH;

        label ("System",   { fig (12.0f),  y, fig (56.0f), rowH });
        ctl (*posSystem_,   69.0f, 117.0f);
        label ("Mounting", { fig (210.0f), y, fig (72.0f), rowH });
        ctl (*posMounting_, 283.0f, 101.0f);
        nextRow();

        label ("No. of\nCabinets", { fig (12.0f), y, fig (56.0f), rowH }, 10.0f);
        ctl (*posCabinets_, 69.0f, 117.0f);
        nextRow();

        label ("Hor.Aim", { fig (12.0f),  y, fig (56.0f), rowH });
        ctl (*posHorAim_, 69.0f, 117.0f);
        label ("Ver.Aim", { fig (210.0f), y, fig (72.0f), rowH });
        ctl (*posVerAim_, 283.0f, 101.0f);
        nextRow();

        // Three 132-wide cells: caption 1 px down, field 16 px down.
        {
            const char* caps[] = { "Pos. x", "Pos. y", "Pos. z" };
            for (int i = 0; i < 3; ++i)
            {
                const float cx = 132.0f * (float) i;
                label (caps[i], { fig (cx + 12.0f), y + fig (1.0f), fig (108.0f), fig (15.0f) });
                posXYZ_[i]->setBounds (fig (cx + 12.0f), y + fig (16.0f), fig (108.0f), ctlH);
            }
            vRules_.push_back ({ fig (132.0f), y, 1, rowH });
            vRules_.push_back ({ fig (264.0f), y, 1, rowH });
        }
        nextRow();

        // ------------------------------------------------- Rigging Points
        headRig_->setBounds (0, y, W, rowH); y += rowH;
        loadBar_->setBounds (0, y, W, rowH); y += rowH;

        label ("Front Pick", { fig (12.0f),  y, fig (70.0f), rowH });
        ctl (*frontPick_,  84.0f, 86.0f);
        label ("Rear Pick",  { fig (210.0f), y, fig (72.0f), rowH });
        ctl (*rearPick_,  283.0f, 101.0f);
        nextRow();

        label ("Single Pick", { fig (12.0f), y, fig (70.0f), rowH });
        ctl (*singlePick_, 84.0f, 86.0f);
        nextRow();

        // ------------------------------------------------- Rigging Angles
        headAng_->setBounds (0, y, W, rowH); y += rowH;

        {
            // "Cab no." hangs level with the first line of "Ver. / Rot.";
            // the other single-line captions are centred in the row.
            const struct { const char* text; float x; bool hang; } caps[] = {
                { "Cab no.", 12.0f, true }, { "Ver.\nRot.", 124.0f, false },
                { "Hor.\nRot.", 173.0f, false }, { "Pol inv.", 214.0f, false },
                { "Mute", 262.0f, false }, { "Splay", 307.0f, false } };
            for (const auto& c : caps)
                label (c.text, c.hang ? juce::Rectangle<int> (fig (c.x), y + fig (10.0f), fig (48.0f), fig (12.0f))
                                      : juce::Rectangle<int> (fig (c.x), y, fig (48.0f), rowH), 10.0f);
        }
        nextRow();

        // "1" at 12 and the model at 27; Ver. 116, Hor. 165 (37 wide), Pol
        // 218 and Mute 262 (28 square), the angle 307..384.
        for (auto& a : angles_)
        {
            label (a->num, { fig (12.0f), y, fig (14.0f), rowH });
            label (a->cab, { fig (27.0f), y, fig (86.0f), rowH });
            ctl (*a->ver,   116.0f, 37.0f);
            ctl (*a->hor,   165.0f, 37.0f);
            ctl (*a->pol,   218.0f, 28.0f);
            ctl (*a->mute,  262.0f, 28.0f);
            ctl (*a->splay, 307.0f, 77.0f);
            nextRow();
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
        headPos_ = std::make_unique<SectionHead> ("Source Positioning", true);
        headRig_ = std::make_unique<SectionHead> ("Rigging Points", false);
        headAng_ = std::make_unique<SectionHead> ("Rigging Angles", false);
        headPos_->onPlus = [this] { if (onAddSource) onAddSource(); };
        for (auto* h : { headPos_.get(), headRig_.get(), headAng_.get() })
            addAndMakeVisible (*h);

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
        posHorAim_ = std::make_unique<FieldBox> ("0", "deg");
        posVerAim_ = std::make_unique<FieldBox> ("0", "deg");
        posHorAim_->onCommit = [this] (double v)
        { if (! building_ && onRotation && selected_ >= 0) onRotation (selected_, (float) v); };
        posVerAim_->onCommit = [this] (double v)
        { if (! building_ && onTilt && selected_ >= 0) onTilt (selected_, (float) v); };
        addAndMakeVisible (*posHorAim_);
        addAndMakeVisible (*posVerAim_);
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
            sources_.push_back (std::move (row));

            auto a = std::make_unique<AngleRow>();
            a->num   = juce::String (idx + 1);
            a->cab   = modelName (sp[i].model);
            a->ver   = std::make_unique<FieldBox> ("0");
            a->hor   = std::make_unique<FieldBox> ("0");
            a->splay = std::make_unique<FieldBox> ("0");
            a->ver->setStyle (8.0f, 6.0f);
            a->hor->setStyle (8.0f, 6.0f);
            a->splay->setStyle (12.0f, 9.0f);
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

            angles_[i]->ver->setValue (juce::String (sp[i].tiltDeg, 1));
            angles_[i]->hor->setValue (juce::String (sp[i].rotationDeg, 1));
            angles_[i]->pol->setToggleState (sp[i].polarityInverted, juce::dontSendNotification);
            angles_[i]->mute->setToggleState (! sp[i].enabled, juce::dontSendNotification);
            prevRot_[i] = i == 0 ? 0.0f : sp[i - 1].rotationDeg;
            const float splay = i == 0 ? 0.0f : sp[i].rotationDeg - sp[i - 1].rotationDeg;
            angles_[i]->splay->setValue (juce::String (splay, 1));
            angles_[i]->splay->setEnabled (i > 0);
        }

        const bool has = selected_ >= 0 && (size_t) selected_ < sp.size();
        if (has)
        {
            const auto& sel = sp[(size_t) selected_];
            lastX_ = sel.x; lastY_ = sel.y; lastHeight_ = sel.baseHeightM;
            posSystem_->setSelectedId (itemFromModel (sel.model), juce::dontSendNotification);
            posMounting_->setSelectedId (sel.baseHeightM > 0.01f ? 2 : 1, juce::dontSendNotification);
            posXYZ_[0]->setValue (juce::String (Units::metresToDisplay (sel.x), 1));
            posXYZ_[1]->setValue (juce::String (Units::metresToDisplay (sel.y), 1));
            posXYZ_[2]->setValue (juce::String (Units::metresToDisplay (sel.baseHeightM), 1));
            posHorAim_->setValue (juce::String (sel.rotationDeg, 1));
            posVerAim_->setValue (juce::String (sel.tiltDeg, 1));
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
    std::vector<std::unique_ptr<SourceRow>> sources_;
    std::vector<std::unique_ptr<AngleRow>>  angles_;
    std::unique_ptr<SectionHead> headPos_, headRig_, headAng_;
    std::vector<std::unique_ptr<PlaneRow>> planes_;
    std::vector<juce::Range<int>>          planeRowY_;
    int                                    selPlane_ = -1;
    std::unique_ptr<juce::ComboBox> posSystem_, posMounting_, posCabinets_,
                                    frontPick_, rearPick_, singlePick_;
    std::unique_ptr<FieldBox> posXYZ_[3], posHorAim_, posVerAim_;
    std::unique_ptr<LoadBar>  loadBar_;

    std::vector<Text>  texts_;
    std::vector<int>   rowRules_;
    std::vector<juce::Range<int>> sourceRowY_;
    std::vector<float> prevRot_;
    std::vector<juce::Rectangle<int>> vRules_;
    int   contentH_  = 400;
    int   selected_  = -1;
    bool  building_  = false;
    float lastX_ = 0, lastY_ = 0, lastHeight_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SourcePanel)
};
