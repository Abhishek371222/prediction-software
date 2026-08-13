#pragma once
#include <JuceHeader.h>
#include <vector>
#include <functional>

// ===========================================================================
// PdfDocument - a small, self-contained PDF 1.4 writer (no external deps).
//
// Generates real vector PDFs: selectable text using the base-14 Helvetica /
// Helvetica-Bold fonts, vector rectangles / lines, and embedded raster images
// (JPEG / DCTDecode) for heatmaps and graphs. Pages are A4 by default.
//
// Public drawing API uses a TOP-LEFT origin (y grows downward), in points
// (1/72 inch); coordinates are flipped to PDF's bottom-left space internally.
// ===========================================================================
class PdfDocument
{
public:
    explicit PdfDocument (double pageW = 595.276, double pageH = 841.89)
        : pageW_ (pageW), pageH_ (pageH) {}

    double pageWidth()  const noexcept { return pageW_; }
    double pageHeight() const noexcept { return pageH_; }

    void newPage()
    {
        pages_.push_back (std::make_unique<Page>());
        cur_ = pages_.back().get();
    }

    // --- Vector primitives -------------------------------------------------
    void fillRect (double x, double y, double w, double h, juce::Colour c)
    {
        ensurePage();
        *cur_ << colourOp (c, false)
              << fmt (x) << " " << fmt (flipY (y + h)) << " " << fmt (w) << " " << fmt (h) << " re f\n";
    }

    void strokeRect (double x, double y, double w, double h, juce::Colour c, double lineWidth = 1.0)
    {
        ensurePage();
        *cur_ << colourOp (c, true) << fmt (lineWidth) << " w "
              << fmt (x) << " " << fmt (flipY (y + h)) << " " << fmt (w) << " " << fmt (h) << " re S\n";
    }

    void line (double x1, double y1, double x2, double y2, juce::Colour c, double lineWidth = 1.0)
    {
        ensurePage();
        *cur_ << colourOp (c, true) << fmt (lineWidth) << " w "
              << fmt (x1) << " " << fmt (flipY (y1)) << " m "
              << fmt (x2) << " " << fmt (flipY (y2)) << " l S\n";
    }

    // --- Text --------------------------------------------------------------
    // (x, yTop) is the top-left of the text's cap height.
    void text (double x, double yTop, double size, const juce::String& s,
               juce::Colour c = juce::Colours::black, bool bold = false)
    {
        ensurePage();
        const double baseline = flipY (yTop + size * 0.80);
        *cur_ << colourOp (c, false)
              << "BT /" << (bold ? "F2" : "F1") << " " << fmt (size) << " Tf "
              << fmt (x) << " " << fmt (baseline) << " Td ("
              << escape (s) << ") Tj ET\n";
    }

    void textRight (double xRight, double yTop, double size, const juce::String& s,
                    juce::Colour c = juce::Colours::black, bool bold = false)
    {
        text (xRight - textWidth (s, size, bold), yTop, size, s, c, bold);
    }

    void textCentred (double xCentre, double yTop, double size, const juce::String& s,
                      juce::Colour c = juce::Colours::black, bool bold = false)
    {
        text (xCentre - textWidth (s, size, bold) * 0.5, yTop, size, s, c, bold);
    }

    double textWidth (const juce::String& s, double size, bool bold) const
    {
        const int* w = bold ? helvBoldW() : helvW();
        double total = 0.0;
        auto utf8 = s.toRawUTF8();
        for (auto* p = utf8; *p != 0; ++p)
        {
            int ch = (unsigned char) *p;
            if (ch < 32 || ch > 126) ch = 32;
            total += w[ch - 32];
        }
        return total / 1000.0 * size;
    }

