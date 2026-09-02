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

        const juce::String softwareName = "Atomik Simulation Engine";

        // Clean client report: cover, info, inputs, results, heatmaps, summary.
        // FR / RT60 / AC / TL graph pages removed.
        const int nHeatPages  = (int) in.heatmaps.size();
        const int totalPages  = 1 /*cover*/ + 1 /*info*/ + 1 /*inputs*/ + 1 /*results*/
                              + nHeatPages + 1 /*summary*/;
        int pageNo = 0;

        // Footer only — no running header (cleaner client-facing pages).
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
        pdf.textRight (W - M, 84, 10, "v1.3.7  -  Q21S Coverage & Directivity", juce::Colour (0xff9aa0a8), false);

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
            ty = sectionTitle (ty, "2.1  Speaker Placement (Q21S)");

            // Speaker table
            const double c[] = { M, M + 60, M + 170, M + 280, M + 360, M + 440 };
            pdf.fillRect (M, ty, cw, 20, panel);
            const char* hdr[] = { "Unit", "X (" , "Y (", "Gain", "Delay", "State" };
            pdf.text (c[0] + 6, ty + 5, 9.5, "UNIT", sub, true);
            pdf.text (c[1] + 6, ty + 5, 9.5, "X (" + len + ")", sub, true);
            pdf.text (c[2] + 6, ty + 5, 9.5, "Y (" + len + ")", sub, true);
            pdf.text (c[3] + 6, ty + 5, 9.5, "GAIN", sub, true);
            pdf.text (c[4] + 6, ty + 5, 9.5, "DELAY", sub, true);
            pdf.text (c[5] + 6, ty + 5, 9.5, "STATE", sub, true);
            juce::ignoreUnused (hdr);
            ty += 22;
            int idx = 1;
            for (const auto& s : in.params.speakers)
            {
                pdf.line (M, ty + 18, W - M, ty + 18, lineCol, 0.6);
                pdf.text (c[0] + 6, ty + 4, 10, "Q21S-" + juce::String (idx), ink);
                pdf.text (c[1] + 6, ty + 4, 10, juce::String (Units::metresToDisplay (s.x), 1), ink);
                pdf.text (c[2] + 6, ty + 4, 10, juce::String (Units::metresToDisplay (s.y), 1), ink);
                pdf.text (c[3] + 6, ty + 4, 10, juce::String (s.gainDB, 0) + " dB", ink);
                pdf.text (c[4] + 6, ty + 4, 10, juce::String (s.delayMs, 1) + " ms", ink);
                pdf.text (c[5] + 6, ty + 4, 10, s.enabled ? "Enabled" : "Off", ink);
                ty += 22; ++idx;
                if (ty > H - 80) break;
            }
        }
        chrome ("Input Parameters");

        // =================================================================
        // 4) SIMULATION RESULTS (summary numbers)
        // =================================================================
        pdf.newPage();
        y = sectionTitle (56, "3.  Simulation Results");
        {
            const double cov6  = AcousticAnalysis::coverageWithin (in.result, 6.0);
            const double cov3  = AcousticAnalysis::coverageWithin (in.result, 3.0);
            int nDev = 0; for (auto& s : in.params.speakers) if (s.enabled) ++nDev;

            const double colW = (cw - 30) / 2.0;
            const double x1 = M + colW + 30;
            const double rh = 48;
            double yy = y;
            kv (M, yy, colW, "Active Devices",      juce::String (nDev)); yy += rh;
            kv (M, yy, colW, "Frequency (current)", juce::String ((int) in.params.frequency) + " Hz"); yy += rh;
            kv (M, yy, colW, "Wavelength",          juce::String (Units::metresToDisplay (in.result.lambda), 2) + " " + len); yy += rh;
            kv (M, yy, colW, "Coverage within 3 dB", juce::String (cov3, 1) + " %"); yy += rh;

            yy = y;
            kv (x1, yy, colW, "Coverage within 6 dB", juce::String (cov6, 1) + " %"); yy += rh;
            if (in.result.hasAbsoluteSpl)
                kv (x1, yy, colW, "Peak SPL (heatmap 0 dB)", juce::String (in.result.peakAbsDb, 1) + " dB SPL");
            else
                kv (x1, yy, colW, "Peak SPL (heatmap 0 dB)", "0 dB (relative)");
            yy += rh;
            kv (x1, yy, colW, "Directivity model",    in.result.usedMeasuredDirectivity ? "Measured" : "Model"); yy += rh;
            kv (x1, yy, colW, "Display dynamic range", juce::String ((int) -in.params.dBfloor) + " dB"); yy += rh;
            kv (x1, yy, colW, "Directivity Plot frequencies",  juce::String ((int) in.heatmaps.size())); yy += rh;

            double ny = y + 6 * rh + 10;
            ny = sectionTitle (ny, "3.1  Overview");
            pdf.textWrapped (M, ny, cw, 12.0,
                "The table above summarises predicted SPL coverage for the current array. "
                "Peak SPL is the absolute level at the heatmap's Rel. SPL = 0 dB cell "
                "(loudest point on the map). Coverage is the share of the field within "
                "3 dB / 6 dB of that peak, using the selected measurement distance and "
                "frequency-dependent directivity.", sub, false);
        }
        chrome ("Simulation Results");

        // =================================================================
        // 4) SPL HEATMAPS (one page per frequency)
        // =================================================================
        for (const auto& hmEntry : in.heatmaps)
        {
            pdf.newPage();
            double hy = sectionTitle (56, "4.  Directivity Plot - " + juce::String (hmEntry.hz) + " Hz");
            fitImage (hmEntry.image, M, hy, cw, H - hy - 110);
            const double capY = H - 96;
            pdf.fillRect (M, capY, cw, 44, panel);
            pdf.text (M + 14, capY + 8,  11, "FREQUENCY", sub, true);
            pdf.text (M + 14, capY + 22, 14, juce::String (hmEntry.hz) + " Hz", ink);
            pdf.text (M + 160, capY + 8,  11, "COVERAGE (within 6 dB)", sub, true);
            pdf.text (M + 160, capY + 22, 14, juce::String (hmEntry.coveragePct, 1) + " %", ink);
            if (hmEntry.hasAbsoluteSpl)
            {
                pdf.text (M + 360, capY + 8,  11, "PEAK (heatmap 0 dB)", sub, true);
                pdf.text (M + 360, capY + 22, 14, juce::String (hmEntry.peakAbsDb, 1) + " dB SPL", ink);
                pdf.text (M + 520, capY + 8,  11, "SCALE", sub, true);
                pdf.text (M + 520, capY + 22, 14,
                          juce::String (hmEntry.peakAbsDb, 1) + " to "
                          + juce::String (hmEntry.peakAbsDb + in.params.dBfloor, 1) + " dB", ink);
            }
            else
            {
                pdf.text (M + 360, capY + 8,  11, "PEAK", sub, true);
                pdf.text (M + 360, capY + 22, 14, "0 dB (rel.)", ink);
                pdf.text (M + 480, capY + 8,  11, "SCALE", sub, true);
                pdf.text (M + 480, capY + 22, 14, "0 to " + juce::String ((int) in.params.dBfloor) + " dB", ink);
            }
            chrome ("Directivity Plot");
        }

        // =================================================================
        // 5) SUMMARY
        // =================================================================
        pdf.newPage();
        y = sectionTitle (56, "5.  Summary & Recommendations");
        {
            const double cov6 = AcousticAnalysis::coverageWithin (in.result, 6.0);
            int weakHz = 0; double weakCov = 1e9;
            for (const auto& h : in.heatmaps) if (h.coveragePct < weakCov) { weakCov = h.coveragePct; weakHz = h.hz; }
            int nDev = 0; for (auto& s : in.params.speakers) if (s.enabled) ++nDev;

            pdf.text (M, y, 14, "Key Findings", ink, true); y += 22;
            juce::StringArray findings;
            findings.add ("Array of " + juce::String (nDev) + " Q21S unit(s) simulated over a "
                          + juce::String (Units::metresToDisplay (in.params.worldW), 0) + " x "
                          + juce::String (Units::metresToDisplay (in.params.worldH), 0) + " " + len + " plane.");
            findings.add ("Predicted coverage within 6 dB of peak: " + juce::String (cov6, 1) + " % of the area.");
            if (! in.heatmaps.empty())
                findings.add ("Weakest uniformity occurs around " + juce::String (weakHz)
                              + " Hz (" + juce::String (weakCov, 1) + " % within 6 dB).");
            findings.add (juce::String ("Directivity source: ")
                          + (in.result.usedMeasuredDirectivity ? "measured polar data." : "analytical model."));
            for (auto& f : findings) { y = pdf.textWrapped (M + 12, y, cw - 12, 12.0, "-  " + f, sub) + 4; }

            y += 12;
            pdf.text (M, y, 14, "Recommendations", ink, true); y += 22;
            juce::StringArray recs;
            recs.add ("Fine-tune inter-unit spacing and delay to widen the uniform-coverage zone.");
            recs.add ("Review Directivity Plots at each frequency for cancellation lobes.");
            recs.add ("Consider additional units or re-aiming if coverage within 6 dB falls below target.");
            recs.add ("Re-run the simulation after any change to device count, position, gain or delay.");
            for (auto& r : recs) { y = pdf.textWrapped (M + 12, y, cw - 12, 12.0, "-  " + r, sub) + 4; }
        }
        chrome ("Summary");
    }
}
