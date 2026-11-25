const Store = (() => {
  const KEY = "shop_changes_v2";
  let mode = "local";

  const emptyChanges = () => ({ products: {}, customers: {}, orders: {}, deleted: { products: [], customers: [], orders: [] } });

  function loadChanges() {
    try {
      return { ...emptyChanges(), ...(JSON.parse(localStorage.getItem(KEY)) || {}) };
    } catch {
      return emptyChanges();
    }
  }

  function saveChanges(c) {
    try {
      localStorage.setItem(KEY, JSON.stringify(c));
    } catch {
      throw new Error("瀏覽器無法儲存資料");
    }
  }

  const baseOrders = () => {
    const items = new Map();
    ORDER_ITEMS.forEach(([order, product, color, size, qty, price]) => {
      if (!items.has(order)) items.set(order, []);
      items.get(order).push({ product, color, size, qty, price });
    });
    return ORDERS.map(([id, customer, date, status, payment, recipient, phone, city, address, note]) => ({
      id, customer, date, status, payment, recipient, phone, city, address, note, items: items.get(id) || [],
    }));
  };

  function merge(base, table, changes) {
    const gone = new Set(changes.deleted[table]);
    const map = new Map(base.filter((r) => !gone.has(r.id)).map((r) => [r.id, r]));
    Object.values(changes[table]).forEach((r) => {
      if (!gone.has(r.id)) map.set(r.id, { ...(map.get(r.id) || {}), ...r });
    });
    return [...map.values()];
  }

  async function api(path, options = {}) {
    const res = await fetch(`/api/${path}`, { headers: { "Content-Type": "application/json" }, ...options });
    const body = res.status === 204 ? null : await res.json();
    if (!res.ok) throw new Error(body?.error || `伺服器錯誤 ${res.status}`);
    return body;
  }

  async function init() {
    if (location.protocol.startsWith("http")) {
      try {
        const res = await fetch("/api/health", { cache: "no-store" });
        if (res.ok) mode = "server";
      } catch {
        mode = "local";
      }
    }
    return mode;
  }

  async function all() {
    if (mode === "server") return api("all");
    const c = loadChanges();
    return {
      products: merge(PRODUCTS.map((p) => ({ active: true, ...p })), "products", c),
      customers: merge(CUSTOMERS, "customers", c),
      orders: merge(baseOrders(), "orders", c),
    };
  }

  const nextId = (rows, prefix, width) => {
    const max = rows.reduce((m, r) => Math.max(m, Number(String(r.id).replace(/\D/g, "")) || 0), 0);
    return `${prefix}${String(max + 1).padStart(width, "0")}`;
  };

  async function saveProduct(p) {
    if (mode === "server") return api(p.id ? `products/${p.id}` : "products", { method: p.id ? "PUT" : "POST", body: JSON.stringify(p) });
    const c = loadChanges();
    const data = await all();
    const row = { ...p, id: p.id || nextId(data.products, "P", 2) };
    c.products[row.id] = row;
    saveChanges(c);
    return row;
  }

  async function deleteProduct(id) {
    if (mode === "server") return api(`products/${id}`, { method: "DELETE" });
    const data = await all();
    if (data.orders.some((o) => o.items.some((i) => i.product === id))) throw new Error("這個商品已經有訂單，不能刪除，可以改成下架。");
    const c = loadChanges();
    c.deleted.products.push(id);
    saveChanges(c);
  }

  async function saveCustomer(cu) {
    if (mode === "server") return api(cu.id ? `customers/${cu.id}` : "customers", { method: cu.id ? "PUT" : "POST", body: JSON.stringify(cu) });
    const c = loadChanges();
    const data = await all();
    if (data.customers.some((x) => x.email === cu.email && x.id !== cu.id)) throw new Error("這個 Email 已經有人用了。");
    const row = { ...cu, id: cu.id || nextId(data.customers, "C", 3), joined: cu.joined || DATA_END };
    c.customers[row.id] = row;
    saveChanges(c);
    return row;
  }

  async function deleteCustomer(id) {
    if (mode === "server") return api(`customers/${id}`, { method: "DELETE" });
    const data = await all();
    if (data.orders.some((o) => o.customer === id)) throw new Error("這位顧客有訂單紀錄，不能刪除。");
    const c = loadChanges();
    c.deleted.customers.push(id);
    saveChanges(c);
  }

  async function createOrder(order) {
    if (mode === "server") return api("orders", { method: "POST", body: JSON.stringify(order) });
    const c = loadChanges();
    const data = await all();
    const row = { ...order, id: nextId(data.orders, "O", 5), date: DATA_END, status: "已成立", isNew: true };
    c.orders[row.id] = row;
    saveChanges(c);
    return row;
  }

  async function updateOrder(id, patch) {
    if (mode === "server") return api(`orders/${id}`, { method: "PATCH", body: JSON.stringify(patch) });
    const c = loadChanges();
    c.orders[id] = { ...(c.orders[id] || {}), id, ...patch };
    saveChanges(c);
  }

  async function deleteOrder(id) {
    if (mode === "server") return api(`orders/${id}`, { method: "DELETE" });
    const c = loadChanges();
    if (c.orders[id]?.isNew) delete c.orders[id];
    else c.deleted.orders.push(id);
    saveChanges(c);
  }

  async function reset() {
    if (mode === "server") return api("reset", { method: "POST" });
    saveChanges(emptyChanges());
  }

  return { init, all, saveProduct, deleteProduct, saveCustomer, deleteCustomer, createOrder, updateOrder, deleteOrder, reset, get mode() { return mode; } };
})();
