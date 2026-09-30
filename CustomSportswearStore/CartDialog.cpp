#include "CartDialog.h"
#include "Catalog.h"
#include "Theme.h"
#include <wx/datetime.h>
#include <wx/graphics.h>
#include <wx/imaglist.h>
#include <wx/statline.h>
#include <algorithm>
#include <memory>
#include <random>

namespace {
    enum Column { kColItem, kColSpec, kColUnit, kColQty, kColSubtotal };
    const std::vector<wxString> kSteps = { wxT("購物車"), wxT("填寫資料"), wxT("完成訂購") };

    wxString MakeOrderNumber() {
        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, 9999);
        return wxDateTime::Now().Format(wxT("CS%Y%m%d-")) + wxString::Format(wxT("%04d"), dist(rng));
    }

    // One "label ........ value" line of a price summary.
    wxStaticText* AddSummaryRow(wxWindow* parent, wxSizer* sizer, const wxString& label, int pointSize, bool bold,
                                const wxColour& valueColour, wxStaticText** labelOut = nullptr) {
        wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
        wxStaticText* l = Theme::MakeLabel(parent, label, pointSize, bold, bold ? Theme::kText : Theme::kMuted);
        row->Add(l, 0, wxALIGN_CENTER_VERTICAL);
        row->AddStretchSpacer();
        wxStaticText* value = Theme::MakeLabel(parent, wxEmptyString, pointSize, bold, valueColour);
        row->Add(value, 0, wxALIGN_CENTER_VERTICAL);
        sizer->Add(row, 0, wxEXPAND | wxTOP, parent->FromDIP(8));
        if (labelOut) *labelOut = l;
        return value;
    }

    // A header + step indicator block shared by the three checkout dialogs.
    void AddDialogTop(wxDialog* dialog, wxSizer* root, const wxString& title, const wxString& subtitle, int step) {
        root->Add(Theme::MakeHeader(dialog, title, subtitle), 0, wxEXPAND);
        root->Add(new Widgets::StepIndicator(dialog, kSteps, step), 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP,
                  dialog->FromDIP(24));
    }

    wxBitmap RenderCheckMark(int px) {
        wxBitmap bmp(px, px, 24);
        {
            wxMemoryDC dc(bmp);
            dc.SetBackground(wxBrush(Theme::kCard));
            dc.Clear();
            std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
            if (gc) {
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->SetBrush(wxBrush(wxColour(22, 140, 84, 40)));
                gc->DrawEllipse(0, 0, px, px);
                const double inset = px * 0.12;
                gc->SetBrush(wxBrush(Theme::kSuccess));
                gc->DrawEllipse(inset, inset, px - 2 * inset, px - 2 * inset);
                gc->SetPen(wxPen(*wxWHITE, std::max(2, px / 12)));
                wxGraphicsPath tick = gc->CreatePath();
                tick.MoveToPoint(px * 0.32, px * 0.52);
                tick.AddLineToPoint(px * 0.45, px * 0.64);
                tick.AddLineToPoint(px * 0.70, px * 0.38);
                gc->StrokePath(tick);
            }
        }
        return bmp;
    }

    // Empty-cart illustration: a bag outline in a soft circle.
    wxBitmap RenderEmptyBag(const wxSize& px) {
        const int d = std::min(px.x, px.y);
        wxBitmap bmp(d, d, 24);
        {
            wxMemoryDC dc(bmp);
            dc.SetBackground(wxBrush(Theme::kCard));
            dc.Clear();
            std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
            if (gc) {
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->SetBrush(wxBrush(wxColour(255, 241, 232)));
                gc->DrawEllipse(0, 0, d, d);
                gc->SetPen(wxPen(Theme::kOrange, std::max(2, d / 30)));
                gc->SetBrush(*wxTRANSPARENT_BRUSH);
                gc->DrawRoundedRectangle(d * 0.30, d * 0.40, d * 0.40, d * 0.34, d * 0.04);
                wxGraphicsPath handle = gc->CreatePath();
                handle.AddArc(d * 0.5, d * 0.40, d * 0.10, 3.14159265358979, 0, true);
                gc->StrokePath(handle);
            }
        }
        return bmp;
    }
}

wxString CartButtonLabel() {
    const ShoppingCart& cart = ShoppingCart::Get();
    return wxString::Format(wxT("  購物車  %d 件｜%s  "), cart.TotalQuantity(), Theme::FormatPrice(cart.Subtotal()));
}

