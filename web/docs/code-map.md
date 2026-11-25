# 程式導覽

這份由 `python/code_map.py` 產生，程式改了重跑一次就會更新行號。

## 架構

```
瀏覽器（index.html、js/app.js）
   │  js/store.js 判斷資料來源
   ├─ 直接開 index.html → 本機示範資料（data/data.js）
   └─ 從 C# 伺服器開 → fetch /api/... → server/Program.cs → server/ShopRepository.cs → SQL Server（運動用品商店）
```

## 新增、修改、刪除、查詢在哪裡

| 功能 | 畫面操作 | 前端 | 資料層 | API 路由 | SQL |
|---|---|---|---|---|---|
| 新增商品 | 後台 → 商品與顧客 → 新增商品 | `js/app.js` 第 713 行 | `js/store.js` 第 81 行 | `server/Program.cs` 第 40 行 | `server/ShopRepository.cs` 第 107 行 |
| 修改商品 | 後台 → 商品與顧客 → 編輯 | `js/app.js` 第 713 行 | `js/store.js` 第 81 行 | `server/Program.cs` 第 41 行 | `server/ShopRepository.cs` 第 116 行 |
| 刪除商品 | 後台 → 商品與顧客 → 刪除 | `js/app.js` 第 694 行 | `js/store.js` 第 91 行 | `server/Program.cs` 第 42 行 | `server/ShopRepository.cs` 第 137 行 |
| 新增顧客 | 後台 → 商品與顧客 → 顧客管理 → 新增顧客 | `js/app.js` 第 753 行 | `js/store.js` 第 100 行 | `server/Program.cs` 第 44 行 | `server/ShopRepository.cs` 第 154 行 |
| 修改顧客 | 後台 → 商品與顧客 → 顧客管理 → 編輯 | `js/app.js` 第 753 行 | `js/store.js` 第 100 行 | `server/Program.cs` 第 45 行 | `server/ShopRepository.cs` 第 159 行 |
| 刪除顧客 | 後台 → 商品與顧客 → 顧客管理 → 刪除 | `js/app.js` 第 705 行 | `js/store.js` 第 111 行 | `server/Program.cs` 第 46 行 | `server/ShopRepository.cs` 第 173 行 |
| 新增訂單（結帳） | 前台 → 購物車 → 前往結帳 → 確認下單 | `js/app.js` 第 469 行 | `js/store.js` 第 120 行 | `server/Program.cs` 第 48 行 | `server/ShopRepository.cs` 第 187 行 |
| 修改訂單狀態 | 後台 → 訂單管理 → 點一筆 → 狀態 → 儲存 | `js/app.js` 第 600 行 | `js/store.js` 第 130 行 | `server/Program.cs` 第 49 行 | `server/ShopRepository.cs` 第 214 行 |
| 刪除訂單 | 後台 → 訂單管理 → 點一筆 → 刪除訂單 | `js/app.js` 第 599 行 | `js/store.js` 第 137 行 | `server/Program.cs` 第 50 行 | `server/ShopRepository.cs` 第 221 行 |
| 讀取檢視表 | 後台 → 商品與顧客 → 檢視表 | `js/app.js` 第 667 行 | `js/store.js` 第 62 行 | `server/Program.cs` 第 52 行 | `server/ShopRepository.cs` 第 234 行 |
| 查詢全部資料 | 打開網頁時 | `js/app.js` 第 55 行 | `js/store.js` 第 66 行 | `server/Program.cs` 第 38 行 | `server/ShopRepository.cs` 第 41 行 |

## SQL 檔

| 檔案 | 內容 | 對應課堂 |
|---|---|---|
| `database/01_建立資料表.sql` | CREATE TABLE、主鍵、外鍵、CHECK、UNIQUE、索引 | ch07 DDL |
| `database/02_新增資料.sql` | INSERT 一年的示範資料（由 `python/export_sql.py` 產生） | ch07 DML |
| `database/03_建立檢視表.sql` | 6 個檢視表：訂單金額、顧客 RFM、商品銷售、顧客公開資料（隱藏欄位）、熱銷前五名（TOP）、上架商品（可更新） | ch10 |
| `database/04_查詢範例.sql` | WHERE、LIKE、BETWEEN、GROUP BY／HAVING、INNER／LEFT JOIN、子查詢、INTERSECT／EXCEPT | ch06、ch08、ch09 |
| `database/05_新增修改刪除.sql` | 在 SSMS 示範 INSERT、UPDATE、DELETE、ALTER TABLE，以及外鍵、CHECK、UNIQUE 擋錯 | ch07 |
| `database/06_檢視表操作.sql` | 查詢檢視表、透過檢視表 INSERT／UPDATE、WITH CHECK OPTION、DROP VIEW | ch10 |
| `database/normalization.md` | 未正規化 → 1NF → 2NF → 3NF → BCNF | ch05 |
| `database/diagrams/er_model.png` | ER 圖（實體、關係、屬性、多值屬性、複合屬性、三元關係） | ch04 |
| `database/diagrams/schema.png` | 資料表關聯圖（主鍵、外鍵、一對多） | ch03 |

## 機器學習在哪裡

| 項目 | 位置 |
|---|---|
| RFM 特徵（最近購買、次數、金額） | `js/app.js` 第 61 行 |
| 標準化（StandardScaler） | `js/ml.js` 第 35 行 |
| K-means 分群（含 k-means++） | `js/ml.js` 第 48 行 |
| PCA 降成兩維 | `js/ml.js` 第 80 行 |
| 手肘法 | `js/app.js` 第 976 行 |
| 餘弦相似度推薦 | `js/ml.js` 第 142 行 |
| 邏輯回歸（梯度下降） | `js/ml.js` 第 111 行 |
| 5 折交叉驗證 | `js/ml.js` 第 27 行 |
| 混淆矩陣、精確率、召回率 | `js/ml.js` 第 126 行 |
| 流失規則基準（幾天沒買） | `js/app.js` 第 1095 行 |
| 推薦準確度（對照只推熱銷） | `js/app.js` 第 1067 行 |
| 切換本店／真實資料 | `js/app.js` 第 899 行 |

## 資料同步

- `python/generate_data.py`：產生一年的模擬資料（CSV）
- `python/export_sql.py`：CSV → `database/02_新增資料.sql`
- `python/sync_from_db.py`：SQL Server 目前的資料 → CSV → `data/data.js`（離線展示和資料庫一致）
- `python/check_model.py`：用 scikit-learn 對照分群、推薦、流失預測的結果
- `python/compare_baselines.py`：模型跟簡單規則、熱銷推薦比較
- `python/prepare_real_data.py`：UCI Online Retail II（真實資料）→ `data/real.js`
