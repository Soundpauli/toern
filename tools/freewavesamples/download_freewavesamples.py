#!/usr/bin/env python3
"""Scrape freewavesamples.com once, then download WAVs from the saved catalog.

Phase 1 (scrape): walk category + sample pages and write data-src.json.
Phase 2 (download): read that file and pull WAVs one by one into category folders.

The catalog is reused unless missing or --force-scrape is set. Files already on
disk are skipped. WAV URLs use ?nocount=1 so the public download counter is not
incremented.

Usage:
  python3 tools/freewavesamples/download_freewavesamples.py --scrape-only
  python3 tools/freewavesamples/download_freewavesamples.py --out ~/Samples/freewavesamples
  python3 tools/freewavesamples/download_freewavesamples.py --max-files 10
"""

from __future__ import annotations

import argparse
import email.message
import http.client
import json
import random
import re
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from html.parser import HTMLParser
from pathlib import Path

ORIGIN = "https://freewavesamples.com"
USER_AGENT = "toern-freewavesamples/1.0 (single-threaded; polite mirror)"
DEFAULT_CATALOG = Path(__file__).resolve().parent / "data-src.json"
SKIP_SLUGS = {
    "about-us-license",
    "contact",
    "favicon.ico",
    "hydrogen-drum-kits",
    "midi-drums",
    "midi-instruments",
    "popular",
    "privacy-policy",
    "random",
    "sources",
}


class LinkParser(HTMLParser):
    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.hrefs: list[str] = []

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        if tag not in {"a", "source"}:
            return
        for key, value in attrs:
            if key in {"href", "src"} and value:
                self.hrefs.append(value)


def links_from(html: str) -> list[str]:
    parser = LinkParser()
    parser.feed(html)
    return parser.hrefs


def same_site(url: str) -> str | None:
    parsed = urllib.parse.urlparse(url)
    if parsed.netloc and parsed.netloc not in {"freewavesamples.com", "www.freewavesamples.com"}:
        return None
    path = urllib.parse.unquote(parsed.path or "/")
    if not path.startswith("/"):
        path = "/" + path
    query = f"?{parsed.query}" if parsed.query else ""
    return urllib.parse.urljoin(ORIGIN, path) + query


def slug_of(url: str) -> str | None:
    path = urllib.parse.urlparse(url).path.strip("/")
    if not path or "/" in path or path in SKIP_SLUGS:
        return None
    if not re.fullmatch(r"[a-z0-9][a-z0-9-]*", path):
        return None
    return path


def category_of(url: str) -> tuple[str, ...] | None:
    path = urllib.parse.urlparse(url).path.strip("/")
    parts = path.split("/")
    if len(parts) < 2 or parts[0] != "sample-type":
        return None
    return tuple(parts[1:])


def safe_part(text: str) -> str:
    cleaned = re.sub(r"[^A-Za-z0-9._-]+", "-", text).strip(".-")
    return cleaned or "untitled"


class Client:
    def __init__(self, delay: float, timeout: float) -> None:
        self.delay = delay
        self.timeout = timeout
        self._next_ok = 0.0
        self.requests = 0

    def _wait(self) -> None:
        remaining = self._next_ok - time.monotonic()
        if remaining > 0:
            time.sleep(remaining)
        self._next_ok = time.monotonic() + self.delay + random.uniform(0.3, 1.1)

    def get(self, url: str, dest: Path | None = None) -> tuple[int, str, bytes]:
        last_error: Exception | None = None
        for attempt in range(6):
            self._wait()
            request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT, "Accept": "*/*"})
            try:
                with urllib.request.urlopen(request, timeout=self.timeout) as response:
                    self.requests += 1
                    status = getattr(response, "status", 200)
                    content_type = response.headers.get("Content-Type", "")
                    if dest is None:
                        return status, content_type, response.read()
                    dest.parent.mkdir(parents=True, exist_ok=True)
                    temporary = dest.with_suffix(dest.suffix + ".part")
                    with temporary.open("wb") as handle:
                        while True:
                            chunk = response.read(64 * 1024)
                            if not chunk:
                                break
                            handle.write(chunk)
                    temporary.replace(dest)
                    return status, content_type, b""
            except urllib.error.HTTPError as error:
                self.requests += 1
                last_error = error
                if error.code not in {429, 500, 502, 503, 504}:
                    raise
                self._backoff(error.headers, attempt)
            except (urllib.error.URLError, TimeoutError, http.client.HTTPException, ConnectionError) as error:
                last_error = error
                self._backoff(email.message.Message(), attempt)
        assert last_error is not None
        raise last_error

    def _backoff(self, headers: email.message.Message, attempt: int) -> None:
        retry_after = headers.get("Retry-After", "")
        if retry_after.isdigit():
            pause = float(retry_after)
        else:
            pause = min(180.0, 12.0 * (2**attempt))
        pause += random.uniform(0.5, 2.0)
        print(f"  server asked us to slow down; waiting {pause:.0f}s", file=sys.stderr)
        self._next_ok = time.monotonic() + pause


