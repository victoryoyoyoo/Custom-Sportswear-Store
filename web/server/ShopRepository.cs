using System.Text.RegularExpressions;
using Microsoft.Data.SqlClient;

namespace SportsShop;

public class ShopRepository(string connectionString)
{
    static readonly string[] Statuses = ["已成立", "已出貨", "已完成", "已取消"];
    static readonly string[] Payments = ["信用卡", "貨到付款", "ATM 轉帳"];
    static readonly Regex PhonePattern = new(@"^09\d{2}-?\d{3}-?\d{3}$");

    async Task<SqlConnection> OpenAsync()
    {
        var conn = new SqlConnection(connectionString);
        await conn.OpenAsync();
        return conn;
    }

    static SqlCommand Command(SqlConnection conn, string sql, SqlTransaction? tx = null, params (string Name, object? Value)[] args)
    {
        var cmd = new SqlCommand(sql, conn, tx);
        foreach (var (name, value) in args) cmd.Parameters.AddWithValue(name, value ?? DBNull.Value);
        return cmd;
    }

    public async Task<object> LoadAllAsync()
    {
        await using var conn = await OpenAsync();

        var sizes = new Dictionary<string, List<string>>();
        await using (var r = await Command(conn, "SELECT 商品編號, 尺寸 FROM 商品尺寸 ORDER BY 商品編號, 排序").ExecuteReaderAsync())
            while (await r.ReadAsync())
            {
                var id = r.GetString(0);
                if (!sizes.ContainsKey(id)) sizes[id] = [];
                sizes[id].Add(r.GetString(1));
            }

        var products = new List<Product>();
        const string productSql = """
            SELECT 商品.商品編號, 商品.圖片代碼, 商品.商品名稱, 分類.分類名稱, 商品.定價, 商品.說明, 商品.上架
            FROM 商品 JOIN 分類 ON 分類.分類編號 = 商品.分類編號
            ORDER BY 商品.商品編號
            """;
        await using (var r = await Command(conn, productSql).ExecuteReaderAsync())
            while (await r.ReadAsync())
            {
                var id = r.GetString(0);
                products.Add(new Product(id, r.GetString(1), r.GetString(2), r.GetString(3), r.GetInt32(4),
                    sizes.GetValueOrDefault(id, []), r.IsDBNull(5) ? null : r.GetString(5), r.GetBoolean(6)));
            }

        var customers = new List<Customer>();
        await using (var r = await Command(conn, "SELECT 顧客編號, 姓名, 電子郵件, 手機, 縣市, 加入日期 FROM 顧客 ORDER BY 顧客編號").ExecuteReaderAsync())
            while (await r.ReadAsync())
                customers.Add(new Customer(r.GetString(0), r.GetString(1), r.GetString(2), r.GetString(3), r.GetString(4), r.GetDateTime(5).ToString("yyyy-MM-dd")));

        var items = new Dictionary<string, List<OrderItem>>();
        await using (var r = await Command(conn, "SELECT 訂單編號, 商品編號, 顏色代碼, 尺寸, 數量, 成交單價 FROM 訂單明細 ORDER BY 訂單編號, 項次").ExecuteReaderAsync())
            while (await r.ReadAsync())
            {
                var id = r.GetString(0);
                if (!items.ContainsKey(id)) items[id] = [];
                items[id].Add(new OrderItem(r.GetString(1), r.GetString(2), r.GetString(3), r.GetInt32(4), r.GetInt32(5)));
            }

        var orders = new List<Order>();
        const string orderSql = "SELECT 訂單編號, 顧客編號, 訂購日期, 狀態, 付款方式, 收件人, 收件電話, 收件縣市, 收件地址, 備註 FROM 訂單";
        await using (var r = await Command(conn, orderSql).ExecuteReaderAsync())
            while (await r.ReadAsync())
            {
                var id = r.GetString(0);
                string? Text(int i) => r.IsDBNull(i) ? null : r.GetString(i);
                orders.Add(new Order(id, r.GetString(1), r.GetDateTime(2).ToString("yyyy-MM-dd"), r.GetString(3), r.GetString(4), r.GetString(5),
                    Text(6), r.GetString(7), Text(8), Text(9), items.GetValueOrDefault(id, [])));
            }

        return new { products, customers, orders };
    }

