#include "LauncherFrame.h"
#include "CartDialog.h"
#include "Catalog.h"
#include "ProductFrame.h"
#include "Theme.h"

LauncherFrame::LauncherFrame()
    : wxFrame(nullptr, wxID_ANY, wxT("全部商品｜運動用品客製購物系統")) {
    SetIcon(wxICON(aaaa_app));
    Theme::InstallFullScreenKeys(this);

    wxPanel* root = new wxPanel(this, wxID_ANY);
    root->SetBackgroundColour(Theme::kPage);
    wxBoxSizer* rootSizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* headerRight = nullptr;
    wxPanel* header = Theme::MakeHeader(root, wxT("運動用品客製購物系統"), wxT("Custom Sportswear Store  ·  客製球隊服裝與配件"), &headerRight);
    headerRight->Add(Theme::MakeFullScreenButton(this, header), 0, wxRIGHT, FromDIP(10));
    auto* orders = Theme::MakeHeaderButton(header);
    orders->SetLabel(wxT("  我的訂單  "));
    headerRight->Add(orders, 0, wxRIGHT, FromDIP(10));
    m_cartButton = Theme::MakeHeaderButton(header);
    headerRight->Add(m_cartButton);
    rootSizer->Add(header, 0, wxEXPAND);
    rootSizer->AddSpacer(FromDIP(28));

    // Intro: eyebrow tag + heading on the left, category filter on the right.
    wxBoxSizer* intro = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* heading = new wxBoxSizer(wxVERTICAL);
    heading->Add(Theme::MakeEyebrow(root, wxT("SHOP ALL")));
    heading->Add(Theme::MakeLabel(root, wxT("全部商品"), 20, true), 0, wxTOP, FromDIP(6));
    m_count = Theme::MakeLabel(root, wxEmptyString, 10, false, Theme::kMuted);
    heading->Add(m_count, 0, wxTOP, FromDIP(2));
    intro->Add(heading, 0, wxALIGN_BOTTOM);
    intro->AddStretchSpacer();
    std::vector<wxString> filters = { wxT("全部") };
    for (const wxString& c : Catalog::Categories()) filters.push_back(c);
    auto* filter = new Widgets::ChipPicker(root, filters, 0);
    intro->Add(filter, 0, wxALIGN_BOTTOM);
    wxSizerItem* introItem = rootSizer->Add(intro, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(36));
    rootSizer->AddSpacer(FromDIP(14));

    const int count = (int)Catalog::Products().size();
    m_grid = new wxGridSizer(0, 4, FromDIP(4), FromDIP(4));
    for (int i = 0; i < count; ++i) {
        m_cards.push_back(MakeProductCard(root, i));
        m_grid->Add(m_cards.back(), 1, wxEXPAND);
    }
    m_root = root;
    wxSizerItem* gridItem = rootSizer->Add(m_grid, 1, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(28));
    rootSizer->AddSpacer(FromDIP(14));

    wxBoxSizer* footer = new wxBoxSizer(wxHORIZONTAL);
    footer->Add(Theme::MakeLabel(root, wxString::Format(wxT("單筆滿 %s 免運・結帳可用優惠碼・F11 全螢幕"),
                                                        Theme::FormatPrice(Catalog::kFreeShippingThreshold)),
                                 10, false, Theme::kMuted),
                0, wxALIGN_CENTER_VERTICAL);
    footer->AddStretchSpacer();
    auto* quit = Theme::MakeSecondaryButton(root, wxT("離開商店"), 10);
    quit->SetMinSize(FromDIP(wxSize(120, 38)));
    footer->Add(quit, 0, wxALIGN_CENTER_VERTICAL);
    wxSizerItem* footerItem = rootSizer->Add(footer, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(36));
    rootSizer->AddSpacer(FromDIP(20));

    root->SetSizer(rootSizer);
    Theme::LimitWidth(root, introItem, 1392, 36);
    Theme::LimitWidth(root, gridItem, 1400, 28);
    Theme::LimitWidth(root, footerItem, 1392, 36);
    Theme::FitFrameToContent(this, root, wxSize(1180, 780));
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
        ApplyFilter(index == 0 ? wxString() : Catalog::Categories()[index - 1]);
    });
    quit->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Close(); });
    Bind(wxEVT_CLOSE_WINDOW, [](wxCloseEvent& event) {
        if (Theme::CanClosePage(event)) event.Skip();  // default handler destroys the frame
    });
    // Coming back from a product page: the cart may have changed there.
    Bind(wxEVT_SHOW, [this](wxShowEvent& event) {
        if (event.IsShown()) {
            m_opening = false;
            RefreshCartButton();
        }
        event.Skip();
    });
    RefreshCartButton();
    ApplyFilter(wxString());
}

