#pragma once
#include <wx/wx.h>
#include <wx/graphics.h>
#include <cstdint>
#include <memory>
#include <vector>
#include "Catalog.h"

class Personalizer;

// The products in 3D, for the 360° view on the product page and for the
// still pictures elsewhere (product list, cart).
//
// There is no 3D engine behind this: each shape is drawn pixel by pixel.
//   Flat    - each row of the artwork is wrapped round an ellipse, the front
//             artwork on its front half and the back artwork on the back, so
//             a shirt turns like one on an invisible mannequin.
//   Balls   - every pixel is a point on a sphere; turned back into the
//             ball's own coordinates it tells which panel / seam it is on.
//   Cap     - the same idea with a half-ellipsoid crown and a curved peak.
//   Round   - a bottle or band keeps its outline; only the print moves,
//             wrapped round the cylinder.
namespace Showcase {

// Premultiplied RGBA, 8 bits per channel: blending is then a multiply-add.
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
    wxImage image;   // the product on a transparent background
    wxRect bounds;   // where it is solid, for the shadow under it
};

class Model {
public:
    virtual ~Model() = default;
    // The product turned `angle` radians (0 = how the page first shows it;
    // positive turns its front to the right), drawn into `size` pixels.
    // `fine` = take the time for full quality (not needed mid-drag).
    virtual Frame Render(double angle, const wxSize& size, bool fine) const = 0;
};

// Builds the model for a product in a colourway, with the customer's print
// when a personalizer is given, sized for `pixels`.
std::unique_ptr<Model> Build(const Product& product, const Colorway& colorway,
                             const Personalizer* personalizer, const wxSize& pixels);

// Angle the 360° view turns to so a print on `side` faces the viewer.
double PrintAngle(const Product& product, int side);

// Light backdrop with a soft floor, and the product's contact shadow.
void DrawStage(wxGraphicsContext* gc, const wxRect2DDouble& area, const wxColour& surround);
void DrawShadow(wxGraphicsContext* gc, const wxRect& productBounds, double strength = 1.0);

// Front view on the stage (cart lines).
wxBitmap Still(const Product& product, const Colorway& colorway, const wxSize& pixels, const wxColour& surround);

// Product-list picture: two colourways posed on the stage.
wxBitmap Tile(const Product& product, const wxSize& pixels, const wxColour& surround);

}  // namespace Showcase
