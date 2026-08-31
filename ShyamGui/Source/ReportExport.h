#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"
#include "ProjectData.h"
#include "AppSettings.h"
#include "AcousticEngine.h"

// ---------------------------------------------------------------------------
// ReportExport - composes a professional, client-ready heatmap sheet (PNG).
// ---------------------------------------------------------------------------
namespace ReportExport
{
    inline juce::String liveStamp()
    {
        const auto t = juce::Time::getCurrentTime();
        return t.formatted ("%d %b %Y") + "   " + t.formatted ("%H:%M:%S");
    }

    inline juce::String liveDate()
    {
        return juce::Time::getCurrentTime().formatted ("%d %b %Y");
    }

    inline juce::String liveTime()
    {
        return juce::Time::getCurrentTime().formatted ("%H:%M:%S");
    }

    inline juce::Image renderHeatmapSheet (const juce::Image& plotImg,
                                           const ProjectData& proj,
                                           const SimParams&   p,
                                           const SimResult&   r,
                                           const juce::String& reportTitle = "SPL Heatmap")
    {
        const juce::Colour ink     (0xff20242a);
        const juce::Colour subInk  (0xff5b6068);
        const juce::Colour line    (0xffd5d8de);
        const juce::Colour panelBg (0xfff5f6f8);
        const juce::Colour headerBg = Brand::charcoal();

        const int margin   = 44;
        const int headerH  = 104;
        const int metaH    = 116;
        const int footerH  = 104;   // same visual weight as header
        const int gap      = 18;

        const int canvasW  = 1300;
        const int contentW = canvasW - 2 * margin;

        const int pw = juce::jmax (1, plotImg.getWidth());
        const int ph = juce::jmax (1, plotImg.getHeight());
        const int plotW = contentW;
        const int plotH = (int) std::round ((double) plotW * ph / pw);

        const int canvasH = margin + headerH + gap + metaH + gap + plotH + gap + footerH + margin;

        juce::Image img (juce::Image::RGB, canvasW, canvasH, true);
        juce::Graphics g (img);
        g.fillAll (juce::Colours::white);

        int y = margin;

        // ---- Header band --------------------------------------------------
        juce::Rectangle<int> header (margin, y, contentW, headerH);
        g.setColour (headerBg);
        g.fillRoundedRectangle (header.toFloat(), 6.0f);

        if (auto logo = Brand::createLogo (Brand::white()))
        {
            const float logoH = 42.0f;
            const float logoW = logoH * Brand::logoAspect;
            Brand::drawLogo (g, logo.get(),
                             { (float) margin + 22.0f,
                               (float) y + 0.5f * ((float) headerH - logoH),
                               logoW, logoH });
        }

        g.setColour (juce::Colours::white);
        g.setFont (Brand::tech (22.0f, true));
        g.drawText ("ATOMIK ACOUSTIC SIMULATION ENGINE",
                    header.getRight() - 620, y + 22, 600, 28, juce::Justification::centredRight);
        g.setColour (Brand::accent());
        g.setFont (Brand::tech (16.0f, true));
        g.drawText (reportTitle.toUpperCase(),
                    header.getRight() - 620, y + 54, 600, 24, juce::Justification::centredRight);

        y += headerH + gap;

        // ---- Project details block ---------------------------------------
        juce::Rectangle<int> meta (margin, y, contentW, metaH);
        g.setColour (panelBg);
        g.fillRoundedRectangle (meta.toFloat(), 6.0f);
        g.setColour (line);
        g.drawRoundedRectangle (meta.toFloat(), 6.0f, 1.0f);

        auto drawPair = [&] (int cx, int cy, int cw, const juce::String& k, const juce::String& v)
        {
            g.setColour (subInk);
            g.setFont (Brand::tech (12.0f, true));
            g.drawText (k.toUpperCase(), cx, cy, cw, 14, juce::Justification::topLeft);
            g.setColour (ink);
            g.setFont (Brand::tech (16.0f));
            g.drawText (v.isEmpty() ? juce::String ("-") : v, cx, cy + 15, cw, 20, juce::Justification::topLeft);
        };

        const auto& m = proj.meta;
        const int colW = (contentW - 60) / 3;
        const int c0 = margin + 20, c1 = c0 + colW + 10, c2 = c1 + colW + 10;
        int ry = y + 16, rgap = 48;
        drawPair (c0, ry,        colW, "Project",  m.projectName);
        drawPair (c1, ry,        colW, "Engineer", m.engineerName);
        drawPair (c2, ry,        colW, "Owner",    m.ownerName);
        drawPair (c0, ry + rgap, colW, "Date",     liveDate());
        drawPair (c1, ry + rgap, colW, "Time",     liveTime());
        drawPair (c2, ry + rgap, colW, "Location",
                  m.city.isNotEmpty() ? (m.city + (m.country.isNotEmpty() ? ", " + m.country : juce::String()))
                                      : m.country);

        y += metaH + gap;

        // ---- Heatmap ------------------------------------------------------
        juce::Rectangle<int> plotBox (margin, y, plotW, plotH);
        g.setColour (Brand::charcoal());
        g.fillRect (plotBox);
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (plotImg, plotBox.toFloat(), juce::RectanglePlacement::stretchToFit);
        g.setColour (line);
        g.drawRect (plotBox, 1);

        y += plotH + gap;

        // ---- Footer (same weight as header) ------------------------------
        juce::Rectangle<int> footer (margin, y, contentW, footerH);
        g.setColour (headerBg);
        g.fillRoundedRectangle (footer.toFloat(), 6.0f);

        if (auto logo = Brand::createLogo (Brand::white()))
        {
            const float logoH = 36.0f;
            const float logoW = logoH * Brand::logoAspect;
            Brand::drawLogo (g, logo.get(),
                             { (float) margin + 22.0f,
                               (float) y + 0.5f * ((float) footerH - logoH),
                               logoW, logoH });
        }

        int nDev = 0; for (const auto& s : p.speakers) if (s.enabled) ++nDev;
        const juce::String u = Units::lengthUnit();
        juce::String facts = juce::String ((int) p.frequency) + " Hz"
            + "   ·   " + juce::String (nDev) + " Q21S"
            + "   ·   λ " + juce::String (Units::metresToDisplay (r.lambda), 2) + " " + u;
        if (r.hasAbsoluteSpl && r.peakAbsDb > 1.0)
        {
            // Peak = absolute SPL at the heatmap's Rel. SPL = 0 dB cell.
            facts += "   ·   Peak " + juce::String (r.peakAbsDb, 1) + " dB SPL"
                  + " (map 0 dB)"
                  + "   ·   Scale " + juce::String (r.peakAbsDb, 1)
                  + " to " + juce::String (r.peakAbsDb + p.dBfloor, 1) + " dB";
        }
        else
            facts += "   ·   Rel. SPL  0 to " + juce::String ((int) p.dBfloor) + " dB";
        facts += juce::String (r.usedMeasuredDirectivity ? "   ·   Measured" : "   ·   Model");

        g.setColour (juce::Colours::white);
        g.setFont (Brand::tech (22.0f, true));
        g.drawText ("www.atomikaudio.com",
                    footer.getRight() - 620, y + 18, 600, 28, juce::Justification::centredRight);

        g.setColour (Brand::accent());
        g.setFont (Brand::tech (14.0f, true));
        g.drawText (facts,
                    footer.getRight() - 720, y + 50, 700, 20, juce::Justification::centredRight);

        g.setColour (juce::Colours::white.withAlpha (0.80f));
        g.setFont (Brand::tech (12.0f));
        g.drawText ("Generated  " + liveStamp() + "    v1.3.6",
                    footer.getRight() - 620, y + 72, 600, 18, juce::Justification::centredRight);

        return img;
    }
}
