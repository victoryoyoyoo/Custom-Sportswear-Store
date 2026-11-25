const DAY = 86400000;
let TODAY = DATA_END;
const CHURN_WINDOW = 90;
const STATUSES = ["已成立", "已出貨", "已完成", "已取消"];
const CITIES = ["台北市", "新北市", "桃園市", "台中市", "台南市", "高雄市", "新竹市", "基隆市", "嘉義市", "宜蘭縣"];
const IMAGE_KEYS = ["jersey", "tee", "hoodie", "shorts", "socks", "sneaker", "backpack", "bottle", "towel", "headband", "wristband"];
const DEFAULT_COLOR = { jersey: "royal", tee: "steel", hoodie: "jade", shorts: "navy", socks: "onyx", sneaker: "crimson", backpack: "navy", bottle: "sunset", towel: "teal", headband: "violet", wristband: "crimson" };

const local = {
  get(key, fallback) {
    try {
      return JSON.parse(localStorage.getItem(key)) ?? fallback;
    } catch {
      return fallback;
    }
  },
  set(key, value) {
    try {
      localStorage.setItem(key, JSON.stringify(value));
    } catch {
      return;
    }
  },
};

const $ = (id) => document.getElementById(id);
const money = (n) => `NT$${Math.round(n).toLocaleString("en-US")}`;
const pct = (v) => `${Math.round(v * 100)}%`;
const days = (a, b) => Math.round((new Date(b) - new Date(a)) / DAY);
const shiftDate = (d, n) => new Date(new Date(d).getTime() + n * DAY).toISOString().slice(0, 10);
const esc = (t) => String(t ?? "").replace(/[&<>"']/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" })[c]);
const colorByKey = Object.fromEntries(COLORS.map((c) => [c.key, c]));
const img = (key, color) => `img/p/${key}_${color || DEFAULT_COLOR[key]}.webp`;
const orderTotal = (o) => o.items.reduce((s, i) => s + i.qty * i.price, 0);
const shippingOf = (sub) => (sub === 0 || sub >= 2000 ? 0 : 80);

let db = { products: [], customers: [], orders: [] };
let state = {};
const drawn = {};

const productById = (id) => db.products.find((p) => p.id === id);
const customerById = (id) => db.customers.find((c) => c.id === id);

async function reload() {
  db = await Store.all();
  db.orders.sort((a, b) => (a.date < b.date ? 1 : a.date > b.date ? -1 : b.id.localeCompare(a.id)));
  recompute();
}

function rfm(orders, refDate, customers) {
  const by = new Map(customers.map((c) => [c, []]));
  orders.forEach((o) => {
    if (o.status !== "已取消" && o.date <= refDate && by.has(o.customer)) by.get(o.customer).push(o);
  });
  return customers.map((c) => {
    const list = by.get(c);
    if (!list.length) return null;
    const dates = list.map((o) => o.date).sort();
    const monetary = list.reduce((s, o) => s + orderTotal(o), 0);
    return { id: c, recency: days(dates[dates.length - 1], refDate), frequency: list.length, monetary, average: monetary / list.length, tenure: days(dates[0], refDate) };
  }).filter(Boolean);
}

function recompute() {
  const ids = db.customers.map((c) => c.id);
  const valid = db.orders.filter((o) => o.status !== "已取消");
  const profiles = rfm(valid, TODAY, ids);
  const cIndex = Object.fromEntries(ids.map((c, i) => [c, i]));
  const pIndex = Object.fromEntries(db.products.map((p, i) => [p.id, i]));
  const matrix = ids.map(() => Array(db.products.length).fill(0));
  valid.forEach((o) => o.items.forEach((i) => {
    if (cIndex[o.customer] !== undefined && pIndex[i.product] !== undefined) matrix[cIndex[o.customer]][pIndex[i.product]] = 1;
  }));
  const columns = db.products.map((_, j) => matrix.map((row) => row[j]));
  const similarity = columns.map((a, i) => columns.map((b, j) => (i === j ? 1 : ML.cosine(a, b))));
  state = { profiles, similarity };
  Object.keys(drawn).forEach((k) => (drawn[k] = false));
}

document.querySelectorAll(".nav-item").forEach((b) => b.addEventListener("click", () => showTab(b.dataset.tab)));

function showTab(name) {
  const side = document.querySelector(`.nav-item[data-tab="${name}"]`)?.dataset.side || "front";
  document.body.dataset.side = side;
  $("side-switch").textContent = side === "front" ? "進入後台" : "回到商店";
  $("who-label").textContent = side === "front" ? "示範登入" : "查看顧客";
  $("who").classList.toggle("off", side === "back" && !["segments", "churn"].includes(name));
  document.querySelectorAll(".nav-item").forEach((n) => n.classList.toggle("active", n.dataset.tab === name));
  document.querySelectorAll(".panel").forEach((p) => p.classList.toggle("active", p.id === name));
  window.scrollTo(0, 0);
  renderTab(name);
}

function renderTab(name) {
  ({ shop: renderShop, cart: renderCart, mine: renderMine, orders: renderOrders, admin: renderAdmin }[name] || (() => {}))();
  if (name === "segments" && !drawn.segments) renderSegments();
  if (name === "recommend" && !drawn.recommend) renderRecommend();
  if (name === "churn" && !drawn.churn) renderChurn();
}

const currentTab = () => document.querySelector(".nav-item.active").dataset.tab;

window.matchMedia("(prefers-color-scheme: dark)").addEventListener("change", () => {
  Object.keys(drawn).forEach((k) => (drawn[k] = false));
  renderTab(currentTab());
});

const customerSelect = $("customer");
function fillCustomerSelect() {
  const keep = customerSelect.value || local.get("shop_customer", "C001");
  customerSelect.innerHTML = db.customers.map((c) => `<option value="${c.id}">${c.id}</option>`).join("");
  customerSelect.value = db.customers.some((c) => c.id === keep) ? keep : db.customers[0]?.id;
}
customerSelect.addEventListener("change", () => {
  local.set("shop_customer", customerSelect.value);
  Object.keys(drawn).forEach((k) => (drawn[k] = false));
  renderTab(currentTab());
});
const me = () => customerSelect.value;

function openDialog(html, cls = "") {
  const d = $("dialog");
  d.className = cls;
  $("dialog-body").onclick = null;
  $("dialog-body").innerHTML = html;
  if (!d.open) d.showModal();
  return d;
}
const closeDialog = () => $("dialog").close();
$("dialog-close").addEventListener("click", closeDialog);
$("dialog").addEventListener("click", (e) => {
  if (e.target.id === "dialog") closeDialog();
});

function confirmBox(text, okLabel = "確定") {
  return new Promise((resolve) => {
    openDialog(`<div class="confirm"><p>${esc(text)}</p><div class="actions end"><button class="ghost" data-c="0">取消</button><button class="btn danger" data-c="1">${okLabel}</button></div></div>`, "small");
    const d = $("dialog");
    const onClose = () => resolve(false);
    d.addEventListener("close", onClose, { once: true });
    $("dialog-body").onclick = (e) => {
      const b = e.target.closest("[data-c]");
      if (!b) return;
      d.removeEventListener("close", onClose);
      $("dialog-body").onclick = null;
      closeDialog();
      resolve(b.dataset.c === "1");
    };
  });
}

let toastTimer;
function toast(text, kind = "", action) {
  const t = $("toast");
  t.textContent = text;
  if (action) {
    const b = document.createElement("button");
    b.textContent = action.label;
    b.onclick = () => {
      t.className = "toast";
      action.run();
    };
    t.append(b);
  }
  t.className = `toast show ${kind} ${action ? "has-action" : ""}`;
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => (t.className = "toast"), action ? 4000 : 2400);
}

let filter = "全部";
$("search").addEventListener("input", renderShop);
$("sort").addEventListener("change", renderShop);
$("category-filter").addEventListener("click", (e) => {
  if (!e.target.dataset.cat) return;
  filter = e.target.dataset.cat;
  renderShop();
});

function soldCount() {
  const n = {};
  db.orders.forEach((o) => o.status !== "已取消" && o.items.forEach((i) => (n[i.product] = (n[i.product] || 0) + i.qty)));
  return n;
}