def page_numbers(found: list[str], category_url: str) -> set[int]:
    numbers = {1}
    base = urllib.parse.urlparse(category_url)
    for href in found:
        absolute = same_site(href)
        if not absolute:
            continue
        parsed = urllib.parse.urlparse(absolute)
        if parsed.path.rstrip("/") != base.path.rstrip("/"):
            continue
        query = urllib.parse.parse_qs(parsed.query)
        for raw in query.get("page", []):
            if raw.isdigit():
                numbers.add(int(raw))
    return numbers


def collect_categories(home_links: list[str]) -> list[str]:
    found: set[str] = set()
    for href in home_links:
        absolute = same_site(href)
        if absolute and category_of(absolute.split("?")[0]):
            found.add(absolute.split("?")[0])
    return sorted(found, key=lambda url: (-len(category_of(url) or ()), url))


def collect_slugs(page_links: list[str]) -> list[str]:
    # Sample cards are linked twice (title and heading). Nav items are linked once.
    counts: dict[str, int] = {}
    order: list[str] = []
    for href in page_links:
        absolute = same_site(href)
        if not absolute:
            continue
        slug = slug_of(absolute.split("?")[0])
        if not slug:
            continue
        counts[slug] = counts.get(slug, 0) + 1
        if slug not in order:
            order.append(slug)
    return [slug for slug in order if counts[slug] >= 2]


def wav_link(page_links: list[str]) -> str | None:
    for href in page_links:
        absolute = same_site(href)
        if not absolute:
            continue
        path = urllib.parse.urlparse(absolute).path
        if path.lower().endswith(".wav"):
            counted = urllib.parse.urljoin(ORIGIN, path)
            return counted + "?nocount=1"
    return None


def already_saved(path: Path) -> bool:
    return path.is_file() and path.stat().st_size > 44


def load_catalog(path: Path) -> list[dict]:
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict) or not isinstance(data.get("samples"), list):
        raise ValueError(f"bad catalog format: {path}")
    return data["samples"]


def save_catalog(path: Path, samples: list[dict], requests: int) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = {
        "origin": ORIGIN,
        "scraped_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "requests": requests,
        "count": len(samples),
        "samples": samples,
    }
    temporary = path.with_suffix(path.suffix + ".part")
    temporary.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    temporary.replace(path)


def scrape_catalog(client: Client, catalog_path: Path) -> list[dict]:
    print(f"scraping {ORIGIN} → {catalog_path}")
    _status, _kind, home = client.get(ORIGIN + "/")
    categories = collect_categories(links_from(home.decode("utf-8", "replace")))
    print(f"{len(categories)} category pages")

    # slug -> category parts discovered from the listing that mentioned it
    by_slug: dict[str, tuple[str, ...]] = {}
    seen_pages: set[str] = set()
    for category_url in categories:
        pending = {1}
        visited: set[int] = set()
        while pending:
            page = min(pending)
            pending.remove(page)
            if page in visited:
                continue
            visited.add(page)
            url = category_url if page == 1 else f"{category_url}?page={page}"
            if url in seen_pages:
                continue
            seen_pages.add(url)
            try:
                _status, content_type, body = client.get(url)
            except Exception as error:  # noqa: BLE001 - keep going after one bad page
                print(f"skip {url}: {error}", file=sys.stderr)
                continue
            if "html" not in content_type.lower():
                continue
            html = body.decode("utf-8", "replace")
            found = links_from(html)
            category = category_of(category_url) or ("uncategorized",)
            added = 0
            for slug in collect_slugs(found):
                by_slug.setdefault(slug, category)
                added += 1
            pending.update(page_numbers(found, category_url) - visited)
            print(f"  {category_url} page {page}: {added} samples")

    print(f"{len(by_slug)} sample pages to resolve")
    samples: list[dict] = []
    failed = 0
    for index, (slug, category) in enumerate(sorted(by_slug.items(), key=lambda item: (item[1], item[0])), start=1):
        page_url = f"{ORIGIN}/{slug}"
        try:
            _status, content_type, body = client.get(page_url)
        except Exception as error:  # noqa: BLE001
            failed += 1
            print(f"[{index}/{len(by_slug)}] fail {slug}: {error}", file=sys.stderr)
            continue
        if "html" not in content_type.lower():
            failed += 1
            continue
        wav = wav_link(links_from(body.decode("utf-8", "replace")))
        if not wav:
            failed += 1
            print(f"[{index}/{len(by_slug)}] no wav: {slug}", file=sys.stderr)
            continue
        filename = safe_part(Path(urllib.parse.urlparse(wav).path).name)
        entry = {
            "slug": slug,
            "category": list(category),
            "page": page_url,
            "wav": wav,
            "filename": filename,
        }
        samples.append(entry)
        print(f"[{index}/{len(by_slug)}] {slug} → {'/'.join(category)}/{filename}")
        if index % 50 == 0:
            save_catalog(catalog_path, samples, client.requests)
            print(f"  checkpoint: {len(samples)} entries in {catalog_path}")

    save_catalog(catalog_path, samples, client.requests)
    print(f"scraped {len(samples)} samples ({failed} failed) → {catalog_path}")
    return samples