    static string NextId(SqlConnection conn, SqlTransaction tx, string table, string column, string prefix, int width)
    {
        var sql = $"SELECT ISNULL(MAX(CAST(SUBSTRING({column}, 2, 10) AS INT)), 0) + 1 FROM {table} WITH (UPDLOCK, HOLDLOCK)";
        var next = (int)Command(conn, sql, tx).ExecuteScalar()!;
        return prefix + next.ToString().PadLeft(width, '0');
    }

    static void CheckProduct(ProductInput p)
    {
        if (string.IsNullOrWhiteSpace(p.Name)) throw new ShopException("請填商品名稱。");
        if (p.Price <= 0) throw new ShopException("價格要大於 0。");
        if (p.Sizes.Count == 0) throw new ShopException("至少要有一個尺寸。");
    }

    public async Task<Product> SaveProductAsync(ProductInput p)
    {
        CheckProduct(p);
        await using var conn = await OpenAsync();
        await using var tx = (SqlTransaction)await conn.BeginTransactionAsync();
        var categoryId = await Command(conn, "SELECT 分類編號 FROM 分類 WHERE 分類名稱 = @分類名稱", tx, ("@分類名稱", p.Category)).ExecuteScalarAsync()
            ?? throw new ShopException("沒有這個分類。");
        var id = p.Id;
        if (id is null)
        {
            id = NextId(conn, tx, "商品", "商品編號", "P", 2);
            const string insert = """
                INSERT INTO 商品 (商品編號, 商品名稱, 分類編號, 定價, 圖片代碼, 說明, 上架)
                VALUES (@商品編號, @商品名稱, @分類編號, @定價, @圖片代碼, @說明, @上架)
                """;
            await Command(conn, insert, tx, ("@商品編號", id), ("@商品名稱", p.Name), ("@分類編號", categoryId), ("@定價", p.Price),
                ("@圖片代碼", p.Key), ("@說明", p.Description), ("@上架", p.Active)).ExecuteNonQueryAsync();
        }
        else
        {
            const string update = """
                UPDATE 商品
                SET 商品名稱 = @商品名稱, 分類編號 = @分類編號, 定價 = @定價, 圖片代碼 = @圖片代碼, 說明 = @說明, 上架 = @上架
                WHERE 商品編號 = @商品編號
                """;
            var rows = await Command(conn, update, tx, ("@商品編號", id), ("@商品名稱", p.Name), ("@分類編號", categoryId), ("@定價", p.Price),
                ("@圖片代碼", p.Key), ("@說明", p.Description), ("@上架", p.Active)).ExecuteNonQueryAsync();
            if (rows == 0) throw new ShopException("找不到這個商品。");
            await Command(conn, "DELETE FROM 商品尺寸 WHERE 商品編號 = @商品編號", tx, ("@商品編號", id)).ExecuteNonQueryAsync();
        }
        for (var i = 0; i < p.Sizes.Count; i++)
            await Command(conn, "INSERT INTO 商品尺寸 (商品編號, 尺寸, 排序) VALUES (@商品編號, @尺寸, @排序)", tx,
                ("@商品編號", id), ("@尺寸", p.Sizes[i]), ("@排序", i + 1)).ExecuteNonQueryAsync();
        await tx.CommitAsync();
        return new Product(id, p.Key, p.Name, p.Category, p.Price, p.Sizes, p.Description, p.Active);
    }

