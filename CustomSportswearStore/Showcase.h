#pragma once
#include <wx/wx.h>
#include <wx/graphics.h>
#include <cstdint>
#include <memory>
#include <vector>
#include "Catalog.h"

class Personalizer;

namespace Showcase {

struct Pixels {
    int width = 0, height = 0;
    std::vector<uint8_t> rgba;

    bool IsOk() const { return width > 0 && height > 0; }
    uint8_t* At(int x, int y) { return &rgba[((size_t)y * width + x) * 4]; }
    const uint8_t* At(int x, int y) const { return &rgba[((size_t)y * width + x) * 4]; }

    static Pixels FromImage(const wxImage& image);
    wxImage ToImage() const;
};

struct Frame {
    wxImage image;
    wxRect bounds;
};

class Model {
public:
    virtual ~Model() = default;
    virtual Frame Render(double angle, const wxSize& size, bool fine) const = 0;
};

std::unique_ptr<Model> Build(const Product& product, const Colorway& colorway,
                             const Personalizer* personalizer, const wxSize& pixels);

double PrintAngle(const Product& product, int side);

void DrawStage(wxGraphicsContext* gc, const wxRect2DDouble& area, const wxColour& surround);
void DrawShadow(wxGraphicsContext* gc, const wxRect& productBounds, double strength = 1.0);

wxBitmap Still(const Product& product, const Colorway& colorway, const wxSize& pixels, const wxColour& surround);

wxBitmap Tile(const Product& product, const wxSize& pixels, const wxColour& surround);

}
