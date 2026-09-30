#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <functional>
#include <vector>
#include "Widgets.h"

// "購物車  3 件｜NT$3,840" — the header button on every page.
wxString CartButtonLabel();

// Step 1: the cart. Items (with thumbnails) on the left, order summary with
// coupon, free-shipping progress and checkout button on the right.
class CartDialog : public wxDialog {
public:
    explicit CartDialog(wxWindow* parent);

private:
    void RefreshCart();
    long SelectedRow() const;
    void ChangeQuantity(int delta);
    void OnRemove();
    void OnClear();
    void OnApplyCoupon();
    void OnCheckout();

    wxPanel* m_itemsPanel = nullptr;
    wxPanel* m_emptyPanel = nullptr;
    wxListView* m_list = nullptr;
    Widgets::FlatButton* m_minus = nullptr;
    Widgets::FlatButton* m_plus = nullptr;
    Widgets::FlatButton* m_remove = nullptr;
    Widgets::FlatButton* m_clear = nullptr;
    Widgets::FlatButton* m_checkout = nullptr;
    wxStaticText* m_count = nullptr;
    wxStaticText* m_subtotal = nullptr;
    wxStaticText* m_discountLabel = nullptr;
    wxStaticText* m_discount = nullptr;
    wxStaticText* m_shipping = nullptr;
    wxStaticText* m_shippingHint = nullptr;
    Widgets::ProgressBar* m_shippingBar = nullptr;
    wxTextCtrl* m_couponInput = nullptr;
    wxStaticText* m_couponMessage = nullptr;
    wxStaticText* m_total = nullptr;
};

// Step 2: shipping and payment. Each field is checked as soon as the user
// leaves it (and live after that), with the message right under the field.
class CheckoutDialog : public wxDialog {
public:
    explicit CheckoutDialog(wxWindow* parent);

    wxString RecipientName() const;
    wxString Phone() const;
    wxString Email() const;
    wxString Address() const;
    wxString DeliveryMethod() const;
    wxString PaymentMethod() const;

private:
    struct Field {
        wxTextCtrl* input;
        wxStaticText* error;
        std::function<wxString()> check;  // message, or empty when valid
        bool touched = false;
    };

    Field& AddField(wxWindow* parent, wxFlexGridSizer* grid, const wxString& label, const wxString& hint,
                    int maxLength, std::function<wxString()> check);
    bool ValidateField(Field& field);
    void OnDeliveryChanged();
    void OnConfirm();

    std::vector<Field> m_fields;
    wxRadioButton* m_homeDelivery = nullptr;
    wxRadioButton* m_storePickup = nullptr;
    wxStaticText* m_addressLabel = nullptr;
    wxChoice* m_payment = nullptr;
};

// Orders placed since the app started: list on the left, the selected order's
// items, totals and shipping details on the right.
class OrdersDialog : public wxDialog {
public:
    explicit OrdersDialog(wxWindow* parent);

private:
    void ShowOrder(long index);
    wxListView* m_list = nullptr;
    wxPanel* m_detail = nullptr;
};

// Step 3: receipt with the order number. Reads the cart, so show it before
// the cart is cleared.
class OrderCompleteDialog : public wxDialog {
public:
    OrderCompleteDialog(wxWindow* parent, const CheckoutDialog& info, const wxString& orderNumber);
};
