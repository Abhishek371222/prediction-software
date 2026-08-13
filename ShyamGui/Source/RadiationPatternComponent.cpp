#include "RadiationPatternComponent.h"
#include "ColourMaps.h"
#include "BrandTheme.h"
#include "AppSettings.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

RadiationPatternComponent::RadiationPatternComponent()
{
    setOpaque (true);
    setWantsKeyboardFocus (false);
}

// ---------------------------------------------------------------------------
void RadiationPatternComponent::updateData (const SimResult& result, const SimParams& params)
{
    result_ = result;
    params_ = params;
    hasData_ = (result_.width > 0);
    // First load, or still at default Fit View: fill the whole plot pane.
    if (! viewInit_ || std::abs (zoom_ - 1.0f) < 0.02f)
        fitView();
    buildImage();
    repaint();
}

void RadiationPatternComponent::setSpeakers (const std::vector<Speaker>& speakers, int selectedIndex)
{
    speakers_ = speakers;
    selected_ = selectedIndex;
    repaint();
}

void RadiationPatternComponent::setMeasuredData (const MeasuredSet& measured)
{
    measured_ = measured;
    if (params_.viewMode == ViewMode::MeasuredPolar) repaint();
}

void RadiationPatternComponent::setMeasuredFrequency (int hz)
{
    if (measuredHz_ == hz) return;
    measuredHz_ = hz;
    if (params_.viewMode == ViewMode::MeasuredPolar
        || params_.viewMode == ViewMode::Directivity) repaint();
}

void RadiationPatternComponent::setMeasuredDistance (float distanceM)
{
    if (std::abs (measuredDistanceM_ - distanceM) < 1.0e-4f) return;
    measuredDistanceM_ = distanceM;
    if (params_.viewMode == ViewMode::MeasuredPolar
        || params_.viewMode == ViewMode::Directivity) repaint();
}

const MeasuredFreq* RadiationPatternComponent::measuredForHz (int hz) const
{
    for (const auto& mf : measured_.freqs)
        if (mf.hz == hz) return &mf;
    return nullptr;
}

bool RadiationPatternComponent::showingBemHeatmap() const noexcept
{
    return params_.viewMode == ViewMode::MeasuredPolar
        && hasData_
        && result_.usedBemField
        && result_.measuredDirectivityHz == measuredHz_
        && ! result_.splDB.empty();
}

// ---------------------------------------------------------------------------
juce::Rectangle<int> RadiationPatternComponent::plotArea() const
{
    return getLocalBounds().withTrimmedRight (kColourbarW + 8);
}

float RadiationPatternComponent::worldScaleX() const
{
    return baseScaleX_ * zoom_;
}

float RadiationPatternComponent::worldScaleY() const
{
    return baseScaleY_ * zoom_;
}

float RadiationPatternComponent::worldScale() const
{
    return 0.5f * (worldScaleX() + worldScaleY());
}

juce::Point<float> RadiationPatternComponent::worldToScreen (float wx, float wy) const
{
    const auto pb = plotArea();
    return { (float) pb.getX() + origin_.x + wx * worldScaleX(),
             (float) pb.getY() + origin_.y + ((float) result_.worldH - wy) * worldScaleY() };
}

juce::Point<float> RadiationPatternComponent::screenToWorld (float sx, float sy) const
{
    const auto pb = plotArea();
    const float sxScale = worldScaleX();
    const float syScale = worldScaleY();
    return { (sx - (float) pb.getX() - origin_.x) / sxScale,
             (float) result_.worldH - (sy - (float) pb.getY() - origin_.y) / syScale };
}

void RadiationPatternComponent::fitView()
{
    const auto pb = plotArea();
    const double ww = (result_.worldW > 0 ? result_.worldW : params_.worldW);
    const double wh = (result_.worldH > 0 ? result_.worldH : params_.worldH);
    if (pb.getWidth() <= 0 || pb.getHeight() <= 0 || ww <= 0 || wh <= 0) return;

    // Uniform px/m so metre grid cells stay square. Cover the plot (may crop
    // one axis); origin (0, 0) is world bottom-left on screen.
    const float sx = (float) (pb.getWidth()  / ww);
    const float sy = (float) (pb.getHeight() / wh);
    const float s  = juce::jmax (sx, sy);
    baseScaleX_ = s;
    baseScaleY_ = s;
    zoom_       = 1.0f;

    const float worldPxW = (float) ww * s;
    const float worldPxH = (float) wh * s;
    origin_ = { 0.5f * ((float) pb.getWidth()  - worldPxW),
                0.5f * ((float) pb.getHeight() - worldPxH) };
    viewInit_   = true;
    clampViewToField();
}

void RadiationPatternComponent::clampViewToField()
{
    const auto pb = plotArea();
    const double ww = (result_.worldW > 0 ? result_.worldW : params_.worldW);
    const double wh = (result_.worldH > 0 ? result_.worldH : params_.worldH);
    if (pb.getWidth() <= 0 || pb.getHeight() <= 0 || ww <= 0 || wh <= 0) return;

    if (zoom_ < 1.0f)
        zoom_ = 1.0f;

    const float worldPxW = (float) ww * worldScaleX();
    const float worldPxH = (float) wh * worldScaleY();
    const float viewW = (float) pb.getWidth();
    const float viewH = (float) pb.getHeight();

    if (worldPxW >= viewW)
        origin_.x = juce::jlimit (viewW - worldPxW, 0.0f, origin_.x);
    else
        origin_.x = 0.0f;

    if (worldPxH >= viewH)
        origin_.y = juce::jlimit (viewH - worldPxH, 0.0f, origin_.y);
    else
        origin_.y = 0.0f;
}

void RadiationPatternComponent::resetView()
{
    viewInit_ = false;
    fitView();
    repaint();
}

void RadiationPatternComponent::zoomIn()
{
    if (! hasData_ || params_.viewMode == ViewMode::Directivity
                   || (params_.viewMode == ViewMode::MeasuredPolar && ! showingBemHeatmap())) return;
    const auto pb = plotArea();
    const float cx = (float) pb.getCentreX();
    const float cy = (float) pb.getCentreY();
    const float newZoom = juce::jlimit (1.0f, kMaxZoom, zoom_ * 1.2f);
    if (std::abs (newZoom - zoom_) < 1e-6f) return;
    auto worldUnder = screenToWorld (cx, cy);
    zoom_ = newZoom;
    origin_.x = cx - (float) pb.getX() - worldUnder.x * worldScaleX();
    origin_.y = cy - (float) pb.getY() - ((float) result_.worldH - worldUnder.y) * worldScaleY();
    clampViewToField();
    repaint();
}

