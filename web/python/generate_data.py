import csv
import json
from datetime import date, timedelta
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parent.parent
rng = np.random.default_rng(2026)

START = date(2025, 11, 21)
DAYS = 365
APPAREL = ["S", "M", "L", "XL", "2XL"]
SHOES = [str(s) for s in range(38, 47)]
ONE = ["單一尺寸"]

COLORS = [
    ("navy", "午夜藍", "#1B2A4A"), ("crimson", "烈焰紅", "#B3202A"), ("teal", "海港青", "#0F7C8C"),
    ("royal", "天際藍", "#2F6FD6"), ("jade", "翡翠綠", "#146B45"), ("onyx", "曜石黑", "#1A1A1F"),
    ("sand", "沙丘金", "#C08A3E"), ("violet", "極光紫", "#5B2C8F"), ("steel", "鋼鐵灰", "#4B5563"),
    ("sunset", "日落橘", "#E8671C"), ("indigo", "月光靛", "#2E3A87"), ("cocoa", "可可棕", "#5A3A22"),
]

PRODUCTS = [
    ("P01", "jersey", "客製球衣", "服裝", 1280, APPAREL, "吸濕排汗網眼布，透氣不悶熱；姓名、背號熱轉印，耐洗不脫落。"),
    ("P02", "tee", "訓練短袖 T 恤", "服裝", 590, APPAREL, "棉感排汗布，觸感柔軟；羅紋圓領，洗後不易變形。"),
    ("P03", "hoodie", "刷毛連帽上衣", "服裝", 1580, APPAREL, "內裡刷毛，賽前熱身、冬天通勤都保暖；雙層帽子附抽繩。"),
    ("P04", "shorts", "籃球褲", "服裝", 890, APPAREL, "鬆緊腰頭附抽繩，穿脫方便；輕量網眼布，快乾透氣。"),
    ("P05", "socks", "籃球襪（2 雙入）", "配件", 290, ["M", "L"], "毛巾底加厚，降低衝擊；足弓加壓，減少滑動。"),
    ("P06", "sneaker", "高筒籃球鞋", "鞋款", 3280, SHOES, "高筒設計，穩定包覆腳踝；緩震中底，落地更輕鬆。"),
    ("P07", "backpack", "球袋後背包", "配件", 1680, ONE, "獨立球袋與鞋袋；耐磨防潑水布料，前袋可印製姓名。"),
    ("P08", "bottle", "運動水壺", "配件", 450, ONE, "食品級 PP，不含雙酚 A；防漏彈蓋，單手就能喝。"),
    ("P09", "towel", "運動毛巾", "配件", 390, ONE, "純棉毛圈，吸水快；附掛環，掛在包包或場邊都方便。"),
    ("P10", "headband", "運動頭帶", "配件", 220, ONE, "毛巾布吸汗，汗水不滴進眼睛；高彈性，久戴不壓頭。"),
    ("P11", "wristband", "運動護腕（2 入）", "配件", 250, ONE, "毛巾布吸汗快乾；高彈性不勒手。"),
]
INDEX = {p[0]: i for i, p in enumerate(PRODUCTS)}

THEMES = {
    "籃球": ["P01", "P04", "P05", "P06", "P11", "P10"],
    "健身": ["P02", "P09", "P08", "P04", "P05"],
    "通勤": ["P03", "P07", "P08", "P02"],
}

TYPES = {
    "loyal": {"share": 0.2, "gap": 18, "items": (2, 4), "premium": 0.6},
    "regular": {"share": 0.28, "gap": 40, "items": (1, 3), "premium": 0.3},
    "bargain": {"share": 0.14, "gap": 35, "items": (1, 2), "premium": 0.05},
    "lapsed": {"share": 0.12, "gap": 30, "items": (1, 3), "premium": 0.3},
    "oneoff": {"share": 0.14, "gap": 20, "items": (1, 2), "premium": 0.1},
    "slow": {"share": 0.12, "gap": 85, "items": (2, 3), "premium": 0.9},
}

SURNAMES = list("陳林黃張李王吳劉蔡楊許鄭謝郭洪曾邱廖賴周徐蘇葉莊呂江何蕭羅高")
GIVEN = list("子宇承恩柏翰品妤欣怡家豪冠廷宥辰雅婷俊傑思穎佳蓉育誠詠晴志明書瑋嘉玲博文")
CITIES = ["台北市", "新北市", "桃園市", "台中市", "台南市", "高雄市", "新竹市", "基隆市", "嘉義市", "宜蘭縣"]
PAYMENTS = ["信用卡", "貨到付款", "ATM 轉帳"]

customers = []
orders = []
lines = []
order_no = 1
n_customers = 400
type_names = list(TYPES)
shares = [TYPES[t]["share"] for t in type_names]
used_names = set()