    public async Task DeleteProductAsync(string id)
    {
        await using var conn = await OpenAsync();
        var used = (int)(await Command(conn, "SELECT COUNT(*) FROM 訂單明細 WHERE 商品編號 = @商品編號", null, ("@商品編號", id)).ExecuteScalarAsync())!;
        if (used > 0) throw new ShopException("這個商品已經有訂單，不能刪除，可以改成下架。");
        var rows = await Command(conn, "DELETE FROM 商品 WHERE 商品編號 = @商品編號", null, ("@商品編號", id)).ExecuteNonQueryAsync();
        if (rows == 0) throw new ShopException("找不到這個商品。");
    }

    public async Task<Customer> SaveCustomerAsync(CustomerInput c)
    {
        if (string.IsNullOrWhiteSpace(c.Name)) throw new ShopException("請填姓名。");
        if (!c.Email.Contains('@')) throw new ShopException("Email 格式不對。");
        if (!PhonePattern.IsMatch(c.Phone)) throw new ShopException("手機格式不對。");
        await using var conn = await OpenAsync();
        await using var tx = (SqlTransaction)await conn.BeginTransactionAsync();
        var taken = (int)(await Command(conn, "SELECT COUNT(*) FROM 顧客 WHERE 電子郵件 = @電子郵件 AND 顧客編號 <> ISNULL(@顧客編號, '')", tx,
            ("@電子郵件", c.Email), ("@顧客編號", c.Id)).ExecuteScalarAsync())!;
        if (taken > 0) throw new ShopException("這個 Email 已經有人用了。");
        var id = c.Id;
        if (id is null)
        {
            id = NextId(conn, tx, "顧客", "顧客編號", "C", 3);
            await Command(conn, "INSERT INTO 顧客 (顧客編號, 姓名, 電子郵件, 手機, 縣市) VALUES (@顧客編號, @姓名, @電子郵件, @手機, @縣市)", tx,
                ("@顧客編號", id), ("@姓名", c.Name), ("@電子郵件", c.Email), ("@手機", c.Phone), ("@縣市", c.City)).ExecuteNonQueryAsync();
        }
        else
        {
            var rows = await Command(conn, "UPDATE 顧客 SET 姓名 = @姓名, 電子郵件 = @電子郵件, 手機 = @手機, 縣市 = @縣市 WHERE 顧客編號 = @顧客編號", tx,
                ("@顧客編號", id), ("@姓名", c.Name), ("@電子郵件", c.Email), ("@手機", c.Phone), ("@縣市", c.City)).ExecuteNonQueryAsync();
            if (rows == 0) throw new ShopException("找不到這位顧客。");
        }
        var joined = ((DateTime)(await Command(conn, "SELECT 加入日期 FROM 顧客 WHERE 顧客編號 = @顧客編號", tx, ("@顧客編號", id)).ExecuteScalarAsync())!).ToString("yyyy-MM-dd");
        await tx.CommitAsync();
        return new Customer(id, c.Name, c.Email, c.Phone, c.City, joined);
    }

    public async Task DeleteCustomerAsync(string id)
    {
        await using var conn = await OpenAsync();
        var used = (int)(await Command(conn, "SELECT COUNT(*) FROM 訂單 WHERE 顧客編號 = @顧客編號", null, ("@顧客編號", id)).ExecuteScalarAsync())!;
        if (used > 0) throw new ShopException("這位顧客有訂單紀錄，不能刪除。");
        var rows = await Command(conn, "DELETE FROM 顧客 WHERE 顧客編號 = @顧客編號", null, ("@顧客編號", id)).ExecuteNonQueryAsync();
        if (rows == 0) throw new ShopException("找不到這位顧客。");
    }