void RadiationPatternComponent::zoomOut()
{
    if (! hasData_ || params_.viewMode == ViewMode::Directivity
                   || (params_.viewMode == ViewMode::MeasuredPolar && ! showingBemHeatmap())) return;
    const auto pb = plotArea();
    const float cx = (float) pb.getCentreX();
    const float cy = (float) pb.getCentreY();
    const float newZoom = juce::jlimit (1.0f, kMaxZoom, zoom_ / 1.2f);
    if (std::abs (newZoom - zoom_) < 1e-6f) return;
    auto worldUnder = screenToWorld (cx, cy);
    zoom_ = newZoom;
    origin_.x = cx - (float) pb.getX() - worldUnder.x * worldScaleX();
    origin_.y = cy - (float) pb.getY() - ((float) result_.worldH - worldUnder.y) * worldScaleY();
    clampViewToField();
    repaint();
}

void RadiationPatternComponent::resized()
{
    const auto pb = plotArea();
    const double ww = (result_.worldW > 0 ? result_.worldW : params_.worldW);
    const double wh = (result_.worldH > 0 ? result_.worldH : params_.worldH);
    if (pb.getWidth() <= 0 || pb.getHeight() <= 0 || ww <= 0 || wh <= 0) return;

    if (! viewInit_ || std::abs (zoom_ - 1.0f) < 0.02f)
    {
        fitView();
        return;
    }

    const float cx = (float) pb.getCentreX();
    const float cy = (float) pb.getCentreY();
    const auto worldUnder = screenToWorld (cx, cy);
    const float sx = (float) (pb.getWidth()  / ww);
    const float sy = (float) (pb.getHeight() / wh);
    const float s  = juce::jmax (sx, sy);
    baseScaleX_ = s;
    baseScaleY_ = s;
    origin_.x = cx - (float) pb.getX() - worldUnder.x * worldScaleX();
    origin_.y = cy - (float) pb.getY() - ((float) result_.worldH - worldUnder.y) * worldScaleY();
    clampViewToField();
}

// ---------------------------------------------------------------------------
void RadiationPatternComponent::buildImage()
{
    if (! hasData_) return;

    const int W = result_.width;
    const int H = result_.height;
    if (W <= 0 || H <= 0) return;

    fieldImage_ = juce::Image (juce::Image::RGB, W, H, false);
    juce::Image::BitmapData bm (fieldImage_, juce::Image::BitmapData::writeOnly);

    const auto mode = params_.viewMode;

    for (int row = 0; row < H; ++row)
    {
        // Engine row 0 = world Y = 0 (bottom); image row 0 = top (max Y).
        const int imgRow = H - 1 - row;
        for (int col = 0; col < W; ++col)
        {
            const size_t idx = (size_t) row * W + col;
            juce::Colour c;

            // SPL heatmap — 7-color scale (black→blue→cyan→green→yellow→orange→red).
            {
                const float dB = result_.splDB[idx];
                if (params_.bandedSPL)
                {
                    c = ColourMaps::splBand (dB, (float) AcousticEngine::kColourStepDB);
                }
                else
                {
                    const float floor = (float) params_.dBfloor;
                    const float t = (floor < 0.0f) ? (dB - floor) / (0.0f - floor) : 0.0f;
                    c = ColourMaps::sevenColor (juce::jlimit (0.0f, 1.0f, t));
                }
            }

            bm.setPixelColour (col, imgRow, c);
        }
    }
}

// ---------------------------------------------------------------------------
void RadiationPatternComponent::paint (juce::Graphics& g)
{
    // Theme canvas: black in dark mode, warm light in light mode (mockup grid).
    g.fillAll (Brand::plotBg());

    // Measured polar view is independent of the simulation result.
    if (params_.viewMode == ViewMode::MeasuredPolar)
    {
        drawMeasuredPolar (g, getLocalBounds());
        return;
    }

    if (! hasData_)
    {
        g.setColour (Brand::ash());
        g.drawText ("Add a Q21S unit and press RUN", getLocalBounds(),
                    juce::Justification::centred);
        return;
    }

    const auto pb = plotArea();

    if (params_.viewMode == ViewMode::Directivity)
    {
        // Full-area CLIO-style plot (no colour bar).
        drawPolarPlot (g, getLocalBounds());
        return;
    }

    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (pb);
        drawField    (g, pb);
        drawLayout   (g, pb);
        drawGrid     (g, pb);
        drawSpeakers (g, pb);
    }

    drawColourbar (g, getLocalBounds().withTrimmedLeft (pb.getWidth() + 8));
}

// ---------------------------------------------------------------------------
void RadiationPatternComponent::drawField (juce::Graphics& g, juce::Rectangle<int>)
{
    if (! fieldImage_.isValid()) return;

    const auto tl = worldToScreen (0.0f, (float) result_.worldH);   // top-left
    const float w = (float) result_.worldW * worldScaleX();
    const float h = (float) result_.worldH * worldScaleY();

    // Smooth gradient (EASE-style) interpolates; banded contours stay crisp.
    const bool banded = (params_.viewMode == ViewMode::SPL && params_.bandedSPL);
    g.setImageResamplingQuality (banded ? juce::Graphics::lowResamplingQuality
                                        : juce::Graphics::highResamplingQuality);
    g.setOpacity (1.0f);
    g.drawImage (fieldImage_,
                 juce::Rectangle<float> (tl.x, tl.y, w, h),
                 juce::RectanglePlacement::stretchToFit);
}

// Pick a "nice" 1 / 2 / 5 x 10^n step (in metres) closest to the requested
// raw spacing, so grid density stays readable at any zoom level.
double RadiationPatternComponent::niceStep (double raw)
{
    if (raw <= 0.0) return 1.0;
    const double mag  = std::pow (10.0, std::floor (std::log10 (raw)));
    const double norm = raw / mag;                       // 1 .. 10
    const double n = (norm < 1.5) ? 1.0 : (norm < 3.5) ? 2.0 : (norm < 7.5) ? 5.0 : 10.0;
    return n * mag;
}

RadiationPatternComponent::GridMetrics RadiationPatternComponent::currentGridMetrics() const
{
    // Aim for ~20 px minor / ~90 px major so cells stay readable; floor at 1 mm.
    const double pxPerM = (double) worldScaleX();
    GridMetrics m;
    if (pxPerM <= 1.0e-6)
        return m;

    m.minor = niceStep (20.0 / pxPerM);
    if (m.minor < kMinGridM)
        m.minor = kMinGridM;

    m.major = niceStep (90.0 / pxPerM);
    if (m.major < m.minor)
        m.major = m.minor;

    // Keep majors on a clean multiple of minors (5× or 10× preferred).
    const double ratio = m.major / m.minor;
    if (ratio < 2.5)
        m.major = m.minor * 5.0;
    else if (ratio < 7.5)
        m.major = m.minor * 5.0;
    else
        m.major = m.minor * 10.0;

    return m;
}

