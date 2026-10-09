#include "Catalog.h"
#include "Lang.h"
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
            sizes.push_back({ labels[i], wxString::Format(L(wxT("%s 號：%s %d cm・%s %d cm（平量）"), wxT("Size %s: %s %d cm · %s %d cm (measured flat)")),
                                                          labels[i], what, a[i], bName, b[i]) });
        return sizes;
    }

    std::vector<SizeOption> ShoeSizes() {
        std::vector<SizeOption> sizes;
        for (int eu = 38; eu <= 46; ++eu)
            sizes.push_back({ wxString::Format(wxT("%d"), eu),
                              wxString::Format(L(wxT("EU %d：腳長約 %.1f cm"), wxT("EU %d: foot length about %.1f cm")), eu, 24.0 + (eu - 38) * 0.5) });
        return sizes;
    }
}

namespace Catalog {

namespace {
    template <typename Row, typename Build>
    const std::vector<Row>& Localized(std::vector<Row>& table, int& builtFor, Build build) {
        const int language = Lang::English() ? 1 : 0;
        if (builtFor != language) {
            std::vector<Row> fresh = build();
            if (table.size() == fresh.size()) std::copy(fresh.begin(), fresh.end(), table.begin());
            else table = std::move(fresh);
            builtFor = language;
        }
        return table;
    }
}

const std::vector<Colorway>& Colorways() {
    static std::vector<Colorway> table;
    static int builtFor = -1;
    return Localized(table, builtFor, [] { return std::vector<Colorway>{
        { wxT("navy"),    L(wxT("午夜藍"), wxT("Midnight Navy")), wxT("Midnight Navy"),    Hex(0x1B2A4A), Hex(0xC9D3E3) },
        { wxT("crimson"), L(wxT("烈焰紅"), wxT("Crimson Flame")), wxT("Crimson Flame"),    Hex(0xB3202A), Hex(0xF2B632) },
        { wxT("teal"),    L(wxT("海港青"), wxT("Harbor Teal")), wxT("Harbor Teal"),      Hex(0x0F7C8C), Hex(0xE8F1F2) },
        { wxT("royal"),   L(wxT("天際藍"), wxT("Royal Sky")), wxT("Royal Sky"),        Hex(0x2F6FD6), Hex(0xFFFFFF) },
        { wxT("jade"),    L(wxT("翡翠綠"), wxT("Jade Green")), wxT("Jade Green"),       Hex(0x146B45), Hex(0xD9B45A) },
        { wxT("onyx"),    L(wxT("曜石黑"), wxT("Onyx Black")), wxT("Onyx Black"),       Hex(0x1A1A1F), Hex(0x8C5BD6) },
        { wxT("sand"),    L(wxT("沙丘金"), wxT("Desert Gold")), wxT("Desert Gold"),      Hex(0xC08A3E), Hex(0x4A2E14) },
        { wxT("violet"),  L(wxT("極光紫"), wxT("Aurora Violet")), wxT("Aurora Violet"),    Hex(0x5B2C8F), Hex(0x3FD2C7) },
        { wxT("steel"),   L(wxT("鋼鐵灰"), wxT("Steel Grey")), wxT("Steel Grey"),       Hex(0x4B5563), Hex(0xF97316) },
        { wxT("sunset"),  L(wxT("日落橘"), wxT("Sunset Orange")), wxT("Sunset Orange"),    Hex(0xE8671C), Hex(0x1F1F1F) },
        { wxT("indigo"),  L(wxT("月光靛"), wxT("Moonlight Indigo")), wxT("Moonlight Indigo"), Hex(0x2E3A87), Hex(0xF3E7C9) },
        { wxT("cocoa"),   L(wxT("可可棕"), wxT("Cocoa Brown")), wxT("Cocoa Brown"),      Hex(0x5A3A22), Hex(0x8FBF6A) },
    }; });
}

const std::vector<Product>& Products() {
    static std::vector<Product> table;
    static int builtFor = -1;
    return Localized(table, builtFor, [] {
        std::vector<Product> list;

        Product jersey{};
        jersey.category = L(wxT("服裝"), wxT("Apparel"));
        jersey.id = wxT("jersey");
        jersey.name = L(wxT("客製球衣"), wxT("Custom Jersey"));
        jersey.englishName = wxT("Custom Jersey");
        jersey.tagline = L(wxT("印上姓名與背號，即時預覽"), wxT("Your name and number, previewed live"));
        jersey.price = 1280;
        jersey.sizeTitle = L(wxT("選擇尺寸"), wxT("Size"));
        jersey.sizes = ApparelSizes(L(wxT("胸寬"), wxT("chest")), { 48, 51, 54, 57, 60 }, { 70, 72, 74, 76, 78 }, L(wxT("衣長"), wxT("length")));
        jersey.defaultSize = 2;
        jersey.features = { L(wxT("吸濕排汗網眼布，透氣不悶熱"), wxT("Moisture-wicking mesh that breathes")), L(wxT("姓名、背號熱轉印，耐洗不脫落"), wxT("Heat-pressed name and number that survive the wash")), L(wxT("領口與袖口雙層滾邊"), wxT("Double binding on the neck and armholes")) };
        jersey.personalization = Personalization::NameAndNumber;
        jersey.maxTextLength = 12;
        jersey.textLabel = L(wxT("背號與姓名"), wxT("Number and name"));
        jersey.artWidth = 520;
        jersey.nameArea = { 260, 112, 250, 40 };
        jersey.numberArea = { 260, 174, 300, 196 };
        jersey.reverseArtId = wxT("jersey_front");
        jersey.sideNames[0] = L(wxT("背面"), wxT("Back"));
        jersey.sideNames[1] = L(wxT("正面"), wxT("Front"));
        jersey.teamArea = { 260, 140, 300, 46 };
        jersey.frontNumberArea = { 260, 198, 200, 122 };
        jersey.sizeAdvice = SizeAdvice::Apparel;
        jersey.tileColorways[0] = wxT("crimson");
        jersey.tileColorways[1] = wxT("navy");
        list.push_back(jersey);

        Product tee{};
        tee.category = L(wxT("服裝"), wxT("Apparel"));
        tee.id = wxT("tee");
        tee.name = L(wxT("訓練短袖 T 恤"), wxT("Training Tee"));
        tee.englishName = wxT("Training Tee");
        tee.tagline = L(wxT("棉感排汗布，胸前可印字"), wxT("Soft wicking fabric, print on the chest"));
        tee.price = 590;
        tee.sizeTitle = L(wxT("選擇尺寸"), wxT("Size"));
        tee.sizes = ApparelSizes(L(wxT("胸寬"), wxT("chest")), { 48, 50, 53, 56, 59 }, { 68, 70, 72, 74, 76 }, L(wxT("衣長"), wxT("length")));
        tee.defaultSize = 2;
        tee.features = { L(wxT("棉感排汗布，觸感柔軟"), wxT("Cotton-feel wicking fabric, soft to the touch")), L(wxT("羅紋圓領，洗後不易變形"), wxT("Ribbed crew neck that keeps its shape")), L(wxT("胸前印字，隊服、活動服都適合"), wxT("Chest print for team kit or event shirts")) };
        tee.personalization = Personalization::Text;
        tee.maxTextLength = 12;
        tee.textLabel = L(wxT("胸前印字（選填）"), wxT("Chest print (optional)"));
        tee.artWidth = 520;
        tee.textArea = { 260, 150, 250, 56 };
        tee.reverseArtId = wxT("tee_back");
        tee.sideNames[0] = L(wxT("正面"), wxT("Front"));
        tee.sideNames[1] = L(wxT("背面"), wxT("Back"));
        tee.sizeAdvice = SizeAdvice::Apparel;
        tee.tileColorways[0] = wxT("royal");
        tee.tileColorways[1] = wxT("sunset");
        list.push_back(tee);

        Product hoodie{};
        hoodie.category = L(wxT("服裝"), wxT("Apparel"));
        hoodie.id = wxT("hoodie");
        hoodie.name = L(wxT("刷毛連帽上衣"), wxT("Fleece Hoodie"));
        hoodie.englishName = wxT("Fleece Hoodie");
        hoodie.tagline = L(wxT("內刷毛保暖，袋鼠口袋"), wxT("Brushed fleece, kangaroo pocket"));
        hoodie.price = 1580;
        hoodie.sizeTitle = L(wxT("選擇尺寸"), wxT("Size"));
        hoodie.sizes = ApparelSizes(L(wxT("胸寬"), wxT("chest")), { 54, 57, 60, 63, 66 }, { 68, 70, 72, 74, 76 }, L(wxT("衣長"), wxT("length")));
        hoodie.defaultSize = 2;
        hoodie.features = { L(wxT("內裡刷毛，賽前熱身、冬天通勤都保暖"), wxT("Brushed inside, warm for warm-ups and winter")), L(wxT("雙層帽子附抽繩"), wxT("Lined hood with drawcords")), L(wxT("袋鼠口袋，胸前可印字"), wxT("Kangaroo pocket, print on the chest")) };
        hoodie.personalization = Personalization::Text;
        hoodie.maxTextLength = 12;
        hoodie.textLabel = L(wxT("胸前印字（選填）"), wxT("Chest print (optional)"));
        hoodie.artWidth = 560;
        hoodie.textArea = { 280, 300, 250, 50 };
        hoodie.reverseArtId = wxT("hoodie_back");
        hoodie.sideNames[0] = L(wxT("正面"), wxT("Front"));
        hoodie.sideNames[1] = L(wxT("背面"), wxT("Back"));
        hoodie.sizeAdvice = SizeAdvice::Apparel;
        hoodie.thickness = 0.50;
        hoodie.tileColorways[0] = wxT("steel");
        hoodie.tileColorways[1] = wxT("crimson");
        list.push_back(hoodie);

        Product shorts{};
        shorts.category = L(wxT("服裝"), wxT("Apparel"));
        shorts.id = wxT("shorts");
        shorts.name = L(wxT("籃球褲"), wxT("Basketball Shorts"));
        shorts.englishName = wxT("Basketball Shorts");
        shorts.tagline = L(wxT("側邊撞色條，可印背號"), wxT("Contrast side stripes, number on the leg"));
        shorts.price = 890;
        shorts.sizeTitle = L(wxT("選擇尺寸"), wxT("Size"));
        shorts.sizes = ApparelSizes(L(wxT("腰圍"), wxT("waist")), { 66, 71, 76, 81, 86 }, { 50, 52, 54, 56, 58 }, L(wxT("褲長"), wxT("length")));
        shorts.defaultSize = 2;
        shorts.features = { L(wxT("鬆緊腰頭附抽繩，穿脫方便"), wxT("Elastic waist with drawcord")), L(wxT("輕量網眼布，快乾透氣"), wxT("Light mesh that dries fast")), L(wxT("褲管背號熱轉印"), wxT("Heat-pressed number on the leg")) };
        shorts.personalization = Personalization::Number;
        shorts.textLabel = L(wxT("褲管背號"), wxT("Leg number"));
        shorts.artWidth = 600;
        shorts.numberArea = { 196, 318, 120, 92 };
        shorts.sizeAdvice = SizeAdvice::Apparel;
        shorts.thickness = 0.50;
        shorts.tileColorways[0] = wxT("jade");
        shorts.tileColorways[1] = wxT("onyx");
        list.push_back(shorts);

        Product socks{};
        socks.category = L(wxT("服裝"), wxT("Apparel"));
        socks.id = wxT("socks");
        socks.name = L(wxT("籃球襪（2 雙入）"), wxT("Crew Socks (2 pairs)"));
        socks.englishName = wxT("Crew Socks, 2 pairs");
        socks.tagline = L(wxT("毛巾底，加厚緩衝"), wxT("Terry sole with extra cushioning"));
        socks.price = 290;
        socks.sizeTitle = L(wxT("選擇尺寸"), wxT("Size"));
        socks.sizes = { { wxT("S"), L(wxT("S：適合腳長 22–24 cm"), wxT("S: feet 22–24 cm")) },
                        { wxT("M"), L(wxT("M：適合腳長 24–26 cm"), wxT("M: feet 24–26 cm")) },
                        { wxT("L"), L(wxT("L：適合腳長 26–28 cm"), wxT("L: feet 26–28 cm")) } };
        socks.defaultSize = 1;
        socks.features = { L(wxT("毛巾底加厚，降低衝擊"), wxT("Thick terry sole softens landings")), L(wxT("足弓加壓，減少滑動"), wxT("Arch support stops slipping")), L(wxT("一組兩雙"), wxT("Two pairs")) };
        socks.personalization = Personalization::None;
        socks.artWidth = 560;
        socks.thickness = 0.35;
        socks.tileColorways[0] = wxT("teal");
        socks.tileColorways[1] = wxT("crimson");
        list.push_back(socks);

        Product sneaker{};
        sneaker.category = L(wxT("鞋款"), wxT("Shoes"));
        sneaker.id = wxT("sneaker");
        sneaker.name = L(wxT("高筒籃球鞋"), wxT("High-Top Sneakers"));
        sneaker.englishName = wxT("High-Top Sneakers");
        sneaker.tagline = L(wxT("高筒包覆，EU 38–46"), wxT("High-top support, EU 38–46"));
        sneaker.price = 3280;
        sneaker.sizeTitle = L(wxT("選擇尺寸（EU）"), wxT("Size (EU)"));
        sneaker.sizes = ShoeSizes();
        sneaker.defaultSize = 4;
        sneaker.features = { L(wxT("高筒設計，穩定包覆腳踝"), wxT("High collar that holds the ankle")), L(wxT("緩震中底，落地更輕鬆"), wxT("Cushioned midsole for softer landings")), L(wxT("耐磨橡膠大底，室內外皆適用"), wxT("Durable rubber outsole, indoor or outdoor")) };
        sneaker.personalization = Personalization::None;
        sneaker.artWidth = 640;
        sneaker.sizeAdvice = SizeAdvice::Shoes;
        sneaker.thickness = 0.38;
        sneaker.tileColorways[0] = wxT("sunset");
        sneaker.tileColorways[1] = wxT("royal");
        list.push_back(sneaker);

        Product ball{};
        ball.category = L(wxT("球具"), wxT("Balls"));
        ball.id = wxT("basketball");
        ball.name = L(wxT("配色籃球"), wxT("Basketball"));
        ball.englishName = wxT("Basketball");
        ball.tagline = L(wxT("雙色球皮，可印名字"), wxT("Two-tone cover, print your name"));
        ball.price = 990;
        ball.sizeTitle = L(wxT("選擇尺寸"), wxT("Size"));
        ball.sizes = { { L(wxT("5 號"), wxT("Size 5")), L(wxT("5 號：國小、室內休閒"), wxT("Size 5: kids, casual indoor play")) },
                       { L(wxT("6 號"), wxT("Size 6")), L(wxT("6 號：女子標準球"), wxT("Size 6: women's official size")) },
                       { L(wxT("7 號"), wxT("Size 7")), L(wxT("7 號：男子標準球"), wxT("Size 7: men's official size")) } };
        ball.defaultSize = 2;
        ball.features = { L(wxT("合成皮革，手感紮實"), wxT("Composite leather with a firm feel")), L(wxT("深溝紋設計，好控球"), wxT("Deep channels for better grip")), L(wxT("可在球皮上印製文字"), wxT("Text printed on the cover")) };
        ball.personalization = Personalization::Text;
        ball.maxTextLength = 10;
        ball.textLabel = L(wxT("印製文字（選填）"), wxT("Printed text (optional)"));
        ball.shape = Shape::Basketball;
        ball.tileColorways[0] = wxT("navy");
        ball.tileColorways[1] = wxT("sunset");
        list.push_back(ball);

        Product soccer{};
        soccer.category = L(wxT("球具"), wxT("Balls"));
        soccer.id = wxT("soccer");
        soccer.name = L(wxT("配色足球"), wxT("Football"));
        soccer.englishName = wxT("Football");
        soccer.tagline = L(wxT("經典 32 片，可印名字"), wxT("Classic 32 panels, print your name"));
        soccer.price = 890;
        soccer.sizeTitle = L(wxT("選擇尺寸"), wxT("Size"));
        soccer.sizes = { { L(wxT("4 號"), wxT("Size 4")), L(wxT("4 號：國小、青少年"), wxT("Size 4: kids and teens")) },
                         { L(wxT("5 號"), wxT("Size 5")), L(wxT("5 號：國中以上標準球"), wxT("Size 5: official size, 12 and up")) } };
        soccer.defaultSize = 1;
        soccer.features = { L(wxT("經典 32 片拼接，飛行穩定"), wxT("Classic 32-panel build that flies true")), L(wxT("機縫球皮，耐踢耐磨"), wxT("Machine-stitched cover built to last")), L(wxT("可在球皮上印製文字"), wxT("Text printed on the cover")) };
        soccer.personalization = Personalization::Text;
        soccer.maxTextLength = 10;
        soccer.textLabel = L(wxT("印製文字（選填）"), wxT("Printed text (optional)"));
        soccer.shape = Shape::SoccerBall;
        soccer.tileColorways[0] = wxT("navy");
        soccer.tileColorways[1] = wxT("crimson");
        list.push_back(soccer);

        Product cap{};
        cap.category = L(wxT("配件"), wxT("Accessories"));
        cap.id = wxT("cap");
        cap.name = L(wxT("棒球帽"), wxT("Baseball Cap"));
        cap.englishName = wxT("Baseball Cap");
        cap.tagline = L(wxT("後扣可調，可加刺繡"), wxT("Snapback, add embroidery"));
        cap.price = 690;
        cap.sizeTitle = L(wxT("尺寸"), wxT("Size"));
        cap.sizes = { { L(wxT("可調式"), wxT("Adjustable")), L(wxT("後扣可調・頭圍 54–60 cm，一個尺寸適合大多數人"), wxT("Snapback · fits heads 54–60 cm, one size fits most")) } };
        cap.defaultSize = 0;
        cap.features = { L(wxT("六片式結構，帽頂立體不易變形"), wxT("Structured six-panel crown")), L(wxT("正面立體刺繡標誌"), wxT("Raised embroidered front logo")), L(wxT("內襯吸濕排汗"), wxT("Moisture-wicking sweatband")) };
        cap.personalization = Personalization::Text;
        cap.maxTextLength = 10;
        cap.textLabel = L(wxT("後側刺繡（選填）"), wxT("Embroidery on the back (optional)"));
        cap.shape = Shape::Cap;
        cap.tileColorways[0] = wxT("jade");
        cap.tileColorways[1] = wxT("royal");
        list.push_back(cap);

        Product headband{};
        headband.category = L(wxT("配件"), wxT("Accessories"));
        headband.id = wxT("headband");
        headband.name = L(wxT("運動頭帶"), wxT("Headband"));
        headband.englishName = wxT("Headband");
        headband.tagline = L(wxT("毛巾布吸汗，可加刺繡"), wxT("Absorbent terry, add embroidery"));
        headband.price = 220;
        headband.sizeTitle = L(wxT("尺寸"), wxT("Size"));
        headband.sizes = { { L(wxT("單一尺寸"), wxT("One size")), L(wxT("彈性毛巾布，頭圍 52–62 cm 都適用"), wxT("Stretch terry for heads 52–62 cm")) } };
        headband.defaultSize = 0;
        headband.features = { L(wxT("毛巾布吸汗，汗水不滴進眼睛"), wxT("Terry keeps sweat out of your eyes")), L(wxT("高彈性，久戴不壓頭"), wxT("Stretchy, comfortable all game")), L(wxT("後側可刺繡文字"), wxT("Embroidery on the back")) };
        headband.personalization = Personalization::Text;
        headband.maxTextLength = 8;
        headband.textLabel = L(wxT("刺繡文字（選填）"), wxT("Embroidered text (optional)"));
        headband.artWidth = 520;
        headband.shape = Shape::Round;
        headband.round = { 260, 220, 61, 155, 32, 155, 26, 170, 3.14159265358979 };
        headband.tileColorways[0] = wxT("teal");
        headband.tileColorways[1] = wxT("onyx");
        list.push_back(headband);

        Product band{};
        band.category = L(wxT("配件"), wxT("Accessories"));
        band.id = wxT("wristband");
        band.name = L(wxT("運動護腕（2 入）"), wxT("Wristbands (pair)"));
        band.englishName = wxT("Wristbands, pair");
        band.tagline = L(wxT("毛巾布吸汗，可加刺繡"), wxT("Absorbent terry, add embroidery"));
        band.price = 250;
        band.sizeTitle = L(wxT("尺寸"), wxT("Size"));
        band.sizes = { { L(wxT("單一尺寸"), wxT("One size")), L(wxT("彈性毛巾布，適合大多數手腕"), wxT("Stretch terry that fits most wrists")) } };
        band.defaultSize = 0;
        band.features = { L(wxT("毛巾布吸汗快乾"), wxT("Absorbent, quick-drying terry")), L(wxT("高彈性不勒手"), wxT("Stretchy without pinching")), L(wxT("一組兩入"), wxT("Pair of two")) };
        band.personalization = Personalization::Text;
        band.maxTextLength = 8;
        band.textLabel = L(wxT("刺繡文字（選填）"), wxT("Embroidered text (optional)"));
        band.artWidth = 520;
        band.shape = Shape::Round;
        band.round = { 260, 170, 48, 195, 44, 195, 32, 200, 3.14159265358979 };
        band.tileColorways[0] = wxT("violet");
        band.tileColorways[1] = wxT("sunset");
        list.push_back(band);

        Product pack{};
        pack.category = L(wxT("配件"), wxT("Accessories"));
        pack.id = wxT("backpack");
        pack.name = L(wxT("球袋後背包"), wxT("Team Backpack"));
        pack.englishName = wxT("Team Backpack");
        pack.tagline = L(wxT("25L，前袋可印名字"), wxT("25 L, name on the front pocket"));
        pack.price = 1680;
        pack.sizeTitle = L(wxT("容量"), wxT("Capacity"));
        pack.sizes = { { wxT("25 L"), L(wxT("25 L：可放一顆 7 號球、球鞋與換洗衣物"), wxT("25 L: fits a size 7 ball, shoes and a change of clothes")) } };
        pack.defaultSize = 0;
        pack.features = { L(wxT("獨立球袋與鞋袋"), wxT("Separate ball and shoe compartments")), L(wxT("耐磨防潑水布料"), wxT("Tough water-repellent fabric")), L(wxT("前袋可印製姓名"), wxT("Name printed on the front pocket")) };
        pack.personalization = Personalization::Text;
        pack.maxTextLength = 12;
        pack.textLabel = L(wxT("前袋印字（選填）"), wxT("Front pocket print (optional)"));
        pack.artWidth = 520;
        pack.textArea = { 260, 398, 176, 44 };
        pack.printOnTrim = true;
        pack.reverseArtId = wxT("backpack_back");
        pack.sideNames[0] = L(wxT("正面"), wxT("Front"));
        pack.sideNames[1] = L(wxT("背面"), wxT("Back"));
        pack.thickness = 0.55;
        pack.tileColorways[0] = wxT("steel");
        pack.tileColorways[1] = wxT("navy");
        list.push_back(pack);

        Product bottle{};
        bottle.category = L(wxT("配件"), wxT("Accessories"));
        bottle.id = wxT("bottle");
        bottle.name = L(wxT("運動水壺"), wxT("Squeeze Bottle"));
        bottle.englishName = wxT("Squeeze Bottle, 750 ml");
        bottle.tagline = L(wxT("擠壓式瓶身，可印名字"), wxT("Squeeze bottle, print your name"));
        bottle.price = 450;
        bottle.sizeTitle = L(wxT("容量"), wxT("Capacity"));
        bottle.sizes = { { wxT("750 ml"), L(wxT("750 ml：一場球賽的份量"), wxT("750 ml: enough for a game")) } };
        bottle.defaultSize = 0;
        bottle.features = { L(wxT("食品級 PP，不含雙酚 A"), wxT("Food-grade, BPA-free PP")), L(wxT("防漏彈蓋，單手就能喝"), wxT("Leak-proof flip cap, one-handed drinking")), L(wxT("瓶身可印姓名，不怕拿錯"), wxT("Your name on it, so nobody takes it")) };
        bottle.personalization = Personalization::Text;
        bottle.maxTextLength = 12;
        bottle.textLabel = L(wxT("瓶身印字（選填）"), wxT("Bottle print (optional)"));
        bottle.artWidth = 360;
        bottle.shape = Shape::Round;
        bottle.round = { 180, 104, 16, 396, 64, 470, 36, 190, 0 };
        bottle.tileColorways[0] = wxT("crimson");
        bottle.tileColorways[1] = wxT("indigo");
        list.push_back(bottle);

        Product towel{};
        towel.category = L(wxT("配件"), wxT("Accessories"));
        towel.id = wxT("towel");
        towel.name = L(wxT("運動毛巾"), wxT("Sports Towel"));
        towel.englishName = wxT("Sports Towel");
        towel.tagline = L(wxT("純棉毛圈，織帶可刺繡"), wxT("Cotton terry, embroider the band"));
        towel.price = 390;
        towel.sizeTitle = L(wxT("尺寸"), wxT("Size"));
        towel.sizes = { { wxT("30 × 100 cm"), L(wxT("30 × 100 cm：掛在脖子上剛好"), wxT("30 × 100 cm: just right round the neck")) } };
        towel.defaultSize = 0;
        towel.features = { L(wxT("純棉毛圈，吸水快"), wxT("Absorbent cotton terry")), L(wxT("附掛環，掛在包包或場邊都方便"), wxT("Hanging loop for your bag or the bench")), L(wxT("織帶可刺繡姓名"), wxT("Name embroidered on the band")) };
        towel.personalization = Personalization::Text;
        towel.maxTextLength = 12;
        towel.textLabel = L(wxT("織帶刺繡（選填）"), wxT("Band embroidery (optional)"));
        towel.artWidth = 480;
        towel.textArea = { 240, 484, 250, 42 };
        towel.printOnTrim = true;
        towel.thickness = 0.03;
        towel.tileColorways[0] = wxT("jade");
        towel.tileColorways[1] = wxT("sand");
        list.push_back(towel);

        return list;
    });
}

const std::vector<Coupon>& Coupons() {
    static std::vector<Coupon> table;
    static int builtFor = -1;
    return Localized(table, builtFor, [] { return std::vector<Coupon>{
        { wxT("WELCOME100"), L(wxT("新朋友折 NT$100（滿 NT$1,000）"), wxT("NT$100 off your first order (over NT$1,000)")), 1000, 0, 100, 0 },
        { wxT("TEAM10"),     L(wxT("團體訂購 9 折（5 件以上）"), wxT("Team order: 10% off (5 items or more)")),       0,    5, 0,   10 },
    }; });
}

const std::vector<wxString>& Categories() {
    static std::vector<wxString> table;
    static int builtFor = -1;
    return Localized(table, builtFor, [] {
        return std::vector<wxString>{ L(wxT("服裝"), wxT("Apparel")), L(wxT("鞋款"), wxT("Shoes")),
                                      L(wxT("球具"), wxT("Balls")), L(wxT("配件"), wxT("Accessories")) };
    });
}

const Coupon* FindCoupon(const wxString& code) {
    wxString wanted = code;
    wanted.Trim().Trim(false);
    for (const Coupon& c : Coupons())
        if (c.code.IsSameAs(wanted, false)) return &c;
    return nullptr;
}

}

Favorites& Favorites::Get() {
    static Favorites instance;
    return instance;
}

void Favorites::Toggle(int productIndex) {
    if (!m_items.erase(productIndex)) m_items.insert(productIndex);
}

OrderHistory& OrderHistory::Get() {
    static OrderHistory instance;
    return instance;
}

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
        return L(wxT("商品金額需滿 "), wxT("Needs a subtotal of ")) + Money(c.minSubtotal);
    if (TotalQuantity() < c.minItems)
        return wxString::Format(L(wxT("需購買 %d 件以上"), wxT("Needs %d or more items")), c.minItems);
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
    if (!coupon) return L(wxT("查無此優惠碼"), wxT("Unknown code"));
    const wxString problem = CouponProblem(*coupon);
    if (!problem.IsEmpty()) return problem;
    m_coupon = coupon;
    return wxString();
}
