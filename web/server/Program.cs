using Microsoft.Data.SqlClient;
using Microsoft.Extensions.FileProviders;
using SportsShop;

var builder = WebApplication.CreateBuilder(args);
var siteRoot = Path.GetFullPath(Path.Combine(builder.Environment.ContentRootPath, ".."));
var repo = new ShopRepository(builder.Configuration.GetConnectionString("Shop")!);
var app = builder.Build();

var files = new PhysicalFileProvider(siteRoot);
app.UseDefaultFiles(new DefaultFilesOptions { FileProvider = files });
app.UseStaticFiles(new StaticFileOptions
{
    FileProvider = files,
    OnPrepareResponse = ctx => ctx.Context.Response.Headers.CacheControl = "no-cache",
});

async Task<IResult> Run(Func<Task<object?>> action)
{
    try
    {
        var result = await action();
        return result is null ? Results.NoContent() : Results.Ok(result);
    }
    catch (ShopException ex)
    {
        return Results.BadRequest(new { error = ex.Message });
    }
    catch (SqlException ex)
    {
        app.Logger.LogError(ex, "SQL error");
        return Results.Problem(statusCode: 500, title: "資料庫發生錯誤", extensions: new Dictionary<string, object?> { ["error"] = "資料庫發生錯誤，請稍後再試。" });
    }
}

var api = app.MapGroup("/api");
api.MapGet("/health", () => Results.Ok(new { ok = true }));
api.MapGet("/all", () => Run(async () => await repo.LoadAllAsync()));

api.MapPost("/products", (ProductInput p) => Run(async () => await repo.SaveProductAsync(p with { Id = null })));
api.MapPut("/products/{id}", (string id, ProductInput p) => Run(async () => await repo.SaveProductAsync(p with { Id = id })));
api.MapDelete("/products/{id}", (string id) => Run(async () => { await repo.DeleteProductAsync(id); return null; }));

api.MapPost("/customers", (CustomerInput c) => Run(async () => await repo.SaveCustomerAsync(c with { Id = null })));
api.MapPut("/customers/{id}", (string id, CustomerInput c) => Run(async () => await repo.SaveCustomerAsync(c with { Id = id })));
api.MapDelete("/customers/{id}", (string id) => Run(async () => { await repo.DeleteCustomerAsync(id); return null; }));

api.MapPost("/orders", (OrderInput o) => Run(async () => await repo.CreateOrderAsync(o)));
api.MapMethods("/orders/{id}", ["PATCH"], (string id, StatusInput s) => Run(async () => { await repo.UpdateOrderStatusAsync(id, s.Status); return null; }));
api.MapDelete("/orders/{id}", (string id) => Run(async () => { await repo.DeleteOrderAsync(id); return null; }));

api.MapGet("/views/{name}", (string name) => Run(async () => await repo.LoadViewAsync(name)));

api.MapPost("/reset", () => Run(async () => { await repo.ResetAsync(Path.Combine(siteRoot, "database")); return null; }));

app.Run();
