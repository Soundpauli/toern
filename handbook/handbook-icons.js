(function () {
  /* Compact 16×16 stroke icons (currentColor). */
  var ICONS = {
    welcome:
      '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.4" stroke-linecap="round" stroke-linejoin="round" d="M2.5 7.2 8 2.5l5.5 4.7V14H9.5V10H6.5v4H2.5z"/></svg>',
    hardware:
      '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><rect x="3" y="2.5" width="10" height="11" rx="1" fill="none" stroke="currentColor" stroke-width="1.4"/><circle cx="8" cy="8" r="2.2" fill="none" stroke="currentColor" stroke-width="1.3"/><path fill="none" stroke="currentColor" stroke-width="1.3" stroke-linecap="round" d="M8 2.5v1.4M8 12.1V13.5M3 8h1.4M11.6 8H13"/></svg>',
    matrix:
      '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.3" d="M2.5 2.5h11v11h-11zM2.5 6.2h11M2.5 9.8h11M6.2 2.5v11M9.8 2.5v11"/></svg>',
    controls:
      '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><circle cx="8" cy="8" r="5.2" fill="none" stroke="currentColor" stroke-width="1.4"/><circle cx="8" cy="8" r="1.5" fill="currentColor"/><path fill="none" stroke="currentColor" stroke-width="1.3" stroke-linecap="round" d="M8 2.8v1.6M8 11.6v1.6M2.8 8h1.6M11.6 8h1.6"/></svg>',
    alert:
      '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.4" stroke-linejoin="round" d="M8 2.4 14.2 13.2H1.8z"/><path fill="none" stroke="currentColor" stroke-width="1.4" stroke-linecap="round" d="M8 6.2v3.2M8 11.2v.1"/></svg>',
    sequencing:
      '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.4" stroke-linecap="round" d="M2.5 11.5h2.2M6.2 8.2h2.2M9.9 4.8h2.2M13.5 4.8v6.7"/><circle cx="3.6" cy="11.5" r="1.1" fill="currentColor"/><circle cx="7.3" cy="8.2" r="1.1" fill="currentColor"/><circle cx="11" cy="4.8" r="1.1" fill="currentColor"/></svg>',
    pages:
      '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.4" d="M3 3.2h7.5v9.6H3z"/><path fill="none" stroke="currentColor" stroke-width="1.3" d="M5.5 3.2V12.8M8 3.2V12.8M10.5 3.2V12.8"/><path fill="none" stroke="currentColor" stroke-width="1.3" stroke-linecap="round" d="M12.2 5.5h1.3v7.3H5.8"/></svg>',
    advanced:
      '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.4" stroke-linecap="round" d="M2.5 11.5h2.2M6.2 8.2h2.2M9.9 4.8h2.2"/><circle cx="3.6" cy="11.5" r="1.1" fill="currentColor"/><circle cx="7.3" cy="8.2" r="1.1" fill="currentColor"/><circle cx="11" cy="4.8" r="1.1" fill="currentColor"/><path fill="none" stroke="currentColor" stroke-width="1.3" stroke-linecap="round" d="M13.2 3.2v4.2M11.1 5.3h4.2"/></svg>',
    note:
      '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.4" stroke-linecap="round" stroke-linejoin="round" d="M6.2 12.2V4.2l6.1-1.4v8"/><circle cx="4.6" cy="12.2" r="1.7" fill="none" stroke="currentColor" stroke-width="1.3"/><circle cx="10.7" cy="10.8" r="1.7" fill="none" stroke="currentColor" stroke-width="1.3"/></svg>',
    sound:
      '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.4" stroke-linecap="round" d="M2.5 8h1.4l1.5-3.2 1.8 6.4 1.7-4.6 1.4 2.6H13.5"/></svg>',
    samples:
      '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><circle cx="8" cy="8" r="5.4" fill="none" stroke="currentColor" stroke-width="1.4"/><circle cx="8" cy="8" r="1.6" fill="none" stroke="currentColor" stroke-width="1.3"/><path fill="none" stroke="currentColor" stroke-width="1.3" stroke-linecap="round" d="M8 2.6v1.5"/></svg>',
    record:
      '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><circle cx="8" cy="8" r="5.2" fill="none" stroke="currentColor" stroke-width="1.4"/><circle cx="8" cy="8" r="2.4" fill="currentColor"/></svg>',
    packs:
      '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.4" stroke-linejoin="round" d="M2.5 5.2h4.2l1.3 1.4H13.5v6.4H2.5z"/><path fill="none" stroke="currentColor" stroke-width="1.3" d="M2.5 5.2V3.8h3.6l.9 1.4"/></svg>',
    tempo:
      '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><circle cx="8" cy="8.2" r="5.2" fill="none" stroke="currentColor" stroke-width="1.4"/><path fill="none" stroke="currentColor" stroke-width="1.4" stroke-linecap="round" d="M8 5v3.4l2.4 1.5"/></svg>',
    play:
      '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.4" stroke-linejoin="round" d="M4.2 3.2v9.6L13 8z"/></svg>',
    midi:
      '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><circle cx="4.2" cy="8" r="1.6" fill="none" stroke="currentColor" stroke-width="1.3"/><circle cx="11.8" cy="8" r="1.6" fill="none" stroke="currentColor" stroke-width="1.3"/><path fill="none" stroke="currentColor" stroke-width="1.3" stroke-linecap="round" d="M5.8 8h4.4M4.2 5.2v1.2M4.2 9.6v1.2M11.8 5.2v1.2M11.8 9.6v1.2"/></svg>',
    song:
      '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.4" stroke-linecap="round" d="M3 4.2h10M3 8h10M3 11.8h7"/></svg>',
    files:
      '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.4" stroke-linejoin="round" d="M4 2.8h5.2L12.5 6v7.2H4z"/><path fill="none" stroke="currentColor" stroke-width="1.3" d="M9.2 2.8V6h3.3"/></svg>',
    menus:
      '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.4" stroke-linecap="round" d="M3 4h10M3 8h10M3 12h10"/></svg>',
    reference:
      '<svg class="hb-ico" viewBox="0 0 16 16" aria-hidden="true"><rect x="2.5" y="3" width="5" height="4.2" fill="none" stroke="currentColor" stroke-width="1.3"/><rect x="8.5" y="3" width="5" height="4.2" fill="none" stroke="currentColor" stroke-width="1.3"/><rect x="2.5" y="8.8" width="5" height="4.2" fill="none" stroke="currentColor" stroke-width="1.3"/><rect x="8.5" y="8.8" width="5" height="4.2" fill="none" stroke="currentColor" stroke-width="1.3"/></svg>',
    encoder:
      '<svg class="hb-ico hb-ico--inline hb-ico--encoder" viewBox="0 0 16 16" aria-hidden="true"><circle cx="8" cy="8" r="5.1" fill="none" stroke="currentColor" stroke-width="1.35"/><circle cx="8" cy="8" r="1.45" fill="currentColor"/><path fill="none" stroke="currentColor" stroke-width="1.25" stroke-linecap="round" d="M8 2.9v1.45M8 11.65v1.45M2.9 8h1.45M11.65 8h1.45M4.2 4.2l1 1M10.8 10.8l1 1M11.8 4.2l-1 1M5.2 10.8l-1 1"/></svg>',
    touch:
      '<svg class="hb-ico hb-ico--inline hb-ico--touch" viewBox="0 0 16 16" aria-hidden="true"><path fill="none" stroke="currentColor" stroke-width="1.3" stroke-linecap="round" stroke-linejoin="round" d="M7.1 9.6V4.35a1.15 1.15 0 0 1 2.3 0V8.2"/><path fill="none" stroke="currentColor" stroke-width="1.3" stroke-linecap="round" stroke-linejoin="round" d="M9.4 8.35V6.7a1 1 0 0 1 2 0v2.55"/><path fill="none" stroke="currentColor" stroke-width="1.3" stroke-linecap="round" stroke-linejoin="round" d="M11.4 9.05V7.85a.95.95 0 0 1 1.9 0v2.7c0 2.15-1.45 3.55-3.7 3.55h-.35c-1.7 0-2.85-.7-3.55-1.55L4.2 9.9a1.05 1.05 0 0 1 1.55-1.4l1.35 1.1"/></svg>'
  };

  window.HBIcons = ICONS;

  var SKIP = {
    SCRIPT: 1,
    STYLE: 1,
    NOSCRIPT: 1,
    SVG: 1,
    CODE: 1,
    PRE: 1,
    TEXTAREA: 1,
    KBD: 1,
    CANVAS: 1
  };

  function iconMarkup(kind) {
    return ICONS[kind] || "";
  }

  function wrapMatch(text, kind) {
    return iconMarkup(kind) + text;
  }

  function decorateTextNode(node) {
    var text = node.nodeValue;
    if (!text || !/[Ee]ncoder|[Tt]ouch/.test(text)) return;

    var re = /\b([Ee]ncoders?)\b|\b([Tt]ouch)(?:[\s\u00a0]*)([123]|pads?|buttons?)\b/g;
    if (!re.test(text)) return;
    re.lastIndex = 0;

    var frag = document.createDocumentFragment();
    var last = 0;
    var m;
    while ((m = re.exec(text))) {
      if (m.index > last) {
        frag.appendChild(document.createTextNode(text.slice(last, m.index)));
      }
      var wrap = document.createElement("span");
      wrap.className = "hb-mention";
      if (m[1]) {
        wrap.className += " hb-mention--encoder";
        wrap.innerHTML = wrapMatch(m[1], "encoder");
      } else {
        wrap.className += " hb-mention--touch";
        wrap.innerHTML = wrapMatch(m[0], "touch");
      }
      frag.appendChild(wrap);
      last = m.index + m[0].length;
    }
    if (last < text.length) {
      frag.appendChild(document.createTextNode(text.slice(last)));
    }
    node.parentNode.replaceChild(frag, node);
  }

  function shouldSkip(node) {
    var el = node.parentElement;
    while (el) {
      if (SKIP[el.tagName]) return true;
      if (el.classList && (el.classList.contains("hb-mention") || el.classList.contains("hb-ico") || el.classList.contains("hb-sidebar"))) {
        return true;
      }
      el = el.parentElement;
    }
    return false;
  }

  function decorateContent() {
    var root = document.getElementById("hb-content") || document.body;
    var walker = document.createTreeWalker(root, NodeFilter.SHOW_TEXT, null);
    var nodes = [];
    var n;
    while ((n = walker.nextNode())) {
      if (!shouldSkip(n) && /[Ee]ncoder|[Tt]ouch/.test(n.nodeValue || "")) nodes.push(n);
    }
    nodes.forEach(decorateTextNode);
  }

  function init() {
    decorateContent();
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
  } else {
    init();
  }
})();
