IF DB_ID(N'運動用品商店') IS NULL
    CREATE DATABASE 運動用品商店;
GO

USE 運動用品商店;
GO

-- 先刪子資料表，再刪父資料表
DROP VIEW IF EXISTS 上架商品檢視;
DROP VIEW IF EXISTS 熱銷商品前五名檢視;
DROP VIEW IF EXISTS 顧客公開資料檢視;
DROP VIEW IF EXISTS 商品銷售檢視;
DROP VIEW IF EXISTS 顧客RFM檢視;
DROP VIEW IF EXISTS 訂單金額檢視;
DROP TABLE IF EXISTS 訂單明細;
DROP TABLE IF EXISTS 訂單;
DROP TABLE IF EXISTS 商品尺寸;
DROP TABLE IF EXISTS 商品;
DROP TABLE IF EXISTS 分類;
DROP TABLE IF EXISTS 顏色;
DROP TABLE IF EXISTS 顧客;
GO

-- 先建立父資料表（顧客、分類、顏色），再建立子資料表
CREATE TABLE 顧客
(
    顧客編號 VARCHAR(10),
    電子郵件 VARCHAR(60)  NOT NULL UNIQUE,
    手機     VARCHAR(12)  NOT NULL,
    縣市     NVARCHAR(10) NOT NULL,
    加入日期 DATE         NOT NULL DEFAULT CAST(GETDATE() AS DATE),
    PRIMARY KEY (顧客編號)
);

CREATE TABLE 分類
(
    分類編號 INT IDENTITY(1, 1),
    分類名稱 NVARCHAR(20) NOT NULL UNIQUE,
    PRIMARY KEY (分類編號)
);

CREATE TABLE 顏色
(
    顏色代碼 VARCHAR(20),
    顏色名稱 NVARCHAR(20) NOT NULL,
    色碼     CHAR(7)      NOT NULL,
    排序     INT          NOT NULL,
    PRIMARY KEY (顏色代碼)
);

CREATE TABLE 商品
(
    商品編號 VARCHAR(10),
    商品名稱 NVARCHAR(30)  NOT NULL,
    分類編號 INT           NOT NULL,
    定價     INT           NOT NULL CHECK (定價 > 0),
    圖片代碼 VARCHAR(20)   NOT NULL,
    說明     NVARCHAR(200),
    上架     BIT           NOT NULL DEFAULT 1,
    PRIMARY KEY (商品編號),
    FOREIGN KEY (分類編號) REFERENCES 分類 (分類編號)
);

CREATE TABLE 商品尺寸
(
    商品編號 VARCHAR(10),
    尺寸     NVARCHAR(10),
    排序     INT NOT NULL,
    PRIMARY KEY (商品編號, 尺寸),
    FOREIGN KEY (商品編號) REFERENCES 商品 (商品編號) ON DELETE CASCADE
);

CREATE TABLE 訂單
(
    訂單編號 VARCHAR(10),
    顧客編號 VARCHAR(10)  NOT NULL,
    訂購日期 DATE         NOT NULL,
    狀態     NVARCHAR(4)  NOT NULL CHECK (狀態 IN (N'已成立', N'已出貨', N'已完成', N'已取消')),
    付款方式 NVARCHAR(10) NOT NULL CHECK (付款方式 IN (N'信用卡', N'貨到付款', N'ATM 轉帳')),
    收件電話 VARCHAR(12),
    收件縣市 NVARCHAR(10) NOT NULL,
    收件地址 NVARCHAR(60),
    備註     NVARCHAR(80),
    PRIMARY KEY (訂單編號),
    FOREIGN KEY (顧客編號) REFERENCES 顧客 (顧客編號)
);

CREATE TABLE 訂單明細
(
    訂單編號 VARCHAR(10),
    項次     INT,
    商品編號 VARCHAR(10)  NOT NULL,
    顏色代碼 VARCHAR(20)  NOT NULL,
    尺寸     NVARCHAR(10) NOT NULL,
    數量     INT          NOT NULL CHECK (數量 BETWEEN 1 AND 20),
    成交單價 INT          NOT NULL CHECK (成交單價 > 0),
    PRIMARY KEY (訂單編號, 項次),
    FOREIGN KEY (訂單編號) REFERENCES 訂單 (訂單編號) ON DELETE CASCADE,
    FOREIGN KEY (商品編號) REFERENCES 商品 (商品編號),
    FOREIGN KEY (顏色代碼) REFERENCES 顏色 (顏色代碼)
);

CREATE INDEX 索引_訂單_顧客 ON 訂單 (顧客編號, 訂購日期);
CREATE INDEX 索引_訂單明細_商品 ON 訂單明細 (商品編號);
GO