function renderShop() {
  const cats = ["全部", ...new Set(db.products.filter((p) => p.active).map((p) => p.category))];
  if (!cats.includes(filter)) filter = "全部";
  $("category-filter").innerHTML = cats.map((c) => `<button class="seg ${c === filter ? "active" : ""}" data-cat="${esc(c)}">${esc(c)}</button>`).join("");
  const q = $("search").value.trim();
  const sold = soldCount();
  let list = db.products.filter((p) => p.active && (filter === "全部" || p.category === filter) && (!q || p.name.includes(q)));
  const sort = $("sort").value;
  if (sort === "price-asc") list = [...list].sort((a, b) => a.price - b.price);
  if (sort === "price-desc") list = [...list].sort((a, b) => b.price - a.price);
  if (sort === "popular") list = [...list].sort((a, b) => (sold[b.id] || 0) - (sold[a.id] || 0));
  $("product-grid").innerHTML = list.length
    ? list.map((p) => `
      <article class="product">
        <button class="product-img" data-open="${p.id}" aria-label="查看${esc(p.name)}"><img src="${img(p.key)}" alt="${esc(p.name)}" loading="lazy"></button>
        <div class="product-body">
          <span class="product-cat">${esc(p.category)}・已售 ${(sold[p.id] || 0).toLocaleString("en-US")}</span>
          <h3><button class="link" data-open="${p.id}">${esc(p.name)}</button></h3>
          <div class="product-foot"><strong>${money(p.price)}</strong><button class="btn-sm" data-open="${p.id}">選購</button></div>
        </div>
      </article>`).join("")
    : `<p class="empty">找不到符合的商品。</p>`;
  renderForYou();
}

function scoreFor(owned) {
  return db.products.map((p, j) => ({
    p,
    score: owned.has(p.id) || !p.active ? -1 : db.products.reduce((s, q, i) => s + (owned.has(q.id) ? state.similarity[i][j] : 0), 0),
  })).filter((s) => s.score > 0).sort((a, b) => b.score - a.score);
}

function renderForYou() {
  const owned = new Set(db.orders.filter((o) => o.customer === me()).flatMap((o) => o.items.map((i) => i.product)));
  let list = scoreFor(owned).slice(0, 4);
  if (list.length) {
    $("for-you-title").textContent = "你可能會喜歡";
    $("for-you-note").textContent = "依這位顧客買過的商品，找出其他顧客常一起買的。";
  } else {
    const sold = soldCount();
    list = db.products.filter((p) => p.active).sort((a, b) => (sold[b.id] || 0) - (sold[a.id] || 0)).slice(0, 4).map((p) => ({ p }));
    $("for-you-title").textContent = "熱銷商品";
    $("for-you-note").textContent = `${name} 每一樣商品都買過了，這裡改列賣得最好的。`;
  }
  $("for-you").innerHTML = list.map((s) => `<button class="mini" data-open="${s.p.id}"><img src="${img(s.p.key)}" alt=""><span>${esc(s.p.name)}</span><small>${money(s.p.price)}</small></button>`).join("");
}

function similarTo(pid, n = 3) {
  const i = db.products.findIndex((p) => p.id === pid);
  if (i < 0) return [];
  return db.products.map((p, j) => ({ p, score: state.similarity[i][j] })).filter((x) => x.p.id !== pid && x.p.active).sort((a, b) => b.score - a.score).slice(0, n);
}

const picker = { pid: null, color: null, size: null, qty: 1 };

function openProduct(pid) {
  const p = productById(pid);
  if (!p) return;
  Object.assign(picker, { pid, color: DEFAULT_COLOR[p.key] || COLORS[0].key, size: p.sizes.length === 1 ? p.sizes[0] : null, qty: 1 });
  openDialog(`
    <div class="detail">
      <div class="detail-img"><img id="detail-img" src="${img(p.key, picker.color)}" alt="${esc(p.name)}"></div>
      <div class="detail-also">
        <h3>買了這個的人也買</h3>
        <div class="mini-row">${similarTo(pid).map((s) => `<button class="mini" data-open="${s.p.id}"><img src="${img(s.p.key)}" alt=""><span>${esc(s.p.name)}</span><small>相似度 ${s.score.toFixed(2)}</small></button>`).join("")}</div>
      </div>
      <div class="detail-info">
        <span class="product-cat">${esc(p.category)}・${p.id}</span>
        <h2>${esc(p.name)}</h2>
        <p class="detail-price">${money(p.price)}</p>
        <p class="detail-desc">${esc(p.description || "")}</p>
        <div class="opt"><span>顏色：<b id="color-name">${colorByKey[picker.color].name}</b></span>
          <div class="swatches">${COLORS.map((c) => `<button class="swatch-btn ${c.key === picker.color ? "on" : ""}" data-color="${c.key}" style="--c:${c.hex}" aria-label="${c.name}" title="${c.name}"></button>`).join("")}</div></div>
        <div class="opt"><span>尺寸</span>
          <div class="sizes">${p.sizes.map((s) => `<button class="size-btn ${s === picker.size ? "on" : ""}" data-size="${esc(s)}">${esc(s)}</button>`).join("")}</div></div>
        <div class="opt row"><span>數量</span><div class="qty"><button data-q="-1" aria-label="減少">−</button><span id="pick-qty">1</span><button data-q="1" aria-label="增加">+</button></div></div>
        <p class="form-msg" id="pick-msg"></p>
        <button class="btn wide" id="pick-add">加入購物車</button>
      </div>
    </div>`, "wide");
  $("dialog-body").onclick = (e) => {
    const c = e.target.closest("[data-color]");
    const s = e.target.closest("[data-size]");
    const q = e.target.closest("[data-q]");
    if (c) {
      picker.color = c.dataset.color;
      $("detail-img").src = img(p.key, picker.color);
      $("color-name").textContent = colorByKey[picker.color].name;
      document.querySelectorAll(".swatch-btn").forEach((b) => b.classList.toggle("on", b === c));
    }
    if (s) {
      picker.size = s.dataset.size;
      document.querySelectorAll(".size-btn").forEach((b) => b.classList.toggle("on", b === s));
      $("pick-msg").textContent = "";
    }
    if (q) {
      picker.qty = Math.min(20, Math.max(1, picker.qty + Number(q.dataset.q)));
      $("pick-qty").textContent = picker.qty;
    }
    if (e.target.id === "pick-add") {
      if (!picker.size) {
        $("pick-msg").textContent = "請先選尺寸。";
        return;
      }
      addToCart(picker.pid, picker.color, picker.size, picker.qty);
      closeDialog();
    }
  };
}

document.addEventListener("click", (e) => {
  const open = e.target.closest("[data-open]");
  if (open) openProduct(open.dataset.open);
});

const cartKey = (pid, color, size) => `${pid}|${color}|${size}`;
const getCart = () => local.get("shop_cart_v2", {});
function setCart(cart) {
  local.set("shop_cart_v2", cart);
  const n = Object.values(cart).reduce((s, v) => s + v, 0);
  const badge = $("cart-count");
  if (String(n || "") !== badge.textContent && n) {
    badge.classList.remove("bump");
    void badge.offsetWidth;
    badge.classList.add("bump");
  }
  badge.textContent = n || "";
}

function addToCart(pid, color, size, qty = 1) {
  const cart = getCart();
  const k = cartKey(pid, color, size);
  cart[k] = Math.min(20, (cart[k] || 0) + qty);
  setCart(cart);
  toast(`已加入購物車：${productById(pid).name}（${colorByKey[color].name}・${size}）`, "", currentTab() === "cart" ? null : { label: "查看購物車", run: () => showTab("cart") });
  if (currentTab() === "cart") renderCart();
}

function cartLines() {
  return Object.entries(getCart()).map(([k, qty]) => {
    const [pid, color, size] = k.split("|");
    return { k, p: productById(pid), color, size, qty };
  }).filter((l) => l.p && l.p.active && l.qty > 0);
}