for c in range(n_customers):
    kind = rng.choice(type_names, p=shares)
    spec = TYPES[kind]
    theme = str(rng.choice(list(THEMES), p=[0.45, 0.35, 0.2]))
    cid = f"C{c + 1:03d}"
    while True:
        name = rng.choice(SURNAMES) + "".join(rng.choice(GIVEN, size=2))
        if name not in used_names:
            used_names.add(name)
            break
    city = str(rng.choice(CITIES))
    fav = [COLORS[i][0] for i in rng.choice(len(COLORS), size=3, replace=False)]
    apparel = str(rng.choice(APPAREL, p=[0.12, 0.3, 0.3, 0.2, 0.08]))
    shoe = str(rng.choice(SHOES[2:8]))
    day = int(rng.integers(150, 290)) if kind == "oneoff" else int(rng.integers(0, 120))
    joined = START + timedelta(days=max(0, day - int(rng.integers(0, 30))))
    customers.append({
        "id": cid, "name": str(name), "email": f"user{c + 1:03d}@example.com",
        "phone": f"09{rng.integers(10, 99)}-{rng.integers(100, 999)}-{rng.integers(100, 999)}",
        "city": city, "joined": joined.isoformat(),
    })
    stop = int(rng.integers(170, 275)) if kind == "lapsed" else day + int(rng.integers(1, 30)) if kind == "oneoff" else DAYS
    gap_scale = 1.0
    first = True
    while day < min(stop, DAYS) or (first and kind == "oneoff"):
        first = False
        pool = THEMES[theme]
        if rng.random() < 0.06:
            pool = [p[0] for p in PRODUCTS]
        if kind == "bargain":
            pool = [p for p in pool if PRODUCTS[INDEX[p]][4] <= 900] or ["P05", "P10"]
        k = int(rng.integers(spec["items"][0], spec["items"][1] + 1))
        picked = list(rng.choice(pool, size=min(k, len(pool)), replace=False))
        if rng.random() < spec["premium"] and "P06" not in picked and theme == "籃球":
            picked.append("P06")
        oid = f"O{order_no:05d}"
        when = START + timedelta(days=day)
        status = "已完成" if day < DAYS - 6 else "已出貨" if day < DAYS - 2 else "已成立"
        if order_no % 67 == 0:
            status = "已取消"
        orders.append({"id": oid, "customer": cid, "date": when.isoformat(), "status": status,
                       "payment": str(rng.choice(PAYMENTS, p=[0.55, 0.3, 0.15])), "recipient": str(name), "city": city})
        for pid in picked:
            p = PRODUCTS[INDEX[pid]]
            size = apparel if p[5] is APPAREL else shoe if p[5] is SHOES else str(rng.choice(p[5]))
            color = str(rng.choice(fav)) if rng.random() < 0.7 else COLORS[int(rng.integers(len(COLORS)))][0]
            qty = 1 if rng.random() < 0.8 else 2
            lines.append([oid, pid, color, size, qty, p[4]])
        order_no += 1
        if kind == "lapsed" and day > 60:
            gap_scale *= 1.3
        day += max(3, int(rng.gamma(3, spec["gap"] * gap_scale / 3)))

def line_numbers(rows):
    seen = {}
    out = []
    for r in rows:
        seen[r[0]] = seen.get(r[0], 0) + 1
        out.append(seen[r[0]])
    return out


def write_csv(name, header, rows):
    with open(ROOT / "data" / name, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(header)
        w.writerows(rows)


phones = {c["id"]: c["phone"] for c in customers}
write_csv("colors.csv", ["color_key", "name", "hex"], COLORS)
write_csv("products.csv", ["product_id", "key", "name", "category", "price", "sizes", "description", "active"],
          [[p[0], p[1], p[2], p[3], p[4], "/".join(p[5]), p[6], 1] for p in PRODUCTS])
write_csv("customers.csv", ["customer_id", "name", "email", "phone", "city", "joined"], [list(c.values()) for c in customers])
write_csv("orders.csv", ["order_id", "customer_id", "date", "status", "payment", "recipient", "phone", "city", "address", "note"],
          [[o["id"], o["customer"], o["date"], o["status"], o["payment"], o["recipient"], phones[o["customer"]], o["city"], "", ""] for o in orders])
write_csv("order_items.csv", ["order_id", "line", "product_id", "color", "size", "qty", "price"],
          [[l[0], n, *l[1:]] for l, n in zip(lines, line_numbers(lines))])

print(f"{len(customers)} customers, {len(orders)} orders, {len(lines)} items")

if __name__ == "__main__":
    from build_datajs import build
    build()