def download_samples(client: Client, samples: list[dict], out: Path, max_files: int) -> int:
    out.mkdir(parents=True, exist_ok=True)
    downloaded = 0
    skipped = 0
    failed = 0
    total = len(samples)
    for index, entry in enumerate(samples, start=1):
        category = entry.get("category") or ["uncategorized"]
        filename = entry.get("filename") or safe_part(Path(urllib.parse.urlparse(entry["wav"]).path).name)
        dest = out.joinpath(*(safe_part(part) for part in category), safe_part(filename))
        if already_saved(dest):
            skipped += 1
            print(f"[{index}/{total}] have {dest.relative_to(out)}")
            continue
        try:
            _status, file_type, _empty = client.get(entry["wav"], dest)
        except Exception as error:  # noqa: BLE001
            failed += 1
            print(f"[{index}/{total}] fail {entry['wav']}: {error}", file=sys.stderr)
            continue
        if "html" in file_type.lower() or not already_saved(dest):
            dest.unlink(missing_ok=True)
            failed += 1
            print(f"[{index}/{total}] not a wav: {entry['wav']}", file=sys.stderr)
            continue
        downloaded += 1
        print(f"[{index}/{total}] saved {dest.relative_to(out)} ({dest.stat().st_size} bytes)")
        if max_files and downloaded >= max_files:
            break
        if downloaded and downloaded % 25 == 0:
            pause = 8.0 + random.uniform(0, 4)
            print(f"  pausing {pause:.0f}s after {downloaded} files")
            client._next_ok = time.monotonic() + pause

    print(f"done: {downloaded} saved, {skipped} already there, {failed} failed, {client.requests} requests")
    return 1 if failed and not downloaded else 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=Path("freewavesamples"), help="directory for category folders")
    parser.add_argument(
        "--catalog",
        type=Path,
        default=DEFAULT_CATALOG,
        help=f"scrape catalog path (default {DEFAULT_CATALOG.name})",
    )
    parser.add_argument("--delay", type=float, default=2.0, help="minimum seconds between requests (default 2)")
    parser.add_argument("--timeout", type=float, default=60.0)
    parser.add_argument("--max-files", type=int, default=0, help="stop after this many new downloads (0 = all)")
    parser.add_argument("--scrape-only", action="store_true", help="build/update catalog and exit")
    parser.add_argument("--force-scrape", action="store_true", help="rebuild catalog even if it already exists")
    parser.add_argument("--download-only", action="store_true", help="require existing catalog; never scrape")
    args = parser.parse_args()
    if args.delay < 1.0:
        parser.error("--delay must be at least 1 second")
    if args.scrape_only and args.download_only:
        parser.error("use only one of --scrape-only / --download-only")

    client = Client(args.delay, args.timeout)
    catalog_path = args.catalog

    if args.download_only:
        if not catalog_path.is_file():
            print(f"missing catalog: {catalog_path}", file=sys.stderr)
            return 1
        samples = load_catalog(catalog_path)
        print(f"using {catalog_path} ({len(samples)} samples)")
        return download_samples(client, samples, args.out, args.max_files)

    need_scrape = args.force_scrape or args.scrape_only or not catalog_path.is_file()
    if need_scrape:
        samples = scrape_catalog(client, catalog_path)
    else:
        samples = load_catalog(catalog_path)
        print(f"using existing catalog {catalog_path} ({len(samples)} samples)")

    if args.scrape_only:
        return 0 if samples else 1
    return download_samples(client, samples, args.out, args.max_files)


if __name__ == "__main__":
    raise SystemExit(main())