    public async Task<Order> CreateOrderAsync(OrderInput o)
    {
        if (o.Items.Count == 0) throw new ShopException("購物車是空的。");
        if (!Payments.Contains(o.Payment)) throw new ShopException("付款方式不對。");
        if (o.Phone is not null && !PhonePattern.IsMatch(o.Phone)) throw new ShopException("手機格式不對。");
        await using var conn = await OpenAsync();
        await using var tx = (SqlTransaction)await conn.BeginTransactionAsync();
        var id = NextId(conn, tx, "訂單", "訂單編號", "O", 5);
        var today = DateTime.Today;
        const string insertOrder = """
            INSERT INTO 訂單 (訂單編號, 顧客編號, 訂購日期, 狀態, 付款方式, 收件人, 收件電話, 收件縣市, 收件地址, 備註)
            VALUES (@訂單編號, @顧客編號, @訂購日期, N'已成立', @付款方式, @收件人, @收件電話, @收件縣市, @收件地址, @備註)
            """;
        await Command(conn, insertOrder, tx, ("@訂單編號", id), ("@顧客編號", o.Customer), ("@訂購日期", today), ("@付款方式", o.Payment),
            ("@收件人", o.Recipient), ("@收件電話", o.Phone), ("@收件縣市", o.City), ("@收件地址", o.Address), ("@備註", o.Note)).ExecuteNonQueryAsync();
        var items = new List<OrderItem>();
        for (var i = 0; i < o.Items.Count; i++)
        {
            var it = o.Items[i];
            var price = await Command(conn, "SELECT 定價 FROM 商品 WHERE 商品編號 = @商品編號 AND 上架 = 1", tx, ("@商品編號", it.Product)).ExecuteScalarAsync()
                ?? throw new ShopException($"商品 {it.Product} 已下架。");
            const string insertItem = """
                INSERT INTO 訂單明細 (訂單編號, 項次, 商品編號, 顏色代碼, 尺寸, 數量, 成交單價)
                VALUES (@訂單編號, @項次, @商品編號, @顏色代碼, @尺寸, @數量, @成交單價)
                """;
            await Command(conn, insertItem, tx, ("@訂單編號", id), ("@項次", i + 1), ("@商品編號", it.Product), ("@顏色代碼", it.Color),
                ("@尺寸", it.Size), ("@數量", it.Qty), ("@成交單價", price)).ExecuteNonQueryAsync();
            items.Add(it with { Price = (int)price });
        }
        await tx.CommitAsync();
        return new Order(id, o.Customer, today.ToString("yyyy-MM-dd"), "已成立", o.Payment, o.Recipient, o.Phone, o.City, o.Address, o.Note, items);
    }

    public async Task UpdateOrderStatusAsync(string id, string status)
    {
        if (!Statuses.Contains(status)) throw new ShopException("沒有這個狀態。");
        await using var conn = await OpenAsync();
        var rows = await Command(conn, "UPDATE 訂單 SET 狀態 = @狀態 WHERE 訂單編號 = @訂單編號", null, ("@訂單編號", id), ("@狀態", status)).ExecuteNonQueryAsync();
        if (rows == 0) throw new ShopException("找不到這筆訂單。");
    }

    public async Task DeleteOrderAsync(string id)
    {
        await using var conn = await OpenAsync();
        var rows = await Command(conn, "DELETE FROM 訂單 WHERE 訂單編號 = @訂單編號", null, ("@訂單編號", id)).ExecuteNonQueryAsync();
        if (rows == 0) throw new ShopException("找不到這筆訂單。");
    }

    public async Task ResetAsync(string databaseFolder)
    {
        var master = new SqlConnectionStringBuilder(connectionString) { InitialCatalog = "master" }.ConnectionString;
        foreach (var file in new[] { "01_建立資料表.sql", "02_新增資料.sql", "03_建立檢視表.sql" })
        {
            var script = await File.ReadAllTextAsync(Path.Combine(databaseFolder, file));
            await using var conn = new SqlConnection(master);
            await conn.OpenAsync();
            foreach (var batch in Regex.Split(script, @"^\s*GO\s*$", RegexOptions.Multiline | RegexOptions.IgnoreCase))
                if (!string.IsNullOrWhiteSpace(batch))
                    await new SqlCommand(batch, conn) { CommandTimeout = 300 }.ExecuteNonQueryAsync();
        }
    }
}
