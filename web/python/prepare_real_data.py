import json
import sys
from pathlib import Path

import numpy as np
import pandas as pd

ROOT = Path(__file__).resolve().parent.parent
RAW = ROOT / "data" / "raw"
OUT = ROOT / "data" / "real.js"
WINDOW = 90
TOP_PRODUCTS = 100


def load():
    csv = RAW / "online_retail_ii.csv"
    if csv.exists():
        df = pd.read_csv(csv, dtype={"StockCode": str, "Invoice": str}, parse_dates=["InvoiceDate"])
    else:
        xlsx = RAW / "online_retail_II.xlsx"
        if not xlsx.exists():
            sys.exit("請先把 online_retail_II.xlsx 放到 data/raw/（下載：https://archive.ics.uci.edu/dataset/502/online+retail+ii）")
        df = pd.concat(pd.read_excel(xlsx, sheet_name=None, dtype={"StockCode": str, "Invoice": str}).values(), ignore_index=True)
        df.to_csv(csv, index=False)
    raw_rows = len(df)
    df = df.dropna(subset=["Customer ID"])
    df = df[~df["Invoice"].str.startswith("C") & (df["Quantity"] > 0) & (df["Price"] > 0) & df["StockCode"].str.match(r"^\d{5}")]
    df = df.assign(customer=df["Customer ID"].astype(int).astype(str), day=df["InvoiceDate"].dt.normalize(), amount=df["Quantity"] * df["Price"])
    return df, raw_rows


def profiles(invoices, ref):
    g = invoices[invoices["day"] <= ref].groupby("customer")
    out = pd.DataFrame({
        "recency": (ref - g["day"].max()).dt.days,
        "frequency": g.size(),
        "monetary": g["amount"].sum().round(2),
        "tenure": (ref - g["day"].min()).dt.days,
    })
    out["average"] = (out["monetary"] / out["frequency"]).round(2)
    return out


def main():
    df, raw_rows = load()
    end = df["day"].max()
    cutoff = end - pd.Timedelta(days=WINDOW)
    invoices = df.groupby(["Invoice", "customer"], as_index=False).agg(day=("day", "first"), amount=("amount", "sum"))

    now = profiles(invoices, end)
    past = profiles(invoices, cutoff)
    came_back = set(invoices.loc[invoices["day"] > cutoff, "customer"])

    reach = df.groupby("StockCode")["customer"].nunique().sort_values(ascending=False)
    top = list(reach.index[:TOP_PRODUCTS])
    names = df[df["StockCode"].isin(top)].groupby("StockCode")["Description"].agg(lambda s: s.mode().iat[0]).str.strip().str.capitalize()
    index = {p: i for i, p in enumerate(top)}
    picked = df[df["StockCode"].isin(top)]
    bought = lambda part: part.groupby("customer")["StockCode"].agg(lambda s: sorted({index[p] for p in s}))
    owned_all = bought(picked)
    owned_before = bought(picked[picked["day"] <= cutoff])
    owned_after = bought(picked[picked["day"] > cutoff])

    customers = []
    for cid, row in now.iterrows():
        entry = {
            "id": cid,
            "now": [int(row.recency), int(row.frequency), float(row.monetary), float(row.average), int(row.tenure)],
            "owned": owned_all.get(cid, []),
        }
        if cid in past.index:
            p = past.loc[cid]
            entry["past"] = [int(p.recency), int(p.frequency), float(p.monetary), float(p.average), int(p.tenure)]
            entry["churn"] = 0 if cid in came_back else 1
            entry["before"] = owned_before.get(cid, [])
            entry["after"] = owned_after.get(cid, [])
        customers.append(entry)

    meta = {
        "name": "UCI Online Retail II",
        "url": "https://archive.ics.uci.edu/dataset/502/online+retail+ii",
        "license": "CC BY 4.0",
        "from": df["day"].min().date().isoformat(),
        "to": end.date().isoformat(),
        "cutoff": cutoff.date().isoformat(),
        "rawRows": raw_rows,
        "rows": len(df),
        "invoices": int(invoices["Invoice"].nunique()),
        "customers": len(customers),
        "products": int(df["StockCode"].nunique()),
    }
    data = {"meta": meta, "products": [{"id": p, "name": names[p]} for p in top], "customers": customers}
    OUT.write_text("const REAL = " + json.dumps(data, ensure_ascii=False, separators=(",", ":")) + ";\n", encoding="utf-8")
    print(f"{OUT.name}: {meta['customers']} customers, {len(top)} products, {OUT.stat().st_size / 1024:.0f} KB")


if __name__ == "__main__":
    main()
