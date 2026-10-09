#include "LauncherFrame.h"
#include "Lang.h"
#include "Showcase.h"
#include "CartDialog.h"
#include "Catalog.h"
#include "ProductFrame.h"
#include "Theme.h"
#include <wx/weakref.h>
#include <wx/srchctrl.h>

LauncherFrame::LauncherFrame()
    : wxFrame(nullptr, wxID_ANY, L(wxT("全部商品｜運動用品客製購物系統"), wxT("Shop All | Custom Sportswear Store"))) {
    SetIcon(wxICON(aaaa_app));
    Theme::InstallFullScreenKeys(this);

    wxPanel* root = new wxPanel(this, wxID_ANY);
    root->SetBackgroundColour(Theme::kPage);
    wxBoxSizer* rootSizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* headerRight = nullptr;
    wxPanel* header = Theme::MakeHeader(root, L(wxT("運動用品客製購物系統"), wxT("Custom Sportswear Store")), L(wxT("Custom Sportswear Store  ·  客製球隊服裝與配件"), wxT("Custom teamwear and accessories")), &headerRight);
    headerRight->Add(Theme::MakeFullScreenButton(this, header), 0, wxRIGHT, FromDIP(10));
    auto* orders = Theme::MakeHeaderButton(header);
    orders->SetLabel(L(wxT("  我的訂單  "), wxT("  My Orders  ")));
    headerRight->Add(orders, 0, wxRIGHT, FromDIP(10));
    m_cartButton = Theme::MakeHeaderButton(header);
    headerRight->Add(m_cartButton);
    rootSizer->Add(header, 0, wxEXPAND);
    rootSizer->AddSpacer(FromDIP(28));

    wxBoxSizer* intro = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* heading = new wxBoxSizer(wxVERTICAL);
    heading->Add(Theme::MakeEyebrow(root, L(wxT("SHOP ALL"), wxT("TEAMWEAR · GEAR"))));
    heading->Add(Theme::MakeLabel(root, L(wxT("全部商品"), wxT("Shop All")), 20, true), 0, wxTOP, FromDIP(6));
    m_count = Theme::MakeLabel(root, wxEmptyString, 10, false, Theme::kMuted);
    heading->Add(m_count, 0, wxTOP, FromDIP(2));
    intro->Add(heading, 0, wxALIGN_BOTTOM);
    intro->AddStretchSpacer();
    m_search = new wxSearchCtrl(root, wxID_ANY, wxEmptyString, wxDefaultPosition, FromDIP(wxSize(200, -1)));
    m_search->SetFont(Theme::Font(11));
    m_search->SetDescriptiveText(L(wxT("搜尋商品"), wxT("Search products")));
    m_search->ShowCancelButton(true);
    intro->Add(m_search, 0, wxALIGN_BOTTOM | wxRIGHT | wxBOTTOM, FromDIP(4));
    intro->AddSpacer(FromDIP(12));
    std::vector<wxString> filters = { L(wxT("全部"), wxT("All")) };
    for (const wxString& c : Catalog::Categories()) filters.push_back(c);
    filters.push_back(L(wxT("♥ 收藏"), wxT("♥ Saved")));
    auto* filter = new Widgets::ChipPicker(root, filters, 0);
    intro->Add(filter, 0, wxALIGN_BOTTOM);
    wxSizerItem* introItem = rootSizer->Add(intro, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(36));
    rootSizer->AddSpacer(FromDIP(14));

    const int count = (int)Catalog::Products().size();
    m_scroll = new wxScrolledWindow(root, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxBORDER_NONE);
    m_scroll->SetBackgroundColour(Theme::kPage);
    m_scroll->SetScrollRate(0, FromDIP(24));
    m_grid = new wxGridSizer(0, 4, FromDIP(4), FromDIP(4));
    for (int i = 0; i < count; ++i) {
        m_cards.push_back(MakeProductCard(m_scroll, i));
        m_grid->Add(m_cards.back(), 1, wxEXPAND);
    }
    wxBoxSizer* scrollSizer = new wxBoxSizer(wxVERTICAL);
    scrollSizer->Add(m_grid, 0, wxEXPAND | wxBOTTOM, FromDIP(6));
    m_scroll->SetSizer(scrollSizer);
    m_scroll->Bind(wxEVT_SIZE, [this](wxSizeEvent& event) {
        const int column = (m_scroll->GetClientSize().x - FromDIP(12)) / 4 - FromDIP(36);
        const int height = std::max(FromDIP(150), column * 64 / 100);
        for (Theme::ImagePanel* picture : m_pictures)
            if (picture->GetMinSize().y != height) picture->SetMinSize(wxSize(FromDIP(210), height));
        m_scroll->FitInside();
        event.Skip();
    });
    m_root = root;
    m_empty = Theme::MakeLabel(root, wxEmptyString, 11, false, Theme::kMuted);
    rootSizer->Add(m_empty, 0, wxALIGN_CENTER | wxBOTTOM, FromDIP(8));
    wxSizerItem* gridItem = rootSizer->Add(m_scroll, 1, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(28));
    rootSizer->AddSpacer(FromDIP(14));

    wxBoxSizer* footer = new wxBoxSizer(wxHORIZONTAL);
    footer->Add(Theme::MakeLabel(root, wxString::Format(L(wxT("單筆滿 %s 免運・結帳可用優惠碼・F11 全螢幕"), wxT("Free shipping over %s · coupons at checkout · F11 full screen")),
                                                        Theme::FormatPrice(Catalog::kFreeShippingThreshold)),
                                 10, false, Theme::kMuted),
                0, wxALIGN_CENTER_VERTICAL);
    footer->AddStretchSpacer();
    auto* quit = Theme::MakeSecondaryButton(root, L(wxT("離開商店"), wxT("Leave store")), 10);
    quit->SetMinSize(FromDIP(wxSize(120, 38)));
    footer->Add(quit, 0, wxALIGN_CENTER_VERTICAL);
    wxSizerItem* footerItem = rootSizer->Add(footer, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(36));
    rootSizer->AddSpacer(FromDIP(20));

    root->SetSizer(rootSizer);
    Theme::LimitWidth(root, introItem, 1392, 36);
    Theme::LimitWidth(root, gridItem, 1400, 28);
    Theme::LimitWidth(root, footerItem, 1392, 36);
    Theme::FitFrameToContent(this, root, wxSize(1180, 820));
    Centre();

    m_cartButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        CartDialog dialog(this);
        Theme::ShowModalDialog(dialog);
        RefreshCartButton();
    });
    orders->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        OrdersDialog dialog(this);
        Theme::ShowModalDialog(dialog);
    });
    filter->OnSelectionChanged([this](int index) {
        const int categories = (int)Catalog::Categories().size();
        m_favoritesOnly = index == categories + 1;
        m_category = index >= 1 && index <= categories ? Catalog::Categories()[index - 1] : wxString();
        ApplyFilter();
    });
    m_search->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { ApplyFilter(); });
    m_search->Bind(wxEVT_SEARCHCTRL_CANCEL_BTN, [this](wxCommandEvent&) {
        m_search->Clear();
        ApplyFilter();
    });
    quit->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Close(); });
    Bind(wxEVT_CLOSE_WINDOW, [](wxCloseEvent& event) {
        if (Theme::CanClosePage(event)) event.Skip();
    });
    Bind(wxEVT_SHOW, [this](wxShowEvent& event) {
        if (event.IsShown()) {
            RefreshCartButton();
            for (size_t i = 0; i < m_hearts.size(); ++i) m_hearts[i]->SetOn(Favorites::Get().Has((int)i));
            ApplyFilter();
        }
        event.Skip();
    });
    RefreshCartButton();
    ApplyFilter();
}

