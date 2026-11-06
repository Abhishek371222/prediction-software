#pragma once
#include <JuceHeader.h>
#include "AcousticEngine.h"
#include "MicReceiver.h"
#include "FigmaPlotStyle.h"
#include "ColourMaps.h"
#include <cmath>
#include <vector>

// =============================================================================
// TopViewMapComponent - the Mapping tab's Top View.
//
// A stand-alone, read-only plan of the last solve: the dark grid sheet
// (#222225) with metre labels along the top and down the left, the SPL map of
// every enabled source (subs included) painted over it, the cabinets and mics
// on top, and the banded 0 .. -36 dB "Rel. SPL" bar on the right.
//
// It draws NO venue / listening-plane boxes - only the grid and the field.
// It shares nothing with RadiationPatternComponent except the solve it is
// handed, so it cannot change how the Sources plan behaves.
//
// Mouse: drag to pan, wheel to zoom about the pointer, double-click to refit.
// =============================================================================
class TopViewMapComponent : public juce::Component
{
public:
    TopViewMapComponent()
    {
        setOpaque (true);
    }

    // ---- data in -----------------------------------------------------------
    void setResult (const SimResult& r, const SimParams& p)
    {
        result_ = r;
        floorDb_ = (float) p.dBfloor;
        banded_  = (p.viewMode == ViewMode::SPL && p.bandedSPL);
        hasData_ = (r.width > 0 && r.height > 0 && r.activeSpeakers > 0);
        buildImage();
        repaint();
    }

    void setSpeakers (const std::vector<Speaker>& s)
    {
        speakers_ = s;
        repaint();
    }

    void setMics (const std::vector<MicReceiver>& m)
    {
        mics_ = m;
        repaint();
    }

    // ---- palette (from the Top View reference) -----------------------------
    static juce::Colour sheet()     { return juce::Colour (0xff222225); }   // the grid sheet
    static juce::Colour pane()      { return juce::Colour (0xff1b1b1e); }   // around the sheet
    static juce::Colour rule()      { return juce::Colour (0xff3a3a40); }   // dotted grid rule
    static juce::Colour frameCol()  { return juce::Colour (0xff4a4a50); }
    static juce::Colour label()     { return juce::Colour (0xffe6e6e6); }
    static juce::Colour axisY()     { return juce::Colour (0xffed2227).withAlpha (0.8f); }
    static juce::Colour cabinet()   { return juce::Colour (0xffed2227); }

    /** The original software's Rel. SPL scale (ColourMaps::sevenColor):
        continuous by default, 6 dB contour bands when banded SPL is on -
        exactly as the Sources plan colours its map and its legend. */
    juce::Colour levelColour (float db) const
    {
        if (banded_)
            return ColourMaps::splBandForFloor (db, floorDb_, ColourMaps::kContourStepDB);
        return ColourMaps::sevenColor (ColourMaps::relDbToColourT (db, floorDb_));
    }

    // ---- juce::Component -----------------------------------------------------
    void paint (juce::Graphics& g) override
    {
        g.fillAll (pane());
        const auto f = fieldBox();
        if (f.getWidth() < 40 || f.getHeight() < 30) return;
        if (! viewInit_) fitView();

        g.setColour (sheet());
        g.fillRect (f);

        {
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (f);
            if (hasData_) drawField (g);
            drawGrid (g, f);
            drawSpeakers (g);
            drawMics (g);
        }

        g.setColour (frameCol());
        g.drawRect (f, 1);
        drawLabels (g, f);
        drawColourBar (g, f);
    }

    void resized() override
    {
        if (! userView_) viewInit_ = false;
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        dragStart_ = e.position;
        centreAtDrag_ = centre_;
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (ppm_ <= 0.0f) return;
        const auto d = e.position - dragStart_;
        centre_ = { centreAtDrag_.x - d.x / ppm_, centreAtDrag_.y + d.y / ppm_ };
        userView_ = true;
        repaint();
    }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        if (ppm_ <= 0.0f) return;
        const auto before = toWorld (e.position);
        const float k = std::pow (1.15f, w.deltaY * 4.0f);
        ppm_ = juce::jlimit (1.0f, 2000.0f, ppm_ * k);
        const auto after = toWorld (e.position);
        centre_ += before - after;
        userView_ = true;
        repaint();
    }

    void mouseDoubleClick (const juce::MouseEvent&) override
    {
        userView_ = false;
        viewInit_ = false;
        repaint();
    }

