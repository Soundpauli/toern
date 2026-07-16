(function () {
  function setNavOpen(open) {
    document.body.classList.toggle("hb-nav-open", open);
    var btn = document.getElementById("hb-nav-toggle");
    var backdrop = document.getElementById("hb-nav-backdrop");
    if (btn) {
      btn.setAttribute("aria-expanded", open ? "true" : "false");
      btn.setAttribute("aria-label", open ? "Close handbook menu" : "Open handbook menu");
    }
    if (backdrop) {
      backdrop.setAttribute("aria-hidden", open ? "false" : "true");
    }
    document.body.style.overflow = open ? "hidden" : "";
  }

  function init() {
    var btn = document.getElementById("hb-nav-toggle");
    var backdrop = document.getElementById("hb-nav-backdrop");
    var sidebar = document.getElementById("hb-sidebar");
    if (!btn || !sidebar) return;

    btn.addEventListener("click", function () {
      setNavOpen(!document.body.classList.contains("hb-nav-open"));
    });

    if (backdrop) {
      backdrop.addEventListener("click", function () {
        setNavOpen(false);
      });
    }

    sidebar.querySelectorAll("a").forEach(function (a) {
      a.addEventListener("click", function () {
        setNavOpen(false);
      });
    });

    document.addEventListener("keydown", function (e) {
      if (e.key === "Escape") setNavOpen(false);
    });
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
  } else {
    init();
  }
})();
