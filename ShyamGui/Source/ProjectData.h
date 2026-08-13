#pragma once
#include <JuceHeader.h>
#include "AcousticEngine.h"
#include <vector>

// ===========================================================================
// ProjectData - the persistent unit of work. Bundles client/engineering
// metadata with the acoustic scene (speakers + simulation settings) and
// serialises to a single self-contained ".atmk" JSON file.
//
// The metadata travels with the project so it can be reused by heatmap exports
// (Phase 3) and the full PDF report (Phase 4). The scene fields mirror the
// editable inputs in ControlPanel / SimParams; the acoustic engine is fed from
// these unchanged.
// ===========================================================================

struct ProjectMeta
{
    juce::String projectName;
    juce::String engineerName;
    juce::String ownerName;
    juce::String address;
    juce::String city;
    juce::String country;
    juce::String email;
    juce::String mobile;
    juce::String date;        // free-form, defaults to today

    static juce::String today()
    {
        return juce::Time::getCurrentTime().formatted ("%d %b %Y");
    }
};

struct ProjectData
{
    ProjectMeta meta;

    // --- Acoustic scene (mirrors the editable simulation inputs) -----------
    std::vector<Speaker> speakers;
    double frequency           = 52.0;
    int    resolution          = 400;
    double dBfloor             = -36.0;
    bool   bandedSPL           = false;
    bool   octaveSmoothing     = true;
    bool   useMeasuredDirectivity = true;

    juce::File file;   // backing file on disk (empty until first save)

    // -----------------------------------------------------------------------
    // Older builds parked the factory array on the lower edge (y = 10 m,
    // x ≈ 10…20). New defaults use the world centre (50, 50). Remap only that
    // legacy factory pattern so Add / Reset / New Project all match.
    static void migrateLegacyCornerDefaults (std::vector<Speaker>& speakers)
    {
        if (speakers.empty() || speakers.size() > 3) return;

        for (const auto& s : speakers)
        {
            if (s.y < 9.95f || s.y > 10.05f) return;
            if (s.x < 9.5f || s.x > 22.0f) return;
        }

        float midX = 0.0f;
        for (const auto& s : speakers) midX += s.x;
        midX /= (float) speakers.size();

        const float dx = 50.0f - midX;
        const float dy = 40.0f;   // 10 → 50
        for (auto& s : speakers)
        {
            s.x += dx;
            s.y += dy;
        }
    }

    static ProjectData makeDefault (const ProjectMeta& m)
    {
        ProjectData p;
        p.meta = m;
        p.speakers.push_back ({ 50.0f, 50.0f, 0.0f, 0.0f, false, false, true });
        return p;
    }

    // --- JSON serialisation ------------------------------------------------
    juce::var toVar() const
    {
        auto* root = new juce::DynamicObject();

        auto* m = new juce::DynamicObject();
        m->setProperty ("projectName",  meta.projectName);
        m->setProperty ("engineerName", meta.engineerName);
        m->setProperty ("ownerName",    meta.ownerName);
        m->setProperty ("address",      meta.address);
        m->setProperty ("city",         meta.city);
        m->setProperty ("country",      meta.country);
        m->setProperty ("email",        meta.email);
        m->setProperty ("mobile",       meta.mobile);
        m->setProperty ("date",         meta.date);
        root->setProperty ("meta", juce::var (m));

        root->setProperty ("frequency",  frequency);
        root->setProperty ("resolution", resolution);
        root->setProperty ("dBfloor",    dBfloor);
        root->setProperty ("bandedSPL",  bandedSPL);
        root->setProperty ("octaveSmoothing", octaveSmoothing);
        root->setProperty ("useMeasuredDirectivity", useMeasuredDirectivity);

        juce::Array<juce::var> spk;
        for (const auto& s : speakers)
        {
            auto* o = new juce::DynamicObject();
            o->setProperty ("x", s.x);
            o->setProperty ("y", s.y);
            o->setProperty ("gainDB", s.gainDB);
            o->setProperty ("delayMs", s.delayMs);
            o->setProperty ("polarityInverted", s.polarityInverted);
            o->setProperty ("reverseOrientation", s.reverseOrientation);
            o->setProperty ("enabled", s.enabled);
            spk.add (juce::var (o));
        }
        root->setProperty ("speakers", spk);
        root->setProperty ("format", "atmk-1");

        return juce::var (root);
    }

