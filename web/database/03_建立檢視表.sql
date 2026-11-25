USE 運動用品商店;
GO

-- 每張訂單的金額（不另外存總額，避免明細改了總額沒改）
CREATE OR ALTER VIEW 訂單金額檢視 AS
SELECT 訂單.訂單編號, 訂單.顧客編號, 訂單.訂購日期, 訂單.狀態,
       SUM(訂單明細.數量 * 訂單明細.成交單價) AS 小計,
       SUM(訂單明細.數量) AS 件數
FROM 訂單
JOIN 訂單明細 ON 訂單明細.訂單編號 = 訂單.訂單編號
GROUP BY 訂單.訂單編號, 訂單.顧客編號, 訂單.訂購日期, 訂單.狀態;
GO

-- 顧客分群用的 RFM：最近購買、購買次數、總金額
CREATE OR ALTER VIEW 顧客RFM檢視 AS
SELECT 顧客.顧客編號, 顧客.姓名,
       DATEDIFF(DAY, MAX(訂單金額檢視.訂購日期), CAST(GETDATE() AS DATE)) AS 最近購買天數,
       COUNT(訂單金額檢視.訂單編號) AS 購買次數,
       SUM(訂單金額檢視.小計) AS 總金額,
       AVG(CAST(訂單金額檢視.小計 AS FLOAT)) AS 平均金額
FROM 顧客
JOIN 訂單金額檢視 ON 訂單金額檢視.顧客編號 = 顧客.顧客編號 AND 訂單金額檢視.狀態 <> N'已取消'
GROUP BY 顧客.顧客編號, 顧客.姓名;
GO

-- 每個商品賣了多少（沒賣過的也列出來）
CREATE OR ALTER VIEW 商品銷售檢視 AS
SELECT 商品.商品編號, 商品.商品名稱, 分類.分類名稱,
       ISNULL(SUM(CASE WHEN 訂單.狀態 <> N'已取消' THEN 訂單明細.數量 END), 0) AS 銷售件數,
       ISNULL(SUM(CASE WHEN 訂單.狀態 <> N'已取消' THEN 訂單明細.數量 * 訂單明細.成交單價 END), 0) AS 銷售金額
FROM 商品
JOIN 分類 ON 分類.分類編號 = 商品.分類編號
LEFT JOIN 訂單明細 ON 訂單明細.商品編號 = 商品.商品編號
LEFT JOIN 訂單 ON 訂單.訂單編號 = 訂單明細.訂單編號
GROUP BY 商品.商品編號, 商品.商品名稱, 分類.分類名稱;
GO