void LauncherFrame::ApplyFilter(const wxString& category) {
    // Rebuild the grid with just the matching cards, so the remaining ones
    // close up instead of leaving holes where hidden cards used to be.
    m_root->Freeze();
    m_grid->Clear(false);  // detach, don't delete
    int shown = 0;
    for (size_t i = 0; i < m_cards.size(); ++i) {
        const bool match = category.IsEmpty() || Catalog::Products()[i].category == category;
        m_cards[i]->Show(match);
        if (match) {
            m_grid->Add(m_cards[i], 1, wxEXPAND);
            ++shown;
        }
    }
    // Keep the grid two rows tall so a short list doesn't stretch into giant cards.
    for (int filler = shown; filler < 8; ++filler) m_grid->AddStretchSpacer();
    m_count->SetLabel(category.IsEmpty()
                          ? wxString::Format(wxT("%zu 類商品・%zu 款配色・可客製姓名與背號"),
                                             Catalog::Products().size(), Catalog::Colorways().size())
                          : wxString::Format(wxT("%s・%d 項商品"), category, shown));
    m_root->Layout();
    m_root->Thaw();
}

wxWindow* LauncherFrame::MakeProductCard(wxWindow* parent, int productIndex) {
    const Product& product = Catalog::Products()[productIndex];
    Widgets::Card* card = Theme::MakeCard(parent, true);
    card->SetCursor(wxCursor(wxCURSOR_HAND));
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    const int pad = FromDIP(24);

    // The picture takes the spare height, so the grid fills a maximised or
    // full-screen window instead of leaving an empty band.
    auto* picture = Theme::ImagePanel::ForAsset(card, wxT("category_") + product.id + wxT(".png"), wxSize(210, 150));
    sizer->Add(picture, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(18));

    wxBoxSizer* titleRow = new wxBoxSizer(wxHORIZONTAL);
    titleRow->Add(Theme::MakeLabel(card, product.name, 13, true), 0, wxALIGN_BOTTOM);
    titleRow->AddStretchSpacer();
    titleRow->Add(Theme::MakeLabel(card, Theme::FormatPrice(product.price), 12, true, Theme::kOrange), 0, wxALIGN_BOTTOM);
    sizer->Add(titleRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, pad);
    sizer->AddSpacer(FromDIP(4));
    sizer->Add(Theme::MakeLabel(card, product.tagline, 10, false, Theme::kMuted), 0, wxLEFT | wxRIGHT, pad);
    sizer->AddSpacer(FromDIP(10));
    sizer->Add(Theme::MakeLabel(card, wxT("查看商品  →"), 10, true, Theme::kOrange), 0, wxLEFT | wxRIGHT | wxBOTTOM, pad);

    card->SetSizer(sizer);
    card->SetToolTip(product.englishName);

    // Every part of the card opens the product.
    auto open = [this, productIndex](wxMouseEvent&) { OpenProduct(productIndex); };
    card->Bind(wxEVT_LEFT_UP, open);
    for (wxWindow* child : card->GetChildren()) {
        child->SetCursor(wxCursor(wxCURSOR_HAND));
        child->Bind(wxEVT_LEFT_UP, open);
    }
    return card;
}

void LauncherFrame::OpenProduct(int productIndex) {
    if (m_opening) return;  // ignore double clicks while the page fades in
    m_opening = true;
    Theme::ShowLike(new ProductFrame(this, productIndex), this, [this] { Hide(); });
}

void LauncherFrame::RefreshCartButton() {
    m_cartButton->SetLabel(CartButtonLabel());
    m_cartButton->GetParent()->Layout();
}