    static ProjectData fromVar (const juce::var& v)
    {
        ProjectData p;
        if (auto* root = v.getDynamicObject())
        {
            if (auto mv = root->getProperty ("meta"); auto* m = mv.getDynamicObject())
            {
                p.meta.projectName  = m->getProperty ("projectName").toString();
                p.meta.engineerName = m->getProperty ("engineerName").toString();
                p.meta.ownerName    = m->getProperty ("ownerName").toString();
                p.meta.address      = m->getProperty ("address").toString();
                p.meta.city         = m->getProperty ("city").toString();
                p.meta.country      = m->getProperty ("country").toString();
                p.meta.email        = m->getProperty ("email").toString();
                p.meta.mobile       = m->getProperty ("mobile").toString();
                p.meta.date         = m->getProperty ("date").toString();
            }

            if (root->hasProperty ("frequency"))  p.frequency  = (double) root->getProperty ("frequency");
            if (root->hasProperty ("resolution")) p.resolution = (int)    root->getProperty ("resolution");
            if (root->hasProperty ("dBfloor"))    p.dBfloor    = (double) root->getProperty ("dBfloor");
            if (root->hasProperty ("bandedSPL"))  p.bandedSPL  = (bool)   root->getProperty ("bandedSPL");
            if (root->hasProperty ("octaveSmoothing"))       p.octaveSmoothing = (bool) root->getProperty ("octaveSmoothing");
            if (root->hasProperty ("useMeasuredDirectivity")) p.useMeasuredDirectivity = (bool) root->getProperty ("useMeasuredDirectivity");

            if (auto sv = root->getProperty ("speakers"); sv.isArray())
            {
                p.speakers.clear();
                for (auto& e : *sv.getArray())
                {
                    if (auto* o = e.getDynamicObject())
                    {
                        Speaker s;
                        s.x       = (float) (double) o->getProperty ("x");
                        s.y       = (float) (double) o->getProperty ("y");
                        s.gainDB  = (float) (double) o->getProperty ("gainDB");
                        s.delayMs = (float) (double) o->getProperty ("delayMs");
                        s.polarityInverted   = (bool) o->getProperty ("polarityInverted");
                        s.reverseOrientation = (bool) o->getProperty ("reverseOrientation");
                        s.enabled            = o->hasProperty ("enabled") ? (bool) o->getProperty ("enabled") : true;
                        p.speakers.push_back (s);
                    }
                }
            }
        }
        if (p.speakers.empty())
            p.speakers.push_back ({ 50.0f, 50.0f, 0.0f, 0.0f, false, false, true });
        migrateLegacyCornerDefaults (p.speakers);
        return p;
    }

    // --- File I/O ----------------------------------------------------------
    bool saveToFile (const juce::File& f)
    {
        const juce::String json = juce::JSON::toString (toVar(), false);
        if (f.replaceWithText (json))
        {
            file = f;
            return true;
        }
        return false;
    }

    static bool loadFromFile (const juce::File& f, ProjectData& out)
    {
        if (! f.existsAsFile()) return false;
        juce::var parsed;
        if (juce::JSON::parse (f.loadFileAsString(), parsed).failed()) return false;
        out = fromVar (parsed);
        out.file = f;
        return true;
    }

    juce::String displayName() const
    {
        if (meta.projectName.isNotEmpty()) return meta.projectName;
        if (file != juce::File()) return file.getFileNameWithoutExtension();
        return "Untitled Project";
    }
};