function renderCart() {
  const lines = cartLines();
  const sub = lines.reduce((s, l) => s + l.p.price * l.qty, 0);
  const ship = shippingOf(sub);
  $("cart-customer").textContent = me();
  $("cart-body").innerHTML = lines.length
    ? lines.map((l) => `<tr>
        <td><div class="cart-item"><img src="${img(l.p.key, l.color)}" alt=""><div><span>${esc(l.p.name)}</span><small>${colorByKey[l.color]?.name || l.color}・${esc(l.size)}</small></div></div></td>
        <td class="num">${money(l.p.price)}</td>
        <td><div class="qty"><button data-k="${l.k}" data-d="-1" aria-label="減少">−</button><span>${l.qty}</span><button data-k="${l.k}" data-d="1" aria-label="增加">+</button></div></td>
        <td class="num">${money(l.p.price * l.qty)}</td>
        <td class="num"><button class="icon-btn" data-remove="${l.k}">移除</button></td>
      </tr>`).join("")
    : `<tr><td colspan="5" class="empty">購物車是空的，到「商品」挑幾樣吧。</td></tr>`;
  $("cart-clear").hidden = !lines.length;
  $("cart-subtotal").textContent = money(sub);
  $("cart-shipping").textContent = sub >= 2000 ? "免運" : money(ship);
  $("cart-total").textContent = money(sub + ship);
  $("free-hint").textContent = sub > 0 && sub < 2000 ? `再買 ${money(2000 - sub)} 就免運` : sub >= 2000 ? "已達免運門檻" : "滿 NT$2,000 免運";
  $("checkout").disabled = !lines.length;
  const recs = scoreFor(new Set(lines.map((l) => l.p.id))).slice(0, 3);
  $("cart-recs").innerHTML = lines.length && recs.length
    ? recs.map((r) => `<div class="rec"><img src="${img(r.p.key)}" alt=""><div><span>${esc(r.p.name)}</span><small>${money(r.p.price)}</small></div><button class="ghost" data-open="${r.p.id}">看看</button></div>`).join("")
    : `<p class="empty">放入商品後，會依其他顧客的購買紀錄推薦加購。</p>`;
}

$("cart-body").addEventListener("click", (e) => {
  const b = e.target.closest("[data-k]");
  const r = e.target.closest("[data-remove]");
  if (!b && !r) return;
  const cart = getCart();
  if (b) cart[b.dataset.k] = Math.min(20, Math.max(0, (cart[b.dataset.k] || 0) + Number(b.dataset.d)));
  if (r) cart[r.dataset.remove] = 0;
  Object.keys(cart).forEach((k) => !cart[k] && delete cart[k]);
  setCart(cart);
  renderCart();
});

$("cart-continue").addEventListener("click", () => showTab("shop"));
$("cart-clear").addEventListener("click", async () => {
  if (await confirmBox("確定要清空購物車嗎？", "清空")) {
    setCart({});
    renderCart();
  }
});

$("checkout").addEventListener("click", () => {
  const c = customerById(me());
  const last = db.orders.filter((o) => o.customer === me() && o.address).at(-1);
  const lines = cartLines();
  const sub = lines.reduce((s, l) => s + l.p.price * l.qty, 0);
  openDialog(`
    <form class="form" id="checkout-form" novalidate>
      <h2>填寫收件資料</h2>
      <div class="grid-form">
        <label>手機<input name="phone" value="${esc(c?.phone)}" placeholder="0912-345-678" required></label>
        <label>縣市<select name="city">${CITIES.map((x) => `<option ${x === c?.city ? "selected" : ""}>${x}</option>`).join("")}</select></label>
        <label>地址<input name="address" value="${esc(last?.address || "")}" placeholder="路名、門牌號碼" required maxlength="60"></label>
      </div>
      <fieldset class="pay"><legend>付款方式</legend>
        ${["信用卡", "貨到付款", "ATM 轉帳"].map((x, i) => `<label><input type="radio" name="payment" value="${x}" ${i === 0 ? "checked" : ""}>${x}</label>`).join("")}
      </fieldset>
      <label class="full">備註<input name="note" maxlength="80" placeholder="選填"></label>
      <div class="sum-line"><span>共 ${lines.reduce((s, l) => s + l.qty, 0)} 件</span><strong>應付 ${money(sub + shippingOf(sub))}</strong></div>
      <p class="form-msg" id="form-msg"></p>
      <div class="actions end"><button type="button" class="ghost" id="form-cancel">返回</button><button class="btn" type="submit">確認下單</button></div>
    </form>`);
  $("form-cancel").onclick = closeDialog;
  $("checkout-form").onsubmit = submitOrder;
});

const PHONE = /^09\d{2}-?\d{3}-?\d{3}$/;

async function submitOrder(e) {
  e.preventDefault();
  const f = e.target;
  const data = Object.fromEntries(new FormData(f));
  const bad = { phone: !PHONE.test(data.phone.trim()), address: data.address.trim().length < 5 };
  Object.entries(bad).forEach(([k, v]) => f[k].classList.toggle("bad", v));
  const errs = [bad.phone && "手機格式（例如 0912-345-678）", bad.address && "地址"].filter(Boolean);
  if (errs.length) {
    $("form-msg").textContent = `請確認：${errs.join("、")}`;
    return;
  }
  const lines = cartLines();
  try {
    const order = await Store.createOrder({
      customer: me(), payment: data.payment, phone: data.phone.trim(), city: data.city,
      address: data.address.trim(), note: data.note.trim(),
      items: lines.map((l) => ({ product: l.p.id, color: l.color, size: l.size, qty: l.qty, price: l.p.price })),
    });
    setCart({});
    await reload();
    const sub = orderTotal(order);
    openDialog(`
      <div class="done">
        <h2>訂單已成立</h2>
        <p>訂單編號 <b>${order.id}</b>，應付 ${money(sub + shippingOf(sub))}（${esc(order.payment)}）。</p>
        <p class="muted">出貨後狀態會更新，可以在「我的訂單」查看。</p>
        <div class="actions end"><button class="ghost" id="done-orders">查看訂單</button><button class="btn" id="done-shop">繼續購物</button></div>
      </div>`, "small");
    $("done-orders").onclick = () => { closeDialog(); showTab("mine"); openOrder(order.id); };
    $("done-shop").onclick = () => { closeDialog(); showTab("shop"); };
    renderCart();
  } catch (err) {
    $("form-msg").textContent = err.message;
  }
}

let orderPage = 0;
["o-search", "o-status"].forEach((id) => $(id).addEventListener("input", () => { orderPage = 0; renderOrderTable(); }));

function renderOrders() {
  const valid = db.orders.filter((o) => o.status !== "已取消");
  const revenue = valid.reduce((s, o) => s + orderTotal(o), 0);
  $("o-revenue").textContent = money(revenue);
  $("o-count").textContent = valid.length.toLocaleString("en-US");
  $("o-avg").textContent = money(revenue / (valid.length || 1));
  $("o-open").textContent = db.orders.filter((o) => o.status === "已成立").length;
  const months = {};
  valid.forEach((o) => (months[o.date.slice(0, 7)] = (months[o.date.slice(0, 7)] || 0) + orderTotal(o)));
  const keys = Object.keys(months).sort();
  const lastDay = new Date(Number(TODAY.slice(0, 4)), Number(TODAY.slice(5, 7)), 0).getDate();
  const label = (k) => (k === TODAY.slice(0, 7) && Number(TODAY.slice(8)) < lastDay ? `${k.replace("-", "/")}（到 ${Number(TODAY.slice(8))} 日）` : k.replace("-", "/"));
  Charts.line($("o-chart"), keys.map(label), [{ label: "營收", values: keys.map((k) => months[k]), color: "a" }], { yMoney: true });
  renderOrderTable();
}

function filteredOrders() {
  const q = $("o-search").value.trim();
  const st = $("o-status").value;
  return db.orders.filter((o) => (!st || o.status === st) && (!q || o.id.includes(q.toUpperCase()) || o.customer.includes(q.toUpperCase())));
}

