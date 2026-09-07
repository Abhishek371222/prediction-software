#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"
#include "CommandRegistry.h"
#include <deque>
#include <functional>
#include <vector>

// ---------------------------------------------------------------------------
// CommandTerminal — AutoCAD-style command line + VS Code look.
// Alias → canonical via CommandRegistry; coloured scrollback; Esc cancels.
// ---------------------------------------------------------------------------
class CommandTerminal : public juce::Component,
                        private juce::TextEditor::Listener,
                        private juce::KeyListener
{
public:
    enum class LineKind { Normal, Prompt, Result, Error };

    struct CommandResult
    {
        enum class Kind { Ok, Fail, ContinueSession, EndSession };
        Kind kind = Kind::Ok;
        juce::String message;
        juce::String nextPrompt;

        static CommandResult ok (const juce::String& msg = {})
        {
            CommandResult r; r.kind = Kind::Ok; r.message = msg; return r;
        }
        static CommandResult fail (const juce::String& msg)
        {
            CommandResult r; r.kind = Kind::Fail; r.message = msg; return r;
        }
        static CommandResult continueSession (const juce::String& prompt,
                                             const juce::String& msg = {})
        {
            CommandResult r;
            r.kind = Kind::ContinueSession;
            r.nextPrompt = prompt;
            r.message = msg;
            return r;
        }
        static CommandResult endSession (const juce::String& msg = {})
        {
            CommandResult r; r.kind = Kind::EndSession; r.message = msg; return r;
        }
    };

    /** Host receives canonical lowercase verb (e.g. "line", never "l"). */
    std::function<CommandResult (const juce::String& verb,
                                 const juce::String& args)> onExecuteCommand;
    std::function<CommandResult (const juce::String& line)> onSessionInput;
    std::function<void()> onSessionCancel;

    CommandTerminal()
    {
        setOpaque (true);

        viewport_.setViewedComponent (&outputContent_, false);
        viewport_.setScrollBarsShown (true, false);
        addAndMakeVisible (viewport_);

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
        input_.addKeyListener (this);
        addAndMakeVisible (input_);

        appendLine ("Atomik command terminal", LineKind::Normal);
        appendLine ("Type HELP or ? for commands. Shortcuts: L LINE, PE PENCIL, ER ERASER, SPK speaker…", LineKind::Normal);
        appendLine ("", LineKind::Normal);
    }

    ~CommandTerminal() override
    {
        input_.removeKeyListener (this);
        input_.removeListener (this);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (bg());
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
        viewport_.setBounds (r);
        refreshOutput();
    }

    void lookAndFeelChanged() override
    {
        input_.setFont (monoFont (13.0f));
        prompt_.setFont (monoFont (13.0f));
        input_.setColour (juce::TextEditor::backgroundColourId, bg());
        input_.setColour (juce::TextEditor::textColourId, fg());
        prompt_.setColour (juce::Label::textColourId, accent());
        prompt_.setColour (juce::Label::backgroundColourId, bg());
        refreshOutput();
        repaint();
    }

    void focusInput() { input_.grabKeyboardFocus(); }
    bool isSessionActive() const noexcept { return sessionActive_; }

    void endSessionLocal()
    {
        sessionActive_ = false;
        sessionHint_.clear();
    }

    void appendResult (const juce::String& s)
    {
        if (s.isNotEmpty())
            appendMultiline (s, LineKind::Result);
    }

    void appendError (const juce::String& s)
    {
        if (s.isNotEmpty())
            appendMultiline (s, LineKind::Error);
    }

    static juce::String builtinHelpText()
    {
        return CommandRegistry::helpText();
    }

private:
    static constexpr const char* kPrompt = ">";

    static juce::Colour bg()     { return juce::Colour (0xff1e1e1e); }
    static juce::Colour fg()     { return juce::Colour (0xffcccccc); }
    static juce::Colour accent() { return juce::Colour (0xff4ec9b0); }
    static juce::Colour error()  { return juce::Colour (0xfff48771); }
    static juce::Colour cursor() { return juce::Colour (0xffaeafad); }

    static juce::Font monoFont (float h)
    {
        return Brand::mono (Brand::UI::scaledFont (h));
    }

    struct Line
    {
        juce::String text;
        LineKind kind = LineKind::Normal;
    };

    class OutputContent : public juce::Component
    {
    public:
        void setLines (const std::deque<Line>* lines, std::function<juce::Colour(LineKind)> colourFn)
        {
            lines_ = lines;
            colourFn_ = std::move (colourFn);
            repaint();
        }

        void paint (juce::Graphics& g) override
        {
            g.fillAll (bg());
            if (lines_ == nullptr) return;

            const auto font = monoFont (13.0f);
            const float lineH = font.getHeight() * 1.25f;
            float y = 4.0f;
            const float x = 4.0f;
            const float w = (float) juce::jmax (1, getWidth() - 8);

            for (const auto& l : *lines_)
            {
                g.setColour (colourFn_ ? colourFn_ (l.kind) : fg());
                g.setFont (font);
                g.drawText (l.text, juce::Rectangle<float> (x, y, w, lineH),
                            juce::Justification::centredLeft, false);
                y += lineH;
            }
        }

        int preferredHeight() const
        {
            if (lines_ == nullptr) return 20;
            const float lineH = monoFont (13.0f).getHeight() * 1.25f;
            return 8 + (int) std::ceil (lineH * (float) lines_->size());
        }

    private:
        const std::deque<Line>* lines_ = nullptr;
        std::function<juce::Colour(LineKind)> colourFn_;
    };

    void textEditorReturnKeyPressed (juce::TextEditor&) override
    {
        runCommand (input_.getText());
        input_.clear();
        historyIndex_ = -1;
    }

    void textEditorEscapeKeyPressed (juce::TextEditor&) override
    {
        input_.clear();
        historyIndex_ = -1;
        if (sessionActive_)
        {
            sessionActive_ = false;
            sessionHint_.clear();
            appendLine ("Command canceled.", LineKind::Result);
            if (onSessionCancel)
                onSessionCancel();
        }
        // Keep caret in the terminal (host must not steal focus to the plot).
        focusInput();
    }

    void textEditorTextChanged (juce::TextEditor&) override {}
    void textEditorFocusLost (juce::TextEditor&) override {}

    bool keyPressed (const juce::KeyPress& key, juce::Component*) override
    {
        if (key.isKeyCode (juce::KeyPress::upKey))
        {
            historyUp();
            return true;
        }
        if (key.isKeyCode (juce::KeyPress::downKey))
        {
            historyDown();
            return true;
        }
        return false;
    }

    void historyUp()
    {
        if (history_.empty()) return;
        if (historyIndex_ < 0)
            historyIndex_ = (int) history_.size() - 1;
        else if (historyIndex_ > 0)
            --historyIndex_;
        input_.setText (history_[(size_t) historyIndex_], false);
        input_.moveCaretToEnd();
    }

    void historyDown()
    {
        if (history_.empty() || historyIndex_ < 0) return;
        if (historyIndex_ + 1 >= (int) history_.size())
        {
            historyIndex_ = -1;
            input_.clear();
            return;
        }
        ++historyIndex_;
        input_.setText (history_[(size_t) historyIndex_], false);
        input_.moveCaretToEnd();
    }

    void pushHistory (const juce::String& line)
    {
        if (line.isEmpty()) return;
        if (! history_.empty() && history_.back() == line) return;
        history_.push_back (line);
        while (history_.size() > 80)
            history_.erase (history_.begin());
    }

    void runCommand (juce::String raw)
    {
        const juce::String line = raw.trim();

        if (line.isEmpty())
        {
            if (sessionActive_ && onSessionInput)
            {
                applyResult (onSessionInput (juce::String()));
                return;
            }
            if (lastCanonical_.isNotEmpty()
                && CommandRegistry::enterRepeats (lastCanonical_)
                && onExecuteCommand != nullptr
                && ! sessionActive_)
            {
                appendLine ("Command: " + lastCanonical_, LineKind::Prompt);
                applyResult (onExecuteCommand (CommandRegistry::toHandlerKey (lastCanonical_), {}));
            }
            return;
        }

        pushHistory (line);
        appendLine ("Command: " + line, LineKind::Prompt);

        if (sessionActive_)
        {
            if (onSessionInput)
                applyResult (onSessionInput (line));
            else
            {
                sessionActive_ = false;
                appendError ("Session aborted (no handler).");
            }
            return;
        }

        juce::String verbTok, args;
        const int sp = line.indexOfChar (' ');
        if (sp < 0)
            verbTok = line;
        else
        {
            verbTok = line.substring (0, sp);
            args = line.substring (sp + 1).trim();
        }

        const juce::String canonical = CommandRegistry::resolve (verbTok);
        if (canonical.isEmpty())
        {
            appendError ("Unknown command \"" + verbTok.toUpperCase() + "\".");
            return;
        }

        if (canonical == "CLS")
        {
            clearScreen();
            lastCanonical_ = {};
            return;
        }

        if (onExecuteCommand == nullptr)
        {
            appendError ("Command host is not connected.");
            return;
        }

        lastCanonical_ = canonical;
        applyResult (onExecuteCommand (CommandRegistry::toHandlerKey (canonical), args));
    }

    void applyResult (const CommandResult& r)
    {
        switch (r.kind)
        {
            case CommandResult::Kind::Ok:
                if (r.message.isNotEmpty())
                    appendMultiline (r.message, LineKind::Result);
                break;
            case CommandResult::Kind::Fail:
                appendMultiline (r.message.isNotEmpty() ? r.message
                                                       : juce::String ("Command failed."),
                                 LineKind::Error);
                break;
            case CommandResult::Kind::ContinueSession:
                sessionActive_ = true;
                if (r.message.isNotEmpty())
                    appendMultiline (r.message, LineKind::Result);
                sessionHint_ = r.nextPrompt;
                if (sessionHint_.isNotEmpty())
                    appendMultiline (sessionHint_, LineKind::Result);
                break;
            case CommandResult::Kind::EndSession:
                sessionActive_ = false;
                sessionHint_.clear();
                if (r.message.isNotEmpty())
                    appendMultiline (r.message, LineKind::Result);
                break;
        }
    }

    void clearScreen()
    {
        lines_.clear();
        refreshOutput();
    }

    void appendMultiline (const juce::String& s, LineKind kind)
    {
        juce::StringArray rows;
        rows.addLines (s);
        if (rows.isEmpty())
            rows.add ({});
        for (auto& row : rows)
            lines_.push_back ({ row, kind });
        trimHistory();
        refreshOutput();
    }

    void appendLine (const juce::String& s, LineKind kind)
    {
        appendMultiline (s, kind);
    }

    void trimHistory()
    {
        while (lines_.size() > 400)
            lines_.pop_front();
    }

    juce::Colour colourFor (LineKind kind) const
    {
        switch (kind)
        {
            case LineKind::Prompt: return accent();
            case LineKind::Error:  return error();
            case LineKind::Result:
            case LineKind::Normal:
            default:               return fg();
        }
    }

    void refreshOutput()
    {
        const int viewW = juce::jmax (1, viewport_.getWidth());
        const int contentH = juce::jmax (viewport_.getHeight(), outputContent_.preferredHeight());
        outputContent_.setSize (viewW, contentH);
        outputContent_.setLines (&lines_, [this] (LineKind k) { return colourFor (k); });
        viewport_.setViewPosition (0, juce::jmax (0, contentH - viewport_.getHeight()));
    }

    juce::Viewport    viewport_;
    OutputContent     outputContent_;
    juce::Label       prompt_;
    juce::TextEditor  input_;
    std::deque<Line>  lines_;
    bool              sessionActive_ = false;
    juce::String      sessionHint_;
    juce::String      lastCanonical_;
    std::vector<juce::String> history_;
    int               historyIndex_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CommandTerminal)
};