void LauncherFrame::ApplyFilter() {
    const wxString query = m_search->GetValue().Trim().Trim(false).Lower();
    std::vector<bool> matches(m_cards.size());
    for (size_t i = 0; i < m_cards.size(); ++i) {
        const Product& p = Catalog::Products()[i];
        bool match = m_category.IsEmpty() || p.category == m_category;
        if (m_favoritesOnly) match = Favorites::Get().Has((int)i);
        if (match && !query.IsEmpty())
            match = p.name.Lower().Contains(query) || p.englishName.Lower().Contains(query) ||
                    p.tagline.Lower().Contains(query) || p.category.Contains(query);
        matches[i] = match;
    }
    const bool changed = matches != m_lastMatches;
    m_lastMatches = matches;

    m_root->Freeze();
    m_grid->Clear(false);
    int shown = 0;
    for (size_t i = 0; i < m_cards.size(); ++i) {
        m_cards[i]->Show(matches[i]);
        if (!matches[i]) continue;
        m_grid->Add(m_cards[i], 1, wxEXPAND);
        if (changed) m_pictures[i]->PlayIntro(70 * shown);
        ++shown;
    }
    for (int filler = shown; filler < 4; ++filler) m_grid->AddStretchSpacer();

    m_empty->Show(shown == 0);
    if (shown == 0)
        m_empty->SetLabel(m_favoritesOnly && query.IsEmpty() ? wxString(L(wxT("還沒有收藏的商品：點商品卡右下角的愛心加入收藏"), wxT("Nothing saved yet: tap the heart on a product card to save it")))
                                                              : wxString(L(wxT("找不到符合的商品，換個關鍵字試試"), wxT("No products match. Try another word."))));
    wxString label = wxString::Format(L(wxT("%zu 類商品・%zu 款配色・可客製姓名與背號"), wxT("%zu products · %zu colourways")),
                                      Catalog::Products().size(), Catalog::Colorways().size());
    if (m_favoritesOnly) label = Lang::English() ? wxT("Saved · ") + Plural(shown, wxT("product"), wxT("products"))
                                                  : wxString::Format(wxT("收藏・%d 項商品"), shown);
    else if (!m_category.IsEmpty() || !query.IsEmpty())
        label = (m_category.IsEmpty() ? wxString() : m_category + L(wxT("・"), wxT(" · "))) +
                (Lang::English() ? Plural(shown, wxT("product"), wxT("products")) : wxString::Format(wxT("%d 項商品"), shown));
    m_count->SetLabel(label);
    m_root->Layout();
    m_scroll->FitInside();
    if (changed) m_scroll->Scroll(0, 0);
    m_root->Thaw();
}

