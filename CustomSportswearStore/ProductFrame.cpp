#include "ProductFrame.h"
#include "CartDialog.h"
#include "SwatchPicker.h"
#include <wx/statline.h>
#include <algorithm>

namespace {
    constexpr int kPad = 28;  // content inset inside the cards
}

ProductFrame::ProductFrame(wxWindow* parent, int productIndex)
    : wxFrame(parent, wxID_ANY, Catalog::Products()[productIndex].name + wxT("｜運動用品客製購物系統")),
      m_productIndex(productIndex),
      m_personalizer(Personalizer::For(Catalog::Products()[productIndex])) {
    SetIcon(wxICON(aaaa_app));
    Theme::InstallFullScreenKeys(this);
    BuildLayout();
    Bind(wxEVT_CLOSE_WINDOW, &ProductFrame::OnClose, this);
}

const Colorway& ProductFrame::CurrentColorway() const {
    return Catalog::Colorways()[m_swatches->GetSelection()];
}

wxStaticText* ProductFrame::AddSection(wxSizer* sizer, const wxString& title) {
    wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
    wxPanel* bar = new wxPanel(m_formCard, wxID_ANY, wxDefaultPosition, FromDIP(wxSize(3, 16)));
    bar->SetBackgroundColour(Theme::kOrange);
    row->Add(bar, 0, wxALIGN_CENTER_VERTICAL);
    row->Add(Theme::MakeLabel(m_formCard, title, 12, true), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(8));
    row->AddStretchSpacer();
    wxStaticText* value = Theme::MakeLabel(m_formCard, wxEmptyString, 10, false, Theme::kMuted);
    row->Add(value, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(kPad));
    sizer->AddSpacer(FromDIP(12));
    return value;
}

