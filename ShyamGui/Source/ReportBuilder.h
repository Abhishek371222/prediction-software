#pragma once
#include <JuceHeader.h>
#include "PdfDocument.h"
#include "BrandTheme.h"
#include "ProjectData.h"
#include "AppSettings.h"
#include "AcousticEngine.h"
#include "AcousticAnalysis.h"
#include <vector>

// ===========================================================================
// ReportBuilder - assembles the full professional PDF engineering report from
// project metadata, simulation inputs/results, derived metrics, graphs and
// heatmaps. Structure mirrors an AFMG-style deliverable: cover, project info,
// input parameters, results, graphs, heatmaps, summary.
// ===========================================================================
namespace ReportBuilder
{
    struct HeatmapEntry
    {
        int hz = 0;
        juce::Image image;
        double coveragePct = 0.0;
        double peakAbsDb = 0.0;
        bool   hasAbsoluteSpl = false;
        AcousticAnalysis::FieldStats stats;   // read off this page's own solve
    };

    struct ReportInputs
    {
        ProjectData project;
        SimParams   params;
        SimResult   result;
        AcousticAnalysis::RoomModel room;

        std::vector<double> rt60, absorption, transmissionLoss;
        std::vector<double> frFreq, frDb;

        std::vector<HeatmapEntry> heatmaps;
        AcousticAnalysis::FieldStats current;   // the scene at its own frequency
        juce::Image frGraph, rt60Graph, absGraph, tlGraph;
    };

    // Rasterise the Atomik wordmark onto a solid tile (PDF images have no alpha).
    inline juce::Image logoTile (int w, int h, juce::Colour fg, juce::Colour bg)
    {
        juce::Image img (juce::Image::RGB, w, h, true);
        juce::Graphics g (img);
        g.fillAll (bg);
        if (auto d = Brand::createLogo (fg))
            d->drawWithin (g, juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (6.0f),
                           juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize, 1.0f);
        return img;
    }

    namespace detail
    {
        inline const juce::Colour ink    { 0xff20242a };
        inline const juce::Colour sub    { 0xff5b6068 };
        inline const juce::Colour lineCol{ 0xffd5d8de };
        inline const juce::Colour panel  { 0xfff5f6f8 };

        inline juce::String S (const juce::String& s) { return s.isEmpty() ? juce::String ("-") : s; }

        // Fixed decimals with no "-0.0": a value that rounds to zero is zero.
        inline juce::String num (double v, int dp)
        {
            const double q = std::pow (10.0, dp);
            if (std::abs (v) * q < 0.5) v = 0.0;
            return juce::String (v, dp);
        }

        inline juce::String level (const AcousticAnalysis::FieldStats& s, double v)
        {
            return num (v, 1) + (s.absolute ? " dB SPL" : " dB (rel.)");
        }

        inline juce::String regionText (const AcousticAnalysis::FieldStats& s)
        {
            if (! s.valid) return "-";
            return juce::String (s.audience ? "Audience areas, " : "Field beyond 2 m of cabinets, ")
                 + num (s.areaM2, 0) + " sq m";
        }
    }

