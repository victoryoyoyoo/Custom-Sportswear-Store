import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DATA = ROOT / "data"
OUT = ROOT / "database" / "02_新增資料.sql"


def read(name):
    with open(DATA / name, encoding="utf-8-sig") as f:
        return list(csv.DictReader(f))


def q(text):
    return "N'" + str(text).replace("'", "''") + "'"


def batches(table, columns, rows, size=500):
    out = []
    for i in range(0, len(rows), size):
        values = ",\n".join("(" + ", ".join(r) + ")" for r in rows[i:i + size])
        out.append(f"INSERT INTO {table} ({', '.join(columns)}) VALUES\n{values};")
    return out


products = read("products.csv")
categories = sorted({p["category"] for p in products}, key=["服裝", "鞋款", "配件"].index)
cat_id = {c: i + 1 for i, c in enumerate(categories)}

parts = ["USE 運動用品商店;", "GO", "SET NOCOUNT ON;"]
parts += batches("分類", ["分類名稱"], [[q(c)] for c in categories])
parts += batches("顏色", ["顏色代碼", "顏色名稱", "色碼", "排序"], [[q(c["color_key"]), q(c["name"]), q(c["hex"]), str(i + 1)] for i, c in enumerate(read("colors.csv"))])
parts += batches("商品", ["商品編號", "商品名稱", "分類編號", "定價", "圖片代碼", "說明", "上架"],
                 [[q(p["product_id"]), q(p["name"]), str(cat_id[p["category"]]), p["price"], q(p["key"]), q(p["description"]), "1" if p["active"] in ("1", "True", "true") else "0"] for p in products])
parts += batches("商品尺寸", ["商品編號", "尺寸", "排序"],
                 [[q(p["product_id"]), q(s), str(i + 1)] for p in products for i, s in enumerate(p["sizes"].split("/"))])
parts += batches("顧客", ["顧客編號", "姓名", "電子郵件", "手機", "縣市", "加入日期"],
                 [[q(c["customer_id"]), q(c["name"]), q(c["email"]), q(c["phone"]), q(c["city"]), q(c["joined"])] for c in read("customers.csv")])
nul = lambda v: q(v) if v else "NULL"
parts += batches("訂單", ["訂單編號", "顧客編號", "訂購日期", "狀態", "付款方式", "收件人", "收件電話", "收件縣市", "收件地址", "備註"],
                 [[q(o["order_id"]), q(o["customer_id"]), q(o["date"]), q(o["status"]), q(o["payment"]), q(o["recipient"]), nul(o["phone"]), q(o["city"]), nul(o["address"]), nul(o["note"])] for o in read("orders.csv")])
items = [[q(it["order_id"]), it["line"], q(it["product_id"]), q(it["color"]), q(it["size"]), it["qty"], it["price"]] for it in read("order_items.csv")]
parts += batches("訂單明細", ["訂單編號", "項次", "商品編號", "顏色代碼", "尺寸", "數量", "成交單價"], items)
parts.append("GO")

OUT.write_text("\n".join(parts) + "\n", encoding="utf-8-sig")
print(OUT.name, round(OUT.stat().st_size / 1e6, 2), "MB")