wxWindow* LauncherFrame::MakeProductCard(wxWindow* parent, int productIndex) {
    const Product& product = Catalog::Products()[productIndex];
    Widgets::Card* card = Theme::MakeCard(parent, true);
    card->SetCursor(wxCursor(wxCURSOR_HAND));
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    const int pad = FromDIP(24);

    auto* picture = new Theme::ImagePanel(card, wxSize(210, 150), [&product](const wxSize& px) {
        return Showcase::Tile(product, px, Theme::kCard);
    });
    m_pictures.push_back(picture);
    auto* heart = new Widgets::HeartToggle(card, Favorites::Get().Has(productIndex), 30);
    heart->SetToolTip(L(wxT("加入收藏"), wxT("Save")));
    heart->OnToggled([this, productIndex](bool) {
        Favorites::Get().Toggle(productIndex);
        if (m_favoritesOnly) CallAfter([this] { ApplyFilter(); });
    });
    m_hearts.push_back(heart);
    sizer->Add(picture, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(18));

    wxBoxSizer* titleRow = new wxBoxSizer(wxHORIZONTAL);
    titleRow->Add(Theme::MakeLabel(card, product.name, 13, true), 0, wxALIGN_BOTTOM);
    titleRow->AddStretchSpacer();
    titleRow->Add(Theme::MakeLabel(card, Theme::FormatPrice(product.price), 12, true, Theme::kOrange), 0, wxALIGN_BOTTOM);
    sizer->Add(titleRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, pad);
    sizer->AddSpacer(FromDIP(4));
    sizer->Add(Theme::MakeLabel(card, product.tagline, 10, false, Theme::kMuted), 0, wxLEFT | wxRIGHT, pad);
    sizer->AddSpacer(FromDIP(10));
    wxBoxSizer* bottom = new wxBoxSizer(wxHORIZONTAL);
    bottom->Add(Theme::MakeLabel(card, L(wxT("查看商品  →"), wxT("View  →")), 10, true, Theme::kOrange), 0, wxALIGN_CENTER_VERTICAL);
    bottom->AddStretchSpacer();
    bottom->Add(heart, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(bottom, 0, wxEXPAND | wxLEFT | wxRIGHT, pad);
    sizer->AddSpacer(pad - FromDIP(6));

    card->SetSizer(sizer);
    if (!Lang::English()) card->SetToolTip(product.englishName);

    auto open = [this, productIndex](wxMouseEvent&) { OpenProduct(productIndex); };
    card->Bind(wxEVT_LEFT_UP, open);
    for (wxWindow* child : card->GetChildren()) {
        if (child == heart) continue;
        child->SetCursor(wxCursor(wxCURSOR_HAND));
        child->Bind(wxEVT_LEFT_UP, open);
    }
    return card;
}

void LauncherFrame::OpenProduct(int productIndex) {
    const wxLongLong now = wxGetLocalTimeMillis();
    if (now - m_lastOpenMs < 600) return;
    m_lastOpenMs = now;

    auto* page = new ProductFrame(this, productIndex);
    wxWeakRef<ProductFrame> alive(page);
    Theme::ShowLike(page, this, [this, alive] {
        if (alive && alive->IsShown()) Hide();
    });
}

void LauncherFrame::RefreshCartButton() {
    m_cartButton->SetLabel(CartButtonLabel());
    m_cartButton->GetParent()->Layout();
}
