import csv
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DATA = ROOT / "data"


def read(name):
    with open(DATA / name, encoding="utf-8-sig") as f:
        return list(csv.DictReader(f))


def build(data_end=None):
    colors = [{"key": c["color_key"], "name": c["name"], "hex": c["hex"]} for c in read("colors.csv")]
    products = [{"id": p["product_id"], "key": p["key"], "name": p["name"], "category": p["category"], "price": int(p["price"]),
                 "sizes": p["sizes"].split("/"), "description": p["description"], "active": p["active"] in ("1", "True", "true")}
                for p in read("products.csv")]
    customers = [{"id": c["customer_id"], "name": c["name"], "email": c["email"], "phone": c["phone"], "city": c["city"], "joined": c["joined"]}
                 for c in read("customers.csv")]
    orders = [[o["order_id"], o["customer_id"], o["date"], o["status"], o["payment"], o["recipient"], o["phone"], o["city"], o["address"], o["note"]]
              for o in read("orders.csv")]
    items = sorted(read("order_items.csv"), key=lambda i: (i["order_id"], int(i["line"])))
    items = [[i["order_id"], i["product_id"], i["color"], i["size"], int(i["qty"]), int(i["price"])] for i in items]
    end = data_end or max(o[2] for o in orders)
    with open(DATA / "data.js", "w", encoding="utf-8") as f:
        f.write("const DATA_END = " + json.dumps(end) + ";\n")
        for name, value in [("COLORS", colors), ("PRODUCTS", products), ("CUSTOMERS", customers), ("ORDERS", orders), ("ORDER_ITEMS", items)]:
            f.write(f"const {name} = " + json.dumps(value, ensure_ascii=False) + ";\n")
    print(f"data.js: {len(products)} products, {len(customers)} customers, {len(orders)} orders, {len(items)} items, end {end}")


if __name__ == "__main__":
    build(sys.argv[1] if len(sys.argv) > 1 else None)
