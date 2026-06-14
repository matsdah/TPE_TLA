"""
Genera fondos para la novela visual: un bosque y una cueva.
Imágenes RGB de 800x600 (relación del player).

Uso:  python make_backgrounds.py
"""

from PIL import Image, ImageDraw, ImageFilter
import os
import math
import random

W, H = 800, 600
HERE = os.path.dirname(os.path.abspath(__file__))


def _lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))


def _vertical_gradient(draw, top, bottom, y0=0, y1=H):
    for y in range(y0, y1):
        t = (y - y0) / max(1, (y1 - y0 - 1))
        draw.line([(0, y), (W, y)], fill=_lerp(top, bottom, t))


def make_forest():
    img = Image.new("RGB", (W, H))
    d = ImageDraw.Draw(img)
    rnd = random.Random(7)

    # Cielo: celeste claro arriba -> cálido cerca del horizonte
    _vertical_gradient(d, (150, 200, 235), (210, 225, 200), 0, 360)

    # Sol difuso
    sun = Image.new("RGB", (W, H), (0, 0, 0))
    sd = ImageDraw.Draw(sun)
    sd.ellipse([600, 50, 760, 210], fill=(255, 240, 200))
    sun = sun.filter(ImageFilter.GaussianBlur(40))
    img = Image.blend(img, Image.composite(sun, img, sun.convert("L")), 0.5)
    d = ImageDraw.Draw(img)

    # Capas de colinas/bosque al fondo (más claras y azuladas = más lejos)
    layers = [
        (360, (120, 165, 130)),
        (400, (90, 145, 110)),
        (445, (65, 120, 90)),
    ]
    for base_y, color in layers:
        pts = [(0, H)]
        x = 0
        while x <= W:
            y = base_y + rnd.randint(-25, 25)
            pts.append((x, y))
            x += rnd.randint(40, 90)
        pts.append((W, H))
        d.polygon(pts, fill=color)

    # Suelo del claro (pasto)
    _vertical_gradient(d, (70, 120, 70), (45, 85, 50), 470, H)

    # Árboles en primer plano (siluetas a los lados, enmarcando la escena)
    def tree(x, scale, color_trunk, color_leaves):
        tw = int(28 * scale)
        th = int(220 * scale)
        ty = 470
        # tronco
        d.rectangle([x - tw // 2, ty - th, x + tw // 2, ty + 30], fill=color_trunk)
        # copa: varios círculos
        cx, cy = x, ty - th
        for (dx, dy, r) in [(-40, 10, 55), (40, 10, 55), (0, -30, 70), (0, 30, 60)]:
            rr = int(r * scale)
            d.ellipse([cx + dx * scale - rr, cy + dy * scale - rr,
                       cx + dx * scale + rr, cy + dy * scale + rr], fill=color_leaves)

    tree(70, 1.5, (70, 50, 35), (40, 95, 55))
    tree(740, 1.6, (65, 45, 32), (35, 88, 50))
    tree(150, 1.0, (75, 55, 38), (50, 110, 65))

    # Niebla suave en la base para profundidad
    fog = Image.new("RGB", (W, H), (0, 0, 0))
    fd = ImageDraw.Draw(fog)
    fd.rectangle([0, 430, W, 510], fill=(200, 220, 200))
    fog = fog.filter(ImageFilter.GaussianBlur(30))
    img = Image.blend(img, Image.composite(fog, img, fog.convert("L").point(lambda p: p // 2)), 0.4)

    img.save(os.path.join(HERE, "forest.png"))
    print("escrito forest.png")


def make_cave():
    img = Image.new("RGB", (W, H))
    d = ImageDraw.Draw(img)
    rnd = random.Random(21)

    # Pared de roca: gradiente oscuro azulado, más claro al centro
    _vertical_gradient(d, (40, 42, 58), (18, 19, 30))

    # Textura de roca suave (manchas grandes y MUY difuminadas)
    rock = Image.new("RGB", (W, H), (28, 30, 44))
    rd = ImageDraw.Draw(rock)
    for _ in range(60):
        x = rnd.randint(0, W)
        y = rnd.randint(0, H)
        r = rnd.randint(40, 120)
        shade = rnd.randint(-14, 16)
        b = 30 + shade
        rd.ellipse([x - r, y - r, x + r, y + r], fill=(b, b + 2, b + 12))
    rock = rock.filter(ImageFilter.GaussianBlur(45))
    img = Image.blend(img, rock, 0.55)
    d = ImageDraw.Draw(img)

    # Entrada de luz al fondo (abertura iluminada en la pared)
    opening = Image.new("RGB", (W, H), (0, 0, 0))
    od = ImageDraw.Draw(opening)
    od.ellipse([320, 150, 480, 380], fill=(120, 130, 150))
    opening = opening.filter(ImageFilter.GaussianBlur(60))
    img = Image.blend(img, Image.composite(opening, img, opening.convert("L")), 0.45)
    d = ImageDraw.Draw(img)

    # Banda de roca del techo
    d.rectangle([0, 0, W, 40], fill=(20, 21, 32))

    # Estalactitas (techo) — siluetas oscuras y nítidas
    for _ in range(11):
        x = rnd.randint(-20, W + 20)
        w = rnd.randint(24, 64)
        h = rnd.randint(60, 180)
        col = (16 + rnd.randint(0, 10), 17 + rnd.randint(0, 10), 26 + rnd.randint(0, 14))
        d.polygon([(x - w // 2, 0), (x + w // 2, 0), (x, h)], fill=col)

    # Estalagmitas (suelo)
    for _ in range(8):
        x = rnd.randint(-20, W + 20)
        w = rnd.randint(30, 80)
        h = rnd.randint(60, 170)
        col = (14 + rnd.randint(0, 8), 15 + rnd.randint(0, 8), 24 + rnd.randint(0, 12))
        d.polygon([(x - w // 2, H), (x + w // 2, H), (x, H - h)], fill=col)

    # Suelo de la cueva
    _vertical_gradient(d, (26, 27, 40), (12, 12, 20), 510, H)
    d.line([(0, 510), (W, 510)], fill=(45, 47, 62), width=2)

    # Viñeta oscura en los bordes para dar profundidad
    vig = Image.new("L", (W, H), 0)
    vd = ImageDraw.Draw(vig)
    vd.ellipse([-120, -100, W + 120, H + 100], fill=255)
    vig = vig.filter(ImageFilter.GaussianBlur(140))
    dark = Image.new("RGB", (W, H), (6, 6, 12))
    img = Image.composite(img, dark, vig)

    img.save(os.path.join(HERE, "cave.png"))
    print("escrito cave.png")


make_forest()
make_cave()
print("Listo.")