// ===========================================================================
// CartDialog
// ===========================================================================
CartDialog::CartDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, wxT("購物車｜運動用品客製購物系統"), wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX) {
    SetBackgroundColour(Theme::kPage);
    wxBoxSizer* root = new wxBoxSizer(wxVERTICAL);
    AddDialogTop(this, root, wxT("購物車"), wxT("確認商品與數量，套用優惠碼後前往結帳"), 0);

    wxBoxSizer* columns = new wxBoxSizer(wxHORIZONTAL);

    // ---------------- left: items ----------------
    Widgets::Card* itemsCard = Theme::MakeCard(this);
    wxBoxSizer* itemsCardSizer = new wxBoxSizer(wxVERTICAL);

    m_itemsPanel = new wxPanel(itemsCard);
    m_itemsPanel->SetBackgroundColour(Theme::kCard);
    wxBoxSizer* items = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* titleRow = new wxBoxSizer(wxHORIZONTAL);
    titleRow->Add(Theme::MakeLabel(m_itemsPanel, wxT("購物清單"), 13, true), 0, wxALIGN_CENTER_VERTICAL);
    titleRow->AddStretchSpacer();
    m_count = Theme::MakeLabel(m_itemsPanel, wxEmptyString, 10, false, Theme::kMuted);
    titleRow->Add(m_count, 0, wxALIGN_CENTER_VERTICAL);
    items->Add(titleRow, 0, wxEXPAND | wxBOTTOM, FromDIP(10));

    m_list = new wxListView(m_itemsPanel, wxID_ANY, wxDefaultPosition, FromDIP(wxSize(-1, 240)),
                            wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_NONE);
    m_list->SetFont(Theme::Font(10));
    m_list->InsertColumn(kColItem, wxT("商品"), wxLIST_FORMAT_LEFT, FromDIP(250));
    m_list->InsertColumn(kColSpec, wxT("規格"), wxLIST_FORMAT_LEFT, FromDIP(150));
    m_list->InsertColumn(kColUnit, wxT("單價"), wxLIST_FORMAT_RIGHT, FromDIP(86));
    m_list->InsertColumn(kColQty, wxT("數量"), wxLIST_FORMAT_CENTER, FromDIP(56));
    m_list->InsertColumn(kColSubtotal, wxT("小計"), wxLIST_FORMAT_RIGHT, FromDIP(96));
    items->Add(m_list, 1, wxEXPAND);

    wxBoxSizer* actions = new wxBoxSizer(wxHORIZONTAL);
    m_minus = Theme::MakeSecondaryButton(m_itemsPanel, wxT("－"), 12);
    m_plus = Theme::MakeSecondaryButton(m_itemsPanel, wxT("＋"), 12);
    m_remove = Theme::MakeSecondaryButton(m_itemsPanel, wxT("移除"), 10);
    m_clear = Theme::MakeSecondaryButton(m_itemsPanel, wxT("清空購物車"), 10);
    for (auto* b : { m_minus, m_plus }) b->SetMinSize(FromDIP(wxSize(40, 36)));
    actions->Add(Theme::MakeLabel(m_itemsPanel, wxT("選取的商品"), 10, false, Theme::kMuted), 0, wxALIGN_CENTER_VERTICAL);
    actions->Add(m_minus, 0, wxLEFT, FromDIP(10));
    actions->Add(m_plus, 0, wxLEFT, FromDIP(6));
    actions->Add(m_remove, 0, wxLEFT, FromDIP(10));
    actions->AddStretchSpacer();
    actions->Add(m_clear);
    items->Add(actions, 0, wxEXPAND | wxTOP, FromDIP(12));
    m_itemsPanel->SetSizer(items);
    itemsCardSizer->Add(m_itemsPanel, 1, wxEXPAND | wxALL, FromDIP(24));

    // empty state
    m_emptyPanel = new wxPanel(itemsCard);
    m_emptyPanel->SetBackgroundColour(Theme::kCard);
    wxBoxSizer* empty = new wxBoxSizer(wxVERTICAL);
    empty->AddStretchSpacer();
    empty->Add(new Theme::ImagePanel(m_emptyPanel, wxSize(120, 120), RenderEmptyBag), 0, wxALIGN_CENTER);
    empty->Add(Theme::MakeLabel(m_emptyPanel, wxT("購物車還是空的"), 15, true), 0, wxALIGN_CENTER | wxTOP, FromDIP(16));
    empty->Add(Theme::MakeLabel(m_emptyPanel, wxT("挑幾件喜歡的配色，印上你的名字吧"), 10, false, Theme::kMuted),
               0, wxALIGN_CENTER | wxTOP, FromDIP(6));
    auto* browse = Theme::MakePrimaryButton(m_emptyPanel, wxT("去逛逛"), 12);
    browse->ShowArrow();
    empty->Add(browse, 0, wxALIGN_CENTER | wxTOP, FromDIP(20));
    empty->AddStretchSpacer();
    m_emptyPanel->SetSizer(empty);
    itemsCardSizer->Add(m_emptyPanel, 1, wxEXPAND | wxALL, FromDIP(24));
    itemsCard->SetSizer(itemsCardSizer);
    columns->Add(itemsCard, 1, wxEXPAND | wxRIGHT, FromDIP(4));

    // ---------------- right: summary ----------------
    Widgets::Card* summaryCard = Theme::MakeCard(this);
    wxBoxSizer* summary = new wxBoxSizer(wxVERTICAL);
    summary->Add(Theme::MakeLabel(summaryCard, wxT("訂單摘要"), 13, true));
    m_subtotal = AddSummaryRow(summaryCard, summary, wxT("商品小計"), 11, false, Theme::kText);
    m_discount = AddSummaryRow(summaryCard, summary, wxT("優惠折扣"), 11, false, Theme::kSuccess, &m_discountLabel);
    m_shipping = AddSummaryRow(summaryCard, summary, wxT("運費"), 11, false, Theme::kText);

    m_shippingBar = new Widgets::ProgressBar(summaryCard);
    summary->Add(m_shippingBar, 0, wxEXPAND | wxTOP, FromDIP(12));
    m_shippingHint = Theme::MakeLabel(summaryCard, wxEmptyString, 9, false, Theme::kMuted);
    summary->Add(m_shippingHint, 0, wxTOP, FromDIP(6));

    summary->Add(new wxStaticLine(summaryCard), 0, wxEXPAND | wxTOP | wxBOTTOM, FromDIP(14));
    summary->Add(Theme::MakeLabel(summaryCard, wxT("優惠碼"), 10, true));
    wxBoxSizer* couponRow = new wxBoxSizer(wxHORIZONTAL);
    m_couponInput = new wxTextCtrl(summaryCard, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                   wxTE_PROCESS_ENTER);
    m_couponInput->SetFont(Theme::Font(11));
    m_couponInput->SetHint(wxT("例如 WELCOME100"));
    auto* apply = Theme::MakeSecondaryButton(summaryCard, wxT("套用"), 10);
    apply->SetMinSize(FromDIP(wxSize(64, 34)));
    couponRow->Add(m_couponInput, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(8));
    couponRow->Add(apply, 0, wxALIGN_CENTER_VERTICAL);
    summary->Add(couponRow, 0, wxEXPAND | wxTOP, FromDIP(6));
    m_couponMessage = Theme::MakeLabel(summaryCard, wxT("可用：WELCOME100・TEAM10"), 9, false, Theme::kMuted);
    summary->Add(m_couponMessage, 0, wxTOP, FromDIP(6));

    summary->Add(new wxStaticLine(summaryCard), 0, wxEXPAND | wxTOP, FromDIP(14));
    m_total = AddSummaryRow(summaryCard, summary, wxT("應付總額"), 15, true, Theme::kOrange);
    summary->AddStretchSpacer();
    m_checkout = Theme::MakePrimaryButton(summaryCard, wxT("前往結帳"), 12);
    m_checkout->ShowArrow();
    summary->Add(m_checkout, 0, wxEXPAND | wxTOP, FromDIP(16));
    auto* keepShopping = Theme::MakeSecondaryButton(summaryCard, wxT("繼續購物"), 10);
    summary->Add(keepShopping, 0, wxEXPAND | wxTOP, FromDIP(8));
    wxBoxSizer* summaryPad = new wxBoxSizer(wxVERTICAL);
    summaryPad->Add(summary, 1, wxEXPAND | wxALL, FromDIP(24));
    summaryCard->SetSizer(summaryPad);
    summaryCard->SetMinSize(FromDIP(wxSize(330, -1)));
    columns->Add(summaryCard, 0, wxEXPAND);

    root->Add(columns, 1, wxEXPAND | wxALL, FromDIP(16));
    SetSizer(root);
    // Size with only one of the two left-hand panels (items / empty) visible.
    m_itemsPanel->Show(!ShoppingCart::Get().IsEmpty());
    m_emptyPanel->Show(ShoppingCart::Get().IsEmpty());
    const wxSize needed = root->ComputeFittingClientSize(this);
    const wxSize preferred = FromDIP(wxSize(1060, 660));
    SetMinClientSize(needed);
    SetClientSize(wxSize(std::max(needed.x, preferred.x), std::max(needed.y, preferred.y)));
    CentreOnParent();

    m_minus->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ChangeQuantity(-1); });
    m_plus->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ChangeQuantity(+1); });
    m_remove->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnRemove(); });
    m_clear->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnClear(); });
    apply->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnApplyCoupon(); });
    m_couponInput->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { OnApplyCoupon(); });
    m_checkout->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnCheckout(); });
    keepShopping->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CANCEL); });
    browse->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CANCEL); });
    m_list->Bind(wxEVT_LIST_ITEM_SELECTED, [this](wxListEvent&) { RefreshCart(); });
    m_list->Bind(wxEVT_LIST_ITEM_DESELECTED, [this](wxListEvent&) { RefreshCart(); });
    m_list->Bind(wxEVT_KEY_DOWN, [this](wxKeyEvent& e) {
        if (e.GetKeyCode() == WXK_DELETE) OnRemove();
        else e.Skip();
    });
    // The spec column takes the spare width so the table always fills the card.
    m_list->Bind(wxEVT_SIZE, [this](wxSizeEvent& event) {
        int others = 0;
        for (int col = 0; col < m_list->GetColumnCount(); ++col)
            if (col != kColSpec) others += m_list->GetColumnWidth(col);
        m_list->SetColumnWidth(kColSpec, std::max(FromDIP(120), m_list->GetClientSize().x - others - FromDIP(4)));
        event.Skip();
    });

    if (const Coupon* c = ShoppingCart::Get().AppliedCoupon()) m_couponInput->SetValue(c->code);
    RefreshCart();
    if (m_list->GetItemCount() > 0) m_list->Select(0);
}

