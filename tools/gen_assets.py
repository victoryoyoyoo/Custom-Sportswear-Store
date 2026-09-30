"""Draws every image in assets/ from scratch.

    python tools/gen_assets.py            # writes into ../assets
    python tools/gen_assets.py out_dir    # or somewhere else
    python tools/gen_assets.py out_dir tee hoodie   # just some products

Everything is vector-style drawing with Pillow: the store's own colourways and
basketball mark, no photos, no third-party logos. Shapes are drawn at 4x and
scaled down for smooth edges; product art is saved at 2x its design size as a
transparent cut-out, because the app lights it, shadows it and turns it round
itself. The balls and the cap have no artwork here: the app draws them in 3D.
"""
import math
import os
import random
import sys

from PIL import Image, ImageDraw, ImageFilter

OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), "..", "assets")
SS = 4         # drawing is done at SS x the design size, then scaled down
ART_SCALE = 2  # product art is saved at this multiple of its design size, so it stays sharp full screen

NAVY = (14, 23, 42)
ORANGE = (242, 106, 33)

# id, 中文名, English name, fabric, trim   (same table as Catalog.cpp)
COLORWAYS = [
    ("navy",    "午夜藍", "Midnight Navy",    "#1B2A4A", "#C9D3E3"),
    ("crimson", "烈焰紅", "Crimson Flame",    "#B3202A", "#F2B632"),
    ("teal",    "海港青", "Harbor Teal",      "#0F7C8C", "#E8F1F2"),
    ("royal",   "天際藍", "Royal Sky",        "#2F6FD6", "#FFFFFF"),
    ("jade",    "翡翠綠", "Jade Green",       "#146B45", "#D9B45A"),
    ("onyx",    "曜石黑", "Onyx Black",       "#1A1A1F", "#8C5BD6"),
    ("sand",    "沙丘金", "Desert Gold",      "#C08A3E", "#4A2E14"),
    ("violet",  "極光紫", "Aurora Violet",    "#5B2C8F", "#3FD2C7"),
    ("steel",   "鋼鐵灰", "Steel Grey",       "#4B5563", "#F97316"),
    ("sunset",  "日落橘", "Sunset Orange",    "#E8671C", "#1F1F1F"),
    ("indigo",  "月光靛", "Moonlight Indigo", "#2E3A87", "#F3E7C9"),
    ("cocoa",   "可可棕", "Cocoa Brown",      "#5A3A22", "#8FBF6A"),
]


# ============================================================== helpers
def rgb(h):
    h = h.lstrip("#")
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


def shade(c, f):
    """f < 1 darkens, f > 1 lightens (towards white)."""
    if f < 1:
        return tuple(int(v * f) for v in c)
    return tuple(int(v + (255 - v) * (f - 1)) for v in c)


def luminance(c):
    return 0.299 * c[0] + 0.587 * c[1] + 0.114 * c[2]


