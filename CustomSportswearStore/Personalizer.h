#pragma once
#include <wx/wx.h>
#include <wx/graphics.h>
#include <wx/spinctrl.h>
#include <functional>
#include <memory>
#include "Catalog.h"

class Personalizer {
public:
    explicit Personalizer(const Product& product) : m_product(product) {}
    virtual ~Personalizer() = default;

    static std::unique_ptr<Personalizer> For(const Product& product);

    virtual void BuildControls(wxWindow* parent, wxSizer* sizer, std::function<void(int side)> onChange) {}
    virtual wxString Describe() const { return wxString(); }
    virtual void Draw(wxGraphicsContext* gc, const wxRect2DDouble& art, const Colorway& colorway, int side) const {}
    virtual wxString PrintText() const { return wxString(); }

    static wxImage TextDecal(const wxString& text, const wxSize& pixels, const wxColour& fill, const wxColour& outline);

protected:
    const Product& m_product;
};

class NameAndNumberPersonalizer : public Personalizer {
public:
    using Personalizer::Personalizer;

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
