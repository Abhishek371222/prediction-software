#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"
#include "UiChrome.h"
#include "ViewHeaderBar.h"
#include "AppSettings.h"

// =============================================================================
// The Venue tab: the room, not the rig.
//
// Sources answers "what is firing and where is it aimed". Venue answers the
// question underneath it - where the audience stands, where the stage is - and
// it is a different drawing with a different left panel, so it is a tab rather
// than another mode of the plan.
//
// GEOMETRY, measured off the reference drawing rather than eyeballed:
//
//   field         #E4E4E4, a light grey sheet - NOT white
//   grid          #D9D9D9, on the 2 m step only; no finer rule between
//   scale         14.5 px per metre at the reference width, i.e. 50 m across
//                 the field, with the labelled +22..-24 range just inside it
//   plan          the SAME scale on both axes, so a square room comes out
//                 square
//   elevation     shares the plan's horizontal mapping exactly - an element's
//                 left edge sits directly above its left end - but its
//                 vertical is exaggerated, 0..16 m filling the pane
//
// The x axis runs the "wrong" way on purpose: the reference counts metres to
// the LEFT of the origin as positive, so a venue laid out stage-left to
// stage-right reads the way it does from behind the desk. Every world->screen
// mapping goes through xToScreen(), so that sign lives in exactly one place.
// =============================================================================

struct VenueElement
{
    // Listening is the audience - the only kind the solver predicts onto.
    // Structure is built scenery (a stage); Object is a prop or a piece of
    // kit standing in the room. Neither of the last two is somewhere people
    // stand, so neither becomes a listening plane.
    enum class Type { Listening, Structure, Object };
    enum class AudHeight { Seated, Standing };

    juce::String name   = "Audience";
    Type         type   = Type::Listening;
    AudHeight    height = AudHeight::Seated;

    /** Ear height above the plane, metres: seated 3.5 ft, standing 5 ft. */
    static float earHeightM (AudHeight h) noexcept
    {
        return h == AudHeight::Standing ? 5.0f * 0.3048f : 3.5f * 0.3048f;
    }
    /** Only a seating (listening) plane has an audience height. */
    bool hasAudHeight() const noexcept { return type == Type::Listening; }

    // Four corners, metres. Kept as four free points rather than a rect so the
    // panel's POINT 1..4 fields each have something real to edit - a venue
    // outline is not always square, and the reference tool lets you move one
    // corner on its own.
    struct P { float x = 0.0f, y = 0.0f, z = 0.0f; };
    P pts[4];

    // A box of w x d metres about the origin, lying at height z. Everything
    // added lands here; it is dragged or typed somewhere else afterwards.
    static VenueElement centred (const juce::String& nm, Type t,
                                 float w, float d, float z)
    {
        VenueElement e;
        e.name = nm;
        e.type = t;
        const float hx = w * 0.5f, hy = d * 0.5f;
        e.pts[0] = { -hx,  hy, z };
        e.pts[1] = {  hx,  hy, z };
        e.pts[2] = {  hx, -hy, z };
        e.pts[3] = { -hx, -hy, z };
        return e;
    }

    // ---- persistence -------------------------------------------------
    // Written as a flat object per element rather than leaning on the
    // annotation format: a venue element carries a type, an ear height and
    // four 3-D points, none of which a plan annotation has.
    juce::var toVar() const
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("name",   name);
        o->setProperty ("type",   type == Type::Structure ? "structure"
                                : type == Type::Object    ? "object" : "listening");
        o->setProperty ("height", height == AudHeight::Standing ? "standing" : "seated");
        juce::Array<juce::var> pa;
        for (const auto& p : pts) { pa.add (p.x); pa.add (p.y); pa.add (p.z); }
        o->setProperty ("pts", pa);
        return juce::var (o);
    }

    static VenueElement fromVar (const juce::var& v)
    {
        VenueElement e;
        if (auto* o = v.getDynamicObject())
        {
            e.name   = o->getProperty ("name").toString();
            const auto t = o->getProperty ("type").toString();
            e.type = t == "structure" ? Type::Structure
                   : t == "object"    ? Type::Object
                                      : Type::Listening;
            e.height = o->getProperty ("height").toString() == "standing"
                           ? AudHeight::Standing : AudHeight::Seated;
            if (auto* pa = o->getProperty ("pts").getArray())
                for (int i = 0; i < 4 && i * 3 + 2 < pa->size(); ++i)
                {
                    e.pts[i].x = (float) (double) (*pa)[i * 3];
                    e.pts[i].y = (float) (double) (*pa)[i * 3 + 1];
                    e.pts[i].z = (float) (double) (*pa)[i * 3 + 2];
                }
        }
        if (e.name.isEmpty()) e.name = "Element";
        return e;
    }

    static juce::var listToVar (const std::vector<VenueElement>& v)
    {
        juce::Array<juce::var> a;
        for (const auto& e : v) a.add (e.toVar());
        return juce::var (a);
    }

    static std::vector<VenueElement> listFromVar (const juce::var& v)
    {
        std::vector<VenueElement> out;
        if (auto* a = v.getArray())
            for (const auto& e : *a) out.push_back (fromVar (e));
        return out;
    }

    juce::Colour colour() const
    {
        switch (type)
        {
            case Type::Listening: return juce::Colour (0xffe8a0a4);   // audience, pink
            case Type::Structure: return juce::Colour (0xffa9c4e8);   // scenery, blue
            case Type::Object:    return juce::Colour (0xffe8d2a0);   // prop, amber
        }
        return juce::Colour (0xffe8a0a4);
    }
};

