#include "Showcase.h"
#include "Personalizer.h"
#include "Theme.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace Showcase {

namespace {
    constexpr double kPi = 3.14159265358979323846;

    struct Vec {
        double x, y, z;
        Vec operator+(const Vec& o) const { return { x + o.x, y + o.y, z + o.z }; }
        Vec operator-(const Vec& o) const { return { x - o.x, y - o.y, z - o.z }; }
        Vec operator*(double k) const { return { x * k, y * k, z * k }; }
        double Dot(const Vec& o) const { return x * o.x + y * o.y + z * o.z; }
        Vec Cross(const Vec& o) const { return { y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x }; }
        Vec Normalized() const { const double l = std::sqrt(Dot(*this)); return l > 0 ? *this * (1.0 / l) : *this; }
    };

    struct Camera {
        double spinCos, spinSin, tiltCos, tiltSin;
        Camera(double spin, double tilt)
            : spinCos(std::cos(spin)), spinSin(std::sin(spin)), tiltCos(std::cos(tilt)), tiltSin(std::sin(tilt)) {}
        Vec ToObject(const Vec& v) const {
            const double y = tiltCos * v.y + tiltSin * v.z, z = -tiltSin * v.y + tiltCos * v.z;
            return { spinCos * v.x - spinSin * z, y, spinSin * v.x + spinCos * z };
        }
        Vec ToView(const Vec& o) const {
            const double x = spinCos * o.x + spinSin * o.z, z = -spinSin * o.x + spinCos * o.z;
            return { x, tiltCos * o.y - tiltSin * z, tiltSin * o.y + tiltCos * z };
        }
    };

    const Vec kLight = Vec{ -0.42, 0.62, 0.66 }.Normalized();

    double Clamp01(double v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
    double Smooth(double e0, double e1, double v) { const double t = Clamp01((v - e0) / (e1 - e0)); return t * t * (3 - 2 * t); }

    struct Colour {
        double r, g, b;
        Colour operator*(double k) const { return { r * k, g * k, b * k }; }
        Colour operator+(const Colour& o) const { return { r + o.r, g + o.g, b + o.b }; }
        static Colour From(const wxColour& c) { return { (double)c.Red(), (double)c.Green(), (double)c.Blue() }; }
        Colour Mix(const Colour& o, double t) const { return *this * (1 - t) + o * t; }
    };

    inline void Over(uint8_t* dst, double r, double g, double b, double a) {
        const double k = 1.0 - a / 255.0;
        dst[0] = (uint8_t)std::min(255.0, r + dst[0] * k + 0.5);
        dst[1] = (uint8_t)std::min(255.0, g + dst[1] * k + 0.5);
        dst[2] = (uint8_t)std::min(255.0, b + dst[2] * k + 0.5);
        dst[3] = (uint8_t)std::min(255.0, a + dst[3] * k + 0.5);
    }

    inline void Sample(const Pixels& p, double x, double y, double out[4]) {
        const int x0 = (int)std::floor(x), y0 = (int)std::floor(y);
        const double fx = x - x0, fy = y - y0;
        out[0] = out[1] = out[2] = out[3] = 0;
        for (int j = 0; j < 2; ++j) {
            const int yy = y0 + j;
            if (yy < 0 || yy >= p.height) continue;
            const double wy = j ? fy : 1 - fy;
            for (int i = 0; i < 2; ++i) {
                const int xx = x0 + i;
                if (xx < 0 || xx >= p.width) continue;
                const double w = wy * (i ? fx : 1 - fx);
                const uint8_t* s = p.At(xx, yy);
                out[0] += s[0] * w; out[1] += s[1] * w; out[2] += s[2] * w; out[3] += s[3] * w;
            }
        }
    }

    inline void Nearest(const Pixels& p, double x, double y, double out[4]) {
        const int xx = (int)std::lround(x), yy = (int)std::lround(y);
        if (xx < 0 || yy < 0 || xx >= p.width || yy >= p.height) { out[0] = out[1] = out[2] = out[3] = 0; return; }
        const uint8_t* s = p.At(xx, yy);
        out[0] = s[0]; out[1] = s[1]; out[2] = s[2]; out[3] = s[3];
    }

    struct Taps { int first; std::vector<double> weights; };

    std::vector<Taps> MakeTaps(int from, int to) {
        const double scale = (double)from / to, radius = std::max(1.0, scale);
        std::vector<Taps> taps(to);
        for (int i = 0; i < to; ++i) {
            const double centre = (i + 0.5) * scale - 0.5;
            const int lo = (int)std::ceil(centre - radius), hi = (int)std::floor(centre + radius);
            taps[i].first = lo;
            double sum = 0;
            for (int j = lo; j <= hi; ++j) {
                const double w = std::max(0.0, 1.0 - std::abs(j - centre) / radius);
                taps[i].weights.push_back(w);
                sum += w;
            }
            for (double& w : taps[i].weights) w /= (sum > 0 ? sum : 1);
        }
        return taps;
    }

    Pixels Resample(const Pixels& src, int width, int height) {
        width = std::max(1, width);
        height = std::max(1, height);
        const auto tx = MakeTaps(src.width, width), ty = MakeTaps(src.height, height);
        std::vector<double> row((size_t)width * src.height * 4, 0.0);
        for (int y = 0; y < src.height; ++y)
            for (int x = 0; x < width; ++x) {
                double* d = &row[((size_t)y * width + x) * 4];
                for (size_t k = 0; k < tx[x].weights.size(); ++k) {
                    const int sx = std::clamp(tx[x].first + (int)k, 0, src.width - 1);
                    const uint8_t* s = src.At(sx, y);
                    const double w = tx[x].weights[k];
                    d[0] += s[0] * w; d[1] += s[1] * w; d[2] += s[2] * w; d[3] += s[3] * w;
                }
            }
        Pixels out;
        out.width = width;
        out.height = height;
        out.rgba.resize((size_t)width * height * 4);
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x) {
                double acc[4] = { 0, 0, 0, 0 };
                for (size_t k = 0; k < ty[y].weights.size(); ++k) {
                    const int sy = std::clamp(ty[y].first + (int)k, 0, src.height - 1);
                    const double* s = &row[((size_t)sy * width + x) * 4];
                    const double w = ty[y].weights[k];
                    acc[0] += s[0] * w; acc[1] += s[1] * w; acc[2] += s[2] * w; acc[3] += s[3] * w;
                }
                uint8_t* d = out.At(x, y);
                for (int c = 0; c < 4; ++c) d[c] = (uint8_t)std::clamp(acc[c] + 0.5, 0.0, 255.0);
            }
        return out;
    }