long CartDialog::SelectedRow() const {
    return m_list->GetFirstSelected();
}

void CartDialog::RefreshCart() {
    ShoppingCart& cart = ShoppingCart::Get();
    const auto& items = cart.Items();
    const long selected = SelectedRow();

    const bool empty = cart.IsEmpty();
    m_itemsPanel->Show(!empty);
    m_emptyPanel->Show(empty);

    // Thumbnails are rebuilt with the rows; carts are small, so this is cheap.
    if (m_list->GetItemCount() != (int)items.size()) {
        const int thumb = FromDIP(52);
        wxImageList* images = new wxImageList(thumb, thumb, false);
        for (const CartItem& item : items) {
            wxImage art = Theme::LoadFittedPixels(item.GetProduct().id + wxT("_") + item.GetColorway().id + wxT(".png"),
                                                  wxSize(thumb, thumb)).ConvertToImage();
            // pad to a centred square so every row lines up
            images->Add(wxBitmap(art.Size(wxSize(thumb, thumb),
                                          wxPoint((thumb - art.GetWidth()) / 2, (thumb - art.GetHeight()) / 2), 255, 255, 255)));
        }
        m_list->AssignImageList(images, wxIMAGE_LIST_SMALL);
        m_list->DeleteAllItems();
        for (size_t i = 0; i < items.size(); ++i) m_list->InsertItem((long)i, wxEmptyString, (int)i);
    }
    for (size_t i = 0; i < items.size(); ++i) {
        const CartItem& item = items[i];
        m_list->SetItem((long)i, kColItem, wxT("  ") + item.Title(), (int)i);
        m_list->SetItem((long)i, kColSpec, item.spec);
        m_list->SetItem((long)i, kColUnit, Theme::FormatPrice(item.unitPrice));
        m_list->SetItem((long)i, kColQty, wxString::Format(wxT("%d"), item.quantity));
        m_list->SetItem((long)i, kColSubtotal, Theme::FormatPrice(item.Subtotal()));
    }

    const bool hasSelection = selected >= 0 && selected < (long)items.size();
    m_minus->Enable(hasSelection && items[selected].quantity > 1);
    m_plus->Enable(hasSelection && items[selected].quantity < ShoppingCart::kMaxQuantityPerLine);
    m_remove->Enable(hasSelection);
    m_clear->Enable(!empty);
    m_checkout->Enable(!empty);
    m_count->SetLabel(wxString::Format(wxT("共 %d 件"), cart.TotalQuantity()));

    m_subtotal->SetLabel(Theme::FormatPrice(cart.Subtotal()));
    const bool hasDiscount = cart.Discount() > 0;
    m_discount->SetLabel(hasDiscount ? wxT("-") + Theme::FormatPrice(cart.Discount()) : wxString(wxT("—")));
    m_discountLabel->SetLabel(hasDiscount ? wxT("優惠折扣（") + cart.AppliedCoupon()->code + wxT("）") : wxString(wxT("優惠折扣")));
    m_shipping->SetLabel(empty ? wxString(wxT("—"))
                               : cart.ShippingFee() == 0 ? wxString(wxT("免運費")) : Theme::FormatPrice(cart.ShippingFee()));
    m_shippingBar->SetValue((double)cart.Subtotal() / Catalog::kFreeShippingThreshold);
    if (empty) {
        m_shippingHint->SetForegroundColour(Theme::kMuted);
        m_shippingHint->SetLabel(wxString::Format(wxT("滿 %s 免運費"), Theme::FormatPrice(Catalog::kFreeShippingThreshold)));
    } else if (cart.AmountToFreeShipping() > 0) {
        m_shippingHint->SetForegroundColour(Theme::kOrangeDark);
        m_shippingHint->SetLabel(wxString::Format(wxT("再買 %s 就免運費"), Theme::FormatPrice(cart.AmountToFreeShipping())));
    } else {
        m_shippingHint->SetForegroundColour(Theme::kSuccess);
        m_shippingHint->SetLabel(wxT("已達免運門檻"));
    }
    // A coupon applied earlier may stop qualifying after the cart changes.
    if (const Coupon* c = cart.AppliedCoupon()) {
        const wxString problem = cart.CouponProblem(*c);
        m_couponMessage->SetForegroundColour(problem.IsEmpty() ? Theme::kSuccess : Theme::kOrangeDark);
        m_couponMessage->SetLabel(problem.IsEmpty() ? c->description : wxT("暫不適用：") + problem);
    }
    m_total->SetLabel(Theme::FormatPrice(cart.Total()));
    Layout();
}

