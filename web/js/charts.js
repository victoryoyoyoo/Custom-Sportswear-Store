const Charts = (() => {
  const instances = new Map();

  function palette() {
    const css = getComputedStyle(document.documentElement);
    const v = (name) => css.getPropertyValue(name).trim();
    return { a: v("--chart-a"), b: v("--chart-b"), neg: v("--chart-neg"), grid: v("--chart-grid"), text: v("--chart-text") };
  }

  function draw(canvas, config) {
    if (instances.has(canvas)) instances.get(canvas).destroy();
    const c = palette();
    Chart.defaults.font.family = getComputedStyle(document.body).fontFamily;
    Chart.defaults.font.size = 12;
    Chart.defaults.color = c.text;
    Chart.defaults.borderColor = c.grid;
    const chart = new Chart(canvas, {
      ...config,
      options: {
        responsive: true,
        maintainAspectRatio: false,
        animation: { duration: 350 },
        ...config.options,
        plugins: {
          legend: { display: false },
          tooltip: { padding: 10, cornerRadius: 4, displayColors: false },
          ...(config.options?.plugins || {}),
        },
      },
    });
    instances.set(canvas, chart);
    return chart;
  }

  const axis = (title, extra = {}) => ({
    title: { display: Boolean(title), text: title },
    grid: { drawTicks: false },
    border: { display: false },
    ticks: { padding: 6 },
    ...extra,
  });

  const legend = { display: true, position: "bottom", labels: { usePointStyle: true, boxWidth: 7, boxHeight: 7, padding: 14 } };

  function scatter(canvas, datasets, { xLabel, yLabel }) {
    return draw(canvas, {
      type: "scatter",
      data: { datasets: datasets.map((d) => ({ pointRadius: 3.5, pointHoverRadius: 5, ...d })) },
      options: {
        scales: { x: axis(xLabel), y: axis(yLabel) },
        plugins: {
          legend,
          tooltip: { padding: 10, cornerRadius: 4, displayColors: false, callbacks: { label: (ctx) => `${ctx.raw.id}（${ctx.dataset.label}）` } },
        },
      },
    });
  }

  function line(canvas, labels, series, { xLabel, marker, yMoney } = {}) {
    const c = palette();
    return draw(canvas, {
      type: "line",
      data: {
        labels,
        datasets: series.map((s) => ({
          label: s.label,
          data: s.values,
          borderColor: c[s.color] || s.color,
          backgroundColor: c[s.color] || s.color,
          borderWidth: 2,
          tension: 0.25,
          pointRadius: labels.map((l) => (l === marker ? 5 : 2.5)),
        })),
      },
      options: {
        interaction: { mode: "index", intersect: false },
        scales: {
          x: axis(xLabel, { grid: { display: false } }),
          y: axis("", { ticks: { padding: 6, callback: (v) => (yMoney ? `${Math.round(v / 1000)}k` : v) } }),
        },
        plugins: {
          tooltip: {
            padding: 10, cornerRadius: 4, displayColors: false,
            callbacks: { label: (ctx) => `${ctx.dataset.label} ${yMoney ? `NT$${Math.round(ctx.parsed.y).toLocaleString("en-US")}` : Math.round(ctx.parsed.y * 10) / 10}` },
          },
        },
      },
    });
  }

  function bars(canvas, items, { xLabel, horizontal, signed } = {}) {
    const c = palette();
    return draw(canvas, {
      type: "bar",
      data: {
        labels: items.map((d) => d.label),
        datasets: [{ data: items.map((d) => Number(d.value.toFixed(3))), backgroundColor: items.map((d) => (signed && d.value < 0 ? c.a : signed ? c.neg : c.a)), borderRadius: 3, maxBarThickness: 22 }],
      },
      options: {
        indexAxis: horizontal ? "y" : "x",
        scales: horizontal
          ? { x: axis(xLabel, signed ? {} : { min: 0 }), y: axis("", { grid: { display: false } }) }
          : { x: axis("", { grid: { display: false } }), y: axis(xLabel, { beginAtZero: true }) },
      },
    });
  }

  return { scatter, line, bars };
})();
