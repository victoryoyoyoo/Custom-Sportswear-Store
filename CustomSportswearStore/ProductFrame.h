#pragma once
#include <wx/wx.h>
#include <memory>
#include "Catalog.h"
#include "Personalizer.h"
#include "Theme.h"

class SwatchPicker;

// One product page, built entirely from a Product row: colour swatches, size
// chips, the product's Personalizer (name/number, text, ...), features,
// quantity and "add to cart". The big preview on the left is re-rendered at
// the panel's real size whenever an option changes.
class ProductFrame : public wxFrame {
public:
    ProductFrame(wxWindow* parent, int productIndex);

private:
    const Product& GetProduct() const { return Catalog::Products()[m_productIndex]; }
    const Colorway& CurrentColorway() const;

    void BuildLayout();
    wxStaticText* AddSection(wxSizer* sizer, const wxString& title);  // returns the right-hand value label
    wxBitmap RenderPreview(const wxSize& pixels) const;
    wxString DescribeSpec() const;

    void RefreshPreview(bool crossfade = false);
    void RefreshTotals();
    void RefreshCartButton();
    void SetQuantity(int quantity);
    void OnAddToCart();
    void OnTeamOrder();
    void OpenCart();
    void OnClose(wxCloseEvent& event);

    int m_productIndex;
    std::unique_ptr<Personalizer> m_personalizer;
    int m_quantity = 1;
    bool m_closing = false;
    bool m_front = false;  // showing the front view (jersey)

    wxPanel* m_root = nullptr;
    wxPanel* m_formCard = nullptr;
    Theme::ImagePanel* m_preview = nullptr;
    SwatchPicker* m_swatches = nullptr;
    Widgets::ChipPicker* m_sizes = nullptr;
    wxStaticText* m_colorValue = nullptr;
    wxStaticText* m_sizeValue = nullptr;
    wxStaticText* m_sizeHint = nullptr;
    wxStaticText* m_previewTitle = nullptr;
    wxStaticText* m_previewSubtitle = nullptr;
    wxStaticText* m_quantityLabel = nullptr;
    wxStaticText* m_subtotal = nullptr;
    Widgets::FlatButton* m_minus = nullptr;
    Widgets::FlatButton* m_plus = nullptr;
    Widgets::FlatButton* m_cartButton = nullptr;
    Widgets::HeartToggle* m_heart = nullptr;
};