void ProductFrame::BuildLayout() {
    const Product& product = GetProduct();
    wxPanel* root = m_root = new wxPanel(this, wxID_ANY);
    root->SetBackgroundColour(Theme::kPage);
    wxBoxSizer* rootSizer = new wxBoxSizer(wxVERTICAL);

    // ---- header ----
    wxBoxSizer* headerRight = nullptr;
    wxPanel* header = Theme::MakeHeader(root, product.name, product.englishName + wxT("  ·  ") + product.tagline,
                                        &headerRight);
    headerRight->Add(Theme::MakeFullScreenButton(this, header), 0, wxRIGHT, FromDIP(10));
    auto* orders = Theme::MakeHeaderButton(header);
    orders->SetLabel(wxT("  我的訂單  "));
    headerRight->Add(orders, 0, wxRIGHT, FromDIP(10));
    orders->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        OrdersDialog dialog(this);
        Theme::ShowModalDialog(dialog);
    });
    m_cartButton = Theme::MakeHeaderButton(header);
    headerRight->Add(m_cartButton);
    rootSizer->Add(header, 0, wxEXPAND);
    rootSizer->AddSpacer(FromDIP(16));

    wxBoxSizer* body = new wxBoxSizer(wxHORIZONTAL);

    // ---- left: live preview ----
    Widgets::Card* previewCard = Theme::MakeCard(root);
    wxBoxSizer* previewSizer = new wxBoxSizer(wxVERTICAL);
    m_preview = new Theme::ImagePanel(previewCard, wxSize(340, 340),
                                      [this](const wxSize& px) { return RenderPreview(px); });
    previewSizer->Add(m_preview, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(kPad));
    m_previewTitle = Theme::MakeLabel(previewCard, wxEmptyString, 16, true);
    m_previewSubtitle = Theme::MakeLabel(previewCard, wxEmptyString, 10, false, Theme::kMuted);
    previewSizer->Add(m_previewTitle, 0, wxALIGN_CENTER | wxTOP, FromDIP(10));
    previewSizer->Add(m_previewSubtitle, 0, wxALIGN_CENTER | wxTOP, FromDIP(2));
    previewSizer->Add(Theme::MakeLabel(previewCard, Theme::FormatPrice(product.price), 15, true, Theme::kOrange),
                      0, wxALIGN_CENTER | wxTOP | wxBOTTOM, FromDIP(8));
    previewSizer->AddSpacer(FromDIP(12));
    previewCard->SetSizer(previewSizer);
    body->Add(previewCard, 4, wxEXPAND | wxRIGHT, FromDIP(8));

    // ---- right: options ----
    m_formCard = Theme::MakeCard(root);
    wxBoxSizer* form = new wxBoxSizer(wxVERTICAL);
    m_formCard->SetSizer(form);

    m_colorValue = AddSection(form, wxT("選擇配色"));
    m_swatches = new SwatchPicker(m_formCard, (int)Catalog::Colorways().size());
    form->Add(m_swatches, 0, wxLEFT | wxRIGHT, FromDIP(21));  // swatch cells carry 7 DIP of padding
    m_swatches->OnSelectionChanged([this](int) { RefreshPreview(true); });

    m_sizeValue = AddSection(form, product.sizeTitle);
    if (product.sizes.size() > 1) {
        std::vector<wxString> labels;
        for (const SizeOption& s : product.sizes) labels.push_back(s.label);
        m_sizes = new Widgets::ChipPicker(m_formCard, labels, product.defaultSize);
        form->Add(m_sizes, 0, wxLEFT | wxRIGHT, FromDIP(kPad));
        form->AddSpacer(FromDIP(8));
        m_sizes->OnSelectionChanged([this](int) { RefreshPreview(); });
    }
    m_sizeHint = Theme::MakeLabel(m_formCard, wxEmptyString, 10, false, Theme::kMuted);
    form->Add(m_sizeHint, 0, wxLEFT | wxRIGHT, FromDIP(kPad));

    if (product.personalization != Personalization::None) {
        AddSection(form, product.textLabel);
        wxBoxSizer* custom = new wxBoxSizer(wxVERTICAL);
        m_personalizer->BuildControls(m_formCard, custom, [this] { RefreshPreview(); });
        form->Add(custom, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(kPad));
    }

    AddSection(form, wxT("商品特色"));
    for (const wxString& line : product.features) {
        wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(Theme::MakeLabel(m_formCard, wxT("•"), 10, true, Theme::kOrange), 0, wxRIGHT, FromDIP(8));
        row->Add(Theme::MakeLabel(m_formCard, line, 10, false, Theme::kMuted));
        form->Add(row, 0, wxLEFT | wxRIGHT, FromDIP(kPad));
        form->AddSpacer(FromDIP(6));
    }

    form->AddStretchSpacer();
    form->Add(new wxStaticLine(m_formCard), 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(kPad));

    // quantity stepper + subtotal
    wxBoxSizer* totals = new wxBoxSizer(wxHORIZONTAL);
    totals->Add(Theme::MakeLabel(m_formCard, wxT("數量"), 12, true), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(14));
    m_minus = Theme::MakeSecondaryButton(m_formCard, wxT("－"), 12);
    m_plus = Theme::MakeSecondaryButton(m_formCard, wxT("＋"), 12);
    m_minus->SetMinSize(FromDIP(wxSize(40, 38)));
    m_plus->SetMinSize(FromDIP(wxSize(40, 38)));
    m_quantityLabel = new wxStaticText(m_formCard, wxID_ANY, wxT("1"), wxDefaultPosition, FromDIP(wxSize(48, -1)),
                                       wxALIGN_CENTRE_HORIZONTAL | wxST_NO_AUTORESIZE);
    m_quantityLabel->SetFont(Theme::Font(13, true));
    m_quantityLabel->SetForegroundColour(Theme::kText);
    totals->Add(m_minus, 0, wxALIGN_CENTER_VERTICAL);
    totals->Add(m_quantityLabel, 0, wxALIGN_CENTER_VERTICAL);
    totals->Add(m_plus, 0, wxALIGN_CENTER_VERTICAL);
    totals->AddStretchSpacer();
    totals->Add(Theme::MakeLabel(m_formCard, wxT("小計"), 11, false, Theme::kMuted), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(12));
    m_subtotal = Theme::MakeLabel(m_formCard, wxEmptyString, 20, true, Theme::kOrange);
    totals->Add(m_subtotal, 0, wxALIGN_CENTER_VERTICAL);
    form->AddSpacer(FromDIP(18));
    form->Add(totals, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(kPad));

    auto* add = Theme::MakePrimaryButton(m_formCard, wxT("加入購物車"), 13);
    add->ShowArrow();
    form->AddSpacer(FromDIP(16));
    form->Add(add, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(kPad));
    form->AddSpacer(FromDIP(kPad));

    body->Add(m_formCard, 5, wxEXPAND);
    wxSizerItem* bodyItem = rootSizer->Add(body, 1, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(16));
    rootSizer->AddSpacer(FromDIP(8));

    // ---- footer ----
    wxBoxSizer* footer = new wxBoxSizer(wxHORIZONTAL);
    auto* back = Theme::MakeSecondaryButton(root, wxT("←  所有商品"), 10);
    back->SetMinSize(FromDIP(wxSize(140, 38)));
    footer->Add(back, 0, wxALIGN_CENTER_VERTICAL);
    footer->AddStretchSpacer();
    footer->Add(Theme::MakeLabel(root, wxString::Format(wxT("單筆滿 %s 免運・未滿運費 %s・F11 全螢幕"),
                                                        Theme::FormatPrice(Catalog::kFreeShippingThreshold),
                                                        Theme::FormatPrice(Catalog::kShippingFee)),
                                 10, false, Theme::kMuted),
                0, wxALIGN_CENTER_VERTICAL);
    wxSizerItem* footerItem = rootSizer->Add(footer, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(24));
    rootSizer->AddSpacer(FromDIP(18));

    root->SetSizer(rootSizer);
    Theme::LimitWidth(root, bodyItem, 1340, 16);
    Theme::LimitWidth(root, footerItem, 1324, 24);

    back->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Close(); });
    m_cartButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OpenCart(); });
    add->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnAddToCart(); });
    m_minus->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { SetQuantity(m_quantity - 1); });
    m_plus->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { SetQuantity(m_quantity + 1); });

    Theme::FitFrameToContent(this, root, wxSize(1060, 740));
    Centre();
    SetQuantity(1);
    RefreshPreview();
    RefreshCartButton();
}

