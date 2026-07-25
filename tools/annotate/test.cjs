#!/usr/bin/env node
/**
 * Smoke-test live style / line-attach / select-all for tools/annotate
 */
const { chromium } = require("playwright");
const http = require("http");
const fs = require("fs");
const path = require("path");

const ROOT = __dirname;
const PORT = 8766;

function contentType(p) {
  if (p.endsWith(".html")) return "text/html";
  if (p.endsWith(".js")) return "application/javascript";
  if (p.endsWith(".css")) return "text/css";
  return "application/octet-stream";
}

const server = http.createServer((req, res) => {
  let url = req.url.split("?")[0];
  if (url === "/") url = "/index.html";
  // allow ../tool-shell.css
  const file = path.normalize(path.join(ROOT, url.replace(/^\//, "")));
  const allowed = file.startsWith(ROOT) || file.startsWith(path.join(ROOT, ".."));
  if (!allowed || !fs.existsSync(file) || fs.statSync(file).isDirectory()) {
    // tool-shell from parent
    const parent = path.normalize(path.join(ROOT, "..", url.replace(/^\//, "")));
    if (url.startsWith("/../") || url.includes("tool-shell")) {
      const alt = path.join(ROOT, "..", "tool-shell.css");
      if (fs.existsSync(alt)) {
        res.writeHead(200, { "Content-Type": "text/css" });
        res.end(fs.readFileSync(alt));
        return;
      }
    }
    res.writeHead(404);
    res.end("missing " + url);
    return;
  }
  res.writeHead(200, { "Content-Type": contentType(file) });
  res.end(fs.readFileSync(file));
});

function assert(cond, msg) {
  if (!cond) throw new Error(msg);
}

(async () => {
  await new Promise((r) => server.listen(PORT, r));
  const browser = await chromium.launch({ headless: true });
  const page = await browser.newPage();
  // Fix CSS path: page loads ../tool-shell.css from annotate/
  await page.route("**/tool-shell.css", async (route) => {
    const css = fs.readFileSync(path.join(ROOT, "..", "tool-shell.css"));
    await route.fulfill({ status: 200, contentType: "text/css", body: css });
  });

  await page.goto(`http://127.0.0.1:${PORT}/index.html`, { waitUntil: "networkidle" });
  await page.waitForFunction(() => window.__ANN_TEST__);

  await page.evaluate(async () => {
    await window.__ANN_TEST__.loadBlank(800, 600);
    window.__ANN_TEST__.addAnn({
      title: "Encoders",
      subtitle: "turn · press",
      target: { x: 500, y: 320 },
      elbow: { x: 300, y: 320 },
      label: { x: 40, y: 300 },
    });
    window.__ANN_TEST__.addAnn({
      title: "Matrix",
      subtitle: "16×16",
      target: { x: 520, y: 200 },
      elbow: { x: 300, y: 200 },
      label: { x: 40, y: 180 },
    });
  });

  // Select all button
  await page.click("#btn-select-all");
  let n = await page.evaluate(() => window.__ANN_TEST__.selectedCount());
  assert(n === 2, `select all expected 2, got ${n}`);

  // Font size live
  await page.fill("#style-title-size", "40");
  await page.dispatchEvent("#style-title-size", "input");
  await page.dispatchEvent("#style-title-size", "change");
  let style = await page.evaluate(() => window.__ANN_TEST__.getStyle());
  assert(style.titleSize === 40, `titleSize expected 40, got ${style.titleSize}`);

  // Font family live
  await page.selectOption("#style-font", "space");
  style = await page.evaluate(() => window.__ANN_TEST__.getStyle());
  assert(style.fontFamily.includes("Space Grotesk"), `font not Space Grotesk: ${style.fontFamily}`);

  // Line color live
  await page.fill("#style-line-color", "#ff0000");
  await page.dispatchEvent("#style-line-color", "change");
  style = await page.evaluate(() => window.__ANN_TEST__.getStyle());
  assert(style.lineColor.toLowerCase() === "#ff0000", `lineColor ${style.lineColor}`);

  // Text color live
  await page.fill("#style-title-color", "#00aa00");
  await page.dispatchEvent("#style-title-color", "input");
  await page.dispatchEvent("#style-title-color", "change");
  style = await page.evaluate(() => window.__ANN_TEST__.getStyle());
  assert(style.titleColor.toLowerCase() === "#00aa00", `titleColor ${style.titleColor}`);

  // Halo border widths live
  await page.fill("#style-halo-text", "12");
  await page.dispatchEvent("#style-halo-text", "input");
  await page.dispatchEvent("#style-halo-text", "change");
  await page.fill("#style-halo-line", "8");
  await page.dispatchEvent("#style-halo-line", "input");
  await page.dispatchEvent("#style-halo-line", "change");
  style = await page.evaluate(() => window.__ANN_TEST__.getStyle());
  assert(style.haloTextWidth === 12, `haloTextWidth expected 12, got ${style.haloTextWidth}`);
  assert(style.haloLineWidth === 8, `haloLineWidth expected 8, got ${style.haloLineWidth}`);
  assert(style.halo === true, "halo should stay on");

  // Line attach: on → under-text path uses text height + Gap Y
  await page.selectOption("#style-line-attach", "on");
  await page.fill("#style-line-margin", "12");
  await page.dispatchEvent("#style-line-margin", "input");
  await page.dispatchEvent("#style-line-margin", "change");
  await page.fill("#style-line-x-gap", "10");
  await page.dispatchEvent("#style-line-x-gap", "input");
  await page.dispatchEvent("#style-line-x-gap", "change");
  await page.fill("#style-line-x-offset", "5");
  await page.dispatchEvent("#style-line-x-offset", "input");
  await page.dispatchEvent("#style-line-x-offset", "change");
  let under = await page.evaluate(() => {
    const ann = window.__ANN_TEST__.getProject().annotations[0];
    const box = window.__ANN_TEST__.measureLabel(ann);
    const st = window.__ANN_TEST__.getStyle();
    const pts = window.__ANN_TEST__.leaderPoints(ann);
    return {
      n: pts.length,
      y2: pts[2].y,
      y3: pts[3].y,
      expectY: ann.label.y + box.h + st.lineMargin,
      left: Math.min(pts[2].x, pts[3].x),
      right: Math.max(pts[2].x, pts[3].x),
      expectLeft: ann.label.x - st.lineXGap + st.lineXOffset,
      expectRight: ann.label.x + box.w + st.lineXGap + st.lineXOffset,
    };
  });
  assert(under.n === 4, `"on" leader should have 4 points (under text), got ${under.n}`);
  assert(Math.abs(under.y2 - under.y3) < 0.01, "under-text segment should be horizontal");
  assert(Math.abs(under.y2 - under.expectY) < 0.01, `under-line y ${under.y2} should be label.y+height+gap ${under.expectY}`);
  assert(Math.abs(under.left - under.expectLeft) < 0.01, `x gap/offset left ${under.left} vs ${under.expectLeft}`);
  assert(Math.abs(under.right - under.expectRight) < 0.01, `x gap/offset right ${under.right} vs ${under.expectRight}`);

  // Changing title size must move the under-line
  await page.fill("#style-title-size", "48");
  await page.dispatchEvent("#style-title-size", "input");
  await page.dispatchEvent("#style-title-size", "change");
  under = await page.evaluate(() => {
    const ann = window.__ANN_TEST__.getProject().annotations[0];
    const box = window.__ANN_TEST__.measureLabel(ann);
    const st = window.__ANN_TEST__.getStyle();
    const pts = window.__ANN_TEST__.leaderPoints(ann);
    return { y: pts[2].y, expectY: ann.label.y + box.h + st.lineMargin, h: box.h };
  });
  assert(under.h >= 48, `box height should reflect title size, got ${under.h}`);
  assert(Math.abs(under.y - under.expectY) < 0.01, `after size change under-line y ${under.y} vs ${under.expectY}`);

  // left mode → 3 points, end left of label
  await page.selectOption("#style-line-attach", "left");
  await page.fill("#style-line-margin", "20");
  await page.dispatchEvent("#style-line-margin", "input");
  await page.dispatchEvent("#style-line-margin", "change");
  const leftCheck = await page.evaluate(() => {
    const ann = window.__ANN_TEST__.getProject().annotations[0];
    const pts = window.__ANN_TEST__.leaderPoints(ann);
    const st = window.__ANN_TEST__.getStyle();
    return { n: pts.length, endX: pts[pts.length - 1].x, labelX: ann.label.x, gap: st.lineMargin, attach: st.lineAttach };
  });
  assert(leftCheck.attach === "left", "attach should be left");
  assert(leftCheck.n === 3, `left leader points ${leftCheck.n}`);
  assert(leftCheck.endX === leftCheck.labelX - leftCheck.gap, `left gap end ${leftCheck.endX} vs ${leftCheck.labelX - leftCheck.gap}`);

  // right mode
  await page.selectOption("#style-line-attach", "right");
  const rightCheck = await page.evaluate(() => {
    const ann = window.__ANN_TEST__.getProject().annotations[0];
    const box = window.__ANN_TEST__.measureLabel(ann);
    const pts = window.__ANN_TEST__.leaderPoints(ann);
    const st = window.__ANN_TEST__.getStyle();
    return { n: pts.length, endX: pts[pts.length - 1].x, expect: ann.label.x + box.w + st.lineMargin };
  });
  assert(rightCheck.n === 3, `right points ${rightCheck.n}`);
  assert(Math.abs(rightCheck.endX - rightCheck.expect) < 0.01, `right end ${rightCheck.endX} vs ${rightCheck.expect}`);

  console.log("OK — select-all, fonts/sizes/colors, line-attach modes all apply live");
  await browser.close();
  server.close();
  process.exit(0);
})().catch(async (err) => {
  console.error("FAIL:", err.message);
  process.exit(1);
});
