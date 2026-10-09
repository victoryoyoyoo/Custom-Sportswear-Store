#pragma once
#include <wx/wx.h>
#include "Lang.h"
#include <wx/datetime.h>
#include <set>
#include <vector>

struct Colorway {
    wxString id;
    wxString name;
    wxString englishName;
    wxColour fabric;
    wxColour trim;
};

struct SizeOption {
    wxString label;
    wxString hint;
};

struct PrintArea {
    double centerX = 0, top = 0, maxWidth = 0, height = 0;
    bool IsSet() const { return height > 0; }
};

enum class SizeAdvice { None, Apparel, Shoes };

enum class Personalization {
    None,
    NameAndNumber,
    Number,
    Text,
};

enum class Shape {
    Flat,
    Basketball,
    SoccerBall,
    Cap,
    Round,
};

struct RoundShape {
    double centerX = 0, radius = 0;
    double sag = 0;
    double emblemY = 0, emblemSize = 0;
    double textY = 0, textHeight = 0, textWidth = 0;
    double textTurn = 0;
};

struct Product {
    wxString id;
    wxString category;
    wxString name;
    wxString englishName;
    wxString tagline;
    int price;
    wxString sizeTitle;
    std::vector<SizeOption> sizes;
    int defaultSize;
    std::vector<wxString> features;
    Personalization personalization;
    int maxTextLength;
    wxString textLabel;
    double artWidth;
    PrintArea nameArea, numberArea, textArea;
    int printSide = 0;
    bool printOnTrim = false;
    PrintArea teamArea, frontNumberArea;
    SizeAdvice sizeAdvice = SizeAdvice::None;

    Shape shape = Shape::Flat;
    double thickness = 0.45;
    wxString reverseArtId;
    wxString sideNames[2];
    RoundShape round;
    wxString tileColorways[2];
};

struct Coupon {
    wxString code;
    wxString description;
    int minSubtotal;
    int minItems;
    int amountOff;
    int percentOff;
};

namespace Catalog {
    const std::vector<Colorway>& Colorways();
    const std::vector<Product>& Products();
    const std::vector<Coupon>& Coupons();
    const Coupon* FindCoupon(const wxString& code);
    const std::vector<wxString>& Categories();

    constexpr int kShippingFee = 80;
    constexpr int kFreeShippingThreshold = 2000;
}

struct CartItem {
    int productIndex;
    int colorwayIndex;
    wxString spec;
    int unitPrice;
    int quantity;

    const Product& GetProduct() const { return Catalog::Products()[productIndex]; }
    const Colorway& GetColorway() const { return Catalog::Colorways()[colorwayIndex]; }
    wxString Title() const { return GetProduct().name + L(wxT("・"), wxT(" · ")) + GetColorway().name; }
    int Subtotal() const { return unitPrice * quantity; }
    bool SameProductAs(const CartItem& o) const {
        return productIndex == o.productIndex && colorwayIndex == o.colorwayIndex && spec == o.spec;
    }
};

struct OrderRecord {
    wxString number;
    wxDateTime placedAt;
    std::vector<CartItem> items;
    int subtotal, discount, shipping, total;
    wxString couponCode;
    wxString recipient, phone, delivery, address, payment;

    int TotalQuantity() const {
        int n = 0;
        for (const CartItem& item : items) n += item.quantity;
        return n;
    }
};

class OrderHistory {
public:
    static OrderHistory& Get();
    void Add(const OrderRecord& order) { m_orders.insert(m_orders.begin(), order); }
    const std::vector<OrderRecord>& Orders() const { return m_orders; }

private:
    OrderHistory() = default;
    std::vector<OrderRecord> m_orders;
};

class Favorites {
public:
    static Favorites& Get();
    bool Has(int productIndex) const { return m_items.count(productIndex) > 0; }
    void Toggle(int productIndex);
    const std::set<int>& Items() const { return m_items; }

private:
    Favorites() = default;
    std::set<int> m_items;
};

class ShoppingCart {
public:
    static ShoppingCart& Get();

    void Add(const CartItem& item);
    void RemoveAt(size_t index);
    void SetQuantity(size_t index, int quantity);
    void Clear();

    const std::vector<CartItem>& Items() const { return m_items; }
    bool IsEmpty() const { return m_items.empty(); }
    int TotalQuantity() const;
    int Subtotal() const;
    int Discount() const;
    int ShippingFee() const;
    int Total() const { return Subtotal() - Discount() + ShippingFee(); }
    int AmountToFreeShipping() const;

    wxString ApplyCoupon(const wxString& code);
    void RemoveCoupon() { m_coupon = nullptr; }
    const Coupon* AppliedCoupon() const { return m_coupon; }
    wxString CouponProblem(const Coupon& coupon) const;

    static constexpr int kMaxQuantityPerLine = 99;

private:
    ShoppingCart() = default;
    std::vector<CartItem> m_items;
    const Coupon* m_coupon = nullptr;
};