    // Word-wrap helper: draws paragraph, returns y after the last line.
    double textWrapped (double x, double yTop, double maxW, double size,
                        const juce::String& s, juce::Colour c = juce::Colours::black,
                        bool bold = false, double leading = 1.35)
    {
        juce::StringArray words;
        words.addTokens (s, " ", "");
        juce::String lineStr;
        double y = yTop;
        for (auto& word : words)
        {
            juce::String tryLine = lineStr.isEmpty() ? word : lineStr + " " + word;
            if (textWidth (tryLine, size, bold) > maxW && lineStr.isNotEmpty())
            {
                text (x, y, size, lineStr, c, bold);
                y += size * leading;
                lineStr = word;
            }
            else lineStr = tryLine;
        }
        if (lineStr.isNotEmpty()) { text (x, y, size, lineStr, c, bold); y += size * leading; }
        return y;
    }

    // --- Images ------------------------------------------------------------
    void drawImage (const juce::Image& image, double x, double yTop, double w, double h,
                    float jpegQuality = 0.85f)
    {
        ensurePage();
        juce::MemoryOutputStream mos;
        juce::JPEGImageFormat fmtJ;
        fmtJ.setQuality (jpegQuality);
        fmtJ.writeImageToStream (image.convertedToFormat (juce::Image::RGB), mos);

        Img im;
        im.data = mos.getMemoryBlock();
        im.w = image.getWidth();
        im.h = image.getHeight();
        const int index = (int) images_.size();
        images_.push_back (std::move (im));

        *cur_ << "q " << fmt (w) << " 0 0 " << fmt (h) << " "
              << fmt (x) << " " << fmt (flipY (yTop + h)) << " cm /Img" << index << " Do Q\n";
    }

    // --- Output ------------------------------------------------------------
    bool writeToFile (const juce::File& f)
    {
        juce::MemoryOutputStream out;
        std::vector<juce::int64> offsets;   // [objNum] -> byte offset (1-based)

        const int nImg   = (int) images_.size();
        const int nPages = (int) pages_.size();

        const int objCatalog = 1;
        const int objPages   = 2;
        const int objF1      = 3;
        const int objF2      = 4;
        const int imgStart   = 5;
        const int pageStart  = imgStart + nImg;   // each page uses 2 objects
        const int totalObjs  = pageStart + nPages * 2 - 1;

        offsets.assign ((size_t) totalObjs + 1, 0);

        out << "%PDF-1.4\n%\xE2\xE3\xCF\xD3\n";

        auto beginObj = [&] (int n) { offsets[(size_t) n] = out.getPosition(); out << n << " 0 obj\n"; };
        auto endObj   = [&] ()      { out << "endobj\n"; };

        // Catalog
        beginObj (objCatalog);
        out << "<< /Type /Catalog /Pages " << objPages << " 0 R >>\n";
        endObj();

        // Pages tree
        beginObj (objPages);
        out << "<< /Type /Pages /Count " << nPages << " /Kids [";
        for (int i = 0; i < nPages; ++i) out << (pageStart + i * 2 + 1) << " 0 R ";
        out << "] >>\n";
        endObj();

        // Fonts
        beginObj (objF1);
        out << "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>\n";
        endObj();
        beginObj (objF2);
        out << "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold /Encoding /WinAnsiEncoding >>\n";
        endObj();

        // Images
        for (int i = 0; i < nImg; ++i)
        {
            beginObj (imgStart + i);
            out << "<< /Type /XObject /Subtype /Image /Width " << images_[(size_t) i].w
                << " /Height " << images_[(size_t) i].h
                << " /ColorSpace /DeviceRGB /BitsPerComponent 8 /Filter /DCTDecode /Length "
                << (int) images_[(size_t) i].data.getSize() << " >>\nstream\n";
            out.write (images_[(size_t) i].data.getData(), images_[(size_t) i].data.getSize());
            out << "\nendstream\n";
            endObj();
        }

        // Pages (content + page object)
        for (int i = 0; i < nPages; ++i)
        {
            const int contentObj = pageStart + i * 2;
            const int pageObj     = pageStart + i * 2 + 1;
            const auto contentStr = pages_[(size_t) i]->toString();

            beginObj (contentObj);
            out << "<< /Length " << (int) contentStr.getNumBytesAsUTF8() << " >>\nstream\n";
            out << contentStr;
            out << "\nendstream\n";
            endObj();

            beginObj (pageObj);
            out << "<< /Type /Page /Parent " << objPages << " 0 R /MediaBox [0 0 "
                << fmt (pageW_) << " " << fmt (pageH_) << "] /Contents " << contentObj
                << " 0 R /Resources << /Font << /F1 " << objF1 << " 0 R /F2 " << objF2 << " 0 R >>";
            if (nImg > 0)
            {
                out << " /XObject <<";
                for (int k = 0; k < nImg; ++k) out << " /Img" << k << " " << (imgStart + k) << " 0 R";
                out << " >>";
            }
            out << " >> >>\n";
            endObj();
        }

        // xref
        const juce::int64 xrefPos = out.getPosition();
        out << "xref\n0 " << (totalObjs + 1) << "\n";
        out << "0000000000 65535 f\r\n";
        for (int n = 1; n <= totalObjs; ++n)
        {
            juce::String e (offsets[(size_t) n]);
            while (e.length() < 10) e = "0" + e;
            out << e << " 00000 n\r\n";
        }
        out << "trailer\n<< /Size " << (totalObjs + 1) << " /Root " << objCatalog << " 0 R >>\n";
        out << "startxref\n" << xrefPos << "\n%%EOF\n";

        return f.replaceWithData (out.getData(), out.getDataSize());
    }

private:
    struct Page
    {
        juce::MemoryOutputStream s;
        template <typename T> Page& operator<< (const T& v) { s << v; return *this; }
        juce::String toString() const { return s.toString(); }
    };
    struct Img { juce::MemoryBlock data; int w = 0, h = 0; };