juce::String RadiationPatternComponent::formatGridLabel (double metres)
{
    const double a = std::abs (metres);
    if (a < 1.0e-12)
        return "0";

    if (a + 1.0e-12 < 0.01)
        return juce::String ((int) std::lround (metres * 1000.0)) + " mm";

    if (a + 1.0e-12 < 1.0)
    {
        const double cm = metres * 100.0;
        if (std::abs (cm - std::round (cm)) < 1.0e-6)
            return juce::String ((int) std::lround (cm)) + " cm";
        return juce::String (metres, 3) + " m";
    }

    if (std::abs (metres - std::round (metres)) < 1.0e-6)
        return juce::String ((int) std::lround (metres)) + " m";

    return juce::String (metres, 2) + " m";
}

void RadiationPatternComponent::drawGrid (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    const float ww = (float) result_.worldW;
    const float wh = (float) result_.worldH;

    if (! showGrid_) return;

    const auto metrics = currentGridMetrics();
    const double minorStep = metrics.minor;
    const double majorStep = metrics.major;
    if (minorStep <= 0.0 || majorStep <= 0.0) return;

    // Only draw lines that intersect the visible plot (needed at 1 mm density).
    const auto tl = screenToWorld ((float) bounds.getX(),      (float) bounds.getY());
    const auto br = screenToWorld ((float) bounds.getRight(),  (float) bounds.getBottom());
    const double visX0 = juce::jlimit (0.0, (double) ww, (double) std::min (tl.x, br.x));
    const double visX1 = juce::jlimit (0.0, (double) ww, (double) std::max (tl.x, br.x));
    const double visY0 = juce::jlimit (0.0, (double) wh, (double) std::min (tl.y, br.y));
    const double visY1 = juce::jlimit (0.0, (double) wh, (double) std::max (tl.y, br.y));

    const double xMin = 0.0;
    const double xMax = (double) ww;
    const double yMin = 0.0;
    const double yMax = (double) wh;

    const juce::Colour minorCol = Brand::plotGrid().withMultipliedAlpha (0.45f);
    const juce::Colour majorCol = Brand::plotGrid();
    const juce::Colour labelInk = Brand::axisLabel();

    auto isMajor = [&] (double v)
    {
        return std::abs (std::remainder (v, majorStep)) < minorStep * 0.25;
    };

    auto forTicks = [] (double lo, double hi, double step, const std::function<void(double)>& fn)
    {
        if (step <= 0.0) return;
        const long i0 = (long) std::ceil  ((lo - 1.0e-12) / step);
        const long i1 = (long) std::floor ((hi + 1.0e-12) / step);
        for (long i = i0; i <= i1; ++i)
            fn ((double) i * step);
    };

    auto vline = [&] (double xm, bool major)
    {
        auto a = worldToScreen ((float) xm, (float) yMin);
        auto b = worldToScreen ((float) xm, (float) yMax);
        g.setColour (major ? majorCol : minorCol);
        g.drawLine (a.x, a.y, b.x, b.y, major ? 1.0f : 0.6f);
    };
    auto hline = [&] (double ym, bool major)
    {
        auto a = worldToScreen ((float) xMin, (float) ym);
        auto b = worldToScreen ((float) xMax, (float) ym);
        g.setColour (major ? majorCol : minorCol);
        g.drawLine (a.x, a.y, b.x, b.y, major ? 1.0f : 0.6f);
    };

    forTicks (visX0, visX1, minorStep, [&] (double x) { if (! isMajor (x)) vline (x, false); });
    forTicks (visY0, visY1, minorStep, [&] (double y) { if (! isMajor (y)) hline (y, false); });
    forTicks (visX0, visX1, majorStep, [&] (double x) { vline (x, true); });
    forTicks (visY0, visY1, majorStep, [&] (double y) { hline (y, true); });

    // Axis labels follow major spacing so zoom reveals 62, 63, 64… then cm/mm.
    g.setFont (Brand::techMed (Brand::Type::gridNum));
    g.setColour (labelInk);
    constexpr int labelW = 56;
    forTicks (visX0, visX1, majorStep, [&] (double x)
    {
        auto a = worldToScreen ((float) x, 0.0f);
        if (a.x < (float) bounds.getX() - 4.0f || a.x > (float) bounds.getRight() + 4.0f)
            return;
        const int chipX = juce::jlimit (bounds.getX() + 2, bounds.getRight() - labelW, (int) a.x + 2);
        const int chipY = bounds.getBottom() - 16;
        g.drawText (formatGridLabel (x),
                    juce::Rectangle<int> (chipX, chipY, labelW, 14),
                    juce::Justification::left);
    });
    forTicks (visY0, visY1, majorStep, [&] (double y)
    {
        auto a = worldToScreen (0.0f, (float) y);
        if (a.y < (float) bounds.getY() - 4.0f || a.y > (float) bounds.getBottom() + 4.0f)
            return;
        const int chipY = juce::jlimit (bounds.getY() + 2, bounds.getBottom() - 16, (int) a.y - 7);
        g.drawText (formatGridLabel (y),
                    juce::Rectangle<int> (bounds.getX() + 2, chipY, labelW, 14),
                    juce::Justification::left);
    });
}

