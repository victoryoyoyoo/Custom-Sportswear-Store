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

    // Adds the input controls; onChange fires on every edit (to redraw).
    virtual void BuildControls(wxWindow* parent, wxSizer* sizer, std::function<void()> onChange) {}
    // Text appended to the size in the cart line, e.g. "#23・WANG".
    virtual wxString Describe() const { return wxString(); }
    // Draws onto the preview. `art` is where the product artwork was drawn
    // (in pixels); PrintArea values are scaled from artwork units into it.
    virtual void Draw(wxGraphicsContext* gc, const wxRect2DDouble& art, const Colorway& colorway) const {}

protected:
    const Product& m_product;
};

class NameAndNumberPersonalizer : public Personalizer {
public:
    using Personalizer::Personalizer;
    void BuildControls(wxWindow* parent, wxSizer* sizer, std::function<void()> onChange) override;
    wxString Describe() const override;
    void Draw(wxGraphicsContext* gc, const wxRect2DDouble& art, const Colorway& colorway) const override;

private:
    wxString PrintedName() const;
    wxSpinCtrl* m_number = nullptr;
    wxTextCtrl* m_name = nullptr;
};

class NumberPersonalizer : public Personalizer {
public:
    using Personalizer::Personalizer;
    void BuildControls(wxWindow* parent, wxSizer* sizer, std::function<void()> onChange) override;
    wxString Describe() const override;
    void Draw(wxGraphicsContext* gc, const wxRect2DDouble& art, const Colorway& colorway) const override;

private:
    wxSpinCtrl* m_number = nullptr;
};

// Short text. Printed straight onto the artwork when the product defines a
// text area; otherwise (embroidery on the back of a cap, on a wristband)
// shown as a stitched label in the corner of the preview.
class TextPersonalizer : public Personalizer {
public:
    using Personalizer::Personalizer;
    void BuildControls(wxWindow* parent, wxSizer* sizer, std::function<void()> onChange) override;
    wxString Describe() const override;
    void Draw(wxGraphicsContext* gc, const wxRect2DDouble& art, const Colorway& colorway) const override;

private:
    wxString Text() const;
    wxTextCtrl* m_text = nullptr;
};
