import csv
import subprocess
import tempfile
from datetime import date
from pathlib import Path

from build_datajs import build

ROOT = Path(__file__).resolve().parent.parent
DATA = ROOT / "data"
SERVER = r"localhost\SQLSERVER2025"
SQLCMD = r"C:\Program Files\Microsoft SQL Server\Client SDK\ODBC\180\Tools\Binn\SQLCMD.EXE"

QUERIES = {
    "colors.csv": (["color_key", "name", "hex"], "SELECT 顏色代碼, 顏色名稱, 色碼 FROM 顏色 ORDER BY 排序"),
    "products.csv": (["product_id", "key", "name", "category", "price", "sizes", "description", "active"], """
        SELECT 商品.商品編號, 商品.圖片代碼, 商品.商品名稱, 分類.分類名稱, 商品.定價,
               (SELECT STRING_AGG(商品尺寸.尺寸, '/') WITHIN GROUP (ORDER BY 商品尺寸.排序) FROM 商品尺寸 WHERE 商品尺寸.商品編號 = 商品.商品編號),
               ISNULL(商品.說明, ''), CAST(商品.上架 AS INT)
        FROM 商品 JOIN 分類 ON 分類.分類編號 = 商品.分類編號 ORDER BY 商品.商品編號"""),
    "customers.csv": (["customer_id", "email", "phone", "city", "joined"],
                      "SELECT 顧客編號, 電子郵件, 手機, 縣市, CONVERT(CHAR(10), 加入日期, 23) FROM 顧客 ORDER BY 顧客編號"),
    "orders.csv": (["order_id", "customer_id", "date", "status", "payment", "phone", "city", "address", "note"],
                   "SELECT 訂單編號, 顧客編號, CONVERT(CHAR(10), 訂購日期, 23), 狀態, 付款方式, ISNULL(收件電話, ''), 收件縣市, ISNULL(收件地址, ''), ISNULL(備註, '') FROM 訂單 ORDER BY 訂單編號"),
    "order_items.csv": (["order_id", "line", "product_id", "color", "size", "qty", "price"],
                        "SELECT 訂單編號, 項次, 商品編號, 顏色代碼, 尺寸, 數量, 成交單價 FROM 訂單明細 ORDER BY 訂單編號, 項次"),
}


def win(path):
    return subprocess.run(["wslpath", "-w", str(path)], capture_output=True, text=True, check=True).stdout.strip()


def query(sql):
    with tempfile.NamedTemporaryFile(suffix=".txt", dir="/mnt/c/Users/Public", delete=False) as tmp:
        out = Path(tmp.name)
    cmd = (f"& '{SQLCMD}' -S '{SERVER}' -E -C -d 運動用品商店 -b -f o:65001 -W -h -1 -s '\u001f' "
           f"-Q \"SET NOCOUNT ON; {' '.join(sql.split())}\" -o '{win(out)}'")
    subprocess.run(["powershell.exe", "-NoProfile", "-Command", cmd], check=True)
    rows = [line.split("\u001f") for line in out.read_text(encoding="utf-8-sig").splitlines() if line.strip()]
    out.unlink()
    return rows


def main():
    for name, (header, sql) in QUERIES.items():
        rows = query(sql)
        with open(DATA / name, "w", newline="", encoding="utf-8") as f:
            w = csv.writer(f)
            w.writerow(header)
            w.writerows(rows)
        print(f"{name}: {len(rows)} rows")
    build()


if __name__ == "__main__":
    main()
