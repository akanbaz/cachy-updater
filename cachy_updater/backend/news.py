"""Fetch Arch (and optional Cachy) news for the updater UI."""

from __future__ import annotations

import re
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from email.utils import parsedate_to_datetime
from html import unescape

from cachy_updater.backend import run_cmd, which

ARCH_NEWS_FEED = "https://archlinux.org/feeds/news/"
# Best-effort Cachy sources; failures are soft warnings.
CACHY_NEWS_FEEDS = (
    "https://cachyos.org/blog/index.xml",
    "https://cachyos.org/index.xml",
)

NEWS_TIMEOUT = 12
_TAG_RE = re.compile(r"<[^>]+>")


@dataclass(slots=True)
class NewsItem:
    title: str
    link: str
    summary: str = ""
    published: str = ""
    source: str = "Arch"


@dataclass(slots=True)
class NewsResult:
    items: list[NewsItem]
    warnings: list[str]


def fetch_news(*, limit: int = 8) -> NewsResult:
    items: list[NewsItem] = []
    warnings: list[str] = []

    arch, arch_warn = _fetch_feed(ARCH_NEWS_FEED, source="Arch", limit=limit)
    items.extend(arch)
    warnings.extend(arch_warn)

    cachy_warnings: list[str] = []
    for url in CACHY_NEWS_FEEDS:
        cachy, warn = _fetch_feed(url, source="CachyOS", limit=max(3, limit // 2))
        if cachy:
            items.extend(cachy)
            cachy_warnings = []
            break
        cachy_warnings.extend(warn)
    if not any(i.source == "CachyOS" for i in items) and cachy_warnings:
        warnings.append("CachyOS news feed unavailable right now (Arch news still shown).")

    # De-dupe by title, keep order
    seen: set[str] = set()
    unique: list[NewsItem] = []
    for item in items:
        key = item.title.casefold()
        if key in seen:
            continue
        seen.add(key)
        unique.append(item)

    return NewsResult(items=unique[:limit], warnings=warnings)


def _fetch_feed(
    url: str, *, source: str, limit: int
) -> tuple[list[NewsItem], list[str]]:
    if not which("curl"):
        return [], [f"curl not found; cannot fetch {source} news."]

    proc = run_cmd(
        [
            "curl",
            "--compressed",
            "-fsSL",
            "--max-time",
            str(NEWS_TIMEOUT),
            url,
        ],
        timeout=NEWS_TIMEOUT + 2,
    )
    if proc.returncode != 0 or not proc.stdout.strip():
        err = (proc.stderr or proc.stdout or "empty response").strip()
        return [], [f"Could not fetch {source} news: {err[:160]}"]

    try:
        root = ET.fromstring(proc.stdout)
    except ET.ParseError as exc:
        return [], [f"Invalid {source} news feed: {exc}"]

    items: list[NewsItem] = []
    # RSS 2.0
    for node in root.findall("./channel/item"):
        title = _text(node.find("title"))
        link = _text(node.find("link"))
        if not title:
            continue
        desc = _strip_html(_text(node.find("description")))
        pub = _format_date(_text(node.find("pubDate")))
        items.append(
            NewsItem(
                title=title,
                link=link,
                summary=desc[:280],
                published=pub,
                source=source,
            )
        )
        if len(items) >= limit:
            break

    if items:
        return items, []

    # Atom fallback
    ns = {"a": "http://www.w3.org/2005/Atom"}
    for node in root.findall("a:entry", ns) or root.findall("entry"):
        title = _text(node.find("a:title", ns)) or _text(node.find("title"))
        link_el = node.find("a:link", ns) or node.find("link")
        link = ""
        if link_el is not None:
            link = link_el.get("href") or _text(link_el)
        if not title:
            continue
        summary = _strip_html(
            _text(node.find("a:summary", ns))
            or _text(node.find("summary"))
            or _text(node.find("a:content", ns))
            or _text(node.find("content"))
        )
        published = _format_date(
            _text(node.find("a:updated", ns))
            or _text(node.find("updated"))
            or _text(node.find("a:published", ns))
            or _text(node.find("published"))
        )
        items.append(
            NewsItem(
                title=title,
                link=link,
                summary=summary[:280],
                published=published,
                source=source,
            )
        )
        if len(items) >= limit:
            break

    if not items:
        return [], [f"No entries parsed from {source} news feed."]
    return items, []


def _text(node: ET.Element | None) -> str:
    if node is None or node.text is None:
        return ""
    return unescape(node.text.strip())


def _strip_html(text: str) -> str:
    return unescape(_TAG_RE.sub(" ", text or "")).strip()


def _format_date(raw: str) -> str:
    if not raw:
        return ""
    try:
        dt = parsedate_to_datetime(raw)
        return dt.strftime("%Y-%m-%d")
    except (TypeError, ValueError, IndexError):
        return raw[:16]
