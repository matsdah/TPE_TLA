"""
Genera sprites de personajes estilo chibi para la novela visual.
Cada sprite es un PNG con fondo transparente (RGBA) de 360x520,
pensado para superponerse sobre el background.

Uso:  python make_sprites.py
"""

from PIL import Image, ImageDraw
import os

W, H = 360, 520
HERE = os.path.dirname(os.path.abspath(__file__))


def _lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(len(a)))


def draw_character(name, skin, hair, hair_dark, outfit, outfit_dark,
                   eye_color=(40, 40, 60), blush=(255, 150, 150),
                   hairstyle="short", mouth="smile"):
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    cx = W // 2
    outline = (60, 50, 55, 255)

    # ---- Cuerpo / torso (vestimenta) ----
    body_top = 300
    d.rounded_rectangle([cx - 95, body_top, cx + 95, H - 10], radius=55,
                        fill=outfit, outline=outline, width=4)
    # Sombra del torso
    d.rounded_rectangle([cx + 35, body_top + 20, cx + 95, H - 10], radius=45,
                        fill=outfit_dark)
    # Cuello de la vestimenta
    d.polygon([(cx - 45, body_top), (cx + 45, body_top),
               (cx, body_top + 55)], fill=outfit_dark, outline=outline)

    # ---- Brazos ----
    d.rounded_rectangle([cx - 120, body_top + 30, cx - 80, H - 60], radius=22,
                        fill=outfit, outline=outline, width=4)
    d.rounded_rectangle([cx + 80, body_top + 30, cx + 120, H - 60], radius=22,
                        fill=outfit, outline=outline, width=4)
    # Manos
    d.ellipse([cx - 124, H - 80, cx - 80, H - 36], fill=skin, outline=outline, width=3)
    d.ellipse([cx + 80, H - 80, cx + 124, H - 36], fill=skin, outline=outline, width=3)

    # ---- Cuello ----
    d.rectangle([cx - 22, 250, cx + 22, 310], fill=skin, outline=outline, width=3)

    # ---- Cabeza ----
    head_box = [cx - 105, 60, cx + 105, 290]
    d.ellipse(head_box, fill=skin, outline=outline, width=4)

    # ---- Pelo (atrás) ----
    if hairstyle == "long":
        # Dos mechones que caen por los costados, detrás de la cabeza,
        # sin cubrir el centro del mentón (para no parecer barba).
        d.rounded_rectangle([cx - 128, 120, cx - 78, 410], radius=26,
                            fill=hair_dark, outline=outline, width=3)
        d.rounded_rectangle([cx + 78, 120, cx + 128, 410], radius=26,
                            fill=hair_dark, outline=outline, width=3)
        d.ellipse(head_box, fill=skin, outline=outline, width=4)

    # ---- Pelo (flequillo / top) ----
    d.pieslice([cx - 110, 45, cx + 110, 250], start=180, end=360,
               fill=hair, outline=outline, width=4)
    if hairstyle == "spiky":
        for i in range(-3, 4):
            x = cx + i * 28
            d.polygon([(x - 18, 95), (x + 18, 95), (x, 40)], fill=hair, outline=outline)
    elif hairstyle == "long":
        # Mechones frontales cortos a los lados de la cara
        d.rounded_rectangle([cx - 112, 140, cx - 86, 250], radius=14, fill=hair, outline=outline, width=3)
        d.rounded_rectangle([cx + 86, 140, cx + 112, 250], radius=14, fill=hair, outline=outline, width=3)
    # mechón highlight
    d.arc([cx - 90, 60, cx + 90, 230], start=200, end=340, fill=hair_dark, width=6)

    # ---- Ojos ----
    eye_y = 185
    for ex in (cx - 42, cx + 42):
        d.ellipse([ex - 24, eye_y - 28, ex + 24, eye_y + 28], fill=(255, 255, 255), outline=outline, width=3)
        d.ellipse([ex - 16, eye_y - 18, ex + 16, eye_y + 22], fill=eye_color)
        d.ellipse([ex - 14, eye_y - 16, ex + 6, eye_y + 4], fill=_lerp(eye_color, (255, 255, 255), 0.4))
        d.ellipse([ex - 8, eye_y - 12, ex + 2, eye_y - 2], fill=(255, 255, 255))  # brillo

    # ---- Cejas ----
    d.line([cx - 62, eye_y - 40, cx - 22, eye_y - 36], fill=hair_dark, width=5)
    d.line([cx + 22, eye_y - 36, cx + 62, eye_y - 40], fill=hair_dark, width=5)

    # ---- Mejillas (blush) ----
    d.ellipse([cx - 80, 215, cx - 48, 240], fill=blush)
    d.ellipse([cx + 48, 215, cx + 80, 240], fill=blush)

    # ---- Boca ----
    if mouth == "smile":
        d.arc([cx - 26, 220, cx + 26, 262], start=10, end=170, fill=outline, width=5)
    elif mouth == "frown":
        d.arc([cx - 26, 248, cx + 26, 290], start=190, end=350, fill=outline, width=5)
    elif mouth == "neutral":
        d.line([cx - 18, 252, cx + 18, 252], fill=outline, width=5)
    elif mouth == "smirk":
        d.arc([cx - 26, 224, cx + 30, 262], start=10, end=120, fill=outline, width=5)

    # ---- Nariz ----
    d.line([cx, 210, cx + 4, 224], fill=_lerp(skin, outline, 0.4), width=3)

    img.save(os.path.join(HERE, name))
    print("escrito", name)


# Aria — protagonista: pelo castaño largo, vestimenta roja, alegre
draw_character("character.png",
               skin=(255, 224, 196), hair=(150, 90, 55), hair_dark=(110, 62, 38),
               outfit=(220, 80, 70), outfit_dark=(180, 55, 50),
               eye_color=(90, 60, 140), blush=(255, 160, 160),
               hairstyle="long", mouth="smile")

# Aria (alias explícito, mismo personaje)
draw_character("aria.png",
               skin=(255, 224, 196), hair=(150, 90, 55), hair_dark=(110, 62, 38),
               outfit=(220, 80, 70), outfit_dark=(180, 55, 50),
               eye_color=(90, 60, 140), blush=(255, 160, 160),
               hairstyle="long", mouth="smile")

# Guía — sereno: pelo verde, vestimenta verde, expresión neutral
draw_character("guide.png",
               skin=(245, 215, 185), hair=(60, 160, 110), hair_dark=(40, 120, 80),
               outfit=(50, 150, 120), outfit_dark=(38, 115, 92),
               eye_color=(40, 110, 90), blush=(200, 230, 200),
               hairstyle="short", mouth="neutral")

# Villano — pelo oscuro puntiagudo, vestimenta granate, sonrisa torcida
draw_character("villain.png",
               skin=(225, 200, 180), hair=(45, 40, 55), hair_dark=(25, 22, 32),
               outfit=(120, 30, 40), outfit_dark=(85, 20, 28),
               eye_color=(160, 40, 40), blush=(180, 140, 140),
               hairstyle="spiky", mouth="smirk")

print("Listo.")
