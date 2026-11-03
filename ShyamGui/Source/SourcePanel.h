#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"
#include "AcousticEngine.h"
#include "RiggingPoints.h"

// =============================================================================
// The panel's type scale, in Figma px.
//
// The Figma set its field labels at 12 and its small captions at 10. Those
// held up on the mock's own full-size canvas and stopped being readable the
// moment the panel was scaled onto a 13" screen - fig() scales them down with
// everything else, and 10 px of Montserrat at that scale is a grey smudge.
//
// Field names and the values beside them are the whole job of this panel, so
// they lead. The one place that stays small is the Rigging Angles table, which
// is a dense grid of narrow columns: making its headings as large as a field
// label would push "Hor. Rot." out of its own column.
//
// Every size in the panel comes from here, so a further nudge is one number.
// =============================================================================
namespace PanelType
{
    // Figma Medium sizes, read off the text boxes of Source screen 1.3.
    inline constexpr float field   = 12.0f;   // System, Hor. Aim, Pos. X, Slope
    inline constexpr float caption = 12.0f;
    inline constexpr float value   = 12.0f;   // what a combo or number box shows
    // The Rigging Angles number boxes. The Figma drew these at 9, which is
    // smaller than anything else in the panel and unreadable once fig()
    // scales it onto a 13" screen - "0 dB" was a smudge. They read at the
    // same size as every other value now; the insets below came down to pay
    // for the width.
    inline constexpr float dense   = 12.0f;
    inline constexpr float column  = 10.0f;   // Cab no., Gain, Delay, Pol., Mute
    inline constexpr float twoLine = 10.0f;   // "Crossover Frequency" over two lines
}

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

    /** The combo's arrow: 11.2 x 5.8, centred 17.4 px in from the right end. */
    static void drawDropArrow (juce::Graphics& g, int w, int h, bool enabled)
    {
        const float hw = UiConfig::Scale::figf (5.6f), hh = UiConfig::Scale::figf (2.9f);
        const float cx = (float) w - UiConfig::Scale::figf (17.4f), cy = (float) h * 0.5f;
        juce::Path t;
        t.addTriangle (cx - hw, cy - hh, cx + hw, cy - hh, cx, cy + hh);
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
        juce::Font getPopupMenuFont() override { return Brand::figMed (PanelType::value); }

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
            return Brand::figMed (PanelType::value);
        }

        void positionComboBoxText (juce::ComboBox& box, juce::Label& l) override
        {
            // Figma: the value is 9 px in, clear of the 28 px arrow cell.
            using UiConfig::Scale::fig;
            l.setBorderSize ({});
            l.setBounds (fig (9.0f), 0, box.getWidth() - fig (9.0f) - fig (28.0f), box.getHeight());
            l.setFont (getComboBoxFont (box));
            l.setColour (juce::Label::textColourId,
                         Brand::text().withMultipliedAlpha (box.isEnabled() ? 1.0f : 0.45f));
        }
    };

    // A filter slope. The closed box has room only for "LR 4 Ord."; the list
    // spells each one out, as Figma's Slope drop-down does.
    class SlopeCombo : public juce::ComboBox
    {
    public:
        SlopeCombo()
        {
            for (int i = 0; i < 4; ++i)
                addItem (juce::String (i < 2 ? "BW " : "LR ") + (i % 2 == 0 ? "2" : "4") + " Ord.", i + 1);
        }

        // The text colour is set where the text is placed, so re-place it.
        void enablementChanged() override
        {
            juce::ComboBox::enablementChanged();
            resized();
        }

        void showPopup() override
        {
            juce::PopupMenu m;
            for (int i = 0; i < 4; ++i)
                m.addItem (i + 1, juce::String (i < 2 ? "BW" : "LR") + " - " + (i % 2 == 0 ? "2" : "4") + " Order",
                           true, getSelectedId() == i + 1);
            auto& lf = getLookAndFeel();
            m.setLookAndFeel (&lf);
            auto opts = juce::PopupMenu::Options().withTargetComponent (this).withMinimumWidth (getWidth());
            for (auto* c : getChildren())
                if (auto* l = dynamic_cast<juce::Label*> (c))
                    opts = lf.getOptionsForComboBoxPopupMenu (*this, *l);
            juce::Component::SafePointer<juce::ComboBox> safe (this);
            m.showMenuAsync (opts, [safe] (int r) { if (safe != nullptr && r > 0) safe->setSelectedId (r); });
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

        /** Where the fold arrow and the plus sit, in Figma px in from the
            right edge, and the arrow's size. Source screen 1.3: the 24 px
            "Sort Down" at x 362 and the 20 px "Plus Math" at x 322. */
        void setGlyphs (float arrowIn, float arrowW, float arrowH, float plusIn)
        {
            arrowIn_ = arrowIn; arrowW_ = arrowW; arrowH_ = arrowH; plusIn_ = plusIn;
            repaint();
        }

        /** An extra toggle in the header, left of the fold arrow. Rigging
            Angles carries the array symmetry mirror there, which is where
            ArrayCalc keeps its own pair button - beside the thing it acts on
            rather than in a row of its own below the table. */
        std::function<void (bool on)> onToggle;
        void setToggle (bool shown, bool on)
        {
            if (toggleShown_ == shown && toggleOn_ == on) return;
            toggleShown_ = shown; toggleOn_ = on;
            repaint();
        }
        bool isToggleOn() const noexcept { return toggleOn_; }

        bool isFolded() const { return folded_; }
        /** Fold from outside - the strip's chevron does this. Silent: the
            caller already knows, and onFold would send it straight back. */
        void setFolded (bool f) { if (folded_ != f) { folded_ = f; repaint(); } }

        // Figma: the plus is centred in the 44 px cell at x 308..352, the fold
        // arrow in 352..396 - 66 and 22 px in from the right edge.
        void mouseUp (const juce::MouseEvent& e) override
        {
            using UiConfig::Scale::fig;
            if (toggleShown_ && onToggle != nullptr
                && e.x > getWidth() - fig (88.0f)
                && e.x < getWidth() - fig (44.0f))
            {
                toggleOn_ = ! toggleOn_;
                repaint();
                onToggle (toggleOn_);
            }
            else if (hasPlus_ && onPlus != nullptr
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
            float x = (float) getWidth() - figf (arrowIn_);
            const float tw = figf (arrowW_), th = figf (arrowH_);
            juce::Path p;
            p.addTriangle (x - tw * 0.5f, h * 0.5f - th * 0.5f,
                           x + tw * 0.5f, h * 0.5f - th * 0.5f,
                           x,             h * 0.5f + th * 0.5f);
            if (folded_)
                p.applyTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi,
                                                                   x, h * 0.5f));
            g.fillPath (p);
            // The mirror: two panels facing each other across a centre line,
            // filled when symmetry is on and outlined when it is off. The
            // same reading as ArrayCalc's pair button.
            if (toggleShown_)
            {
                const float cx = (float) getWidth() - figf (66.0f);
                const float cy = h * 0.5f;
                const float pw = figf (6.0f), ph = figf (11.0f), gap = figf (2.5f);
                const juce::Rectangle<float> lhs (cx - gap - pw, cy - ph * 0.5f, pw, ph);
                const juce::Rectangle<float> rhs (cx + gap,      cy - ph * 0.5f, pw, ph);
                g.setColour (Brand::text().withAlpha (toggleOn_ ? 1.0f : 0.55f));
                if (toggleOn_) { g.fillRect (lhs); g.fillRect (rhs); }
                else           { g.drawRect (lhs, 1.0f); g.drawRect (rhs, 1.0f); }
                g.fillRect (cx - 0.5f, cy - ph * 0.65f, 1.0f, ph * 1.3f);
            }

            if (hasPlus_)
            {
                x = (float) getWidth() - figf (plusIn_);
                const float a = figf (15.0f), t = juce::jmax (1.2f, figf (2.0f));
                g.fillRect (x - t * 0.5f, h * 0.5f - a * 0.5f, t, a);
                g.fillRect (x - a * 0.5f, h * 0.5f - t * 0.5f, a, t);
            }
        }

    private:
        juce::String title_;
        bool         hasPlus_;
        bool         folded_ = false;
        bool         toggleShown_ = false, toggleOn_ = false;
        float        arrowIn_ = 22.0f, arrowW_ = 14.0f, arrowH_ = 8.0f, plusIn_ = 66.0f;
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

        /** How far one wheel notch moves this box. 0 - the default - means
            the box takes no wheel and the panel scrolls instead. */
        void setWheelStep (double step) { wheelStep_ = step; }

        /** Restrict this box to a fixed set of values, the way ArrayCalc's
            Splay column is a dropdown of the angles the cabinet's link
            mechanically allows rather than a number you can type.

            Given a non-empty list the box stops being typeable: a click opens
            the list, the wheel steps through it, and nothing off the list can
            be entered. Clearing it makes the box a free number again. */
        void setAllowedValues (std::vector<double> v, int decimals = 0)
        {
            allowed_ = std::move (v);
            allowedDp_ = decimals;
            const bool closed = ! allowed_.empty();
            setEditable (! closed, ! closed, false);
            repaint();
        }

        /** The listed value nearest @p v, or @p v when there is no list. */
        double nearestAllowed (double v) const
        {
            if (allowed_.empty()) return v;
            double best = allowed_.front();
            for (double a : allowed_)
                if (std::abs (a - v) < std::abs (best - v)) best = a;
            return best;
        }

        /** The wheel nudges the number, where the box has one to nudge.

            Rolling over a field is how everyone expects to trim a gain or an
            angle, and it beats selecting the text to type one character. A box
            that cannot take it - no step set, disabled, or mid-edit - hands
            the wheel back to the panel so the sidebar still scrolls. */
        void mouseDown (const juce::MouseEvent& e) override
        {
            if (allowed_.empty() || ! isEnabled()) { juce::Label::mouseDown (e); return; }

            juce::PopupMenu m;
            const double now = getText().getDoubleValue();
            for (int i = 0; i < (int) allowed_.size(); ++i)
                m.addItem (i + 1, juce::String (allowed_[(size_t) i], allowedDp_)
                                      + (unit_.isEmpty() ? juce::String() : " " + unit_),
                           true, std::abs (allowed_[(size_t) i] - now) < 1.0e-6);

            m.showMenuAsync (juce::PopupMenu::Options()
                                 .withTargetComponent (this)
                                 .withMinimumWidth (getWidth()),
                             [safe = juce::Component::SafePointer<FieldBox> (this)] (int r)
                             {
                                 if (safe == nullptr || r <= 0) return;
                                 if (r > (int) safe->allowed_.size()) return;
                                 safe->commitValue (safe->allowed_[(size_t) (r - 1)]);
                             });
        }

        /** Put @p v in the box and tell the scene, in one place so the list,
            the wheel and the spinner cannot disagree about the format. */
        void commitValue (double v)
        {
            const int dp = ! allowed_.empty() ? allowedDp_
                         : wheelStep_ >= 1.0 ? 0 : (wheelStep_ >= 0.1 ? 1 : 2);
            setText (juce::String (v, dp), juce::dontSendNotification);
            if (onCommit) onCommit (v);
            repaint();
        }

        /** One notch on the value. True when it took the wheel. */
        bool wheelNudge (const juce::MouseWheelDetails& w)
        {
            const int dir = w.deltaY > 0.0f ? 1 : (w.deltaY < 0.0f ? -1 : 0);
            if (dir == 0 || wheelStep_ <= 0.0 || ! isEnabled() || onCommit == nullptr)
                return false;

            // A closed list steps from one listed value to the next, never
            // by the raw step - the whole point is that nothing between them
            // can be reached.
            if (! allowed_.empty())
            {
                const double now = nearestAllowed (getText().getDoubleValue());
                int at = 0;
                for (int i = 0; i < (int) allowed_.size(); ++i)
                    if (std::abs (allowed_[(size_t) i] - now) < 1.0e-6) at = i;
                const int want = juce::jlimit (0, (int) allowed_.size() - 1, at + dir);
                if (want != at) commitValue (allowed_[(size_t) want]);
                return true;
            }

            // While the editor is open the value being nudged is the one in
            // it, not the one behind it - otherwise a half-typed entry would
            // be thrown away by the first notch.
            auto* ed = getCurrentTextEditor();
            const double now = ed != nullptr ? ed->getText().getDoubleValue()
                                             : getText().getDoubleValue();

            // Rounded to the step's own precision, or 0.1 steps would walk
            // off into 0.30000000000000004 after three notches.
            const int    dp      = wheelStep_ >= 1.0 ? 0 : (wheelStep_ >= 0.1 ? 1 : 2);
            const double rounded = std::round ((now + dir * wheelStep_) / wheelStep_) * wheelStep_;
            const auto   text    = juce::String (rounded, dp);

            if (ed != nullptr)
            {
                // Show it in the editor and commit it, so the scene keeps up
                // with the wheel instead of waiting for Enter.
                ed->setText (text, false);
                onCommit (rounded);
            }
            else
            {
                setText (text, juce::sendNotification);
            }
            return true;
        }

        void mouseWheelMove (const juce::MouseEvent& e,
                             const juce::MouseWheelDetails& w) override
        {
            // Rolling over a closed box scrolls the sidebar: a panel made of
            // fields otherwise caught the wheel on every row and could not be
            // scrolled past them. Click into a box to trim it with the wheel.
            if (getCurrentTextEditor() != nullptr && wheelNudge (w)) return;

            if (auto* p = getParentComponent())
                p->mouseWheelMove (e.getEventRelativeTo (p), w);
        }

        /** The editor this box types into, which hands the wheel back.

            A TextEditor eats the wheel to scroll its own text, so a box you
            had clicked into ignored it. Routing through the editor itself -
            rather than hanging a mouse listener on it - means the nudge runs
            EXACTLY once per notch; a listener registered for nested children
            fired once per level of the editor's own hierarchy, which turned
            one notch into fifteen. */
        class WheelEditor : public juce::TextEditor
        {
        public:
            explicit WheelEditor (FieldBox& owner) : owner_ (owner) {}

            void mouseWheelMove (const juce::MouseEvent& e,
                                 const juce::MouseWheelDetails& w) override
            {
                if (! owner_.wheelNudge (w))
                    juce::TextEditor::mouseWheelMove (e, w);
            }

        private:
            FieldBox& owner_;
        };

        juce::TextEditor* createEditorComponent() override
        {
            auto* ed = new WheelEditor (*this);
            ed->applyFontToAllText (getLookAndFeel().getLabelFont (*this));
            copyAllExplicitColoursTo (*ed);
            return ed;
        }

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
            if (isBeingEdited() || getText() == v) return;
            // A listed box shows the nearest listed value, so a scene that
            // carries something off the list (an older project, or an angle
            // set from the plan) still reads as one of the allowed ones.
            setText (allowed_.empty() ? v
                                      : juce::String (nearestAllowed (v.getDoubleValue()), allowedDp_),
                     juce::dontSendNotification);
        }

        /** "2 Deg." read as one string after the value, rather than the unit
            pinned to the right edge; with @p arrow the box wears the combo's
            arrow, as the Figma's aim and angle boxes do. */
        void setInlineUnit (bool inlineUnit, bool arrow)
        {
            inline_ = inlineUnit; arrow_ = arrow;
            repaint();
        }

        /** The Figma's narrow angle cell: a small up/down pair in place of
            the combo arrow, so the value keeps the width. */
        void setSpinner (bool on) { spin_ = on; repaint(); }

        /** Figma fills the Rigging Angles cells #E8E8E8, a step lighter than
            the other value boxes. */
        void setFill (juce::Colour c) { fill_ = c; repaint(); }

        void paint (juce::Graphics& g) override
        {
            g.setColour (isEnabled() ? fill_ : fill_.withAlpha (0.55f));
            g.fillRect (getLocalBounds());
            if (isBeingEdited()) return;

            using UiConfig::Scale::fig;
            const auto f = Brand::figMed (fontPx_);
            g.setFont (f);
            g.setColour (Brand::text().withMultipliedAlpha (isEnabled() ? 1.0f : 0.45f));
            if (inline_)
            {
                const int arrowW = spin_ ? fig (11.0f) : arrow_ ? fig (24.0f) : 0;
                g.drawText (getText() + (unit_.isEmpty() ? juce::String() : " " + unit_),
                            getLocalBounds().withTrimmedLeft (fig (inset_)).withTrimmedRight (arrowW),
                            juce::Justification::centredLeft, true);
                if (spin_)
                {
                    const float cx = (float) getWidth() - (float) fig (6.0f);
                    const float cy = (float) getHeight() * 0.5f;
                    const float hw = (float) fig (3.0f), hh = (float) fig (3.0f), gap = (float) fig (1.5f);
                    juce::Path p;
                    p.addTriangle (cx - hw, cy - gap, cx + hw, cy - gap, cx, cy - gap - hh);
                    p.addTriangle (cx - hw, cy + gap, cx + hw, cy + gap, cx, cy + gap + hh);
                    g.fillPath (p);
                }
                else if (arrow_) drawDropArrow (g, getWidth(), getHeight(), isEnabled());
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
        float fontPx_ = PanelType::value;
        double wheelStep_ = 0.0;
        std::vector<double> allowed_;
        int    allowedDp_ = 0;
        float inset_  = 9.0f;
        bool  inline_ = false, arrow_ = false, spin_ = false;
        juce::Colour fill_ = fieldFill();
    };

    // M and O-slash. They latch, and they show it - a marker that does
    // something has to look different once it has, or a muted source reads
    // exactly like a live one.
    class SquareMark : public juce::Button
    {
    public:
        explicit SquareMark (const juce::String& glyph)
            : juce::Button (glyph), glyph_ (glyph) { setClickingTogglesState (true); }

        /** The wheel is the panel's, not this mark's. Without this the
            Rigging Angles table only scrolled where the pointer happened to
            be over bare panel rather than over one of the row's controls -
            which is most of the table. */
        void mouseWheelMove (const juce::MouseEvent& e,
                             const juce::MouseWheelDetails& w) override
        {
            if (auto* p = getParentComponent())
                p->mouseWheelMove (e.getEventRelativeTo (p), w);
        }

        /** Figma's Rigging Angles Pol. and Mute: white, in a 1 px black edge,
            where the Sources list's M is a flat grey square. */
        void setOutlined (bool o) { outlined_ = o; repaint(); }

        void paintButton (juce::Graphics& g, bool hl, bool) override
        {
            // Figma: a flat grey square like the value boxes; latched, red.
            const auto r = getLocalBounds().toFloat();
            const bool on = getToggleState();
            const auto base = outlined_ ? Brand::white() : fieldFill();
            g.setColour (on ? Brand::accent()
                            : (hl ? base.darker (0.06f) : base));
            g.fillRect (r);
            if (outlined_ && ! on)
            {
                g.setColour (Brand::text().withMultipliedAlpha (isEnabled() ? 1.0f : 0.45f));
                g.drawRect (getLocalBounds(), 1);
            }
            g.setColour (on ? Brand::onAccent()
                            : Brand::text().withMultipliedAlpha (isEnabled() ? 1.0f : 0.45f));
            g.setFont (Brand::figMed (12.0f));
            g.drawText (glyph_, getLocalBounds(), juce::Justification::centred, false);
        }

    private:
        juce::String glyph_;
        bool outlined_ = false;
    };

    // A latching tick: white square in a 1 px black edge, red with a white
    // tick when on - the Rigging Angles marks' look, for a yes/no setting.
    class TickBox : public juce::Button
    {
    public:
        TickBox() : juce::Button ("tick") { setClickingTogglesState (true); }

        void mouseWheelMove (const juce::MouseEvent& e,
                             const juce::MouseWheelDetails& w) override
        {
            if (auto* p = getParentComponent())
                p->mouseWheelMove (e.getEventRelativeTo (p), w);
        }

        void paintButton (juce::Graphics& g, bool hl, bool) override
        {
            const auto r = getLocalBounds().toFloat();
            const bool on = getToggleState();
            g.setColour (on ? Brand::accent() : (hl ? Brand::white().darker (0.06f) : Brand::white()));
            g.fillRect (r);
            if (! on)
            {
                g.setColour (Brand::text().withMultipliedAlpha (isEnabled() ? 1.0f : 0.45f));
                g.drawRect (getLocalBounds(), 1);
                return;
            }
            juce::Path tick;
            tick.startNewSubPath (r.getX() + r.getWidth() * 0.25f, r.getCentreY());
            tick.lineTo (r.getX() + r.getWidth() * 0.43f, r.getY() + r.getHeight() * 0.68f);
            tick.lineTo (r.getX() + r.getWidth() * 0.76f, r.getY() + r.getHeight() * 0.32f);
            g.setColour (Brand::onAccent());
            g.strokePath (tick, juce::PathStrokeType (juce::jmax (1.5f, r.getWidth() * 0.11f),
                                                      juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
        }
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
            // Figma: one Medium 12 line across x 14..386 - the state on the
            // left, the share against the right end.
            using UiConfig::Scale::fig;
            g.setColour (Brand::white());
            g.setFont (Brand::figMed (12.0f));
            const auto line = getLocalBounds().withTrimmedLeft (fig (14.0f))
                                              .withTrimmedRight (fig (14.0f));
            g.drawText (state_, line, juce::Justification::centredLeft, true);
            g.drawText (shareText_, line, juce::Justification::centredRight, true);
        }

    private:
        juce::String state_     = "LOAD OKAY";
        juce::String shareText_;
        float        share_     = -1.0f;
    };

    //-------------------------------------------------------------- panel
    SourcePanel() { setLookAndFeel (&lnf_); buildFixed(); }
    ~SourcePanel() override { setLookAndFeel (nullptr); }

    /** Most cabinets one source can hold. */
    static constexpr int kMaxCabinets = 48;

    // --- what the panel tells the scene -----------------------------------
    std::function<void (int index)>                   onSelect;
    std::function<void (int index, float x, float y)> onPosition;
    std::function<void (int index, float deg)>        onRotation;
    std::function<void (int index, float deg)>        onTilt;
    std::function<void (int index, float metres)>     onHeight;
    /** The selected source's ground-row spacing, centre to centre, metres. */
    std::function<void (int index, float metres)>     onInterdistance;
    /** The selected source's deployment: Mount::Ground, Stacked or Flown. */
    std::function<void (int index, int mount)>        onMount;
    /** One box's angle from its neighbour, in tilt units (positive down). */
    std::function<void (int index, float splayDeg)>   onSplay;
    /** Array Symmetry switched on the selected source. */
    std::function<void (int index, bool on)>          onSymmetric;
    /** The Sources list's M: the whole source muted or live. */
    std::function<void (int index, bool on)>          onSourceEnabled;
    std::function<void (int index, int model)>        onModel;
    /** The selected source's cabinet count changed. */
    std::function<void (int sourceIndex, int count)>  onCabinetCount;
    std::function<void (int index, bool on)>          onEnabled;
    std::function<void (int index, bool inverted)>    onPolarity;
    /** Hor. Aim: the selected source's whole array turned to this heading. */
    std::function<void (int index, float deg)>        onArrayRotation;
    std::function<void (int index, float dB)>         onGain;
    std::function<void (int index, float ms)>         onDelay;
    std::function<void()>                             onAddSource;
    std::function<void (int index)>                   onDeleteSource;
    // Listening planes share the Sources list with the cabinets: a
    // plane is a thing you add from the same "+", so it belongs in the
    // same list rather than in a second one nobody thinks to look at.
    std::function<void (int planeIndex)>              onSelectPlane;
    std::function<void (int planeIndex)>              onDeletePlane;
    std::function<void (int index, const SpeakerFilter&)> onFilter;

    /** The Mapping tab's version of the panel: Filters in place of Rigging
        Points, as the MAPPING V1.1 frame draws it. */
    void setMappingLayout (bool on)
    {
        if (mapping_ == on) return;
        mapping_ = on;
        refold();
    }

    /** Rebuild from the scene. Cheap enough for every change. */
    void setScene (const std::vector<Speaker>& sp, int selected)
    {
        scene_ = sp;
        if (! rowsMatch (sp))
            buildRows (sp);
        selected_ = juce::jlimit (-1, (int) sp.size() - 1, selected);
        // The panel edits SOURCES: whichever box was picked, it works from
        // its source's first cabinet - the row the list shows.
        if (selected_ >= 0)
            if (const auto m = arrayMembers (sp, selected_); ! m.empty()) selected_ = m.front();
        wireCabinetCount();
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
        resized();
        repaint();
    }

    /** Re-stamp every unit suffix and re-read the values in the new unit.
        Called when AppSettings broadcasts a change. */
    void refreshUnits()
    {
        const juce::String u = Units::lengthUnit();
        const double step = lengthWheelStep();
        for (auto& f : posXYZ_)
            if (f != nullptr) { f->setUnit (u); f->setWheelStep (step); }
        if (posInterdist_ != nullptr) { posInterdist_->setUnit (u); posInterdist_->setWheelStep (step); }
        resized();
        repaint();
    }

    /** One wheel notch on a LENGTH field, in whatever unit is showing.

        It is the app's own snap increment - 100 mm in SI, a foot in Imperial
        - rather than a flat 0.1. A tenth of a FOOT is 30 mm, which is finer
        than the 0.1 m the Ortho gap quantises to, so an Imperial notch on
        Interdistance rounded straight back to where it started and the field
        looked broken. Tying the step to the snap increment keeps every notch
        big enough to land somewhere new, in either unit. */
    static double lengthWheelStep()
    {
        return juce::jmax (0.01, Units::metresToDisplay (Units::snapStepMetres()));
    }

    /** The Sources list, folded away or shown. Its header is hidden - the
        view strip above the panel IS that header - so the strip's chevron is
        the only thing that can fold it, and it drives this. */
    void setListFolded (bool f)
    {
        if (headSrc_ == nullptr || headSrc_->isFolded() == f) return;
        headSrc_->setFolded (f);
        refold();
    }
    bool isListFolded() const noexcept
    {
        return headSrc_ != nullptr && headSrc_->isFolded();
    }

    int getContentHeight() const { return contentH_; }

    void mouseDown (const juce::MouseEvent& ev) override
    {
        const auto e = ev.getEventRelativeTo (this);

        // The grip gets first refusal: it sits on a divider that is otherwise
        // part of the header, and a drag starting there is a resize.
        if (overAngleGrip (e))
        {
            angDragging_    = true;
            angDragY0_      = e.getScreenPosition().y;
            angDragOffset0_ = angOffsetY_;
            repaint();
            return;
        }

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

    /** Over the grip the pointer says so; a drag there resizes the table
        rather than doing whatever the panel would otherwise do. */
    void mouseMove (const juce::MouseEvent& e) override
    {
        setMouseCursor (overAngleGrip (e) ? juce::MouseCursor::UpDownResizeCursor
                                          : juce::MouseCursor::NormalCursor);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! angDragging_) return;

        // Measured on the SCREEN, not in the panel. Moving the box changes
        // the panel's height, which can slide it under the pointer - a delta
        // taken in panel coordinates would feed back on itself and run away.
        const int dy = e.getScreenPosition().y - angDragY0_;

        const int want = juce::jlimit (0, angOffsetLimit(), angDragOffset0_ + dy);
        if (want == angOffsetY_) return;

        angOffsetY_ = want;
        relayoutAfterResize();
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (angDragging_)
        {
            angDragging_ = false;
            repaint();
            return;
        }

        if (e.mods.isPopupMenu() || e.eventComponent != this) return;

        // Clicking a source row selects it, as clicking a unit on the plan does.
        for (size_t i = 0; i < sourceRowY_.size(); ++i)
            if (e.y >= sourceRowY_[i].getStart() && e.y < sourceRowY_[i].getEnd())
            {
                if (onSelect) onSelect ((int) i);
                return;
            }

        // A plane row answers to the same click; once selected, Backspace or
        // the row's right-click removes it.
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
            // 0.9, not 1.0: a label that overruns its cell by a hair now
            // narrows by a few per cent instead of losing its last letter.
            // At 1.0 "No. of Cabinets" came out as "Cabinet".
            g.drawFittedText (t.text, t.area, t.just, 2, 0.9f);
        }

        // The splitter grip: three short rules centred on the divider above
        // the Rigging Angles header, the way the divider between ArrayCalc's
        // stacked boxes is drawn.
        if (! angGripY_.isEmpty())
        {
            const float cy = (float) angGripY_.getStart() + angGripY_.getLength() * 0.5f;
            const float cx = (float) getWidth() * 0.5f;
            const float w  = UiConfig::Scale::figf (9.0f);
            g.setColour (Brand::text().withAlpha (angDragging_ ? 0.55f : 0.3f));
            for (int k = -1; k <= 1; ++k)
                g.fillRect (cx + (float) k * (w + 3.0f) - w * 0.5f, cy - 0.5f, w, 1.0f);
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

        // Every number below is a Figma pixel from "Source screen_FOR TOP -
        // 1.3" (panel x 0..400, panel y = frame y - 88), mapped onto the
        // window by fig(). The Mapping tab follows "MAPPING_RIGGING FOR TOP
        // V1.1" where the two frames differ. Heads are 32 tall and rows 44,
        // but each is pitched a pixel or two less - their 1 px edges overlap.
        using UiConfig::Scale::fig;
        const int W    = getWidth();
        const int hdrH = fig (32.0f);
        const int ctlH = fig (28.0f);
        int rowH = fig (43.0f);
        int ctlY = fig (7.0f);
        int y = 0;

        auto label = [this] (const juce::String& t, juce::Rectangle<int> a,
                             float size = PanelType::field, bool bold = false, bool dim = false)
        { texts_.push_back ({ t, a, size, bold, dim, juce::Justification::centredLeft }); };
        auto caption = [this] (const juce::String& t, juce::Rectangle<int> a)
        { texts_.push_back ({ t, a, PanelType::column, false, false, juce::Justification::centred }); };
        auto ctl = [&] (juce::Component& c, float x, float w)
        { c.setBounds (fig (x), y + ctlY, fig (w), ctlH); };
        auto head = [&] (SectionHead& h, float pitch = 31.0f)
        { h.setBounds (0, y, W, hdrH); y += fig (pitch); return ! h.isFolded(); };
        // The next row's pitch and how far down it its controls sit.
        auto rowAt = [&] (float pitch, float down) { rowH = fig (pitch); ctlY = fig (down); };
        auto row = [&] (bool striped)
        {
            if (striped) bands_.push_back ({ y, y + rowH });
            y += rowH;
            rowRules_.push_back (y - 1);
        };
        auto gap = [&] (float figPx) { y += fig (figPx); };
        auto show = [] (bool on, std::initializer_list<juce::Component*> cs)
        { for (auto* c : cs) if (c != nullptr) c->setVisible (on); };

        // ------------------------------------------------------- Sources
        // Name at 14, "System" at 179, the combo 244..354, the mute 363.6..391.9.
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
        if (srcOpen)
        {
            const int listTop = y;
            rowAt (43.0f, 7.0f);
            for (size_t i = 0; i < sources_.size(); ++i)
            {
                if (! isHead ((int) i)) { sourceRowY_.push_back ({ y, y }); continue; }
                auto& s = sources_[i];
                const int n = (int) arrayMembers (scene_, (int) i).size();
                // A source carries its cabinet count, as ArrayCalc's list does.
                label (s->name + (n > 1 ? "  (Arrays_" + juce::String (n) + ")" : juce::String()),
                       { fig (14.0f), y, fig (160.0f), rowH });
                label ("System", { fig (179.0f), y, fig (64.0f), rowH });
                ctl (*s->system, 244.0f, 110.0f);
                ctl (*s->mute,   363.6f, 28.3f);
                sourceRowY_.push_back ({ y, y + rowH });
                y += rowH;
                listRules_.push_back (y - 1);
            }
            // Name only: what a plane is, and its Delete, live on the Venue tab
            // and on the row's right-click.
            for (auto& p : planes_)
            {
                label (p->name, { fig (14.0f), y, fig (160.0f), rowH });
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
            // Source Positioning's head at frame y 246 (Source) or 266
            // (Mapping) - the list keeps that room however few rows it has,
            // and grows past it with 29 / 49 px under its last row.
            gap (mapping_ ? 49.0f : 29.0f);
            y = juce::jmax (y, listTop + fig (mapping_ ? 178.0f : 158.0f));
        }

        // --------------------------------------------- Source Positioning
        const bool posOpen = head (*headPos_);
        show (posOpen, { posSystem_.get(), posMounting_.get(), posCabinets_.get(),
                         posInterdist_.get(),
                         posHorAim_.get(), posVerAim_.get(),
                         posXYZ_[0].get(), posXYZ_[1].get(), posXYZ_[2].get() });
        if (posOpen)
        {
            // Labels at 14 and 203; controls 81..183 and 282..384.
            rowAt (mapping_ ? 43.0f : 44.0f, 7.0f);
            label ("System",   { fig (14.0f),  y, fig (66.0f), rowH });
            ctl (*posSystem_,   81.0f, 102.0f);
            label ("Mounting", { fig (203.0f), y, fig (78.0f), rowH });
            ctl (*posMounting_, 282.0f, 102.0f);
            row (false);

            rowAt (mapping_ ? 43.0f : 42.0f, mapping_ ? 7.0f : 6.0f);
            label ("No. of\nCabinets", { fig (14.0f), y, fig (66.0f), rowH });
            ctl (*posCabinets_, 81.0f, 102.0f);
            // The right half of this row was empty, and the gap between
            // cabinets is the other half of "how many" - so it goes here,
            // beside the count rather than in a menu three clicks away.
            label ("Interdistance", { fig (203.0f), y, fig (78.0f), rowH });
            ctl (*posInterdist_, 282.0f, 102.0f);
            row (false);

            rowAt (44.0f, 7.0f);
            label ("Hor. Aim", { fig (14.0f),  y, fig (66.0f), rowH });
            ctl (*posHorAim_, 81.0f, 102.0f);
            // A sub has no vertical aim to set, so it is not offered.
            if (selectedIsSub())
            {
                posVerAim_->setVisible (false);
            }
            else
            {
                const float verX = mapping_ ? 203.0f : 226.0f;
                label ("Ver. Aim", { fig (verX), y, fig (282.0f - verX), rowH });
                ctl (*posVerAim_, 282.0f, 102.0f);
            }
            row (false);

            // Three cells split at 133 and 266: caption 5 px down, a 110-wide
            // field 19 down (Mapping: 7 and 25 in a taller cell).
            const int posH = fig (mapping_ ? 65.0f : 52.0f);
            const float capY = mapping_ ? 7.0f : 5.0f, boxY = mapping_ ? 25.0f : 19.0f;
            const char* caps[] = { "Pos. X", "Pos. Y", "Pos. Z" };
            const float boxX[] = { 12.0f, 145.0f, 278.0f };
            for (int i = 0; i < 3; ++i)
            {
                label (caps[i], { fig (boxX[i] + 2.0f), y + fig (capY), fig (106.0f), fig (15.0f) });
                posXYZ_[i]->setBounds (fig (boxX[i]), y + fig (boxY), fig (110.0f), ctlH);
            }
            vRules_.push_back ({ fig (mapping_ ? 130.0f : 133.0f), y, 1, posH });
            vRules_.push_back ({ fig (266.0f), y, 1, posH });
            y += posH;
            rowRules_.push_back (y - 1);
            if (! mapping_) gap (28.0f);
        }

        // ---------------------------------------------- Filters (Mapping)
        // Each pass is one 76 px cell: its name 14 down, then Crossover
        // Frequency (two 10 px lines) with its box at 81, Slope at 240 with
        // its box at 282.
        headFlt_->setVisible (mapping_);
        const bool fltOpen = mapping_ && head (*headFlt_, 32.0f);
        show (fltOpen, { fltLpFreq_.get(), fltLpSlope_.get(), fltHpFreq_.get(), fltHpSlope_.get() });
        if (fltOpen)
        {
            auto pass = [&] (const juce::String& name, juce::ComboBox& freq, juce::ComboBox& slope)
            {
                label (name, { fig (12.0f), y + fig (14.0f), fig (200.0f), fig (15.0f) },
                       PanelType::field, true);
                label ("Crossover\nFrequency", { fig (12.0f), y + fig (41.0f), fig (66.0f), ctlH },
                       PanelType::twoLine);
                freq.setBounds (fig (81.0f), y + fig (41.0f), fig (102.0f), ctlH);
                label ("Slope", { fig (240.0f), y + fig (40.0f), fig (40.0f), ctlH });
                slope.setBounds (fig (282.0f), y + fig (40.0f), fig (102.0f), ctlH);
                y += fig (76.0f);
                rowRules_.push_back (y - 1);
            };
            pass ("Low Pass",  *fltLpFreq_, *fltLpSlope_);
            pass ("High Pass", *fltHpFreq_, *fltHpSlope_);
        }

        // ------------------------------------------------- Rigging Points
        headRig_->setVisible (! mapping_);
        const bool rigOpen = ! mapping_ && head (*headRig_);
        show (rigOpen, { loadBar_.get(), frontPick_.get(), rearPick_.get(), singlePick_.get() });
        if (rigOpen)
        {
            loadBar_->setBounds (0, y, W, fig (44.0f)); y += fig (44.0f);

            rowAt (42.0f, 6.0f);
            label ("Front Pick", { fig (14.0f),  y, fig (82.0f), rowH });
            ctl (*frontPick_,  97.0f, 102.0f);
            label ("Rear Pick",  { fig (211.0f), y, fig (74.0f), rowH });
            rearPick_->setBounds (fig (286.0f), y + fig (7.0f), fig (102.0f), ctlH);
            row (false);

            // The lever hole is a vertical angle, which a sub does not have.
            if (selectedIsSub())
            {
                singlePick_->setVisible (false);
            }
            else
            {
                rowAt (44.0f, 7.0f);
                label ("Single Pick", { fig (14.0f), y, fig (82.0f), rowH });
                // Wider than Figma's 102, because it carries more: a hole and the
                // tilt it gives ("-15 Deg.  (B5)"), not a bare number.
                ctl (*singlePick_, 97.0f, 170.0f);
                row (false);
            }
        }

        // ------------------------------------------------- Rigging Angles
        // The divider along the top of this header is a drag handle, as it is
        // between ArrayCalc's stacked boxes: pulling it moves the WHOLE box -
        // header, column headings and every cabinet row - down the panel, and
        // opens up the space above it. Nothing is trimmed and nothing is
        // hidden; the box keeps all its rows and simply sits lower, which is
        // what the sidebar's own scroll bar is for.
        // "Source screen_FOR SUB" drops this box 28 px below Rigging Points
        // and nudges its cells a pixel; the Mapping tab keeps the TOP grid.
        const bool selSub = selected_ >= 0 && selected_ < (int) scene_.size()
                         && isSubwooferModel (scene_[(size_t) selected_].model);
        const bool subGrid = selSub && ! mapping_;
        if (subGrid) gap (28.0f);
        angNaturalTop_ = y;
        angOffsetY_ = juce::jmin (angOffsetY_, angOffsetLimit());
        y += angOffsetY_;
        const int angHeadTop = y;
        const bool angOpen = head (*headAng_);
        // Only the selected source's own boxes. The header names one source, so
        // listing the whole scene under it was the table contradicting its own
        // title.
        for (size_t i = 0; i < angles_.size(); ++i)
            show (angOpen && inSelectedArray ((int) i),
                  { angles_[i]->gain.get(), angles_[i]->delay.get(), angles_[i]->pol.get(),
                    angles_[i]->mute.get(), angles_[i]->angle.get() });
        if (angOpen)
        {
            // Figma 1.3: each caption centred over its own column. The last
            // column is a sub's own Hor. Rot., or a top's Angles - its angle
            // from the box above.
            const float labX = subGrid ? 13.0f : 14.0f;
            const float gainX = subGrid ? 142.0f : 141.0f, delayX = subGrid ? 203.0f : 202.0f,
                        polX  = subGrid ? 264.0f : 265.0f, muteX  = subGrid ? 297.0f : 298.0f,
                        angX  = subGrid ? 330.0f : 331.0f;
            rowAt (43.0f, 7.0f);
            label ("Cab No.", { fig (labX), y, fig (80.0f), rowH }, PanelType::column);
            const struct { const char* text; float x, w; } cells[] = {
                { "Gain",  gainX, 56.0f }, { "Delay", delayX, 56.0f },
                { "Pol.",  polX,  28.0f }, { "Mute",  muteX,  28.0f },
                { selSub ? "Hor. Rot." : "Angles", angX, 56.0f } };
            for (const auto& c : cells)
                caption (c.text, { fig (c.x + c.w * 0.5f - 30.0f), y, fig (60.0f), rowH });
            row (false);

            int cabNo = 0;
            for (size_t i = 0; i < angles_.size(); ++i)
            {
                if (! inSelectedArray ((int) i)) continue;
                auto& a = angles_[i];
                // Numbered within the ARRAY - cabinet 1 is its top box.
                a->num = juce::String (++cabNo);
                label (a->num + ". " + a->cab, { fig (labX), y, fig (110.0f), rowH });
                ctl (*a->gain,  gainX, 56.0f);
                ctl (*a->delay, delayX, 56.0f);
                ctl (*a->pol,   polX, 28.0f);
                ctl (*a->mute,  muteX, 28.0f);
                ctl (*a->angle, angX, 56.0f);
                row (false);
            }

            // Mirroring needs a partner, so the header's mirror only appears
            // once the source has two boxes. The tick box it replaces is kept
            // alive and wired - it is simply given no room - so this is one
            // line away from coming back as a row.
            symToggle_->setVisible (false);
            headAng_->setToggle (cabNo > 1, selectedIsSymmetric());

            // Figma puts this header straight under the section above, so the
            // grip is the bottom of that section's last row, clear of its
            // controls. Straddling the header's top edge put half of it on
            // the SectionHead child, which takes its own clicks.
            angGripY_ = { angHeadTop - fig (7.0f), angHeadTop };
            gap (28.0f);
        }
        else
        {
            angGripY_ = { 0, 0 };
            symToggle_->setVisible (false);
        }

        contentH_ = y;
    }

private:
    /** Re-stamp the MOUNTING list for @p model. Ids are the mount plus one:
        Ground 1, Stacked 2, Flown 3. A sub is put on the deck or built up
        from it, never hung, so it has no Flown - offering one invited a rig
        nobody would build. Rebuilt only when the list actually changes,
        because reloading a combo closes its popup. */
    void setMountingForModel (int model)
    {
        if (posMounting_ == nullptr) return;
        const int want = isSubwooferModel (model) ? 2 : 3;
        if (posMounting_->getNumItems() == want && posMounting_->getItemText (1) == "Stacked") return;

        const int keep = posMounting_->getSelectedId();
        posMounting_->clear (juce::dontSendNotification);
        posMounting_->addItem ("Ground", 1);
        posMounting_->addItem ("Stacked", 2);
        if (want == 3) posMounting_->addItem ("Flown", 3);
        if (keep >= 1 && keep <= want)
            posMounting_->setSelectedId (keep, juce::dontSendNotification);
    }

    /** How far the Rigging Angles box may be dragged: down until its header
        rests on the bottom of the visible sidebar, never below it. */
    int angOffsetLimit() const
    {
        if (auto* vp = findParentComponentOfClass<juce::Viewport>())
            return juce::jmax (0, vp->getViewPositionY() + vp->getMaximumVisibleHeight()
                                    - UiConfig::Scale::fig (32.0f) - angNaturalTop_);
        return UiConfig::Scale::fig (600.0f);
    }

    /** Is this event on the Rigging Angles resize grip? */
    bool overAngleGrip (const juce::MouseEvent& e) const
    {
        return ! angGripY_.isEmpty() && angGripY_.contains (e.y);
    }

    /** Re-lay the panel after the box moved, and tell the viewport the
        panel is a different size - the sidebar scrolls the whole panel, so a
        box pushed further down has to be given the room. */
    void relayoutAfterResize()
    {
        resized();
        if (auto* vp = findParentComponentOfClass<juce::Viewport>())
        {
            const int h = juce::jmax (vp->getMaximumVisibleHeight(), contentH_);
            if (h != getHeight()) setSize (getWidth(), h);
        }
        repaint();
    }

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
    };
    struct AngleRow
    {
        int spIndex = -1;              // the cabinet this row edits
        juce::String num, cab;
        std::unique_ptr<FieldBox>   gain, delay, angle;
        std::unique_ptr<SquareMark> pol, mute;
    };
    struct Text
    {
        juce::String text; juce::Rectangle<int> area;
        float size; bool bold; bool dim; juce::Justification just;
    };

    static constexpr float kCrossoverHz[] = { 20, 25, 31.5f, 40, 50, 63, 80, 100, 125, 160,
                                              200, 250, 315, 400, 500, 630, 800, 1000, 1250,
                                              1600, 2000, 2500, 3150, 4000, 5000, 6300, 8000,
                                              10000, 12500, 16000, 20000 };

    static juce::String hzText (float hz)
    {
        return hz >= 1000.0f ? num (hz / 1000.0) + "kHz" : num (hz) + "Hz";
    }

    static int nearestCrossoverItem (float hz)
    {
        int best = 0;
        for (int i = 1; i < (int) std::size (kCrossoverHz); ++i)
            if (std::abs (std::log (kCrossoverHz[i] / hz)) < std::abs (std::log (kCrossoverHz[best] / hz)))
                best = i;
        return best + 1;
    }

    /** The Low Pass and High Pass boxes, folded into the selected source's
        filter. A frequency of "Off" drops that end; with both ends on it is a
        band pass, the high pass at fcHz and the low pass at fcHiHz. */
    void commitFilter()
    {
        if (building_ || onFilter == nullptr
            || selected_ < 0 || selected_ >= (int) scene_.size()) return;
        SpeakerFilter f = scene_[(size_t) selected_].filter;

        auto hz = [] (const juce::ComboBox& c)
        { return kCrossoverHz[juce::jlimit (0, (int) std::size (kCrossoverHz) - 1, c.getSelectedId() - 2)]; };
        auto family = [] (const juce::ComboBox& c)
        { return c.getSelectedId() <= 2 ? SpeakerFilter::Family::Butterworth : SpeakerFilter::Family::LinkwitzRiley; };
        auto order = [] (const juce::ComboBox& c) { return c.getSelectedId() % 2 == 1 ? 2 : 4; };

        const bool lp = fltLpFreq_->getSelectedId() > 1;
        const bool hp = fltHpFreq_->getSelectedId() > 1;
        f.type = lp && hp ? SpeakerFilter::Type::BandPass
               : lp       ? SpeakerFilter::Type::LowPass
               : hp       ? SpeakerFilter::Type::HighPass
                          : SpeakerFilter::Type::Off;

        const juce::ComboBox& mainFreq  = lp && ! hp ? *fltLpFreq_  : *fltHpFreq_;
        const juce::ComboBox& mainSlope = lp && ! hp ? *fltLpSlope_ : *fltHpSlope_;
        f.fcHz   = hz (mainFreq);
        f.family = family (mainSlope);
        f.order  = order (mainSlope);
        if (lp && hp)
        {
            f.fcHiHz   = hz (*fltLpFreq_);
            f.familyHi = family (*fltLpSlope_);
            f.orderHi  = order (*fltLpSlope_);
        }
        fltLpSlope_->setEnabled (lp);
        fltHpSlope_->setEnabled (hp);
        onFilter (selected_, f);
    }

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
        headFlt_ = std::make_unique<SectionHead> ("Filters", false);
        headSrc_->onPlus = [this] { if (onAddSource) onAddSource(); };
        headPos_->onPlus = [this] { if (onAddSource) onAddSource(); };
        for (auto* h : { headSrc_.get(), headPos_.get(), headRig_.get(), headAng_.get(), headFlt_.get() })
        {
            h->setGlyphs (26.0f, 17.0f, 9.3f, 68.0f);
            h->onFold = [this] (bool) { refold(); };
            addAndMakeVisible (*h);
        }
        // The mirror in the Rigging Angles header drives the same callback the
        // tick box under the table used to, so the scene does not know or care
        // which control was touched.
        headAng_->onToggle = [this] (bool on)
        {
            if (building_ || onSymmetric == nullptr || selected_ < 0) return;
            onSymmetric (selected_, on);
        };

        auto combo = [this] (std::unique_ptr<juce::ComboBox>& c,
                             std::initializer_list<const char*> items)
        {
            c = std::make_unique<juce::ComboBox>();
            int id = 1;
            for (auto* it : items) c->addItem (it, id++);
            addAndMakeVisible (*c);
        };
        combo (posSystem_,   { "Q212P", "Q21S", "BEM 2inch" });
        // Item 1 is always Ground; item 2 is whatever OFF the ground means for
        // this cabinet, and its name changes with the model - see
        // setMountingForModel(). Built here with the default pair so the combo
        // is never empty before a source is selected.
        combo (posMounting_, { "Ground", "Flown" });
        combo (posCabinets_, { "1" });
        // Front and rear pick are hole numbers on the bumper; single pick is
        // the lever hole, which IS the tilt. All three come from the supplied
        // bracket - see RiggingPoints.h.
        combo (frontPick_,   { "1", "2", "3", "4", "5" });
        combo (rearPick_,    { "1", "2", "3", "4", "5" });
        {
            // Every hole the bracket actually has, in table order, each shown
            // with the tilt it produces - so the list IS the bracket.
            singlePick_ = std::make_unique<juce::ComboBox>();
            int id = 1;
            for (const auto& p : Rigging::allPicks())
                singlePick_->addItem (juce::String (Rigging::tiltForPick (p), 0)
                                          + " Deg.  (" + Rigging::label (p) + ")", id++);
            addAndMakeVisible (*singlePick_);
        }

        posSystem_->onChange = [this]
        {
            if (! building_ && onModel && selected_ >= 0)
                onModel (selected_, modelFromItem (posSystem_->getSelectedId()));
        };
        posMounting_->onChange = [this]
        {
            // Item ids are the mount plus one: Ground 1, Stacked 2, Flown 3.
            if (building_ || onMount == nullptr || selected_ < 0) return;
            onMount (selected_, posMounting_->getSelectedId() - 1);
        };

        posCabinets_->setSelectedId (1, juce::dontSendNotification);
        posCabinets_->setEnabled (false);
        posCabinets_->setTooltip ("Grouped arrays are not modelled yet - one source is one cabinet.");

        // Single pick is real: choosing a hole sets the source's down-tilt, and
        // typing a tilt moves the list to the hole that gives it.
        singlePick_->setTooltip ("Lever hole. Configuration A leans the box up, B leans it down; "
                                 "each hole is "
                                 + juce::String (Rigging::kDegPerPoint, 0) + " degrees.");
        singlePick_->onChange = [this]
        {
            if (building_ || onTilt == nullptr || selected_ < 0) return;
            const auto picks = Rigging::allPicks();
            const int  i     = singlePick_->getSelectedId() - 1;
            if (i < 0 || i >= picks.size()) return;
            // The bracket's table is positive up (A leans up, B down); the
            // scene's tilt is positive down.
            onTilt (selected_, -Rigging::tiltForPick (picks.getReference (i)));
        };

        // The two bumper holes still need cabinet weights before they can say
        // anything true about load, so they stay unavailable rather than
        // showing numbers that would be believed.
        for (auto* c : { frontPick_.get(), rearPick_.get() })
        {
            c->setEnabled (false);
            c->setTooltip ("Rigging loads need cabinet weights, which the model does not carry yet.");
        }
        frontPick_->setSelectedId (1, juce::dontSendNotification);
        rearPick_->setSelectedId (2, juce::dontSendNotification);
        singlePick_->setSelectedId (6, juce::dontSendNotification);   // A0, level

        loadBar_ = std::make_unique<LoadBar>();
        addAndMakeVisible (*loadBar_);

        // Filters - a Low Pass and a High Pass, each with its own crossover
        // (third-octave centres, the way a processor offers it, or Off) and
        // its own slope, a family and an order.
        for (auto* fq : { &fltLpFreq_, &fltHpFreq_ })
        {
            *fq = std::make_unique<juce::ComboBox>();
            (*fq)->addItem ("Off", 1);
            for (int i = 0; i < (int) std::size (kCrossoverHz); ++i)
                (*fq)->addItem (hzText (kCrossoverHz[i]), i + 2);
            addAndMakeVisible (**fq);
        }
        for (auto* sl : { &fltLpSlope_, &fltHpSlope_ })
        {
            *sl = std::make_unique<SlopeCombo>();
            addAndMakeVisible (**sl);
        }
        for (auto* c : { fltLpFreq_.get(), fltLpSlope_.get(), fltHpFreq_.get(), fltHpSlope_.get() })
            c->onChange = [this] { commitFilter(); };

        for (int i = 0; i < 3; ++i)
        {
            posXYZ_[i] = std::make_unique<FieldBox> ("0", Units::lengthUnit());
            // A tenth, which is also the grid the plan snaps cabinets to.
            posXYZ_[i]->setWheelStep (lengthWheelStep());
            posXYZ_[i]->onCommit = [this, i] (double v) { commitPosition (i, v); };
            addAndMakeVisible (*posXYZ_[i]);
        }
        posInterdist_ = std::make_unique<FieldBox> ("0", Units::lengthUnit());
        posInterdist_->setWheelStep (lengthWheelStep());
        posInterdist_->setTooltip ("Interdistance: centre-to-centre distance between neighbouring "
                                   "cabinets of a ground row. Stacked and flown cabinets touch, so "
                                   "it applies to Ground only.");
        posInterdist_->onCommit = [this] (double v)
        {
            if (building_ || onInterdistance == nullptr || selected_ < 0) return;
            // Typed in whatever unit the app is showing; the scene is metres.
            const double m = Units::displayToMetres (v);
            if (m > 0.0) onInterdistance (selected_, (float) m);
        };
        addAndMakeVisible (*posInterdist_);

        posHorAim_ = std::make_unique<FieldBox> ("0", "Deg.");
        posVerAim_ = std::make_unique<FieldBox> ("0", "Deg.");
        posHorAim_->setInlineUnit (true, true);
        posVerAim_->setInlineUnit (true, true);
        posHorAim_->setWheelStep (1.0);
        posVerAim_->setWheelStep (1.0);
        posHorAim_->setTooltip ("Horizontal aim of the whole source: 0 forward, +90 right, "
                                "-90 left, 180 rear.");
        posVerAim_->setTooltip ("Vertical aim of the whole source: 0 level, +90 up, -90 down.");
        // Hor. Aim turns the whole source to the typed heading; each box
        // keeps its offset from it. Panel convention is positive right, the
        // scene's is counter-clockwise, hence the sign.
        posHorAim_->onCommit = [this] (double v)
        {
            if (building_ || onArrayRotation == nullptr || selected_ < 0
                || selected_ >= (int) scene_.size()) return;
            const float heading = sourceHeadingDeg (scene_, selected_);
            const float delta   = wrap180 ((float) -v - heading);
            onArrayRotation (selected_, scene_[(size_t) selected_].rotationDeg + delta);
        };
        posVerAim_->onCommit = [this] (double v)
        {
            if (building_ || onTilt == nullptr || selected_ < 0) return;
            onTilt (aimBox(), (float) -v);
        };
        addAndMakeVisible (*posHorAim_);
        addAndMakeVisible (*posVerAim_);

        symToggle_ = std::make_unique<TickBox>();
        symToggle_->setTooltip ("Array Symmetry: a change to one cabinet is applied to its "
                                "partner from the other end (1 and last, 2 and second-last...).");
        symToggle_->onClick = [this]
        {
            if (building_ || onSymmetric == nullptr || selected_ < 0) return;
            onSymmetric (selected_, symToggle_->getToggleState());
        };
        addAndMakeVisible (*symToggle_);
    }

    /** The tilt angles the rigging lever can actually be pinned at, from the
        supplied bracket table - see RiggingPoints.h. -15 to +15 in threes:
        B5..B1 then A0..A5, ascending so a list reads bottom-up. */
    static const std::vector<double>& riggingTilts()
    {
        static const std::vector<double> t = []
        {
            std::vector<double> v;
            for (const auto& p : Rigging::allPicks())
                v.push_back ((double) Rigging::tiltForPick (p));
            std::sort (v.begin(), v.end());
            return v;
        }();
        return t;
    }

    /** Is the selected source mirroring its cabinets? Read from the scene,
        which is where the flag lives - the header only shows it. */
    bool selectedIsSymmetric() const
    {
        return selected_ >= 0 && selected_ < (int) scene_.size()
            && scene_[(size_t) selected_].symmetric;
    }

    bool selectedIsSub() const
    {
        return selected_ >= 0 && selected_ < (int) scene_.size()
            && isSubwooferModel (scene_[(size_t) selected_].model);
    }

    /** -180 < d <= 180. */
    static float wrap180 (float d) noexcept
    {
        d = std::fmod (d, 360.0f);
        if (d > 180.0f)   d -= 360.0f;
        if (d <= -180.0f) d += 360.0f;
        return d;
    }

    /** The box that carries the selected source's Ver. Aim: the top box of
        a flown hang, the bottom box of a stack. */
    int aimBox() const
    {
        const auto members = arrayMembers (scene_, selected_);
        if (members.empty()) return selected_;
        return sourceMount (scene_, selected_) == Mount::Stacked ? members.back()
                                                                 : members.front();
    }

    /** The cabinet-count box, filled from the selected array and able to
        change it. It used to be greyed at 1 because a source was one box. */
    void wireCabinetCount()
    {
        if (posCabinets_ == nullptr) return;
        if (posCabinets_->getNumItems() != kMaxCabinets)
        {
            posCabinets_->clear (juce::dontSendNotification);
            for (int n = 1; n <= kMaxCabinets; ++n) posCabinets_->addItem (juce::String (n), n);
            posCabinets_->setTooltip ({});
        }

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
                juce::jlimit (1, kMaxCabinets, (int) arrayMembers (scene_, selected_).size()),
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
            if (sources_[i]->model != sp[i].model
                || ! sources_[i]->name.endsWith ("-" + juce::String (sp[i].arrayId >= 0 ? sp[i].arrayId
                                                                                        : (int) i + 1)))
                return false;
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
            // Named by the source's own id, which never changes - numbering by
            // list position renamed every source after a deletion.
            row->name  = modelName (sp[i].model) + "-"
                       + juce::String (sp[i].arrayId >= 0 ? sp[i].arrayId : idx + 1);
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
                if (building_ || onSourceEnabled == nullptr) return;
                onSourceEnabled (idx, ! sources_[(size_t) idx]->mute->getToggleState());
            };
            addAndMakeVisible (*row->system);
            addAndMakeVisible (*row->mute);
            row->system->addMouseListener (this, false);
            row->mute->addMouseListener (this, false);
            sources_.push_back (std::move (row));

            auto a = std::make_unique<AngleRow>();
            a->num   = juce::String (idx + 1);
            a->cab   = modelName (sp[i].model);
            a->gain  = std::make_unique<FieldBox> ("0", "dB");
            a->delay = std::make_unique<FieldBox> ("0", "ms");
            a->angle = std::make_unique<FieldBox> ("0", "Deg");
            // Figma set these in from the left by 13, 10 and 7 against 9 px
            // type. At 12 px the value needs that room back, so each inset
            // comes down by about half - the text still clears the cell's
            // left edge and "0.56 ms" still fits before the spinner.
            a->gain->setStyle  (PanelType::dense, 7.0f);
            a->delay->setStyle (PanelType::dense, 5.0f);
            a->angle->setStyle (PanelType::dense, isSubwooferModel (sp[i].model) ? 6.0f : 4.0f);
            // One notch each: a dB, a tenth of a millisecond, a degree.
            a->gain->setWheelStep  (1.0);
            a->delay->setWheelStep (0.1);
            a->angle->setWheelStep (1.0);

            // A TOP's Angles cell is the splay to the box above it, and that
            // angle is a HOLE in the lever - only the eleven the bracket has
            // can be pinned. Offered as a closed list, the way ArrayCalc's
            // Splay column lists the angles its link mechanically allows.
            //
            // A SUB's last column is its own Hor. Rot. instead, which is a
            // heading and not a lever hole, so it stays a free number.
            if (! isSubwooferModel (sp[i].model))
                a->angle->setAllowedValues (riggingTilts());
            else
                a->angle->setAllowedValues ({});
            for (auto* f : { a->gain.get(), a->delay.get(), a->angle.get() })
                f->setFill (juce::Colour (0xffe8e8e8));
            a->gain->setInlineUnit  (true, false);
            a->delay->setInlineUnit (true, false);
            a->angle->setInlineUnit (true, false);
            a->angle->setSpinner (true);
            a->gain->onCommit = [this, idx] (double v)
            { if (! building_ && onGain)  onGain  (idx, (float) v); };
            a->delay->onCommit = [this, idx] (double v)
            { if (! building_ && onDelay) onDelay (idx, (float) v); };
            a->angle->onCommit = [this, idx] (double v)
            {
                // A sub's last column is its own horizontal rotation. A top's
                // is its vertical angle from its neighbour in the hang (the
                // box above when flown, below when stacked); the anchor box
                // has none, its angle being the array's Ver. Aim. Both are
                // shown in the panel's convention - positive right, positive
                // up - and handed on in the scene's.
                if (building_ || (size_t) idx >= scene_.size()) return;
                if (isSubwooferModel (scene_[(size_t) idx].model))
                {
                    if (onRotation) onRotation (idx, (float) -v);
                    return;
                }
                if (onSplay) onSplay (idx, (float) -v);
            };
            a->pol  = std::make_unique<SquareMark> (juce::String::charToString ((juce_wchar) 0x00d8));
            a->mute = std::make_unique<SquareMark> ("M");
            a->pol->setOutlined (true);
            a->mute->setOutlined (true);
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
            for (juce::Component* c : { (juce::Component*) a->gain.get(),
                                        (juce::Component*) a->delay.get(),
                                        (juce::Component*) a->pol.get(),
                                        (juce::Component*) a->mute.get(),
                                        (juce::Component*) a->angle.get() })
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
        prevTilt_.assign (sp.size(), 0.0f);

        for (size_t i = 0; i < sources_.size() && i < sp.size(); ++i)
        {
            sources_[i]->system->setSelectedId (itemFromModel (sp[i].model), juce::dontSendNotification);
            sources_[i]->mute->setToggleState (! sp[i].enabled, juce::dontSendNotification);

            angles_[i]->gain->setValue (num (sp[i].gainDB));
            // Delays are set to the hundredth of a millisecond.
            const double ms = std::round ((double) sp[i].delayMs * 100.0) / 100.0;
            angles_[i]->delay->setValue (std::abs (ms - std::round (ms)) < 0.005
                                             ? juce::String ((int) std::lround (ms))
                                             : juce::String (ms, 2));
            angles_[i]->pol->setToggleState (sp[i].polarityInverted, juce::dontSendNotification);
            angles_[i]->mute->setToggleState (! sp[i].enabled, juce::dontSendNotification);
            // The angle is from this box's neighbour IN THE SAME ARRAY: the
            // box above when flown, the box below when stacked (a stack is
            // built on its bottom box). Taken across the scene it measured
            // the gap between two unrelated sources.
            const auto members = arrayMembers (sp, (int) i);
            const bool stacked = sourceMount (sp, (int) i) == Mount::Stacked;
            int ref = -1;
            for (size_t k = 0; k < members.size(); ++k)
                if (members[k] == (int) i)
                {
                    if (stacked && k + 1 < members.size()) ref = members[k + 1];
                    if (! stacked && k > 0)                ref = members[k - 1];
                }

            prevTilt_[i] = ref < 0 ? 0.0f : sp[(size_t) ref].tiltDeg;
            const bool sub = isSubwooferModel (sp[i].model);
            // Shown positive right / positive up; the scene is
            // counter-clockwise and positive down.
            angles_[i]->angle->setValue (num (sub ? wrap180 (-sp[i].rotationDeg)
                                            : ref < 0 ? 0.0f : -(sp[i].tiltDeg - prevTilt_[i])));
            // The anchor box has no neighbour to be angled from.
            angles_[i]->angle->setEnabled (sub || ref >= 0);
        }

        const bool has = selected_ >= 0 && (size_t) selected_ < sp.size();

        // The lever hole that matches this source's tilt. The lever is the
        // flown frame's, so only a flown source has one to set.
        const bool flown = has && sourceMount (sp, selected_) == Mount::Flown;
        singlePick_->setEnabled (flown);
        if (has)
        {
            const float tilt  = -sp[(size_t) selected_].tiltDeg;   // table: positive up
            const auto  picks = Rigging::allPicks();
            const auto  want  = Rigging::pickForTilt (tilt);
            for (int i = 0; i < picks.size(); ++i)
                if (picks.getReference (i) == want)
                { singlePick_->setSelectedId (i + 1, juce::dontSendNotification); break; }

            // An angle typed in between two holes cannot be pinned; say so
            // rather than rounding it behind the user's back.
            singlePick_->setTooltip (Rigging::isOnAHole (tilt)
                ? "Lever hole " + Rigging::label (want)
                : "No hole at " + juce::String (tilt, 1)
                      + " deg - nearest is " + Rigging::label (want)
                      + " at " + juce::String (Rigging::tiltForPick (want), 0) + " deg.");
        }
        if (has)
        {
            const auto& sel = sp[(size_t) selected_];
            // The SOURCE's position: its plan centre and reference height,
            // whatever box of it the selection landed on.
            const auto ref = sourceRef (sp, selected_);
            lastX_ = ref.cx; lastY_ = ref.cy; lastHeight_ = ref.z;
            posSystem_->setSelectedId (itemFromModel (sel.model), juce::dontSendNotification);
            setMountingForModel (sel.model);
            posMounting_->setSelectedId (sourceMount (sp, selected_) + 1, juce::dontSendNotification);
            posXYZ_[0]->setValue (num (Units::metresToDisplay (ref.cx)));
            posXYZ_[1]->setValue (num (Units::metresToDisplay (ref.cy)));
            posXYZ_[2]->setValue (num (Units::metresToDisplay (ref.z)));
            posHorAim_->setValue (num (wrap180 (-sourceHeadingDeg (sp, selected_))));
            posVerAim_->setValue (num (-sp[(size_t) aimBox()].tiltDeg));
            posInterdist_->setValue (num (Units::metresToDisplay (rowSpacingM (sp, selected_))));
            symToggle_->setToggleState (sel.symmetric, juce::dontSendNotification);
            {
                const auto& f = sel.filter;
                using T = SpeakerFilter::Type;
                auto slopeId = [] (SpeakerFilter::Family fam, int ord)
                { return (fam == SpeakerFilter::Family::Butterworth ? 1 : 3) + (ord == 2 ? 0 : 1); };
                const bool lp = f.type == T::LowPass  || f.type == T::BandPass;
                const bool hp = f.type == T::HighPass || f.type == T::BandPass;
                const bool lpHi = f.type == T::BandPass;
                fltLpFreq_->setSelectedId (lp ? nearestCrossoverItem (lpHi ? f.fcHiHz : f.fcHz) + 1 : 1,
                                           juce::dontSendNotification);
                fltLpSlope_->setSelectedId (lpHi ? slopeId (f.familyHi, f.orderHi) : slopeId (f.family, f.order),
                                            juce::dontSendNotification);
                fltHpFreq_->setSelectedId (hp ? nearestCrossoverItem (f.fcHz) + 1 : 1,
                                           juce::dontSendNotification);
                fltHpSlope_->setSelectedId (slopeId (f.family, f.order), juce::dontSendNotification);
            }
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
                                    (juce::Component*) posInterdist_.get(),
                                    (juce::Component*) posHorAim_.get(),
                                    (juce::Component*) posVerAim_.get(),
                                    (juce::Component*) fltLpFreq_.get(),
                                    (juce::Component*) fltLpSlope_.get(),
                                    (juce::Component*) fltHpFreq_.get(),
                                    (juce::Component*) fltHpSlope_.get() })
            if (c != nullptr) c->setEnabled (has);

        // A slope with its frequency Off shapes nothing.
        if (has)
        {
            fltLpSlope_->setEnabled (fltLpFreq_->getSelectedId() > 1);
            fltHpSlope_->setEnabled (fltHpFreq_->getSelectedId() > 1);
        }

        // Only a ground row has a gap to set: stacked and flown boxes touch,
        // hinged to each other, and one box has no neighbour.
        if (posInterdist_ != nullptr && has)
            posInterdist_->setEnabled (sourceMount (sp, selected_) == Mount::Ground
                                       && arrayMembers (sp, selected_).size() > 1);
        if (symToggle_ != nullptr) symToggle_->setEnabled (has);

        building_ = false;
    }

    PanelLnF lnf_;
    // The scene as last handed in. Needed because which rows belong to the
    // selected SOURCE is a question about arrays, not about row order.
    std::vector<Speaker> scene_;

    std::vector<std::unique_ptr<SourceRow>> sources_;
    std::vector<std::unique_ptr<AngleRow>>  angles_;
    std::unique_ptr<SectionHead> headSrc_, headPos_, headRig_, headAng_, headFlt_;
    std::unique_ptr<juce::ComboBox> fltLpFreq_, fltLpSlope_, fltHpFreq_, fltHpSlope_;
    bool mapping_ = false;
    std::vector<std::unique_ptr<PlaneRow>> planes_;
    std::vector<juce::Range<int>>          planeRowY_;
    int                                    selPlane_ = -1;
    std::unique_ptr<juce::ComboBox> posSystem_, posMounting_, posCabinets_,
                                    frontPick_, rearPick_, singlePick_;
    std::unique_ptr<FieldBox> posXYZ_[3], posHorAim_, posVerAim_, posInterdist_;
    std::unique_ptr<TickBox>  symToggle_;
    std::unique_ptr<LoadBar>  loadBar_;

    std::vector<Text>  texts_;
    std::vector<int>   rowRules_, listRules_;
    std::vector<juce::Range<int>> bands_;
    juce::Rectangle<int> emptyArea_;
    std::vector<juce::Range<int>> sourceRowY_;
    std::vector<float> prevTilt_;
    std::vector<juce::Rectangle<int>> vRules_;
    int   contentH_  = 400;
    int   selected_  = -1;
    bool  building_  = false;

    // --- Rigging Angles, movable ------------------------------------------
    // angOffsetY_ is how far down the panel the whole box has been dragged,
    // in pixels. 0 is where it naturally falls, which is where it opens.
    int   angOffsetY_     = 0;
    int   angNaturalTop_  = 0;
    bool  angDragging_    = false;
    int   angDragY0_      = 0;
    int   angDragOffset0_ = 0;
    juce::Range<int> angGripY_;
    float lastX_ = 0, lastY_ = 0, lastHeight_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SourcePanel)
};
