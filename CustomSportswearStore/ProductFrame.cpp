#include "ProductFrame.h"
#include "Lang.h"
#include "CartDialog.h"
#include "ProductDialogs.h"
#include "SwatchPicker.h"
#include "Turntable.h"
#include <wx/statline.h>
#include <algorithm>
#include <cmath>

namespace {
    constexpr int kPad = 28;
}

ProductFrame::ProductFrame(wxWindow* parent, int productIndex)
    : wxFrame(parent, wxID_ANY, Catalog::Products()[productIndex].name + L(wxT("｜運動用品客製購物系統"), wxT(" | Custom Sportswear Store"))),
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

    wxBoxSizer* headerRight = nullptr;
    wxPanel* header = Theme::MakeHeader(root, product.name, Lang::English() ? product.tagline : product.englishName + wxT("  ·  ") + product.tagline,
                                        &headerRight);
    headerRight->Add(Theme::MakeFullScreenButton(this, header), 0, wxRIGHT, FromDIP(10));
    auto* orders = Theme::MakeHeaderButton(header);
    orders->SetLabel(L(wxT("  我的訂單  "), wxT("  My Orders  ")));
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

    Widgets::Card* previewCard = Theme::MakeCard(root);
    wxBoxSizer* previewSizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* previewTop = new wxBoxSizer(wxHORIZONTAL);
    if (product.shape == Shape::Flat && !product.reverseArtId.IsEmpty()) {
        m_sideChips = new Widgets::ChipPicker(previewCard, { product.sideNames[0], product.sideNames[1] }, 0);
        previewTop->Add(m_sideChips, 0, wxALIGN_CENTER_VERTICAL);
        m_sideChips->OnSelectionChanged([this](int index) {
            if (!m_syncingSide) m_preview->TurnTo(Showcase::PrintAngle(GetProduct(), index));
        });
    } else {
        previewTop->Add(Theme::MakeLabel(previewCard, L(wxT("360° 預覽"), wxT("360° view")), 10, true, Theme::kMuted), 0, wxALIGN_CENTER_VERTICAL);
    }
    previewTop->AddStretchSpacer();
    m_heart = new Widgets::HeartToggle(previewCard, Favorites::Get().Has(m_productIndex));
    m_heart->SetToolTip(L(wxT("加入收藏"), wxT("Save")));
    m_heart->OnToggled([this](bool) { Favorites::Get().Toggle(m_productIndex); });
    previewTop->Add(m_heart, 0, wxALIGN_CENTER_VERTICAL);
    previewSizer->Add(previewTop, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(20));

    m_preview = new Turntable(previewCard, wxSize(340, 320), [this](const wxSize& px) {
        return Showcase::Build(GetProduct(), CurrentColorway(), m_personalizer.get(), px);
    });
    m_preview->OnSideChanged([this](int side) {
        if (!m_sideChips) return;
        m_syncingSide = true;
        m_sideChips->SetSelection(side);
        m_syncingSide = false;
    });
    previewSizer->Add(m_preview, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(12));
    m_previewTitle = Theme::MakeLabel(previewCard, wxEmptyString, 16, true);
    m_previewSubtitle = Theme::MakeLabel(previewCard, wxEmptyString, 10, false, Theme::kMuted);
    previewSizer->Add(m_previewTitle, 0, wxALIGN_CENTER | wxTOP, FromDIP(10));
    previewSizer->Add(m_previewSubtitle, 0, wxALIGN_CENTER | wxTOP, FromDIP(2));
    previewSizer->Add(Theme::MakeLabel(previewCard, Theme::FormatPrice(product.price), 15, true, Theme::kOrange),
                      0, wxALIGN_CENTER | wxTOP | wxBOTTOM, FromDIP(8));
    previewSizer->AddSpacer(FromDIP(12));
    previewCard->SetSizer(previewSizer);
    body->Add(previewCard, 4, wxEXPAND | wxRIGHT, FromDIP(8));

    m_formCard = Theme::MakeCard(root);
    wxBoxSizer* form = new wxBoxSizer(wxVERTICAL);
    m_formCard->SetSizer(form);

    m_colorValue = AddSection(form, L(wxT("選擇配色"), wxT("Colour")));
    m_swatches = new SwatchPicker(m_formCard, (int)Catalog::Colorways().size());
    form->Add(m_swatches, 0, wxLEFT | wxRIGHT, FromDIP(21));
    m_swatches->OnSelectionChanged([this](int) { RefreshPreview(true); });

    m_sizeValue = AddSection(form, product.sizeTitle);
    if (product.sizes.size() > 1) {
        std::vector<wxString> labels;
        for (const SizeOption& s : product.sizes) labels.push_back(s.label);
        m_sizes = new Widgets::ChipPicker(m_formCard, labels, product.defaultSize);
        wxBoxSizer* sizeRow = new wxBoxSizer(wxHORIZONTAL);
        sizeRow->Add(m_sizes, 0, wxALIGN_CENTER_VERTICAL);
        if (product.sizeAdvice != SizeAdvice::None) {
            sizeRow->AddStretchSpacer();
            auto* advice = Theme::MakeSecondaryButton(m_formCard, L(wxT("尺寸建議"), wxT("Size advice")), 10);
            advice->SetMinSize(FromDIP(wxSize(-1, 36)));
            sizeRow->Add(advice, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(12));
            advice->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
                SizeAdvisorDialog dialog(this, GetProduct());
                if (Theme::ShowModalDialog(dialog) == wxID_OK) m_sizes->SetSelection(dialog.Recommended());
            });
        }
        form->Add(sizeRow, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(kPad));
        form->AddSpacer(FromDIP(8));
        m_sizes->OnSelectionChanged([this](int) { RefreshPreview(); });
    }
    m_sizeHint = Theme::MakeLabel(m_formCard, wxEmptyString, 10, false, Theme::kMuted);
    form->Add(m_sizeHint, 0, wxLEFT | wxRIGHT, FromDIP(kPad));

    if (product.personalization != Personalization::None) {
        AddSection(form, product.textLabel);
        wxBoxSizer* custom = new wxBoxSizer(wxVERTICAL);
        m_personalizer->BuildControls(m_formCard, custom, [this](int side) {
            RefreshPreview();
            m_preview->TurnTo(Showcase::PrintAngle(GetProduct(), side));
        });
        form->Add(custom, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(kPad));
    }

    AddSection(form, L(wxT("商品特色"), wxT("Details")));
    for (const wxString& line : product.features) {
        wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(Theme::MakeLabel(m_formCard, wxT("•"), 10, true, Theme::kOrange), 0, wxRIGHT, FromDIP(8));
        row->Add(Theme::MakeLabel(m_formCard, line, 10, false, Theme::kMuted));
        form->Add(row, 0, wxLEFT | wxRIGHT, FromDIP(kPad));
        form->AddSpacer(FromDIP(6));
    }

    form->AddStretchSpacer();
    form->Add(new wxStaticLine(m_formCard), 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(kPad));

    wxBoxSizer* totals = new wxBoxSizer(wxHORIZONTAL);
    totals->Add(Theme::MakeLabel(m_formCard, L(wxT("數量"), wxT("Quantity")), 12, true), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(14));
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
    totals->Add(Theme::MakeLabel(m_formCard, L(wxT("小計"), wxT("Subtotal")), 11, false, Theme::kMuted), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(12));
    m_subtotal = Theme::MakeLabel(m_formCard, wxEmptyString, 20, true, Theme::kOrange);
    totals->Add(m_subtotal, 0, wxALIGN_CENTER_VERTICAL);
    form->AddSpacer(FromDIP(18));
    form->Add(totals, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(kPad));

    auto* add = Theme::MakePrimaryButton(m_formCard, L(wxT("加入購物車"), wxT("Add to cart")), 13);
    add->ShowArrow();
    form->AddSpacer(FromDIP(16));
    wxBoxSizer* actions = new wxBoxSizer(wxHORIZONTAL);
    actions->Add(add, 1, wxEXPAND);
    if (product.personalization == Personalization::NameAndNumber) {
        auto* team = Theme::MakeSecondaryButton(m_formCard, L(wxT("團體訂購"), wxT("Team order")), 11);
        team->SetMinSize(FromDIP(wxSize(120, 46)));
        team->SetToolTip(L(wxT("一次輸入整隊的姓名、背號與尺寸"), wxT("Enter a whole team's names, numbers and sizes at once")));
        actions->Add(team, 0, wxEXPAND | wxLEFT, FromDIP(10));
        team->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnTeamOrder(); });
    }
    form->Add(actions, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(kPad));
    form->AddSpacer(FromDIP(kPad));

    body->Add(m_formCard, 5, wxEXPAND);
    wxSizerItem* bodyItem = rootSizer->Add(body, 1, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(16));
    rootSizer->AddSpacer(FromDIP(8));

    wxBoxSizer* footer = new wxBoxSizer(wxHORIZONTAL);
    auto* back = Theme::MakeSecondaryButton(root, L(wxT("←  所有商品"), wxT("←  All products")), 10);
    back->SetMinSize(FromDIP(wxSize(140, 38)));
    footer->Add(back, 0, wxALIGN_CENTER_VERTICAL);
    footer->AddStretchSpacer();
    footer->Add(Theme::MakeLabel(root, wxString::Format(L(wxT("單筆滿 %s 免運・未滿運費 %s・F11 全螢幕"), wxT("Free shipping over %s, otherwise %s · F11 full screen")),
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
    m_preview->Spin(450);
}

wxString ProductFrame::DescribeSpec() const {
    const Product& product = GetProduct();
    const int size = m_sizes ? m_sizes->GetSelection() : product.defaultSize;
    wxString spec = product.sizes[size].label;
    if (product.id == wxT("sneaker")) spec = wxT("EU ") + spec;
    const wxString custom = m_personalizer->Describe();
    if (!custom.IsEmpty()) spec += L(wxT("・"), wxT(" · ")) + custom;
    return spec;
}

void ProductFrame::RefreshPreview(bool crossfade) {
    const Product& product = GetProduct();
    const Colorway& c = CurrentColorway();
    const int size = m_sizes ? m_sizes->GetSelection() : product.defaultSize;
    const wxString look = c.id + wxT("|") + m_personalizer->Describe();
    if (look != m_previewLook) {
        m_previewLook = look;
        m_preview->Rebuild(crossfade);
    }
    m_previewTitle->SetLabel(product.name + L(wxT("・"), wxT(" · ")) + c.name);
    m_previewSubtitle->SetLabel(Lang::English() ? DescribeSpec() : c.englishName + wxT("  ·  ") + DescribeSpec());
    m_colorValue->SetLabel(Lang::English() ? c.name : c.name + wxT("  ") + c.englishName);
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
    const int target = GetProduct().price * m_quantity;
    if (m_shownSubtotal < 0) {
        m_shownSubtotal = target;
        m_subtotal->SetLabel(Theme::FormatPrice(target));
        m_formCard->Layout();
        return;
    }
    const int from = m_shownSubtotal;
    m_subtotalTween.Start(280, [this, from, target](double t) {
        m_shownSubtotal = (int)std::lround(from + (target - from) * t);
        m_subtotal->SetLabel(Theme::FormatPrice(m_shownSubtotal));
        m_formCard->Layout();
    });
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
    Widgets::ShowToast(this, L(wxT("已加入購物車"), wxT("Added to cart")),
                       wxString::Format(L(wxT("%s（%s）× %d・點此查看"), wxT("%s (%s) × %d · click to view")), item.Title(), item.spec, item.quantity),
                       [this] { OpenCart(); });
}

void ProductFrame::OnTeamOrder() {
    auto* jersey = dynamic_cast<NameAndNumberPersonalizer*>(m_personalizer.get());
    TeamOrderDialog dialog(this, GetProduct(), CurrentColorway(), jersey ? jersey->TeamName() : wxString());
    if (Theme::ShowModalDialog(dialog) != wxID_OK) return;

    const auto players = dialog.Players();
    for (const auto& p : players) {
        const wxString spec = GetProduct().sizes[p.sizeIndex].label + L(wxT("・"), wxT(" · ")) +
                              NameAndNumberPersonalizer::Spec(p.number, p.name, dialog.TeamName());
        ShoppingCart::Get().Add({ m_productIndex, m_swatches->GetSelection(), spec, GetProduct().price, 1 });
    }
    RefreshCartButton();
    m_cartButton->Flash(Theme::kOrange);
    const wxString team = dialog.TeamName().IsEmpty() ? wxString(L(wxT("團體訂購"), wxT("Team order"))) : dialog.TeamName();
    Widgets::ShowToast(this, Lang::English() ? wxT("Added ") + Plural(players.size(), wxT("jersey"), wxT("jerseys"))
                                       : wxString::Format(wxT("已加入 %zu 件球衣"), players.size()),
                       CurrentColorway().name + L(wxT("・"), wxT(" · ")) + team + L(wxT("・點此查看購物車"), wxT(" · click to view the cart")), [this] { OpenCart(); });
}

void ProductFrame::OpenCart() {
    CartDialog dialog(this);
    Theme::ShowModalDialog(dialog);
    RefreshCartButton();
}

void ProductFrame::OnClose(wxCloseEvent& event) {
    if (!Theme::CanClosePage(event)) return;
    if (m_closing) return;
    m_closing = true;
    wxFrame* launcher = wxDynamicCast(GetParent(), wxFrame);
    if (!launcher) {
        Destroy();
        return;
    }
    Theme::ShowLike(launcher, this, [this] { Destroy(); });
}