    inline void build (PdfDocument& pdf, const ReportInputs& in)
    {
        using namespace detail;
        const double W = pdf.pageWidth();
        const double H = pdf.pageHeight();
        const double M = 52.0;
        const double cw = W - 2 * M;
        const auto& meta = in.project.meta;
        const juce::String len = Units::lengthUnit();

        const juce::String softwareName = "Atomik Integral";

        // Clean client report: cover, info, inputs, results, heatmaps, summary.
        // FR / RT60 / AC / TL graph pages removed.
        const int nHeatPages  = (int) in.heatmaps.size();
        const int totalPages  = 1 /*cover*/ + 1 /*info*/ + 1 /*inputs*/ + 1 /*results*/
                              + nHeatPages + 1 /*summary*/;
        int pageNo = 0;

        // Footer only - no running header (cleaner client-facing pages).
        auto chrome = [&] (const juce::String& /*runningTitle*/, bool /*topHeader*/ = true)
        {
            ++pageNo;
            const auto now = juce::Time::getCurrentTime();
            const juce::String dateStr = now.formatted ("%d %b %Y") + "  " + now.formatted ("%H:%M:%S");
            pdf.line (M, H - 40, W - M, H - 40, lineCol, 0.8);
            pdf.text (M, H - 34, 9, meta.projectName.isEmpty() ? "Untitled Project" : meta.projectName, sub);
            pdf.textCentred (W * 0.5, H - 34, 9, "www.atomikaudio.com", sub);
            pdf.textRight (W - M, H - 34, 9,
                           dateStr + "   |   Page " + juce::String (pageNo) + " / " + juce::String (totalPages), sub);
        };

        auto sectionTitle = [&] (double y, const juce::String& t) -> double
        {
            pdf.fillRect (M, y, 5, 22, Brand::accent());
            pdf.text (M + 12, y + 2, 30, t, ink, true);
            pdf.line (M, y + 28, W - M, y + 28, lineCol, 0.8);
            return y + 44;
        };

        // key/value row inside a two-column block
        auto kv = [&] (double x, double y, double colW, const juce::String& k, const juce::String& v)
        {
            juce::ignoreUnused (colW);
            pdf.text (x, y, 10.5, k.toUpperCase(), sub, true);
            pdf.text (x, y + 14, 13.0, S (v), ink, false);
        };

        auto fitImage = [&] (const juce::Image& im, double x, double y, double maxW, double maxH)
        {
            if (im.isNull()) return;
            const double ar = (double) im.getWidth() / juce::jmax (1, im.getHeight());
            double w = maxW, h = w / ar;
            if (h > maxH) { h = maxH; w = h * ar; }
            pdf.drawImage (im, x + (maxW - w) * 0.5, y, w, h);
        };

        // =================================================================
        // 1) COVER PAGE
        // =================================================================
        pdf.newPage();
        pdf.fillRect (0, 0, W, 168, Brand::charcoal());
        pdf.drawImage (logoTile (430, 70, Brand::white(), Brand::charcoal()),
                       M, 40, 215, 52);
        pdf.textRight (W - M, 64, 13, softwareName.toUpperCase(), juce::Colour (0xffd7d9de), false);
        pdf.textRight (W - M, 84, 10, "v1.5.1  -  Q21S / Q212P Coverage & Directivity", juce::Colour (0xff9aa0a8), false);

        pdf.text (M, 300, 14, "ACOUSTIC SIMULATION REPORT", Brand::accent(), true);
        pdf.text (M, 322, 36, S (meta.projectName), ink, true);
        pdf.line (M, 376, W - M, 376, lineCol, 1.0);

        pdf.text (M, 400, 11, "PREPARED BY", sub, true);
        pdf.text (M, 414, 14, S (meta.engineerName), ink);
        pdf.text (M, 446, 11, "PREPARED FOR", sub, true);
        pdf.text (M, 460, 14, S (meta.ownerName), ink);
        pdf.text (M, 492, 11, "DATE", sub, true);
        pdf.text (M, 506, 14, juce::Time::getCurrentTime().formatted ("%d %b %Y  %H:%M:%S"), ink);

        pdf.fillRect (M, H - 150, cw, 60, panel);
        pdf.text (M + 16, H - 138, 10, "SITE", sub, true);
        pdf.text (M + 16, H - 124, 12,
                  juce::String (S (meta.city)) + (meta.country.isNotEmpty() ? ", " + meta.country : juce::String()),
                  ink);
        pdf.text (M + 16, H - 106, 10, S (meta.address), sub);
        chrome ("Cover", false);

        // =================================================================
        // 2) PROJECT INFORMATION
        // =================================================================
        pdf.newPage();
        double y = sectionTitle (56, "1.  Project Information");
        {
            const double colW = (cw - 30) / 2.0;
            const double x0 = M, x1 = M + colW + 30;
            const double rh = 46;
            struct P { const char* k; juce::String v; };
            std::vector<P> left  { {"Project Name", meta.projectName}, {"Engineer", meta.engineerName},
                                   {"Owner", meta.ownerName}, {"Email", meta.email},
                                   {"Date", juce::Time::getCurrentTime().formatted ("%d %b %Y  %H:%M:%S")} };
            std::vector<P> right { {"Address", meta.address}, {"City", meta.city},
                                   {"Country", meta.country}, {"Mobile", meta.mobile},
                                   {"Software", softwareName} };
            double yy = y;
            for (auto& p : left)  { kv (x0, yy, colW, p.k, p.v); yy += rh; }
            yy = y;
            for (auto& p : right) { kv (x1, yy, colW, p.k, p.v); yy += rh; }
        }
        chrome ("Project Information");

        // =================================================================
        // 3) INPUT PARAMETERS
        // =================================================================
        pdf.newPage();
        y = sectionTitle (56, "2.  Input Parameters");
        {
            const double colW = (cw - 30) / 2.0;
            const double x1 = M + colW + 30;
            auto row = [&] (double x, double yy, const juce::String& k, const juce::String& v)
            { kv (x, yy, colW, k, v); };

            const double rh = 44;
            double yy = y;
            row (M, yy, "World Width",  juce::String (Units::metresToDisplay (in.params.worldW), 1) + " " + len); yy += rh;
            row (M, yy, "World Depth",  juce::String (Units::metresToDisplay (in.params.worldH), 1) + " " + len); yy += rh;
            row (M, yy, "Assumed Ceiling Ht.", juce::String (Units::metresToDisplay (in.room.h), 1) + " " + len);        yy += rh;
            row (M, yy, "Grid Resolution",     juce::String (in.params.resolution) + " x " + juce::String (in.params.resolution)); yy += rh;

            yy = y;
            row (x1, yy, "Display Range",       "0 to " + juce::String ((int) in.params.dBfloor) + " dB"); yy += rh;
            row (x1, yy, "1/3-Octave Smoothing", "On");                yy += rh;
            row (x1, yy, "Measured Directivity", "On (always)");          yy += rh;

            double ty = juce::jmax (y + 5 * rh, yy) + 16;
            ty = sectionTitle (ty, "2.1  Speaker Placement");

            // Speaker table - every value the Sources panel holds for a box:
            // where it is (incl. height), where it points, how it is mounted.
            //                   Unit X   Y   Z   H.Aim V.Aim Mount Gain Delay State
            const double cwid[] = { 62, 42, 42, 42, 46,   44,   54,   40,  48 };
            double c[10]; c[0] = M;
            for (int k = 0; k < 9; ++k) c[k + 1] = c[k] + cwid[k];
            pdf.fillRect (M, ty, cw, 20, panel);
            const juce::String hdr[] = { "UNIT", "X (" + len + ")", "Y (" + len + ")", "Z (" + len + ")",
                                         "H. AIM", "V. AIM", "MOUNT", "GAIN", "DELAY", "STATE" };
            for (int k = 0; k < 10; ++k) pdf.text (c[k] + 4, ty + 6, 8, hdr[k], sub, true);
            ty += 22;
            const auto& sp = in.params.speakers;
            for (int i = 0; i < (int) sp.size(); ++i)
            {
                const auto& s = sp[(size_t) i];
                double head = s.rotationDeg;   // as the Sources panel's Horizontal Aim shows it
                while (head > 180.0)   head -= 360.0;
                while (head <= -180.0) head += 360.0;
                const int mount = sourceMount (sp, i);
                const char* mountName = mount == Mount::Flown ? "Flown"
                                      : mount == Mount::Stacked ? "Stacked" : "Ground";

                pdf.line (M, ty + 18, W - M, ty + 18, lineCol, 0.6);
                const juce::String cells[] = {
                    juce::String (speakerModelTag (s.model)) + "-" + juce::String (speakerModelOrdinal (sp, i)),
                    num (Units::metresToDisplay (s.x), 1),
                    num (Units::metresToDisplay (s.y), 1),
                    num (Units::metresToDisplay (s.baseHeightM), 2),
                    num (head, 0) + juce::String (" deg"),
                    num (-s.tiltDeg, 0) + juce::String (" deg"),
                    mountName,
                    num (s.gainDB, 0) + " dB",
                    num (s.delayMs, 1) + " ms",
                    s.enabled ? "Enabled" : "Off" };
                for (int k = 0; k < 10; ++k) pdf.text (c[k] + 4, ty + 4, 9, cells[k], ink);
                ty += 22;
                if (ty > H - 110) break;
            }
            pdf.text (M, ty + 8, 8.5,
                      "Z = underside of the cabinet above the floor.  H. Aim: counter-clockwise from +X.  "
                      "V. Aim: positive = up.", sub);
        }
        chrome ("Input Parameters");

        // =================================================================
        // 4) SIMULATION RESULTS (summary numbers)
        // =================================================================
        pdf.newPage();
        y = sectionTitle (56, "3.  Simulation Results");
        {
            const auto& st = in.current;
            int nDev = 0; for (auto& s : in.params.speakers) if (s.enabled) ++nDev;
            auto pct = [] (const AcousticAnalysis::FieldStats& s, double v)
            { return s.valid ? num (v, 1) + " %" : juce::String ("-"); };
            auto lvl = [] (const AcousticAnalysis::FieldStats& s, double v)
            { return s.valid ? level (s, v) : juce::String ("-"); };

            const double colW = (cw - 30) / 2.0;
            const double x1 = M + colW + 30;
            const double rh = 46;
            double yy = y;
            kv (M, yy, colW, "Active Devices",      juce::String (nDev)); yy += rh;
            kv (M, yy, colW, "Frequency (current)", juce::String ((int) (in.params.frequency + 0.5)) + " Hz"); yy += rh;
            kv (M, yy, colW, "Wavelength",          num (Units::metresToDisplay (in.result.lambda), 2) + " " + len); yy += rh;
            kv (M, yy, colW, "Evaluated region",    regionText (st)); yy += rh;
            kv (M, yy, colW, "Coverage within 3 dB", pct (st, st.cov3)); yy += rh;
            kv (M, yy, colW, "Coverage within 6 dB", pct (st, st.cov6)); yy += rh;

            yy = y;
            kv (x1, yy, colW, "Maximum level",   lvl (st, st.maxDb)); yy += rh;
            kv (x1, yy, colW, "Average level",   lvl (st, st.avgDb)); yy += rh;
            kv (x1, yy, colW, "Minimum level",   lvl (st, st.minDb)); yy += rh;
            kv (x1, yy, colW, "Std. deviation",  st.valid ? num (st.stdDb, 1) + " dB" : juce::String ("-")); yy += rh;
            kv (x1, yy, colW, "Directivity model", in.result.usedMeasuredDirectivity ? "Measured" : "Model"); yy += rh;
            kv (x1, yy, colW, "Directivity Plot frequencies", juce::String ((int) in.heatmaps.size())); yy += rh;

            double ny = y + 6 * rh + 10;
            ny = sectionTitle (ny, "3.1  Overview");
            pdf.textWrapped (M, ny, cw, 12.0,
                juce::String ("Values are solved for the scene as exported, at the current frequency. ")
                + (st.absolute ? "Levels are absolute dB SPL from each cabinet's measured calibration. "
                               : "Levels are relative dB (not every unit is calibrated for absolute SPL). ")
                + (st.audience ? "Statistics cover the drawn audience areas only, at seated ear height. "
                               : "No audience area is drawn, so statistics cover the whole field except the "
                                 "2 m around each cabinet, where the level is dominated by the box itself. ")
                + "Coverage is the share of that region within 3 dB / 6 dB of its loudest point; "
                  "the standard deviation is the spread of level across it (lower = more even).",
                sub, false);
        }
        chrome ("Simulation Results");

        // =================================================================
        // 4) SPL HEATMAPS (one page per frequency)
        // =================================================================
        int plotNo = 0;
        for (const auto& hmEntry : in.heatmaps)
        {
            ++plotNo;
            const auto& st = hmEntry.stats;
            pdf.newPage();
            double hy = sectionTitle (56, "4." + juce::String (plotNo) + "  Directivity Plot - "
                                          + juce::String (hmEntry.hz) + " Hz");

            // The plot fills the text width; the canvas is drawn 5:4 to match.
            const double imgH = cw * 0.80;
            fitImage (hmEntry.image, M, hy, cw, imgH);
            double yy = hy + imgH + 14;

            // Headline numbers, directly under the plot, five even columns.
            const double bandH = 50;
            pdf.fillRect (M, yy, cw, bandH, Brand::charcoal());
            pdf.fillRect (M, yy, 4, bandH, Brand::accent());
            const double colW5 = (cw - 8) / 5.0;
            const juce::Colour capK (0xff9aa0a8), capV (0xffffffff);
            auto cell = [&] (int k, const juce::String& key, const juce::String& val)
            {
                const double x = M + 12 + k * colW5;
                pdf.text (x, yy + 10, 7.5, key, capK, true);
                pdf.text (x, yy + 26, 12, val, capV, false);
            };
            const juce::String u = st.absolute ? " dB SPL" : " dB";
            cell (0, "FREQUENCY", juce::String (hmEntry.hz) + " Hz");
            cell (1, "MAX LEVEL",   st.valid ? num (st.maxDb, 1) + u : juce::String ("-"));
            cell (2, "AVERAGE",     st.valid ? num (st.avgDb, 1) + u : juce::String ("-"));
            cell (3, "WITHIN 6 dB", st.valid ? num (st.cov6, 1) + " %" : juce::String ("-"));
            cell (4, "WITHIN 3 dB", st.valid ? num (st.cov3, 1) + " %" : juce::String ("-"));
            yy += bandH + 20;

            // The rest of what this solve says, in three columns.
            const double colW3 = cw / 3.0;
            const double rh = 40;
            kv (M,             yy, colW3, "Evaluated region", regionText (st));
            yy += rh;
            kv (M,             yy, colW3, "Minimum level",  st.valid ? level (st, st.minDb) : juce::String ("-"));
            kv (M + colW3,     yy, colW3, "Std. deviation", st.valid ? num (st.stdDb, 1) + " dB" : juce::String ("-"));
            kv (M + 2 * colW3, yy, colW3, "Wavelength",
                num (Units::metresToDisplay (343.0 / juce::jmax (1, hmEntry.hz)), 2) + " " + len);
            yy += rh;
            kv (M,             yy, colW3, "Map scale",
                "0 to " + juce::String ((int) in.params.dBfloor) + " dB (rel.)");
            kv (M + colW3,     yy, colW3, "Peak on map",
                hmEntry.hasAbsoluteSpl ? num (hmEntry.peakAbsDb, 1) + " dB SPL" : juce::String ("0 dB (rel.)"));
            kv (M + 2 * colW3, yy, colW3, "Level type", st.absolute ? "Absolute (calibrated)" : "Relative");
            chrome ("Directivity Plot");
        }

        // =================================================================
        // 5) SUMMARY
        // =================================================================
        pdf.newPage();
        y = sectionTitle (56, "5.  Summary & Recommendations");
        {
            const auto& st = in.current;

            // Weakest / strongest page by evenness (6 dB coverage), real solves only.
            const HeatmapEntry* weak = nullptr;
            const HeatmapEntry* best = nullptr;
            for (const auto& h : in.heatmaps)
            {
                if (! h.stats.valid) continue;
                if (weak == nullptr || h.stats.cov6 < weak->stats.cov6) weak = &h;
                if (best == nullptr || h.stats.cov6 > best->stats.cov6) best = &h;
            }

            // Fleet, by each model's own name.
            int nModel[4] = { 0, 0, 0, 0 };
            for (auto& s : in.params.speakers)
                if (s.enabled && s.model >= 0 && s.model < 4) ++nModel[s.model];
            juce::String fleet;
            for (int m : { 3, 0, 2 })
                if (nModel[m] > 0)
                    fleet << (fleet.isEmpty() ? "" : " + ") << nModel[m] << " x " << speakerModelTag (m);
            if (fleet.isEmpty()) fleet = "No active units";

            pdf.text (M, y, 14, "Key Findings", ink, true); y += 22;
            juce::StringArray findings;
            findings.add (fleet + ", simulated over a "
                          + num (Units::metresToDisplay (in.params.worldW), 0) + " x "
                          + num (Units::metresToDisplay (in.params.worldH), 0) + " " + len + " plane.");
            if (st.valid)
            {
                findings.add ("At " + juce::String ((int) (in.params.frequency + 0.5)) + " Hz, over "
                              + regionText (st).toLowerCase() + ": maximum " + level (st, st.maxDb)
                              + ", average " + level (st, st.avgDb)
                              + ", spread " + num (st.stdDb, 1) + " dB (std. dev.).");
                findings.add (num (st.cov6, 1) + " % of that region is within 6 dB of its loudest point, "
                              + num (st.cov3, 1) + " % within 3 dB.");
            }
            if (weak != nullptr && best != nullptr && weak != best)
                findings.add ("Most even at " + juce::String (best->hz) + " Hz ("
                              + num (best->stats.cov6, 1) + " % within 6 dB); least even at "
                              + juce::String (weak->hz) + " Hz (" + num (weak->stats.cov6, 1) + " %).");
            findings.add (juce::String ("Directivity source: ")
                          + (in.result.usedMeasuredDirectivity ? "measured polar data." : "analytical model."));
            for (auto& f : findings) { y = pdf.textWrapped (M + 12, y, cw - 12, 12.0, "-  " + f, sub) + 4; }

            y += 12;
            pdf.text (M, y, 14, "Recommendations", ink, true); y += 22;
            juce::StringArray recs;
            if (! st.audience)
                recs.add ("Draw the audience areas on the plan: coverage is then measured where people "
                          "actually are, at ear height, instead of over the whole field.");
            if (st.valid && st.cov6 < 50.0)
                recs.add ("Under half of the region is within 6 dB - re-aim or add units toward the "
                          "quiet areas visible on the plots.");
            if (st.valid && st.stdDb > 6.0)
                recs.add ("Level spread is " + num (st.stdDb, 1) + " dB - consider splay angles, "
                          "delay or extra fills to even it out.");
            if (weak != nullptr && weak->stats.cov6 < 30.0)
                recs.add ("Check the " + juce::String (weak->hz) + " Hz plot for cancellation lobes; "
                          "spacing or delay between units sets where they fall.");
            recs.add ("Re-run the simulation after any change to device count, position, gain or delay.");
            for (auto& r : recs) { y = pdf.textWrapped (M + 12, y, cw - 12, 12.0, "-  " + r, sub) + 4; }
        }
        chrome ("Summary");
    }
}
