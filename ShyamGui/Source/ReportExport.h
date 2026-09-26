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
                                           const juce::String& reportTitle = "SPL Gradient Plot")
    {
        const juce::Colour ink     (0xff20242a);
        const juce::Colour subInk  (0xff5b6068);
        const juce::Colour line    (0xffd5d8de);
        const juce::Colour panelBg (0xfff5f6f8);
        const juce::Colour headerBg = Brand::charcoal();

        const int margin   = 44;
        const int headerH  = 104;
        const int metaH    = 116;
        const int metricsH = 112;   // labelled key/value panel, two rows
        const int signOffH = 16;    // plain sign-off line under it
        const int gap      = 18;

        const int canvasW  = 1300;
        const int contentW = canvasW - 2 * margin;

        const int pw = juce::jmax (1, plotImg.getWidth());
        const int ph = juce::jmax (1, plotImg.getHeight());
        const int plotW = contentW;
        const int plotH = (int) std::round ((double) plotW * ph / pw);

        const int canvasH = margin + headerH + gap + metaH + gap + plotH + gap
                          + metricsH + 14 + signOffH + margin;

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

        // Shared by the project block and the metrics panel below, so a label
        // and its value are styled identically wherever they appear.
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

        // ---- Metrics panel ------------------------------------------------
        // Was a charcoal strip carrying a second logo and a run-on line of
        // facts. The logo is already in the header, and a dark band at the
        // foot fought the plot for attention while making the numbers hard to
        // pick out. The metrics now use the same labelled key/value language
        // as the project block above, so every figure has a heading and the
        // eye can find one without reading the whole line.
        juce::Rectangle<int> metrics (margin, y, contentW, metricsH);
        g.setColour (panelBg);
        g.fillRoundedRectangle (metrics.toFloat(), 6.0f);
        g.setColour (line);
        g.drawRoundedRectangle (metrics.toFloat(), 6.0f, 1.0f);

        // Accent rule along the top edge ties the panel to the header band.
        g.setColour (Brand::accent());
        g.fillRoundedRectangle ((float) metrics.getX(), (float) metrics.getY(),
                                (float) metrics.getWidth(), 3.0f, 1.5f);

        int nQ21S = 0, nBEM2inch = 0;
        for (const auto& s : p.speakers)
            if (s.enabled) { if (s.model == 2) ++nBEM2inch; else ++nQ21S; }
        juce::String fleet;
        if (nQ21S > 0)     fleet << nQ21S << " Q21S";
        if (nBEM2inch > 0) fleet << (fleet.isEmpty() ? "" : " + ") << nBEM2inch << " BEM2inch";
        if (fleet.isEmpty()) fleet = "None";

        const juce::String u = Units::lengthUnit();

        // Peak SPL is deliberately not reported. It is only a real level when
        // every active unit's model is absolutely calibrated, and the 2" horn's
        // BEM run is unit-drive normalised, so the figure was not comparable
        // between models -- see MeasurementData::calibrationFor(). Scale is
        // therefore given as the relative range the gradient legend actually
        // shows, which is correct for every model and every mix of them.
        const juce::String scaleVal = "0 to " + juce::String ((int) p.dBfloor) + " dB";

        const int mCols = 4;
        const int mColW = (contentW - 40 - (mCols - 1) * 14) / mCols;
        const int mx0   = margin + 20;
        auto mcol = [&] (int i) { return mx0 + i * (mColW + 14); };
        const int mr0 = y + 18, mrGap = 46;

        drawPair (mcol (0), mr0, mColW, "Frequency",  juce::String ((int) p.frequency) + " Hz");
        drawPair (mcol (1), mr0, mColW, "Wavelength", juce::String (Units::metresToDisplay (r.lambda), 2) + " " + u);
        drawPair (mcol (2), mr0, mColW, "Devices",    fleet);
        drawPair (mcol (3), mr0, mColW, "Directivity", r.usedMeasuredDirectivity ? "Measured" : "Model");

        drawPair (mcol (0), mr0 + mrGap, mColW, "Scale (Rel. SPL)", scaleVal);
        drawPair (mcol (1), mr0 + mrGap, mColW, "Dynamic range",
                  juce::String ((int) -p.dBfloor) + " dB");
        drawPair (mcol (2), mr0 + mrGap, mColW, "Grid",
                  juce::String (p.resolution) + " x " + juce::String (p.resolution));

        y += metricsH + 14;

        // ---- Sign-off line (no band: plain type on the sheet) -------------
        g.setColour (subInk);
        g.setFont (Brand::tech (12.0f, true));
        g.drawText ("www.atomikaudio.com",
                    margin, y, contentW / 2, 16, juce::Justification::centredLeft);
        g.setFont (Brand::tech (12.0f));
        g.drawText ("Generated  " + liveStamp() + "    v1.4.0.6",
                    margin + contentW / 2, y, contentW / 2, 16, juce::Justification::centredRight);

        return img;
    }
}