// -----------------------------------------------------------------------------
// Shared look for both venue panes.
// -----------------------------------------------------------------------------
// What part of the world the venue panes are looking at.
//
// ONE of these is shared by the plan and the elevation, which is how ArrayCalc
// behaves: zooming or panning the plan moves the profile under it by the same
// amount, so a point stays in the same column in both. Only the horizontal is
// shared - the elevation's vertical is its own exaggerated scale.
//
// The defaults are exactly the numbers the panes were drawn with before any of
// this existed, so an untouched view looks the way it always did.
struct VenueCamera
{
    double xCentre = -1.0;    // metres; midpoint of the labelled +22..-24
    double yCentre =  0.0;    // metres; the plan's vertical centre
    double spanM   = 50.0;    // metres across the field

    static constexpr double kMinSpanM =   4.0;
    static constexpr double kMaxSpanM = 400.0;

    void zoomBy (double factor, double aboutX, double aboutY)
    {
        const double want = juce::jlimit (kMinSpanM, kMaxSpanM, spanM * factor);
        const double k    = want / spanM;
        // Keep the world point under the cursor pinned where it is: the point
        // you zoom toward should not slide out from under the pointer.
        xCentre = aboutX + (xCentre - aboutX) * k;
        yCentre = aboutY + (yCentre - aboutY) * k;
        spanM   = want;
    }

    void reset() { xCentre = -1.0; yCentre = 0.0; spanM = 50.0; }
};

class VenuePlotBase : public juce::Component
{
public:
    void setElements (const std::vector<VenueElement>* e, int sel)
    {
        elems_ = e; sel_ = sel; repaint();
    }

    void setCamera (VenueCamera* c) { cam_ = c; repaint(); }

    /** Someone moved or reshaped an element on the canvas. */
    std::function<void (int index, VenueElement)> onElementChanged;
    /** Someone clicked an element, or clicked empty space (-1). */
    std::function<void (int index)>               onSelect;
    /** The camera moved, so the other pane has to redraw too. */
    std::function<void()>                         onCameraMoved;
    /** Delete / Duplicate / Mirror x / Mirror y, which ArrayCalc keeps in a
        Tools block beside the drawing. There is no room for that block in
        this design, so they are reached by right-clicking the shape they act
        on - no new chrome, and the target is never ambiguous. */
    std::function<void (int index, int action)>   onAction;
    enum Action { ActDelete = 1, ActDuplicate, ActMirrorX, ActMirrorY };

protected:
    const std::vector<VenueElement>* elems_ = nullptr;
    int                              sel_   = -1;

    static juce::Colour fieldCol()  { return juce::Colour (0xffe4e4e4); }
    static juce::Colour gridCol()   { return juce::Colour (0xffd9d9d9); }
    static juce::Colour frameCol()  { return juce::Colour (0xff000000); }
    static juce::Colour groundCol() { return juce::Colour (0xff3e3e3e); }
    static juce::Colour tickCol()   { return juce::Colour (0xff000000); }
    static juce::Colour markCol()   { return juce::Colour (0xffe02020); }

    // Labelled extent. The field shows a little more than this on each side,
    // which is why the outermost label sits inside the frame rather than on it.
    static constexpr double kXLabelMax =  22.0;
    static constexpr double kXLabelMin = -24.0;
    static constexpr double kXMid      =  -1.0;   // midpoint of the two
    static constexpr double kSpanM     =  50.0;   // metres across the field
    // Grid and label step, in METRES. In Imperial it is five feet, so the
    // rules land on whole feet and the numbers beside them are round - a
    // 2 m grid labelled in feet reads 6.6 ft, 13.1 ft, 19.7 ft.
    static double stepM()
    {
        return Units::imperial() ? 5.0 * 0.3048 : 2.0;
    }

