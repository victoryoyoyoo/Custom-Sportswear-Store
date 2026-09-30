#pragma once
#include <wx/wx.h>
#include <wx/datetime.h>
#include <set>
#include <vector>

// ---------------------------------------------------------------------------
// Store data. Every product and colourway is one row in a table here, so the
// UI never hard-codes a product: the category page, the product page and the
// cart are all built from these rows. (They map one-to-one onto database
// tables if the catalogue ever moves out of the code.)
// ---------------------------------------------------------------------------

struct Colorway {
    wxString id;          // file-name key: <product>_<id>.png
    wxString name;        // 午夜藍
    wxString englishName;
    wxColour fabric;      // main colour
    wxColour trim;        // piping, stripes, print outline
};

struct SizeOption {
    wxString label;       // "L", "42", "7 號"
    wxString hint;        // shown under the size chips
};

// Where custom print goes on the artwork, in the artwork's own pixel units.
struct PrintArea {
    double centerX = 0, top = 0, maxWidth = 0, height = 0;
    bool IsSet() const { return height > 0; }
};

// Which "help me pick a size" tool a product offers.
enum class SizeAdvice { None, Apparel, Shoes };

enum class Personalization {
    None,
    NameAndNumber,        // jersey: name above a big number
    Number,               // shorts: number on the leg
    Text,                 // embroidery / print of a short text
};

// How the product page's 360° view turns the product round.
enum class Shape {
    Flat,                 // clothes, shoes, bags: front and back artwork wrapped round the body
    Basketball,           // drawn as a real sphere, seams and all
    SoccerBall,
    Cap,                  // drawn in 3D: crown, peak, strap at the back
    Round,                // bottle or band: the outline stays, the print travels round it
};

// A Round product's cylinder, in artwork units.
struct RoundShape {
    double centerX = 0, radius = 0;
    double sag = 0;               // how far a ring round it dips at the front (seen from a little above)
    double emblemY = 0, emblemSize = 0;              // centre height and size of the logo
    double textY = 0, textHeight = 0, textWidth = 0; // printed text: centre height, height, arc length
    double textTurn = 0;          // where round the text goes, in radians from the logo
};

struct Product {
    wxString id;          // art: <id>_<colorway>.png
    wxString category;    // 服裝 / 鞋款 / 球具 / 配件, for the filter on the product list
    wxString name;
    wxString englishName;
    wxString tagline;     // one line on the category page
    int price;            // NT$
    wxString sizeTitle;
    std::vector<SizeOption> sizes;  // a single entry means "one size"
    int defaultSize;
    std::vector<wxString> features;
    Personalization personalization;
    int maxTextLength;
    wxString textLabel;   // e.g. 刺繡文字
    double artWidth;      // design width of the artwork, for PrintArea scaling
    PrintArea nameArea, numberArea, textArea;
    int printSide = 0;    // Number / Text print: 0 = the side the page opens on, 1 = the reverse
    bool printOnTrim = false;  // text sits on a trim-coloured panel, so it's printed in the fabric colour
    PrintArea teamArea, frontNumberArea;  // jersey: printed on the reverse (the front of the shirt)
    SizeAdvice sizeAdvice = SizeAdvice::None;

    Shape shape = Shape::Flat;
    double thickness = 0.45;  // Flat: how deep the product is, as a fraction of its width
    wxString reverseArtId;    // Flat: <reverseArtId>_<colorway>.png is the other side; empty = mirror image
    wxString sideNames[2];    // Flat with reverse art: labels of the two sides, e.g. 背面 / 正面
    RoundShape round;         // Round only
    wxString tileColorways[2];  // the two colourways posed on the product-list card
};

struct Coupon {
    wxString code;
    wxString description;
    int minSubtotal;      // NT$
    int minItems;
    int amountOff;        // NT$, or
    int percentOff;       // %
};

namespace Catalog {
    const std::vector<Colorway>& Colorways();
    const std::vector<Product>& Products();
    const std::vector<Coupon>& Coupons();
    const Coupon* FindCoupon(const wxString& code);  // case-insensitive, nullptr if unknown
    const std::vector<wxString>& Categories();       // in display order

    constexpr int kShippingFee = 80;             // NT$ per order
    constexpr int kFreeShippingThreshold = 2000; // on the subtotal before discounts
}

// One line in the cart. Adding the same product + colourway + spec again just
// raises the quantity of the existing line.
struct CartItem {
    int productIndex;
    int colorwayIndex;
    wxString spec;        // "L・#23・WANG"
    int unitPrice;
    int quantity;

    const Product& GetProduct() const { return Catalog::Products()[productIndex]; }
    const Colorway& GetColorway() const { return Catalog::Colorways()[colorwayIndex]; }
    wxString Title() const { return GetProduct().name + wxT("・") + GetColorway().name; }
    int Subtotal() const { return unitPrice * quantity; }
    bool SameProductAs(const CartItem& o) const {
        return productIndex == o.productIndex && colorwayIndex == o.colorwayIndex && spec == o.spec;
    }
};

// A finished order, kept for the "我的訂單" window.
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

// Orders placed since the app started, newest first. (In memory for now; this
// is the one place that would write to an orders table instead.)
class OrderHistory {
public:
    static OrderHistory& Get();
    void Add(const OrderRecord& order) { m_orders.insert(m_orders.begin(), order); }
    const std::vector<OrderRecord>& Orders() const { return m_orders; }

private:
    OrderHistory() = default;
    std::vector<OrderRecord> m_orders;
};

// Products the user marked with the heart. (In memory, like the cart.)
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

// The one shopping cart shared by every window.
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
    int Discount() const;             // from the applied coupon, 0 if none / no longer eligible
    int ShippingFee() const;          // 0 when empty or over the free-shipping threshold
    int Total() const { return Subtotal() - Discount() + ShippingFee(); }
    int AmountToFreeShipping() const; // 0 when already free

    // Returns an empty string on success, otherwise why the code can't be used.
    wxString ApplyCoupon(const wxString& code);
    void RemoveCoupon() { m_coupon = nullptr; }
    const Coupon* AppliedCoupon() const { return m_coupon; }
    wxString CouponProblem(const Coupon& coupon) const;  // empty if eligible right now

    static constexpr int kMaxQuantityPerLine = 99;

private:
    ShoppingCart() = default;
    std::vector<CartItem> m_items;
    const Coupon* m_coupon = nullptr;
};
