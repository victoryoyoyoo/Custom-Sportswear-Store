# 程式架構

整支程式約 4,300 行 C++，分成三層：**資料**（商品、配色、購物車）、**畫面**（五個視窗）、
**外觀**（配色、字型、自繪元件與動畫）。畫面只讀資料、只用外觀層的元件，彼此不互相牽扯。

```
CustomSportswearStore/
├─ App.cpp              程式進入點，開啟歡迎頁
├─ Catalog.h/.cpp       資料：商品表、配色表、優惠碼表、ShoppingCart
├─ Personalizer.h/.cpp  客製化方式（Strategy 模式）
├─ WelcomeFrame         歡迎頁
├─ LauncherFrame        全部商品（8 張商品卡）
├─ ProductFrame         單一商品頁（任何商品都用這一個類別）
├─ ProductDialogs       尺寸建議、團體訂購兩個對話框
├─ CartDialog           購物車、填寫資料、訂購完成、我的訂單四個對話框
├─ SwatchPicker         色票選擇器（自繪）
├─ Widgets              自繪元件與動畫：FlatButton、Card、ChipPicker、ProgressBar、
│                       StepIndicator、Toast、Tween、FadeIn
└─ Theme                配色、字型、共用版面工具、ImagePanel
```

## 1. 資料層：商品是「資料」，不是程式碼

`Catalog.cpp` 有三張表：

| 表 | 一筆資料有什麼 |
| --- | --- |
| `Colorways()` | 配色 id、中英文名稱、主色、配色 |
| `Products()` | 商品 id、分類、名稱、價格、尺寸清單與說明、商品特色、客製化方式、印字位置 |
| `Coupons()` | 優惠碼、說明、門檻（金額或件數）、折扣（金額或百分比） |

畫面從這些表把東西「長」出來：全部商品頁跑 `Products()` 產生 8 張卡片，分類標籤依
`category` 篩選；商品頁讀
`sizes` 產生尺寸標籤、讀 `features` 產生特色清單。**新增一類商品，只要在 `Products()`
加一筆資料，再用 `tools/gen_assets.py` 產生它的圖，畫面程式一行都不用改。**

`ShoppingCart` 是整支程式共用的一台購物車，用函式內的 `static` 物件保證只有一個
（`ShoppingCart::Get()`）。它負責：

- 加入商品時，同商品＋同配色＋同規格就合併成一列，只加數量
- 小計、優惠折扣、運費（滿 NT$2,000 免運，看折扣前的金額）、應付總額
- 優惠碼能不能用的判斷（`CouponProblem()`），購物車內容改變時會重新檢查

`Favorites` 記錄愛心收藏的商品。`OrderHistory` 保存這次開啟程式後完成的訂單（`OrderRecord`：編號、時間、商品、金額、收件資訊），
「我的訂單」視窗就是讀它。

### 之後要接資料庫的話

目前資料都在記憶體裡，程式關掉就不見。要改成資料庫時，接點只有這兩個地方：

- `Catalog::Products()`、`Colorways()`、`Coupons()`：改成從資料表讀出來
- `OrderHistory::Add()`：結帳完成時（`CartDialog::OnCheckout()`）把訂單和每一列商品寫進 orders／order_items

畫面層只透過這幾個函式拿資料，所以換成資料庫之後，視窗程式不需要跟著改。

## 2. 畫面流程

```
App::OnInit
  └─ WelcomeFrame ──(進入商店)──► LauncherFrame
                                    └─(點商品卡)──► ProductFrame
                                                      ├─(購物車按鈕 / 通知)──► CartDialog
                                                      │                          └─► CheckoutDialog ──► OrderCompleteDialog
                                                      └─(返回 / ×)──► LauncherFrame
```

- 換頁用 `Theme::ShowLike()`：新視窗用跟舊視窗一樣的狀態（一般／最大化／全螢幕）
  淡入，淡入完成後才把舊視窗藏起來或關掉，畫面不會閃一下桌面。
- 商品頁不管按「所有商品」還是右上角 ×，都會把全部商品頁帶回來
  （`ProductFrame::OnClose`），不會留下一個看不見、卻讓程式一直在背景執行的視窗。
- 購物車、填寫資料、完成、我的訂單都是 **modal 對話框**：開著的時候後面的視窗不能操作，
  關掉後商品頁再更新右上角的購物車按鈕。