    // The field's inset from the pane, in Figma px: room for the tick labels
    // along the top and down the left. Each pane sets its own.
    struct Margins { float l, t, r, b; };
    Margins margins_ { 59.0f, 54.0f, 39.0f, 26.0f };

    juce::Rectangle<int> fieldArea() const
    {
        using UiConfig::Scale::fig;
        return getLocalBounds().withTrimmedLeft   (fig (margins_.l))
                               .withTrimmedTop    (fig (margins_.t))
                               .withTrimmedRight  (fig (margins_.r))
                               .withTrimmedBottom (fig (margins_.b));
    }

    // The smallest multiple of the grid step that leaves wantPx between one
    // label and the next. The grid itself stays on stepM() - a dense grid with
    // sparse numbers is how a drawing is normally dimensioned.
    static double labelStep (double pxPerM, double wantPx)
    {
        for (double mult : { 1.0, 2.0, 2.5, 5.0, 10.0 })
            if (stepM() * mult * pxPerM >= wantPx)
                return stepM() * mult;
        return stepM() * 10.0;
    }

    VenueCamera* cam_ = nullptr;

    double camSpan()  const { return cam_ != nullptr ? cam_->spanM   : kSpanM; }
    double camXMid()  const { return cam_ != nullptr ? cam_->xCentre : kXMid;  }
    double camYMid()  const { return cam_ != nullptr ? cam_->yCentre : 0.0;    }

    double pxPerMetre (juce::Rectangle<int> f) const
    {
        return juce::jmax (1.0, (double) f.getWidth()) / juce::jmax (1.0e-6, camSpan());
    }

    // Positive metres run RIGHT, negative LEFT - the way a drawing is read
    // and the way ArrayCalc has it. Shared by both panes, so the plan and the
    // profile always agree on which side of the room you are looking at.
    float xToScreen (double m, juce::Rectangle<int> f) const
    {
        return (float) (f.getCentreX() + (m - camXMid()) * pxPerMetre (f));
    }
    // Plan: the same scale as the horizontal, so the drawing stays true.
    float yToScreen (double m, juce::Rectangle<int> f) const
    {
        return (float) (f.getCentreY() - (m - camYMid()) * pxPerMetre (f));
    }

    // ...and back again, which is what every mouse gesture needs.
    double screenToX (float px, juce::Rectangle<int> f) const
    {
        return camXMid() + ((double) px - f.getCentreX()) / pxPerMetre (f);
    }
    double screenToY (float py, juce::Rectangle<int> f) const
    {
        return camYMid() - ((double) py - f.getCentreY()) / pxPerMetre (f);
    }
    // Elevation: 0..zTop fills the pane. Deliberately exaggerated - a 16 m
    // trim across a 50 m room would otherwise be a few pixels tall.
    static float zToScreen (double m, juce::Rectangle<int> f, double zTop)
    {
        return (float) (f.getBottom() - (m / juce::jmax (1.0e-9, zTop)) * f.getHeight());
    }

    // Labelled in whatever unit the app is set to, not always metres.
    // Zero names its axis: "0(x)" along the top, "0(y)" down the side.
    static juce::String tick (double m, const char* axis = "y")
    {
        if (std::abs (m) < 1.0e-9) return juce::String ("0(") + axis + ")";
        return juce::String ((int) std::lround (Units::metresToDisplay (m)))
             + Units::lengthUnit();
    }

    void drawField (juce::Graphics& g, juce::Rectangle<int> f) const
    {
        g.setColour (fieldCol());
        g.fillRect (f);
    }

    void drawFrame (juce::Graphics& g, juce::Rectangle<int> f) const
    {
        g.setColour (frameCol());
        g.drawRect (f, 1);
    }

    // Vertical rules, on the shared horizontal mapping.
    // What the field actually shows now, which is the labelled range only
    // until the camera is moved.
    // With x running right, the LEFT edge is now the smaller metre.
    double visXMin (juce::Rectangle<int> f) const { return screenToX ((float) f.getX(),     f); }
    double visXMax (juce::Rectangle<int> f) const { return screenToX ((float) f.getRight(), f); }
    // Screen y grows downward, so the BOTTOM edge is the smallest metre.
    double visYMin (juce::Rectangle<int> f) const { return screenToY ((float) f.getBottom(), f); }
    double visYMax (juce::Rectangle<int> f) const { return screenToY ((float) f.getY(),      f); }