    Pixels LoadFitted(const wxString& fileName, double maxW, double maxH) {
        wxImage image(Theme::AssetPath(fileName), wxBITMAP_TYPE_PNG);
        if (!image.IsOk()) return Pixels();
        const double k = std::min(maxW / image.GetWidth(), maxH / image.GetHeight());
        return Resample(Pixels::FromImage(image), (int)std::lround(image.GetWidth() * k),
                        (int)std::lround(image.GetHeight() * k));
    }

    Pixels LoadDecal(const wxString& fileName) {
        wxImage image(Theme::AssetPath(fileName), wxBITMAP_TYPE_PNG);
        return image.IsOk() ? Pixels::FromImage(image) : Pixels();
    }

    void Print(Pixels& face, const Personalizer* personalizer, const Colorway& colorway, int side) {
        if (!personalizer || !face.IsOk()) return;
        wxImage image = face.ToImage();
        {
            std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(image));
            if (!gc) return;
            gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
            personalizer->Draw(gc.get(), wxRect2DDouble(0, 0, face.width, face.height), colorway, side);
        }
        face = Pixels::FromImage(image);
    }

    Pixels TextDecal(const Personalizer* personalizer, const wxSize& pixels, const wxColour& fill, const wxColour& outline) {
        const wxString text = personalizer ? personalizer->PrintText() : wxString();
        if (text.IsEmpty() || pixels.x < 8 || pixels.y < 4) return Pixels();
        return Pixels::FromImage(Personalizer::TextDecal(text, pixels, fill, outline));
    }

    wxColour Darker(const wxColour& c, double k) {
        return wxColour((unsigned char)(c.Red() * k), (unsigned char)(c.Green() * k), (unsigned char)(c.Blue() * k));
    }

    struct Bounds {
        int left = INT_MAX, top = INT_MAX, right = -1, bottom = -1;
        void Add(int x, int y) {
            left = std::min(left, x); right = std::max(right, x);
            top = std::min(top, y); bottom = std::max(bottom, y);
        }
        wxRect Rect() const { return right < 0 ? wxRect() : wxRect(left, top, right - left + 1, bottom - top + 1); }
    };

    double Hash(int x, int y, int z) {
        uint32_t h = (uint32_t)x * 73856093u ^ (uint32_t)y * 19349663u ^ (uint32_t)z * 83492791u;
        h = (h ^ (h >> 13)) * 1274126177u;
        return (h & 0xFFFF) / 65535.0;
    }

    double Noise(const Vec& p, double frequency) {
        const double x = p.x * frequency, y = p.y * frequency, z = p.z * frequency;
        const int ix = (int)std::floor(x), iy = (int)std::floor(y), iz = (int)std::floor(z);
        double fx = x - ix, fy = y - iy, fz = z - iz;
        fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy); fz = fz * fz * (3 - 2 * fz);
        double v = 0;
        for (int dz = 0; dz < 2; ++dz)
            for (int dy = 0; dy < 2; ++dy)
                for (int dx = 0; dx < 2; ++dx)
                    v += (dx ? fx : 1 - fx) * (dy ? fy : 1 - fy) * (dz ? fz : 1 - fz) * Hash(ix + dx, iy + dy, iz + dz);
        return v;
    }

    struct Decal {
        const Pixels* image = nullptr;
        Vec centre, right, up;
        double halfWidth = 0, halfHeight = 0;

        Decal() = default;
        Decal(const Pixels* img, const Vec& dir, double halfW, double halfH) : image(img), halfWidth(halfW), halfHeight(halfH) {
            centre = dir.Normalized();
            right = Vec{ 0, 1, 0 }.Cross(centre).Normalized();
            up = centre.Cross(right);
        }
        double Apply(const Vec& p, Colour& colour) const {
            if (!image || !image->IsOk() || p.Dot(centre) < 0.2) return 0;
            const double u = p.Dot(right) / halfWidth * 0.5 + 0.5, v = 0.5 - p.Dot(up) / halfHeight * 0.5;
            if (u < 0 || u > 1 || v < 0 || v > 1) return 0;
            double s[4];
            Sample(*image, u * image->width - 0.5, v * image->height - 0.5, s);
            const double a = s[3] / 255.0;
            if (a <= 0) return 0;
            colour = colour * (1 - a) + Colour{ s[0], s[1], s[2] };
            return a;
        }
    };

    class WrapModel : public Model {
    public:
        WrapModel(Pixels front, Pixels back, double depth, const wxSize& build)
            : m_front(std::move(front)), m_back(std::move(back)), m_depth(depth), m_build(build) {
            m_left.assign(m_front.height, INT_MAX);
            m_right.assign(m_front.height, -1);
            for (int y = 0; y < m_front.height; ++y)
                for (const Pixels* art : { &m_front, &m_back }) {
                    if (!art->IsOk()) continue;
                    for (int x = 0; x < art->width; ++x)
                        if (art->At(x, y)[3] > 16) { m_left[y] = std::min(m_left[y], x); m_right[y] = std::max(m_right[y], x); }
                }
            std::vector<Colour> left(m_front.height, Colour{ 0, 0, 0 }), right(m_front.height, Colour{ 0, 0, 0 });
            for (int y = 0; y < m_front.height; ++y) {
                if (m_right[y] < 0) continue;
                const int w = m_right[y] - m_left[y] + 1;
                const int inset = std::max(1, w * 2 / 100), band = std::max(2, w * 4 / 100);
                auto average = [&](const Pixels& art, int from, int to) {
                    Colour sum{ 0, 0, 0 };
                    double weight = 0;
                    for (int x = std::max(0, from); x <= std::min(art.width - 1, to); ++x) {
                        const uint8_t* px = art.At(x, y);
                        sum = sum + Colour{ (double)px[0], (double)px[1], (double)px[2] };
                        weight += px[3];
                    }
                    return std::make_pair(sum, weight);
                };
                auto addTo = [](Colour& total, double& weight, std::pair<Colour, double> part) { total = total + part.first; weight += part.second; };
                Colour l{ 0, 0, 0 }, r{ 0, 0, 0 };
                double lw = 0, rw = 0;
                addTo(l, lw, average(m_front, m_left[y] + inset, m_left[y] + inset + band));
                addTo(r, rw, average(m_front, m_right[y] - inset - band, m_right[y] - inset));
                if (m_back.IsOk()) {
                    addTo(r, rw, average(m_back, m_left[y] + inset, m_left[y] + inset + band));
                    addTo(l, lw, average(m_back, m_right[y] - inset - band, m_right[y] - inset));
                }
                left[y] = lw > 0 ? l * (255.0 / lw) : Colour{ 128, 128, 128 };
                right[y] = rw > 0 ? r * (255.0 / rw) : Colour{ 128, 128, 128 };
            }
            m_sideLeft.assign(m_front.height, Colour{ 0, 0, 0 });
            m_sideRight.assign(m_front.height, Colour{ 0, 0, 0 });
            const int soften = std::max(2, m_front.height / 120);
            for (int y = 0; y < m_front.height; ++y) {
                Colour l{ 0, 0, 0 }, r{ 0, 0, 0 };
                int n = 0;
                for (int k = y - soften; k <= y + soften; ++k)
                    if (k >= 0 && k < m_front.height && m_right[k] >= 0) { l = l + left[k]; r = r + right[k]; ++n; }
                if (n) { m_sideLeft[y] = l * (1.0 / n); m_sideRight[y] = r * (1.0 / n); }
            }
        }

        Frame Render(double angle, const wxSize& size, bool fine) const override {
            Pixels out;
            out.width = size.x;
            out.height = size.y;
            out.rgba.assign((size_t)size.x * size.y * 4, 0);
            Bounds bounds;
            if (!m_front.IsOk() || size.x < 1 || size.y < 1) return { out.ToImage(), wxRect() };
            const double scale = (double)size.x / m_build.x;
            const double ox = (size.x - m_front.width * scale) / 2, oy = (size.y - m_front.height * scale) / 2;
            const double axis = (m_front.width - 1) / 2.0;
            const double c = std::cos(angle), s = std::sin(angle);
            const double lightX = kLight.x, lightZ = kLight.z;

            for (int y = 0; y < size.y; ++y) {
                const double srcY = (y + 0.5 - oy) / scale - 0.5;
                const int row = (int)std::lround(srcY);
                if (row < 0 || row >= m_front.height || m_right[row] < 0) continue;
                const double half = (m_right[row] - m_left[row] + 1) / 2.0;
                const double centre = (m_left[row] + m_right[row]) / 2.0 - axis;
                const double deep = half * m_depth;
                const double reach = std::hypot(half * c, deep * s);
                const double tilt = std::atan2(deep * s, half * c);
                if (reach < 1e-6) continue;
                for (int x = 0; x < size.x; ++x) {
                    const double rel = (x + 0.5 - ox) / scale - 0.5 - axis - centre * c;
                    const double cover = Clamp01((reach - std::abs(rel)) * scale + 0.5);
                    if (cover <= 0) continue;
                    const double spread = std::acos(std::clamp(rel / reach, -1.0, 1.0));
                    double phi[2] = { tilt + spread, tilt - spread };
                    auto depthOf = [&](double p) { return -(centre + half * std::cos(p)) * s + deep * std::sin(p) * c; };
                    if (depthOf(phi[1]) > depthOf(phi[0])) std::swap(phi[0], phi[1]);
                    double px[4] = { 0, 0, 0, 0 };
                    double light = 1;
                    for (int hit = 0; hit < 2 && px[3] < 8; ++hit) {
                        const double X = centre + half * std::cos(phi[hit]), Z = deep * std::sin(phi[hit]);
                        const bool frontHalf = m_depth < 0.02 ? c >= 0 : Z >= 0;
                        const Pixels& art = frontHalf || !m_back.IsOk() ? m_front : m_back;
                        const double srcX = frontHalf || !m_back.IsOk() ? axis + X : axis - X;
                        if (fine) Sample(art, srcX, srcY, px); else Nearest(art, srcX, srcY, px);
                        if (hit == 0) {
                            const double sinPhi = std::sin(phi[hit]), cosPhi = std::cos(phi[hit]);
                            const double stretch = std::abs(-half * c * sinPhi + deep * s * cosPhi) / std::max(1e-6, std::abs(half * sinPhi));
                            const double side = Smooth(1.1, 1.9, stretch);
                            if (side > 0 && px[3] > 8) {
                                const Colour& sc = X - centre >= 0 ? m_sideRight[row] : m_sideLeft[row];
                                const double solid = px[3] + (255 - px[3]) * side;
                                const double k = side * solid / 255.0;
                                px[0] = px[0] * (1 - side) + sc.r * k;
                                px[1] = px[1] * (1 - side) + sc.g * k;
                                px[2] = px[2] * (1 - side) + sc.b * k;
                                px[3] = solid;
                            }
                            const double nx0 = cosPhi / std::max(half, 1e-6), nz0 = sinPhi / std::max(deep, 1e-6);
                            const double len = std::hypot(nx0, nz0);
                            const double nx = (nx0 * c + nz0 * s) / len, nz = (-nx0 * s + nz0 * c) / len;
                            light = std::min(1.0, 0.50 + 0.56 * std::max(0.0, nx * lightX + nz * lightZ) / lightZ);
                        } else {
                            light = 0.55;
                        }
                    }
                    if (px[3] < 1) continue;
                    const double a = px[3] * cover;
                    Over(out.At(x, y), px[0] * light * cover, px[1] * light * cover, px[2] * light * cover, a);
                    if (a > 60) bounds.Add(x, y);
                }
            }
            return { out.ToImage(), bounds.Rect() };
        }

    private:
        Pixels m_front, m_back;
        double m_depth;
        wxSize m_build;
        std::vector<int> m_left, m_right;
        std::vector<Colour> m_sideLeft, m_sideRight;
    };

    class BallModel : public Model {
    public:
        BallModel(bool soccer, const Colorway& colorway, Pixels emblem, Pixels text)
            : m_soccer(soccer), m_fabric(Colour::From(colorway.fabric)), m_trim(Colour::From(colorway.trim)),
              m_emblem(std::move(emblem)), m_text(std::move(text)) {
            m_emblemDecal = Decal(&m_emblem, { 0.42, 0.40, 0.81 }, 0.25, 0.25);
            m_textDecal = Decal(&m_text, { -0.26, -0.40, 0.88 }, 0.46, 0.13);
            if (m_soccer) {
                const double g = (1 + std::sqrt(5.0)) / 2, ig = 1 / g;
                for (int a : { -1, 1 })
                    for (int b : { -1, 1 }) {
                        m_pentagons.push_back(Vec{ 0, (double)a, b * g }.Normalized());
                        m_pentagons.push_back(Vec{ (double)a, b * g, 0 }.Normalized());
                        m_pentagons.push_back(Vec{ a * g, 0, (double)b }.Normalized());
                        m_hexagons.push_back(Vec{ a * ig, 0, b * g }.Normalized());
                        m_hexagons.push_back(Vec{ 0, a * g, b * ig }.Normalized());
                        m_hexagons.push_back(Vec{ a * g, b * ig, 0 }.Normalized());
                        for (int c : { -1, 1 }) m_hexagons.push_back(Vec{ (double)a, (double)b, (double)c }.Normalized());
                    }
            }
        }

        Frame Render(double angle, const wxSize& size, bool) const override {
            Pixels out;
            out.width = size.x;
            out.height = size.y;
            out.rgba.assign((size_t)size.x * size.y * 4, 0);
            const double R = std::min(size.x, size.y) * 0.40;
            const double cx = size.x / 2.0, cy = size.y * 0.47;
            const Camera camera(angle, 0.28);
            const double pixel = 1.0 / R;
            const Vec halfway = (kLight + Vec{ 0, 0, 1 }).Normalized();
            Bounds bounds;
            const int y0 = std::max(0, (int)(cy - R - 2)), y1 = std::min(size.y - 1, (int)(cy + R + 2));
            const int x0 = std::max(0, (int)(cx - R - 2)), x1 = std::min(size.x - 1, (int)(cx + R + 2));
            for (int y = y0; y <= y1; ++y)
                for (int x = x0; x <= x1; ++x) {
                    const double nx = (x + 0.5 - cx) / R, ny = -(y + 0.5 - cy) / R;
                    const double r = std::sqrt(nx * nx + ny * ny);
                    const double cover = Clamp01((1 - r) * R + 0.5);
                    if (cover <= 0) continue;
                    Vec n = r < 1 ? Vec{ nx, ny, std::sqrt(1 - r * r) } : Vec{ nx / r, ny / r, 0 };
                    const Vec p = camera.ToObject(n);

                    double groove = 0, grain;
                    Colour colour = m_soccer ? Soccer(p, pixel, groove, grain) : Basketball(p, pixel, groove, grain);
                    m_emblemDecal.Apply(p, colour);
                    m_textDecal.Apply(p, colour);

                    const double diffuse = std::clamp(n.Dot(kLight) * 0.75 + 0.35, 0.12, 1.15);
                    const double sparkle = m_soccer ? 0.85 + grain * 0.3 : 0.6 + grain * 0.8;
                    const double spec = std::pow(std::max(0.0, n.Dot(halfway)), 40) * (m_soccer ? 0.40 : 0.30)
                                        * (1 - groove) * sparkle;
                    const double rim = std::pow(1 - n.z, 3) * 0.25;
                    colour = colour * (diffuse * (1 - rim)) + Colour{ 255, 255, 255 } * spec;
                    const double a = cover * 255;
                    uint8_t* d = out.At(x, y);
                    d[0] = (uint8_t)std::clamp(colour.r * cover, 0.0, 255.0);
                    d[1] = (uint8_t)std::clamp(colour.g * cover, 0.0, 255.0);
                    d[2] = (uint8_t)std::clamp(colour.b * cover, 0.0, 255.0);
                    d[3] = (uint8_t)a;
                    if (cover > 0.3) bounds.Add(x, y);
                }
            return { out.ToImage(), bounds.Rect() };
        }

    private:
        Colour Basketball(const Vec& p, double pixel, double& groove, double& grain) const {
            const double curve = std::abs(p.x) - (0.62 + 0.30 * (p.y * p.y - p.z * p.z));
            Colour colour = m_fabric.Mix(m_trim, Clamp01(curve / (1.5 * pixel) + 0.5));
            auto seam = [pixel](double d, double width) { return Clamp01((width - std::abs(d)) / (1.2 * pixel) + 0.5); };
            const double curveDistance = curve / std::sqrt(1 + 0.36 * (p.y * p.y + p.z * p.z));
            groove = std::max({ seam(p.x, 0.02), seam(p.y, 0.02), seam(curveDistance, 0.02) });
            grain = Noise(p, 95.0);
            colour = colour * (1 + (grain - 0.5) * 0.09);
            return colour.Mix({ 28, 26, 30 }, groove);
        }

        Colour Soccer(const Vec& p, double pixel, double& groove, double& grain) const {
            constexpr double kPentagon = 0.28749, kHexagon = 0.36486;
            double best = 1e9, second = 1e9;
            bool pentagon = false;
            auto consider = [&](const std::vector<Vec>& centres, double inner, bool isPentagon) {
                double d1 = -2, d2 = -2;
                for (const Vec& c : centres) {
                    const double d = p.Dot(c);
                    if (d > d1) { d2 = d1; d1 = d; } else if (d > d2) d2 = d;
                }
                for (double d : { d1, d2 }) {
                    const double m = std::acos(std::clamp(d, -1.0, 1.0)) - inner;
                    if (m < best) { second = best; best = m; if (d == d1) pentagon = isPentagon; }
                    else if (m < second) second = m;
                }
            };
            consider(m_pentagons, kPentagon, true);
            consider(m_hexagons, kHexagon, false);
            const double edge = (second - best) / 2;
            groove = Clamp01((0.010 - edge) / (1.2 * pixel) + 0.5);
            grain = Noise(p, 70.0);
            Colour colour = pentagon ? m_fabric : Colour{ 246, 246, 243 };
            colour = colour * (0.88 + 0.12 * Smooth(0.0, 0.07, edge));
            colour = colour * (1 + (grain - 0.5) * 0.05);
            return colour.Mix({ 70, 72, 78 }, groove * 0.9);
        }

        bool m_soccer;
        Colour m_fabric, m_trim;
        Pixels m_emblem, m_text;
        Decal m_emblemDecal, m_textDecal;
        std::vector<Vec> m_pentagons, m_hexagons;
    };

    class CapModel : public Model {
    public:
        CapModel(const Colorway& colorway, Pixels emblem, Pixels text)
            : m_fabric(Colour::From(colorway.fabric)), m_trim(Colour::From(colorway.trim)),
              m_emblem(std::move(emblem)), m_text(std::move(text)) {}

        Frame Render(double angle, const wxSize& size, bool fine) const override {
            Pixels out;
            out.width = size.x;
            out.height = size.y;
            out.rgba.assign((size_t)size.x * size.y * 4, 0);
            const Camera camera(angle + 0.75, 0.40);
            const double ppu = std::min(size.x, size.y) * 0.27;
            const double cx = size.x / 2.0, cy = size.y * 0.52;
            const Vec pivot{ 0, 0.32, 0.25 };
            const Vec dir = camera.ToObject({ 0, 0, -1 });
            const int n = fine ? 2 : 1;
            Bounds bounds;
            for (int y = 0; y < size.y; ++y)
                for (int x = 0; x < size.x; ++x) {
                    double r = 0, g = 0, b = 0, a = 0;
                    for (int sy = 0; sy < n; ++sy)
                        for (int sx = 0; sx < n; ++sx) {
                            const double vx = (x + (sx + 0.5) / n - cx) / ppu, vy = -(y + (sy + 0.5) / n - cy) / ppu;
                            const Vec origin = camera.ToObject({ vx, vy, 6 }) + pivot;
                            Colour c;
                            double cover;
                            if (!Trace(origin, dir, camera, 1.0 / ppu, c, cover)) continue;
                            r += c.r * cover; g += c.g * cover; b += c.b * cover; a += cover;
                        }
                    if (a <= 0) continue;
                    const double k = 1.0 / (n * n);
                    uint8_t* d = out.At(x, y);
                    d[0] = (uint8_t)std::clamp(r * k, 0.0, 255.0);
                    d[1] = (uint8_t)std::clamp(g * k, 0.0, 255.0);
                    d[2] = (uint8_t)std::clamp(b * k, 0.0, 255.0);
                    d[3] = (uint8_t)std::clamp(a * k * 255, 0.0, 255.0);
                    if (a * k > 0.3) bounds.Add(x, y);
                }
            return { out.ToImage(), bounds.Rect() };
        }

    private:
        static constexpr double kA = 1.0, kB = 0.92, kC = 1.08;
        static double Bottom(double x, double z) { return (0.03 - 0.10 * x * x) * Clamp01(z / 0.4); }
        static constexpr double kPeakZ = 0.40, kPeakA = 0.98, kPeakC = 1.40;
        static double PeakY(double x, double z) { const double f = std::max(0.0, z - 0.7); return 0.03 - 0.10 * x * x - 0.07 * f * f; }
        static bool InOpening(const Vec& p) {
            if (p.z > 0 || std::abs(p.x) >= 0.27) return false;
            const double k = p.x / 0.27;
            return p.y - Bottom(p.x, p.z) < 0.28 * std::sqrt(1 - k * k);
        }
        static bool OnStrap(const Vec& p) { return p.z < 0 && std::abs(p.x) < 0.30 && p.y < 0.075; }

        bool Trace(const Vec& o, const Vec& d, const Camera& camera, double pixel, Colour& colour, double& cover) const {
            cover = 1;
            double tCrown = 1e9, tLining = 1e9, tPeak = 1e9;
            Vec crownHit{}, liningHit{}, peakHit{};
            {
                const Vec os{ o.x / kA, o.y / kB, o.z / kC }, ds{ d.x / kA, d.y / kB, d.z / kC };
                const double A = ds.Dot(ds), B = 2 * os.Dot(ds), C = os.Dot(os) - 1;
                const double disc = B * B - 4 * A * C;
                if (disc >= 0) {
                    const double q = std::sqrt(disc);
                    const double t0 = (-B - q) / (2 * A), t1 = (-B + q) / (2 * A);
                    const Vec p0 = o + d * t0, p1 = o + d * t1;
                    const bool above0 = p0.y >= Bottom(p0.x, p0.z);
                    if (above0 && (!InOpening(p0) || OnStrap(p0))) { tCrown = t0; crownHit = p0; }
                    else if (above0) { tLining = t0; liningHit = p1; }
                    else if (p1.y >= Bottom(p1.x, p1.z)) { tLining = t1; liningHit = p1; }
                }
            }
            {
                double t = -o.y / d.y;
                for (int i = 0; i < 5; ++i) {
                    const Vec p = o + d * t;
                    const double f = std::max(0.0, p.z - 0.7);
                    const double g = p.y - PeakY(p.x, p.z);
                    const double dg = d.y - (-0.20 * p.x * d.x - 0.14 * f * d.z);
                    if (std::abs(dg) < 1e-9) break;
                    t -= g / dg;
                }
                const Vec p = o + d * t;
                const double e = std::sqrt(p.x * p.x / (kPeakA * kPeakA) + (p.z - kPeakZ) * (p.z - kPeakZ) / (kPeakC * kPeakC));
                const double footprint = p.x * p.x / (kA * kA) + p.z * p.z / (kC * kC);
                if (e < 1 && footprint >= 1 && p.z > 0 && std::abs(p.y - PeakY(p.x, p.z)) < 1e-3) {
                    tPeak = t;
                    peakHit = p;
                }
            }
            const double tNear = std::min({ tCrown, tLining, tPeak });
            if (tNear >= 1e9) return false;

            Vec normal;
            if (tNear == tPeak) {
                const Vec& p = peakHit;
                const double f = std::max(0.0, p.z - 0.7);
                normal = Vec{ 0.20 * p.x, 1, 0.14 * f }.Normalized();
                const bool underside = camera.ToView(normal).z < 0;
                const double e = std::sqrt(p.x * p.x / (kPeakA * kPeakA) + (p.z - kPeakZ) * (p.z - kPeakZ) / (kPeakC * kPeakC));
                cover = Clamp01((1 - e) * kPeakA / pixel + 0.5);
                if (underside) {
                    colour = m_trim * 0.78;
                    normal = normal * -1;
                } else {
                    colour = m_fabric * 0.96;
                    for (double row : { 0.92, 0.86, 0.80, 0.74 })
                        if (std::abs(e - row) < 0.006 && std::fmod(std::atan2(p.x, p.z - kPeakZ) * 60 + 100, 1.0) < 0.6)
                            colour = Thread();
                    if (e > 0.975) colour = colour * 0.85;
                }
                const double footprint = std::sqrt(p.x * p.x / (kA * kA) + p.z * p.z / (kC * kC));
                colour = colour * (1 - 0.35 * std::exp(-(footprint - 1) / 0.06));
            } else if (tNear == tLining) {
                colour = m_fabric * 0.30;
                normal = Vec{ -liningHit.x / (kA * kA), -liningHit.y / (kB * kB), -liningHit.z / (kC * kC) }.Normalized();
            } else {
                const Vec& p = crownHit;
                normal = Vec{ p.x / (kA * kA), p.y / (kB * kB), p.z / (kC * kC) }.Normalized();
                colour = Crown(p, pixel);
            }
            const Vec n = camera.ToView(normal);
            const Vec halfway = (kLight + Vec{ 0, 0, 1 }).Normalized();
            const double light = 0.42 + 0.68 * std::max(0.0, n.Dot(kLight));
            colour = colour * light + Colour{ 255, 255, 255 } * (0.08 * std::pow(std::max(0.0, n.Dot(halfway)), 24));
            return true;
        }

        Colour Thread() const {
            const double l = 0.299 * m_fabric.r + 0.587 * m_fabric.g + 0.114 * m_fabric.b;
            return l < 90 ? m_fabric.Mix({ 255, 255, 255 }, 0.45) : m_fabric * 0.62;
        }

        Colour Crown(const Vec& p, double pixel) const {
            if (OnStrap(p) && InOpening(p)) {
                Colour c = m_trim * 0.7;
                for (double sx : { -0.15, 0.0, 0.15 })
                    if (std::hypot(p.x - sx, p.y - 0.037) < 0.022) c = m_trim * 0.4;
                return c;
            }
            Colour c = m_fabric;
            const double around = std::sqrt(p.x * p.x + p.z * p.z);
            const double phi = std::atan2(p.x, p.z);
            const double seam = std::abs(std::remainder(phi, kPi / 3)) * around;
            c = c.Mix(m_fabric * 0.66, Clamp01((0.010 - seam) / (1.2 * pixel) + 0.5));
            if (std::abs(seam - 0.035) < 0.0055 && std::fmod(p.y * 28 + 10, 1.0) < 0.55) c = Thread();
            const double panelMid = std::abs(std::remainder(phi - kPi / 6, kPi / 3)) * around;
            if (std::hypot(panelMid, p.y - 0.60) < 0.028) c = m_fabric * 0.35;
            if (p.y / kB > 0.975) c = m_trim.Mix(m_trim * 0.7, Smooth(0.975, 1.0, p.y / kB));
            if (p.y - Bottom(p.x, p.z) < 0.05) c = m_trim;
            if (p.z > 0.2) ApplyFlat(m_emblem, p.x / 0.27 * 0.5 + 0.5, 0.5 - (p.y - 0.40) / 0.27 * 0.5, c);
            if (p.z < -0.2) ApplyFlat(m_text, -p.x / 0.46 * 0.5 + 0.5, 0.5 - (p.y - 0.45) / 0.11 * 0.5, c);
            return c;
        }

        static void ApplyFlat(const Pixels& image, double u, double v, Colour& colour) {
            if (!image.IsOk() || u < 0 || u > 1 || v < 0 || v > 1) return;
            double s[4];
            Sample(image, u * image.width - 0.5, v * image.height - 0.5, s);
            colour = colour * (1 - s[3] / 255.0) + Colour{ s[0], s[1], s[2] };
        }

        Colour m_fabric, m_trim;
        Pixels m_emblem, m_text;
    };

    class RoundModel : public Model {
    public:
        struct Print { Pixels image; double turn, centreY, width, height; };

        RoundModel(Pixels base, double centreX, double radius, double sag, std::vector<Print> prints, const wxSize& build)
            : m_base(std::move(base)), m_centreX(centreX), m_radius(radius), m_sag(sag), m_prints(std::move(prints)), m_build(build) {}

        Frame Render(double angle, const wxSize& size, bool fine) const override {
            Pixels out;
            out.width = size.x;
            out.height = size.y;
            out.rgba.assign((size_t)size.x * size.y * 4, 0);
            if (!m_base.IsOk()) return { out.ToImage(), wxRect() };
            const double scale = (double)size.x / m_build.x;
            const double ox = (size.x - m_base.width * scale) / 2, oy = (size.y - m_base.height * scale) / 2;
            Bounds bounds;
            for (int y = 0; y < size.y; ++y)
                for (int x = 0; x < size.x; ++x) {
                    const double bx = (x + 0.5 - ox) / scale - 0.5, by = (y + 0.5 - oy) / scale - 0.5;
                    double px[4];
                    if (fine) Sample(m_base, bx, by, px); else Nearest(m_base, bx, by, px);
                    if (px[3] < 1) continue;
                    Colour c{ px[0], px[1], px[2] };
                    const double sx = (bx + 0.5 - m_centreX) / m_radius;
                    if (std::abs(sx) < 1) {
                        const double phi = std::asin(sx), cosPhi = std::sqrt(1 - sx * sx);
                        const double light = std::clamp(0.80 + 0.26 * std::cos(phi + 0.55), 0.55, 1.06);
                        for (const Print& print : m_prints) {
                            if (!print.image.IsOk()) continue;
                            const double u = std::remainder(phi - print.turn - angle, 2 * kPi) * m_radius / print.width + 0.5;
                            const double v = (by + 0.5 - m_sag * cosPhi - (print.centreY - print.height / 2)) / print.height;
                            if (u < 0 || u > 1 || v < 0 || v > 1) continue;
                            double s[4];
                            Sample(print.image, u * print.image.width - 0.5, v * print.image.height - 0.5, s);
                            const double a = s[3] / 255.0 * (px[3] / 255.0);
                            c = c * (1 - a) + Colour{ s[0], s[1], s[2] } * (light * px[3] / 255.0);
                        }
                    }
                    uint8_t* d = out.At(x, y);
                    d[0] = (uint8_t)std::clamp(c.r, 0.0, 255.0);
                    d[1] = (uint8_t)std::clamp(c.g, 0.0, 255.0);
                    d[2] = (uint8_t)std::clamp(c.b, 0.0, 255.0);
                    d[3] = (uint8_t)px[3];
                    if (px[3] > 60) bounds.Add(x, y);
                }
            return { out.ToImage(), bounds.Rect() };
        }

    private:
        Pixels m_base;
        double m_centreX, m_radius, m_sag;
        std::vector<Print> m_prints;
        wxSize m_build;
    };

    const Colorway& FindColorway(const wxString& id, size_t fallback) {
        for (const Colorway& c : Catalog::Colorways())
            if (c.id == id) return c;
        return Catalog::Colorways()[fallback % Catalog::Colorways().size()];
    }
}

