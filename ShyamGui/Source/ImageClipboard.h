#pragma once
#include <JuceHeader.h>

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
 #ifdef min
  #undef min
 #endif
 #ifdef max
  #undef max
 #endif
 // windows.h still ships these as legacy memory-model keywords, and they are
 // perfectly ordinary variable names elsewhere in this codebase.
 #ifdef far
  #undef far
 #endif
 #ifdef near
  #undef near
 #endif
#endif

// ---------------------------------------------------------------------------
// Put a picture on the system clipboard, so it can be pasted straight into
// Paint, a document, a chat window - anywhere that takes an image.
//
// juce::SystemClipboard carries text only, so this goes to the platform.
// On Windows that means a device-independent bitmap under CF_DIB: the format
// every paste target has understood since forever. 24-bit, bottom-up, rows
// padded to 4 bytes, which is the shape the format demands.
// ---------------------------------------------------------------------------
namespace ImageClipboard
{
#if JUCE_WINDOWS

inline bool copyImage (const juce::Image& img)
{
    if (! img.isValid()) return false;

    const int w = img.getWidth();
    const int h = img.getHeight();
    if (w <= 0 || h <= 0) return false;

    const size_t stride = (size_t) ((w * 3 + 3) / 4) * 4;   // 4-byte aligned rows
    const size_t bytes  = stride * (size_t) h;
    const size_t total  = sizeof (BITMAPINFOHEADER) + bytes;

    // The clipboard takes ownership of moveable global memory, so this is
    // deliberately not a juce::HeapBlock.
    HGLOBAL mem = GlobalAlloc (GMEM_MOVEABLE, total);
    if (mem == nullptr) return false;

    auto* base = static_cast<unsigned char*> (GlobalLock (mem));
    if (base == nullptr) { GlobalFree (mem); return false; }

    std::memset (base, 0, total);
    auto* bih = reinterpret_cast<BITMAPINFOHEADER*> (base);
    bih->biSize        = sizeof (BITMAPINFOHEADER);
    bih->biWidth       = (LONG) w;
    bih->biHeight      = (LONG) h;          // positive: rows run bottom to top
    bih->biPlanes      = 1;
    bih->biBitCount    = 24;
    bih->biCompression = BI_RGB;
    bih->biSizeImage   = (DWORD) bytes;

    unsigned char* bits = base + sizeof (BITMAPINFOHEADER);
    {
        const juce::Image::BitmapData src (img, juce::Image::BitmapData::readOnly);
        for (int y = 0; y < h; ++y)
        {
            unsigned char* row = bits + (size_t) (h - 1 - y) * stride;
            for (int x = 0; x < w; ++x)
            {
                const auto c = src.getPixelColour (x, y);
                row[x * 3 + 0] = c.getBlue();     // DIBs are BGR, not RGB
                row[x * 3 + 1] = c.getGreen();
                row[x * 3 + 2] = c.getRed();
            }
        }
    }
    GlobalUnlock (mem);

    if (! OpenClipboard (nullptr)) { GlobalFree (mem); return false; }
    EmptyClipboard();
    const bool ok = (SetClipboardData (CF_DIB, mem) != nullptr);
    CloseClipboard();
    // Only on failure is the block still ours to release; on success the
    // clipboard owns it and freeing it here would be a double free.
    if (! ok) GlobalFree (mem);
    return ok;
}

#else

inline bool copyImage (const juce::Image&) { return false; }

#endif
}
