"""Convertit le logo Clever Cellar en bitmap monochrome packe pour Adafruit_GFX.
Usage: python make_logo.py
Genere include/logo.h

Le sous-echantillonnage naif (resize lisse puis seuil) dilue les traits fins du
logo a 56x56 (quasi invisibles). On binarise d'abord a haute resolution, puis on
sous-echantillonne en gardant tout pixel noir present dans chaque bloc source
(equivalent a une dilatation), ce qui preserve les traits fins.
"""
from PIL import Image

SRC = "ChartGraphique/clever_cellar_engraving_512px.png"
OUT = "include/logo.h"
SIZE = 56  # cote du bitmap carre (px) - laisse de la marge sur l'OLED 128x64
THRESHOLD = 200  # pixel <seuil -> trait noir (large pour capter l'anti-aliasing)

src_rgba = Image.open(SRC).convert("RGBA")
bg = Image.new("RGBA", src_rgba.size, (255, 255, 255, 255))
src = Image.alpha_composite(bg, src_rgba).convert("L")
W, H = src.size
bw_full = src.point(lambda p: 1 if p < THRESHOLD else 0)

bw_full_px = bw_full.load()
data = bytearray(((SIZE + 7) // 8) * SIZE)
bytes_per_row = (SIZE + 7) // 8

for oy in range(SIZE):
    y0, y1 = (oy * H) // SIZE, ((oy + 1) * H) // SIZE
    for ox in range(SIZE):
        x0, x1 = (ox * W) // SIZE, ((ox + 1) * W) // SIZE
        lit = False
        for sy in range(y0, max(y1, y0 + 1)):
            for sx in range(x0, max(x1, x0 + 1)):
                if bw_full_px[sx, sy]:
                    lit = True
                    break
            if lit:
                break
        if lit:
            data[oy * bytes_per_row + (ox // 8)] |= (0x80 >> (ox % 8))

with open(OUT, "w") as f:
    f.write("#pragma once\n")
    f.write("#include <Arduino.h>\n\n")
    f.write("// Logo Clever Cellar - genere par tools/make_logo.py depuis\n")
    f.write(f"// ChartGraphique/clever_cellar_engraving_512px.png ({SIZE}x{SIZE}, 1-bit)\n")
    f.write(f"#define LOGO_WIDTH  {SIZE}\n")
    f.write(f"#define LOGO_HEIGHT {SIZE}\n")
    f.write("const unsigned char LOGO_BITMAP[] PROGMEM = {\n")
    for i in range(0, len(data), bytes_per_row):
        row = data[i:i+bytes_per_row]
        f.write("  " + ", ".join(f"0x{b:02X}" for b in row) + ",\n")
    f.write("};\n")

print(f"OK: {OUT} ({SIZE}x{SIZE}, {len(data)} bytes)")
