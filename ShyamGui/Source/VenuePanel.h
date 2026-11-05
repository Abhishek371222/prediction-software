#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"
#include "UiChrome.h"
#include "AppSettings.h"
#include "VenueTab.h"
#include "SourcePanel.h"
#include "InlineRename.h"

// Figma "Venue screen - 1.2" colours. Every colour this panel paints comes
// from here; the value boxes and dropdowns share SourcePanel's #D9D9D9.
namespace VenueSkin
{
    inline juce::Colour panelBg()   { return juce::Colour (0xffffffff); }
    inline juce::Colour rowBg()     { return juce::Colour (0xfff0f0f0); }
    inline juce::Colour rowSel()    { return Brand::selection(); }
    inline juce::Colour rowRule()   { return juce::Colour (0x33000000); }
    inline juce::Colour pointRule() { return juce::Colour (0xff999999); }
    inline juce::Colour edge()      { return juce::Colour (0xff000000); }
    inline juce::Colour ink()       { return juce::Colour (0xff000000); }
}

// The look of this panel's dropdowns while they are OPEN.
//
// A ComboBox's popup is drawn by its LookAndFeel, not by the box's own colour
// IDs, so styling the box left the list that drops out of it in the default
// scheme. The open list is the Sources panel's (SourcePanel::drawListItem),
// so every dropdown in the side panels drops the same white list.
class VenueComboLnF : public juce::LookAndFeel_V4
{
public:
    VenueComboLnF()
    {
        setColour (juce::PopupMenu::backgroundColourId,            kList);
        setColour (juce::PopupMenu::textColourId,                  VenueSkin::ink());
        setColour (juce::PopupMenu::highlightedBackgroundColourId, VenueSkin::rowSel());
        setColour (juce::PopupMenu::highlightedTextColourId,       VenueSkin::ink());
        // The CLOSED box too. Taking over a combo's LookAndFeel replaces the
        // panel's, so anything this one leaves unset falls back to the JUCE
        // default - which is how Aud. Height ended up a dark box on a white
        // panel. A per-component colour still wins over these, so setting a
        // default here is safe.
        setColour (juce::ComboBox::backgroundColourId,             kList);
        setColour (juce::ComboBox::outlineColourId,                juce::Colours::transparentBlack);
        setColour (juce::ComboBox::textColourId,                   VenueSkin::ink());
        setColour (juce::ComboBox::arrowColourId,                  VenueSkin::ink());
    }

    /** The panel's type scales with its width, so the list has to be told. */
    void setItemFont (juce::Font f) { itemFont_ = f; }
    juce::Font getPopupMenuFont() override { return itemFont_; }

    void drawPopupMenuBackground (juce::Graphics& g, int w, int h) override
    {
        SourcePanel::drawListBackground (g, w, h);
    }

    void getIdealPopupMenuItemSize (const juce::String& text, bool isSeparator, int,
                                    int& idealWidth, int& idealHeight) override
    {
        SourcePanel::listItemSize (text, isSeparator, itemFont_, idealWidth, idealHeight);
    }

    /** V4's box, with the solid 14 x 8 triangle the section headers use in
        place of its stroked chevron. */
    void drawComboBox (juce::Graphics& g, int width, int height, bool,
                       int, int, int, int, juce::ComboBox& box) override
    {
        const auto r = juce::Rectangle<int> (0, 0, width, height).toFloat();
        g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (box.findColour (juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);

        const float tw = UiConfig::Scale::figf (14.0f), th = UiConfig::Scale::figf (8.0f);
        const float cx = (float) width - 20.0f, cy = (float) height * 0.5f;
        juce::Path t;
        t.addTriangle (cx - tw * 0.5f, cy - th * 0.5f,
                       cx + tw * 0.5f, cy - th * 0.5f,
                       cx,             cy + th * 0.5f);
        g.setColour (box.findColour (juce::ComboBox::arrowColourId)
                        .withAlpha (box.isEnabled() ? 0.9f : 0.2f));
        g.fillPath (t);
    }

    /** The Sources panel's list: no tick, the current choice in blue. */
    void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool, const juce::String& text,
                            const juce::String&, const juce::Drawable*,
                            const juce::Colour*) override
    {
        SourcePanel::drawListItem (g, area, isSeparator, isActive, isHighlighted, isTicked,
                                   text, itemFont_);
    }

private:
    // The same grey the value boxes use.
    const juce::Colour kList { 0xffd9d9d9 };
    juce::Font itemFont_ { Brand::tech (13.0f) };
};

