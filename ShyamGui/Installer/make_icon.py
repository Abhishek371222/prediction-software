"""
Generate Assets/atomik_icon.ico from the Atomik wordmark (the same artwork used
for the in-app taskbar icon: white wordmark on a charcoal tile).

The wordmark is rendered from the brand SVG via headless Chrome (white on black
for a clean anti-aliased trim), then composited onto a charcoal square at the
standard Windows icon sizes and packed into a multi-resolution .ico.
"""
import os, sys, subprocess, tempfile
from PIL import Image

CHROME_CANDIDATES = [
    r"C:\Program Files\Google\Chrome\Application\chrome.exe",
    r"C:\Program Files (x86)\Google\Chrome\Application\chrome.exe",
    r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
    r"C:\Program Files\Microsoft\Edge\Application\msedge.exe",
]

OUT_ICO = r"D:\shayam gui\Assets\atomik_icon.ico"
OUT_PNG = r"D:\shayam gui\Assets\atomik_icon_256.png"

CHARCOAL = (35, 31, 32, 255)   # #231f20

SVG = """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 566.93 283.46">
  <g fill="#ffffff">
    <polygon points="43.37 178.68 72.37 114.23 72.37 114.23 68.35 105.28 35.32 178.68 43.37 178.68"/>
    <polyline points="93.33 178.68 101.38 178.68 101.38 178.68 88.17 149.32 88.17 149.32 80.12 149.32 93.33 178.68"/>
    <polygon points="443.63 105.28 436.29 105.28 436.29 178.68 436.29 178.68 443.63 178.68 443.63 178.68 443.63 105.28 443.63 105.28"/>
    <polyline points="450.21 141.98 486.91 178.68 486.91 178.68 497.29 178.68 497.29 178.68 460.59 141.98 497.29 105.29 497.29 105.28 486.91 105.28 486.91 105.29 450.21 141.98"/>
    <polygon points="177.42 105.28 140.37 105.28 140.37 178.68 147.71 178.68 147.71 112.62 177.42 112.62 177.42 105.28"/>
    <rect x="109.56" y="105.28" width="21.36" height="7.34"/>
    <polygon points="402.59 178.68 402.59 105.28 402.59 105.28 395.25 105.28 395.25 178.68 395.25 178.68 402.59 178.68 402.59 178.68"/>
    <polygon points="302.83 112.93 295.49 104.78 295.49 178.68 295.49 178.68 302.83 178.68 302.83 112.93"/>
    <polygon points="361.55 178.68 361.55 104.78 354.21 112.94 354.21 178.68 361.55 178.68"/>
    <polygon points="344.76 123.44 328.52 141.48 328.52 141.48 312.28 123.44 312.28 134.41 328.52 152.45 328.52 152.45 344.76 134.41 344.76 123.44"/>
    <path d="M204.89,123.63h0c0-6.08,4.93-11.01,11.01-11.01h29.36,0c6.08,0,11.01,4.93,11.01,11.01v13.62h7.34v-13.62c0-10.13-8.22-18.35-18.35-18.35h0s-29.36,0-29.36,0c-10.13,0-18.35,8.22-18.35,18.35h0s0,13.62,0,13.62h7.34v-13.63Z"/>
    <path d="M256.28,160.33h0c0,6.08-4.93,11.01-11.01,11.01h-29.36,0c-6.08,0-11.01-4.93-11.01-11.01v-13.62h-7.34v13.62c0,10.13,8.22,18.35,18.35,18.35h0s29.36,0,29.36,0c10.13,0,18.35-8.22,18.35-18.35h0s0-13.62,0-13.62h-7.34v13.63Z"/>
    <circle cx="521.37" cy="141.73" r="10.25"/>
  </g>
</svg>"""


def find_browser():
    for p in CHROME_CANDIDATES:
        if os.path.exists(p):
            return p
    sys.exit("No Chrome/Edge found for rendering.")


def render_wordmark(tmp):
    html = (
        "<!doctype html><html><head><meta charset='utf-8'>"
        "<style>html,body{margin:0;padding:0;background:#000;}"
        "#w{width:1600px;height:800px;display:flex;align-items:center;justify-content:center;}"
        "svg{width:1500px;height:auto;}</style></head>"
        "<body><div id='w'>" + SVG + "</div></body></html>"
    )
    html_path = os.path.join(tmp, "wordmark.html")
    png_path = os.path.join(tmp, "wordmark.png")
    with open(html_path, "w", encoding="utf-8") as f:
        f.write(html)

    browser = find_browser()
    subprocess.run([
        browser, "--headless=new", "--disable-gpu", "--hide-scrollbars",
        "--no-sandbox", "--force-device-scale-factor=1",
        "--user-data-dir=" + os.path.join(tmp, "profile"),
        "--screenshot=" + png_path, "--window-size=1600,800",
        "file:///" + html_path.replace("\\", "/"),
    ], check=True, timeout=120)
    return png_path


def main():
    tmp = tempfile.mkdtemp()
    png = render_wordmark(tmp)

    # White glyphs on black -> use luminance as alpha, trim to glyph bounds.
    lum = Image.open(png).convert("L")
    bbox = lum.getbbox()
    if not bbox:
        sys.exit("Rendered wordmark was empty.")
    word = Image.new("RGBA", lum.size, (255, 255, 255, 0))
    word.putalpha(lum)
    word = word.crop(bbox)

    sizes = [16, 24, 32, 48, 64, 128, 256]
    frames = []
    for S in sizes:
        tile = Image.new("RGBA", (S, S), CHARCOAL)
        tw = int(round(S * 0.84))
        ratio = tw / word.width
        th = int(round(word.height * ratio))
        max_h = int(S * 0.46)
        if th > max_h:
            th = max_h
            ratio = th / word.height
            tw = int(round(word.width * ratio))
        wm = word.resize((max(1, tw), max(1, th)), Image.LANCZOS)
        tile.alpha_composite(wm, ((S - tw) // 2, (S - th) // 2))
        frames.append(tile)

    frames[-1].save(OUT_PNG)
    frames[-1].save(OUT_ICO, format="ICO",
                    sizes=[(f.width, f.height) for f in frames])
    print("Wrote", OUT_ICO, "and", OUT_PNG)


if __name__ == "__main__":
    main()
