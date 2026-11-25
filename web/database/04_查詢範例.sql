USE 運動用品商店;
GO

-- 1. WHERE、比較運算子（ch08）：定價 1000 元以上、還在上架的商品
SELECT 商品編號, 商品名稱, 定價
FROM 商品
WHERE 定價 >= 1000 AND 上架 = 1
ORDER BY 定價 DESC;

-- 2. LIKE 模糊查詢（ch08）：名稱有「籃球」的商品
SELECT 商品編號, 商品名稱, 定價
FROM 商品
WHERE 商品名稱 LIKE N'%籃球%';

-- 3. BETWEEN 範圍查詢（ch08）：2026 年 9 月的訂單
SELECT 訂單編號, 顧客編號, 訂購日期, 狀態
FROM 訂單
WHERE 訂購日期 BETWEEN '2026-09-01' AND '2026-09-30';

-- 4. 聚合函數、GROUP BY、HAVING（ch08）：每個縣市的顧客數，只列 30 人以上
SELECT 縣市, COUNT(*) AS 顧客數
FROM 顧客
GROUP BY 縣市
HAVING COUNT(*) >= 30
ORDER BY 顧客數 DESC;

-- 5. 內部合併 INNER JOIN（ch09）：訂單明細加上顧客縣市、商品、顏色名稱
SELECT TOP 20 訂單.訂單編號, 訂單.訂購日期, 顧客.縣市, 商品.商品名稱, 顏色.顏色名稱, 訂單明細.尺寸, 訂單明細.數量, 訂單明細.成交單價
FROM 訂單
JOIN 顧客 ON 顧客.顧客編號 = 訂單.顧客編號
JOIN 訂單明細 ON 訂單明細.訂單編號 = 訂單.訂單編號
JOIN 商品 ON 商品.商品編號 = 訂單明細.商品編號
JOIN 顏色 ON 顏色.顏色代碼 = 訂單明細.顏色代碼
ORDER BY 訂單.訂購日期 DESC;

-- 6. 左外部合併 LEFT OUTER JOIN（ch09）：每個商品賣了幾件，沒賣過的也要列出
SELECT 商品.商品編號, 商品.商品名稱, ISNULL(SUM(訂單明細.數量), 0) AS 銷售件數
FROM 商品
LEFT JOIN 訂單明細 ON 訂單明細.商品編號 = 商品.商品編號
GROUP BY 商品.商品編號, 商品.商品名稱
ORDER BY 銷售件數 DESC;

-- 7. 子查詢（ch09 隨堂練習）：消費總額高於平均的顧客
SELECT 顧客編號, 總金額
FROM 顧客RFM檢視
WHERE 總金額 > (SELECT AVG(總金額) FROM 顧客RFM檢視)
ORDER BY 總金額 DESC;

-- 8. 子查詢 NOT IN：最近 90 天沒有下單的顧客（流失名單）
SELECT 顧客編號, 縣市
FROM 顧客
WHERE 顧客編號 NOT IN (
    SELECT 顧客編號 FROM 訂單
    WHERE 訂購日期 > DATEADD(DAY, -90, CAST(GETDATE() AS DATE)) AND 狀態 <> N'已取消'
);

-- 9. 每月營收（訂單頁的折線圖）
SELECT FORMAT(訂購日期, 'yyyy-MM') AS 月份, SUM(小計) AS 營收
FROM 訂單金額檢視
WHERE 狀態 <> N'已取消'
GROUP BY FORMAT(訂購日期, 'yyyy-MM')
ORDER BY 月份;

-- 10. 自我合併：被同一位顧客一起買過的商品組合（推薦功能的原始資料）
SELECT TOP 10 甲.商品編號 AS 商品甲, 乙.商品編號 AS 商品乙, COUNT(DISTINCT 訂單甲.顧客編號) AS 顧客數
FROM 訂單明細 AS 甲
JOIN 訂單 AS 訂單甲 ON 訂單甲.訂單編號 = 甲.訂單編號
JOIN 訂單 AS 訂單乙 ON 訂單乙.顧客編號 = 訂單甲.顧客編號
JOIN 訂單明細 AS 乙 ON 乙.訂單編號 = 訂單乙.訂單編號 AND 乙.商品編號 > 甲.商品編號
GROUP BY 甲.商品編號, 乙.商品編號
ORDER BY 顧客數 DESC;

-- 11. 交集 INTERSECT（ch06 關聯式代數）：同時買過球衣（P01）和籃球鞋（P06）的顧客
SELECT 訂單.顧客編號 FROM 訂單 JOIN 訂單明細 ON 訂單明細.訂單編號 = 訂單.訂單編號 WHERE 訂單明細.商品編號 = 'P01'
INTERSECT
SELECT 訂單.顧客編號 FROM 訂單 JOIN 訂單明細 ON 訂單明細.訂單編號 = 訂單.訂單編號 WHERE 訂單明細.商品編號 = 'P06';

-- 12. 差集 EXCEPT（ch06）：買過球衣、但沒買過籃球鞋的顧客（可以推薦球鞋）
SELECT 訂單.顧客編號 FROM 訂單 JOIN 訂單明細 ON 訂單明細.訂單編號 = 訂單.訂單編號 WHERE 訂單明細.商品編號 = 'P01'
EXCEPT
SELECT 訂單.顧客編號 FROM 訂單 JOIN 訂單明細 ON 訂單明細.訂單編號 = 訂單.訂單編號 WHERE 訂單明細.商品編號 = 'P06';