// =============================================================================
// The Venue tab's left panel.
//
//   Elements                               +  v     (the view strip above)
//     Audience                Type [ Listening  v ]
//     Stage                   Type [ Structure  v ]
//   Element Settings (Audience)                 v
//     Name [Audience  ]   Aud. Height [Seated     v]
//     POINT 1
//     Pos. x          Pos. y          Pos. z
//     [0.0       ft]  [0.0       ft]  [0.0       ft]
//     POINT 2 ...
//
// Laid out in Figma px (Venue screen - 1.2, panel y = frame y - 88) through
// UiConfig::Scale::fig, so a maximised 1920 x 1080 window is 1:1 with the
// drawing and every other window scales it as a whole.
// =============================================================================
class VenuePanel : public juce::Component
{
public:
    std::function<void()>                          onAddElement;
    std::function<void (int)>                      onSelect;
    std::function<void (int, VenueElement::Type)>  onType;
    std::function<void (int, const juce::String&)> onName;
    std::function<void (int, VenueElement::AudHeight)> onAudHeight;
    /** corner 0..3, axis 0=x 1=y 2=z, value in METRES. */
    std::function<void (int, int, int, float)>     onPoint;
    /** Remove the element at this index. */
    std::function<void (int)>                      onDelete;
    /** The row's padlock: lock (true) or unlock the element on the plan. */
    std::function<void (int, bool)>                onLock;
    /** A column's sync icon: copy POINT 1's value on axis (0 x, 1 y, 2 z)
        of element @p index into POINTs 2-4. */
    std::function<void (int index, int axis)>      onSyncAxis;

    VenuePanel()
    {
        setLookAndFeel (&lnf_);

        // No "Elements" header here: the view strip directly above this panel
        // is that header, plus and chevron included. See VenueView.
        headSet_ = std::make_unique<SourcePanel::SectionHead> ("Element Settings", false);
        headSet_->onFold = [this] (bool f)
        {
            settingsFolded_ = f;
            resized();
            repaint();
            if (onLayoutChanged) onLayoutChanged();
        };
        addAndMakeVisible (*headSet_);

        name_ = std::make_unique<SourcePanel::FieldBox> (juce::String());
        name_->setStyle (12.0f, 12.1f);
        name_->onTextChange = [this]
        {
            if (! building_ && onName && sel_ >= 0) onName (sel_, name_->getText());
        };
        addAndMakeVisible (*name_);

        // Inline rename: a click on a row's NAME edits it in place. Enter
        // saves, Escape cancels, clicking away saves.
        renameEd_.setMultiLine (false);
        renameEd_.setReturnKeyStartsNewLine (false);
        renameEd_.setColour (juce::TextEditor::backgroundColourId, juce::Colours::white);
        renameEd_.setColour (juce::TextEditor::textColourId, VenueSkin::ink());
        renameEd_.setColour (juce::TextEditor::outlineColourId, Brand::accent());
        renameEd_.setColour (juce::TextEditor::focusedOutlineColourId, Brand::accent());
        renameEd_.setColour (juce::TextEditor::highlightColourId, VenueSkin::rowSel());
        renameEd_.setColour (juce::CaretComponent::caretColourId, VenueSkin::ink());
        renameEd_.onReturnKey = [this] { endRename (true); };
        renameEd_.onEscapeKey = [this] { endRename (false); };
        renameEd_.onFocusLost = [this] { endRename (true); };
        // A click anywhere else saves too - most surfaces never take focus.
        clickAway_.onClickAway = [this] { endRename (true); };
        addChildComponent (renameEd_);

        audH_ = std::make_unique<juce::ComboBox>();
        audH_->setLookAndFeel (&comboLnf_);
        audH_->addItem ("Seated", 1);
        audH_->addItem ("Standing", 2);
        audH_->onChange = [this]
        {
            if (! building_ && onAudHeight && sel_ >= 0)
                onAudHeight (sel_, audH_->getSelectedId() == 2 ? VenueElement::AudHeight::Standing
                                                               : VenueElement::AudHeight::Seated);
        };
        addAndMakeVisible (*audH_);

        for (int c = 0; c < 4; ++c)
            for (int a = 0; a < 3; ++a)
            {
                auto f = std::make_unique<SourcePanel::FieldBox> (juce::String(), Units::lengthUnit());
                f->setStyle (12.0f, 10.1f);
                f->onCommit = [this, c, a] (double shown)
                {
                    if (building_ || onPoint == nullptr || sel_ < 0) return;
                    onPoint (sel_, c, a, (float) Units::displayToMetres (shown));
                };
                addAndMakeVisible (*f);
                pt_.push_back (std::move (f));
            }
    }

