#include "Catalog.h"
#include <algorithm>

namespace {
    wxString Money(int amount) {
        wxString digits = wxString::Format(wxT("%d"), amount), out;
        for (size_t i = 0; i < digits.length(); ++i) {
            if (i > 0 && (digits.length() - i) % 3 == 0) out += wxT(',');
            out += digits[i];
        }
        return wxT("NT$") + out;
    }

    wxColour Hex(unsigned long v) { return wxColour((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF); }

    std::vector<SizeOption> ApparelSizes(const wxString& what, const int (&a)[5], const int (&b)[5], const wxString& bName) {
        const wchar_t* labels[] = { L"S", L"M", L"L", L"XL", L"2XL" };
        std::vector<SizeOption> sizes;
        for (int i = 0; i < 5; ++i)
            sizes.push_back({ labels[i], wxString::Format(wxT("%s 號：%s %d cm・%s %d cm（平量）"),
                                                          labels[i], what, a[i], bName, b[i]) });
        return sizes;
    }

    std::vector<SizeOption> ShoeSizes() {
        std::vector<SizeOption> sizes;
        for (int eu = 38; eu <= 46; ++eu)
            sizes.push_back({ wxString::Format(wxT("%d"), eu),
                              wxString::Format(wxT("EU %d：腳長約 %.1f cm"), eu, 24.0 + (eu - 38) * 0.5) });
        return sizes;
    }
}

namespace Catalog {

const std::vector<Colorway>& Colorways() {
    static const std::vector<Colorway> kColorways = {
        { wxT("navy"),    wxT("午夜藍"), wxT("Midnight Navy"),    Hex(0x1B2A4A), Hex(0xC9D3E3) },
        { wxT("crimson"), wxT("烈焰紅"), wxT("Crimson Flame"),    Hex(0xB3202A), Hex(0xF2B632) },
        { wxT("teal"),    wxT("海港青"), wxT("Harbor Teal"),      Hex(0x0F7C8C), Hex(0xE8F1F2) },
        { wxT("royal"),   wxT("天際藍"), wxT("Royal Sky"),        Hex(0x2F6FD6), Hex(0xFFFFFF) },
        { wxT("jade"),    wxT("翡翠綠"), wxT("Jade Green"),       Hex(0x146B45), Hex(0xD9B45A) },
        { wxT("onyx"),    wxT("曜石黑"), wxT("Onyx Black"),       Hex(0x1A1A1F), Hex(0x8C5BD6) },
        { wxT("sand"),    wxT("沙丘金"), wxT("Desert Gold"),      Hex(0xC08A3E), Hex(0x4A2E14) },
        { wxT("violet"),  wxT("極光紫"), wxT("Aurora Violet"),    Hex(0x5B2C8F), Hex(0x3FD2C7) },
        { wxT("steel"),   wxT("鋼鐵灰"), wxT("Steel Grey"),       Hex(0x4B5563), Hex(0xF97316) },
        { wxT("sunset"),  wxT("日落橘"), wxT("Sunset Orange"),    Hex(0xE8671C), Hex(0x1F1F1F) },
        { wxT("indigo"),  wxT("月光靛"), wxT("Moonlight Indigo"), Hex(0x2E3A87), Hex(0xF3E7C9) },
        { wxT("cocoa"),   wxT("可可棕"), wxT("Cocoa Brown"),      Hex(0x5A3A22), Hex(0x8FBF6A) },
    };
    return kColorways;
}

const std::vector<Product>& Products() {
    static const std::vector<Product> kProducts = [] {
        std::vector<Product> list;

        Product jersey{};
        jersey.category = wxT("服裝");
        jersey.id = wxT("jersey");
        jersey.name = wxT("客製球衣");
        jersey.englishName = wxT("Custom Jersey");
        jersey.tagline = wxT("印上姓名與背號，即時預覽");
        jersey.price = 1280;
        jersey.sizeTitle = wxT("選擇尺寸");
        jersey.sizes = ApparelSizes(wxT("胸寬"), { 48, 51, 54, 57, 60 }, { 70, 72, 74, 76, 78 }, wxT("衣長"));
        jersey.defaultSize = 2;
        jersey.features = { wxT("吸濕排汗網眼布，透氣不悶熱"), wxT("姓名、背號熱轉印，耐洗不脫落"), wxT("領口與袖口雙層滾邊") };
        jersey.personalization = Personalization::NameAndNumber;
        jersey.maxTextLength = 12;
        jersey.textLabel = wxT("背號與姓名");
        jersey.artWidth = 520;
        jersey.nameArea = { 260, 112, 250, 40 };
        jersey.numberArea = { 260, 174, 300, 196 };
        jersey.frontArtId = wxT("jersey_front");
        jersey.teamArea = { 260, 140, 300, 46 };
        jersey.frontNumberArea = { 260, 198, 200, 122 };
        jersey.sizeAdvice = SizeAdvice::Apparel;
        list.push_back(jersey);

        Product shorts{};
        shorts.category = wxT("服裝");
        shorts.id = wxT("shorts");
        shorts.name = wxT("籃球褲");
        shorts.englishName = wxT("Basketball Shorts");
        shorts.tagline = wxT("側邊撞色條，可印背號");
        shorts.price = 890;
        shorts.sizeTitle = wxT("選擇尺寸");
        shorts.sizes = ApparelSizes(wxT("腰圍"), { 66, 71, 76, 81, 86 }, { 50, 52, 54, 56, 58 }, wxT("褲長"));
        shorts.defaultSize = 2;
        shorts.features = { wxT("鬆緊腰頭附抽繩，穿脫方便"), wxT("輕量網眼布，快乾透氣"), wxT("褲管背號熱轉印") };
        shorts.personalization = Personalization::Number;
        shorts.textLabel = wxT("褲管背號");
        shorts.artWidth = 600;
        shorts.numberArea = { 196, 318, 120, 92 };
        shorts.sizeAdvice = SizeAdvice::Apparel;
        list.push_back(shorts);

        Product sneaker{};
        sneaker.category = wxT("鞋款");
        sneaker.id = wxT("sneaker");
        sneaker.name = wxT("高筒籃球鞋");
        sneaker.englishName = wxT("High-Top Sneakers");
        sneaker.tagline = wxT("高筒包覆，EU 38–46");
        sneaker.price = 3280;
        sneaker.sizeTitle = wxT("選擇尺寸（EU）");
        sneaker.sizes = ShoeSizes();
        sneaker.defaultSize = 4;
        sneaker.features = { wxT("高筒設計，穩定包覆腳踝"), wxT("緩震中底，落地更輕鬆"), wxT("耐磨橡膠大底，室內外皆適用") };
        sneaker.personalization = Personalization::None;
        sneaker.artWidth = 640;
        sneaker.sizeAdvice = SizeAdvice::Shoes;
        list.push_back(sneaker);

        Product cap{};
        cap.category = wxT("配件");
        cap.id = wxT("cap");
        cap.name = wxT("棒球帽");
        cap.englishName = wxT("Baseball Cap");
        cap.tagline = wxT("後扣可調，可加刺繡");
        cap.price = 690;
        cap.sizeTitle = wxT("尺寸");
        cap.sizes = { { wxT("可調式"), wxT("後扣可調・頭圍 54–60 cm，一個尺寸適合大多數人") } };
        cap.defaultSize = 0;
        cap.features = { wxT("六片式結構，帽頂立體不易變形"), wxT("正面立體刺繡標誌"), wxT("內襯吸濕排汗") };
        cap.personalization = Personalization::Text;
        cap.maxTextLength = 10;
        cap.textLabel = wxT("後側刺繡（選填）");
        cap.artWidth = 520;
        list.push_back(cap);

        Product ball{};
        ball.category = wxT("球具");
        ball.id = wxT("basketball");
        ball.name = wxT("配色籃球");
        ball.englishName = wxT("Basketball");
        ball.tagline = wxT("雙色球皮，可印名字");
        ball.price = 990;
        ball.sizeTitle = wxT("選擇尺寸");
        ball.sizes = { { wxT("5 號"), wxT("5 號：國小、室內休閒") },
                       { wxT("6 號"), wxT("6 號：女子標準球") },
                       { wxT("7 號"), wxT("7 號：男子標準球") } };
        ball.defaultSize = 2;
        ball.features = { wxT("合成皮革，手感紮實"), wxT("深溝紋設計，好控球"), wxT("可在球皮上印製文字") };
        ball.personalization = Personalization::Text;
        ball.maxTextLength = 10;
        ball.textLabel = wxT("印製文字（選填）");
        ball.artWidth = 560;
        ball.textArea = { 200, 318, 110, 30 };
        list.push_back(ball);

        Product socks{};
        socks.category = wxT("服裝");
        socks.id = wxT("socks");
        socks.name = wxT("籃球襪（2 雙入）");
        socks.englishName = wxT("Crew Socks, 2 pairs");
        socks.tagline = wxT("毛巾底，加厚緩衝");
        socks.price = 290;
        socks.sizeTitle = wxT("選擇尺寸");
        socks.sizes = { { wxT("S"), wxT("S：適合腳長 22–24 cm") },
                        { wxT("M"), wxT("M：適合腳長 24–26 cm") },
                        { wxT("L"), wxT("L：適合腳長 26–28 cm") } };
        socks.defaultSize = 1;
        socks.features = { wxT("毛巾底加厚，降低衝擊"), wxT("足弓加壓，減少滑動"), wxT("一組兩雙") };
        socks.personalization = Personalization::None;
        socks.artWidth = 560;
        list.push_back(socks);

        Product band{};
        band.category = wxT("配件");
        band.id = wxT("wristband");
        band.name = wxT("運動護腕（2 入）");
        band.englishName = wxT("Wristbands, pair");
        band.tagline = wxT("毛巾布吸汗，可加刺繡");
        band.price = 250;
        band.sizeTitle = wxT("尺寸");
        band.sizes = { { wxT("單一尺寸"), wxT("彈性毛巾布，適合大多數手腕") } };
        band.defaultSize = 0;
        band.features = { wxT("毛巾布吸汗快乾"), wxT("高彈性不勒手"), wxT("一組兩入") };
        band.personalization = Personalization::Text;
        band.maxTextLength = 8;
        band.textLabel = wxT("刺繡文字（選填）");
        band.artWidth = 520;
        list.push_back(band);

        Product pack{};
        pack.category = wxT("配件");
        pack.id = wxT("backpack");
        pack.name = wxT("球袋後背包");
        pack.englishName = wxT("Team Backpack");
        pack.tagline = wxT("25L，前袋可印名字");
        pack.price = 1680;
        pack.sizeTitle = wxT("容量");
        pack.sizes = { { wxT("25 L"), wxT("25 L：可放一顆 7 號球、球鞋與換洗衣物") } };
        pack.defaultSize = 0;
        pack.features = { wxT("獨立球袋與鞋袋"), wxT("耐磨防潑水布料"), wxT("前袋可印製姓名") };
        pack.personalization = Personalization::Text;
        pack.maxTextLength = 12;
        pack.textLabel = wxT("前袋印字（選填）");
        pack.artWidth = 520;
        pack.textArea = { 260, 398, 176, 44 };
        list.push_back(pack);

        return list;
    }();
    return kProducts;
}

const std::vector<Coupon>& Coupons() {
    static const std::vector<Coupon> kCoupons = {
        { wxT("WELCOME100"), wxT("新朋友折 NT$100（滿 NT$1,000）"), 1000, 0, 100, 0 },
        { wxT("TEAM10"),     wxT("團體訂購 9 折（5 件以上）"),       0,    5, 0,   10 },
    };
    return kCoupons;
}

const std::vector<wxString>& Categories() {
    static const std::vector<wxString> kCategories = { wxT("服裝"), wxT("鞋款"), wxT("球具"), wxT("配件") };
    return kCategories;
}

const Coupon* FindCoupon(const wxString& code) {
    wxString wanted = code;
    wanted.Trim().Trim(false);
    for (const Coupon& c : Coupons())
        if (c.code.IsSameAs(wanted, false)) return &c;
    return nullptr;
}

}  // namespace Catalog

// ---------------------------------------------------------------------------
// Favorites
// ---------------------------------------------------------------------------
Favorites& Favorites::Get() {
    static Favorites instance;
    return instance;
}

void Favorites::Toggle(int productIndex) {
    if (!m_items.erase(productIndex)) m_items.insert(productIndex);
}

// ---------------------------------------------------------------------------
// OrderHistory
// ---------------------------------------------------------------------------
OrderHistory& OrderHistory::Get() {
    static OrderHistory instance;
    return instance;
}

// ---------------------------------------------------------------------------
// ShoppingCart
// ---------------------------------------------------------------------------
ShoppingCart& ShoppingCart::Get() {
    static ShoppingCart instance;
    return instance;
}

void ShoppingCart::Add(const CartItem& item) {
    for (CartItem& existing : m_items) {
        if (existing.SameProductAs(item)) {
            existing.quantity = std::min(existing.quantity + item.quantity, kMaxQuantityPerLine);
            return;
        }
    }
    m_items.push_back(item);
}

void ShoppingCart::RemoveAt(size_t index) {
    if (index < m_items.size()) m_items.erase(m_items.begin() + index);
}

void ShoppingCart::SetQuantity(size_t index, int quantity) {
    if (index < m_items.size()) m_items[index].quantity = std::clamp(quantity, 1, kMaxQuantityPerLine);
}

void ShoppingCart::Clear() {
    m_items.clear();
    m_coupon = nullptr;
}

int ShoppingCart::TotalQuantity() const {
    int total = 0;
    for (const CartItem& item : m_items) total += item.quantity;
    return total;
}

int ShoppingCart::Subtotal() const {
    int total = 0;
    for (const CartItem& item : m_items) total += item.Subtotal();
    return total;
}

wxString ShoppingCart::CouponProblem(const Coupon& c) const {
    if (Subtotal() < c.minSubtotal)
        return wxT("商品金額需滿 ") + Money(c.minSubtotal);
    if (TotalQuantity() < c.minItems)
        return wxString::Format(wxT("需購買 %d 件以上"), c.minItems);
    return wxString();
}

int ShoppingCart::Discount() const {
    if (!m_coupon || !CouponProblem(*m_coupon).IsEmpty()) return 0;
    if (m_coupon->percentOff > 0) return Subtotal() * m_coupon->percentOff / 100;
    return std::min(m_coupon->amountOff, Subtotal());
}

int ShoppingCart::ShippingFee() const {
    if (m_items.empty() || Subtotal() >= Catalog::kFreeShippingThreshold) return 0;
    return Catalog::kShippingFee;
}

int ShoppingCart::AmountToFreeShipping() const {
    return std::max(0, Catalog::kFreeShippingThreshold - Subtotal());
}

wxString ShoppingCart::ApplyCoupon(const wxString& code) {
    const Coupon* coupon = Catalog::FindCoupon(code);
    if (!coupon) return wxT("查無此優惠碼");
    const wxString problem = CouponProblem(*coupon);
    if (!problem.IsEmpty()) return problem;
    m_coupon = coupon;
    return wxString();
}