    void drawXGrid (juce::Graphics& g, juce::Rectangle<int> f) const
    {
        g.setColour (gridCol());
        // Snapped to the step, so an Imperial grid lands on whole feet rather
        // than wherever -24 m happens to fall between them.
        const double lo = visXMin (f), hi = visXMax (f);
        for (double m = std::ceil (lo / stepM()) * stepM();
             m <= hi + 1.0e-6; m += stepM())
        {
            const int x = juce::roundToInt (xToScreen (m, f));
            if (x > f.getX() && x < f.getRight())
                g.drawVerticalLine (x, (float) f.getY(), (float) f.getBottom());
        }
    }

    // Figma: SemiBold, the text box's top `figTop` px down the pane, centred
    // on its rule.
    void drawXLabels (juce::Graphics& g, juce::Rectangle<int> f, float figPx, float figTop) const
    {
        using UiConfig::Scale::fig;
        const auto font = Brand::figSemi (figPx);
        g.setFont (font);
        g.setColour (tickCol());
        const int h  = (int) std::ceil (font.getHeight());
        const int w  = fig (60.0f);
        const double st = labelStep (pxPerMetre (f), fig (40.0f));
        const double lo = visXMin (f), hi = visXMax (f);
        for (double m = std::ceil (lo / st) * st; m <= hi + 1.0e-6; m += st)
        {
            const int x = juce::roundToInt (xToScreen (m, f));
            if (x <= f.getX() || x >= f.getRight()) continue;

            // Measured, not boxed. The guard above only asks where the RULE
            // is, so a rule just inside the frame drew its number half
            // outside it - the row of figures ran wider than the drawing it
            // belongs to. A label is skipped unless the text itself fits.
            const auto txt = tick (m, "x");
            const int  tw  = font.getStringWidth (txt);
            if (x - tw / 2 < f.getX() || x + tw / 2 > f.getRight()) continue;

            g.drawText (txt, x - w / 2, fig (figTop), w, h,
                        juce::Justification::centred, false);
        }
    }

    // Figma: SemiBold, left-aligned at `figLeft`, centred on the rule at y.
    // A figure too wide for the column moves left rather than losing letters.
    // `atEdges` lets the figures on the frame's top and bottom rules show
    // (the elevation's 0 and its top); otherwise they are left out.
    void drawYLabel (juce::Graphics& g, juce::Rectangle<int> f, int y,
                     const juce::String& s, float figLeft, bool atEdges = false) const
    {
        const auto& font = g.getCurrentFont();
        const int h = (int) std::ceil (font.getHeight());
        // Vertically the same rule as the top row: a figure whose line sits
        // within a half-line of the frame would be drawn half outside it, so
        // it is left out rather than clipped in half.
        if (atEdges ? (y - h / 2 < 0 || y + h / 2 > getHeight())
                    : (y - h / 2 < f.getY() || y + h / 2 > f.getBottom())) return;
        const int right = f.getX() - 2;
        const int x = juce::jmax (0, juce::jmin (UiConfig::Scale::fig (figLeft),
                                                 right - 2 - font.getStringWidth (s)));
        g.drawText (s, x, y - h / 2, juce::jmax (1, right - x), h,
                    juce::Justification::centredLeft, false);
    }

    // ---- zoom and pan -----------------------------------------------
    // The wheel zooms about the pointer and a drag on empty canvas pans, which
    // is what ArrayCalc's Zoom in / Zoom out buttons and scrollbars do - the
    // same two gestures, reached without leaving the drawing.
    static constexpr double kZoomPerNotch = 1.15;

    void zoomAt (juce::Point<float> p, double factor)
    {
        if (cam_ == nullptr) return;
        const auto f = fieldArea();
        cam_->zoomBy (factor, screenToX (p.x, f), screenToY (p.y, f));
        if (onCameraMoved) onCameraMoved();
        repaint();
    }

    void panBy (juce::Point<float> deltaPx)
    {
        if (cam_ == nullptr) return;
        const auto f = fieldArea();
        const double perM = pxPerMetre (f);
        // Dragging the drawing right moves the camera LEFT, which with x
        // running right means a smaller centre.
        cam_->xCentre -= (double) deltaPx.x / perM;
        cam_->yCentre -= (double) deltaPx.y / perM;
        if (onCameraMoved) onCameraMoved();
        repaint();
    }