    ~VenuePanel() override
    {
        // Every component must let go of a LookAndFeel before it dies. Both
        // of this panel's are members, so without this the combos would be
        // left pointing at a destroyed object during their own teardown.
        if (audH_ != nullptr) audH_->setLookAndFeel (nullptr);
        for (auto& r : rows_)
            if (r != nullptr && r->type != nullptr) r->type->setLookAndFeel (nullptr);
        setLookAndFeel (nullptr);
    }

    void setElements (const std::vector<VenueElement>& e, int selected)
    {
        if (! rowsMatch (e)) buildRows (e);
        sel_ = juce::jlimit (-1, (int) e.size() - 1, selected);
        pushValues (e);
        resized();
        repaint();
    }

    int getContentHeight() const { return contentH_; }

    /** Called when a fold changes how tall the panel's content is. */
    std::function<void()> onLayoutChanged;

    /** The Elements list folds from the header strip above the panel. */
    void setListFolded (bool f)
    {
        if (listFolded_ == f) return;
        listFolded_ = f;
        resized();
        repaint();
        if (onLayoutChanged) onLayoutChanged();
    }
    bool isListFolded() const noexcept { return listFolded_; }

    /** Start editing element @p idx's name in place, on its list row. The
        plan's right-click Rename lands here too, so there is one editor. */
    void beginRename (int idx)
    {
        if (idx < 0 || (size_t) idx >= rows_.size()) return;
        if (listFolded_) setListFolded (false);
        if ((size_t) idx >= nameArea_.size()) return;

        renaming_ = idx;
        renameEd_.setFont (Brand::figMed (12.0f));
        renameEd_.setIndents (UiConfig::Scale::fig (4.0f), 0);
        renameEd_.setJustification (juce::Justification::centredLeft);
        renameEd_.setBounds (editorBounds (idx));
        renameEd_.setText (rows_[(size_t) idx]->name, juce::dontSendNotification);
        renameEd_.setVisible (true);
        renameEd_.toFront (true);
        renameEd_.grabKeyboardFocus();
        renameEd_.selectAll();
        clickAway_.start (renameEd_);
        repaint();
    }

    bool isRenaming() const noexcept { return renaming_ >= 0; }