void CartDialog::ChangeQuantity(int delta) {
    const long row = SelectedRow();
    if (row < 0) return;
    ShoppingCart& cart = ShoppingCart::Get();
    cart.SetQuantity((size_t)row, cart.Items()[row].quantity + delta);
    RefreshCart();
}

void CartDialog::OnRemove() {
    const long row = SelectedRow();
    if (row < 0) return;
    const CartItem& item = ShoppingCart::Get().Items()[row];
    if (wxMessageBox(wxString::Format(wxT("要從購物車移除這項商品嗎？\n\n%s（%s）"), item.Title(), item.spec),
                     wxT("移除商品"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION, this) != wxYES)
        return;
    ShoppingCart::Get().RemoveAt((size_t)row);
    RefreshCart();
    if (m_list->GetItemCount() > 0) m_list->Select(std::min<long>(row, m_list->GetItemCount() - 1));
}

void CartDialog::OnClear() {
    if (wxMessageBox(wxT("確定要清空購物車嗎？"), wxT("清空購物車"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION, this) != wxYES)
        return;
    ShoppingCart::Get().Clear();
    m_couponInput->Clear();
    m_couponMessage->SetForegroundColour(Theme::kMuted);
    m_couponMessage->SetLabel(wxT("可用：WELCOME100・TEAM10"));
    RefreshCart();
}

void CartDialog::OnApplyCoupon() {
    ShoppingCart& cart = ShoppingCart::Get();
    const wxString code = m_couponInput->GetValue().Trim().Trim(false);
    if (code.IsEmpty()) {
        cart.RemoveCoupon();
        m_couponMessage->SetForegroundColour(Theme::kMuted);
        m_couponMessage->SetLabel(wxT("已取消優惠碼"));
    } else {
        const wxString problem = cart.ApplyCoupon(code);
        m_couponMessage->SetForegroundColour(problem.IsEmpty() ? Theme::kSuccess : Theme::kError);
        m_couponMessage->SetLabel(problem.IsEmpty() ? wxT("已套用：") + cart.AppliedCoupon()->description : problem);
        if (problem.IsEmpty()) m_couponInput->SetValue(cart.AppliedCoupon()->code);
    }
    RefreshCart();
}

void CartDialog::OnCheckout() {
    CheckoutDialog checkout(this);
    if (checkout.ShowModal() != wxID_OK) return;

    const ShoppingCart& cart = ShoppingCart::Get();
    OrderRecord order{ MakeOrderNumber(), wxDateTime::Now(), cart.Items(),
                       cart.Subtotal(), cart.Discount(), cart.ShippingFee(), cart.Total(),
                       cart.Discount() > 0 ? cart.AppliedCoupon()->code : wxString(),
                       checkout.RecipientName(), checkout.Phone(), checkout.DeliveryMethod(),
                       checkout.Address(), checkout.PaymentMethod() };
    OrderHistory::Get().Add(order);

    OrderCompleteDialog done(this, checkout, order.number);
    done.ShowModal();
    ShoppingCart::Get().Clear();
    EndModal(wxID_OK);
}

// ===========================================================================
// CheckoutDialog
// ===========================================================================
CheckoutDialog::CheckoutDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, wxT("填寫收件資料｜運動用品客製購物系統")) {
    SetBackgroundColour(Theme::kPage);
    const ShoppingCart& cart = ShoppingCart::Get();
    m_fields.reserve(4);  // Field references and callbacks must stay valid

    wxBoxSizer* root = new wxBoxSizer(wxVERTICAL);
    AddDialogTop(this, root, wxT("填寫收件資料"),
                 wxString::Format(wxT("共 %d 件商品・應付總額 %s"), cart.TotalQuantity(), Theme::FormatPrice(cart.Total())), 1);

    Widgets::Card* card = Theme::MakeCard(this);
    wxFlexGridSizer* grid = new wxFlexGridSizer(2, FromDIP(4), FromDIP(16));
    grid->AddGrowableCol(1, 1);

    AddField(card, grid, wxT("收件人"), wxT("收件人真實姓名"), 20, [this] {
        return RecipientName().IsEmpty() ? wxString(wxT("請填寫收件人姓名")) : wxString();
    });
    AddField(card, grid, wxT("手機號碼"), wxT("09 開頭的 10 碼手機號碼"), 10, [this] {
        const wxString p = Phone();
        if (p.IsEmpty()) return wxString(wxT("請填寫手機號碼"));
        if (p.length() != 10 || !p.StartsWith(wxT("09")) || !p.IsNumber())
            return wxString(wxT("格式不正確，請輸入 09 開頭的 10 碼數字"));
        return wxString();
    });
    AddField(card, grid, wxT("電子信箱"), wxT("選填，寄送訂單確認信"), 60, [this] {
        const wxString e = Email();
        if (e.IsEmpty()) return wxString();
        const int at = e.Find(wxT('@'));
        if (at <= 0 || e.find(wxT('.'), at) == wxString::npos || e.EndsWith(wxT(".")))
            return wxString(wxT("格式不正確，例如 name@example.com"));
        return wxString();
    });

    // delivery method
    grid->Add(Theme::MakeLabel(card, wxT("配送方式"), 11, false, Theme::kMuted), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    wxPanel* delivery = new wxPanel(card);
    delivery->SetBackgroundColour(Theme::kCard);
    wxBoxSizer* deliveryRow = new wxBoxSizer(wxHORIZONTAL);
    m_homeDelivery = new wxRadioButton(delivery, wxID_ANY, wxT("宅配到府"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    m_storePickup = new wxRadioButton(delivery, wxID_ANY, wxT("超商取貨"));
    for (auto* rb : { m_homeDelivery, m_storePickup }) rb->SetFont(Theme::Font(11));
    m_homeDelivery->SetValue(true);
    deliveryRow->Add(m_homeDelivery);
    deliveryRow->Add(m_storePickup, 0, wxLEFT, FromDIP(24));
    delivery->SetSizer(deliveryRow);
    grid->Add(delivery, 0, wxTOP | wxBOTTOM, FromDIP(10));

    AddField(card, grid, wxT("收件地址"), wxEmptyString, 80, [this] {
        if (Address().length() >= 5) return wxString();
        return wxString(m_storePickup->GetValue() ? wxT("請填寫完整的取貨門市") : wxT("請填寫完整的收件地址"));
    });
    // AddField put the label in the cell just before the input.
    m_addressLabel = wxDynamicCast(grid->GetItem(grid->GetItemCount() - 4)->GetWindow(), wxStaticText);

    grid->Add(Theme::MakeLabel(card, wxT("付款方式"), 11, false, Theme::kMuted), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    m_payment = new wxChoice(card, wxID_ANY);
    m_payment->SetFont(Theme::Font(11));
    grid->Add(m_payment, 0, wxEXPAND | wxTOP | wxBOTTOM, FromDIP(6));

    wxBoxSizer* cardSizer = new wxBoxSizer(wxVERTICAL);
    cardSizer->Add(grid, 1, wxEXPAND | wxALL, FromDIP(28));
    card->SetSizer(cardSizer);
    root->Add(card, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(16));

    wxBoxSizer* footer = new wxBoxSizer(wxHORIZONTAL);
    auto* back = Theme::MakeSecondaryButton(this, wxT("返回購物車"), 11);
    auto* confirm = Theme::MakePrimaryButton(this, wxT("確認下單"), 12);
    confirm->ShowArrow();
    footer->Add(Theme::MakeLabel(this, wxT("本系統為課堂專題展示，不會實際收款或寄送"), 9, false, Theme::kMuted),
                0, wxALIGN_CENTER_VERTICAL);
    footer->AddStretchSpacer();
    footer->Add(back, 0, wxALIGN_CENTER_VERTICAL);
    footer->Add(confirm, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(12));
    root->Add(footer, 0, wxEXPAND | wxALL, FromDIP(24));

    SetSizer(root);
    SetMinClientSize(wxSize(FromDIP(620), -1));
    Fit();
    CentreOnParent();

    m_homeDelivery->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent&) { OnDeliveryChanged(); });
    m_storePickup->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent&) { OnDeliveryChanged(); });
    confirm->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnConfirm(); });
    back->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CANCEL); });
    OnDeliveryChanged();
    m_fields.front().input->SetFocus();
}

