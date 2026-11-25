USE 運動用品商店;
GO

-- 1. 查詢檢視表（ch10 範例 2）：跟查一般資料表一樣
SELECT * FROM 熱銷商品前五名檢視;

SELECT 顧客編號, 總金額
FROM 顧客RFM檢視
WHERE 購買次數 >= 20
ORDER BY 總金額 DESC;

-- 顧客公開資料檢視看不到電子郵件和手機
SELECT TOP 5 * FROM 顧客公開資料檢視;

-- 2. 透過檢視表新增（ch10 10-3-1）：資料其實寫進基底資料表「商品」
INSERT INTO 上架商品檢視 (商品編號, 商品名稱, 分類編號, 定價, 圖片代碼, 說明, 上架)
VALUES ('P99', N'示範護膝', 3, 590, 'wristband', N'透過檢視表新增', 1);

SELECT 商品編號, 商品名稱, 定價, 上架 FROM 商品 WHERE 商品編號 = 'P99';

-- 3. 透過檢視表修改（ch10 10-3-2）：基底資料表跟著變
UPDATE 上架商品檢視
SET 定價 = 650
WHERE 商品編號 = 'P99';

SELECT 商品編號, 商品名稱, 定價 FROM 商品 WHERE 商品編號 = 'P99';

-- WITH CHECK OPTION：改成下架後就不屬於「上架商品」了，資料庫會擋下，商品維持上架
BEGIN TRY
    UPDATE 上架商品檢視 SET 上架 = 0 WHERE 商品編號 = 'P99';
END TRY
BEGIN CATCH
    SELECT ERROR_NUMBER() AS 錯誤代碼, ERROR_MESSAGE() AS 被擋下的原因;
END CATCH;

SELECT 商品編號, 商品名稱, 上架 FROM 商品 WHERE 商品編號 = 'P99';

-- 刪掉示範資料
DELETE FROM 上架商品檢視 WHERE 商品編號 = 'P99';
GO

-- 4. 刪除檢視表（ch10 10-4）：不會影響基底資料表
CREATE VIEW 示範檢視 AS SELECT 商品編號, 商品名稱 FROM 商品;
GO
SELECT COUNT(*) AS 檢視表筆數 FROM 示範檢視;
DROP VIEW 示範檢視;
SELECT COUNT(*) AS 商品筆數_刪除檢視表後 FROM 商品;
GO
