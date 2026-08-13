#pragma once
#include <JuceHeader.h>

// ===========================================================================
// LayoutLayer - an imported architectural reference (floor plan / blueprint)
// shown beneath the acoustic objects as a visual underlay. Supports raster
// images (PNG/JPG) and vector DXF drawings. The layer is purely a reference:
// it never feeds the acoustic engine.
//
// Placement is expressed in WORLD METRES so it stays locked to the simulation
// grid while zooming/panning. The content's bounding box is anchored at
// originM (world metres, bottom-left) and spans widthM across; the height is
// derived from the source aspect ratio. Scale, rotation, opacity, visibility
// and lock are user-controllable.
// ===========================================================================
struct LayoutLayer
{
    enum class Kind { None, Image, Dxf };

    Kind          kind = Kind::None;
    juce::String  name;

    juce::Image            image;       // Image kind
    juce::Path             path;        // Dxf kind (source units, y-DOWN)
    juce::Rectangle<float> srcBounds;   // content bounds in source units

    // Placement (world metres) ---------------------------------------------
    juce::Point<float> originM { 10.0f, 10.0f };  // world pos of content bottom-left
    float widthM      = 12.0f;   // world width of the content bbox (defines scale)
    float rotationDeg = 0.0f;
    float opacity     = 0.65f;
    bool  visible     = true;
    bool  locked      = false;

    bool valid() const noexcept
    {
        return kind != Kind::None && (image.isValid() || ! path.isEmpty());
    }

    float aspect() const noexcept
    {
        return srcBounds.getHeight() > 0.0f ? srcBounds.getWidth() / srcBounds.getHeight() : 1.0f;
    }
    float heightM() const noexcept { return aspect() > 0.0f ? widthM / aspect() : widthM; }

    // Metres per source unit (uniform scale).
    float metresPerUnit() const noexcept
    {
        return srcBounds.getWidth() > 0.0f ? widthM / srcBounds.getWidth() : 1.0f;
    }

    void clear()
    {
        kind = Kind::None;
        name = {};
        image = juce::Image();
        path.clear();
        srcBounds = {};
    }
};