CheckoutDialog::Field& CheckoutDialog::AddField(wxWindow* parent, wxFlexGridSizer* grid, const wxString& label,
                                                const wxString& hint, int maxLength, std::function<wxString()> check) {
    // Same top border as the input so the two line up.
    grid->Add(Theme::MakeLabel(parent, label, 11, false, Theme::kMuted), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT | wxTOP, FromDIP(6));
    wxTextCtrl* input = new wxTextCtrl(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, FromDIP(wxSize(340, -1)));
    input->SetFont(Theme::Font(11));
    input->SetHint(hint);
    input->SetMaxLength(maxLength);
    grid->Add(input, 1, wxEXPAND | wxTOP, FromDIP(6));
    grid->AddSpacer(0);
    wxStaticText* error = Theme::MakeLabel(parent, wxT(" "), 9, false, Theme::kError);
    grid->Add(error, 0, wxTOP, FromDIP(2));

    m_fields.push_back({ input, error, std::move(check) });
    const size_t index = m_fields.size() - 1;
    // Check a field once the user leaves it, then keep it live so the message
    // disappears as soon as the input is fixed.
    input->Bind(wxEVT_KILL_FOCUS, [this, index](wxFocusEvent& e) {
        m_fields[index].touched = true;
        ValidateField(m_fields[index]);
        e.Skip();
    });
    input->Bind(wxEVT_TEXT, [this, index](wxCommandEvent&) {
        if (m_fields[index].touched) ValidateField(m_fields[index]);
    });
    return m_fields.back();
}