    void mouseWheelMove (const juce::MouseEvent& e,
                         const juce::MouseWheelDetails& w) override
    {
        if (std::abs (w.deltaY) < 1.0e-6) return;
        zoomAt (e.position, w.deltaY > 0 ? 1.0 / kZoomPerNotch : kZoomPerNotch);
    }

    void drawMark (juce::Graphics& g, float x, float y, bool big) const
    {
        const float r = (float) UiConfig::Scale::px (big ? 5 : 4);
        g.setColour (markCol());
        g.fillRect (x - r * 0.5f, y - r * 0.5f, r, r);
    }
};

// -----------------------------------------------------------------------------
// Plan.
// -----------------------------------------------------------------------------
class VenueTopPlot : public VenuePlotBase
{
public:
    // ---- picking ------------------------------------------------------
    // A handle first, then a body, then nothing. Handles win because they sit
    // ON the outline they belong to, so a body test would always swallow them.
    enum class Grab { None, Pan, Body, Corner, Edge };

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) { showContextMenu (e); return; }

        grab_ = Grab::None;
        grabIdx_ = -1;
        const auto f = fieldArea();
        dragFromX_ = screenToX (e.position.x, f);
        dragFromY_ = screenToY (e.position.y, f);
        lastPx_    = e.position;

        if (elems_ == nullptr) { grab_ = Grab::Pan; return; }

        // Handles belong to the SELECTED element only - that is what the
        // drawing shows, and picking a handle on a shape you have not chosen
        // would move something you cannot see the numbers for.
        if (sel_ >= 0 && sel_ < (int) elems_->size())
        {
            const auto& el = (*elems_)[(size_t) sel_];
            const float r = (float) UiConfig::Scale::px (6);
            for (int i = 0; i < 4; ++i)
            {
                const juce::Point<float> c { xToScreen (el.pts[i].x, f),
                                             yToScreen (el.pts[i].y, f) };
                if (c.getDistanceFrom (e.position) <= r)
                { grab_ = Grab::Corner; grabIdx_ = i; start_ = el; return; }

                const auto& n = el.pts[(i + 1) % 4];
                const juce::Point<float> mid { (c.x + xToScreen (n.x, f)) * 0.5f,
                                               (c.y + yToScreen (n.y, f)) * 0.5f };
                if (mid.getDistanceFrom (e.position) <= r)
                { grab_ = Grab::Edge; grabIdx_ = i; start_ = el; return; }
            }
        }

        // Topmost first, so a shape drawn over another takes the click.
        for (int i = (int) elems_->size() - 1; i >= 0; --i)
            if (hits ((*elems_)[(size_t) i], e.position, f))
            {
                if (i != sel_ && onSelect) onSelect (i);
                grab_ = Grab::Body;
                grabIdx_ = i;
                start_ = (*elems_)[(size_t) i];
                return;
            }

        if (onSelect) onSelect (-1);
        grab_ = Grab::Pan;
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        const auto f = fieldArea();

        if (grab_ == Grab::Pan)
        {
            panBy (e.position - lastPx_);
            lastPx_ = e.position;
            return;
        }
        if (grabIdx_ < 0 || onElementChanged == nullptr) return;

        const double dx = screenToX (e.position.x, f) - dragFromX_;
        const double dy = screenToY (e.position.y, f) - dragFromY_;

        VenueElement out = start_;
        if (grab_ == Grab::Body)
        {
            for (auto& p : out.pts) { p.x += (float) dx; p.y += (float) dy; }
        }
        else if (grab_ == Grab::Corner)
        {
            // The opposite corner stays put and the shape is rebuilt as an
            // upright rectangle between it and the pointer - moving one point
            // alone sheared it into a parallelogram.
            const int i = grabIdx_, o = (i + 2) % 4;
            const int nA = (i + 1) % 4, nB = (i + 3) % 4;
            const float cx = start_.pts[i].x + (float) dx;
            const float cy = start_.pts[i].y + (float) dy;
            const float ox = start_.pts[o].x, oy = start_.pts[o].y;
            const auto& c0 = start_.pts[i];
            const auto& a0 = start_.pts[nA];
            // The neighbour that sat above/below the corner keeps its x.
            const bool aSharesX = std::abs (a0.x - c0.x) < std::abs (a0.y - c0.y);
            out.pts[i].x = cx; out.pts[i].y = cy;
            out.pts[o].x = ox; out.pts[o].y = oy;
            out.pts[nA].x = aSharesX ? cx : ox; out.pts[nA].y = aSharesX ? oy : cy;
            out.pts[nB].x = aSharesX ? ox : cx; out.pts[nB].y = aSharesX ? cy : oy;
        }
        else if (grab_ == Grab::Edge)
        {
            // An edge moves straight out or in, carrying both of its ends: a
            // side edge only in x, a top/bottom edge only in y. That resizes
            // the rectangle instead of slanting it.
            const int a = grabIdx_, b = (grabIdx_ + 1) % 4;
            const auto& a0 = start_.pts[a];
            const auto& b0 = start_.pts[b];
            if (std::abs (b0.x - a0.x) >= std::abs (b0.y - a0.y))
            {
                const float y = (a0.y + b0.y) * 0.5f + (float) dy;
                out.pts[a].y = y; out.pts[b].y = y;
            }
            else
            {
                const float x = (a0.x + b0.x) * 0.5f + (float) dx;
                out.pts[a].x = x; out.pts[b].x = x;
            }
        }
        else return;

        onElementChanged (grab_ == Grab::Body ? grabIdx_ : sel_, out);
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        grab_ = Grab::None;
        grabIdx_ = -1;
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (Brand::panel());
        const auto f = fieldArea();
        if (f.getWidth() < 40 || f.getHeight() < 30) return;

        drawField (g, f);

        // Horizontal rules across WHAT IS VISIBLE. This used to run from
        // -halfM to +halfM about the origin, which is only the same thing
        // while the camera sits on it: pan up and the rules stayed behind
        // with zero, leaving the top of the field blank and unlabelled.
        g.setColour (gridCol());
        const double yLo = visYMin (f), yHi = visYMax (f);
        for (double m = std::ceil (yLo / stepM()) * stepM(); m <= yHi + 1.0e-6; m += stepM())
        {
            const int y = juce::roundToInt (yToScreen (m, f));
            if (y >= f.getY() && y <= f.getBottom())
                g.drawHorizontalLine (y, (float) f.getX(), (float) f.getRight());
        }
        drawXGrid (g, f);
        drawFrame (g, f);

        // Figma: SemiBold 12 both ways - across the top 26 down, down the
        // left 24 in.
        drawXLabels (g, f, 12.0f, 26.0f);
        g.setFont (Brand::figSemi (12.0f));
        g.setColour (tickCol());
        {
            const double st = labelStep (pxPerMetre (f), UiConfig::Scale::fig (40.0f));
            for (double m = std::ceil (yLo / st) * st; m <= yHi + 1.0e-6; m += st)
            {
                const int y = juce::roundToInt (yToScreen (m, f));
                if (y <= f.getY() + 4 || y >= f.getBottom() - 4) continue;
                drawYLabel (g, f, y, tick (m), 24.0f);
            }
        }

        // Clipped to the field. An element dragged past the edge used to keep
        // painting over the tick labels and the pane around them, so a plane
        // half out of the drawing looked like a bug in the frame.
        {
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (f);

            if (elems_ != nullptr)
                for (int i = 0; i < (int) elems_->size(); ++i)
                    drawElement (g, f, (*elems_)[(size_t) i], i == sel_);

            drawMark (g, xToScreen (0.0, f), yToScreen (0.0, f), false);
        }
    }

