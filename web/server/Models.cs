namespace SportsShop;

public record Product(string Id, string Key, string Name, string Category, int Price, List<string> Sizes, string? Description, bool Active);

public record Customer(string Id, string Name, string Email, string Phone, string City, string Joined);

public record OrderItem(string Product, string Color, string Size, int Qty, int Price);

public record Order(
    string Id, string Customer, string Date, string Status, string Payment, string Recipient,
    string? Phone, string City, string? Address, string? Note, List<OrderItem> Items);

public record ProductInput(string? Id, string Key, string Name, string Category, int Price, List<string> Sizes, string? Description, bool Active);

public record CustomerInput(string? Id, string Name, string Email, string Phone, string City);

public record OrderInput(string Customer, string Payment, string Recipient, string? Phone, string City, string? Address, string? Note, List<OrderItem> Items);

public record StatusInput(string Status);

public class ShopException(string message) : Exception(message);