// ---------------------------------------------------------------------------
// Floating terminal window — undocked from the main bottom panel.
// ---------------------------------------------------------------------------
class TerminalFloatWindow : public juce::DocumentWindow
{
public:
    TerminalFloatWindow (CommandTerminal& terminal, std::function<void()> onRequestDock)
        : DocumentWindow ("Terminal",
                          Brand::panelDark(),
                          DocumentWindow::closeButton | DocumentWindow::minimiseButton),
          onRequestDock_ (std::move (onRequestDock))
    {
        setUsingNativeTitleBar (true);
        setResizable (true, false);
        // false: do not shrink/grow the float when content bounds change (avoids
        // looking "minimised" when the host layouts other UI).
        setContentNonOwned (&terminal, false);
        setResizeLimits (420, 220, 1600, 1000);
        centreWithSize (720, 380);
    }

    void closeButtonPressed() override
    {
        if (onRequestDock_)
            onRequestDock_();
    }

    bool keyPressed (const juce::KeyPress& key) override
    {
        // Esc cancels the in-progress command inside CommandTerminal — never
        // treat it as a window close/minimise shortcut.
        if (key.isKeyCode (juce::KeyPress::escapeKey))
            return true;
        return DocumentWindow::keyPressed (key);
    }

    void lookAndFeelChanged() override
    {
        DocumentWindow::lookAndFeelChanged();
        setBackgroundColour (Brand::panelDark());
    }

private:
    std::function<void()> onRequestDock_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TerminalFloatWindow)
};