    void mouseMove (const juce::MouseEvent& ev) override
    {
        bool overLock = false;
        for (const auto& a : lockArea_) if (a.contains (ev.getPosition())) overLock = true;
        if (syncEnabled())
            for (const auto& a : syncArea_) if (a.contains (ev.getPosition())) overLock = true;
        setMouseCursor (overLock ? juce::MouseCursor::PointingHandCursor
                      : nameIndexAt (ev.getPosition()) >= 0 ? juce::MouseCursor::IBeamCursor
                                                            : juce::MouseCursor::NormalCursor);
    }

    void mouseUp (const juce::MouseEvent& ev) override
    {
        if (! ev.mods.isPopupMenu() && syncEnabled())
            for (int a = 0; a < 3; ++a)
                if (syncArea_[a].contains (ev.getPosition()))
                {
                    if (onSyncAxis) onSyncAxis (sel_, a);
                    return;
                }

        for (size_t i = 0; i < rowY_.size(); ++i)
            if (ev.y >= rowY_[i].getStart() && ev.y < rowY_[i].getEnd())
            {
                const int idx = (int) i;

                // The padlock: toggles the lock and does nothing else.
                if (! ev.mods.isPopupMenu() && (size_t) idx < lockArea_.size()
                    && lockArea_[(size_t) idx].contains (ev.getPosition()))
                {
                    if (onLock) onLock (idx, ! rows_[(size_t) idx]->locked);
                    return;
                }

                const bool onName = nameIndexAt (ev.getPosition()) == idx;
                if (onSelect) onSelect (idx);

                // Right-click the row: just Rename and Delete.
                if (ev.mods.isPopupMenu())
                {
                    showRowMenu (idx, { ev.getScreenX(), ev.getScreenY(), 1, 1 });
                    return;
                }

                // Left-click ON THE NAME: edit it straight away - no menu.
                if (onName)
                    beginRename (idx);
                return;
            }
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (VenueSkin::panelBg());

        // Every element row carries a band: #F0F0F0 for the rest, the
        // selection blue for the one being edited.
        for (size_t i = 0; i < rowY_.size(); ++i)
        {
            const auto r = rowY_[i];
            g.setColour ((int) i == sel_ ? VenueSkin::rowSel() : VenueSkin::rowBg());
            g.fillRect (0, r.getStart(), getWidth(), r.getLength());
        }

        g.setColour (VenueSkin::ink());
        for (const auto& t : texts_)
        {
            if (renaming_ >= 0 && (size_t) renaming_ < nameArea_.size()
                && t.area == nameArea_[(size_t) renaming_])
                continue;   // its editor is on top
            g.setFont (Brand::figMed (t.size));
            g.drawText (t.text, t.area, juce::Justification::centredLeft, true);
        }

        for (size_t i = 0; i < lockArea_.size() && i < rows_.size(); ++i)
            drawPadlock (g, lockArea_[i].toFloat(), rows_[i]->locked);

        if (! settingsFolded_)
            for (const auto& a : syncArea_)
                if (! a.isEmpty()) drawSyncIcon (g, a.toFloat(), syncEnabled());

        if (rows_.empty() && ! listFolded_)
        {
            g.setColour (VenueSkin::ink().withAlpha (0.55f));
            g.setFont (Brand::figMed (12.0f));
            g.drawText ("No elements yet - add an audience or a stage.",
                        juce::Rectangle<int> (0, 0, getWidth(), UiConfig::Scale::fig (88.0f)),
                        juce::Justification::centred, true);
        }
    }

    // The rules go over the children: the section header is a child and would
    // otherwise paint over its own outline.
    void paintOverChildren (juce::Graphics& g) override
    {
        const float W = (float) getWidth();
        for (const auto& r : rules_)
        {
            g.setColour (r.strong ? VenueSkin::pointRule() : VenueSkin::rowRule());
            g.drawHorizontalLine (r.y, 0.0f, W);
        }

        g.setColour (VenueSkin::edge());
        g.drawVerticalLine (0, 0.0f, (float) getHeight());
        g.drawVerticalLine (getWidth() - 1, 0.0f, (float) getHeight());
    }

