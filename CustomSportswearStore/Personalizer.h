#pragma once
#include <wx/wx.h>
#include <wx/graphics.h>
#include <wx/spinctrl.h>
#include <functional>
#include <memory>
#include "Catalog.h"

// How a product can be customised (Strategy pattern). The product page
// doesn't know about names, numbers or embroidery: it asks the product's
// Personalizer to add its input fields, to describe the choice for the cart
// line, and to draw it onto the preview.
class Personalizer {
public:
    explicit Personalizer(const Product& product) : m_product(product) {}
    virtual ~Personalizer() = default;

    static std::unique_ptr<Personalizer> For(const Product& product);

    // Adds the input controls. onChange fires on every edit (to redraw) and
    // says which side of the product the edit shows on (0 = the side the
    // page opens on, 1 = the other one), so the 360° view can turn to it.
    virtual void BuildControls(wxWindow* parent, wxSizer* sizer, std::function<void(int side)> onChange) {}
    // Text appended to the size in the cart line, e.g. "#23・WANG".
    virtual wxString Describe() const { return wxString(); }
    // Prints onto one side of a flat product's artwork. `art` is where the
    // artwork was drawn (in pixels); PrintArea values are scaled into it.
    virtual void Draw(wxGraphicsContext* gc, const wxRect2DDouble& art, const Colorway& colorway, int side) const {}
    // What to print on a round product (ball, cap, band, bottle); the 3D
    // view wraps it round the surface. Empty when there is nothing.
    virtual wxString PrintText() const { return wxString(); }

    // `text` drawn as a print into a transparent image `pixels` in size,
    // as large as fits, centred.
    static wxImage TextDecal(const wxString& text, const wxSize& pixels, const wxColour& fill, const wxColour& outline);

protected:
    const Product& m_product;
};

class NameAndNumberPersonalizer : public Personalizer {
public:
    using Personalizer::Personalizer;

    // "#23・WANG・正面 TIGERS" (empty parts left out). Shared with team orders.
    static wxString Spec(int number, const wxString& name, const wxString& team);
    wxString TeamName() const;
    void BuildControls(wxWindow* parent, wxSizer* sizer, std::function<void(int side)> onChange) override;
    wxString Describe() const override;
    void Draw(wxGraphicsContext* gc, const wxRect2DDouble& art, const Colorway& colorway, int side) const override;

private:
    wxString PrintedName() const;
    wxSpinCtrl* m_number = nullptr;
    wxTextCtrl* m_name = nullptr;
    wxTextCtrl* m_team = nullptr;
};

class NumberPersonalizer : public Personalizer {
public:
    using Personalizer::Personalizer;
    void BuildControls(wxWindow* parent, wxSizer* sizer, std::function<void(int side)> onChange) override;
    wxString Describe() const override;
    void Draw(wxGraphicsContext* gc, const wxRect2DDouble& art, const Colorway& colorway, int side) const override;

private:
    wxSpinCtrl* m_number = nullptr;
};

// Short text: printed into the product's text area on flat products, or
// handed to the 3D view (PrintText) to wrap round a ball, cap or band.
class TextPersonalizer : public Personalizer {
public:
    using Personalizer::Personalizer;
    void BuildControls(wxWindow* parent, wxSizer* sizer, std::function<void(int side)> onChange) override;
    wxString Describe() const override;
    void Draw(wxGraphicsContext* gc, const wxRect2DDouble& art, const Colorway& colorway, int side) const override;
    wxString PrintText() const override;

private:
    wxString Text() const;
    wxTextCtrl* m_text = nullptr;
};
