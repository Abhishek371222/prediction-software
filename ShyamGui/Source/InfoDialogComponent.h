#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"

// ---------------------------------------------------------------------------
// InfoDialogComponent - a plain read-only panel: heading, optional sub-heading,
// a block of body text, and a red close cross in the top-right corner.
//
// Replaces juce::AlertWindow::showMessageBoxAsync for the Help cluster's info
// and "?" popups. The stock message box hard-codes two things the design does
// not want: a large tinted icon disc beside the text, and an OK button under
// it. Neither is configurable, so the panel is drawn here instead -- styled
// like PreferencesComponent so every overlay in the app matches.
// ---------------------------------------------------------------------------
class InfoDialogComponent : public juce::Component
{
public:
    InfoDialogComponent()
    {
        title_.setJustificationType (juce::Justification::centredLeft);
        title_.setBorderSize ({});
        addAndMakeVisible (title_);

        subtitle_.setJustificationType (juce::Justification::centredLeft);
        subtitle_.setBorderSize ({});
        addAndMakeVisible (subtitle_);

        body_.setJustificationType (juce::Justification::topLeft);
        body_.setBorderSize ({});
        addAndMakeVisible (body_);

        // Drawn rather than a text glyph so the arms stay true at any scale.
        closeBtn_.setComponentID ("infoDlgClose");
        closeBtn_.onClick = [this] { if (onClose) onClose(); };
        addAndMakeVisible (closeBtn_);

        applyColours();
    }

    /** @param subtitle optional line under the heading; pass {} to omit it. */
    void setContent (const juce::String& heading,
                     const juce::String& subtitle,
                     const juce::String& body)
    {
        title_.setText (heading, juce::dontSendNotification);
        subtitle_.setText (subtitle, juce::dontSendNotification);
        subtitle_.setVisible (subtitle.isNotEmpty());
        body_.setText (body, juce::dontSendNotification);
        resized();
    }

    /** Height that exactly fits the current body text at the given width.

        The body is measured with a GlyphArrangement rather than estimated from
        a line count: a guessed line height left the panel with a wide band of
        dead space under the last line. */
    int preferredHeight (int forWidth) const
    {
        const int pad = UiConfig::Scale::px (UiConfig::Layout::prefsPadding);

        const auto font = bodyFont();

        // Two measures, because neither alone is safe. The glyph box is exact
        // for wrapped text but drops trailing blank lines, which clipped the
        // last shortcut off the list; the line count catches those but assumes
        // no wrapping. Take whichever is larger, plus half a line of slack.
        juce::GlyphArrangement ga;
        ga.addJustifiedText (font, body_.getText(),
                             0.0f, 0.0f, (float) juce::jmax (10, forWidth - 2 * pad),
                             juce::Justification::topLeft);
        const int glyphH = juce::roundToInt (ga.getBoundingBox (0, -1, true).getBottom());

        const int lineH  = juce::jmax (1, juce::roundToInt (font.getHeight()));
        const int countH = juce::jmax (1, juce::StringArray::fromLines (body_.getText()).size()) * lineH;

        const int bodyH = juce::jmax (10, juce::jmax (glyphH, countH) + lineH / 2);

        return pad
             + UiConfig::Scale::px (UiConfig::Layout::prefsTitleRowH)
             + UiConfig::Scale::px (6)
             + (subtitle_.isVisible() ? UiConfig::Scale::px (22) : 0)
             + UiConfig::Scale::px (10)
             + bodyH
             + pad;
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (Brand::panel());
        g.setColour (Brand::border());
        g.drawRect (getLocalBounds(), 1);

        const float pad = (float) UiConfig::Scale::px (UiConfig::Layout::prefsPadding);
        const float dividerY = (float) (title_.getBottom() + UiConfig::Scale::px (6));
        g.setColour (Brand::border().withAlpha (0.45f));
        g.drawLine (pad, dividerY, (float) getWidth() - pad, dividerY, 1.0f);
    }

    void resized() override
    {
        const int pad = UiConfig::Scale::px (UiConfig::Layout::prefsPadding);
        const int W   = getWidth() - 2 * pad;
        const int cross = UiConfig::Scale::px (22);

        title_.setFont (Brand::tech (Brand::UI::scaledFont (Brand::Type::prefsTitle), true));
        subtitle_.setFont (Brand::tech (Brand::UI::scaledFont (Brand::Type::prefsLabel)));
        body_.setFont (bodyFont());

        // Cross sits in the top-right corner, clear of the heading.
        closeBtn_.setBounds (getWidth() - pad - cross, pad, cross, cross);

        int y = pad;
        const int titleH = UiConfig::Scale::px (UiConfig::Layout::prefsTitleRowH);
        title_.setBounds (pad, y, juce::jmax (10, W - cross - pad), titleH);
        y += titleH + UiConfig::Scale::px (6);

        if (subtitle_.isVisible())
        {
            const int subH = UiConfig::Scale::px (22);
            subtitle_.setBounds (pad, y, W, subH);
            y += subH;
        }
        y += UiConfig::Scale::px (10);
        body_.setBounds (pad, y, W, juce::jmax (10, getHeight() - y - pad));
    }

    void lookAndFeelChanged() override { applyColours(); }

    std::function<void()> onClose;

private:
    static juce::Font bodyFont()
    {
        return Brand::mono (Brand::UI::scaledFont (Brand::Type::prefsNote));
    }

    void applyColours()
    {
        title_.setColour    (juce::Label::textColourId, Brand::heading());
        subtitle_.setColour (juce::Label::textColourId, Brand::ash());
        body_.setColour     (juce::Label::textColourId, Brand::text());
        repaint();
    }

    // A red X, drawn as two strokes so it scales cleanly and reads as a close
    // affordance rather than a letter.
    class CloseCross : public juce::Button
    {
    public:
        CloseCross() : juce::Button ("close") { setTooltip ("Close"); }

        void paintButton (juce::Graphics& g, bool over, bool down) override
        {
            auto r = getLocalBounds().toFloat().reduced ((float) getWidth() * 0.26f);
            const auto col = down ? Brand::accent().darker (0.25f)
                           : over ? Brand::accent()
                                  : Brand::accent().withAlpha (0.85f);
            if (over || down)
            {
                g.setColour (Brand::accent().withAlpha (down ? 0.22f : 0.12f));
                g.fillEllipse (getLocalBounds().toFloat());
            }
            const float t = juce::jmax (1.5f, (float) getWidth() * 0.11f);
            g.setColour (col);
            g.drawLine (r.getX(), r.getY(), r.getRight(), r.getBottom(), t);
            g.drawLine (r.getX(), r.getBottom(), r.getRight(), r.getY(), t);
        }
    };

    juce::Label title_, subtitle_, body_;
    CloseCross  closeBtn_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InfoDialogComponent)
};