private:
    void showContextMenu (const juce::MouseEvent& e)
    {
        if (elems_ == nullptr || onAction == nullptr) return;
        const auto f = fieldArea();

        int idx = -1;
        for (int i = (int) elems_->size() - 1; i >= 0; --i)
            if (hits ((*elems_)[(size_t) i], e.position, f)) { idx = i; break; }
        if (idx < 0) return;                 // nothing under the pointer

        if (idx != sel_ && onSelect) onSelect (idx);

        juce::PopupMenu m;
        m.addSectionHeader ((*elems_)[(size_t) idx].name);
        m.addItem (ActDuplicate, "Duplicate");
        m.addItem (ActMirrorX,   "Mirror x");
        m.addItem (ActMirrorY,   "Mirror y");
        m.addSeparator();
        m.addItem (ActDelete,    "Delete");

        // At the pointer, not at the component. withTargetComponent anchors
        // the menu to the whole plot, which put it in a corner nowhere near
        // the shape it was acting on.
        auto cb = onAction;
        const juce::Rectangle<int> at (e.getScreenX(), e.getScreenY(), 1, 1);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (at),
                         [cb, idx] (int r) { if (r > 0) cb (idx, r); });
    }

    Grab                 grab_ = Grab::None;
    int                  grabIdx_ = -1;
    VenueElement         start_;
    double               dragFromX_ = 0.0, dragFromY_ = 0.0;
    juce::Point<float>   lastPx_;

    // Even-odd crossing test: the outline is four free points, so it is not
    // always a rectangle and a bounds check would pick the wrong one.
    bool hits (const VenueElement& el, juce::Point<float> p,
               juce::Rectangle<int> f) const
    {
        bool in = false;
        for (int i = 0, j = 3; i < 4; j = i++)
        {
            const float xi = xToScreen (el.pts[i].x, f), yi = yToScreen (el.pts[i].y, f);
            const float xj = xToScreen (el.pts[j].x, f), yj = yToScreen (el.pts[j].y, f);
            if (((yi > p.y) != (yj > p.y))
                && (p.x < (xj - xi) * (p.y - yi) / (yj - yi + 1.0e-9f) + xi))
                in = ! in;
        }
        return in;
    }

    void drawElement (juce::Graphics& g, juce::Rectangle<int> f,
                      const VenueElement& e, bool sel) const
    {
        juce::Path p;
        for (int i = 0; i < 4; ++i)
        {
            const float x = xToScreen (e.pts[i].x, f);
            const float y = yToScreen (e.pts[i].y, f);
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        p.closeSubPath();

        g.setColour (e.colour().withAlpha (0.75f));
        g.fillPath (p);
        g.setColour (markCol().withAlpha (sel ? 1.0f : 0.45f));
        g.strokePath (p, juce::PathStrokeType (1.2f));

        if (! sel) return;
        // Corners, and one on each edge's midpoint - the marks the reference
        // drawing puts there.
        for (int i = 0; i < 4; ++i)
        {
            const float x  = xToScreen (e.pts[i].x, f);
            const float y  = yToScreen (e.pts[i].y, f);
            const auto& n  = e.pts[(i + 1) % 4];
            const float nx = xToScreen (n.x, f);
            const float ny = yToScreen (n.y, f);
            drawMark (g, x, y, true);
            drawMark (g, (x + nx) * 0.5f, (y + ny) * 0.5f, true);
        }
    }
};

