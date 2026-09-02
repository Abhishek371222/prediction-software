#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"
#include <deque>

// ---------------------------------------------------------------------------
// CommandTerminal — VS Code–inspired command panel (xterm-like look).
// Stub shell for now: only `cls` / `clear` work; all other input → invalid cmd.
// More commands will be wired later.
// ---------------------------------------------------------------------------
class CommandTerminal : public juce::Component,
                        private juce::TextEditor::Listener
{
public:
    CommandTerminal()
    {
        setOpaque (true);

        output_.setMultiLine (true, true);
        output_.setReadOnly (true);
        output_.setScrollbarsShown (true);
        output_.setCaretVisible (false);
        output_.setPopupMenuEnabled (false);
        output_.setFont (monoFont (13.0f));
        output_.setColour (juce::TextEditor::backgroundColourId, bg());
        output_.setColour (juce::TextEditor::textColourId, fg());
        output_.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        output_.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
        output_.setColour (juce::TextEditor::highlightColourId, juce::Colour (0xff264f78));
        output_.setColour (juce::CaretComponent::caretColourId, cursor());
        output_.setLineSpacing (1.15f);
        addAndMakeVisible (output_);

        prompt_.setText (kPrompt, juce::dontSendNotification);
        prompt_.setJustificationType (juce::Justification::centredLeft);
        prompt_.setFont (monoFont (13.0f));
        prompt_.setColour (juce::Label::textColourId, accent());
        prompt_.setColour (juce::Label::backgroundColourId, bg());
        prompt_.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (prompt_);

        input_.setMultiLine (false);
        input_.setReturnKeyStartsNewLine (false);
        input_.setFont (monoFont (13.0f));
        input_.setColour (juce::TextEditor::backgroundColourId, bg());
        input_.setColour (juce::TextEditor::textColourId, fg());
        input_.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        input_.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
        input_.setColour (juce::TextEditor::highlightColourId, juce::Colour (0xff264f78));
        input_.setColour (juce::CaretComponent::caretColourId, cursor());
        input_.setTextToShowWhenEmpty ("", fg().withAlpha (0.35f));
        input_.addListener (this);
        addAndMakeVisible (input_);

        appendLine ("Atomik command terminal");
        appendLine ("");
    }

    ~CommandTerminal() override
    {
        input_.removeListener (this);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (bg());
        // Thin top rule — similar to VS Code panel separator.
        g.setColour (juce::Colour (0xff3c3c3c));
        g.fillRect (0, 0, getWidth(), 1);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (6, 4);
        const int lineH = 22;
        auto inputRow = r.removeFromBottom (lineH);
        const int promptW = 18;
        prompt_.setBounds (inputRow.removeFromLeft (promptW));
        input_.setBounds (inputRow);
        output_.setBounds (r);
    }

    void lookAndFeelChanged() override
    {
        output_.setFont (monoFont (13.0f));
        input_.setFont (monoFont (13.0f));
        prompt_.setFont (monoFont (13.0f));
        output_.setColour (juce::TextEditor::backgroundColourId, bg());
        output_.setColour (juce::TextEditor::textColourId, fg());
        input_.setColour (juce::TextEditor::backgroundColourId, bg());
        input_.setColour (juce::TextEditor::textColourId, fg());
        prompt_.setColour (juce::Label::textColourId, accent());
        prompt_.setColour (juce::Label::backgroundColourId, bg());
        repaint();
    }

    void focusInput()
    {
        input_.grabKeyboardFocus();
    }

private:
    static constexpr const char* kPrompt = ">";

    static juce::Colour bg()     { return juce::Colour (0xff1e1e1e); } // VS Code terminal bg
    static juce::Colour fg()     { return juce::Colour (0xffcccccc); }
    static juce::Colour accent() { return juce::Colour (0xff4ec9b0); } // prompt teal
    static juce::Colour error()  { return juce::Colour (0xfff48771); }
    static juce::Colour cursor() { return juce::Colour (0xffaeafad); }

    static juce::Font monoFont (float h)
    {
        return Brand::mono (Brand::UI::scaledFont (h));
    }

    void textEditorReturnKeyPressed (juce::TextEditor&) override
    {
        runCommand (input_.getText());
        input_.clear();
    }

    void textEditorEscapeKeyPressed (juce::TextEditor&) override
    {
        input_.clear();
    }

    void textEditorTextChanged (juce::TextEditor&) override {}
    void textEditorFocusLost (juce::TextEditor&) override {}

    void runCommand (juce::String raw)
    {
        const juce::String line = raw.trim();
        appendLine (juce::String (kPrompt) + " " + line);

        if (line.isEmpty())
            return;

        const juce::String cmd = line.toLowerCase();
        if (cmd == "cls" || cmd == "clear")
        {
            clearScreen();
            return;
        }

        appendError ("'" + line + "' is not recognized as a valid command.");
    }

    void clearScreen()
    {
        lines_.clear();
        refreshOutput();
    }

    void appendLine (const juce::String& s)
    {
        lines_.push_back (s);
        trimHistory();
        refreshOutput();
    }

    void appendError (const juce::String& s)
    {
        // Prefix so errors are easy to spot; colour is same buffer (read-only editor).
        lines_.push_back ("[!] " + s);
        trimHistory();
        refreshOutput();
    }

    void trimHistory()
    {
        while (lines_.size() > 400)
            lines_.pop_front();
    }

    void refreshOutput()
    {
        juce::String all;
        for (const auto& l : lines_)
        {
            all += l;
            all += "\n";
        }
        output_.setText (all, juce::dontSendNotification);
        output_.moveCaretToEnd();
        output_.scrollEditorToPositionCaret (0, output_.getHeight());
    }

    juce::TextEditor output_;
    juce::Label      prompt_;
    juce::TextEditor input_;
    std::deque<juce::String> lines_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CommandTerminal)
};