// ---------------------------------------------------------------------------
void RadiationPatternComponent::drawLayout (juce::Graphics& g, juce::Rectangle<int>)
{
    if (layout_ == nullptr || ! layout_->valid() || ! layout_->visible) return;

    const float sx  = worldScaleX();
    const float sy  = worldScaleY();
    const float mpu = layout_->metresPerUnit();
    const float kx  = mpu * sx;
    const float ky  = mpu * sy;
    if (kx <= 0.0f || ky <= 0.0f) return;

    const auto& L = *layout_;
    const float x0 = L.srcBounds.getX(), y0 = L.srcBounds.getY();
    const float h  = L.srcBounds.getHeight();
    const float ox = L.originM.x, oy = L.originM.y;

    const auto pb = plotArea();
    const float bx = (float) pb.getX() + origin_.x + (ox - x0 * mpu) * sx;
    const float by = (float) pb.getY() + origin_.y
                   + ((float) result_.worldH - oy - (y0 + h) * mpu) * sy;

    juce::AffineTransform t (kx, 0.0f, bx, 0.0f, ky, by);
    if (std::abs (L.rotationDeg) > 0.01f)
    {
        auto p0 = worldToScreen (ox, oy);
        t = t.followedBy (juce::AffineTransform::rotation (
                juce::degreesToRadians (L.rotationDeg), p0.x, p0.y));
    }

    juce::Graphics::ScopedSaveState ss (g);
    if (L.kind == LayoutLayer::Kind::Image && L.image.isValid())
    {
        g.setOpacity (L.opacity);
        g.drawImageTransformed (L.image, t, false);
    }
    else if (L.kind == LayoutLayer::Kind::Dxf && ! L.path.isEmpty())
    {
        g.setColour (juce::Colour (0xff39c0ff).withAlpha (L.opacity));
        g.strokePath (L.path, juce::PathStrokeType (1.2f), t);
    }

    // Outline + handle when in edit mode so the user can see/grab it.
    if (layoutEditMode_)
    {
        juce::Path box;
        box.addRectangle (L.srcBounds);
        g.setColour (Brand::accent().withAlpha (0.9f));
        g.strokePath (box, juce::PathStrokeType (1.5f), t);
        auto p0 = worldToScreen (ox, oy);
        g.fillEllipse (p0.x - 5.0f, p0.y - 5.0f, 10.0f, 10.0f);
    }
}