function renderOrderTable() {
  const list = filteredOrders();
  const per = 12;
  const pages = Math.max(1, Math.ceil(list.length / per));
  orderPage = Math.min(orderPage, pages - 1);
  $("o-body").innerHTML = list.length
    ? list.slice(orderPage * per, orderPage * per + per).map((o) => `
      <tr class="clickable" data-order="${o.id}" tabindex="0">
        <td>${o.id}</td><td>${o.date}</td><td>${o.customer}</td>
        <td class="ellipsis">${o.items.map((i) => `${esc(productById(i.product)?.name || i.product)} ×${i.qty}`).join("、")}</td>
        <td class="num">${money(orderTotal(o))}</td><td><span class="status s${STATUSES.indexOf(o.status)}">${o.status}</span></td>
      </tr>`).join("")
    : `<tr><td colspan="6" class="empty">沒有符合的訂單。</td></tr>`;
  $("o-page").textContent = `${orderPage + 1} / ${pages}（共 ${list.length.toLocaleString("en-US")} 筆）`;
  $("o-prev").disabled = orderPage === 0;
  $("o-next").disabled = orderPage >= pages - 1;
}
$("o-prev").addEventListener("click", () => { orderPage -= 1; renderOrderTable(); });
$("o-next").addEventListener("click", () => { orderPage += 1; renderOrderTable(); });
$("o-body").addEventListener("click", (e) => {
  const r = e.target.closest("[data-order]");
  if (r) openOrder(r.dataset.order);
});
$("o-body").addEventListener("keydown", (e) => {
  const r = e.target.closest("[data-order]");
  if (r && e.key === "Enter") openOrder(r.dataset.order);
});

$("side-switch").addEventListener("click", () => showTab(document.body.dataset.side === "front" ? "orders" : "shop"));

function renderMine() {
  $("mine-title").textContent = `${me()} 的訂單`;
  const list = db.orders.filter((o) => o.customer === me());
  $("mine-body").innerHTML = list.length
    ? list.map((o) => `
      <tr class="clickable" data-order="${o.id}" tabindex="0">
        <td>${o.id}</td><td>${o.date}</td>
        <td class="ellipsis">${o.items.map((i) => `${esc(productById(i.product)?.name || i.product)} ×${i.qty}`).join("、")}</td>
        <td class="num">${money(orderTotal(o) + shippingOf(orderTotal(o)))}</td><td><span class="status s${STATUSES.indexOf(o.status)}">${o.status}</span></td>
      </tr>`).join("")
    : `<tr><td colspan="5" class="empty">還沒有訂單，去逛逛吧。</td></tr>`;
}
$("mine-body").addEventListener("click", (e) => {
  const r = e.target.closest("[data-order]");
  if (r) openOrder(r.dataset.order);
});
$("mine-body").addEventListener("keydown", (e) => {
  const r = e.target.closest("[data-order]");
  if (r && e.key === "Enter") openOrder(r.dataset.order);
});

function openOrder(id) {
  const o = db.orders.find((x) => x.id === id);
  if (!o) return;
  const sub = orderTotal(o);
  openDialog(`
    <div class="order-detail">
      <h2>訂單 ${o.id}</h2>
      <dl class="kv">
        <dt>日期</dt><dd>${o.date}</dd><dt>顧客</dt><dd>${o.customer}</dd>
        <dt>收件電話</dt><dd>${esc(o.phone || "")}</dd><dt>地址</dt><dd>${esc(o.city)}${esc(o.address || "")}</dd>
        <dt>付款</dt><dd>${esc(o.payment)}</dd>${o.note ? `<dt>備註</dt><dd>${esc(o.note)}</dd>` : ""}
      </dl>
      <table class="table"><thead><tr><th>商品</th><th>顏色／尺寸</th><th class="num">單價</th><th class="num">數量</th><th class="num">小計</th></tr></thead><tbody>
        ${o.items.map((i) => `<tr><td>${esc(productById(i.product)?.name || i.product)}</td><td>${colorByKey[i.color]?.name || i.color}／${esc(i.size)}</td><td class="num">${money(i.price)}</td><td class="num">${i.qty}</td><td class="num">${money(i.price * i.qty)}</td></tr>`).join("")}
      </tbody></table>
      <dl class="sum"><dt>商品金額</dt><dd>${money(sub)}</dd><dt>運費</dt><dd>${shippingOf(sub) ? money(shippingOf(sub)) : "免運"}</dd><dt class="total">總計</dt><dd class="total">${money(sub + shippingOf(sub))}</dd></dl>
      ${document.body.dataset.side === "back" ? `<div class="actions between">
        <button class="ghost danger-text" id="od-delete">刪除訂單</button>
        <div class="actions"><label class="inline">狀態 <select id="od-status">${STATUSES.map((s) => `<option ${s === o.status ? "selected" : ""}>${s}</option>`).join("")}</select></label><button class="btn" id="od-save">儲存</button></div>
      </div>` : `<p class="muted">訂單狀態：${o.status}</p>`}
    </div>`, "wide");
  if (document.body.dataset.side !== "back") return;
  $("od-save").onclick = async () => {
    const status = $("od-status").value;
    await Store.updateOrder(o.id, { status });
    await reload();
    closeDialog();
    renderOrders();
    toast(`訂單 ${o.id} 已改成「${status}」`);
  };
  $("od-delete").onclick = async () => {
    if (!(await confirmBox(`確定要刪除訂單 ${o.id}？刪除後無法復原。`, "刪除"))) return;
    await Store.deleteOrder(o.id);
    await reload();
    renderOrders();
    toast(`已刪除訂單 ${o.id}`);
  };
}

$("o-export").addEventListener("click", () => {
  const rows = [["order_id", "date", "customer_id", "status", "product_id", "color", "size", "qty", "price"]];
  filteredOrders().forEach((o) => o.items.forEach((i) => rows.push([o.id, o.date, o.customer, o.status, i.product, i.color, i.size, i.qty, i.price])));
  const a = document.createElement("a");
  a.href = URL.createObjectURL(new Blob(["﻿" + rows.map((r) => r.join(",")).join("\n")], { type: "text/csv" }));
  a.download = "orders.csv";
  a.click();
  setTimeout(() => URL.revokeObjectURL(a.href), 1000);
});

let adminView = "products";
let custPage = 0;
document.querySelectorAll("[data-admin]").forEach((b) => b.addEventListener("click", () => {
  adminView = b.dataset.admin;
  renderAdmin();
}));
$("c-search").addEventListener("input", () => { custPage = 0; renderAdmin(); });

const VIEWS = [
  { name: "熱銷商品前五名檢視", note: "TOP 5 加上 JOIN 四張表，算出賣最多件的商品。", sql: "SELECT * FROM 熱銷商品前五名檢視 ORDER BY 銷售件數 DESC" },
  { name: "商品銷售檢視", note: "LEFT JOIN，沒賣過的商品也會列出來。", sql: "SELECT * FROM 商品銷售檢視 ORDER BY 銷售金額 DESC" },
  { name: "顧客RFM檢視", note: "每位顧客最近購買、次數、總金額，顧客分群用的就是這些欄位。", sql: "SELECT TOP 20 * FROM 顧客RFM檢視 ORDER BY 總金額 DESC" },
  { name: "顧客公開資料檢視", note: "只給客服看的欄位，電子郵件和手機被藏起來。", sql: "SELECT TOP 20 * FROM 顧客公開資料檢視 ORDER BY 顧客編號" },
  { name: "訂單金額檢視", note: "訂單總金額不存在資料表裡，由明細即時加總。", sql: "SELECT TOP 20 * FROM 訂單金額檢視 ORDER BY 訂購日期 DESC, 訂單編號 DESC" },
];
$("v-name").innerHTML = VIEWS.map((v) => `<option>${v.name}</option>`).join("");
$("v-name").addEventListener("change", renderViews);

function localView(name) {
  const valid = db.orders.filter((o) => o.status !== "已取消");
  const sales = db.products.map((p) => {
    const lines = valid.flatMap((o) => o.items.filter((i) => i.product === p.id));
    return [p.id, p.name, p.category, lines.reduce((s, i) => s + i.qty, 0), lines.reduce((s, i) => s + i.qty * i.price, 0)];
  });
  if (name === "熱銷商品前五名檢視") return { columns: ["商品編號", "商品名稱", "分類名稱", "銷售件數"], rows: [...sales].sort((a, b) => b[3] - a[3]).slice(0, 5).map((r) => r.slice(0, 4)) };
  if (name === "商品銷售檢視") return { columns: ["商品編號", "商品名稱", "分類名稱", "銷售件數", "銷售金額"], rows: [...sales].sort((a, b) => b[4] - a[4]) };
  if (name === "顧客RFM檢視") {
    return { columns: ["顧客編號", "最近購買天數", "購買次數", "總金額", "平均金額"], rows: state.profiles.map((p) => [p.id, p.recency, p.frequency, p.monetary, Math.round(p.average)]).sort((a, b) => b[3] - a[3]).slice(0, 20) };
  }
  if (name === "顧客公開資料檢視") return { columns: ["顧客編號", "縣市", "加入日期"], rows: [...db.customers].sort((a, b) => a.id.localeCompare(b.id)).slice(0, 20).map((c) => [c.id, c.city, c.joined]) };
  return {
    columns: ["訂單編號", "顧客編號", "訂購日期", "狀態", "小計", "件數"],
    rows: [...db.orders].sort((a, b) => (a.date < b.date ? 1 : a.date > b.date ? -1 : b.id.localeCompare(a.id))).slice(0, 20).map((o) => [o.id, o.customer, o.date, o.status, orderTotal(o), o.items.reduce((s, i) => s + i.qty, 0)]),
  };
}