    void ensurePage() { if (cur_ == nullptr) newPage(); }
    double flipY (double y) const { return pageH_ - y; }

    static juce::String fmt (double v) { return juce::String (v, 3); }

    juce::String colourOp (juce::Colour c, bool stroke) const
    {
        const double r = c.getFloatRed(), g = c.getFloatGreen(), b = c.getFloatBlue();
        return fmt (r) + " " + fmt (g) + " " + fmt (b) + (stroke ? " RG " : " rg ");
    }

    static juce::String escape (const juce::String& s)
    {
        juce::String out;
        auto utf8 = s.toRawUTF8();
        for (auto* p = utf8; *p != 0; ++p)
        {
            const int ch = (unsigned char) *p;
            if (ch == '(' || ch == ')' || ch == '\\') { out << '\\'; out << (juce::juce_wchar) ch; }
            else if (ch < 32 || ch > 126)              out << ' ';
            else                                       out << (juce::juce_wchar) ch;
        }
        return out;
    }

    // Helvetica advance widths (1/1000 em), codes 32..126.
    static const int* helvW()
    {
        static const int w[95] = {
            278,278,355,556,556,889,667,191,333,333,389,584,278,333,278,278,
            556,556,556,556,556,556,556,556,556,556,278,278,584,584,584,556,
            1015,667,667,722,722,667,611,778,722,278,500,667,556,833,722,778,
            667,778,722,667,611,722,667,944,667,667,611,278,278,278,469,556,
            333,556,556,500,556,556,278,556,556,222,222,500,222,833,556,556,
            556,556,333,500,278,556,500,722,500,500,500,334,260,334,584 };
        return w;
    }
    static const int* helvBoldW()
    {
        static const int w[95] = {
            278,333,474,556,556,889,722,238,333,333,389,584,278,333,278,278,
            556,556,556,556,556,556,556,556,556,556,333,333,584,584,584,611,
            975,722,722,722,722,667,611,778,722,278,556,722,611,833,722,778,
            667,778,722,667,611,722,667,944,667,667,611,333,278,333,584,556,
            333,556,611,556,611,556,333,611,611,278,278,556,278,889,611,611,
            611,611,389,556,333,611,556,778,556,556,500,389,280,389,584 };
        return w;
    }

    double pageW_, pageH_;
    std::vector<std::unique_ptr<Page>> pages_;
    Page* cur_ = nullptr;

    std::vector<Img> images_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PdfDocument)
};
