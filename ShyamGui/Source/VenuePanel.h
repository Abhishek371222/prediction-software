#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"
#include "UiChrome.h"
#include "AppSettings.h"
#include "VenueTab.h"
#include "SourcePanel.h"

// Figma "Venue screen - 1.2" colours. Every colour this panel paints comes
// from here; the value boxes and dropdowns share SourcePanel's #D9D9D9.
namespace VenueSkin
{
    inline juce::Colour panelBg()   { return juce::Colour (0xffffffff); }
    inline juce::Colour rowBg()     { return juce::Colour (0xfff0f0f0); }
    inline juce::Colour rowSel()    { return juce::Colour (0xffebb2b3); }
    inline juce::Colour rowRule()   { return juce::Colour (0x33000000); }
    inline juce::Colour pointRule() { return juce::Colour (0xff999999); }
    inline juce::Colour edge()      { return juce::Colour (0xff000000); }
    inline juce::Colour ink()       { return juce::Colour (0xff000000); }
}

// The look of this panel's dropdowns while they are OPEN.
//
// A ComboBox's popup is drawn by its LookAndFeel, not by the box's own colour
// IDs, so styling the box left the list that drops out of it in the default
// scheme. Sampled from the reference: the list is the same grey as a value
// box, and the row under the cursor carries the same pink as a selected
// element row - one highlight colour for "this is the one", wherever it is.
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
        // panel. A per-component colour (the selected row's pink) still wins
        // over these, so setting a default here is safe.
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
        g.fillAll (kList);
        g.setColour (VenueSkin::edge().withAlpha (0.45f));
        g.drawRect (0, 0, w, h, 1);
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

    VenuePanel()
    {
        setLookAndFeel (&lnf_);

        // No "Elements" header here: the view strip directly above this panel
        // is that header, plus and chevron included. See VenueView.
        headSet_ = std::make_unique<SourcePanel::SectionHead> ("Element Settings", false);
        addAndMakeVisible (*headSet_);

        name_ = std::make_unique<SourcePanel::FieldBox> (juce::String());
        name_->setStyle (12.0f, 12.1f);
        name_->onTextChange = [this]
        {
            if (! building_ && onName && sel_ >= 0) onName (sel_, name_->getText());
        };
        addAndMakeVisible (*name_);

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

    void mouseUp (const juce::MouseEvent& ev) override
    {
        for (size_t i = 0; i < rowY_.size(); ++i)
            if (ev.y >= rowY_[i].getStart() && ev.y < rowY_[i].getEnd())
            {
                const int idx = (int) i;
                if (onSelect) onSelect (idx);
                if (ev.mods.isPopupMenu() && onDelete != nullptr)
                {
                    juce::PopupMenu m;
                    m.addItem (1, "Delete");
                    auto cb = onDelete;
                    const juce::Rectangle<int> at (ev.getScreenX(), ev.getScreenY(), 1, 1);
                    m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (at),
                                     [cb, idx] (int r) { if (r == 1) cb (idx); });
                }
                return;
            }
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (VenueSkin::panelBg());

        // Every element row carries a band: #F0F0F0 for the rest, the pink
        // for the one being edited.
        for (size_t i = 0; i < rowY_.size(); ++i)
        {
            const auto r = rowY_[i];
            g.setColour ((int) i == sel_ ? VenueSkin::rowSel() : VenueSkin::rowBg());
            g.fillRect (0, r.getStart(), getWidth(), r.getLength());
        }

        g.setColour (VenueSkin::ink());
        for (const auto& t : texts_)
        {
            g.setFont (Brand::figMed (t.size));
            g.drawText (t.text, t.area, juce::Justification::centredLeft, true);
        }

        if (rows_.empty())
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
        texts_.clear(); rules_.clear(); rowY_.clear();

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
            text (r->name, fig (14.0f), y, fig (190.0f), rowH);
            text ("Type",  fig (212.0f), y, fig (55.0f), rowH);
            r->type->setBounds (box (269.0f, y + ctlY, 117.0f, 28.0f));
            rowY_.push_back ({ y, y + rowH });
            rules_.push_back ({ y, false });
            rules_.push_back ({ y + rowH - 1, false });
            y += pitch;
        }

        // The list area is 158 tall in the drawing: two rows and white space.
        const int listH = juce::jmax (fig (158.0f), (int) rows_.size() * pitch + fig (72.0f));
        y = listH;

        // --- Element Settings header: 44 tall.
        headSet_->setBounds (0, y, getWidth(), rowH);
        rules_.push_back ({ y, false });
        y += pitch;

        const bool has = sel_ >= 0 && (size_t) sel_ < rows_.size();

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
                text (juce::String ("Pos. ") + (a == 0 ? "x" : a == 1 ? "y" : "z"),
                      fig (colX[a]), top + fig (35.0f), fig (109.1f), lineH);
                auto& fld = pt_[(size_t) (c * 3 + a)];
                fld->setUnit (u);
                fld->setBounds (box (colX[a], top + fig (50.0f), 109.1f, 28.0f));
                fld->setEnabled (has);
            }
            y = top + fig (87.0f);
        }
        rules_.push_back ({ y, true });

        name_->setEnabled (has);
        audH_->setEnabled (has && selHasAudH_);

        contentH_ = y + fig (12.0f);
    }

private:
    struct Row
    {
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
            auto& t = *rows_[i]->type;
            t.setSelectedId (e[i].type == VenueElement::Type::Structure ? 2
                           : e[i].type == VenueElement::Type::Object    ? 3 : 1,
                             juce::dontSendNotification);
            // The selected row's dropdown takes the row's pink with a white
            // outline, as drawn.
            if ((int) i == sel_)
            {
                t.setColour (juce::ComboBox::backgroundColourId, VenueSkin::rowSel());
                t.setColour (juce::ComboBox::outlineColourId, juce::Colours::white);
            }
            else
            {
                t.removeColour (juce::ComboBox::backgroundColourId);
                t.removeColour (juce::ComboBox::outlineColourId);
            }
            t.repaint();
        }

        if (sel_ >= 0 && (size_t) sel_ < e.size())
        {
            const auto& s = e[(size_t) sel_];
            headSet_->setTitle ("Element Settings (" + s.name + ")");
            name_->setValue (s.name);
            selHasAudH_ = s.hasAudHeight();
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
    VenueComboLnF                          comboLnf_;
    std::vector<std::unique_ptr<SourcePanel::FieldBox>> pt_;

    std::vector<Text>             texts_;
    std::vector<Rule>             rules_;
    std::vector<juce::Range<int>> rowY_;
    int  sel_       = -1;
    int  contentH_  = 0;
    bool building_  = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VenuePanel)
};