async function renderViews() {
  const v = VIEWS.find((x) => x.name === $("v-name").value) || VIEWS[0];
  $("v-note").textContent = `${v.note}${Store.mode === "server" ? "下面的資料由 C# 程式直接查詢 SQL Server 的檢視表。" : "目前沒連資料庫，用同樣的規則在網頁上算；連 SQL Server 時會直接查詢檢視表。"}`;
  $("v-sql").textContent = v.sql + ";";
  try {
    const data = Store.mode === "server" ? await Store.view(v.name) : localView(v.name);
    const money2 = (c) => /金額|小計/.test(c);
    const cell = (c, x) => (x === null || x === undefined ? "" : typeof x === "number" ? (money2(c) ? money(x) : Math.round(x).toLocaleString("en-US")) : esc(x));
    $("v-table").innerHTML = `<thead><tr>${data.columns.map((c) => `<th${typeof data.rows[0]?.[data.columns.indexOf(c)] === "number" ? ' class="num"' : ""}>${esc(c)}</th>`).join("")}</tr></thead><tbody>${data.rows.map((r) => `<tr>${r.map((x, i) => `<td${typeof x === "number" ? ' class="num"' : ""}>${cell(data.columns[i], x)}</td>`).join("")}</tr>`).join("")}</tbody>`;
  } catch (err) {
    $("v-table").innerHTML = `<tbody><tr><td class="empty">${esc(err.message)}</td></tr></tbody>`;
  }
}

function renderAdmin() {
  document.querySelectorAll("[data-admin]").forEach((b) => b.classList.toggle("active", b.dataset.admin === adminView));
  $("admin-products").hidden = adminView !== "products";
  $("admin-customers").hidden = adminView !== "customers";
  $("admin-views").hidden = adminView !== "views";
  if (adminView === "views") renderViews();
  $("mode-label").textContent = Store.mode === "server" ? "資料來源：SQL Server" : "資料來源：本機示範資料（存在這台電腦的瀏覽器）";
  const sold = soldCount();
  $("p-body").innerHTML = db.products.map((p) => `
    <tr>
      <td><img class="thumb" src="${img(p.key)}" alt=""></td><td>${p.id}</td><td>${esc(p.name)}</td><td>${esc(p.category)}</td>
      <td class="num">${money(p.price)}</td><td class="sizes-cell">${p.sizes.map(esc).join(" ")}</td><td class="num">${(sold[p.id] || 0).toLocaleString("en-US")}</td>
      <td><span class="status ${p.active ? "s2" : "s3"}">${p.active ? "上架中" : "已下架"}</span></td>
      <td class="num nowrap"><button class="icon-btn" data-edit-p="${p.id}">編輯</button><button class="icon-btn danger-text" data-del-p="${p.id}">刪除</button></td>
    </tr>`).join("");
  const q = $("c-search").value.trim();
  const orderCount = {};
  db.orders.forEach((o) => (orderCount[o.customer] = (orderCount[o.customer] || 0) + 1));
  const list = db.customers.filter((c) => !q || c.id.includes(q.toUpperCase()) || c.email.includes(q) || c.city.includes(q));
  const per = 12;
  const pages = Math.max(1, Math.ceil(list.length / per));
  custPage = Math.min(custPage, pages - 1);
  $("c-body").innerHTML = list.slice(custPage * per, custPage * per + per).map((c) => `
    <tr><td>${c.id}</td><td>${esc(c.email)}</td><td class="nowrap">${esc(c.phone)}</td><td>${esc(c.city)}</td><td class="nowrap">${c.joined}</td><td class="num">${orderCount[c.id] || 0}</td>
    <td class="num nowrap"><button class="icon-btn" data-edit-c="${c.id}">編輯</button><button class="icon-btn danger-text" data-del-c="${c.id}">刪除</button></td></tr>`).join("") || `<tr><td colspan="7" class="empty">沒有符合的顧客。</td></tr>`;
  $("c-page").textContent = `${custPage + 1} / ${pages}（共 ${list.length} 位）`;
  $("c-prev").disabled = custPage === 0;
  $("c-next").disabled = custPage >= pages - 1;
}
$("c-prev").addEventListener("click", () => { custPage -= 1; renderAdmin(); });
$("c-next").addEventListener("click", () => { custPage += 1; renderAdmin(); });

function productForm(p) {
  const isNew = !p;
  p = p || { name: "", category: "配件", price: 500, sizes: ["單一尺寸"], description: "", key: "towel", active: true };
  openDialog(`
    <form class="form" id="p-form" novalidate>
      <h2>${isNew ? "新增商品" : `編輯商品 ${p.id}`}</h2>
      <div class="grid-form">
        <label>商品名稱<input name="name" value="${esc(p.name)}" required maxlength="30"></label>
        <label>分類<select name="category">${["服裝", "鞋款", "配件"].map((x) => `<option ${x === p.category ? "selected" : ""}>${x}</option>`).join("")}</select></label>
        <label>價格（元）<input name="price" type="number" min="1" max="100000" value="${p.price}" required></label>
        <label>尺寸（用空白分開）<input name="sizes" value="${esc(p.sizes.join(" "))}" required></label>
        <label>商品圖<select name="key">${IMAGE_KEYS.map((k) => `<option value="${k}" ${k === p.key ? "selected" : ""}>${k}</option>`).join("")}</select></label>
        <label>狀態<select name="active"><option value="1" ${p.active ? "selected" : ""}>上架中</option><option value="0" ${!p.active ? "selected" : ""}>已下架</option></select></label>
      </div>
      <label class="full">說明<textarea name="description" rows="3" maxlength="200">${esc(p.description)}</textarea></label>
      <p class="form-msg" id="form-msg"></p>
      <div class="actions end"><button type="button" class="ghost" id="form-cancel">取消</button><button class="btn" type="submit">${isNew ? "新增" : "儲存"}</button></div>
    </form>`);
  $("form-cancel").onclick = closeDialog;
  $("p-form").onsubmit = async (e) => {
    e.preventDefault();
    const d = Object.fromEntries(new FormData(e.target));
    const sizes = d.sizes.trim().split(/\s+/).filter(Boolean);
    const price = Number(d.price);
    if (!d.name.trim() || !sizes.length || !(price > 0)) {
      $("form-msg").textContent = "請填商品名稱、尺寸，價格要大於 0。";
      return;
    }
    try {
      const saved = await Store.saveProduct({ ...p, name: d.name.trim(), category: d.category, price, sizes, key: d.key, description: d.description.trim(), active: d.active === "1" });
      await reload();
      closeDialog();
      renderAdmin();
      toast(isNew ? `已新增商品 ${saved.id}` : `已更新商品 ${saved.id}`);
    } catch (err) {
      $("form-msg").textContent = err.message;
    }
  };
}

