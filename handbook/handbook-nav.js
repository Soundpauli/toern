(function () {
  var NAV = [
    {
      group: "Get started",
      items: [
        { href: "index.html", title: "Welcome", desc: "First boot", icon: "welcome" },
        { href: "hardware.html", title: "Hardware", desc: "Ports & buttons", icon: "hardware" },
        { href: "matrix.html", title: "The matrix", desc: "Rows & voices", icon: "matrix" },
        { href: "controls.html", title: "Controls", desc: "Encoders & touch", icon: "controls" },
        { href: "troubleshooting.html", title: "Troubleshoot", desc: "Fix surprises", tone: "alert", icon: "alert" }
      ]
    },
    {
      group: "Compose",
      items: [
        { href: "sequencing.html", title: "Sequencing", desc: "Draw, triggers, single", icon: "sequencing" },
        { href: "pages-more.html", title: "Pages+More", desc: "Page row, delete pages", icon: "pages" },
        { href: "advanced-sequencing.html", title: "Advanced Sequencing", desc: "Mutes, shift, copy", icon: "advanced" },
        { href: "samples.html", title: "Samples", desc: "Sample browser", icon: "samples" },
        { href: "sound.html", title: "Sound design", desc: "Filters & ADSR", icon: "sound" },
        { href: "record.html", title: "Record", desc: "Capture & REC menu", icon: "record" },
        { href: "note-detail.html", title: "Note detail", desc: "Per-step edit", icon: "note" }
      ]
    },
    {
      group: "Operate",
      items: [
        { href: "files.html", title: "Files", desc: "Load, save, NEW", icon: "files" },
        { href: "sample-packs.html", title: "Sample packs", desc: "Pack slots & folders", icon: "packs" },
        { href: "tempo-midi.html", title: "Tempo", desc: "BPM & clock", icon: "tempo" },
        { href: "play.html", title: "Settings", desc: "Playing Attributes", icon: "play" },
        { href: "midi.html", title: "MIDI", desc: "Notes, sync, PPQN", icon: "midi" },
        { href: "song.html", title: "Song", desc: "Arranger & pages", icon: "song" },
        { href: "menus.html", title: "Menu map", desc: "Full tree", icon: "menus" },
        { href: "reference.html", title: "Screen index", desc: "All displays", icon: "reference" }
      ]
    }
  ];

  /* Fallback icons if handbook-icons.js is not yet available (sync load order). */
  var FALLBACK = {
    welcome: '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.4" stroke-linecap="round" stroke-linejoin="round" d="M2.5 7.2 8 2.5l5.5 4.7V14H9.5V10H6.5v4H2.5z"/></svg>',
    hardware: '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><rect x="3" y="2.5" width="10" height="11" rx="1" fill="none" stroke="currentColor" stroke-width="1.4"/><circle cx="8" cy="8" r="2.2" fill="none" stroke="currentColor" stroke-width="1.3"/></svg>',
    matrix: '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.3" d="M2.5 2.5h11v11h-11zM2.5 6.2h11M2.5 9.8h11M6.2 2.5v11M9.8 2.5v11"/></svg>',
    controls: '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><circle cx="8" cy="8" r="5.2" fill="none" stroke="currentColor" stroke-width="1.4"/><circle cx="8" cy="8" r="1.5" fill="currentColor"/></svg>',
    alert: '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.4" stroke-linejoin="round" d="M8 2.4 14.2 13.2H1.8z"/><path fill="none" stroke="currentColor" stroke-width="1.4" stroke-linecap="round" d="M8 6.2v3.2M8 11.2v.1"/></svg>',
    sequencing: '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.4" stroke-linecap="round" d="M2.5 11.5h2.2M6.2 8.2h2.2M9.9 4.8h2.2"/><circle cx="3.6" cy="11.5" r="1.1" fill="currentColor"/><circle cx="7.3" cy="8.2" r="1.1" fill="currentColor"/><circle cx="11" cy="4.8" r="1.1" fill="currentColor"/></svg>',
    pages: '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.4" d="M3 3.2h7.5v9.6H3z"/><path fill="none" stroke="currentColor" stroke-width="1.3" d="M5.5 3.2V12.8M8 3.2V12.8M10.5 3.2V12.8"/><path fill="none" stroke="currentColor" stroke-width="1.3" stroke-linecap="round" d="M12.2 5.5h1.3v7.3H5.8"/></svg>',
    advanced: '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.4" stroke-linecap="round" d="M2.5 11.5h2.2M6.2 8.2h2.2M9.9 4.8h2.2"/><circle cx="3.6" cy="11.5" r="1.1" fill="currentColor"/><circle cx="7.3" cy="8.2" r="1.1" fill="currentColor"/><circle cx="11" cy="4.8" r="1.1" fill="currentColor"/><path fill="none" stroke="currentColor" stroke-width="1.3" stroke-linecap="round" d="M13.2 3.2v4.2M11.1 5.3h4.2"/></svg>',
    note: '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.4" stroke-linecap="round" stroke-linejoin="round" d="M6.2 12.2V4.2l6.1-1.4v8"/><circle cx="4.6" cy="12.2" r="1.7" fill="none" stroke="currentColor" stroke-width="1.3"/><circle cx="10.7" cy="10.8" r="1.7" fill="none" stroke="currentColor" stroke-width="1.3"/></svg>',
    sound: '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.4" stroke-linecap="round" d="M2.5 8h1.4l1.5-3.2 1.8 6.4 1.7-4.6 1.4 2.6H13.5"/></svg>',
    samples: '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><circle cx="8" cy="8" r="5.4" fill="none" stroke="currentColor" stroke-width="1.4"/><circle cx="8" cy="8" r="1.6" fill="none" stroke="currentColor" stroke-width="1.3"/></svg>',
    record: '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><circle cx="8" cy="8" r="5.2" fill="none" stroke="currentColor" stroke-width="1.4"/><circle cx="8" cy="8" r="2.4" fill="currentColor"/></svg>',
    packs: '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.4" stroke-linejoin="round" d="M2.5 5.2h4.2l1.3 1.4H13.5v6.4H2.5z"/></svg>',
    tempo: '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><circle cx="8" cy="8.2" r="5.2" fill="none" stroke="currentColor" stroke-width="1.4"/><path fill="none" stroke="currentColor" stroke-width="1.4" stroke-linecap="round" d="M8 5v3.4l2.4 1.5"/></svg>',
    play: '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.4" stroke-linejoin="round" d="M4.2 3.2v9.6L13 8z"/></svg>',
    midi: '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><circle cx="4.2" cy="8" r="1.6" fill="none" stroke="currentColor" stroke-width="1.3"/><circle cx="11.8" cy="8" r="1.6" fill="none" stroke="currentColor" stroke-width="1.3"/><path fill="none" stroke="currentColor" stroke-width="1.3" stroke-linecap="round" d="M5.8 8h4.4"/></svg>',
    song: '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.4" stroke-linecap="round" d="M3 4.2h10M3 8h10M3 11.8h7"/></svg>',
    files: '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.4" stroke-linejoin="round" d="M4 2.8h5.2L12.5 6v7.2H4z"/></svg>',
    menus: '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.4" stroke-linecap="round" d="M3 4h10M3 8h10M3 12h10"/></svg>',
    reference: '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><rect x="2.5" y="3" width="5" height="4.2" fill="none" stroke="currentColor" stroke-width="1.3"/><rect x="8.5" y="3" width="5" height="4.2" fill="none" stroke="currentColor" stroke-width="1.3"/><rect x="2.5" y="8.8" width="5" height="4.2" fill="none" stroke="currentColor" stroke-width="1.3"/><rect x="8.5" y="8.8" width="5" height="4.2" fill="none" stroke="currentColor" stroke-width="1.3"/></svg>'
  };

  function iconHtml(name) {
    var pack = window.HBIcons || FALLBACK;
    return pack[name] || FALLBACK[name] || "";
  }

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

  function sectionTone(group) {
    if (group === "Get started") return "start";
    if (group === "Compose") return "compose";
    if (group === "Operate") return "operate";
    return "start";
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
      var tone = sectionTone(section.group);
      var sectionClasses = "hb-sidebar__section hb-sidebar__section--" + tone;

      html.push('<div class="' + sectionClasses + '">');
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
            '><span class="hb-sidebar__link-ico" aria-hidden="true">' +
            iconHtml(item.icon) +
            '</span><span class="hb-sidebar__link-text"><span class="hb-sidebar__link-title">' +
            escapeHtml(item.title) +
            '</span><span class="hb-sidebar__link-desc">' +
            escapeHtml(item.desc) +
            "</span></span></a></li>"
        );
      });
      html.push("</ul></div>");
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
