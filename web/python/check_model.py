from pathlib import Path

import numpy as np
import pandas as pd
from sklearn.cluster import KMeans
from sklearn.linear_model import LogisticRegression
from sklearn.metrics import accuracy_score, confusion_matrix, precision_score, recall_score
from sklearn.metrics.pairwise import cosine_similarity
from sklearn.model_selection import KFold, cross_val_score, train_test_split
from sklearn.pipeline import make_pipeline
from sklearn.preprocessing import StandardScaler

ROOT = Path(__file__).resolve().parent.parent
items = pd.read_csv(ROOT / "data" / "order_items.csv")
head = pd.read_csv(ROOT / "data" / "orders.csv", parse_dates=["date"])
head = head[head.status != "已取消"]
items["amount"] = items.qty * items.price
orders = head.merge(items.groupby("order_id", as_index=False).amount.sum(), on="order_id")
lines = items.merge(head[["order_id", "customer_id"]], on="order_id")
end = orders.date.max()


def rfm(df, ref):
    g = df[df.date <= ref].groupby("customer_id")
    out = pd.DataFrame({
        "recency": (ref - g.date.max()).dt.days,
        "frequency": g.size(),
        "monetary": g.amount.sum(),
        "average": g.amount.mean(),
        "tenure": (ref - g.date.min()).dt.days,
    })
    return out


now = rfm(orders, end)
X = np.column_stack([now.recency, now.frequency, np.log1p(now.monetary), np.log1p(now.average)])
Z = StandardScaler().fit_transform(X)
for k in range(2, 9):
    print(f"K={k} inertia={KMeans(k, n_init=10, random_state=7).fit(Z).inertia_:.1f}")
km = KMeans(4, n_init=10, random_state=7).fit(Z)
now["group"] = km.labels_
print(now.groupby("group")[["recency", "frequency", "monetary"]].mean().round(1).assign(size=now.group.value_counts()))

matrix = pd.crosstab(lines.customer_id, lines.product_id).clip(upper=1)
sim = pd.DataFrame(cosine_similarity(matrix.T), index=matrix.columns, columns=matrix.columns)
print("P01 most similar:", sim["P01"].drop("P01").sort_values(ascending=False).head(3).round(2).to_dict())

cutoff = end - pd.Timedelta(days=90)
past = rfm(orders, cutoff)
after = set(orders[orders.date > cutoff].customer_id)
y = (~past.index.isin(after)).astype(int)
F = np.column_stack([past.recency, past.frequency, np.log1p(past.monetary), np.log1p(past.average), past.tenure])
Xtr, Xte, ytr, yte = train_test_split(F, y, test_size=0.25, random_state=3)
s = StandardScaler().fit(Xtr)
clf = LogisticRegression().fit(s.transform(Xtr), ytr)
p = clf.predict(s.transform(Xte))
print(f"Churn rate {y.mean():.2f}  ACC={accuracy_score(yte, p):.3f}  precision={precision_score(yte, p):.3f}  recall={recall_score(yte, p):.3f}")
print(confusion_matrix(yte, p, labels=[1, 0]))
cv = cross_val_score(make_pipeline(StandardScaler(), LogisticRegression()), F, y, cv=KFold(5, shuffle=True, random_state=11))
print(f"Churn 5-fold ACC {cv.mean():.3f} +/- {cv.std():.3f}")
