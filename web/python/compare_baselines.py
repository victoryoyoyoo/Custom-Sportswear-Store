import csv
from collections import defaultdict
from datetime import date, timedelta
from pathlib import Path

import numpy as np
from sklearn.linear_model import LogisticRegression
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler

DATA = Path(__file__).resolve().parent.parent / "data"
WINDOW = 90


def load():
    orders = {r["order_id"]: r for r in csv.DictReader(open(DATA / "orders.csv", encoding="utf-8")) if r["status"] != "已取消"}
    items = defaultdict(list)
    for r in csv.DictReader(open(DATA / "order_items.csv", encoding="utf-8")):
        if r["order_id"] in orders:
            items[r["order_id"]].append(r)
    end = max(date.fromisoformat(o["date"]) for o in orders.values())
    return orders, items, end


def churn(orders, items, cutoff):
    by = defaultdict(list)
    for o in orders.values():
        by[o["customer_id"]].append(o)
    X, y = [], []
    for os_ in by.values():
        past = [o for o in os_ if date.fromisoformat(o["date"]) <= cutoff]
        if not past:
            continue
        ds = sorted(date.fromisoformat(o["date"]) for o in past)
        money = sum(int(i["qty"]) * int(i["price"]) for o in past for i in items[o["order_id"]])
        X.append([(cutoff - ds[-1]).days, len(past), np.log1p(money), np.log1p(money / len(past)), (cutoff - ds[0]).days])
        y.append(0 if any(date.fromisoformat(o["date"]) > cutoff for o in os_) else 1)
    X, y = np.array(X), np.array(y)
    tr, te = train_test_split(np.arange(len(y)), test_size=0.25, random_state=3)
    scaler = StandardScaler().fit(X[tr])
    model = LogisticRegression().fit(scaler.transform(X[tr]), y[tr])
    model_acc = (model.predict(scaler.transform(X[te])) == y[te]).mean()
    rule_days = max(range(7, 200, 7), key=lambda t: ((X[tr, 0] >= t) == y[tr]).mean())
    rule_acc = ((X[te, 0] >= rule_days) == y[te]).mean()
    base = max(y[te].mean(), 1 - y[te].mean())
    return len(y), y.mean(), model_acc, rule_days, rule_acc, base


def recommend(orders, items, cutoff, k=3):
    before, after = defaultdict(set), defaultdict(set)
    for o in orders.values():
        side = before if date.fromisoformat(o["date"]) <= cutoff else after
        for i in items[o["order_id"]]:
            side[o["customer_id"]].add(i["product_id"])
    products = sorted({i["product_id"] for rows in items.values() for i in rows})
    customers = sorted(before)
    m = np.array([[p in before[c] for p in products] for c in customers], float)
    norm = np.linalg.norm(m, axis=0) + 1e-9
    sim = (m.T @ m) / np.outer(norm, norm)
    np.fill_diagonal(sim, 0)
    popular = m.sum(0)
    hits_cf = hits_pop = n = 0
    for c in customers:
        new = after[c] - before[c]
        if not new:
            continue
        n += 1
        owned = np.array([p in before[c] for p in products])
        score = sim[owned].sum(0)
        score[owned] = -1
        pop = popular.copy()
        pop[owned] = -1
        hits_cf += bool(new & {products[j] for j in np.argsort(-score)[:k]})
        hits_pop += bool(new & {products[j] for j in np.argsort(-pop)[:k]})
    return n, hits_cf / n, hits_pop / n


def real():
    import json
    text = (DATA / "real.js").read_text(encoding="utf-8")
    data = json.loads(text[text.index("{"):text.rindex("}") + 1])
    rows = [c for c in data["customers"] if "past" in c]
    X = np.array([[c["past"][0], c["past"][1], np.log1p(c["past"][2]), np.log1p(c["past"][3]), c["past"][4]] for c in rows])
    y = np.array([c["churn"] for c in rows])
    from sklearn.model_selection import KFold
    model_acc, rule_acc = [], []
    for tr, te in KFold(5, shuffle=True, random_state=11).split(X):
        scaler = StandardScaler().fit(X[tr])
        model = LogisticRegression().fit(scaler.transform(X[tr]), y[tr])
        model_acc.append((model.predict(scaler.transform(X[te])) == y[te]).mean())
        days = max(range(7, 400, 7), key=lambda t: ((X[tr, 0] >= t) == y[tr]).mean())
        rule_acc.append(((X[te, 0] >= days) == y[te]).mean())
    print(f"真實資料流失預測（{len(y)} 位，流失比例 {y.mean():.1%}，5 折）")
    print(f"  邏輯回歸 {np.mean(model_acc):.1%}｜規則 {np.mean(rule_acc):.1%}｜全部猜同一類 {max(y.mean(), 1 - y.mean()):.1%}")
    n_products = len(data["products"])
    m = np.array([[j in c["before"] for j in range(n_products)] for c in rows], float)
    norm = np.linalg.norm(m, axis=0) + 1e-9
    sim = (m.T @ m) / np.outer(norm, norm)
    np.fill_diagonal(sim, 0)
    popular = m.sum(0)
    n = cf = pop = 0
    for i, c in enumerate(rows):
        new = set(c["after"]) - set(c["before"])
        if not new:
            continue
        n += 1
        owned = m[i] > 0
        score = sim[owned].sum(0)
        score[owned] = -1
        p = popular.copy()
        p[owned] = -1
        cf += bool(new & set(np.argsort(-score)[:5]))
        pop += bool(new & set(np.argsort(-p)[:5]))
    print(f"真實資料推薦前 5 名命中率（{n} 位）")
    print(f"  相似度推薦 {cf / n:.1%}｜只推熱銷商品 {pop / n:.1%}")


def main():
    orders, items, end = load()
    cutoff = end - timedelta(days=WINDOW)
    n, rate, model_acc, rule_days, rule_acc, base = churn(orders, items, cutoff)
    print(f"流失預測（{n} 位顧客，流失比例 {rate:.1%}）")
    print(f"  邏輯回歸 {model_acc:.1%}｜規則「{rule_days} 天沒買就算流失」{rule_acc:.1%}｜全部猜同一類 {base:.1%}")
    n, cf, pop = recommend(orders, items, cutoff)
    print(f"商品推薦（{n} 位之後買了新商品的顧客，推薦前 3 名命中率）")
    print(f"  相似度推薦 {cf:.1%}｜只推熱銷商品 {pop:.1%}")
    if (DATA / "real.js").exists():
        real()


if __name__ == "__main__":
    main()
