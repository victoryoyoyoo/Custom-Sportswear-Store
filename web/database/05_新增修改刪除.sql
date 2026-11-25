USE 運動用品商店;
GO

-- 1. INSERT（ch07）：新增一位顧客
INSERT INTO 顧客 (顧客編號, 電子郵件, 手機, 縣市)
VALUES ('C999', 'demo999@example.com', '0912-345-678', N'台北市');

SELECT * FROM 顧客 WHERE 顧客編號 = 'C999';

-- 2. INSERT：幫他下一張訂單（先新增父資料表「訂單」，再新增子資料表「訂單明細」）
INSERT INTO 訂單 (訂單編號, 顧客編號, 訂購日期, 狀態, 付款方式, 收件電話, 收件縣市, 收件地址)
VALUES ('O99999', 'C999', CAST(GETDATE() AS DATE), N'已成立', N'信用卡', '0912-345-678', N'台北市', N'中正區忠孝西路一段 1 號');

INSERT INTO 訂單明細 (訂單編號, 項次, 商品編號, 顏色代碼, 尺寸, 數量, 成交單價)
VALUES ('O99999', 1, 'P01', 'navy', 'L', 1, 1280),
       ('O99999', 2, 'P08', 'teal', N'單一尺寸', 2, 450);

SELECT * FROM 訂單金額檢視 WHERE 訂單編號 = 'O99999';

-- 3. UPDATE（ch07）：顧客換手機、訂單出貨
UPDATE 顧客
SET 手機 = '0987-654-321'
WHERE 顧客編號 = 'C999';

UPDATE 訂單
SET 狀態 = N'已出貨'
WHERE 訂單編號 = 'O99999';

SELECT 顧客.顧客編號, 顧客.手機, 訂單.訂單編號, 訂單.狀態
FROM 顧客 JOIN 訂單 ON 訂單.顧客編號 = 顧客.顧客編號
WHERE 顧客.顧客編號 = 'C999';

-- 4. 完整性限制：下面這幾行會被資料庫擋下來（可以一行一行選起來執行看錯誤訊息）
-- 外鍵：顧客 C999 有訂單，不能直接刪
-- DELETE FROM 顧客 WHERE 顧客編號 = 'C999';
-- CHECK：狀態只能是 已成立／已出貨／已完成／已取消
-- UPDATE 訂單 SET 狀態 = N'不見了' WHERE 訂單編號 = 'O99999';
-- UNIQUE：電子郵件不能重複
-- INSERT INTO 顧客 (顧客編號, 電子郵件, 手機, 縣市) VALUES ('C998', 'demo999@example.com', '0911-111-111', N'台中市');

-- 5. DELETE（ch07）：先刪子資料表再刪父資料表
-- 訂單明細設了 ON DELETE CASCADE，刪訂單時明細會一起刪掉
DELETE FROM 訂單 WHERE 訂單編號 = 'O99999';
DELETE FROM 顧客 WHERE 顧客編號 = 'C999';

SELECT COUNT(*) AS 剩下的示範資料 FROM 顧客 WHERE 顧客編號 = 'C999';

-- 6. ALTER TABLE（ch07）：加一個欄位，再拿掉
ALTER TABLE 顧客 ADD 生日 DATE NULL;
SELECT TOP 3 * FROM 顧客;
ALTER TABLE 顧客 DROP COLUMN 生日;
GO
