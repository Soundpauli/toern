(function () {
  var INDEX_KEY = "hb-search-index-v2-" + (window.HBLanguage ? window.HBLanguage.id : "en");
  var INDEX_VER = "20261016w";

  function pages() {
    return (window.HBHandbook && window.HBHandbook.pages) || [];
  }

  function escapeHtml(s) {
    return String(s)
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;");
  }

  function normalize(s) {
    return String(s || "")
      .replace(/\u0152/g, "OE")
      .replace(/\u0153/g, "oe")
      .toLowerCase()
      .replace(/[^\S\n]+/g, " ")
      .trim();
  }

  function tokens(q) {
    return normalize(q)
      .split(/[^a-z0-9]+/)
      .filter(function (t) {
        return t.length > 1;
      });
  }

  function extractDoc(html, meta) {
    var doc = new DOMParser().parseFromString(html, "text/html");
    if (window.HBLanguage) window.HBLanguage.translate(doc, meta.href);
    var root = doc.querySelector("#hb-content .wrap") || doc.querySelector(".wrap") || doc.body;
    if (!root) return null;

    root.querySelectorAll("script, style, nav.hb-pager, .hb-chapter-meta, .hb-cover__matrix").forEach(function (el) {
      el.remove();
    });

    var title =
      (doc.querySelector(".handbook-page__title") && doc.querySelector(".handbook-page__title").textContent) ||
      meta.title;
    var summary =
      (doc.querySelector(".handbook-page__summary") && doc.querySelector(".handbook-page__summary").textContent) ||
      meta.desc ||
      "";

    var sections = [];
    root.querySelectorAll("section[id], h2[id], h3[id], h4[id]").forEach(function (el) {
      var id = el.id;
      if (!id || id.indexOf("page-") === 0) return;
      var heading =
        (el.matches("h2, h3, h4") && el.textContent) ||
        (el.querySelector("h2, h3, h4, .hb-subsection-heading") &&
          el.querySelector("h2, h3, h4, .hb-subsection-heading").textContent) ||
        "";
      var text = normalize(el.textContent || "");
      if (text.length < 20) return;
      sections.push({ id: id, heading: normalize(heading), text: text });
    });

    var body = normalize(root.textContent || "");
    return {
      href: meta.href,
      title: (title || meta.title || "").trim(),
      desc: (summary || meta.desc || "").trim(),
      group: meta.group,
      body: body,
      sections: sections
    };
  }

  function loadCached() {
    try {
      var raw = sessionStorage.getItem(INDEX_KEY);
      if (!raw) return null;
      var data = JSON.parse(raw);
      if (!data || data.ver !== INDEX_VER || !Array.isArray(data.docs)) return null;
      return data.docs;
    } catch (e) {
      return null;
    }
  }

  function saveCached(docs) {
    try {
      sessionStorage.setItem(INDEX_KEY, JSON.stringify({ ver: INDEX_VER, docs: docs }));
    } catch (e) {}
  }

  function buildIndex() {
    var list = pages().filter(function (p) {
      return p.href && p.href !== "search.html";
    });
    return Promise.all(
      list.map(function (meta) {
        return fetch(meta.href, { credentials: "same-origin" })
          .then(function (r) {
            if (!r.ok) throw new Error(String(r.status));
            return r.text();
          })
          .then(function (html) {
            return extractDoc(html, meta);
          })
          .catch(function () {
            return {
              href: meta.href,
              title: meta.title,
              desc: meta.desc || "",
              group: meta.group,
              body: normalize((meta.title || "") + " " + (meta.desc || "")),
              sections: []
            };
          });
      })
    ).then(function (docs) {
      docs = docs.filter(Boolean);
      saveCached(docs);
      return docs;
    });
  }

  function ensureIndex() {
    var cached = loadCached();
    if (cached) return Promise.resolve(cached);
    return buildIndex();
  }

  function countMatches(hay, term) {
    if (!hay || !term) return 0;
    var n = 0;
    var from = 0;
    while (true) {
      var i = hay.indexOf(term, from);
      if (i === -1) break;
      n++;
      from = i + term.length;
    }
    return n;
  }

  function bestSection(doc, terms) {
    var best = null;
    var bestScore = 0;
    (doc.sections || []).forEach(function (sec) {
      var score = 0;
      terms.forEach(function (t) {
        score += countMatches(sec.heading, t) * 8;
        score += countMatches(sec.text, t);
      });
      if (score > bestScore) {
        bestScore = score;
        best = sec;
      }
    });
    return best;
  }

  function snippetFrom(text, terms, width) {
    width = width || 160;
    var hay = text || "";
    var hit = -1;
    var term = "";
    for (var i = 0; i < terms.length; i++) {
      var idx = hay.indexOf(terms[i]);
      if (idx !== -1 && (hit === -1 || idx < hit)) {
        hit = idx;
        term = terms[i];
      }
    }
    if (hit === -1) {
      return hay.slice(0, width).trim() + (hay.length > width ? "…" : "");
    }
    var start = Math.max(0, hit - Math.floor(width / 3));
    var end = Math.min(hay.length, start + width);
    var snip = hay.slice(start, end).trim();
    if (start > 0) snip = "…" + snip;
    if (end < hay.length) snip += "…";
    return snip;
  }

  function highlight(text, terms) {
    var out = escapeHtml(text);
    terms
      .slice()
      .sort(function (a, b) {
        return b.length - a.length;
      })
      .forEach(function (t) {
        var re = new RegExp("(" + t.replace(/[.*+?^${}()|[\]\\]/g, "\\$&") + ")", "gi");
        out = out.replace(re, "<mark>$1</mark>");
      });
    return out;
  }

  function searchDocs(docs, query) {
    var terms = tokens(query);
    if (!terms.length) return [];

    return docs
      .map(function (doc) {
        var titleN = normalize(doc.title);
        var descN = normalize(doc.desc);
        var bodyN = doc.body || "";
        var score = 0;
        var matchedAll = true;

        terms.forEach(function (t) {
          var inTitle = titleN.indexOf(t) !== -1;
          var inDesc = descN.indexOf(t) !== -1;
          var inBody = bodyN.indexOf(t) !== -1;
          if (!inTitle && !inDesc && !inBody) matchedAll = false;
          if (inTitle) score += 40 + countMatches(titleN, t) * 10;
          if (inDesc) score += 12 + countMatches(descN, t) * 3;
          if (inBody) score += 2 + Math.min(20, countMatches(bodyN, t));
        });

        if (!matchedAll || score <= 0) return null;

        var sec = bestSection(doc, terms);
        var href = doc.href + (sec && sec.id ? "#" + sec.id : "");
        var snipSource = (sec && sec.text) || bodyN || descN;
        return {
          href: href,
          title: doc.title,
          group: doc.group,
          score: score,
          snippet: snippetFrom(snipSource, terms),
          section: sec && sec.heading ? sec.heading : ""
        };
      })
      .filter(Boolean)
      .sort(function (a, b) {
        return b.score - a.score || a.title.localeCompare(b.title);
      });
  }

  function renderResults(host, query, results, status) {
    var terms = tokens(query);
    var html = [];

    if (status === "loading") {
      host.innerHTML = '<p class="hb-search-status">Indexing chapters…</p>';
      return;
    }
    if (status === "error") {
      host.innerHTML = '<p class="hb-search-status hb-search-status--err">Search could not load chapter text. Try again.</p>';
      return;
    }
    if (!query.trim()) {
      host.innerHTML =
        '<p class="hb-search-status">Type a word or phrase — pages, menus, gestures, and encoder tables are all searchable.</p>';
      return;
    }
    if (!results.length) {
      host.innerHTML =
        '<p class="hb-search-status">No matches for <strong>' +
        escapeHtml(query.trim()) +
        "</strong>. Try a shorter word, a menu name, or a control (e.g. encoder, mute, BPM).</p>";
      return;
    }

    html.push(
      '<p class="hb-search-status"><strong>' +
        results.length +
        "</strong> result" +
        (results.length === 1 ? "" : "s") +
        " for <strong>" +
        escapeHtml(query.trim()) +
        "</strong></p>"
    );
    html.push('<ol class="hb-search-list">');
    results.forEach(function (r) {
      html.push('<li class="hb-search-hit">');
      html.push('<a class="hb-search-hit__link" href="' + escapeHtml(window.HBLanguage ? window.HBLanguage.url(r.href) : r.href) + '">');
      if (r.group) html.push('<span class="hb-search-hit__group">' + escapeHtml(r.group) + "</span>");
      html.push('<span class="hb-search-hit__title">' + highlight(r.title, terms) + "</span>");
      if (r.section) html.push('<span class="hb-search-hit__section">' + highlight(r.section, terms) + "</span>");
      html.push('<span class="hb-search-hit__snip">' + highlight(r.snippet, terms) + "</span>");
      html.push("</a></li>");
    });
    html.push("</ol>");
    host.innerHTML = html.join("");
  }

  function queryFromUrl() {
    try {
      return new URLSearchParams(location.search).get("q") || "";
    } catch (e) {
      return "";
    }
  }

  function initAskForm(form, input) {
    if (!form || !input) return;
    var q = queryFromUrl();
    if (q && !input.value) input.value = q;
    input.addEventListener("keydown", function (e) {
      if (e.key === "Enter" && !e.shiftKey) {
        e.preventDefault();
        form.requestSubmit ? form.requestSubmit() : form.submit();
      }
    });
  }

  function run() {
    var form = document.getElementById("hb-ask-form");
    var input = document.getElementById("hb-ask-q");
    var host = document.getElementById("hb-search-results");
    if (!host) return;

    initAskForm(form, input);
    var query = queryFromUrl();
    if (input && query) input.value = query;

    if (!query.trim()) {
      renderResults(host, "", [], "ready");
      if (input) input.focus();
      return;
    }

    renderResults(host, query, [], "loading");
    (window.HBLanguage ? window.HBLanguage.ready : Promise.resolve())
      .then(ensureIndex)
      .then(function (docs) {
        renderResults(host, query, searchDocs(docs, query), "ready");
      })
      .catch(function () {
        renderResults(host, query, [], "error");
      });
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", run);
  } else {
    run();
  }
})();
