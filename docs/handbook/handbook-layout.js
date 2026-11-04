/* One panel choice for the whole handbook, including every LED preview. */
(function () {
  "use strict";
  var width = 32;
  try { if (localStorage.getItem("toern-handbook-width") === "16") width = 16; } catch (_) {}

  function apply() {
    document.documentElement.dataset.matrixWidth = String(width);
    document.documentElement.style.setProperty("--matrix-ratio", width / 16);
    document.querySelectorAll("[data-hb-steps]").forEach(function (el) { el.textContent = width; });
    document.querySelectorAll("[data-hb-pages]").forEach(function (el) { el.textContent = 256 / width; });
    document.querySelectorAll("[data-hb-size]").forEach(function (el) { el.textContent = width + " × 16"; });
    document.querySelectorAll(".hb-layout-choice button").forEach(function (el) {
      el.setAttribute("aria-pressed", String(Number(el.dataset.width) === width));
    });
    // The hardware port map is a schematic, but its matrix area follows the choice.
    document.querySelectorAll("[data-hb-matrix-area]").forEach(function (el) {
      el.setAttribute("y", width === 32 ? "180" : "80");
      el.setAttribute("height", width === 32 ? "200" : "400");
    });
  }

  window.HBLayout = {
    refresh: apply,
    get width() { return width; },
    mount: function (parent) {
      var choice = document.createElement("div");
      choice.className = "hb-layout-choice";
      choice.setAttribute("role", "group");
      choice.setAttribute("aria-label", "Handbook matrix layout");
      choice.innerHTML = '<span>LED grid</span><button type="button" data-width="32">32×16</button><button type="button" data-width="16">16×16</button>';
      choice.addEventListener("click", function (event) {
        var button = event.target.closest("button[data-width]");
        if (!button) return;
        width = Number(button.dataset.width);
        try { localStorage.setItem("toern-handbook-width", String(width)); } catch (_) {}
        apply();
        window.dispatchEvent(new Event("hb-layout-change"));
      });
      if (!parent) return;
      choice.setAttribute("aria-label", "Matrix layout");
      parent.appendChild(choice);
      apply();
    }
  };
  apply();
  document.addEventListener("DOMContentLoaded", apply);
})();