Pixels Pixels::FromImage(const wxImage& image) {
    Pixels p;
    p.width = image.GetWidth();
    p.height = image.GetHeight();
    p.rgba.resize((size_t)p.width * p.height * 4);
    const unsigned char* rgb = image.GetData();
    const unsigned char* alpha = image.HasAlpha() ? image.GetAlpha() : nullptr;
    const size_t n = (size_t)p.width * p.height;
    for (size_t i = 0; i < n; ++i) {
        const unsigned a = alpha ? alpha[i] : 255;
        p.rgba[i * 4 + 0] = (uint8_t)((rgb[i * 3 + 0] * a + 127) / 255);
        p.rgba[i * 4 + 1] = (uint8_t)((rgb[i * 3 + 1] * a + 127) / 255);
        p.rgba[i * 4 + 2] = (uint8_t)((rgb[i * 3 + 2] * a + 127) / 255);
        p.rgba[i * 4 + 3] = (uint8_t)a;
    }
    return p;
}

wxImage Pixels::ToImage() const {
    wxImage image(std::max(1, width), std::max(1, height), false);
    image.InitAlpha();
    unsigned char* rgb = image.GetData();
    unsigned char* alpha = image.GetAlpha();
    const size_t n = (size_t)width * height;
    for (size_t i = 0; i < n; ++i) {
        const unsigned a = rgba[i * 4 + 3];
        alpha[i] = (unsigned char)a;
        for (int c = 0; c < 3; ++c)
            rgb[i * 3 + c] = a ? (unsigned char)std::min(255u, (rgba[i * 4 + c] * 255u + a / 2) / a) : 0;
    }
    if (n == 0) memset(alpha, 0, 1);
    return image;
}