bool CheckoutDialog::ValidateField(Field& field) {
    const wxString message = field.check();
    field.error->SetForegroundColour(message.IsEmpty() ? Theme::kSuccess : Theme::kError);
    field.error->SetLabel(message.IsEmpty() ? (field.input->IsEmpty() ? wxString(wxT(" ")) : wxString(wxT("✓")))
                                            : wxT("⚠ ") + message);
    Layout();
    return message.IsEmpty();
}

namespace {
    wxString Trimmed(const wxTextCtrl* ctrl) {
        wxString v = ctrl->GetValue();
        return v.Trim().Trim(false);
    }
}

wxString CheckoutDialog::RecipientName() const { return Trimmed(m_fields[0].input); }
wxString CheckoutDialog::Phone() const { return Trimmed(m_fields[1].input); }
wxString CheckoutDialog::Email() const { return Trimmed(m_fields[2].input); }
wxString CheckoutDialog::Address() const { return Trimmed(m_fields[3].input); }

wxString CheckoutDialog::DeliveryMethod() const {
    return m_storePickup->GetValue() ? wxT("超商取貨") : wxT("宅配到府");
}

wxString CheckoutDialog::PaymentMethod() const {
    return m_payment->GetStringSelection();
}

void CheckoutDialog::OnDeliveryChanged() {
    const bool store = m_storePickup->GetValue();
    if (m_addressLabel) m_addressLabel->SetLabel(store ? wxT("取貨門市") : wxT("收件地址"));
    m_fields[3].input->SetHint(store ? wxT("超商與門市名稱，例如：台北車站門市") : wxT("縣市、區、路名與門牌號碼"));

    // Paying on delivery is called 取貨付款 at a convenience store.
    const int previous = m_payment->GetSelection();
    m_payment->Clear();
    m_payment->Append(store ? wxT("取貨付款") : wxT("貨到付款"));
    m_payment->Append(wxT("ATM 轉帳"));
    m_payment->SetSelection(previous == wxNOT_FOUND ? 0 : previous);
    if (m_fields[3].touched) ValidateField(m_fields[3]);
    Layout();
}

void CheckoutDialog::OnConfirm() {
    Field* firstBad = nullptr;
    for (Field& f : m_fields) {
        f.touched = true;
        if (!ValidateField(f) && !firstBad) firstBad = &f;
    }
    if (firstBad) {
        firstBad->input->SetFocus();
        firstBad->input->SelectAll();
        wxBell();
        return;
    }
    EndModal(wxID_OK);
}

