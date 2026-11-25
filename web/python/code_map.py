import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "docs" / "code-map.md"


def find(rel, pattern):
    lines = (ROOT / rel).read_text(encoding="utf-8").splitlines()
    for n, line in enumerate(lines, 1):
        if re.search(pattern, line):
            return f"`{rel}` 第 {n} 行"
    raise SystemExit(f"找不到 {pattern} in {rel}")


CRUD = [
    ("新增商品", "管理 → 新增商品", r"function productForm", r"saveProduct", r"MapPost\(\"/products\"", r"INSERT INTO 商品 \("),
    ("修改商品", "管理 → 編輯", r"function productForm", r"saveProduct", r"MapPut\(\"/products/", r"UPDATE 商品$"),
    ("刪除商品", "管理 → 刪除", r"data-del-p", r"deleteProduct", r"MapDelete\(\"/products/", r"DELETE FROM 商品 WHERE"),
    ("新增顧客", "管理 → 顧客管理 → 新增顧客", r"function customerForm", r"saveCustomer", r"MapPost\(\"/customers\"", r"INSERT INTO 顧客"),
    ("修改顧客", "管理 → 顧客管理 → 編輯", r"function customerForm", r"saveCustomer", r"MapPut\(\"/customers/", r"UPDATE 顧客 SET"),
    ("刪除顧客", "管理 → 顧客管理 → 刪除", r"data-del-c", r"deleteCustomer", r"MapDelete\(\"/customers/", r"DELETE FROM 顧客 WHERE"),
    ("新增訂單（結帳）", "購物車 → 前往結帳 → 確認下單", r"async function submitOrder", r"createOrder", r"MapPost\(\"/orders\"", r"INSERT INTO 訂單 \("),
    ("修改訂單狀態", "訂單 → 點一筆 → 狀態 → 儲存", r"od-save", r"updateOrder", r"MapMethods\(\"/orders/", r"UPDATE 訂單 SET 狀態"),
    ("刪除訂單", "訂單 → 點一筆 → 刪除訂單", r"od-delete", r"deleteOrder", r"MapDelete\(\"/orders/", r"DELETE FROM 訂單 WHERE"),
    ("查詢全部資料", "打開網頁時", r"async function reload", r"async function all", r"MapGet\(\"/all\"", r"SELECT 商品.商品編號, 商品.圖片代碼"),
]

ML = [
    ("RFM 特徵（最近購買、次數、金額）", r"function rfm\("),
    ("標準化（StandardScaler）", r"function fitScaler"),
    ("K-means 分群（含 k-means++）", r"function kmeans"),
    ("PCA 降成兩維", r"function pca2"),
    ("手肘法", r"const elbow"),
    ("餘弦相似度推薦", r"function cosine"),
    ("邏輯回歸（梯度下降）", r"function fitLogisticRegression"),
    ("5 折交叉驗證", r"function kFolds"),
    ("混淆矩陣、精確率、召回率", r"function classificationMetrics"),
    ("流失規則基準（幾天沒買）", r"function bestRuleDays"),
    ("推薦準確度（對照只推熱銷）", r"function recommendEval"),
    ("切換本店／真實資料", r"function realSource"),
]
ML_FILES = {r"function rfm\(": "js/app.js", r"const elbow": "js/app.js", r"function bestRuleDays": "js/app.js",
            r"function recommendEval": "js/app.js", r"function realSource": "js/app.js"}


def main():
    out = ["# 程式導覽", "", "這份由 `python/code_map.py` 產生，程式改了重跑一次就會更新行號。", "",
           "## 架構", "",
           "```", "瀏覽器（index.html、js/app.js）", "   │  js/store.js 判斷資料來源", "   ├─ 直接開 index.html → 本機示範資料（data/data.js）",
           "   └─ 從 C# 伺服器開 → fetch /api/... → server/Program.cs → server/ShopRepository.cs → SQL Server（運動用品商店）", "```", "",
           "## 新增、修改、刪除、查詢在哪裡", "",
           "| 功能 | 畫面操作 | 前端 | 資料層 | API 路由 | SQL |", "|---|---|---|---|---|---|"]
    for name, ui, fe, st, route, sql in CRUD:
        out.append(f"| {name} | {ui} | {find('js/app.js', fe)} | {find('js/store.js', st)} | {find('server/Program.cs', route)} | {find('server/ShopRepository.cs', sql)} |")
    out += ["", "## SQL 檔", "",
            "| 檔案 | 內容 | 對應課堂 |", "|---|---|---|",
            "| `database/01_建立資料表.sql` | CREATE TABLE、主鍵、外鍵、CHECK、UNIQUE、索引 | ch07 DDL |",
            "| `database/02_新增資料.sql` | INSERT 一年的示範資料（由 `python/export_sql.py` 產生） | ch07 DML |",
            "| `database/03_建立檢視表.sql` | 檢視表：訂單總額、顧客 RFM、商品銷售 | ch08 GROUP BY、ch09 JOIN |",
            "| `database/04_查詢範例.sql` | WHERE、LIKE、BETWEEN、GROUP BY／HAVING、INNER／LEFT JOIN、子查詢、INTERSECT／EXCEPT | ch06、ch08、ch09 |",
            "| `database/05_新增修改刪除.sql` | 在 SSMS 示範 INSERT、UPDATE、DELETE、ALTER TABLE，以及外鍵、CHECK、UNIQUE 擋錯 | ch07 |",
            "| `database/normalization.md` | 未正規化 → 1NF → 2NF → 3NF → BCNF | ch05 |",
            "| `database/diagrams/er_model.png` | ER 圖（實體、關係、屬性、多值屬性、複合屬性、三元關係） | ch04 |",
            "| `database/diagrams/schema.png` | 資料表關聯圖（主鍵、外鍵、一對多） | ch03 |",
            "", "## 機器學習在哪裡", "", "| 項目 | 位置 |", "|---|---|"]
    for name, pat in ML:
        out.append(f"| {name} | {find(ML_FILES.get(pat, 'js/ml.js'), pat)} |")
    out += ["", "## 資料同步", "",
            "- `python/generate_data.py`：產生一年的模擬資料（CSV）",
            "- `python/export_sql.py`：CSV → `database/02_新增資料.sql`",
            "- `python/sync_from_db.py`：SQL Server 目前的資料 → CSV → `data/data.js`（離線展示和資料庫一致）",
            "- `python/check_model.py`：用 scikit-learn 對照分群、推薦、流失預測的結果",
            "- `python/compare_baselines.py`：模型跟簡單規則、熱銷推薦比較",
            "- `python/prepare_real_data.py`：UCI Online Retail II（真實資料）→ `data/real.js`", ""]
    OUT.parent.mkdir(exist_ok=True)
    OUT.write_text("\n".join(out), encoding="utf-8")
    print(OUT)


if __name__ == "__main__":
    main()