function customerForm(c) {
  const isNew = !c;
  c = c || { email: "", phone: "", city: "台北市" };
  openDialog(`
    <form class="form" id="c-form" novalidate>
      <h2>${isNew ? "新增顧客" : `編輯顧客 ${c.id}`}</h2>
      <div class="grid-form">
        <label>Email<input name="email" type="email" value="${esc(c.email)}" required maxlength="60"></label>
        <label>手機<input name="phone" value="${esc(c.phone)}" placeholder="0912-345-678" required></label>
        <label>縣市<select name="city">${CITIES.map((x) => `<option ${x === c.city ? "selected" : ""}>${x}</option>`).join("")}</select></label>
      </div>
      <p class="form-msg" id="form-msg"></p>
      <div class="actions end"><button type="button" class="ghost" id="form-cancel">取消</button><button class="btn" type="submit">${isNew ? "新增" : "儲存"}</button></div>
    </form>`);
  $("form-cancel").onclick = closeDialog;
  $("c-form").onsubmit = async (e) => {
    e.preventDefault();
    const d = Object.fromEntries(new FormData(e.target));
    const errs = [];
    if (!/^[^@\s]+@[^@\s]+\.[^@\s]+$/.test(d.email.trim())) errs.push("Email 格式");
    if (!PHONE.test(d.phone.trim())) errs.push("手機格式");
    if (errs.length) {
      $("form-msg").textContent = `請確認：${errs.join("、")}`;
      return;
    }
    try {
      const saved = await Store.saveCustomer({ ...c, email: d.email.trim(), phone: d.phone.trim(), city: d.city });
      await reload();
      fillCustomerSelect();
      closeDialog();
      renderAdmin();
      toast(isNew ? `已新增顧客 ${saved.id}` : `已更新顧客 ${saved.id}`);
    } catch (err) {
      $("form-msg").textContent = err.message;
    }
  };
}

$("admin").addEventListener("click", async (e) => {
  const t = e.target.closest("button");
  if (!t) return;
  try {
    if (t.id === "p-add") productForm(null);
    if (t.id === "c-add") customerForm(null);
    if (t.dataset.editP) productForm(productById(t.dataset.editP));
    if (t.dataset.editC) customerForm(customerById(t.dataset.editC));
    if (t.dataset.delP && (await confirmBox(`確定要刪除商品 ${t.dataset.delP}「${productById(t.dataset.delP)?.name}」？`, "刪除"))) {
      await Store.deleteProduct(t.dataset.delP);
      await reload();
      renderAdmin();
      toast("已刪除商品");
    }
    if (t.dataset.delC && (await confirmBox(`確定要刪除顧客 ${t.dataset.delC}？`, "刪除"))) {
      await Store.deleteCustomer(t.dataset.delC);
      await reload();
      fillCustomerSelect();
      renderAdmin();
      toast("已刪除顧客");
    }
    if (t.id === "reset-all" && (await confirmBox("要把資料還原成一開始的樣子嗎？新增、修改、刪除的內容都會清掉。", "還原"))) {
      await Store.reset();
      await reload();
      fillCustomerSelect();
      setCart({});
      renderAdmin();
      toast("已還原示範資料");
    }
  } catch (err) {
    toast(err.message, "error");
  }
});

const ACTIONS = {
  頂級大客戶: "專人窗口、大量採購優惠",
  高價值常客: "VIP 專屬價、新品搶先買",
  一般顧客: "推薦常一起買的商品",
  小額常客: "組合優惠，提高每次金額",
  新進顧客: "首購後送回購折扣",
  流失中的老顧客: "專人聯絡、送回購優惠",
  只買過一兩次: "第二次購買折扣，拉回來再買",
};

function nameGroup(c, g, sleepDays) {
  const [, f, m, a] = c;
  if (g.recency > sleepDays) return g.frequency >= 2.5 ? "流失中的老顧客" : "只買過一兩次";
  if (m > 0.7) return "高價值常客";
  if (f > -0.2 && a < -0.4) return "小額常客";
  if (f < -0.4 && g.recency < 60) return "新進顧客";
  return "一般顧客";
}

const RFM_FIELDS = [
  { key: "recency", label: "最近一次購買（幾天前）" },
  { key: "frequency", label: "購買次數" },
  { key: "monetary", label: "購買總金額" },
  { key: "average", label: "平均每次金額" },
];

const pound = (n) => `£${Math.round(n).toLocaleString("en-US")}`;
const asProfile = (id, v) => ({ id, recency: v[0], frequency: v[1], monetary: v[2], average: v[3], tenure: v[4] });

function cosineMatrix(columns) {
  const norms = columns.map((c) => Math.sqrt(c.reduce((s, v) => s + v * v, 0)));
  return columns.map((a, i) => columns.map((b, j) => {
    if (i === j) return 1;
    if (!norms[i] || !norms[j]) return 0;
    let dot = 0;
    for (let k = 0; k < a.length; k += 1) dot += a[k] * b[k];
    return dot / (norms[i] * norms[j]);
  }));
}

let source = "shop";
let realCache = null;

function shopSource() {
  const cutoff = shiftDate(TODAY, -CHURN_WINDOW);
  const valid = db.orders.filter((o) => o.status !== "已取消");
  const sets = (pick) => {
    const m = new Map();
    valid.filter(pick).forEach((o) => {
      if (!m.has(o.customer)) m.set(o.customer, new Set());
      o.items.forEach((i) => m.get(o.customer).add(i.product));
    });
    return m;
  };
  return {
    kind: "shop",
    profiles: state.profiles,
    products: db.products.map((p) => ({ id: p.id, name: p.name.replace(/（.*）/, "") })),
    similarity: state.similarity,
    money,
    topK: 3,
    label: (id) => id,
    me: me(),
    cutoff,
    churnRows() {
      const past = rfm(valid, cutoff, db.customers.map((c) => c.id));
      const back = new Set(valid.filter((o) => o.date > cutoff).map((o) => o.customer));
      return { rows: past, y: past.map((p) => (back.has(p.id) ? 0 : 1)) };
    },
    recSets: () => ({ before: sets((o) => o.date <= cutoff), after: sets((o) => o.date > cutoff) }),
    buyers: (pid) => new Set(valid.filter((o) => o.items.some((i) => i.product === pid)).map((o) => o.customer)).size,
  };
}

function realSource() {
  if (realCache) return realCache;
  const customers = REAL.customers;
  const products = REAL.products.map((p) => ({ id: p.id, name: p.name }));
  const columns = products.map((_, j) => customers.map((c) => (c.owned.includes(j) ? 1 : 0)));
  const withPast = customers.filter((c) => c.past);
  const toSets = (key) => new Map(withPast.map((c) => [c.id, new Set(c[key].map((j) => products[j].id))]));
  realCache = {
    kind: "real",
    profiles: customers.map((c) => asProfile(c.id, c.now)),
    products,
    similarity: cosineMatrix(columns),
    money: pound,
    topK: 5,
    label: (id) => `顧客 ${id}`,
    me: null,
    cutoff: REAL.meta.cutoff,
    churnRows: () => ({ rows: withPast.map((c) => asProfile(c.id, c.past)), y: withPast.map((c) => c.churn) }),
    recSets: () => ({ before: toSets("before"), after: toSets("after") }),
    buyers: (pid) => {
      const j = products.findIndex((p) => p.id === pid);
      return customers.filter((c) => c.owned.includes(j)).length;
    },
  };
  return realCache;
}

const mlSource = () => (source === "real" ? realSource() : shopSource());

const SOURCE_NOTES = {
  shop: () => `本店資料：${db.customers.length} 位顧客、${db.orders.length.toLocaleString("en-US")} 筆訂單的模擬資料，跟購物、訂單頁同一份，下單後會跟著變。`,
  real: () => `真實資料：${REAL.meta.name}（${REAL.meta.license}），英國一家網路禮品店 ${REAL.meta.from} 到 ${REAL.meta.to} 的交易。原始 ${REAL.meta.rawRows.toLocaleString("en-US")} 筆，去掉退貨、沒有顧客編號和運費後剩 ${REAL.meta.rows.toLocaleString("en-US")} 筆，${REAL.meta.customers.toLocaleString("en-US")} 位顧客；推薦只取最多人買的 ${REAL.products.length} 種商品。金額單位是英鎊。`,
};

function renderSourceBars() {
  document.querySelectorAll(".src-switch .seg").forEach((b) => b.classList.toggle("active", b.dataset.src === source));
  document.querySelectorAll(".src-note").forEach((n) => (n.textContent = SOURCE_NOTES[source]()));
}

document.querySelectorAll(".src-switch .seg").forEach((b) => b.addEventListener("click", () => {
  if (source === b.dataset.src) return;
  source = b.dataset.src;
  ["segments", "recommend", "churn"].forEach((k) => (drawn[k] = false));
  renderTab(currentTab());
}));