wxBitmap ProductFrame::RenderPreview(const wxSize& pixels) const {
    const Product& product = GetProduct();
    const Colorway& colorway = CurrentColorway();
    wxBitmap canvas(pixels.x, pixels.y, 24);
    {
        wxMemoryDC dc(canvas);
        dc.SetBackground(wxBrush(Theme::kCard));
        dc.Clear();
        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        wxImage art(Theme::AssetPath(product.id + wxT("_") + colorway.id + wxT(".png")), wxBITMAP_TYPE_PNG);
        if (gc && art.IsOk()) {
            const double fit = std::min((double)pixels.x / art.GetWidth(), (double)pixels.y / art.GetHeight());
            const double w = art.GetWidth() * fit, h = art.GetHeight() * fit;
            const wxRect2DDouble area((pixels.x - w) / 2, (pixels.y - h) / 2, w, h);
            // Resampled by the graphics backend: wxImage::Scale leaves a
            // contour line across the soft shadows. Overdraw 1px at the edges.
            gc->SetInterpolationQuality(wxINTERPOLATION_BEST);
            gc->DrawBitmap(wxBitmap(art), area.m_x - 1, area.m_y - 1, w + 2, h + 2);
            m_personalizer->Draw(gc.get(), area, colorway);
        }
    }
    return canvas;
}

wxString ProductFrame::DescribeSpec() const {
    const Product& product = GetProduct();
    const int size = m_sizes ? m_sizes->GetSelection() : product.defaultSize;
    wxString spec = product.sizes[size].label;
    if (product.id == wxT("sneaker")) spec = wxT("EU ") + spec;
    const wxString custom = m_personalizer->Describe();
    if (!custom.IsEmpty()) spec += wxT("・") + custom;
    return spec;
}

void ProductFrame::RefreshPreview(bool crossfade) {
    const Product& product = GetProduct();
    const Colorway& c = CurrentColorway();
    const int size = m_sizes ? m_sizes->GetSelection() : product.defaultSize;
    m_preview->Rerender(crossfade);
    m_previewTitle->SetLabel(product.name + wxT("・") + c.name);
    m_previewSubtitle->SetLabel(c.englishName + wxT("  ·  ") + DescribeSpec());
    m_colorValue->SetLabel(c.name + wxT("  ") + c.englishName);
    m_sizeValue->SetLabel(product.sizes[size].label);
    m_sizeHint->SetLabel(product.sizes[size].hint);
    m_formCard->Layout();
    m_preview->GetParent()->Layout();
}

void ProductFrame::SetQuantity(int quantity) {
    m_quantity = std::clamp(quantity, 1, ShoppingCart::kMaxQuantityPerLine);
    m_quantityLabel->SetLabel(wxString::Format(wxT("%d"), m_quantity));
    m_minus->Enable(m_quantity > 1);
    m_plus->Enable(m_quantity < ShoppingCart::kMaxQuantityPerLine);
    RefreshTotals();
}

void ProductFrame::RefreshTotals() {
    m_subtotal->SetLabel(Theme::FormatPrice(GetProduct().price * m_quantity));
    m_formCard->Layout();
}

void ProductFrame::RefreshCartButton() {
    m_cartButton->SetLabel(CartButtonLabel());
    m_cartButton->GetParent()->Layout();
}

void ProductFrame::OnAddToCart() {
    CartItem item{ m_productIndex, m_swatches->GetSelection(), DescribeSpec(), GetProduct().price, m_quantity };
    ShoppingCart::Get().Add(item);
    RefreshCartButton();
    m_cartButton->Flash(Theme::kOrange);
    Widgets::ShowToast(this, wxT("已加入購物車"),
                       wxString::Format(wxT("%s（%s）× %d・點此查看"), item.Title(), item.spec, item.quantity),
                       [this] { OpenCart(); });
}

void ProductFrame::OpenCart() {
    CartDialog dialog(this);
    Theme::ShowModalDialog(dialog);
    RefreshCartButton();
}

void ProductFrame::OnClose(wxCloseEvent& event) {
    if (!Theme::CanClosePage(event)) return;
    // Whether it's the back button or the window's X, hand control back to the
    // product list (otherwise the hidden list keeps the app alive with no
    // window). This page stays up until the list has faded in.
    if (m_closing) return;
    m_closing = true;
    wxFrame* launcher = wxDynamicCast(GetParent(), wxFrame);
    if (!launcher) {
        Destroy();
        return;
    }
    Theme::ShowLike(launcher, this, [this] { Destroy(); });
}