// -----------------------------------------------------------------------------
// Elevation.
// -----------------------------------------------------------------------------
class VenueElevationPlot : public VenuePlotBase
{
public:
    VenueElevationPlot() { margins_ = { 59.0f, 41.0f, 39.0f, 44.0f }; }

    // Panning only. Shapes are edited in the plan, where both of their
    // horizontal coordinates are visible; dragging here could only ever move
    // one of them and would look like it had lost the other.
    void mouseDown (const juce::MouseEvent& e) override { lastPx_ = e.position; }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        panBy ({ e.position.x - lastPx_.x, 0.0f });
        lastPx_ = e.position;
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (Brand::panel());
        auto f = fieldArea();
        if (f.getWidth() < 40 || f.getHeight() < 30) return;

        drawField (g, f);

        g.setColour (gridCol());
        for (double m = 0.0; m <= kZTop + 1.0e-6; m += stepM())
        {
            const int y = juce::roundToInt (zToScreen (m, f, kZTop));
            if (y >= f.getY() && y <= f.getBottom())
                g.drawHorizontalLine (y, (float) f.getX(), (float) f.getRight());
        }
        drawXGrid (g, f);
        drawFrame (g, f);

        // Figma: SemiBold 6 across the top (20 down), SemiBold 10 down the
        // left (33 in).
        // The same size as the plan's, and as this pane's own side figures.
        // The top row used to be 6 against 10 down the side, which read as a
        // different, lesser axis rather than the other half of the same one.
        drawXLabels (g, f, 12.0f, 20.0f);
        g.setFont (Brand::figSemi (12.0f));
        g.setColour (tickCol());
        {
            // The vertical here is exaggerated, so its step is picked from the
            // pixels per METRE of this pane, not the plan's.
            const double pxPerZ = (double) f.getHeight() / kZTop;
            const double st = labelStep (pxPerZ, UiConfig::Scale::fig (14.0f));
            for (double m = 0.0; m <= kZTop + 1.0e-6; m += st)
            {
                const int y = juce::roundToInt (zToScreen (m, f, kZTop));
                drawYLabel (g, f, y, tick (m), 24.0f, true);
            }
        }

        // The deck is a LINE on z = 0 inside the field, the way the Sources
        // elevation draws it. A thick bar slung under the field read as a
        // horizontal scrollbar.
        {
            const float g0 = zToScreen (0.0, f, kZTop);
            g.setColour (groundCol());
            g.drawLine ((float) f.getX(), g0, (float) f.getRight(), g0, 2.0f);
        }

