/* English stays in the HTML. Other languages replace explicitly keyed copy only. */
(function () {
  "use strict";
  var DEFAULT_LANGUAGE = "fun";
  var LANGUAGES = [
    { id: "fun", label: "fun", language: "en", file: "locales/fun.json" },
    { id: "en", label: "English", language: "en" }
  ];
  var STORAGE_KEY = "toern-handbook-language";
  var requested = new URLSearchParams(location.search).get("lang");
  var saved = null;
  try { saved = localStorage.getItem(STORAGE_KEY); } catch (_) {}
  // Keep previously shared links and saved preferences working after the rename.
  function normalizeLanguage(id) { return id === "douglas" ? "fun" : id; }
  var choice = normalizeLanguage(requested || saved);
  var selected = LANGUAGES.find(function (item) { return item.id === choice; })
    || LANGUAGES.find(function (item) { return item.id === DEFAULT_LANGUAGE; });
  if (requested || saved === "douglas") {
    try { localStorage.setItem(STORAGE_KEY, selected.id); } catch (_) {}
  }
  if (requested === "douglas") {
    var canonical = new URL(location.href);
    canonical.searchParams.set("lang", "fun");
    history.replaceState(history.state, "", canonical.href);
  }
  var bundle = { pages: {} };
  var failed = false;
  var page = location.pathname.split("/").pop() || "index.html";
  var ready = selected.file
    ? fetch(selected.file + "?v=20261016x", { credentials: "same-origin" })
      .then(function (response) {
        if (!response.ok) throw new Error("Language unavailable");
        return response.json();
      })
      .then(function (data) { bundle = data; })
      .catch(function () { failed = true; })
    : Promise.resolve();

  function translate(doc, filename) {
    var copy = (bundle.pages || {})[filename] || {};
    doc.querySelectorAll("[data-hb-copy]").forEach(function (element) {
      var value = copy[element.getAttribute("data-hb-copy")];
      if (typeof value === "string") element.innerHTML = value;
    });
  }

  function localizedUrl(href) {
    var url = new URL(href, location.href);
    if (url.origin === location.origin && /(?:\/|\.html)$/.test(url.pathname)) {
      // Explicit English also overrides a different preference in another tab.
      url.searchParams.set("lang", selected.id);
    }
    return url.href;
  }

  function refreshLinks() {
    document.querySelectorAll("a[href]").forEach(function (link) {
      var href = link.getAttribute("href");
      if (!href || href.charAt(0) === "#") return;
      link.href = localizedUrl(href);
    });
  }

  function status() {
    var note = document.querySelector(".hb-language__note");
    if (!note) return;
    note.textContent = failed ? "Language unavailable. Showing English."
      : selected.id !== "en" && page !== "search.html" && !(bundle.pages || {})[page]
        ? "This chapter is available in English." : "";
    note.hidden = !note.textContent;
  }

  function mount(parent) {
    if (!parent || parent.querySelector("select")) return;
    var label = document.createElement("label");
    label.htmlFor = "hb-language";
    label.textContent = "Language";
    var select = document.createElement("select");
    select.id = "hb-language";
    LANGUAGES.forEach(function (item) {
      var option = document.createElement("option");
      option.value = item.id;
      option.textContent = item.label;
      select.appendChild(option);
    });
    select.value = selected.id;
    select.addEventListener("change", function () {
      var url = new URL(location.href);
      url.searchParams.set("lang", select.value);
      // A full navigation restores the English source before applying another language.
      location.assign(url.href);
    });
    var note = document.createElement("p");
    note.className = "hb-language__note";
    note.setAttribute("role", "status");
    note.hidden = true;
    parent.appendChild(label);
    parent.appendChild(select);
    parent.appendChild(note);
    ready.then(status);
  }

  window.HBLanguage = {
    id: selected.id,
    ready: ready,
    mount: mount,
    translate: translate,
    url: localizedUrl,
    refreshLinks: refreshLinks
  };

  document.addEventListener("submit", function (event) {
    var form = event.target;
    if (!(form instanceof HTMLFormElement) || form.method.toLowerCase() !== "get") return;
    var action = new URL(form.action, location.href);
    if (action.origin !== location.origin) return;
    var field = form.querySelector('input[name="lang"]');
    if (!field) {
      field = document.createElement("input");
      field.type = "hidden";
      field.name = "lang";
      form.appendChild(field);
    }
    field.value = selected.id;
  }, true);

  document.addEventListener("DOMContentLoaded", function () {
    ready.then(function () {
      translate(document, page);
      document.documentElement.lang = failed ? "en" : (bundle.language || selected.language);
      document.documentElement.dataset.hbLanguage = selected.id;
      if (window.HBLayout && window.HBLayout.refresh) window.HBLayout.refresh();
      refreshLinks();
      status();
    });
  });
})();
