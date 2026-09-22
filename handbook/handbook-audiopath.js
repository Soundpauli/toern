/* Audio-path graphs: node/edge data → SVG. */
(function () {
  "use strict";

  var NW = 150;
  var NH = 52;
  var GX = 32;
  var GY = 30;
  var PAD = 20;

  var KINDS = {
    source: { fill: "#fff5f3", stroke: "#d01010", ink: "#7a1010" },
    env: { fill: "#fff6ec", stroke: "#e05810", ink: "#8a3a0a" },
    fx: { fill: "#f3fbff", stroke: "#1a91d6", ink: "#0c5a82" },
    amp: { fill: "#f1faf4", stroke: "#009020", ink: "#0b5a28" },
    mix: { fill: "#f4f2ff", stroke: "#2020b0", ink: "#1a1a6a" },
    bus: { fill: "#f3f5f8", stroke: "#1a2537", ink: "#1a2537" },
    out: { fill: "#eef8ff", stroke: "#2eb9ff", ink: "#0c5a82" },
    meter: { fill: "#fafbfc", stroke: "#5b6878", ink: "#5b6878" },
    mute: { fill: "#fafbfc", stroke: "#a84848", ink: "#7a3030", dash: "5 3" }
  };

  function esc(s) {
    return String(s == null ? "" : s)
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;");
  }

  function nodeBox(n) {
    return {
      x: PAD + n.c * (NW + GX),
      y: PAD + n.r * (NH + GY),
      w: n.w ? n.w * NW + (n.w - 1) * GX : NW,
      h: n.h ? n.h * NH + (n.h - 1) * GY : NH
    };
  }

  function port(box, side) {
    if (side === "l") return { x: box.x, y: box.y + box.h / 2 };
    if (side === "r") return { x: box.x + box.w, y: box.y + box.h / 2 };
    if (side === "t") return { x: box.x + box.w / 2, y: box.y };
    return { x: box.x + box.w / 2, y: box.y + box.h };
  }

  function edgePath(a, b, fromSide, toSide) {
    var p0 = port(a, fromSide || "r");
    var p1 = port(b, toSide || "l");
    var dx = Math.max(18, Math.abs(p1.x - p0.x) * 0.45);
    if (fromSide === "b" && toSide === "t") {
      var mid = (p0.y + p1.y) / 2;
      return "M" + p0.x + " " + p0.y + " C " + p0.x + " " + mid + ", " + p1.x + " " + mid + ", " + p1.x + " " + p1.y;
    }
    return (
      "M" +
      p0.x +
      " " +
      p0.y +
      " C " +
      (p0.x + dx) +
      " " +
      p0.y +
      ", " +
      (p1.x - dx) +
      " " +
      p1.y +
      ", " +
      p1.x +
      " " +
      p1.y
    );
  }

  function renderGraph(el, graph) {
    var map = {};
    var maxC = 0;
    var maxR = 0;
    graph.nodes.forEach(function (n) {
      map[n.id] = n;
      var w = n.w || 1;
      var h = n.h || 1;
      if (n.c + w - 1 > maxC) maxC = n.c + w - 1;
      if (n.r + h - 1 > maxR) maxR = n.r + h - 1;
    });
    var W = PAD * 2 + (maxC + 1) * NW + maxC * GX;
    var H = PAD * 2 + (maxR + 1) * NH + maxR * GY;
    var uid = "ap" + Math.random().toString(36).slice(2, 8);

    var parts = [];
    parts.push(
      '<svg class="ap-svg" viewBox="0 0 ' +
        W +
        " " +
        H +
        '" width="' +
        W +
        '" height="' +
        H +
        '" role="img" aria-label="' +
        esc(graph.title || "Audio graph") +
        '">'
    );
    parts.push(
      "<defs><marker id=\"" +
        uid +
        'arr" viewBox="0 0 10 7" refX="9" refY="3.5" markerWidth="8" markerHeight="6" orient="auto"><path d="M0 0 L10 3.5 L0 7 z" fill="#5b6878"/></marker></defs>'
    );

    graph.edges.forEach(function (e) {
      var a = nodeBox(map[e.from]);
      var b = nodeBox(map[e.to]);
      var d = edgePath(a, b, e.fromSide, e.toSide);
      parts.push(
        '<path class="ap-wire" d="' +
          d +
          '" fill="none" stroke="#5b6878" stroke-width="1.6" marker-end="url(#' +
          uid +
          'arr)"/>'
      );
      if (e.label) {
        var p0 = port(a, e.fromSide || "r");
        var p1 = port(b, e.toSide || "l");
        var lx = (p0.x + p1.x) / 2;
        var ly = (p0.y + p1.y) / 2 - 6;
        parts.push(
          '<text class="ap-elabel" x="' +
            lx +
            '" y="' +
            ly +
            '" text-anchor="middle">' +
            esc(e.label) +
            "</text>"
        );
      }
    });

    graph.nodes.forEach(function (n) {
      var box = nodeBox(n);
      var k = KINDS[n.k] || KINDS.bus;
      var dash = k.dash ? ' stroke-dasharray="' + k.dash + '"' : "";
      parts.push("<g class=\"ap-node ap-node--" + esc(n.k || "bus") + '">');
      parts.push(
        '<rect x="' +
          box.x +
          '" y="' +
          box.y +
          '" width="' +
          box.w +
          '" height="' +
          box.h +
          '" rx="3" fill="' +
          k.fill +
          '" stroke="' +
          k.stroke +
          '" stroke-width="1.6"' +
          dash +
          "/>"
      );
      var cx = box.x + box.w / 2;
      var ty = n.s ? box.y + box.h / 2 - 5 : box.y + box.h / 2 + 4;
      parts.push(
        '<text class="ap-nlabel" x="' +
          cx +
          '" y="' +
          ty +
          '" text-anchor="middle" fill="' +
          k.ink +
          '">' +
          esc(n.l) +
          "</text>"
      );
      if (n.s) {
        parts.push(
          '<text class="ap-nsub" x="' +
            cx +
            '" y="' +
            (box.y + box.h / 2 + 12) +
            '" text-anchor="middle" fill="' +
            k.ink +
            '">' +
            esc(n.s) +
            "</text>"
        );
      }
      parts.push("</g>");
    });

    parts.push("</svg>");
    el.innerHTML = parts.join("");
    el.setAttribute("role", "figure");
    if (graph.title) el.setAttribute("aria-label", graph.title);
  }

  var GRAPHS = {
    overview: {
      title: "System mix",
      nodes: [
        { id: "s14", l: "ch 1–4", s: "mixer1  ·  1/3", k: "bus", c: 0, r: 0 },
        { id: "s58", l: "ch 5–8", s: "mixer2  ·  1/3", k: "bus", c: 0, r: 1 },
        { id: "syn", l: "ch 11 / 13 / 14", s: "mixersynth_end  ·  0.80", k: "bus", c: 0, r: 2 },
        { id: "prev", l: "preview", s: "mixer0", k: "source", c: 0, r: 3 },
        { id: "inp", l: "I2S in", s: "audioInputAmp 1.0", k: "source", c: 0, r: 4 },
        { id: "end", l: "mixer_end", s: "0.50 / 0.50 / SYN / mon", k: "mix", c: 1, r: 1, h: 2 },
        { id: "st", l: "mixer_stereo L/R", s: "VOL → 2-CH", k: "mix", c: 2, r: 1, h: 2 },
        { id: "dac", l: "i2s1", s: "R=0  L=1", k: "out", c: 3, r: 1 },
        { id: "cod", l: "SGTL5000", s: "phones + line", k: "out", c: 4, r: 1 }
      ],
      edges: [
        { from: "s14", to: "end", label: "in0  0.50" },
        { from: "s58", to: "end", label: "in1  0.50" },
        { from: "syn", to: "end", label: "in2  0.60" },
        { from: "inp", to: "end", label: "in3  mon" },
        { from: "end", to: "st", label: "in0" },
        { from: "prev", to: "st", label: "in1" },
        { from: "st", to: "dac" },
        { from: "dac", to: "cod" }
      ]
    },
    samples: {
      title: "Sample voice 1–8",
      nodes: [
        { id: "snd", l: "soundN", s: "amp = vel/127", k: "source", c: 0, r: 1 },
        { id: "env", l: "envelopeN", s: "ADSR", k: "env", c: 1, r: 1 },
        { id: "bc", l: "bitcrusherN", s: "0 = bypass", k: "fx", c: 2, r: 1 },
        { id: "amp", l: "ampN", s: "vol 0–16 → 0–1", k: "amp", c: 3, r: 1 },
        { id: "svf", l: "filterN", s: "SVF LP/BP/HP", k: "fx", c: 4, r: 1 },
        { id: "fm", l: "filtermixerN", s: "in0 LP  in2 HP", k: "mix", c: 5, r: 1 },
        { id: "fv", l: "freeverbN", s: "ch 1,2,5–8", k: "fx", c: 6, r: 0 },
        { id: "fvm", l: "freeverbmixerN", s: "wet 0–0.54  dry 1–0.88", k: "mix", c: 7, r: 0 },
        { id: "m12", l: "mixer1 / mixer2", s: "every in = 1/3", k: "bus", c: 8, r: 1 },
        { id: "m34", l: "mixer1 in2/3", s: "ch 3, 4  no verb", k: "bus", c: 7, r: 2 }
      ],
      edges: [
        { from: "snd", to: "env" },
        { from: "env", to: "bc" },
        { from: "bc", to: "amp" },
        { from: "amp", to: "svf" },
        { from: "svf", to: "fm" },
        { from: "fm", to: "fv", label: "wet" },
        { from: "fv", to: "fvm", label: "in0" },
        { from: "fm", to: "fvm", label: "in3 dry" },
        { from: "fvm", to: "m12" },
        { from: "fm", to: "m34", label: "ch 3+4" }
      ]
    },
    ch11: {
      title: "Channel 11 poly",
      nodes: [
        { id: "o1", l: "Swaveform1", s: "amp 1.0", k: "source", c: 0, r: 0 },
        { id: "o2", l: "Swaveform2", s: "amp 1.0", k: "source", c: 0, r: 1 },
        { id: "o3", l: "Swaveform3", s: "amp 1.0", k: "source", c: 0, r: 2 },
        { id: "ml", l: "Smixer1", s: "L  pan×vol×0.33", k: "mix", c: 1, r: 0 },
        { id: "mr", l: "Smixer2", s: "R  pan×vol×0.33", k: "mix", c: 1, r: 2 },
        { id: "ld", l: "Sladder1/2", s: "filter env in1", k: "fx", c: 2, r: 1 },
        { id: "se", l: "Senvelope", s: "×3 voices", k: "env", c: 3, r: 1 },
        { id: "lr", l: "SmixerL4 / R4", s: "in 0–2  gain 1.0", k: "mix", c: 4, r: 1 },
        { id: "ch", l: "CHMixer11", s: "L 0.50  R 0.50", k: "mix", c: 5, r: 1 },
        { id: "mw", l: "mixer_waveform11", s: "in3=1  in0–2=0", k: "mix", c: 6, r: 1 },
        { id: "dead", l: "waveform11_1/2", s: "muted leftovers", k: "mute", c: 6, r: 0 },
        { id: "bc", l: "bitcrusher11", s: "then amp11", k: "fx", c: 7, r: 1 },
        { id: "svf", l: "filter11", s: "filtermixer11", k: "fx", c: 8, r: 1 },
        { id: "fv", l: "freeverb11", s: "wet send", k: "fx", c: 8, r: 0 },
        { id: "sm", l: "synthmixer11", s: "in0 wet  in3 dry 0.20", k: "mix", c: 9, r: 1 },
        { id: "se2", l: "mixersynth_end", s: "in0 = 0.80", k: "bus", c: 10, r: 1 }
      ],
      edges: [
        { from: "o1", to: "ml" },
        { from: "o1", to: "mr" },
        { from: "o2", to: "ml" },
        { from: "o2", to: "mr" },
        { from: "o3", to: "ml" },
        { from: "o3", to: "mr" },
        { from: "ml", to: "ld" },
        { from: "mr", to: "ld" },
        { from: "ld", to: "se" },
        { from: "se", to: "lr" },
        { from: "lr", to: "ch" },
        { from: "ch", to: "mw", label: "in3" },
        { from: "dead", to: "mw", label: "in0/1 = 0", fromSide: "b", toSide: "t" },
        { from: "mw", to: "bc" },
        { from: "bc", to: "svf" },
        { from: "svf", to: "fv", label: "wet" },
        { from: "fv", to: "sm", label: "in0 ×RVRB" },
        { from: "svf", to: "sm", label: "in3 0.20" },
        { from: "sm", to: "se2" }
      ]
    },
    ch1314: {
      title: "Channels 13–14",
      nodes: [
        { id: "w1", l: "waveformN_1", s: "amp 0.1125", k: "source", c: 0, r: 0 },
        { id: "w2", l: "waveformN_2", s: "amp 0.1125", k: "source", c: 0, r: 2 },
        { id: "mw", l: "mixer_waveform", s: "in0/in1 = 1.0", k: "mix", c: 1, r: 1 },
        { id: "env", l: "envelopeN", s: "VCA", k: "env", c: 2, r: 1 },
        { id: "bc", l: "bitcrusherN", s: "then ampN", k: "fx", c: 3, r: 1 },
        { id: "svf", l: "filterN", s: "filtermixerN", k: "fx", c: 4, r: 1 },
        { id: "sm", l: "synthmixerN", s: "in3 = 0.10 patched", k: "mix", c: 5, r: 1 },
        { id: "se", l: "mixersynth_end", s: "in2 / in3 = 0.80", k: "bus", c: 6, r: 1 }
      ],
      edges: [
        { from: "w1", to: "mw", label: "in0" },
        { from: "w2", to: "mw", label: "in1" },
        { from: "mw", to: "env" },
        { from: "env", to: "bc" },
        { from: "bc", to: "svf" },
        { from: "svf", to: "sm", label: "in3" },
        { from: "sm", to: "se" }
      ]
    },
    preview: {
      title: "Preview",
      nodes: [
        { id: "s0", l: "sound0", s: "RAM / seek", k: "source", c: 0, r: 0 },
        { id: "e0", l: "envelope0", s: "ADSR", k: "env", c: 1, r: 0 },
        { id: "sd", l: "playSdWav1", s: "SD stream", k: "source", c: 0, r: 2 },
        { id: "ap", l: "ampPreview", s: "gain 1.0", k: "amp", c: 1, r: 2 },
        { id: "pk", l: "peak1", s: "UI only", k: "meter", c: 1, r: 3 },
        { id: "m0", l: "mixer0", s: "in0 ×1.25  in1 ×1", k: "mix", c: 2, r: 1 },
        { id: "st", l: "mixer_stereo L/R", s: "in1  PREV 0–0.5", k: "out", c: 3, r: 1 }
      ],
      edges: [
        { from: "s0", to: "e0" },
        { from: "e0", to: "m0", label: "in0" },
        { from: "sd", to: "ap" },
        { from: "ap", to: "m0", label: "in1" },
        { from: "sd", to: "pk" },
        { from: "m0", to: "st" }
      ]
    },
    input: {
      title: "Input / record / monitor",
      nodes: [
        { id: "i2s", l: "audioInput", s: "I2S ADC", k: "source", c: 0, r: 1 },
        { id: "amp", l: "audioInputAmp", s: "gain 1.0", k: "amp", c: 1, r: 1 },
        { id: "q", l: "queue1", s: "record", k: "fx", c: 2, r: 0 },
        { id: "pk", l: "peakRec", s: "REC meter", k: "meter", c: 2, r: 2 },
        { id: "end", l: "mixer_end in3", s: "0 / 0–0.17 / 1.0", k: "bus", c: 2, r: 1 }
      ],
      edges: [
        { from: "i2s", to: "amp" },
        { from: "amp", to: "q" },
        { from: "amp", to: "pk" },
        { from: "amp", to: "end" }
      ]
    },
    stereo: {
      title: "Stereo out",
      nodes: [
        { id: "end", l: "mixer_end", s: "main mix", k: "bus", c: 0, r: 1 },
        { id: "m0", l: "mixer0", s: "preview", k: "source", c: 0, r: 2 },
        { id: "m1", l: "mixer1", s: "ch 1–4", k: "bus", c: 0, r: 0 },
        { id: "m2", l: "mixer2", s: "ch 5–8", k: "bus", c: 0, r: 3 },
        { id: "l", l: "mixer_stereoL", s: "→ i2s1 ch1", k: "mix", c: 1, r: 1 },
        { id: "r", l: "mixer_stereoR", s: "→ i2s1 ch0", k: "mix", c: 1, r: 2 },
        { id: "dac", l: "i2s1", s: "SGTL5000", k: "out", c: 2, r: 1, h: 2 }
      ],
      edges: [
        { from: "end", to: "l", label: "in0" },
        { from: "end", to: "r", label: "in0" },
        { from: "m0", to: "l", label: "in1" },
        { from: "m0", to: "r", label: "in1" },
        { from: "m1", to: "l", label: "in2 L+R" },
        { from: "m2", to: "r", label: "in2 L+R" },
        { from: "l", to: "dac" },
        { from: "r", to: "dac" }
      ]
    }
  };

  function init() {
    document.querySelectorAll("[data-ap-graph]").forEach(function (el) {
      var name = el.getAttribute("data-ap-graph");
      var g = GRAPHS[name];
      if (!g) return;
      renderGraph(el, g);
    });
  }

  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", init);
  else init();
})();
