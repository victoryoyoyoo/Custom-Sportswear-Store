const ML = (() => {
  function seededRandom(seed) {
    let state = seed >>> 0;
    return () => {
      state = (state * 1664525 + 1013904223) >>> 0;
      return state / 4294967296;
    };
  }

  function shuffledIndices(n, seed) {
    const random = seededRandom(seed);
    const order = Array.from({ length: n }, (_, i) => i);
    for (let i = n - 1; i > 0; i--) {
      const j = Math.floor(random() * (i + 1));
      [order[i], order[j]] = [order[j], order[i]];
    }
    return order;
  }

  function trainTestSplit(X, y, testRatio, seed) {
    const order = shuffledIndices(X.length, seed);
    const testSize = Math.round(X.length * testRatio);
    const pick = (ids) => ({ X: ids.map((i) => X[i]), y: ids.map((i) => y[i]), ids });
    return { train: pick(order.slice(testSize)), test: pick(order.slice(0, testSize)) };
  }

  function kFolds(n, k, seed) {
    const order = shuffledIndices(n, seed);
    return Array.from({ length: k }, (_, fold) => ({
      train: order.filter((_, i) => i % k !== fold),
      test: order.filter((_, i) => i % k === fold),
    }));
  }

  function fitScaler(X) {
    const d = X[0].length;
    const mean = Array(d).fill(0);
    const std = Array(d).fill(0);
    X.forEach((row) => row.forEach((v, j) => (mean[j] += v / X.length)));
    X.forEach((row) => row.forEach((v, j) => (std[j] += (v - mean[j]) ** 2 / X.length)));
    for (let j = 0; j < d; j++) std[j] = Math.sqrt(std[j]) || 1;
    return { mean, std, transform: (rows) => rows.map((row) => row.map((v, j) => (v - mean[j]) / std[j])) };
  }

  const dist2 = (a, b) => a.reduce((s, v, i) => s + (v - b[i]) ** 2, 0);
  const dot = (a, b) => a.reduce((s, v, i) => s + v * b[i], 0);

  function kmeans(X, k, seed = 1, maxIter = 100) {
    const random = seededRandom(seed);
    const centers = [X[Math.floor(random() * X.length)]];
    while (centers.length < k) {
      const d = X.map((x) => Math.min(...centers.map((c) => dist2(x, c))));
      const total = d.reduce((s, v) => s + v, 0);
      let r = random() * total;
      let idx = 0;
      while (r > d[idx] && idx < d.length - 1) r -= d[idx++];
      centers.push(X[idx]);
    }
    let labels = Array(X.length).fill(0);
    for (let iter = 0; iter < maxIter; iter++) {
      const next = X.map((x) => {
        let best = 0;
        centers.forEach((c, j) => {
          if (dist2(x, c) < dist2(x, centers[best])) best = j;
        });
        return best;
      });
      const changed = next.some((v, i) => v !== labels[i]);
      labels = next;
      for (let j = 0; j < k; j++) {
        const members = X.filter((_, i) => labels[i] === j);
        if (members.length) centers[j] = members[0].map((_, d) => members.reduce((s, m) => s + m[d], 0) / members.length);
      }
      if (!changed && iter > 0) break;
    }
    const inertia = X.reduce((s, x, i) => s + dist2(x, centers[labels[i]]), 0);
    return { labels, centers, inertia };
  }

  function pca2(X) {
    const d = X[0].length;
    const cov = Array.from({ length: d }, () => Array(d).fill(0));
    X.forEach((x) => {
      for (let a = 0; a < d; a++) for (let b = 0; b < d; b++) cov[a][b] += (x[a] * x[b]) / X.length;
    });
    const components = [];
    const variances = [];
    let M = cov.map((r) => [...r]);
    for (let c = 0; c < 2; c++) {
      let v = Array.from({ length: d }, (_, i) => 1 / Math.sqrt(d) + i * 1e-3);
      for (let it = 0; it < 200; it++) {
        const w = M.map((row) => dot(row, v));
        const norm = Math.sqrt(dot(w, w)) || 1;
        v = w.map((x) => x / norm);
      }
      const lambda = dot(v, M.map((row) => dot(row, v)));
      components.push(v);
      variances.push(lambda);
      M = M.map((row, a) => row.map((val, b) => val - lambda * v[a] * v[b]));
    }
    const total = cov.reduce((s, row, i) => s + row[i], 0);
    return {
      components,
      explained: variances.map((v) => v / total),
      project: (rows) => rows.map((x) => components.map((comp) => dot(x, comp))),
    };
  }

  const sigmoid = (z) => 1 / (1 + Math.exp(-z));

  function fitLogisticRegression(X, y, { learningRate = 0.1, epochs = 3000 } = {}) {
    const Xb = X.map((r) => [1, ...r]);
    const d = Xb[0].length;
    const weights = Array(d).fill(0);
    for (let epoch = 0; epoch < epochs; epoch++) {
      const gradient = Array(d).fill(0);
      Xb.forEach((row, i) => {
        const error = sigmoid(dot(row, weights)) - y[i];
        row.forEach((v, j) => (gradient[j] += (error * v) / Xb.length));
      });
      for (let j = 0; j < d; j++) weights[j] -= learningRate * gradient[j];
    }
    return { weights, predictProba: (rows) => rows.map((r) => sigmoid(dot([1, ...r], weights))) };
  }

  function classificationMetrics(actual, predicted) {
    const m = { tp: 0, tn: 0, fp: 0, fn: 0 };
    actual.forEach((v, i) => {
      if (v === 1 && predicted[i] === 1) m.tp++;
      else if (v === 0 && predicted[i] === 0) m.tn++;
      else if (v === 0 && predicted[i] === 1) m.fp++;
      else m.fn++;
    });
    return {
      ...m,
      accuracy: (m.tp + m.tn) / actual.length,
      precision: m.tp / (m.tp + m.fp || 1),
      recall: m.tp / (m.tp + m.fn || 1),
    };
  }

  function cosine(a, b) {
    const n = Math.sqrt(dot(a, a) * dot(b, b));
    return n ? dot(a, b) / n : 0;
  }

  return { trainTestSplit, kFolds, fitScaler, kmeans, pca2, fitLogisticRegression, classificationMetrics, cosine };
})();
