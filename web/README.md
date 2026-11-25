# 運動用品商店

可以實際下單的運動用品商店，資料存在 SQL Server，後台用機器學習分析顧客。

- 商品、購物車、結帳、訂單
- 管理：商品、顧客的新增、修改、刪除，訂單狀態修改與刪除
- 顧客分群：K-means（最近購買、購買次數、總金額、平均金額），PCA 畫圖，手肘法選 K
- 商品推薦：顧客 × 商品購買紀錄的餘弦相似度
- 流失預測：邏輯回歸，預測接下來 90 天會不會再買，附 5 折交叉驗證
- 每個模型都跟簡單做法比較（幾天沒買就算流失、只推熱銷商品），分析頁可以切換成真實資料

## 真實資料

[UCI Online Retail II](https://archive.ics.uci.edu/dataset/502/online+retail+ii)（Daqing Chen，CC BY 4.0），英國網路商店 2009–2011 年的交易。`data/real.js` 是整理好的結果；要重新產生，把 `online_retail_II.xlsx` 放進 `data/raw/`，執行 `python python/prepare_real_data.py`。

| | 模型 | 簡單做法 | 全部猜同一類 |
|---|---|---|---|
| 流失預測（5,256 位，5 折） | 73% | 71%（119 天沒買） | 56% |
| 推薦前 5 名命中率（1,624 位） | 32% | 27%（只推熱銷） | |

## 執行

### 連 SQL Server

1. 用 SSMS 依序執行 `database/01_建立資料表.sql`、`02_新增資料.sql`、`03_建立檢視表.sql`（`04_查詢範例.sql` 是課堂語法的查詢範例）
2. 需要 .NET 8 SDK，在 `server` 資料夾執行 `dotnet run`，或雙擊 `server/start.bat`
3. 打開 http://localhost:5080

連線字串在 `server/appsettings.json`，預設是 `localhost\SQLSERVER2025`、資料庫 `運動用品商店`、Windows 驗證。

### 不連資料庫

直接用 Chrome 或 Edge 打開 `index.html`，會用 `data/data.js` 的示範資料，新增的內容存在瀏覽器裡。

## 檔案

- `index.html`、`style.css`、`js/`：網頁（`store.js` 決定資料來自 SQL Server 還是示範資料）
- `server/`：C# ASP.NET Core 伺服器，`ShopRepository.cs` 放所有 SQL
- `database/`：建表、資料、檢視表、查詢範例、正規化、ER 圖
- `data/`：CSV、網頁用的 `data.js`、真實資料 `real.js`
- `python/`：產生資料、整理真實資料、匯出 SQL、從資料庫同步、用 scikit-learn 對照、跟簡單做法比較、產生程式導覽
- `docs/code-map.md`：每個功能的程式位置