def bez(p0, p1, p2, n=28):
    """Quadratic Bezier as a point list."""
    return [((1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * p1[0] + t * t * p2[0],
             (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * p1[1] + t * t * p2[1])
            for t in (i / n for i in range(n + 1))]


def path(*segments):
    """Join point lists / single points into one outline."""
    out = []
    for seg in segments:
        pts = [seg] if isinstance(seg[0], (int, float)) else list(seg)
        if out and pts and out[-1] == pts[0]:
            pts = pts[1:]
        out += pts
    return out


class Canvas:
    """Supersampled RGBA canvas working in design units (w x h)."""

    def __init__(self, w, h):
        self.w, self.h = w, h
        self.img = Image.new("RGBA", (w * SS, h * SS), (0, 0, 0, 0))

    def P(self, pts):
        return [(x * SS, y * SS) for x, y in pts]

    def B(self, box):
        return tuple(v * SS for v in box)

    @property
    def draw(self):
        return ImageDraw.Draw(self.img)

    def layer(self):
        return Image.new("RGBA", self.img.size, (0, 0, 0, 0))

    def mask(self, pts=None, ellipse=None):
        m = Image.new("L", self.img.size, 0)
        d = ImageDraw.Draw(m)
        if pts is not None:
            d.polygon(self.P(pts), fill=255)
        if ellipse is not None:
            d.ellipse(self.B(ellipse), fill=255)
        return m

    def composite_clipped(self, layer, mask):
        clipped = self.layer()
        clipped.paste(layer, (0, 0), mask)
        self.img.alpha_composite(clipped)

    def shadow(self, pts=None, ellipse=None, dx=8, dy=12, blur=14, alpha=70):
        lay = self.layer()
        d = ImageDraw.Draw(lay)
        if pts is not None:
            d.polygon([(x + dx * SS, y + dy * SS) for x, y in self.P(pts)], fill=(0, 0, 0, alpha))
        if ellipse is not None:
            x0, y0, x1, y1 = ellipse
            d.ellipse(self.B((x0 + dx, y0 + dy, x1 + dx, y1 + dy)), fill=(0, 0, 0, alpha))
        self.img.alpha_composite(lay.filter(ImageFilter.GaussianBlur(blur * SS)))

    def light(self, mask, box, alpha=34, blur=40, colour=(255, 255, 255)):
        """Soft highlight (or shadow, with a dark colour) clipped to mask."""
        lay = self.layer()
        ImageDraw.Draw(lay).ellipse(self.B(box), fill=colour + (alpha,))
        self.composite_clipped(lay.filter(ImageFilter.GaussianBlur(blur * SS)), mask)

    def mesh(self, mask, base, step=7, alpha=26):
        """Athletic mesh: a staggered grid of tiny holes."""
        lay = self.layer()
        d = ImageDraw.Draw(lay)
        dark = shade(base, 0.55) if luminance(base) > 60 else shade(base, 1.35)
        r = 1.1 * SS
        for j, y in enumerate(range(0, self.h, step)):
            off = step / 2 if j % 2 else 0
            for x in range(0, self.w, step):
                cx, cy = (x + off) * SS, y * SS
                d.ellipse((cx - r, cy - r, cx + r, cy + r), fill=dark + (alpha,))
        self.composite_clipped(lay, mask)

    def stitch(self, pts, colour, dash=6, gap=4, width=1.4):
        """Dashed stitching line along a polyline."""
        d = self.draw
        pts = self.P(pts)
        on, left = True, dash * SS
        for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
            seg = math.hypot(x1 - x0, y1 - y0)
            pos = 0.0
            while pos < seg:
                step = min(left, seg - pos)
                if on:
                    a, b = pos / seg, (pos + step) / seg
                    d.line([(x0 + (x1 - x0) * a, y0 + (y1 - y0) * a), (x0 + (x1 - x0) * b, y0 + (y1 - y0) * b)],
                           fill=colour, width=max(1, int(width * SS)))
                pos += step
                left -= step
                if left <= 0:
                    on = not on
                    left = (dash if on else gap) * SS

    def line(self, pts, colour, width):
        self.draw.line(self.P(pts), fill=colour, width=int(width * SS), joint="curve")

    def poly(self, pts, colour):
        self.draw.polygon(self.P(pts), fill=colour)

    def ellipse(self, box, colour=None, outline=None, width=1):
        self.draw.ellipse(self.B(box), fill=colour, outline=outline, width=int(width * SS))

    def paste(self, img, xy):
        self.img.alpha_composite(img, (int(xy[0] * SS), int(xy[1] * SS)))

    def done(self):
        return self.img.resize((self.w * ART_SCALE, self.h * ART_SCALE), Image.LANCZOS)


def emblem(fabric, trim, size):
    """The store's mark: a basketball inside a ring, in the colourway's trim."""
    S = size * SS
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.ellipse((0, 0, S - 1, S - 1), fill=trim)
    m = S * 0.07
    d.ellipse((m, m, S - m, S - m), fill=fabric)
    m = S * 0.12
    d.ellipse((m, m, S - m, S - m), outline=trim, width=max(1, int(S * 0.014)))
    c, r = S / 2, S * 0.26
    w = max(1, int(S * 0.04))
    d.ellipse((c - r, c - r, c + r, c + r), outline=trim, width=w)
    d.line([(c, c - r), (c, c + r)], fill=trim, width=w)
    d.line([(c - r, c), (c + r, c)], fill=trim, width=w)
    d.arc((c - r * 1.9, c - r, c - r * 0.1, c + r), -52, 52, fill=trim, width=w)
    d.arc((c + r * 0.1, c - r, c + r * 1.9, c + r), 128, 232, fill=trim, width=w)
    return img.resize((size, size), Image.LANCZOS)


def on_white(img):
    bg = Image.new("RGBA", img.size, (255, 255, 255, 255))
    bg.alpha_composite(img)
    return bg.convert("RGB")


# ============================================================== products
# Each function returns an RGBA image. The C++ side prints names / numbers /
# embroidery onto some of them; those positions live in Catalog.cpp and use the
# same design units as the canvas sizes here.

def jersey(fabric, trim, front=False):
    """Basketball tank top (520 x 600): back view, or the front with a deeper neckline."""
    c = Canvas(520, 600)
    p, s = rgb(fabric), rgb(trim)
    dip = 118 if front else 72
    neck = bez((205, 30), (260, dip), (315, 30))
    arm_r = bez((370, 30), (382, 165), (448, 205))
    arm_l = bez((72, 205), (138, 165), (150, 30))
    hem = bez((452, 560), (260, 586), (68, 560))
    body = path((150, 30), neck, (370, 30), arm_r, (452, 560), hem, (72, 205), arm_l)

    c.poly(body, p)
    m = c.mask(body)
    c.mesh(m, p)
    # side panels in the trim colour, with a darker inner edge for depth
    for side in (-1, 1):
        x = 260 + side * 176
        c.poly([(x - 16 * side, 215), (x + 16 * side, 205), (x + 18 * side, 560), (x - 18 * side, 566)], s)
        c.poly([(x - 22 * side, 222), (x - 16 * side, 215), (x - 18 * side, 566), (x - 25 * side, 568)], shade(p, 0.72))
    # binding on neck and armholes
    for curve in (neck, arm_r, arm_l, [(150, 30), (205, 30)], [(315, 30), (370, 30)]):
        c.line(curve, s, 14)
    c.line(bez((70, 548), (260, 573), (450, 548)), shade(p, 0.78), 10)
    thread = shade(p, 1.45) if luminance(p) < 90 else shade(p, 0.6)
    c.stitch(bez((212, 44), (260, dip + 12), (308, 44)), thread)
    c.stitch(bez((74, 540), (260, 564), (446, 540)), thread)
    # light from the upper left, fabric folds, darker hem
    c.light(m, (140, 50, 340, 640), alpha=30, blur=40)
    c.light(m, (60, 380, 470, 700), alpha=40, blur=50, colour=(0, 0, 0))
    c.line(bez((180, 250), (200, 400), (170, 540)), shade(p, 0.9), 6)
    if front:
        c.paste(emblem(p, s, 30 * SS), (104, 496))   # small tag above the hem
    else:
        c.paste(emblem(p, s, 30 * SS), (245, 62))    # neck tag
    return c.done()


def shorts(fabric, trim):
    """Basketball shorts, front view (600 x 600)."""
    c = Canvas(600, 600)
    p, s = rgb(fabric), rgb(trim)
    body = path((130, 110), (470, 110), bez((470, 110), (492, 300), (512, 480)), (512, 480),
                bez((512, 480), (410, 506), (318, 490)), (318, 490), (300, 322), (282, 490),
                bez((282, 490), (190, 506), (88, 480)), bez((88, 480), (108, 300), (130, 110)))
    c.poly(body, p)
    m = c.mask(body)
    c.mesh(m, p)
    # side stripes
    for side in (-1, 1):
        if side < 0:
            stripe = [(130, 140), (156, 140), (130, 486), (100, 482)]
        else:
            stripe = [(444, 140), (470, 140), (500, 482), (470, 486)]
        c.poly(stripe, s)
    # waistband with drawcord
    band = [(126, 96), (474, 96), (476, 140), (124, 140)]
    c.poly(band, shade(p, 0.8))
    for y in range(102, 138, 6):
        c.line([(128, y), (472, y)], shade(p, 0.7), 1.2)
    c.line([(300, 140), (292, 200)], s, 5)
    c.line([(300, 140), (310, 196)], s, 5)
    c.ellipse((286, 196, 298, 208), s)
    c.ellipse((304, 192, 316, 204), s)
    # leg hems and centre seam
    c.line(bez((92, 466), (190, 490), (282, 476)), shade(p, 0.75), 9)
    c.line(bez((318, 476), (410, 490), (508, 466)), shade(p, 0.75), 9)
    thread = shade(p, 1.45) if luminance(p) < 90 else shade(p, 0.6)
    c.stitch([(300, 142), (300, 320)], thread)
    c.stitch(bez((96, 456), (190, 479), (280, 466)), thread)
    c.stitch(bez((320, 466), (410, 479), (504, 456)), thread)
    c.light(m, (120, 90, 330, 520), alpha=28, blur=45)
    c.light(m, (250, 300, 600, 700), alpha=36, blur=50, colour=(0, 0, 0))
    c.paste(emblem(p, s, 40 * SS), (400, 400))
    return c.done()


def cap(fabric, trim):
    """Six-panel baseball cap, three-quarter side view, bill to the right (520 x 420)."""
    c = Canvas(520, 420)
    p, s = rgb(fabric), rgb(trim)
    crown = path(bez((40, 266), (20, 58), (215, 50)), bez((215, 50), (385, 58), (392, 252)),
                 bez((392, 252), (215, 292), (40, 266)))
    c.poly(crown, p)
    m = c.mask(crown)
    c.mesh(m, p, step=6, alpha=16)
    # back closure opening
    c.poly(path(bez((52, 266), (60, 224), (104, 216)), bez((104, 216), (126, 250), (116, 276))), shade(p, 0.6))
    seam = shade(p, 0.72)
    for ctrl, end in (((140, 88), (112, 276)), ((305, 72), (338, 262)), ((238, 120), (232, 286))):
        c.line(bez((218, 52), ctrl, end), seam, 3)
    thread = shade(p, 1.45) if luminance(p) < 90 else shade(p, 0.62)
    for ctrl, end in (((150, 90), (124, 272)), ((296, 76), (328, 262))):
        c.stitch(bez((222, 58), ctrl, end), thread, dash=5, gap=4, width=1.1)
    c.light(m, (120, 58, 310, 190), alpha=44, blur=28)
    c.light(m, (200, 170, 420, 330), alpha=30, blur=30, colour=(0, 0, 0))
    c.line(bez((44, 264), (215, 290), (392, 250)), s, 7)
    # bill
    top = bez((300, 262), (425, 222), (508, 262))
    under = bez((508, 262), (470, 302), (322, 292))
    c.poly(path(top, under), shade(p, 0.9))
    c.line(under, shade(p, 0.5), 9)
    c.line(bez((322, 267), (425, 238), (494, 264)), s, 4)
    c.stitch(bez((330, 276), (425, 250), (486, 270)), thread, dash=5, gap=3, width=1)
    c.ellipse((206, 40, 230, 60), s)
    for ex, ey in ((128, 118), (322, 112)):
        c.ellipse((ex - 5, ey - 5, ex + 5, ey + 5), shade(p, 0.55))
    e = emblem(p, s, 110 * SS).resize((96 * SS, 110 * SS), Image.LANCZOS)
    c.paste(e, (250, 118))
    return c.done()


def sneaker(fabric, trim):
    """High-top basketball shoe, side view, toe to the right (640 x 480)."""
    c = Canvas(640, 480)
    p, s = rgb(fabric), rgb(trim)
    white, sole_dark = (246, 246, 244), (52, 52, 58)
    upper = path((96, 120), bez((96, 120), (150, 96), (206, 118)), bez((206, 118), (240, 170), (300, 176)),
                 bez((300, 176), (360, 200), (430, 262)), bez((430, 262), (560, 300), (586, 352)),
                 (590, 382), (80, 382), bez((80, 382), (70, 250), (96, 120)))
    c.shadow(upper, dx=6, dy=8, blur=10, alpha=50)
    c.poly(upper, p)
    m = c.mask(upper)
    c.mesh(m, p, step=6, alpha=14)
    # toe cap and heel counter, a shade darker
    c.poly(path(bez((470, 280), (560, 300), (586, 352)), (590, 382), (470, 382), bez((470, 382), (440, 330), (470, 280))),
           shade(p, 0.82))
    c.poly(path((80, 382), bez((80, 382), (72, 280), (92, 200)), bez((92, 200), (150, 250), (170, 382))), shade(p, 0.82))
    # sweeping side band (our own shape) in the trim colour
    band = path(bez((126, 334), (300, 262), (520, 318)), bez((520, 318), (528, 332), (506, 336)),
                bez((506, 336), (300, 290), (132, 352)))
    c.poly(band, s)
    c.line(bez((140, 368), (320, 316), (500, 356)), shade(s, 0.85) if luminance(s) > 60 else shade(s, 1.5), 3)
    # collar padding and heel pull tab
    c.line(bez((98, 124), (150, 102), (204, 122)), shade(p, 0.62), 14)
    c.poly([(84, 118), (104, 110), (100, 190), (80, 198)], s)
    # lace area: tongue, eyelets, laces
    c.poly(path(bez((206, 118), (225, 88), (262, 96)), bez((262, 96), (280, 150), (300, 176)),
                bez((300, 176), (250, 170), (206, 118))), shade(p, 0.9))
    lace_line = bez((222, 150), (320, 200), (420, 262), n=6)
    lace = s if luminance(s) > 80 else white
    for i, (x, y) in enumerate(lace_line[:-1]):
        c.ellipse((x - 5, y - 5, x + 5, y + 5), shade(p, 0.45))
        nx, ny = lace_line[i + 1]
        c.line([(x - 10, y + 8), (nx + 10, ny - 8)], lace, 5)
    # midsole with a trim accent line, outsole
    mid = path((70, 372), (604, 372), bez((604, 372), (618, 392), (598, 412)), (86, 412),
               bez((86, 412), (60, 400), (70, 372)))
    c.poly(mid, white)
    c.line([(84, 392), (596, 392)], s if luminance(s) < 200 else shade(p, 1.0), 4)
    c.poly(path((86, 412), (598, 412), bez((598, 412), (590, 428), (570, 430)), (100, 430),
                bez((100, 430), (80, 426), (86, 412))), sole_dark)
    for x in range(110, 580, 22):
        c.line([(x, 416), (x + 10, 428)], (70, 70, 78), 2)
    thread = shade(p, 1.45) if luminance(p) < 90 else shade(p, 0.62)
    c.stitch(bez((96, 300), (300, 230), (470, 290)), thread, dash=5, gap=4, width=1)
    c.stitch(bez((476, 290), (446, 330), (474, 376)), thread, dash=5, gap=4, width=1)
    c.light(m, (150, 90, 420, 260), alpha=36, blur=35)
    c.light(m, (260, 260, 640, 460), alpha=30, blur=35, colour=(0, 0, 0))
    c.paste(emblem(p, s, 42 * SS), (116, 212))
    return c.done()


def socks(fabric, trim):
    """Pair of crew socks, side view (560 x 560)."""
    c = Canvas(560, 560)
    p, s = rgb(fabric), rgb(trim)

    def one_sock(ox, oy, tone):
        body = path((ox + 70, oy + 40), (ox + 190, oy + 40), (ox + 190, oy + 300),
                    bez((ox + 190, oy + 300), (ox + 220, oy + 330), (ox + 330, oy + 350)),
                    bez((ox + 330, oy + 350), (ox + 380, oy + 380), (ox + 350, oy + 420)),
                    (ox + 150, oy + 420), bez((ox + 150, oy + 420), (ox + 60, oy + 420), (ox + 70, oy + 330)))
        c.shadow(body, dx=6, dy=8, blur=10, alpha=45)
        c.poly(body, tone)
        m = c.mask(body)
        # ribbed cuff with two trim stripes
        c.poly([(ox + 70, oy + 40), (ox + 190, oy + 40), (ox + 190, oy + 120), (ox + 70, oy + 120)], shade(tone, 0.92))
        for y in (oy + 70, oy + 96):
            c.poly([(ox + 70, y), (ox + 190, y), (ox + 190, y + 12), (ox + 70, y + 12)], s)
        for x in range(ox + 78, ox + 190, 9):
            c.line([(x, oy + 42), (x, oy + 118)], shade(tone, 0.8), 1.3)
        # heel and toe patches
        c.poly(path(bez((ox + 70, oy + 330), (ox + 66, oy + 400), (ox + 150, oy + 420)), (ox + 150, oy + 380),
                    bez((ox + 150, oy + 380), (ox + 100, oy + 370), (ox + 104, oy + 320))), s)
        c.poly(path(bez((ox + 300, oy + 346), (ox + 380, oy + 380), (ox + 350, oy + 420)), (ox + 290, oy + 420),
                    bez((ox + 290, oy + 420), (ox + 300, oy + 380), (ox + 290, oy + 346))), s)
        c.light(m, (ox + 40, oy + 20, ox + 160, oy + 380), alpha=30, blur=30)
        c.light(m, (ox + 150, oy + 250, ox + 420, oy + 500), alpha=30, blur=30, colour=(0, 0, 0))
        return m

    one_sock(150, 40, shade(p, 0.82))
    one_sock(60, 70, p)
    c.paste(emblem(p, s, 44 * SS), (158, 290))
    return c.done()


def backpack(fabric, trim):
    """Backpack, front view (520 x 600)."""
    c = Canvas(520, 600)
    p, s = rgb(fabric), rgb(trim)
    # top handle
    c.line(bez((220, 90), (260, 40), (300, 90)), shade(p, 0.6), 12)
    body = path(bez((140, 150), (140, 70), (260, 70)), bez((260, 70), (380, 70), (380, 150)),
                (388, 530), bez((388, 530), (260, 552), (132, 530)))
    c.poly(body, p)
    m = c.mask(body)
    c.mesh(m, p, step=5, alpha=12)
    # front pocket in the trim colour
    pocket = path((170, 330), (350, 330), bez((350, 330), (366, 330), (366, 346)), (366, 496),
                  bez((366, 496), (260, 512), (154, 496)), (154, 346), bez((154, 346), (154, 330), (170, 330)))
    c.shadow(pocket, dx=0, dy=4, blur=5, alpha=50)
    c.poly(pocket, s)
    pm = c.mask(pocket)
    c.light(pm, (150, 300, 300, 480), alpha=30, blur=25)
    # zips with pulls
    zip_c = shade(p, 0.45)
    c.line(bez((150, 160), (260, 110), (370, 160)), zip_c, 5)
    c.line([(170, 350), (350, 350)], shade(s, 0.6) if luminance(s) > 60 else shade(s, 1.8), 4)
    for x, y in ((340, 146), (340, 350)):
        c.poly([(x - 5, y), (x + 5, y), (x + 4, y + 28), (x - 4, y + 28)], (200, 200, 205))
    thread = shade(p, 1.45) if luminance(p) < 90 else shade(p, 0.62)
    c.stitch(bez((150, 176), (260, 128), (370, 176)), thread)
    c.stitch(path((160, 340), (160, 492)), thread)
    c.stitch(path((360, 340), (360, 492)), thread)
    c.light(m, (130, 60, 320, 380), alpha=32, blur=40)
    c.light(m, (240, 300, 480, 640), alpha=36, blur=45, colour=(0, 0, 0))
    c.paste(emblem(p, s, 70 * SS), (225, 214))
    return c.done()


def tee(fabric, trim, front=True):
    """Short-sleeve training tee (520 x 600): front with a crew neck, or the back."""
    c = Canvas(520, 600)
    p, s = rgb(fabric), rgb(trim)
    dip = 96 if front else 62
    neck = bez((196, 44), (260, dip), (324, 44))
    body = path((196, 44), neck, (324, 44), bez((324, 44), (372, 52), (408, 70)),
                (486, 186), (420, 226), (410, 206), bez((410, 206), (402, 400), (420, 572)),
                bez((420, 572), (260, 588), (100, 572)), bez((100, 572), (118, 400), (110, 206)),
                (100, 226), (34, 186), (112, 70), bez((112, 70), (148, 52), (196, 44)))
    c.poly(body, p)
    m = c.mask(body)
    # cotton jersey knit: very fine horizontal courses instead of the mesh
    lay = c.layer()
    d = ImageDraw.Draw(lay)
    dark = shade(p, 0.6) if luminance(p) > 60 else shade(p, 1.4)
    for y in range(40, 590, 3):
        d.line([(0, y * SS), (c.w * SS, y * SS)], fill=dark + (10,), width=1)
    c.composite_clipped(lay, m)
    # sleeve cuffs in the trim colour
    c.line([(40, 180), (104, 218)], s, 12)
    c.line([(480, 180), (416, 218)], s, 12)
    # rib collar
    c.line(neck, s, 16)
    c.line(bez((200, 52), (260, dip + 8), (320, 52)), shade(s, 0.8) if luminance(s) > 70 else shade(s, 1.4), 2)
    thread = shade(p, 1.45) if luminance(p) < 90 else shade(p, 0.6)
    c.stitch(bez((104, 560), (260, 576), (416, 560)), thread)
    c.stitch([(114, 212), (110, 70)], shade(p, 0.85), dash=4, gap=3, width=1)
    c.stitch([(406, 212), (410, 70)], shade(p, 0.85), dash=4, gap=3, width=1)
    # soft drape and light from the upper left
    c.line(bez((160, 300), (176, 430), (150, 560)), shade(p, 0.92), 8)
    c.line(bez((372, 330), (360, 450), (380, 560)), shade(p, 0.93), 6)
    c.light(m, (130, 60, 330, 620), alpha=30, blur=45)
    c.light(m, (60, 400, 480, 720), alpha=36, blur=50, colour=(0, 0, 0))
    if front:
        c.paste(emblem(p, s, 30 * SS), (380, 510))
    else:
        c.paste(emblem(p, s, 26 * SS), (247, 66))
    return c.done()


def hoodie(fabric, trim, front=True):
    """Pullover hoodie (560 x 640): front with pocket and drawcords, or the back with the hood."""
    c = Canvas(560, 640)
    p, s = rgb(fabric), rgb(trim)
    rib = shade(p, 0.82)
    body = path((196, 70), bez((196, 70), (280, 92), (364, 70)), bez((364, 70), (430, 80), (462, 120)),
                bez((462, 120), (520, 300), (528, 540)), (478, 548), bez((478, 548), (462, 380), (446, 250)),
                (440, 560), (120, 560), (114, 250), bez((114, 250), (98, 380), (82, 548)), (32, 540),
                bez((32, 540), (40, 300), (98, 120)), bez((98, 120), (130, 80), (196, 70)))
    c.poly(body, p)
    m = c.mask(body)
    # brushed fleece: soft speckle
    rnd = random.Random(11)
    lay = c.layer()
    d = ImageDraw.Draw(lay)
    dark = shade(p, 0.55) if luminance(p) > 60 else shade(p, 1.35)
    for _ in range(16000):
        x, y = rnd.uniform(30, 530) * SS, rnd.uniform(60, 600) * SS
        d.point((x, y), fill=dark + (40,))
    c.composite_clipped(lay, m)
    # ribbed cuffs and waistband
    for box in ([(32, 540), (82, 548), (82, 590), (30, 584)], [(478, 548), (528, 540), (530, 584), (478, 590)],
                [(120, 556), (440, 556), (440, 600), (120, 600)]):
        c.poly(box, rib)
    for x in range(124, 440, 7):
        c.line([(x, 560), (x, 598)], shade(p, 0.72), 1.2)
    # set-in sleeve seams
    seam = shade(p, 0.78)
    c.line(bez((150, 84), (118, 170), (114, 250)), seam, 3)
    c.line(bez((410, 84), (442, 170), (446, 250)), seam, 3)
    if front:
        # hood opening lying around the neck, with the lining showing
        hood = path(bez((180, 76), (190, 20), (280, 14)), bez((280, 14), (370, 20), (380, 76)),
                    bez((380, 76), (330, 150), (280, 158)), bez((280, 158), (230, 150), (180, 76)))
        c.poly(hood, shade(p, 0.9))
        c.poly(path(bez((206, 70), (230, 40), (280, 38)), bez((280, 38), (330, 40), (354, 70)),
                    bez((354, 70), (320, 128), (280, 132)), bez((280, 132), (240, 128), (206, 70))), shade(p, 0.5))
        c.line(bez((180, 76), (230, 150), (280, 158)), s, 6)
        c.line(bez((380, 76), (330, 150), (280, 158)), s, 6)
        # drawcords with metal tips
        for x0, x1 in ((252, 244), (308, 318)):
            c.line(bez((x0, 140), (x0 - 4 + (x1 - x0), 210), (x1, 262)), s, 6)
            c.poly([(x1 - 4, 260), (x1 + 4, 260), (x1 + 4, 282), (x1 - 4, 282)], (200, 200, 205))
        # kangaroo pocket
        pocket = path((170, 380), (390, 380), (430, 520), (130, 520))
        c.shadow(pocket, dx=0, dy=3, blur=4, alpha=40)
        c.poly(pocket, shade(p, 0.96))
        thread = shade(p, 1.45) if luminance(p) < 90 else shade(p, 0.62)
        c.stitch([(172, 388), (136, 514)], thread)
        c.stitch([(388, 388), (424, 514)], thread)
        c.stitch([(176, 386), (384, 386)], thread)
        c.paste(emblem(p, s, 30 * SS), (382, 470))
    else:
        # the hood lies flat down the back
        hood = path(bez((170, 72), (160, 250), (280, 300)), bez((280, 300), (400, 250), (390, 72)),
                    bez((390, 72), (280, 96), (170, 72)))
        c.shadow(hood, dx=0, dy=6, blur=8, alpha=50)
        c.poly(hood, shade(p, 0.95))
        c.line(bez((280, 92), (284, 200), (280, 300)), seam, 3)
        c.line(bez((170, 72), (280, 96), (390, 72)), s, 6)
        c.paste(emblem(p, s, 24 * SS), (268, 104))
    c.light(m, (100, 60, 330, 620), alpha=30, blur=50)
    c.light(m, (240, 360, 600, 720), alpha=38, blur=55, colour=(0, 0, 0))
    return c.done()


def towel(fabric, trim):
    """Sports towel hanging from its loop (480 x 640)."""
    c = Canvas(480, 640)
    p, s = rgb(fabric), rgb(trim)
    c.line(bez((222, 70), (240, 20), (258, 70)), shade(p, 0.6), 8)
    body = path(bez((96, 90), (96, 70), (116, 70)), (364, 70), bez((364, 70), (384, 70), (384, 90)),
                bez((384, 90), (392, 330), (378, 590)), bez((378, 590), (318, 606), (240, 596)),
                bez((240, 596), (162, 606), (102, 590)), bez((102, 590), (88, 330), (96, 90)))
    c.poly(body, p)
    m = c.mask(body)
    # terry loops
    rnd = random.Random(5)
    lay = c.layer()
    d = ImageDraw.Draw(lay)
    for _ in range(14000):
        x, y = rnd.uniform(90, 390) * SS, rnd.uniform(66, 606) * SS
        d.line([(x, y), (x + 1.2 * SS, y + 2.6 * SS)], fill=(0, 0, 0, 20), width=SS)
    c.composite_clipped(lay, m)
    # woven dobby border near the bottom, in the trim colour (text goes here)
    band = path((96, 470), bez((96, 470), (240, 478), (386, 470)), (386, 538),
                bez((386, 538), (240, 546), (94, 538)))
    c.poly(band, s)
    edge = shade(s, 0.75) if luminance(s) > 70 else shade(s, 1.5)
    c.line(bez((96, 478), (240, 486), (386, 478)), edge, 2)
    c.line(bez((96, 530), (240, 538), (386, 530)), edge, 2)
    # hemmed sides and a gentle fold
    c.line(bez((100, 92), (92, 330), (104, 588)), shade(p, 0.8), 4)
    c.line(bez((380, 92), (388, 330), (376, 588)), shade(p, 0.8), 4)
    c.line(bez((96, 76), (240, 72), (384, 76)), shade(p, 0.8), 6)
    c.line(bez((250, 120), (236, 300), (258, 460)), shade(p, 0.92), 10)
    c.light(m, (80, 40, 300, 500), alpha=34, blur=45)
    c.light(m, (200, 300, 480, 700), alpha=34, blur=50, colour=(0, 0, 0))
    c.paste(emblem(p, s, 40 * SS), (322, 104))
    return c.done()


def backpack_back(fabric, trim):
    """The same backpack from behind: padded back panel and shoulder straps (520 x 600)."""
    c = Canvas(520, 600)
    p, s = rgb(fabric), rgb(trim)
    c.line(bez((220, 90), (260, 40), (300, 90)), shade(p, 0.6), 12)
    body = path(bez((140, 150), (140, 70), (260, 70)), bez((260, 70), (380, 70), (380, 150)),
                (388, 530), bez((388, 530), (260, 552), (132, 530)))
    c.poly(body, p)
    m = c.mask(body)
    # quilted air-mesh back panel
    panel = path(bez((166, 140), (166, 112), (194, 112)), (326, 112), bez((326, 112), (354, 112), (354, 140)),
                 (358, 470), bez((358, 470), (260, 486), (162, 470)))
    c.poly(panel, shade(p, 0.84))
    pm = c.mask(panel)
    c.mesh(pm, shade(p, 0.84), step=5, alpha=30)
    for y in (200, 290, 380):
        c.line([(168, y), (356, y + 2)], shade(p, 0.66), 3)
    c.line([(260, 116), (260, 480)], shade(p, 0.66), 3)
    # shoulder straps with trim-coloured adjusters and webbing
    strap = shade(p, 0.55)
    for side in (-1, 1):
        x = 260 + side * 62
        pts = bez((x, 96), (x + side * 40, 280), (x + side * 26, 470))
        c.line(pts, strap, 44)
        c.line(bez((x + side * 22, 470), (x + side * 40, 506), (x + side * 70, 520)), shade(p, 0.4), 10)
        c.poly([(x + side * 16 - 16, 420), (x + side * 16 + 16, 420), (x + side * 16 + 16, 440), (x + side * 16 - 16, 440)], s)
        thread = shade(strap, 1.8) if luminance(strap) < 90 else shade(strap, 0.6)
        c.stitch(bez((x - 14, 110), (x + side * 40 - 14, 280), (x + side * 26 - 14, 456)), thread, dash=5, gap=4, width=1)
        c.stitch(bez((x + 14, 110), (x + side * 40 + 14, 280), (x + side * 26 + 14, 456)), thread, dash=5, gap=4, width=1)
    # sternum strap
    c.line([(214, 250), (306, 250)], shade(p, 0.4), 8)
    c.poly([(248, 242), (272, 242), (272, 258), (248, 258)], s)
    c.light(m, (130, 60, 320, 380), alpha=26, blur=40)
    c.light(m, (240, 300, 480, 640), alpha=36, blur=45, colour=(0, 0, 0))
    return c.done()


def band_base(fabric, trim, x0, x1, top, bottom, ry, stripes, loops=9000, seed=3):
    """Round terry band seen a little from above. No logo: the app prints it
    and turns it round the band."""
    c = Canvas(x1 + x0, bottom + int(ry * 2.2) + 30)
    p, s = rgb(fabric), rgb(trim)
    cx = (x0 + x1) / 2
    body = path(bez((x0, top), (cx, top + ry * 1.9), (x1, top)), (x1, bottom),
                bez((x1, bottom), (cx, bottom + ry * 1.9), (x0, bottom)))
    c.poly(body, p)
    m = c.mask(body)
    rnd = random.Random(seed)
    lay = c.layer()
    d = ImageDraw.Draw(lay)
    for _ in range(loops):
        x, y = rnd.uniform(x0, x1) * SS, rnd.uniform(top, bottom + ry * 2) * SS
        d.line([(x, y), (x + 1.5 * SS, y + 3 * SS)], fill=(0, 0, 0, 22), width=SS)
    c.composite_clipped(lay, m)
    for y in stripes:
        c.poly(path(bez((x0, y), (cx, y + ry * 1.9), (x1, y)), (x1, y + 12),
                    bez((x1, y + 12), (cx, y + 12 + ry * 1.9), (x0, y + 12))), s)
    c.ellipse((x0, top - ry, x1, top + ry), shade(p, 0.85))
    c.ellipse((x0 + 26, top - ry + 12, x1 - 26, top + ry - 12), shade(p, 0.5))
    # cylinder shading: lit from the left, turning away on the right
    c.light(m, (x0 - 40, top - 60, x0 + (x1 - x0) * 0.5, bottom + ry * 3), alpha=40, blur=30)
    c.light(m, (x0 + (x1 - x0) * 0.62, top - 80, x1 + 90, bottom + ry * 3), alpha=46, blur=34, colour=(0, 0, 0))
    return c.done()


def wristband(fabric, trim):
    """Terry wristband, slightly from above (520 x 440 canvas)."""
    return band_base(fabric, trim, 90, 430, 140, 250, 50, (158, 220))


def headband(fabric, trim):
    """Wide terry headband (520 x 360 canvas)."""
    return band_base(fabric, trim, 40, 480, 120, 190, 64, (124, 174), loops=12000, seed=9)


def bottle(fabric, trim):
    """Squeeze sports bottle, 750 ml (360 x 640). The print is added by the app."""
    c = Canvas(360, 640)
    p, s = rgb(fabric), rgb(trim)
    cx, r, top, bottom, ry = 180, 104, 190, 590, 16
    body = path((cx - r, top), bez((cx - r, top), (cx, top - 2 * ry), (cx + r, top)),
                (cx + r, 250), bez((cx + r, 250), (cx + r - 12, 300), (cx + r, 350)), (cx + r, bottom),
                bez((cx + r, bottom), (cx, bottom + 2 * ry), (cx - r, bottom)), (cx - r, 350),
                bez((cx - r, 350), (cx - r + 12, 300), (cx - r, 250)))
    c.poly(body, p)
    m = c.mask(body)
    # grip rings at the waist
    for y in (272, 300, 328):
        c.line(bez((cx - r + 8, y), (cx, y + 2 * ry), (cx + r - 8, y)), shade(p, 0.8), 3)
    # base cup
    c.poly(path((cx - r, 548), bez((cx - r, 548), (cx, 548 + 2 * ry), (cx + r, 548)), (cx + r, bottom),
                bez((cx + r, bottom), (cx, bottom + 2 * ry), (cx - r, bottom))), shade(p, 0.75))
    # cylinder shading: highlight strip on the left, falling off to the right
    c.light(m, (cx - r * 0.95, top - 40, cx - r * 0.35, bottom + 40), alpha=70, blur=14)
    c.light(m, (cx + r * 0.35, top - 60, cx + r * 1.6, bottom + 60), alpha=60, blur=22, colour=(0, 0, 0))
    # lid, spout and flip cap in the trim colour
    lid = s
    lid_dark = shade(s, 0.7) if luminance(s) > 60 else shade(s, 1.6)
    c.poly(path((cx - r + 6, top - 8), bez((cx - r + 6, top - 8), (cx, top - 8 + 2 * ry), (cx + r - 6, top - 8)),
                (cx + r - 6, top - 64), (cx - r + 6, top - 64)), lid)
    for x in range(int(cx - r + 14), int(cx + r - 6), 10):
        c.line([(x, top - 60), (x, top - 14)], lid_dark, 2)
    c.ellipse((cx - r + 6, top - 64 - ry, cx + r - 6, top - 64 + ry), shade(lid, 0.9) if luminance(lid) > 60 else shade(lid, 1.3))
    c.poly([(cx - 26, top - 70), (cx + 26, top - 70), (cx + 16, top - 118), (cx - 16, top - 118)], lid_dark)
    c.ellipse((cx - 16, top - 124, cx + 16, top - 112), lid_dark)
    c.line(bez((cx + 30, top - 72), (cx + 66, top - 140), (cx + 20, top - 150)), lid, 10)
    lm = c.mask(path((cx - r + 6, top - 64), (cx + r - 6, top - 64), (cx + r - 6, top), (cx - r + 6, top)))
    c.light(lm, (cx - r, top - 90, cx - 20, top + 10), alpha=50, blur=12)
    c.light(lm, (cx + 30, top - 90, cx + r + 40, top + 10), alpha=40, blur=14, colour=(0, 0, 0))
    return c.done()


# Artwork the app turns round in its 360° view, saved as transparent cut-outs
# (the app adds the backdrop, shadow and lighting). Basketball, football and
# cap are not here: the app draws those in 3D and only needs the emblem.
PRODUCTS = {
    "jersey": jersey,
    "jersey_front": lambda fabric, trim: jersey(fabric, trim, front=True),
    "tee": tee,
    "tee_back": lambda fabric, trim: tee(fabric, trim, front=False),
    "hoodie": hoodie,
    "hoodie_back": lambda fabric, trim: hoodie(fabric, trim, front=False),
    "shorts": shorts,
    "socks": socks,
    "sneaker": sneaker,
    "headband": headband,
    "wristband": wristband,
    "backpack": backpack,
    "backpack_back": backpack_back,
    "bottle": bottle,
    "towel": towel,
}


# ============================================================== brand art
def app_logo():
    S = 256 * SS
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.rounded_rectangle((0, 0, S - 1, S - 1), int(S * 0.22), fill=NAVY)
    d.ellipse((int(S * .2), int(S * .2), int(S * .8), int(S * .8)), fill=ORANGE)
    w = int(S * 0.025)
    dark = (120, 45, 10)
    d.line([(S * .5, S * .2), (S * .5, S * .8)], fill=dark, width=w)
    d.line([(S * .2, S * .5), (S * .8, S * .5)], fill=dark, width=w)
    d.arc((int(S * .02), int(S * .2), int(S * .5), int(S * .8)), -60, 60, fill=dark, width=w)
    d.arc((int(S * .5), int(S * .2), int(S * .98), int(S * .8)), 120, 240, fill=dark, width=w)
    return img.resize((256, 256), Image.LANCZOS)


def main():
    os.makedirs(OUT, exist_ok=True)
    only = set(sys.argv[2:])  # optional: limit to some products (or "emblem") while tweaking
    for name, fn in PRODUCTS.items():
        if only and name not in only:
            continue
        for cid, _, _, fabric, trim in COLORWAYS:
            fn(fabric, trim).save(f"{OUT}/{name}_{cid}.png", optimize=True)
    if not only or "emblem" in only:
        for cid, _, _, fabric, trim in COLORWAYS:
            emblem(rgb(fabric), rgb(trim), 256).save(f"{OUT}/emblem_{cid}.png", optimize=True)
    if not only:
        logo = app_logo()
        logo.save(f"{OUT}/app_logo.png")
        logo.save(f"{OUT}/app.ico", sizes=[(16, 16), (32, 32), (48, 48), (256, 256)])
    print("wrote", OUT)


if __name__ == "__main__":
    main()