// ===========================================================================
// OrderCompleteDialog
// ===========================================================================
OrderCompleteDialog::OrderCompleteDialog(wxWindow* parent, const CheckoutDialog& info, const wxString& orderNumber)
    : wxDialog(parent, wxID_ANY, wxT("訂購完成｜運動用品客製購物系統")) {
    SetBackgroundColour(Theme::kPage);
    const ShoppingCart& cart = ShoppingCart::Get();
    wxBoxSizer* root = new wxBoxSizer(wxVERTICAL);
    AddDialogTop(this, root, wxT("訂購完成"), wxT("感謝您的訂購"), 2);

    Widgets::Card* card = Theme::MakeCard(this);
    wxBoxSizer* body = new wxBoxSizer(wxVERTICAL);
    body->Add(new Theme::ImagePanel(card, wxSize(72, 72), [](const wxSize& px) { return RenderCheckMark(std::min(px.x, px.y)); }),
              0, wxALIGN_CENTER);
    body->Add(Theme::MakeLabel(card, wxT("訂單已成立，謝謝您的購買！"), 16, true), 0, wxALIGN_CENTER | wxTOP, FromDIP(12));
    body->Add(Theme::MakeLabel(card, wxT("訂單編號  ") + orderNumber, 12, true, Theme::kOrange), 0, wxALIGN_CENTER | wxTOP, FromDIP(4));

    // items + totals on a pale panel
    wxPanel* box = new wxPanel(card);
    box->SetBackgroundColour(wxColour(246, 248, 251));
    wxBoxSizer* lines = new wxBoxSizer(wxVERTICAL);
    for (const CartItem& item : cart.Items()) {
        wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(Theme::MakeLabel(box, wxString::Format(wxT("%s（%s）× %d"), item.Title(), item.spec, item.quantity), 10),
                 1, wxALIGN_CENTER_VERTICAL);
        row->Add(Theme::MakeLabel(box, Theme::FormatPrice(item.Subtotal()), 10), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(16));
        lines->Add(row, 0, wxEXPAND | wxBOTTOM, FromDIP(6));
    }
    auto addLine = [&](const wxString& label, const wxString& value, const wxColour& colour) {
        wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(Theme::MakeLabel(box, label, 10, false, Theme::kMuted), 1);
        row->Add(Theme::MakeLabel(box, value, 10, false, colour));
        lines->Add(row, 0, wxEXPAND | wxBOTTOM, FromDIP(6));
    };
    if (cart.Discount() > 0)
        addLine(wxT("優惠折扣（") + cart.AppliedCoupon()->code + wxT("）"), wxT("-") + Theme::FormatPrice(cart.Discount()), Theme::kSuccess);
    addLine(wxT("運費"), cart.ShippingFee() == 0 ? wxString(wxT("免運費")) : Theme::FormatPrice(cart.ShippingFee()), Theme::kMuted);
    wxBoxSizer* totalRow = new wxBoxSizer(wxHORIZONTAL);
    totalRow->Add(Theme::MakeLabel(box, wxT("應付總額"), 12, true), 1, wxALIGN_CENTER_VERTICAL);
    totalRow->Add(Theme::MakeLabel(box, Theme::FormatPrice(cart.Total()), 14, true, Theme::kOrange), 0, wxALIGN_CENTER_VERTICAL);
    lines->Add(totalRow, 0, wxEXPAND | wxTOP, FromDIP(2));
    wxBoxSizer* boxPad = new wxBoxSizer(wxVERTICAL);
    boxPad->Add(lines, 1, wxEXPAND | wxALL, FromDIP(16));
    box->SetSizer(boxPad);
    body->Add(box, 0, wxEXPAND | wxTOP, FromDIP(18));

    wxFlexGridSizer* details = new wxFlexGridSizer(2, FromDIP(8), FromDIP(16));
    auto addDetail = [&](const wxString& label, const wxString& value) {
        details->Add(Theme::MakeLabel(card, label, 10, false, Theme::kMuted));
        details->Add(Theme::MakeLabel(card, value, 10));
    };
    const bool store = info.DeliveryMethod() == wxT("超商取貨");
    addDetail(wxT("收件人"), info.RecipientName() + wxT("（") + info.Phone() + wxT("）"));
    addDetail(wxT("配送方式"), info.DeliveryMethod());
    addDetail(store ? wxT("取貨門市") : wxT("收件地址"), info.Address());
    addDetail(wxT("付款方式"), info.PaymentMethod());
    if (!info.Email().IsEmpty()) addDetail(wxT("確認信"), info.Email());
    body->Add(details, 0, wxTOP, FromDIP(18));

    const wxString note = info.PaymentMethod() == wxT("ATM 轉帳")
        ? wxT("轉帳帳號會以簡訊傳送，請於 3 日內完成付款，確認入帳後出貨。")
        : wxT("商品將於 3–5 個工作天內出貨，出貨時會以簡訊通知您。");
    body->Add(Theme::MakeLabel(card, note, 10, false, Theme::kMuted), 0, wxTOP, FromDIP(16));

    wxBoxSizer* cardPad = new wxBoxSizer(wxVERTICAL);
    cardPad->Add(body, 1, wxEXPAND | wxALL, FromDIP(30));
    card->SetSizer(cardPad);
    root->Add(card, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(16));

    auto* ok = Theme::MakePrimaryButton(this, wxT("完成"), 12);
    ok->SetId(wxID_OK);
    root->Add(ok, 0, wxEXPAND | wxALL, FromDIP(24));
    SetSizer(root);
    SetMinClientSize(wxSize(FromDIP(540), -1));
    Fit();
    CentreOnParent();
    ok->SetFocus();
}

