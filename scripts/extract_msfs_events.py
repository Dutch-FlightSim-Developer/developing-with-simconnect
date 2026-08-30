#!/usr/bin/env python3
"""Throwaway scraper: MSFS SDK legacy key-event docs -> msfs-events/{2020,2024}/*.json

Not part of the build. Run manually whenever SDK docs are refreshed:

    python scripts/extract_msfs_events.py \
        --sdk2024 "H:\\MSFS 2024 SDK" \
        --sdk2020 "H:\\MSFS 2020 SDK" \
        --out msfs-events
"""
from __future__ import annotations

import argparse
import html
import json
import re
from pathlib import Path

WS_RE = re.compile(r"\s+")


def clean_text(text: str) -> str:
    text = html.unescape(text)
    text = re.sub(r"\{\{<\s*anchor[^}]*/>\s*\}\}", "", text)
    text = re.sub(r"\{\{<[^}]*>\}\}", "", text)
    text = re.sub(r"\[([^\]]+)\]\([^)]+\)", r"\1", text)  # markdown links -> text
    text = re.sub(r"<a[^>]*>.*?</a>", "", text, flags=re.S)
    text = re.sub(r"</?span[^>]*>", " | ", text)
    text = re.sub(r"<[^>]+>", "", text)
    text = text.replace("`", "").replace("**", "")
    text = WS_RE.sub(" ", text).strip()
    return text


def split_params(raw: str) -> list[dict]:
    raw = raw.strip(" |").strip()
    if not raw:
        return []
    parts = [p.strip(" |") for p in re.split(r"\s*\|\s*", raw) if p.strip(" |")]
    parts = [p for p in parts if p.upper() != "N/A"]
    params = []
    for part in parts:
        m = re.match(r"^\[(\d+)\]:\s*(.*)$", part)
        if m:
            params.append({"index": int(m.group(1)), "type": "DWORD", "description": m.group(2).strip()})
        else:
            params.append({"index": len(params), "type": "DWORD", "description": part})
    return params


ROW_RE_2024 = re.compile(
    r"^\|\s*(?P<name>.+?)\s*\|\s*(?P<eventid>.+?)\s*\|\s*(?P<params>.+?)\s*\|\s*(?P<desc>.+?)\s*\|\s*$"
)


def extract_2024_category(md_path: Path) -> list[dict]:
    events: list[dict] = []
    lines = md_path.read_text(encoding="utf-8").splitlines()
    in_table = False
    for line in lines:
        stripped = line.strip()
        if not stripped.startswith("|"):
            in_table = False
            continue
        if re.match(r"^\|\s*-{2,}", stripped):
            in_table = True
            continue
        if not in_table:
            continue
        m = ROW_RE_2024.match(stripped)
        if not m:
            continue
        name = clean_text(m.group("name"))
        event_id = clean_text(m.group("eventid"))
        params = split_params(clean_text(m.group("params")))
        desc = clean_text(m.group("desc"))
        if not name:
            continue
        events.append({"name": name, "eventId": event_id, "params": params, "description": desc})
    return events


def extract_2024(sdk_root: Path, out_dir: Path) -> None:
    base = sdk_root / "Documentation" / "public" / "flighting" / "programming-apis" / "key-events"
    out_dir.mkdir(parents=True, exist_ok=True)
    for category_dir in sorted(p for p in base.iterdir() if p.is_dir()):
        md_path = category_dir / "index.md"
        if not md_path.exists():
            continue
        events = extract_2024_category(md_path)
        if not events:
            continue
        out_path = out_dir / f"{category_dir.name}.json"
        out_path.write_text(
            json.dumps({"source": "MSFS2024_SDK", "category": category_dir.name, "events": events}, indent=2),
            encoding="utf-8",
        )
        print(f"2024: {category_dir.name}: {len(events)} events -> {out_path}")


ROW_RE_2020 = re.compile(r"<tr>\s*<td>(.*?)</td>\s*<td>(.*?)</td>\s*<td>(.*?)</td>\s*</tr>", re.S)
NAME_RE_2020 = re.compile(r'<code[^>]*>(.*?)</code>')


def extract_2020_category(htm_path: Path) -> list[dict]:
    events: list[dict] = []
    content = htm_path.read_text(encoding="utf-8", errors="ignore")
    for name_cell, params_cell, desc_cell in ROW_RE_2020.findall(content):
        name_m = NAME_RE_2020.search(name_cell)
        if not name_m:
            continue  # header row, no <code> name cell
        name = clean_text(name_m.group(1))
        if not name:
            continue
        params = split_params(clean_text(params_cell))
        desc = clean_text(desc_cell)
        events.append({"name": name, "eventId": f"KEY_{name}", "params": params, "description": desc})
    return events


def extract_2020(sdk_root: Path, out_dir: Path) -> None:
    base = sdk_root / "Documentation" / "html" / "Programming_Tools" / "Event_IDs"
    out_dir.mkdir(parents=True, exist_ok=True)
    for htm_path in sorted(base.glob("*.htm")):
        if htm_path.stem == "Event_IDs":
            continue  # index page, not a category table
        events = extract_2020_category(htm_path)
        if not events:
            continue
        out_path = out_dir / f"{htm_path.stem}.json"
        out_path.write_text(
            json.dumps({"source": "MSFS2020_SDK", "category": htm_path.stem, "events": events}, indent=2),
            encoding="utf-8",
        )
        print(f"2020: {htm_path.stem}: {len(events)} events -> {out_path}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--sdk2024", type=Path, required=True, help="MSFS 2024 SDK root")
    parser.add_argument("--sdk2020", type=Path, required=True, help="MSFS 2020 SDK root")
    parser.add_argument("--out", type=Path, default=Path("msfs-events"), help="output root directory")
    args = parser.parse_args()

    extract_2024(args.sdk2024, args.out / "2024")
    extract_2020(args.sdk2020, args.out / "2020")


if __name__ == "__main__":
    main()