std::unique_ptr<Model> Build(const Product& product, const Colorway& colorway,
                             const Personalizer* personalizer, const wxSize& pixels) {
    const wxString suffix = wxT("_") + colorway.id + wxT(".png");
    switch (product.shape) {
    case Shape::Basketball:
    case Shape::SoccerBall: {
        const bool soccer = product.shape == Shape::SoccerBall;
        Pixels text = soccer ? TextDecal(personalizer, { 520, 146 }, colorway.fabric, wxColour(250, 250, 248))
                             : TextDecal(personalizer, { 520, 146 }, colorway.trim, Darker(colorway.fabric, 0.55));
        return std::make_unique<BallModel>(soccer, colorway, LoadDecal(wxT("emblem") + suffix), std::move(text));
    }
    case Shape::Cap:
        return std::make_unique<CapModel>(colorway, LoadDecal(wxT("emblem") + suffix),
                                          TextDecal(personalizer, { 520, 124 }, colorway.trim, Darker(colorway.fabric, 0.6)));
    case Shape::Round: {
        Pixels base = LoadFitted(product.id + suffix, pixels.x * 0.86, pixels.y * 0.84);
        const RoundShape& r = product.round;
        const double k = base.width / product.artWidth;
        std::vector<RoundModel::Print> prints;
        Pixels emblem = LoadDecal(wxT("emblem") + suffix);
        prints.push_back({ std::move(emblem), 0.0, r.emblemY * k, r.emblemSize * k, r.emblemSize * k });
        const wxSize textPixels((int)(r.textWidth * k), (int)(r.textHeight * k));
        prints.push_back({ TextDecal(personalizer, textPixels, colorway.trim, Darker(colorway.fabric, 0.6)),
                           r.textTurn, r.textY * k, r.textWidth * k, r.textHeight * k });
        return std::make_unique<RoundModel>(std::move(base), r.centerX * k, r.radius * k, r.sag * k, std::move(prints), pixels);
    }
    case Shape::Flat:
        break;
    }
    Pixels front = LoadFitted(product.id + suffix, pixels.x * 0.82, pixels.y * 0.80);
    Print(front, personalizer, colorway, 0);
    Pixels back;
    if (!product.reverseArtId.IsEmpty() && front.IsOk()) {
        wxImage image(Theme::AssetPath(product.reverseArtId + suffix), wxBITMAP_TYPE_PNG);
        if (image.IsOk()) {
            back = Resample(Pixels::FromImage(image), front.width, front.height);
            Print(back, personalizer, colorway, 1);
        }
    }
    return std::make_unique<WrapModel>(std::move(front), std::move(back), product.thickness, pixels);
}