// ===========================================================================
// OrdersDialog
// ===========================================================================
OrdersDialog::OrdersDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, wxT("我的訂單｜運動用品客製購物系統"), wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX) {
    SetBackgroundColour(Theme::kPage);
    const auto& orders = OrderHistory::Get().Orders();
    wxBoxSizer* root = new wxBoxSizer(wxVERTICAL);
    root->Add(Theme::MakeHeader(this, wxT("我的訂單"),
                                orders.empty() ? wxString(wxT("這次開啟程式後完成的訂單會列在這裡"))
                                               : wxString::Format(wxT("共 %zu 筆訂單"), orders.size())),
              0, wxEXPAND);

    wxBoxSizer* columns = new wxBoxSizer(wxHORIZONTAL);
    if (orders.empty()) {
        Widgets::Card* card = Theme::MakeCard(this);
        wxBoxSizer* empty = new wxBoxSizer(wxVERTICAL);
        empty->AddStretchSpacer();
        empty->Add(new Theme::ImagePanel(card, wxSize(110, 110), RenderEmptyBag), 0, wxALIGN_CENTER);
        empty->Add(Theme::MakeLabel(card, wxT("還沒有訂單"), 15, true), 0, wxALIGN_CENTER | wxTOP, FromDIP(14));
        empty->Add(Theme::MakeLabel(card, wxT("完成第一筆訂購後，訂單明細會出現在這裡"), 10, false, Theme::kMuted),
                   0, wxALIGN_CENTER | wxTOP, FromDIP(6));
        auto* browse = Theme::MakePrimaryButton(card, wxT("去逛逛"), 12);
        browse->ShowArrow();
        empty->Add(browse, 0, wxALIGN_CENTER | wxTOP, FromDIP(18));
        empty->AddStretchSpacer();
        card->SetSizer(empty);
        card->SetMinSize(FromDIP(wxSize(560, 380)));
        columns->Add(card, 1, wxEXPAND);
        browse->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CANCEL); });
    } else {
        Widgets::Card* listCard = Theme::MakeCard(this);
        wxBoxSizer* listSizer = new wxBoxSizer(wxVERTICAL);
        m_list = new wxListView(listCard, wxID_ANY, wxDefaultPosition, FromDIP(wxSize(470, 360)),
                                wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_NONE);
        m_list->SetFont(Theme::Font(10));
        m_list->InsertColumn(0, wxT("訂單編號"), wxLIST_FORMAT_LEFT, FromDIP(150));
        m_list->InsertColumn(1, wxT("時間"), wxLIST_FORMAT_LEFT, FromDIP(120));
        m_list->InsertColumn(2, wxT("件數"), wxLIST_FORMAT_CENTER, FromDIP(56));
        m_list->InsertColumn(3, wxT("金額"), wxLIST_FORMAT_RIGHT, FromDIP(110));
        for (size_t i = 0; i < orders.size(); ++i) {
            m_list->InsertItem((long)i, orders[i].number);
            m_list->SetItem((long)i, 1, orders[i].placedAt.Format(wxT("%m/%d %H:%M")));
            m_list->SetItem((long)i, 2, wxString::Format(wxT("%d"), orders[i].TotalQuantity()));
            m_list->SetItem((long)i, 3, Theme::FormatPrice(orders[i].total));
        }
        listSizer->Add(Theme::MakeLabel(listCard, wxT("訂單列表"), 13, true), 0, wxBOTTOM, FromDIP(10));
        listSizer->Add(m_list, 1, wxEXPAND);
        wxBoxSizer* listPad = new wxBoxSizer(wxVERTICAL);
        listPad->Add(listSizer, 1, wxEXPAND | wxALL, FromDIP(24));
        listCard->SetSizer(listPad);
        columns->Add(listCard, 1, wxEXPAND | wxRIGHT, FromDIP(4));

        Widgets::Card* detailCard = Theme::MakeCard(this);
        m_detail = new wxPanel(detailCard);
        m_detail->SetBackgroundColour(Theme::kCard);
        wxBoxSizer* detailPad = new wxBoxSizer(wxVERTICAL);
        detailPad->Add(m_detail, 1, wxEXPAND | wxALL, FromDIP(24));
        detailCard->SetSizer(detailPad);
        detailCard->SetMinSize(FromDIP(wxSize(420, -1)));
        columns->Add(detailCard, 0, wxEXPAND);

        m_list->Bind(wxEVT_LIST_ITEM_SELECTED, [this](wxListEvent& e) { ShowOrder(e.GetIndex()); });
    }
    root->Add(columns, 1, wxEXPAND | wxALL, FromDIP(16));

    auto* close = Theme::MakeSecondaryButton(this, wxT("關閉"), 11);
    close->SetMinSize(FromDIP(wxSize(120, 40)));
    root->Add(close, 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(24));
    close->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_OK); });

    SetSizer(root);
    if (m_list) {
        m_list->Select(0);
        ShowOrder(0);
    }
    Fit();
    SetMinClientSize(GetClientSize());
    CentreOnParent();
}

void OrdersDialog::ShowOrder(long index) {
    const auto& orders = OrderHistory::Get().Orders();
    if (!m_detail || index < 0 || index >= (long)orders.size()) return;
    const OrderRecord& o = orders[index];

    m_detail->Freeze();
    m_detail->DestroyChildren();
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
    s->Add(Theme::MakeLabel(m_detail, o.number, 14, true, Theme::kOrange));
    s->Add(Theme::MakeLabel(m_detail, o.placedAt.Format(wxT("%Y/%m/%d %H:%M")), 10, false, Theme::kMuted), 0, wxTOP, FromDIP(2));
    s->Add(new wxStaticLine(m_detail), 0, wxEXPAND | wxTOP | wxBOTTOM, FromDIP(12));

    auto row = [&](const wxString& left, const wxString& right, const wxColour& colour, bool bold = false,
                   bool mutedLabel = true) {
        wxBoxSizer* r = new wxBoxSizer(wxHORIZONTAL);
        r->Add(Theme::MakeLabel(m_detail, left, bold ? 12 : 10, bold, bold || !mutedLabel ? Theme::kText : Theme::kMuted),
               1, wxALIGN_CENTER_VERTICAL);
        r->Add(Theme::MakeLabel(m_detail, right, bold ? 13 : 10, bold, colour), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(12));
        s->Add(r, 0, wxEXPAND | wxBOTTOM, FromDIP(6));
    };
    for (const CartItem& item : o.items)
        row(wxString::Format(wxT("%s（%s）× %d"), item.Title(), item.spec, item.quantity), Theme::FormatPrice(item.Subtotal()),
            Theme::kText, false, false);
    if (o.discount > 0)
        row(wxT("優惠折扣（") + o.couponCode + wxT("）"), wxT("-") + Theme::FormatPrice(o.discount), Theme::kSuccess);
    row(wxT("運費"), o.shipping == 0 ? wxString(wxT("免運費")) : Theme::FormatPrice(o.shipping), Theme::kMuted);
    row(wxT("應付總額"), Theme::FormatPrice(o.total), Theme::kOrange, true);

    s->Add(new wxStaticLine(m_detail), 0, wxEXPAND | wxTOP | wxBOTTOM, FromDIP(10));
    row(wxT("收件人"), o.recipient + wxT("（") + o.phone + wxT("）"), Theme::kText);
    row(wxT("配送方式"), o.delivery, Theme::kText);
    row(o.delivery == wxT("超商取貨") ? wxT("取貨門市") : wxT("收件地址"), o.address, Theme::kText);
    row(wxT("付款方式"), o.payment, Theme::kText);
    m_detail->SetSizer(s, true);
    m_detail->Layout();
    m_detail->Thaw();
    Layout();
}