    void resized() override
    {
        texts_.clear(); rules_.clear(); rowY_.clear(); nameArea_.clear(); lockArea_.clear();
        for (auto& a : syncArea_) a = {};

        using UiConfig::Scale::fig;
        // The open list is drawn by the LookAndFeel, which has no idea what
        // scale this panel is at, so it is told on every layout pass.
        comboLnf_.setItemFont (Brand::figMed (12.0f));
        // A box from Figma x and width, rounded at both ends so neighbouring
        // boxes keep their spacing at any scale.
        auto box = [] (float x, int y, float w, float h)
        {
            const int l = fig (x);
            return juce::Rectangle<int> (l, y, fig (x + w) - l, fig (h));
        };
        auto text = [this] (const juce::String& t, int x, int y, int w, int h, float size = 12.0f)
        { texts_.push_back ({ t, { x, y, w, h }, size }); };

        const int rowH  = fig (44.0f);
        const int pitch = fig (43.0f);
        const int ctlY  = fig (8.0f);
        const int lineH = fig (15.0f);

        // --- element rows: 44 tall, one shared rule between neighbours.
        int y = 0;
        for (auto& r : rows_)
        {
            r->type->setVisible (! listFolded_);
            if (listFolded_) continue;
            text (r->name, fig (14.0f), y, fig (162.0f), rowH);
            nameArea_.push_back ({ fig (14.0f), y, fig (162.0f), rowH });
            {
                const int sz = fig (22.0f);
                lockArea_.push_back ({ fig (181.0f), y + (rowH - sz) / 2, sz, sz });
            }
            text ("Type",  fig (212.0f), y, fig (55.0f), rowH);
            r->type->setBounds (box (269.0f, y + ctlY, 117.0f, 28.0f));
            rowY_.push_back ({ y, y + rowH });
            rules_.push_back ({ y, false });
            rules_.push_back ({ y + rowH - 1, false });
            y += pitch;
        }

        // An open editor follows its row through a relayout.
        if (renaming_ >= 0 && (size_t) renaming_ < nameArea_.size() && ! listFolded_)
            renameEd_.setBounds (editorBounds (renaming_));
        else if (renaming_ >= 0)
            endRename (false);

        // The list area is 158 tall in the drawing: two rows and white space.
        // Folded, it takes no room and Element Settings moves up under the strip.
        const int listH = listFolded_ ? 0
                        : juce::jmax (fig (158.0f), (int) rows_.size() * pitch + fig (72.0f));
        y = listH;

        // --- Element Settings header: 44 tall.
        headSet_->setBounds (0, y, getWidth(), rowH);
        rules_.push_back ({ y, false });
        y += pitch;

        const bool has = sel_ >= 0 && (size_t) sel_ < rows_.size();

        name_->setVisible (! settingsFolded_);
        audH_->setVisible (! settingsFolded_);
        for (auto& f : pt_) f->setVisible (! settingsFolded_);
        if (settingsFolded_)
        {
            rules_.push_back ({ y, false });
            contentH_ = y + fig (12.0f);
            return;
        }

        // --- Name row: 44 tall.
        text ("Name",        fig (12.1f),  y, fig (48.0f), rowH);
        name_->setBounds (box (61.6f, y + ctlY, 102.0f, 28.0f));
        text ("Aud. Height", fig (187.9f), y, fig (82.0f), rowH);
        audH_->setBounds (box (271.7f, y + ctlY, 118.2f, 28.0f));
        rules_.push_back ({ y, false });
        rules_.push_back ({ y + rowH - 1, false });
        y += rowH;

        // --- POINT blocks: 88 tall at an 87 pitch. "POINT n" 10 in, the
        //     captions 35 in, the 109.1 x 28 boxes 50 in.
        const juce::String u = Units::lengthUnit();
        const float colX[] = { 12.1f, 145.5f, 278.8f };
        for (int c = 0; c < 4; ++c)
        {
            const int top = y;
            rules_.push_back ({ top, true });
            text ("POINT " + juce::String (c + 1), fig (colX[0]), top + fig (10.0f), fig (120.0f), lineH);

            for (int a = 0; a < 3; ++a)
            {
                const juce::String cap = juce::String ("Pos. ") + (a == 0 ? "x" : a == 1 ? "y" : "z");
                text (cap, fig (colX[a]), top + fig (35.0f), fig (109.1f), lineH);
                // POINT 1 carries the column's sync icon, just after "Pos. x".
                if (c == 0)
                {
                    const int capW = juce::roundToInt (Brand::figMed (12.0f).getStringWidthFloat (cap));
                    const int sz   = fig (16.0f);
                    syncArea_[a] = { fig (colX[a]) + capW + fig (6.0f),
                                     top + fig (35.0f) + (lineH - sz) / 2, sz, sz };
                }
                auto& fld = pt_[(size_t) (c * 3 + a)];
                fld->setUnit (u);
                fld->setBounds (box (colX[a], top + fig (50.0f), 109.1f, 28.0f));
                fld->setEnabled (has && ! selLocked_);
            }
            y = top + fig (87.0f);
        }
        rules_.push_back ({ y, true });

        name_->setEnabled (has);
        audH_->setEnabled (has && selHasAudH_);

        contentH_ = y + fig (12.0f);
    }

private:
    juce::Rectangle<int> editorBounds (int idx) const
    {
        using UiConfig::Scale::fig;
        return nameArea_[(size_t) idx].reduced (0, fig (8.0f)).withTrimmedLeft (-fig (4.0f));
    }

