import { searchBitMidi, sortBitMidi, downloadBitMidi } from "./firmware/bitMidi.js";

export function createBitMidiBrowser(root, onLoad, onSelect = () => {}) {
  const form = root.querySelector("form");
  const input = root.querySelector("input");
  const sort = root.querySelector("select");
  const status = root.querySelector("[role=status]");
  const list = root.querySelector("ul");
  const more = root.querySelector("[data-more]");
  let items = [], query = "", page = -1, pages = 0, total = 0;
  let searchController, loadController;
  let searching = false, loadingId = null;

  function cancelLoad() {
    loadController?.abort();
    loadController = null;
    loadingId = null;
    render();
  }

  function render() {
    list.replaceChildren();
    for (const item of sortBitMidi(items, sort.value)) {
      const row = document.createElement("li");
      const title = document.createElement("span");
      title.className = "bitmidi-name";
      title.textContent = item.name.replace(/\.midi?$/i, "");
      title.title = item.name;
      const count = document.createElement("small");
      count.textContent = `${item.plays.toLocaleString()} plays`;
      const load = document.createElement("button");
      load.type = "button";
      load.textContent = item.id === loadingId ? "Loading…" : "Load";
      load.disabled = item.id === loadingId;
      load.setAttribute("aria-label", `Load ${item.name}`);
      load.addEventListener("click", () => select(item));
      row.append(title, count, load);
      list.append(row);
    }
    more.hidden = page < 0 || page + 1 >= pages;
    more.disabled = searching;
    more.textContent = searching ? "Searching…" : "More results";
    list.setAttribute("aria-busy", String(searching));
  }

  async function search(append = false) {
    const nextQuery = append ? query : input.value.trim();
    if (!nextQuery) { input.focus(); return; }
    searchController?.abort();
    cancelLoad();
    const controller = searchController = new AbortController();
    const nextPage = append ? page + 1 : 0;
    if (!append) { items = []; page = -1; pages = 0; total = 0; query = nextQuery; }
    searching = true;
    status.textContent = "Searching BitMidi…";
    render();
    try {
      const result = await searchBitMidi(nextQuery, nextPage, controller.signal);
      if (controller.signal.aborted) return;
      const seen = new Set(items.map(item => item.id));
      items.push(...result.items.filter(item => !seen.has(item.id)));
      page = nextPage; pages = result.pageTotal; total = result.total;
      status.textContent = total ? `${items.length} of ${total.toLocaleString()} results` : "No matches. Try a different artist or title.";
    } catch (err) {
      if (!controller.signal.aborted) status.textContent = err.message || "Could not search BitMidi. Try again.";
    } finally {
      if (searchController === controller) { searching = false; render(); }
    }
  }

  async function select(item) {
    cancelLoad();
    onSelect();
    const controller = loadController = new AbortController();
    loadingId = item.id;
    status.textContent = `Loading ${item.name}…`;
    render();
    try {
      const file = await downloadBitMidi(item, controller.signal);
      if (controller.signal.aborted) return;
      const loaded = await onLoad(file, controller.signal);
      if (controller.signal.aborted) return;
      status.textContent = loaded ? `Loaded ${item.name}. Adjust the tracks below.` : "Could not import this arrangement. Try another result.";
      if (loaded) root.open = false;
    } catch (err) {
      if (!controller.signal.aborted) status.textContent = err.message || "Could not load this MIDI. Try again.";
    } finally {
      if (loadController === controller) { loadController = null; loadingId = null; render(); }
    }
  }

  form.addEventListener("submit", event => { event.preventDefault(); search(); });
  sort.addEventListener("change", render);
  more.addEventListener("click", () => search(true));
  return { cancelLoad };
}