const clusterCache = new Map();
function clusterCustomers(k, profiles = mlSource().profiles) {
  const hit = clusterCache.get(k + ":" + profiles.length + ":" + (profiles[0]?.id || ""));
  if (hit && hit.profiles === profiles) return hit.out;
  const X = profiles.map((p) => RFM_FIELDS.map((f) => (f.key === "recency" ? p[f.key] : Math.log1p(p[f.key]))));
  const Z = ML.fitScaler(X).transform(X);
  const result = ML.kmeans(Z, k, 7);
  const groups = Array.from({ length: k }, (_, j) => {
    const members = profiles.filter((_, i) => result.labels[i] === j);
    const avg = (key) => members.reduce((s, m) => s + m[key], 0) / (members.length || 1);
    const c = result.centers[j];
    return { index: j, size: members.length, recency: avg("recency"), frequency: avg("frequency"), monetary: avg("monetary"), score: c[1] + c[2] - c[0] };
  });
  const order = [...groups].sort((a, b) => b.score - a.score);
  const recencies = profiles.map((p) => p.recency).sort((x, y) => x - y);
  const sleepDays = Math.max(120, 1.5 * (recencies[Math.floor(recencies.length / 2)] || 0));
  order.forEach((g) => (g.base = nameGroup(result.centers[g.index], g, sleepDays)));
  const rename = (family, key, names) => {
    const list = order.filter((g) => family.includes(g.base)).sort((a, b) => b[key] - a[key]);
    if (list.length > 1) list.forEach((g, i) => (g.base = names[Math.min(i, names.length - 1)]));
  };
  rename(["高價值常客"], "monetary", ["頂級大客戶", "高價值常客"]);
  rename(["流失中的老顧客", "只買過一兩次"], "frequency", ["流失中的老顧客", "只買過一兩次"]);
  const used = {};
  order.forEach((g) => (used[g.base] = (used[g.base] || 0) + 1));
  const seen = {};
  order.forEach((g) => {
    seen[g.base] = (seen[g.base] || 0) + 1;
    g.name = used[g.base] > 1 ? `${g.base} ${String.fromCharCode(64 + seen[g.base])}` : g.base;
  });
  const pca = ML.pca2(Z);
  const elbow = [2, 3, 4, 5, 6, 7, 8].map((kk) => ({ k: kk, inertia: ML.kmeans(Z, kk, 7).inertia }));
  const out = { labels: result.labels, groups, order, points: pca.project(Z), explained: pca.explained, elbow };
  clusterCache.set(k + ":" + profiles.length + ":" + (profiles[0]?.id || ""), { profiles, out });
  return out;
}

function segmentOf(customer) {
  const cl = clusterCustomers(Number($("k-select").value), state.profiles);
  const i = state.profiles.findIndex((p) => p.id === customer);
  return i < 0 ? null : cl.groups[cl.labels[i]].name;
}

const GROUP_COLORS = ["--g1", "--g2", "--g3", "--g4", "--g5", "--g6"];
const cssVar = (name) => getComputedStyle(document.documentElement).getPropertyValue(name).trim();

function renderSegments() {
  drawn.segments = true;
  renderSourceBars();
  const src = mlSource();
  const k = Number($("k-select").value);
  const cl = clusterCustomers(k, src.profiles);
  const rankOf = Object.fromEntries(cl.order.map((g, r) => [g.index, r]));
  const view = $("seg-view").value;
  const mine = src.me ? src.profiles.findIndex((p) => p.id === src.me) : -1;
  const pointOf = (i) => {
    const p = src.profiles[i];
    const id = src.label(p.id);
    if (view === "pca") return { x: cl.points[i][0], y: cl.points[i][1], id };
    const [fx, fy] = view.split("-");
    return { x: p[fx], y: p[fy], id };
  };
  const datasets = cl.order.map((g) => ({
    label: g.name,
    data: src.profiles.map((_, i) => i).filter((i) => cl.labels[i] === g.index).map(pointOf),
    backgroundColor: cssVar(GROUP_COLORS[rankOf[g.index]]),
    pointRadius: src.profiles.length > 1000 ? 2 : 4,
  }));
  if (mine >= 0) datasets.unshift({ label: "查看中的顧客", data: [pointOf(mine)], backgroundColor: "transparent", borderColor: cssVar("--text"), borderWidth: 2, pointRadius: 9, pointHoverRadius: 10 });
  const label = (key) => RFM_FIELDS.find((f) => f.key === key)?.label || key;
  const axes = view === "pca" ? [`主成分 1（${pct(cl.explained[0])}）`, `主成分 2（${pct(cl.explained[1])}）`] : view.split("-").map(label);
  Charts.scatter($("seg-chart"), datasets, { xLabel: axes[0], yLabel: axes[1] });
  Charts.line($("elbow-chart"), cl.elbow.map((e) => e.k), [{ label: "組內距離平方和", values: cl.elbow.map((e) => e.inertia), color: "a" }], { xLabel: "K", marker: k });
  $("seg-table").innerHTML = `<thead><tr><th>群組</th><th class="num">人數</th><th class="num">最近購買</th><th class="num">次數</th><th class="num">總金額</th><th>建議做法</th></tr></thead><tbody>${cl.order.map((g, r) => `
    <tr><td class="nowrap"><i class="swatch" style="background:${cssVar(GROUP_COLORS[r])}"></i>${g.name}</td><td class="num">${g.size.toLocaleString("en-US")}</td><td class="num">${Math.round(g.recency)} 天前</td><td class="num">${g.frequency.toFixed(1)}</td><td class="num">${src.money(g.monetary)}</td><td>${ACTIONS[g.base]}</td></tr>`).join("")}</tbody>`;
  $("seg-me").textContent = src.kind === "real" ? "" : mine >= 0 ? `${me()} 屬於「${cl.groups[cl.labels[mine]].name}」` : `${me()} 還沒有購買紀錄`;
}

["k-select", "seg-view"].forEach((id) => $(id).addEventListener("change", () => { drawn.segments = false; renderSegments(); }));