    /** The row whose NAME TEXT is under @p p (the drawn text, not the whole
        name column), or -1. */
    int nameIndexAt (juce::Point<int> p) const
    {
        using UiConfig::Scale::fig;
        const auto f = Brand::figMed (12.0f);
        for (size_t i = 0; i < nameArea_.size() && i < rows_.size(); ++i)
        {
            auto a = nameArea_[i];
            const int w = juce::roundToInt (f.getStringWidthFloat (rows_[i]->name)) + fig (6.0f);
            a.setWidth (juce::jlimit (fig (24.0f), a.getWidth(), w));
            a = a.withSizeKeepingCentre (a.getWidth(), juce::roundToInt (f.getHeight()) + fig (8.0f));
            if (a.contains (p)) return (int) i;
        }
        return -1;
    }

    void showRowMenu (int idx, juce::Rectangle<int> at)
    {
        juce::PopupMenu m;
        m.addItem (1, "Rename");
        m.addItem (2, "Delete", onDelete != nullptr);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (at),
            [safe = juce::Component::SafePointer<VenuePanel> (this), idx] (int r)
            {
                if (safe == nullptr) return;
                if (r == 1) safe->beginRename (idx);
                else if (r == 2 && safe->onDelete) safe->onDelete (idx);
            });
    }

    /** Close the editor; @p save sends the new name on (if it changed and is
        not blank). Guarded, because hiding it moves focus, which reports
        focus-lost back here. */
    void endRename (bool save)
    {
        if (renaming_ < 0) return;
        const int idx = renaming_;
        renaming_ = -1;
        clickAway_.stop();
        const auto text = renameEd_.getText().trim();
        renameEd_.setVisible (false);
        repaint();
        if (save && text.isNotEmpty() && (size_t) idx < rows_.size()
            && text != rows_[(size_t) idx]->name && onName)
        {
            // After the click that ended the edit has been delivered: the
            // save rebuilds the rows, which must not happen under it.
            juce::MessageManager::callAsync (
                [safe = juce::Component::SafePointer<VenuePanel> (this), idx, text]
                { if (safe != nullptr && safe->onName) safe->onName (idx, text); });
        }
    }

    /** Sync only means something with an element selected that may move. */
    bool syncEnabled() const noexcept
    {
        return sel_ >= 0 && (size_t) sel_ < rows_.size() && ! selLocked_ && ! settingsFolded_;
    }

    /** Two arrows chasing each other round a circle - "make these the same". */
    static void drawSyncIcon (juce::Graphics& g, juce::Rectangle<float> r, bool enabled)
    {
        const auto b  = r.reduced (r.getWidth() * 0.12f);
        const float cx = b.getCentreX(), cy = b.getCentreY();
        const float rad = b.getWidth() * 0.42f;
        const float st  = juce::jmax (1.2f, b.getWidth() * 0.11f);
        const float ah  = b.getWidth() * 0.26f;   // arrowhead size
        g.setColour (VenueSkin::ink().withAlpha (enabled ? 0.9f : 0.3f));
        for (int k = 0; k < 2; ++k)
        {
            const float a0 = juce::MathConstants<float>::pi * (k == 0 ? 0.15f : 1.15f);
            const float a1 = a0 + juce::MathConstants<float>::pi * 0.72f;
            juce::Path arc;
            arc.addCentredArc (cx, cy, rad, rad, 0.0f, a0, a1, true);
            g.strokePath (arc, juce::PathStrokeType (st, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
            // Arrowhead at the arc's end, pointing along the turn.
            const juce::Point<float> tip (cx + rad * std::sin (a1), cy - rad * std::cos (a1));
            const juce::Point<float> dir (std::cos (a1), std::sin (a1));      // tangent
            const juce::Point<float> nrm (-dir.y, dir.x);
            juce::Path head;
            head.addTriangle (tip + dir * ah * 0.6f,
                              tip - dir * ah * 0.4f + nrm * ah * 0.55f,
                              tip - dir * ah * 0.4f - nrm * ah * 0.55f);
            g.fillPath (head);
        }
    }

    /** A small padlock: shackle closed when locked, swung open when not. */
    static void drawPadlock (juce::Graphics& g, juce::Rectangle<float> r, bool locked)
    {
        const auto b = r.reduced (r.getWidth() * 0.18f);
        const float bw = b.getWidth() * 0.78f, bh = b.getHeight() * 0.5f;
        const juce::Rectangle<float> body (b.getCentreX() - bw * 0.5f, b.getBottom() - bh, bw, bh);
        const float sw = bw * 0.62f, st = juce::jmax (1.2f, b.getWidth() * 0.11f);
        const float sx = body.getCentreX() - sw * 0.5f + (locked ? 0.0f : bw * 0.32f);
        const float sTop = b.getY();
        juce::Path shackle;
        shackle.startNewSubPath (sx, body.getY());
        shackle.lineTo (sx, sTop + sw * 0.5f);
        shackle.addCentredArc (sx + sw * 0.5f, sTop + sw * 0.5f, sw * 0.5f, sw * 0.5f, 0.0f,
                               -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi);
        shackle.lineTo (sx + sw, locked ? body.getY() : body.getY() - bh * 0.35f);
        const auto ink = locked ? VenueSkin::ink() : VenueSkin::ink().withAlpha (0.45f);
        g.setColour (ink);
        g.strokePath (shackle, juce::PathStrokeType (st, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
        g.fillRoundedRectangle (body, juce::jmax (1.0f, bw * 0.12f));
    }

    struct Row
    {
        bool         locked = false;
        juce::String name;
        std::unique_ptr<juce::ComboBox> type;
    };
    struct Text
    {
        juce::String text; juce::Rectangle<int> area; float size;
    };
    struct Rule { int y; bool strong; };

    bool rowsMatch (const std::vector<VenueElement>& e) const
    {
        if (rows_.size() != e.size()) return false;
        for (size_t i = 0; i < e.size(); ++i)
            if (rows_[i]->name != e[i].name) return false;
        return true;
    }

    void buildRows (const std::vector<VenueElement>& e)
    {
        rows_.clear();
        for (size_t i = 0; i < e.size(); ++i)
        {
            const int idx = (int) i;
            auto r = std::make_unique<Row>();
            r->name = e[i].name;
            r->type = std::make_unique<juce::ComboBox>();
            r->type->setLookAndFeel (&comboLnf_);
            r->type->addItem ("Listening", 1);
            r->type->addItem ("Structure", 2);
            r->type->addItem ("Object",    3);
            r->type->onChange = [this, idx]
            {
                if (building_ || onType == nullptr) return;
                const int id = rows_[(size_t) idx]->type->getSelectedId();
                onType (idx, id == 2 ? VenueElement::Type::Structure
                           : id == 3 ? VenueElement::Type::Object
                                     : VenueElement::Type::Listening);
            };
            addAndMakeVisible (*r->type);
            rows_.push_back (std::move (r));
        }
    }

    void pushValues (const std::vector<VenueElement>& e)
    {
        building_ = true;
        for (size_t i = 0; i < rows_.size() && i < e.size(); ++i)
        {
            rows_[i]->locked = e[i].locked;
            auto& t = *rows_[i]->type;
            t.setSelectedId (e[i].type == VenueElement::Type::Structure ? 2
                           : e[i].type == VenueElement::Type::Object    ? 3 : 1,
                             juce::dontSendNotification);
            // The dropdown keeps its grey box on the blue selected row, as
            // the Sources list draws it.
            t.removeColour (juce::ComboBox::backgroundColourId);
            t.removeColour (juce::ComboBox::outlineColourId);
            t.repaint();
        }

        if (sel_ >= 0 && (size_t) sel_ < e.size())
        {
            const auto& s = e[(size_t) sel_];
            headSet_->setTitle ("Element Settings (" + s.name + ")");
            name_->setValue (s.name);
            selHasAudH_ = s.hasAudHeight();
            selLocked_  = s.locked;
            audH_->setSelectedId (s.height == VenueElement::AudHeight::Standing ? 2 : 1,
                                  juce::dontSendNotification);
            for (int c = 0; c < 4; ++c)
                for (int a = 0; a < 3; ++a)
                {
                    const float m = a == 0 ? s.pts[c].x : a == 1 ? s.pts[c].y : s.pts[c].z;
                    pt_[(size_t) (c * 3 + a)]->setValue (juce::String (Units::metresToDisplay (m), 1));
                }
        }
        else
        {
            headSet_->setTitle ("Element Settings");
            selHasAudH_ = false;
            selLocked_  = false;
            name_->setValue ({});
            for (auto& f : pt_) f->setValue ({});
        }
        building_ = false;
    }

    SourcePanel::PanelLnF lnf_;
    std::unique_ptr<SourcePanel::SectionHead> headSet_;
    std::vector<std::unique_ptr<Row>> rows_;
    std::unique_ptr<SourcePanel::FieldBox> name_;
    std::unique_ptr<juce::ComboBox>        audH_;
    bool                                   selHasAudH_ = false;
    bool                                   listFolded_ = false;
    bool                                   settingsFolded_ = false;
    VenueComboLnF                          comboLnf_;
    std::vector<std::unique_ptr<SourcePanel::FieldBox>> pt_;

    std::vector<Text>             texts_;
    std::vector<Rule>             rules_;
    std::vector<juce::Range<int>> rowY_;
    std::vector<juce::Rectangle<int>> nameArea_;
    std::vector<juce::Rectangle<int>> lockArea_;
    juce::Rectangle<int>          syncArea_[3];   // POINT 1's x / y / z sync icons
    bool                          selLocked_ = false;
    juce::TextEditor              renameEd_;
    ClickAwayWatcher              clickAway_;
    int  renaming_  = -1;
    int  sel_       = -1;
    int  contentH_  = 0;
    bool building_  = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VenuePanel)
};