double PrintAngle(const Product& product, int side) {
    switch (product.shape) {
    case Shape::Flat:  return side == 1 ? kPi : 0.0;
    case Shape::Cap:   return kPi;
    case Shape::Round: return -product.round.textTurn;
    default:           return 0.0;
    }
}

void DrawStage(wxGraphicsContext* gc, const wxRect2DDouble& area, const wxColour& surround) {
    wxGraphicsGradientStops stops(wxColour(247, 248, 250), wxColour(232, 235, 240));
    stops.Add(wxColour(242, 244, 247), 0.55f);
    gc->SetBrush(gc->CreateLinearGradientBrush(area.m_x, area.m_y, area.m_x, area.m_y + area.m_height, stops));
    gc->SetPen(*wxTRANSPARENT_PEN);
    const double radius = std::min(area.m_width, area.m_height) * 0.035;
    gc->DrawRoundedRectangle(area.m_x, area.m_y, area.m_width, area.m_height, radius);
    (void)surround;
}

void DrawShadow(wxGraphicsContext* gc, const wxRect& bounds, double strength) {
    if (bounds.IsEmpty()) return;
    const double w = bounds.width * 0.95, h = std::max(6.0, w * 0.13);
    const double cx = bounds.x + bounds.width / 2.0, cy = bounds.GetBottom() - h * 0.12;
    gc->PushState();
    gc->Translate(cx, cy);
    gc->Scale(1.0, h / w);
    const unsigned char a = (unsigned char)std::clamp(95.0 * strength, 0.0, 255.0);
    wxGraphicsGradientStops stops(wxColour(24, 28, 38, a), wxColour(24, 28, 38, 0));
    stops.Add(wxColour(24, 28, 38, (unsigned char)(a * 0.45)), 0.5f);
    gc->SetBrush(gc->CreateRadialGradientBrush(0, 0, 0, 0, w / 2, stops));
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->DrawEllipse(-w / 2, -w / 2, w, w);
    gc->PopState();
}

