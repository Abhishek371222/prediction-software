#pragma once
#include <JuceHeader.h>

// ===========================================================================
// AppSettings - persistent user preferences (theme + unit system), shared
// app-wide via a singleton. Persisted with juce::PropertiesFile so choices
// survive restarts. Components observe changes via juce::ChangeBroadcaster.
//
// IMPORTANT: this only affects *display* and *styling*. The acoustic engine
// and all internal calculations stay in SI units regardless of the selection.
// ===========================================================================

enum class ThemeMode  { Dark = 0, Light = 1 };
enum class UnitSystem { SI = 0, Imperial = 1 };

class AppSettings : public juce::ChangeBroadcaster
{
public:
    static AppSettings& get()
    {
        static AppSettings instance;
        return instance;
    }

    ThemeMode  theme() const noexcept { return theme_; }
    UnitSystem units() const noexcept { return units_; }

    bool isDark()     const noexcept { return theme_ == ThemeMode::Dark; }
    bool isImperial() const noexcept { return units_ == UnitSystem::Imperial; }

    void setTheme (ThemeMode t)
    {
        if (t == theme_) return;
        theme_ = t;
        store();
        sendChangeMessage();
    }

    void setUnits (UnitSystem u)
    {
        if (u == units_) return;
        units_ = u;
        store();
        sendChangeMessage();
    }

    bool showGrid() const noexcept { return showGrid_; }

    void setShowGrid (bool b)
    {
        if (b == showGrid_) return;
        showGrid_ = b;
        store();
        sendChangeMessage();
    }

    bool sidebarCollapsed() const noexcept { return sidebarCollapsed_; }

    void setSidebarCollapsed (bool b)
    {
        if (b == sidebarCollapsed_) return;
        sidebarCollapsed_ = b;
        store();
        sendChangeMessage();
    }

    // --- Recent projects (most-recent first, capped) -----------------------
    juce::StringArray recentProjects() const
    {
        juce::StringArray raw;
        if (props_ != nullptr)
            raw.addLines (props_->getValue ("recentProjects", {}));
        raw.removeEmptyStrings();

        // Only surface projects whose .atmk file still exists, and prune any
        // dead entries from storage so they never reappear.
        juce::StringArray out;
        for (const auto& p : raw)
            if (juce::File (p).existsAsFile())
                out.add (p);

        if (out.size() != raw.size() && props_ != nullptr)
        {
            props_->setValue ("recentProjects", out.joinIntoString ("\n"));
            props_->saveIfNeeded();
        }
        return out;
    }

    // --- Measurement dataset (0 = Q21S / OpenField, 1 = Room / ShyamGuild legacy)
    // Default = Q21S BEM polar set from MeasurementIntegrationPack.
    int measurementSource() const
    {
        return props_ != nullptr ? props_->getIntValue ("measurementSource", 0) : 0;
    }

    void setMeasurementSource (int s)
    {
        if (props_ == nullptr) return;
        props_->setValue ("measurementSource", s);
        props_->saveIfNeeded();
    }

    void addRecentProject (const juce::File& f)
    {
        if (props_ == nullptr || f == juce::File()) return;
        auto list = recentProjects();
        const auto path = f.getFullPathName();
        list.removeString (path);
        list.insert (0, path);
        while (list.size() > 8) list.remove (list.size() - 1);
        props_->setValue ("recentProjects", list.joinIntoString ("\n"));
        props_->saveIfNeeded();
    }

private:
    AppSettings()
    {
        juce::PropertiesFile::Options o;
        o.applicationName     = "AtomikAcousticEngine";
        o.filenameSuffix      = "settings";
        o.osxLibrarySubFolder = "Application Support";
        o.folderName          = "Atomik";
        props_ = std::make_unique<juce::PropertiesFile> (o);

        theme_ = (ThemeMode)  props_->getIntValue ("theme", (int) ThemeMode::Dark);
        units_ = (UnitSystem) props_->getIntValue ("units", (int) UnitSystem::SI);
        showGrid_ = props_->getBoolValue ("showGrid", true);
        sidebarCollapsed_ = props_->getBoolValue ("sidebarCollapsed", false);
    }

    void store()
    {
        if (props_ == nullptr) return;
        props_->setValue ("theme", (int) theme_);
        props_->setValue ("units", (int) units_);
        props_->setValue ("showGrid", showGrid_);
        props_->setValue ("sidebarCollapsed", sidebarCollapsed_);
        props_->saveIfNeeded();
    }

    std::unique_ptr<juce::PropertiesFile> props_;
    ThemeMode  theme_ = ThemeMode::Dark;
    UnitSystem units_ = UnitSystem::SI;
    bool       showGrid_ = true;
    bool       sidebarCollapsed_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AppSettings)
};

// ---------------------------------------------------------------------------
// Units - conversion + formatting helpers. Internal values are ALWAYS SI
// (metres, millimetres, kilograms, Celsius); these only convert for display.
// ---------------------------------------------------------------------------
namespace Units
{
    inline bool imperial() { return AppSettings::get().isImperial(); }

    inline juce::String degree() { return juce::String::charToString ((juce::juce_wchar) 0x00B0); }

    // ---- Length (base unit: metre) ---------------------------------------
    inline const char* lengthUnit() { return imperial() ? "ft" : "m"; }
    inline double metresToDisplay (double m) { return imperial() ? m * 3.280839895 : m; }
    inline double displayToMetres (double d) { return imperial() ? d / 3.280839895 : d; }

    // ---- Small length (base unit: millimetre) ----------------------------
    inline const char* smallLengthUnit() { return imperial() ? "in" : "mm"; }
    inline double mmToDisplay (double mm) { return imperial() ? mm / 25.4 : mm; }
    inline double displayToMm (double d)  { return imperial() ? d * 25.4 : d; }

    // ---- Mass (base unit: kilogram) --------------------------------------
    inline const char* massUnit() { return imperial() ? "lbs" : "kg"; }
    inline double kgToDisplay (double kg) { return imperial() ? kg * 2.2046226218 : kg; }
    inline double displayToKg (double d)  { return imperial() ? d / 2.2046226218 : d; }

    // ---- Temperature (base unit: Celsius) --------------------------------
    inline juce::String tempUnit() { return degree() + (imperial() ? "F" : "C"); }
    inline double celsiusToDisplay (double c) { return imperial() ? c * 9.0 / 5.0 + 32.0 : c; }
    inline double displayToCelsius (double d) { return imperial() ? (d - 32.0) * 5.0 / 9.0 : d; }

    // Convenience formatter: "<value> <unit>" for a metre quantity.
    inline juce::String metres (double m, int decimals = 1)
    {
        return juce::String (metresToDisplay (m), decimals) + " " + lengthUnit();
    }
}