        // Clipped to the field, as the plan is: a run that reaches past the
        // edge must stop at the frame, not paint across the tick labels.
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (f);

        if (elems_ != nullptr)
            for (int i = 0; i < (int) elems_->size(); ++i)
            {
                const auto& e = (*elems_)[(size_t) i];
                float lo = e.pts[0].x, hi = e.pts[0].x, z = e.pts[0].z;
                for (int k = 1; k < 4; ++k)
                {
                    lo = juce::jmin (lo, e.pts[k].x);
                    hi = juce::jmax (hi, e.pts[k].x);
                    z  = juce::jmax (z,  e.pts[k].z);
                }
                const float x0 = xToScreen (lo, f);
                const float x1 = xToScreen (hi, f);
                // Kept inside the field so a run on the deck is not clipped
                // down to a sliver by the frame.
                const float t  = 3.0f;
                const float zy = juce::jmin (zToScreen (z, f, kZTop), (float) f.getBottom() - t * 0.5f);
                g.setColour (markCol().withAlpha (i == sel_ ? 1.0f : 0.45f));
                g.drawLine (x0, zy, x1, zy, t);

                // A seating plane also shows where the ears are: a dotted line
                // the audience height above it.
                if (e.hasAudHeight())
                {
                    const float earM = VenueElement::earHeightM (e.height);
                    const float ey   = zToScreen (z + earM, f, kZTop);
                    const float dashes[] = { 3.0f, 3.0f };
                    g.drawDashedLine ({ x0, ey, x1, ey }, dashes, 2, 2.0f);
                }
                if (i == sel_) { drawMark (g, x0, zy, true); drawMark (g, x1, zy, true); }
            }

        drawMark (g, xToScreen (0.0, f), zToScreen (0.0, f, kZTop), false);
    }

private:
    juce::Point<float> lastPx_;
    static constexpr double kZTop = 16.0;
};

// -----------------------------------------------------------------------------
// The centre of the Venue tab: the plan, then the elevation under its own
// foldable header - the same stack the Sources tab uses, so switching tabs does
// not rearrange the window.
// -----------------------------------------------------------------------------
class VenueView : public juce::Component
{
public:
    VenueView()
    {
        addAndMakeVisible (top_);
        addAndMakeVisible (elevHead_);
        addAndMakeVisible (elev_);

        // One camera, both panes - pan or zoom either and the other follows,
        // so a metre stays in the same column top and bottom.
        top_.setCamera  (&cam_);
        elev_.setCamera (&cam_);
        top_.onCameraMoved  = [this] { elev_.repaint(); };
        elev_.onCameraMoved = [this] { top_.repaint(); };
        top_.onSelect = [this] (int i) { if (onSelect) onSelect (i); };
        top_.onElementChanged = [this] (int i, VenueElement el)
        { if (onElementChanged) onElementChanged (i, el); };
        top_.onAction = [this] (int i, int a) { if (onAction) onAction (i, a); };
        elevHead_.onFold = [this] (bool) { resized(); };
    }

    void setElements (const std::vector<VenueElement>* e, int sel)
    {
        top_.setElements (e, sel);
        elev_.setElements (e, sel);
    }

    std::function<void (int)>                    onSelect;
    std::function<void (int, VenueElement)>      onElementChanged;
    std::function<void (int, int)>               onAction;
    using Action = VenueTopPlot::Action;

    /** Back to the framing the panes open with. */
    void resetView() { cam_.reset(); top_.repaint(); elev_.repaint(); }

    void resized() override
    {
        // Figma V1.1: a 30 px header; the plan (560) and the elevation (402)
        // share the rest in that ratio - the same split the Sources tab uses.
        const int hdrH  = juce::jmax (18, UiConfig::Scale::fig (30.0f));
        const int elevH = elevHead_.isFolded() ? 0
                        : juce::jmax (80, (int) ((getHeight() - hdrH) * (402.0f / 962.0f)));
        const int topH  = juce::jmax (80, getHeight() - hdrH - elevH);

        top_.setBounds      (0, 0,             getWidth(), topH);
        elevHead_.setBounds (0, topH,          getWidth(), hdrH);
        elev_.setBounds     (0, topH + hdrH,   getWidth(), elevH);
        elev_.setVisible (elevH > 0);
    }

private:
    VenueCamera        cam_;
    VenueTopPlot       top_;
    PaneHeaderBar      elevHead_ { "Elevation view" };
    VenueElevationPlot elev_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VenueView)
};