wxBitmap Still(const Product& product, const Colorway& colorway, const wxSize& pixels, const wxColour& surround) {
    wxBitmap canvas(std::max(1, pixels.x), std::max(1, pixels.y), 24);
    wxMemoryDC dc(canvas);
    dc.SetBackground(wxBrush(surround));
    dc.Clear();
    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (gc) {
        DrawStage(gc.get(), wxRect2DDouble(0, 0, pixels.x, pixels.y), surround);
        const Frame frame = Build(product, colorway, nullptr, pixels)->Render(0, pixels, true);
        DrawShadow(gc.get(), frame.bounds, 0.8);
        gc->DrawBitmap(wxBitmap(frame.image), 0, 0, pixels.x, pixels.y);
    }
    dc.SelectObject(wxNullBitmap);
    return canvas;
}

wxBitmap Tile(const Product& product, const wxSize& pixels, const wxColour& surround) {
    wxBitmap canvas(std::max(1, pixels.x), std::max(1, pixels.y), 24);
    wxMemoryDC dc(canvas);
    dc.SetBackground(wxBrush(surround));
    dc.Clear();
    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (gc) {
        DrawStage(gc.get(), wxRect2DDouble(0, 0, pixels.x, pixels.y), surround);
        struct Pose { size_t index; double x, y, w, h, angle; };
        const bool ball = product.shape == Shape::Basketball || product.shape == Shape::SoccerBall;
        const Pose poses[2] = { { 0, 0.02, -0.02, 0.62, 0.84, ball ? 0.9 : -0.45 },
                                { 1, 0.33, 0.10, 0.66, 0.92, ball ? -0.5 : 0.40 } };
        for (const Pose& pose : poses) {
            const wxSize size((int)(pixels.x * pose.w), (int)(pixels.y * pose.h));
            if (size.x < 8 || size.y < 8) continue;
            const Colorway& colorway = FindColorway(product.tileColorways[pose.index], pose.index);
            const Frame frame = Build(product, colorway, nullptr, size)->Render(pose.angle, size, true);
            const double px = pixels.x * pose.x, py = pixels.y * pose.y;
            wxRect shadow = frame.bounds;
            shadow.Offset((int)px, (int)py);
            DrawShadow(gc.get(), shadow, 0.7);
            gc->DrawBitmap(wxBitmap(frame.image), px, py, size.x, size.y);
        }
    }
    dc.SelectObject(wxNullBitmap);
    return canvas;
}

}