function renderRecommend() {
  drawn.recommend = true;
  renderSourceBars();
  const src = mlSource();
  const sel = $("rec-product");
  const keep = src.products.some((p) => p.id === sel.value) ? sel.value : src.products[0]?.id;
  sel.innerHTML = src.products.map((p) => `<option value="${esc(p.id)}">${esc(p.name)}</option>`).join("");
  sel.value = keep;
  renderRecommendDetail();
  const ev = recommendEval(src);
  $("rec-eval").innerHTML = `<div class="kpi"><span>相似度推薦</span><strong>${pct(ev.cf)}</strong></div><div class="kpi"><span>只推熱銷商品</span><strong>${pct(ev.pop)}</strong></div>`;
  $("rec-eval-note").textContent = `用 ${src.cutoff} 以前的購買紀錄推薦前 ${src.topK} 名，再看之後 ${CHURN_WINDOW} 天實際買了什麼：${ev.n.toLocaleString("en-US")} 位買了新商品的顧客中，推薦清單至少猜中一樣的比例。`;
  const dark = matchMedia("(prefers-color-scheme: dark)").matches;
  const rgb = dark ? "127,167,214" : "47,93,140";
  const shown = src.products.slice(0, 11);
  const short = (n) => (n.length > 18 ? n.slice(0, 17) + "…" : n);
  const off = shown.flatMap((_, i) => shown.map((__, j) => (i === j ? null : src.similarity[i][j])).filter((v) => v !== null));
  const lo = Math.min(...off);
  const hi = Math.max(...off);
  const head = `<div></div>${shown.map((p) => `<div class="h-col" title="${esc(p.name)}">${esc(short(p.name))}</div>`).join("")}`;
  const rows = shown.map((p, i) => `<div class="h-label" title="${esc(p.name)}">${esc(short(p.name))}</div>${shown.map((q, j) => {
    const v = src.similarity[i][j];
    const t = i === j ? 0 : (v - lo) / (hi - lo || 1);
    return `<div title="${esc(p.name)} × ${esc(q.name)}：${v.toFixed(2)}" style="background:rgba(${rgb},${(0.08 + t * 0.82).toFixed(2)});color:${t > 0.6 ? "#fff" : "inherit"}">${i === j ? "—" : v.toFixed(2)}</div>`;
  }).join("")}`).join("");
  const grid = $("sim-heatmap");
  grid.style.gridTemplateColumns = `${src.kind === "real" ? 150 : 96}px repeat(${shown.length}, minmax(52px, 1fr))`;
  grid.innerHTML = head + rows;
  $("sim-note").textContent = src.kind === "real" ? `顏色越深，兩個商品越常被同一批顧客買。這裡只列最多人買的 ${shown.length} 種，推薦是用全部 ${src.products.length} 種算的。` : "顏色越深，兩個商品越常被同一批顧客買。";
}
$("rec-product").addEventListener("change", renderRecommendDetail);

function renderRecommendDetail() {
  const src = mlSource();
  const pid = $("rec-product").value;
  const i = src.products.findIndex((p) => p.id === pid);
  if (i < 0) return;
  const top = src.products.map((p, j) => ({ p, score: src.similarity[i][j] })).filter((_, j) => j !== i).sort((a, b) => b.score - a.score).slice(0, 5);
  Charts.bars($("rec-chart"), top.map((s) => ({ label: s.p.name.length > 26 ? s.p.name.slice(0, 25) + "…" : s.p.name, value: s.score })), { xLabel: "餘弦相似度", horizontal: true });
  $("rec-note").textContent = `共有 ${src.buyers(pid).toLocaleString("en-US")} 位顧客買過「${src.products[i].name}」。`;
}

function recommendEval(src) {
  const { before, after } = src.recSets();
  const ids = src.products.map((p) => p.id);
  const owners = [...before.keys()];
  const columns = ids.map((pid) => owners.map((c) => (before.get(c).has(pid) ? 1 : 0)));
  const sim = cosineMatrix(columns);
  const popular = columns.map((col) => col.reduce((s, v) => s + v, 0));
  const top = (scores, owned) => scores.map((v, j) => ({ v: owned.has(ids[j]) ? -1 : v, id: ids[j] })).sort((a, b) => b.v - a.v).slice(0, src.topK).map((x) => x.id);
  let n = 0;
  let cf = 0;
  let pop = 0;
  owners.forEach((c) => {
    const owned = before.get(c);
    const fresh = [...(after.get(c) || [])].filter((p) => !owned.has(p));
    if (!fresh.length) return;
    n += 1;
    const have = ids.map((pid) => owned.has(pid));
    const score = ids.map((_, j) => have.reduce((s, h, i) => (h && i !== j ? s + sim[i][j] : s), 0));
    if (top(score, owned).some((p) => fresh.includes(p))) cf += 1;
    if (top(popular, owned).some((p) => fresh.includes(p))) pop += 1;
  });
  return { n, cf: n ? cf / n : 0, pop: n ? pop / n : 0 };
}

const CHURN_FEATURES = ["距上次購買天數", "購買次數", "總金額（log）", "平均金額（log）", "成為顧客天數"];
const churnFeatures = (p) => [p.recency, p.frequency, Math.log1p(p.monetary), Math.log1p(p.average), p.tenure];
const predictLabels = (model, scaler, X) => model.predictProba(scaler.transform(X)).map((p) => (p >= 0.5 ? 1 : 0));

function bestRuleDays(rows, labels) {
  const acc = (t) => rows.reduce((s, r, i) => s + ((r[0] >= t ? 1 : 0) === labels[i] ? 1 : 0), 0) / rows.length;
  let best = 7;
  for (let t = 7; t < 400; t += 7) if (acc(t) > acc(best)) best = t;
  return { days: best, accuracy: acc };
}

function churnModel(src) {
  const { rows, y } = src.churnRows();
  const X = rows.map(churnFeatures);
  const { train, test } = ML.trainTestSplit(X, y, 0.25, 3);
  const scaler = ML.fitScaler(train.X);
  const model = ML.fitLogisticRegression(scaler.transform(train.X), train.y);
  const metrics = ML.classificationMetrics(test.y, predictLabels(model, scaler, test.X));
  const folds = ML.kFolds(X.length, 5, 11).map(({ train: a, test: b }) => {
    const tx = a.map((i) => X[i]);
    const ty = a.map((i) => y[i]);
    const vx = b.map((i) => X[i]);
    const vy = b.map((i) => y[i]);
    const sc = ML.fitScaler(tx);
    const md = ML.fitLogisticRegression(sc.transform(tx), ty);
    const rule = bestRuleDays(tx, ty);
    return {
      model: ML.classificationMetrics(vy, predictLabels(md, sc, vx)).accuracy,
      rule: bestRuleDays(vx, vy).accuracy(rule.days),
      days: rule.days,
    };
  });
  const mean = (key) => folds.reduce((s, f) => s + f[key], 0) / folds.length;
  const churnRate = y.reduce((s, v) => s + v, 0) / y.length;
  const now = src.profiles.map((p) => ({ p, prob: model.predictProba(scaler.transform([churnFeatures(p)]))[0] }));
  const ruleDays = folds.map((f) => f.days).sort((a, b) => a - b)[2];
  return { metrics, cvMean: mean("model"), ruleCv: mean("rule"), ruleDays, baseline: Math.max(churnRate, 1 - churnRate), model, now, size: { all: y.length, train: train.y.length, test: test.y.length }, churnRate };
}

function renderChurn() {
  drawn.churn = true;
  renderSourceBars();
  const src = mlSource();
  const cm = churnModel(src);
  const m = cm.metrics;
  $("ch-acc").textContent = pct(m.accuracy);
  $("ch-cv").textContent = pct(cm.cvMean);
  $("ch-base").textContent = pct(cm.baseline);
  $("ch-rule").textContent = pct(cm.ruleCv);
  $("ch-rule-label").textContent = `規則：${cm.ruleDays} 天沒買（5 折）`;
  $("ch-base-label").textContent = `全部猜「${cm.churnRate > 0.5 ? "會流失" : "不會流失"}」`;
  $("ch-rec").textContent = `${pct(m.precision)} / ${pct(m.recall)}`;
  $("ch-setup").textContent = `用 ${src.cutoff} 以前的紀錄當特徵，看之後 ${CHURN_WINDOW} 天有沒有再買，沒再買就算流失（${cm.size.all.toLocaleString("en-US")} 人，流失比例 ${pct(cm.churnRate)}）。訓練 ${cm.size.train.toLocaleString("en-US")} 人、測試 ${cm.size.test.toLocaleString("en-US")} 人。規則的天數只用訓練資料挑，跟模型比的是同樣的 5 折。`;
  $("ch-confusion").innerHTML = `
    <tr><th></th><th>預測流失</th><th>預測留下</th></tr>
    <tr><th>實際流失</th><td class="hit">${m.tp}</td><td>${m.fn}</td></tr>
    <tr><th>實際留下</th><td>${m.fp}</td><td class="hit">${m.tn}</td></tr>`;
  const w = cm.model.weights.slice(1).map((v, i) => ({ label: CHURN_FEATURES[i], value: v })).sort((a, b) => b.value - a.value);
  Charts.bars($("ch-weights"), w, { xLabel: "係數（越大越容易流失）", horizontal: true, signed: true });
  const risky = cm.now.filter((x) => x.p.recency >= 14 && x.p.recency <= 120).sort((a, b) => b.prob - a.prob).slice(0, 10);
  $("ch-list").innerHTML = risky.map((x) => `<tr${src.me && x.p.id === src.me ? ' class="me"' : ""}><td>${esc(src.label(x.p.id))}</td><td class="num">${x.p.recency} 天前</td><td class="num">${x.p.frequency}</td><td class="num">${src.money(x.p.monetary)}</td><td class="num"><b>${pct(x.prob)}</b></td></tr>`).join("");
  const mine = src.me ? cm.now.find((x) => x.p.id === src.me) : null;
  $("ch-me").textContent = mine ? `${me()} 的流失機率：${pct(mine.prob)}` : "";
}

(async () => {
  if ((await Store.init()) === "server") {
    const d = new Date();
    TODAY = `${d.getFullYear()}-${String(d.getMonth() + 1).padStart(2, "0")}-${String(d.getDate()).padStart(2, "0")}`;
  }
  await reload();
  fillCustomerSelect();
  setCart(getCart());
  renderShop();
})();
