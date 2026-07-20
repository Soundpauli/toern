(function () {
  var NAV = [
    {
      group: "Get started",
      items: [
        { href: "index.html", title: "Welcome", desc: "First boot" },
        { href: "hardware.html", title: "Hardware", desc: "Ports & buttons" },
        { href: "matrix.html", title: "The matrix", desc: "Rows & voices" },
        { href: "controls.html", title: "Controls", desc: "Encoders & touch" },
        { href: "troubleshooting.html", title: "Troubleshoot", desc: "Fix surprises", tone: "alert" }
      ]
    },
    {
      group: "Compose",
      items: [
        { href: "sequencing.html", title: "Sequencing", desc: "Draw, paint, single" },
        { href: "note-detail.html", title: "Note detail", desc: "Per-step edit" },
        { href: "sound.html", title: "Sound design", desc: "Filters & ADSR" },
        { href: "samples.html", title: "Samples", desc: "Sample browser, record" },
        { href: "sample-packs.html", title: "Sample packs", desc: "Pack slots & folders" }
      ]
    },
    {
      group: "Operate",
      items: [
        { href: "tempo-midi.html", title: "Tempo & MIDI", desc: "Clock & sync" },
        { href: "song.html", title: "Song", desc: "Arranger & pages" },
        { href: "files.html", title: "Files", desc: "Load, save, autosave" },
        { href: "menus.html", title: "Menu map", desc: "Full tree" },
        { href: "reference.html", title: "Screen index", desc: "All displays" }
      ]
    }
  ];

  function currentPage() {
    var file = (location.pathname.split("/").pop() || "").split("?")[0];
    if (!file || file.indexOf(".html") === -1) return "index.html";
    return file;
  }

  function escapeHtml(s) {
    return String(s)
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;");
  }

  function renderSidebar(sidebar) {
    var page = currentPage();
    var html = [];
    html.push(
      '<header class="hb-sidebar__brand">',
      '<h1><a href="index.html" class="hb-sidebar__logo">TŒRN</a></h1>',
      '<p class="hb-sidebar__kicker">HANDBOOK</p>',
      "</header>",
      '<nav class="hb-sidebar__nav">'
    );

    NAV.forEach(function (section) {
      html.push('<p class="hb-sidebar__group">' + escapeHtml(section.group) + "</p><ul>");
      section.items.forEach(function (item) {
        var active = item.href === page;
        var classes = "hb-sidebar__link";
        if (active) classes += " hb-sidebar__link--active";
        if (item.tone === "alert") classes += " hb-sidebar__link--alert";
        html.push(
          "<li><a href=\"" +
            escapeHtml(item.href) +
            "\" class=\"" +
            classes +
            "\"" +
            (active ? ' aria-current="page"' : "") +
            "><span class=\"hb-sidebar__link-title\">" +
            escapeHtml(item.title) +
            '</span><span class="hb-sidebar__link-desc">' +
            escapeHtml(item.desc) +
            "</span></a></li>"
        );
      });
      html.push("</ul>");
    });

    html.push("</nav>");
    sidebar.innerHTML = html.join("");
  }

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

  function initMobileNav(sidebar) {
    var btn = document.getElementById("hb-nav-toggle");
    var backdrop = document.getElementById("hb-nav-backdrop");
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

  function init() {
    var sidebar = document.getElementById("hb-sidebar");
    if (!sidebar) return;
    renderSidebar(sidebar);
    initMobileNav(sidebar);
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
  } else {
    init();
  }
})();