void RadiationPatternComponent::drawSpeakers (juce::Graphics& g, juce::Rectangle<int>)
{
    // Distance reference rings (2 / 4 / 8 m) — toggleable from the plot toolbar.
    if (showDistanceRings_)
    {
        static constexpr float kRingM[] = { 2.0f, 4.0f, 8.0f };
        for (const auto& spk : speakers_)
        {
            if (! spk.enabled) continue;
            const auto c = worldToScreen (spk.x, spk.y);
            for (float rm : kRingM)
            {
                const float rx = rm * worldScaleX();
                const float ry = rm * worldScaleY();
                g.setColour (Brand::white().withAlpha (0.55f));
                g.drawEllipse (c.x - rx, c.y - ry, rx * 2.0f, ry * 2.0f, 1.0f);
                g.setFont (Brand::mono (Brand::Type::gridNum));
                g.setColour (Brand::white().withAlpha (0.75f));
                const juce::String lab = juce::String ((int) rm) + " m";
                g.drawText (lab,
                            (int) (c.x + rx * 0.707f) + 4,
                            (int) (c.y - ry * 0.707f) - 10,
                            36, 14, juce::Justification::centredLeft, false);
            }
        }
    }

    // White tile + black speaker glyph; selected = thick black boundary (visible on heatmap).
    for (int i = 0; i < (int) speakers_.size(); ++i)
    {
        const auto& spk = speakers_[i];
        const auto c = worldToScreen (spk.x, spk.y);
        const bool isSel = (i == selected_);
        const float alpha = spk.enabled ? 1.0f : 0.42f;
        const bool reverse = spk.reverseOrientation;

        const float tile = juce::jlimit (18.0f, 28.0f, 22.0f * Brand::UI::scale);
        juce::Rectangle<float> box (c.x - tile * 0.5f, c.y - tile * 0.5f, tile, tile);

        // White sound-icon tile
        g.setColour (Brand::white().withAlpha (alpha));
        g.fillRoundedRectangle (box, 3.0f);

        if (isSel)
        {
            // Black selection frame — reads clearly on red/blue heatmaps.
            g.setColour (juce::Colours::black.withAlpha (0.95f * alpha));
            g.drawRoundedRectangle (box.expanded (3.0f), 5.0f, 2.6f);
            g.drawRoundedRectangle (box, 3.0f, 2.2f);
        }
        else
        {
            g.setColour (Brand::charcoal().withAlpha (0.22f * alpha));
            g.drawRoundedRectangle (box, 3.0f, 1.0f);
        }

        // Black speaker icon inside the tile (cone facing right + 3 wave arcs)
        {
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (box.toNearestInt());

            if (reverse)
                g.addTransform (juce::AffineTransform (-1.0f, 0.0f, box.getCentreX() * 2.0f,
                                                        0.0f, 1.0f, 0.0f));

            // Inset design space so waves clear the rounded border.
            const float pad = tile * 0.14f;
            const auto inner = box.reduced (pad);
            const float s = inner.getWidth() / 24.0f;
            const float ox = inner.getX();
            const float oy = inner.getY();
            auto P = [&] (float x, float y) { return juce::Point<float> (ox + x * s, oy + y * s); };

            // Classic volume icon: cabinet + cone pointing right (fits in 24×24)
            juce::Path body;
            body.startNewSubPath (P (3.0f, 8.5f));
            body.lineTo (P (8.0f, 8.5f));
            body.lineTo (P (8.0f, 15.5f));
            body.lineTo (P (3.0f, 15.5f));
            body.closeSubPath();
            body.startNewSubPath (P (8.0f, 8.5f));
            body.lineTo (P (13.5f, 5.5f));
            body.lineTo (P (13.5f, 18.5f));
            body.lineTo (P (8.0f, 15.5f));
            body.closeSubPath();

            g.setColour (Brand::charcoal().withAlpha (alpha));
            g.fillPath (body);

            // JUCE arcs: 0 rad = 12 o'clock, clockwise. Right side ≈ π/2.
            // Keep outermost arc radius ≤ ~8.5 so it stays inside the inset box.
            const auto origin = P (13.8f, 12.0f);
            const float a0 = juce::MathConstants<float>::halfPi - 0.65f;
            const float a1 = juce::MathConstants<float>::halfPi + 0.65f;
            g.setColour (Brand::charcoal().withAlpha (alpha));
            for (float r : { 2.6f, 4.4f, 6.2f })
            {
                juce::Path arc;
                const float rr = r * s;
                arc.addCentredArc (origin.x, origin.y, rr, rr, 0.0f, a0, a1, true);
                g.strokePath (arc, juce::PathStrokeType (1.55f * s,
                    juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
        }

        // Subtle inverted-polarity cue (no green/red +/- badge)
        if (spk.polarityInverted)
        {
            g.setColour (Brand::red().withAlpha (0.95f * alpha));
            g.fillEllipse (box.getRight() - 7.0f, box.getY() + 2.0f, 5.0f, 5.0f);
        }

        // Label above — white Montserrat
        g.setColour (Brand::white().withAlpha (alpha));
        g.setFont (Brand::tech (isSel ? Brand::Type::speakerIdSelected
                                      : Brand::Type::speakerId, true));
        g.drawText ("Q21S_" + juce::String (i + 1),
                    (int) (c.x - 40), (int) (box.getY() - 16.0f), 80, 14,
                    juce::Justification::centred);
    }
}

// ---------------------------------------------------------------------------
void RadiationPatternComponent::drawColourbar (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // Layout matches mockup: thin vertical Rel. SPL strip + 0/-6/…/-36 db + caption.
    const int barX = bounds.getX() + 4;
    const int barY = bounds.getY() + 16;
    const int barW = 16;
    const int barH = bounds.getHeight() - 46;
    if (barH < 20) return;

    const auto tickCol = Brand::text();                 // white (dark) / charcoal (light)
    const auto outline = AppSettings::get().isDark()
                             ? juce::Colour (0xffc8c8c8)
                             : Brand::border();
    g.setFont (Brand::techBold (Brand::Type::colourBarTick));

    auto drawTick = [&] (int db, int ty)
    {
        g.setColour (tickCol);
        g.drawText (juce::String (db) + " db",
                    barX + barW + 4, ty - 7, 54, 14,
                    juce::Justification::centredLeft);
    };

    if (params_.viewMode == ViewMode::SPL && params_.bandedSPL)
    {
        const int bands = 7;
        const float bh = (float) barH / (float) bands;
        for (int i = 0; i < bands; ++i)
        {
            g.setColour (ColourMaps::splPalette()[i]);
            g.fillRect ((float) barX, barY + i * bh, (float) barW, bh + 0.5f);
            const int db = (int) std::lround ((float) i * (params_.dBfloor / (bands - 1)));
            drawTick (db, (int) (barY + i * bh));
        }
        g.setColour (outline);
        g.drawRect (barX, barY, barW, (int) (bh * bands), 1);
    }
    else // continuous — same Graph-colors/mockup gradient as field colormap
    {
        for (int yy = 0; yy < barH; ++yy)
        {
            const float t = 1.0f - (float) yy / (float) juce::jmax (1, barH - 1);
            g.setColour (ColourMaps::sevenColor (t));
            g.fillRect (barX, barY + yy, barW, 1);
        }
        g.setColour (outline);
        g.drawRect (barX, barY, barW, barH, 1);

        constexpr int nT = 6; // 0, -6, -12, -18, -24, -30, -36
        for (int i = 0; i <= nT; ++i)
        {
            const float frac = (float) i / (float) nT;
            const int ty = barY + (int) (frac * (barH - 1));
            const int dB = (int) std::lround (frac * params_.dBfloor);
            drawTick (dB, ty);
        }
    }

    g.setColour (tickCol);
    g.setFont (Brand::techBold (Brand::Type::colourBarTitle));
    g.drawText ("Rel. SPL", barX - 2, barY + barH + 6, barW + 72, 14,
                juce::Justification::centredLeft);
}

// ---------------------------------------------------------------------------
// CLIO-style Atomik polar frame (vector only — never pastes graph images).
// Polar frame aligned with SPL heatmap: 0 deg = forward = right (+X),
// 90 deg = up (+Y), angles increase counter-clockwise (math / heatmap sense).
// dB rings 6 / 0 / -6 / -12 / -18 / -24, ATOMIK branding.
// ---------------------------------------------------------------------------
namespace
{
    juce::String utf8Deg()  { return juce::String::fromUTF8 ("\xc2\xb0"); }
    juce::String utf8Dot()  { return juce::String::fromUTF8 (" \xc2\xb7 "); }

    struct ClioFrame
    {
        float cx = 0, cy = 0, radius = 0;
        float dbMin = -24.0f, dbMax = 6.0f;

        float dbToR (float db) const
        {
            return juce::jlimit (0.0f, 1.0f, (db - dbMin) / (dbMax - dbMin)) * radius;
        }

        // 0° right (forward / heatmap +X), 90° top (+Y), CCW.
        juce::Point<float> toXY (float deg, float db) const
        {
            const float a  = deg * (float) M_PI / 180.0f;
            const float rr = dbToR (db);
            return { cx + rr * std::cos (a), cy - rr * std::sin (a) };
        }
    };

    void drawClioChrome (juce::Graphics& g, juce::Rectangle<int> bounds,
                         int hz, const juce::String& subtitle,
                         ClioFrame& out)
    {
        // Light CLIO-like canvas on Atomik dark chrome.
        const juce::Colour paper (0xfff4f5f7);
        const juce::Colour ink   (0xff1a1c20);
        const juce::Colour grid  (0xffb8bec8);
        const juce::Colour axis  (0xff5a6270);
        const juce::Colour brand = Brand::accent();

        g.fillAll (Brand::panelDark());

        auto sheet = bounds.reduced (10, 8);
        g.setColour (paper);
        g.fillRoundedRectangle (sheet.toFloat(), 4.0f);
        g.setColour (Brand::border());
        g.drawRoundedRectangle (sheet.toFloat(), 4.0f, 1.0f);

        // Header: ATOMIK + frequency (left), title (centre).
        g.setColour (ink);
        g.setFont (Brand::tech (Brand::Type::exportBrand, true));
        g.drawText ("ATOMIK", sheet.getX() + 14, sheet.getY() + 8, 120, 18,
                    juce::Justification::centredLeft);

        g.setFont (Brand::tech (Brand::Type::exportFrequency, true));
        g.setColour (brand);
        const juce::String freqLab = juce::String (hz) + " Hz";
        g.drawText (freqLab, sheet.getX() + 14, sheet.getY() + 28, 140, 22,
                    juce::Justification::centredLeft);
        // Underline frequency like CLIO.
        const float fw = g.getCurrentFont().getStringWidthFloat (freqLab);
        g.drawLine ((float) sheet.getX() + 14.0f, (float) sheet.getY() + 50.0f,
                    (float) sheet.getX() + 14.0f + fw, (float) sheet.getY() + 50.0f, 1.5f);

        g.setColour (ink);
        g.setFont (Brand::tech (Brand::Type::exportChartTitle, true));
        g.drawText ("2D Directivity Analysis",
                    sheet.getX(), sheet.getY() + 10, sheet.getWidth(), 22,
                    juce::Justification::centred);

        if (subtitle.isNotEmpty())
        {
            g.setColour (axis);
            g.setFont (Brand::tech (Brand::Type::exportSubtitle));
            g.drawText (subtitle, sheet.getX(), sheet.getY() + 32, sheet.getWidth(), 16,
                        juce::Justification::centred);
        }

        auto plot = sheet.withTrimmedTop (58).withTrimmedBottom (18).reduced (36, 12);
        out.cx = (float) plot.getCentreX();
        out.cy = (float) plot.getCentreY() + 4.0f;
        out.radius = (float) std::min (plot.getWidth(), plot.getHeight()) * 0.44f;
        out.dbMin = -24.0f;
        out.dbMax = 6.0f;

        // Concentric dB rings (CLIO scale).
        g.setFont (Brand::mono (Brand::Type::exportPolarRing));
        for (int db = 6; db >= -24; db -= 6)
        {
            const float rr = out.dbToR ((float) db);
            g.setColour (grid);
            g.drawEllipse (out.cx - rr, out.cy - rr, 2 * rr, 2 * rr,
                           db == 0 ? 1.3f : 0.9f);
            // Labels along the forward (+X / 0°) spoke — right side.
            g.setColour (axis);
            g.drawText (juce::String (db),
                        (int) (out.cx + rr + 4), (int) (out.cy - 7),
                        28, 12, juce::Justification::left);
        }

        // Radial spokes + angle labels every 30 deg (0 right, 90 top).
        for (int deg = 0; deg < 360; deg += 30)
        {
            auto p = out.toXY ((float) deg, out.dbMax);
            g.setColour (grid);
            g.drawLine (out.cx, out.cy, p.x, p.y, 0.9f);

            auto lp = out.toXY ((float) deg, out.dbMax + 3.0f);
            g.setColour (axis);
            g.setFont (Brand::mono (Brand::Type::exportPolarRing));
            juce::String lab;
            if (deg == 0)        lab = "0" + utf8Deg();
            else if (deg == 180) lab = "180" + utf8Deg();
            else if (deg < 180)  lab = juce::String (deg) + utf8Deg();
            else                 lab = juce::String (deg - 360) + utf8Deg();
            g.drawText (lab, (int) lp.x - 18, (int) lp.y - 7, 36, 14,
                        juce::Justification::centred);
        }

        // Centre brand mark (replaces CLIO).
        g.setColour (ink.withAlpha (0.35f));
        g.setFont (Brand::tech (Brand::Type::exportPolarCenter, true));
        g.drawText ("ATOMIK", (int) out.cx - 36, (int) out.cy - 8, 72, 16,
                    juce::Justification::centred);
    }

    // Closed polar stroke in (angle, dB) — dense 0.5° samples, rounded joins.
    // Interpolates in dB (not XY) so lobes stay smooth curves, not hard corners.
    void strokeClioCurve (juce::Graphics& g, const ClioFrame& fr,
                          const std::vector<float>& deg,
                          const std::vector<float>& dbRel,
                          juce::Colour col, float strokeW)
    {
        if (deg.size() < 2 || deg.size() != dbRel.size()) return;

        const int nIn = (int) deg.size();
        auto dbAt = [&] (float a) -> float
        {
            while (a < 0.0f)    a += 360.0f;
            while (a >= 360.0f) a -= 360.0f;
            int i0 = 0;
            for (int i = 1; i < nIn; ++i)
            {
                if (deg[(size_t) i] >= a) { i0 = i - 1; break; }
                i0 = i;
            }
            const int i1 = (i0 + 1) % nIn;
            float a0 = deg[(size_t) i0];
            float a1 = deg[(size_t) i1];
            if (i1 == 0) a1 += 360.0f;
            if (a < a0) a += 360.0f;
            const float t = (a1 - a0) > 1.0e-6f ? (a - a0) / (a1 - a0) : 0.0f;
            const float tt = t * t * (3.0f - 2.0f * t);   // smoothstep
            return dbRel[(size_t) i0] + tt * (dbRel[(size_t) i1] - dbRel[(size_t) i0]);
        };

        constexpr int nOut = 720;
        juce::Path path;
        for (int i = 0; i <= nOut; ++i)
        {
            const float a = (float) i * (360.0f / (float) nOut);
            auto p = fr.toXY (a, juce::jlimit (fr.dbMin, fr.dbMax, dbAt (a)));
            if (i == 0) path.startNewSubPath (p);
            else        path.lineTo (p);
        }
        path.closeSubPath();

        g.setColour (col);
        g.strokePath (path, juce::PathStrokeType (strokeW, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }

    // Dot markers at exact measured angles (data-point traceability).
    void drawMeasuredDots (juce::Graphics& g, const ClioFrame& fr,
                           const std::vector<float>& deg,
                           const std::vector<float>& dbRel,
                           juce::Colour col, float radiusPx = 3.0f)
    {
        if (deg.size() != dbRel.size()) return;
        // Dense 1° sweeps: skip dots (curve already shows every sample).
        if (deg.size() > 90) return;

        g.setColour (col);
        for (size_t i = 0; i < deg.size(); ++i)
        {
            auto p = fr.toXY (deg[i], juce::jlimit (fr.dbMin, fr.dbMax, dbRel[i]));
            g.fillEllipse (p.x - radiusPx, p.y - radiusPx, radiusPx * 2.0f, radiusPx * 2.0f);
        }
    }

    // -6 dB beamwidth ring + label (professional acoustic annotation).
    void drawBeamwidthRing (juce::Graphics& g, const ClioFrame& fr, float beamwidthDeg)
    {
        if (beamwidthDeg < 1.0f || beamwidthDeg > 359.0f) return;

        const float rr = fr.dbToR (-6.0f);
        g.setColour (juce::Colour (0xff5a6270).withAlpha (0.55f));
        // Dashed look: short arcs around the -6 dB ring.
        for (int a = 0; a < 360; a += 8)
        {
            auto p0 = fr.toXY ((float) a,       -6.0f);
            auto p1 = fr.toXY ((float) (a + 4), -6.0f);
            g.drawLine (p0.x, p0.y, p1.x, p1.y, 1.0f);
        }
        juce::ignoreUnused (rr);

        // Arc markers at +/- half beamwidth from on-axis.
        const float half = beamwidthDeg * 0.5f;
        g.setColour (juce::Colour (0xffc45c26));
        auto L = fr.toXY (-half, -6.0f);
        auto R = fr.toXY ( half, -6.0f);
        g.fillEllipse (L.x - 3.5f, L.y - 3.5f, 7.0f, 7.0f);
        g.fillEllipse (R.x - 3.5f, R.y - 3.5f, 7.0f, 7.0f);

        g.setFont (Brand::mono (Brand::Type::exportPolarRing));
        g.setColour (juce::Colour (0xff1a1c20));
        const juce::String lab = "BW " + juce::String (beamwidthDeg, 0)
                                 + juce::String::fromUTF8 ("\xc2\xb0");
        g.drawText (lab, (int) fr.cx - 40, (int) (fr.cy + fr.radius * 0.55f), 80, 14,
                    juce::Justification::centred);
    }
}

// ---------------------------------------------------------------------------
// DIRECTIVITY — CLIO-style Atomik plot of the *unit* measured horizontal
// pattern (from Room / Ground Plane .xlsx). With 2+ enabled subs, the array
// far-field (engine polarMag) is overlaid so add/delete/enable is visible.
// ---------------------------------------------------------------------------
void RadiationPatternComponent::drawPolarPlot (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    int nEnabled = 0;
    for (const auto& s : speakers_) if (s.enabled) ++nEnabled;

    const int hz = (int) (params_.frequency + 0.5);
    const juce::String setName = measured_.sourceName.isNotEmpty()
                                     ? measured_.sourceName : "Measured";

    juce::String sub = setName + utf8Dot() + "Unit directivity";
    if (nEnabled >= 2)
        sub += utf8Dot() + "Array overlay (" + juce::String (nEnabled) + " subs)";
    else if (nEnabled == 1)
        sub += utf8Dot() + "1 sub";
    else
        sub += utf8Dot() + "No active sub";

    ClioFrame fr;
    drawClioChrome (g, bounds, hz, sub, fr);

    std::vector<float> degs, dbs;
    juce::String distLabel = juce::String (measuredDistanceM_, 1) + " m";
    const auto syn = MeasurementData::curveForFrequency (measured_, hz, measuredDistanceM_);
    const MeasuredCurve* primary = syn.ok ? &syn.curve : nullptr;

    // Smooth polar for every catalogue frequency (native xlsx or log-blend).
    if (primary != nullptr)
    {
        MeasurementData::polarStrokeSamples (*primary, degs, dbs);
        const juce::Colour ink (0xff1a1c20);
        strokeClioCurve (g, fr, degs, dbs, ink, 2.6f);
        if (primary->beamwidthDeg > 1.0f)
            drawBeamwidthRing (g, fr, primary->beamwidthDeg);
    }
    else
    {
        g.setColour (juce::Colour (0xff5a6270));
        g.setFont (Brand::tech (Brand::Type::colourBarTick));
        g.drawText ("No measured reading for " + juce::String (hz) + " Hz",
                    bounds.withTrimmedTop (bounds.getHeight() / 2),
                    juce::Justification::centred);
    }

    // Array far-field overlay when 2+ subs are enabled (updates on delete/enable).
    if (nEnabled >= 2 && ! result_.polarMag.empty())
    {
        degs.clear(); dbs.clear();
        const int n = (int) result_.polarMag.size();
        for (int i = 0; i < n; ++i)
        {
            const float mag = juce::jmax (1.0e-6f, result_.polarMag[(size_t) i]);
            degs.push_back ((float) i * 360.0f / (float) n);
            dbs.push_back (20.0f * std::log10 (mag));
        }
        // Dashed-look via thinner accent stroke.
        strokeClioCurve (g, fr, degs, dbs, Brand::accent().withAlpha (0.85f), 1.6f);
    }

    // Speaker markers (live list — reflects delete / enable immediately).
    float mx = 0.0f, my = 0.0f;
    if (nEnabled > 0)
    {
        for (const auto& s : speakers_) if (s.enabled) { mx += s.x; my += s.y; }
        mx /= (float) nEnabled; my /= (float) nEnabled;
        const float sc = fr.radius * 0.08f;
        for (int i = 0; i < (int) speakers_.size(); ++i)
        {
            const auto& s = speakers_[i];
            if (! s.enabled) continue;
            const float px = fr.cx + (s.x - mx) * sc;
            const float py = fr.cy - (s.y - my) * sc;
            juce::Rectangle<float> cab (px - 5.0f, py - 5.0f, 10.0f, 10.0f);
            g.setColour (Brand::accent());
            g.fillRoundedRectangle (cab, 2.0f);
            g.setColour (juce::Colour (0xff1a1c20));
            g.drawRoundedRectangle (cab, 2.0f, (i == selected_) ? 2.0f : 1.0f);
        }
    }

    // Legend
    const int lx = bounds.getX() + 24;
    int ly = bounds.getY() + 58;
    g.setFont (Brand::tech (Brand::Type::axis));
    g.setColour (juce::Colour (0xff1a1c20));
    g.fillRect (lx, ly + 4, 16, 3);
    g.drawText ("Unit @ " + distLabel, lx + 20, ly, 160, 14, juce::Justification::left);
    ly += 15;
    if (primary != nullptr && primary->beamwidthDeg > 1.0f)
    {
        g.setColour (juce::Colour (0xff5a6270));
        g.setFont (Brand::mono (Brand::Type::polarLegendMeta));
        g.drawText ("BW " + juce::String (primary->beamwidthDeg, 0)
                    + juce::String::fromUTF8 ("\xc2\xb0"),
                    lx, ly, 120, 12, juce::Justification::left);
        ly += 14;
    }
    if (nEnabled >= 2)
    {
        g.setColour (Brand::accent().withAlpha (0.85f));
        g.fillRect (lx, ly + 4, 16, 3);
        g.setColour (juce::Colour (0xff1a1c20));
        g.setFont (Brand::tech (Brand::Type::axis));
        g.drawText ("Array (" + juce::String (nEnabled) + " subs)",
                    lx + 20, ly, 140, 14, juce::Justification::left);
    }
}

// ---------------------------------------------------------------------------
// Measured polar — CLIO-style Atomik plot from real MeasurementIntegrationPack
// readings (Room = ShyamGuild, Ground Plane = Factory). Exact CSV points only.
// ---------------------------------------------------------------------------
void RadiationPatternComponent::drawMeasuredPolar (juce::Graphics& g,
                                                   juce::Rectangle<int> bounds)
{
    if (! measured_.ok)
    {
        g.fillAll (Brand::panelDark());
        g.setColour (Brand::ash());
        g.setFont (Brand::tech (Brand::Type::polarEmptyMessage));
        g.drawFittedText ("No measurement data found\n"
                          "Place MeasurementIntegrationPack/Data (or shyamGuildMeasurements)\n"
                          "inside the project folder, then rebuild / re-run.",
                          bounds.reduced (24), juce::Justification::centred, 4);
        return;
    }

    juce::StringArray availHz;
    for (const auto& f : measured_.freqs) if (f.ok) availHz.add (juce::String (f.hz));

    // Exact match preferred; else nearest measured frequency.
    const MeasuredFreq* mf = measuredForHz (measuredHz_);
    int displayHz = measuredHz_;
    if (mf == nullptr)
    {
        int best = 100000;
        for (const auto& f : measured_.freqs)
        {
            if (! f.ok) continue;
            const int d = std::abs (f.hz - measuredHz_);
            if (d < best) { best = d; mf = &f; displayHz = f.hz; }
        }
        if (mf == nullptr) displayHz = measuredHz_;
    }
    else
    {
        displayHz = mf->hz;
    }

    const juce::String setName = measured_.sourceName.isNotEmpty()
                                     ? measured_.sourceName : "Measured";

    juce::String sub = setName + utf8Dot() + "Horizontal"
                       + utf8Dot() + juce::String (measuredDistanceM_, 1) + " m";
    ClioFrame fr;
    drawClioChrome (g, bounds, displayHz, sub, fr);

    if (mf == nullptr || ! mf->ok)
    {
        g.setColour (juce::Colour (0xff5a6270));
        g.setFont (Brand::tech (Brand::Type::colourBarTick));
        g.drawText ("No reading for " + juce::String (measuredHz_)
                    + " Hz  (available: " + availHz.joinIntoString (", ") + " Hz)",
                    bounds.withTrimmedTop (bounds.getHeight() / 2),
                    juce::Justification::centred);
        return;
    }

    std::vector<float> degs, dbs;
    const auto syn = MeasurementData::curveForFrequency (measured_, displayHz, measuredDistanceM_);
    const MeasuredCurve* primary = syn.ok ? &syn.curve : MeasurementData::curveAtDistance (*mf, measuredDistanceM_);
    if (primary != nullptr)
    {
        MeasurementData::polarStrokeSamples (*primary, degs, dbs);
        const juce::Colour ink (0xff1a1c20);
        strokeClioCurve (g, fr, degs, dbs, ink, 2.6f);
        if (primary->beamwidthDeg > 1.0f)
            drawBeamwidthRing (g, fr, primary->beamwidthDeg);
    }

    const int lx = bounds.getX() + 24;
    int ly = bounds.getY() + 58;
    g.setFont (Brand::tech (Brand::Type::polarLegend));
    g.setColour (juce::Colour (0xff1a1c20));
    g.fillRect (lx, ly + 4, 18, 3);
    g.drawText (juce::String (measuredDistanceM_, 1) + " m",
                lx + 24, ly, 80, 14, juce::Justification::left);
    ly += 16;

    if (primary != nullptr && primary->beamwidthDeg > 1.0f)
    {
        g.setColour (juce::Colour (0xff5a6270));
        g.setFont (Brand::mono (Brand::Type::exportPolarRing));
        g.drawText ("BW " + juce::String (primary->beamwidthDeg, 0)
                    + juce::String::fromUTF8 ("\xc2\xb0") + " (-6 dB)",
                    lx, ly, 160, 12, juce::Justification::left);
    }
}

// ---------------------------------------------------------------------------
int RadiationPatternComponent::speakerHitTest (juce::Point<float> p) const
{
    const float tile = juce::jlimit (18.0f, 28.0f, 22.0f * Brand::UI::scale);
    const float half = tile * 0.5f + 6.0f;
    for (int i = (int) speakers_.size() - 1; i >= 0; --i)
    {
        auto c = worldToScreen (speakers_[i].x, speakers_[i].y);
        if (std::abs (p.x - c.x) <= half && std::abs (p.y - c.y) <= half + 8.0f)
            return i;
    }
    return -1;
}

void RadiationPatternComponent::mouseDown (const juce::MouseEvent& e)
{
    if (! hasData_ || params_.viewMode == ViewMode::Directivity
                   || (params_.viewMode == ViewMode::MeasuredPolar && ! showingBemHeatmap())) return;

    lastMouse_ = e.position;
    const int hit = speakerHitTest (e.position);
    if (hit >= 0)
    {
        drag_ = Drag::Speaker;
        draggedSpeaker_ = hit;
        selected_ = hit;
        if (onSpeakerSelected) onSpeakerSelected (hit);
        repaint();
    }
    else if (layoutEditMode_ && layout_ != nullptr && layout_->valid()
             && layout_->visible && ! layout_->locked)
    {
        drag_ = Drag::Layer;   // in edit mode, a non-speaker drag moves the layout
    }
    else
    {
        drag_ = Drag::Pan;
    }
}

void RadiationPatternComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (drag_ == Drag::Pan)
    {
        origin_ += (e.position - lastMouse_);
        lastMouse_ = e.position;
        clampViewToField();
        repaint();
    }
    else if (drag_ == Drag::Layer && layout_ != nullptr)
    {
        const auto d = e.position - lastMouse_;
        lastMouse_ = e.position;
        layout_->originM.x += d.x / worldScaleX();
        layout_->originM.y -= d.y / worldScaleY();     // screen y down -> world y up
        if (layoutSnap_)
        {
            layout_->originM.x = std::round (layout_->originM.x);
            layout_->originM.y = std::round (layout_->originM.y);
        }
        if (onLayoutMoved) onLayoutMoved();
        repaint();
    }
    else if (drag_ == Drag::Speaker && draggedSpeaker_ >= 0)
    {
        auto w = screenToWorld (e.position.x, e.position.y);
        const float nx = juce::jlimit (0.0f, (float) result_.worldW, w.x);
        const float ny = juce::jlimit (0.0f, (float) result_.worldH, w.y);
        if (draggedSpeaker_ < (int) speakers_.size())
        {
            speakers_[(size_t) draggedSpeaker_].x = nx;
            speakers_[(size_t) draggedSpeaker_].y = ny;
        }
        if (onSpeakerMoved) onSpeakerMoved (draggedSpeaker_, nx, ny);
        repaint();
    }
}

void RadiationPatternComponent::mouseUp (const juce::MouseEvent&)
{
    drag_ = Drag::None;
    draggedSpeaker_ = -1;
}

void RadiationPatternComponent::mouseWheelMove (const juce::MouseEvent& e,
                                                const juce::MouseWheelDetails& wheel)
{
    if (! hasData_ || params_.viewMode == ViewMode::Directivity
                   || (params_.viewMode == ViewMode::MeasuredPolar && ! showingBemHeatmap())) return;

    const float factor = (wheel.deltaY > 0 ? 1.1f : 1.0f / 1.1f);
    // Zoom in to inspect sections (down to 1 mm cells); never past full-pane cover.
    const float newZoom = juce::jlimit (1.0f, kMaxZoom, zoom_ * factor);
    if (std::abs (newZoom - zoom_) < 1e-6f) return;

    // Keep the world point under the cursor fixed.
    auto worldUnder = screenToWorld (e.position.x, e.position.y);
    zoom_ = newZoom;
    const auto pb = plotArea();
    origin_.x = e.position.x - (float) pb.getX() - worldUnder.x * worldScaleX();
    origin_.y = e.position.y - (float) pb.getY() - ((float) result_.worldH - worldUnder.y) * worldScaleY();
    clampViewToField();
    repaint();
}

void RadiationPatternComponent::mouseMove (const juce::MouseEvent&) {}
void RadiationPatternComponent::mouseExit (const juce::MouseEvent&) {}