- 對話框是建立在 stack 上的區域變數（`CartDialog dialog(this);`），在 `ShowModal()`
  回來之前，擁有它的頁面絕對不能被刪掉，否則視窗框架會去 `delete` 一個 stack 物件。
  滑鼠點不到被停用的頁面，但從工作列按「關閉視窗」還是會送出關閉要求，所以所有對話框都經過
  `Theme::ShowModalDialog()` 計數，頁面的關閉事件用 `Theme::CanClosePage()` 在對話框開著時拒絕關閉。
- 加入購物車的通知（`Toast`）比它所屬的頁面晚被刪除，所以它監聽頁面的 `wxEVT_DESTROY`，
  頁面一消失就放開手上的指標，不會在之後碰到已經不存在的物件。

## 3. 客製化：Strategy 模式

商品頁不知道「背號」「刺繡」是什麼。它只跟 `Personalizer` 說三件事：

| 方法 | 做什麼 |
| --- | --- |
| `BuildControls()` | 在表單裡放自己需要的輸入欄位 |
| `Describe()` | 回傳購物車上的規格文字，例如 `#23・WANG` |
| `Draw()` | 把客製內容畫到預覽圖上 |

四種實作：

| 類別 | 用在 | 預覽圖上的樣子 |
| --- | --- | --- |
| `NameAndNumberPersonalizer` | 球衣 | 大背號＋姓名，燙印描邊 |
| `NumberPersonalizer` | 籃球褲 | 褲管上的背號 |
| `TextPersonalizer` | 帽子、籃球、護腕、後背包 | 有印字位置就直接印上；沒有就在角落畫一個刺繡標籤 |
| `Personalizer`（基底） | 球鞋、襪子 | 不客製，什麼都不畫 |

`Personalizer::For(product)` 依商品資料裡的 `personalization` 欄位挑一個。要加新的客製方式，
新增一個子類別就好，商品頁完全不用動。

印字位置（`PrintArea`）用「原圖的像素座標」記在商品資料裡。預覽圖不管畫多大，
`Draw()` 都用「實際寬度 ÷ 原圖寬度」換算，所以文字永遠落在同一個位置。

## 4. 預覽圖與高 DPI

`Theme::ImagePanel` 每次尺寸改變，就用當下的實際像素重新畫一次圖。
視窗放大、全螢幕，或在 125%、150% 縮放的螢幕上，圖都是 1:1 的像素，不會被系統放大而糊掉。

圖的縮放交給 `wxGraphicsContext`（GDI+）處理，而不是 `wxImage::Scale`：後者在柔和的陰影
漸層上會留下一圈看得見的等高線。

## 5. 自繪元件與動畫

原生的 Windows 按鈕沒辦法改圓角、做動畫，所以按鈕、卡片、色票等都是自己畫的
（`wxEVT_PAINT` ＋ `wxGraphicsContext`）：

| 元件 | 效果 |
| --- | --- |
| `FlatButton` | 滑鼠移上去時顏色平滑過渡；按下時縮一點；主要按鈕是膠囊形，箭頭會往前推 |
| `Card` | 柔和的環境陰影＋淡色外框＋白色內層；可點的卡片在滑鼠移上去時浮起來 |
| `SwatchPicker` / `ChipPicker` | 色票、尺寸標籤，支援滑鼠和方向鍵 |
| `ProgressBar` | 免運進度條，數值改變時滑動到新位置 |
| `StepIndicator` | 結帳三步驟 |
| `Toast` | 加入購物車的通知，從購物車按鈕下方滑出、幾秒後淡出 |

所有動畫都用同一個 `Widgets::Tween`：一個約 60 fps 的計時器，把 0 → 1 的進度套上
減速曲線（前段快、後段慢慢停下），再交給各元件決定要變什麼（顏色、位置、透明度）。

## 6. 素材與打包

- `tools/gen_assets.py`：用 Pillow 畫出全部 107 張圖（8 類商品 × 12 配色、商品卡、主視覺、圖示）。
  以 3 倍大小繪製再縮小，邊緣才會平滑；商品圖事先疊在白底上，程式讀進來就不用處理透明度。
- `tools/package.ps1`：編譯 Release 版，把 exe、`assets/`、C++ 執行階段 DLL 和說明包成 zip。
  對方解壓縮就能執行，不需要安裝 Visual Studio。