private:
    // ---- geometry ------------------------------------------------------------
    juce::Rectangle<int> fieldBox() const
    {
        using UiConfig::Scale::fig;
        // Room for the distance row above, the height column to the left and
        // the colour bar strip on the right.
        return getLocalBounds().withTrimmedLeft (fig (56.0f)).withTrimmedTop (fig (34.0f))
                               .withTrimmedRight (fig (86.0f)).withTrimmedBottom (fig (12.0f));
    }

    juce::Point<float> toScreen (float wx, float wy) const
    {
        const auto f = fieldBox().toFloat();
        return { f.getCentreX() + (wx - centre_.x) * ppm_,
                 f.getCentreY() - (wy - centre_.y) * ppm_ };
    }

    juce::Point<float> toWorld (juce::Point<float> s) const
    {
        const auto f = fieldBox().toFloat();
        return { centre_.x + (s.x - f.getCentreX()) / ppm_,
                 centre_.y - (s.y - f.getCentreY()) / ppm_ };
    }

    /** The opening view: the origin three cells in from the left, the centre
        line across the middle, at the Figma scale snapped to whole cells. */
    void fitView()
    {
        const auto f = fieldBox();
        if (f.getWidth() <= 0) return;
        ppm_ = FigPlot::snappedPxPerM (f.getWidth());
        const float leftM = (float) FigPlot::openingLeftM();
        centre_ = { leftM + 0.5f * (float) f.getWidth() / ppm_, 0.0f };
        viewInit_ = true;
    }

    // ---- the map -----------------------------------------------------------
    void buildImage()
    {
        image_ = {};
        if (! hasData_) return;
        const int W = result_.width, H = result_.height;
        const auto& src = (result_.splRelDB.size() == (size_t) W * (size_t) H) ? result_.splRelDB
                                                                              : result_.splDB;
        if (src.size() != (size_t) W * (size_t) H) { hasData_ = false; return; }

        // Upsample the dB grid (bilinear, in dB) before colouring, so band
        // edges and gradients come out smooth instead of as solver cells.
        const int up = juce::jlimit (1, 6, 2400 / juce::jmax (W, H));
        const int OW = (W - 1) * up + 1, OH = (H - 1) * up + 1;
        auto at = [&] (int c, int r) -> float
        {
            const float v = src[(size_t) r * (size_t) W + (size_t) c];
            return std::isfinite (v) ? v : -200.0f;
        };

        image_ = juce::Image (juce::Image::ARGB, OW, OH, true);
        juce::Image::BitmapData bm (image_, juce::Image::BitmapData::writeOnly);
        for (int oy = 0; oy < OH; ++oy)
        {
            const int   r0 = juce::jmin (H - 2, oy / up);
            const float fy = (float) (oy - r0 * up) / (float) up;
            const int   r1 = juce::jmin (H - 1, r0 + 1);
            const int   imgRow = OH - 1 - oy;   // engine row 0 = lowest world Y
            for (int ox = 0; ox < OW; ++ox)
            {
                const int   c0 = juce::jmin (W - 2, ox / up);
                const float fx = (float) (ox - c0 * up) / (float) up;
                const int   c1 = juce::jmin (W - 1, c0 + 1);
                const float db = (at (c0, r0) * (1 - fx) + at (c1, r0) * fx) * (1 - fy)
                               + (at (c0, r1) * (1 - fx) + at (c1, r1) * fx) * fy;
                // Below the floor the sheet shows through.
                if (floorDb_ < 0.0f && db <= floorDb_)
                    bm.setPixelColour (ox, imgRow, juce::Colours::transparentBlack);
                else
                    bm.setPixelColour (ox, imgRow, levelColour (db));
            }
        }
    }

    void drawField (juce::Graphics& g)
    {
        if (! image_.isValid()) return;
        const auto tl = toScreen ((float) result_.worldX0,
                                  (float) (result_.worldY0 + result_.worldH));
        const auto br = toScreen ((float) (result_.worldX0 + result_.worldW),
                                  (float) result_.worldY0);
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (image_, juce::Rectangle<float> (tl, br));
    }

    // ---- grid & labels -----------------------------------------------------
    double gridStep() const { return FigPlot::gridStepM (ppm_, UiConfig::Scale::figf (30.0f)); }

    void drawGrid (juce::Graphics& g, juce::Rectangle<int> f)
    {
        if (ppm_ <= 0.0f) return;
        const double step = gridStep();
        const auto a = toWorld (f.getTopLeft().toFloat());
        const auto b = toWorld (f.getBottomRight().toFloat());
        const float dashes[] = { 2.0f, 3.0f };

        g.setColour (rule().withAlpha (hasData_ ? 0.55f : 1.0f));
        FigPlot::forSteps (a.x, b.x, step, [&] (double m)
        {
            const float x = std::round (toScreen ((float) m, 0.0f).x) + 0.5f;
            g.drawDashedLine ({ x, (float) f.getY(), x, (float) f.getBottom() }, dashes, 2, 1.0f);
        });
        FigPlot::forSteps (b.y, a.y, step, [&] (double m)
        {
            const float y = std::round (toScreen (0.0f, (float) m).y) + 0.5f;
            g.drawDashedLine ({ (float) f.getX(), y, (float) f.getRight(), y }, dashes, 2, 1.0f);
        });

        // The centre line, red and dotted, as on the reference.
        const float y0 = toScreen (0.0f, 0.0f).y;
        if (y0 > f.getY() && y0 < f.getBottom())
        {
            g.setColour (axisY());
            g.drawDashedLine ({ (float) f.getX(), y0, (float) f.getRight(), y0 }, dashes, 2, 1.0f);
        }
    }

    void drawLabels (juce::Graphics& g, juce::Rectangle<int> f)
    {
        if (ppm_ <= 0.0f) return;
        using UiConfig::Scale::fig;
        const double step = FigPlot::labelStepM (gridStep(), ppm_, UiConfig::Scale::figf (40.0f));
        const auto font = Brand::figSemi (12.0f);
        g.setFont (font);
        g.setColour (label());
        const int h = (int) std::ceil (font.getHeight());

        const auto a = toWorld (f.getTopLeft().toFloat());
        const auto b = toWorld (f.getBottomRight().toFloat());

        // Along the top. The x origin is named for the y axis, as the
        // reference has it ("0(y)").
        FigPlot::forSteps (a.x, b.x, step, [&] (double m)
        {
            const int x = juce::roundToInt (toScreen ((float) m, 0.0f).x);
            const auto txt = FigPlot::tick (m, step, "y");
            const int tw = font.getStringWidth (txt) + 2;
            const int lx = x - tw / 2;
            if (lx < f.getX() - tw / 2 || lx + tw > getWidth()) return;
            g.drawText (txt, lx, f.getY() - fig (10.0f) - h, tw, h, juce::Justification::centred, false);
        });

        // Down the left.
        FigPlot::forSteps (b.y, a.y, step, [&] (double m)
        {
            const int y = juce::roundToInt (toScreen (0.0f, (float) m).y);
            if (y - h / 2 < f.getY() || y + h / 2 > f.getBottom()) return;
            const auto txt = FigPlot::tick (m, step, "y");
            g.drawText (txt, 0, y - h / 2, f.getX() - fig (10.0f), h,
                        juce::Justification::centredRight, false);
        });
    }

    // ---- sources & mics ----------------------------------------------------
    void drawSpeakers (juce::Graphics& g)
    {
        const float minPx = (float) UiConfig::Scale::fig (8.0f);
        for (const auto& s : speakers_)
        {
            if (! s.enabled) continue;
            const auto cab = cabinetFor (s.model);
            // Plan footprint: depth along the firing direction, width across.
            const float d = juce::jmax (minPx, cab.depthM * ppm_);
            const float w = juce::jmax (minPx, cab.widthM * ppm_);
            const auto c = toScreen (s.x, s.y);
            float deg = s.rotationDeg + (s.reverseOrientation ? 180.0f : 0.0f);

            juce::Path p;
            p.addRectangle (-d * 0.5f, -w * 0.5f, d, w);
            p.applyTransform (juce::AffineTransform::rotation (-juce::degreesToRadians (deg))
                                  .translated (c.x, c.y));
            g.setColour (cabinet());
            g.fillPath (p);
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.strokePath (p, juce::PathStrokeType (1.0f));
        }
    }

    void drawMics (juce::Graphics& g)
    {
        const float r = (float) UiConfig::Scale::fig (5.0f);
        g.setFont (Brand::figSemi (11.0f));
        for (const auto& m : mics_)
        {
            const auto c = toScreen (m.x, m.y);
            g.setColour (juce::Colours::white);
            g.fillEllipse (c.x - r, c.y - r, 2 * r, 2 * r);
            g.setColour (juce::Colours::black);
            g.drawEllipse (c.x - r, c.y - r, 2 * r, 2 * r, 1.0f);
            g.setColour (label());
            g.drawText (micDisplayName (m), (int) (c.x + r + 3), (int) (c.y - 8), 60, 16,
                        juce::Justification::centredLeft, false);
        }
    }

    // ---- the Rel. SPL bar --------------------------------------------------
    void drawColourBar (juce::Graphics& g, juce::Rectangle<int> f)
    {
        using UiConfig::Scale::fig;
        const int barW = fig (18.0f);
        const int barX = f.getRight() + fig (14.0f);
        const int barY = f.getY();
        const int barH = juce::jmax (20, f.getHeight() - fig (22.0f));
        const float floor = floorDb_ < 0.0f ? floorDb_ : -36.0f;
        const auto outline = juce::Colour (0xffc8c8c8);
        const int th = fig (14.0f);

        g.setFont (Brand::figSemi (12.0f));
        auto drawTick = [&] (int db, int ty)
        {
            g.setColour (label());
            g.drawText (juce::String (db) + " db", barX + barW + 4, ty - th / 2, fig (54.0f), th,
                        juce::Justification::centredLeft, false);
        };

        if (banded_)
        {
            const float step = ColourMaps::kContourStepDB;
            const int bands = juce::jmax (1, (int) std::lround (-floor / step) + 1);
            const float bh = (float) barH / (float) bands;
            for (int i = 0; i < bands; ++i)
            {
                const float db = -step * (float) i;
                g.setColour (ColourMaps::splBandForFloor (db, floor, step));
                g.fillRect ((float) barX, barY + i * bh, (float) barW, bh + 0.5f);
                drawTick ((int) std::lround (db), (int) (barY + i * bh));
            }
            g.setColour (outline);
            g.drawRect (barX, barY, barW, (int) (bh * (float) bands + 0.5f), 1);
        }
        else
        {
            const float span = juce::jmax (ColourMaps::kRelSplDesignSpanDB, -floor);
            for (int yy = 0; yy < barH; ++yy)
            {
                const float db = -((float) yy / (float) juce::jmax (1, barH - 1)) * span;
                g.setColour (ColourMaps::sevenColor (ColourMaps::relDbToColourT (db, floor)));
                g.fillRect (barX, barY + yy, barW, 1);
            }
            g.setColour (outline);
            g.drawRect (barX, barY, barW, barH, 1);

            const float step = ColourMaps::kRelSplStepDB;
            const int nTicks = juce::jmax (1, (int) std::lround (-floor / step));
            for (int i = 0; i <= nTicks; ++i)
            {
                const float db = -step * (float) i;
                if (db < floor - 0.01f) break;
                const float frac = span > 1.0e-3f ? -db / span : 0.0f;
                drawTick ((int) std::lround (db), barY + (int) std::lround (frac * (float) (barH - 1)));
            }
        }

        g.setColour (label());
        g.setFont (Brand::figSemi (10.0f));
        g.drawText ("Rel. SPL", barX, barY + barH + fig (4.0f), fig (70.0f), th,
                    juce::Justification::centredLeft, false);
    }

    // ---- state ---------------------------------------------------------------
    SimResult                result_;
    float                    floorDb_ = -36.0f;
    bool                     hasData_ = false;
    bool                     banded_  = false;
    juce::Image              image_;
    std::vector<Speaker>     speakers_;
    std::vector<MicReceiver> mics_;

    float              ppm_ = 24.0f;                // px per metre
    juce::Point<float> centre_ { 18.0f, 0.0f };     // world point at the sheet's centre
    bool               viewInit_ = false, userView_ = false;
    juce::Point<float> dragStart_, centreAtDrag_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TopViewMapComponent)
};
