from pathlib import Path
path = Path(r"d:\shayam gui\Source\BrandTheme.h")
text = path.read_text(encoding="utf-8")
start = text.index("// ===========================================================================\n// Atomik brand theme")
end = text.index("    // Typography")
new = Path(r"d:\shayam gui\Tools\palette_block.txt").read_text(encoding="utf-8")
path.write_text(text[:start] + new + text[end:], encoding="utf-8")
print("ok", start, end)
