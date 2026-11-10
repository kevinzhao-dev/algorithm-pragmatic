#!/usr/bin/env -S python3 -I
"""Insert or refresh the OCW links in exercise READMEs.

Usage:
    python3 -I tools/readme_links.py problems/6.006/0114-doubly-linked-list/README.md
    python3 -I tools/readme_links.py --all

Under each README's H1 this writes "**Original problem:** ..." for assignment
problems (exercise numbers UU11-UU19) or "**Based on:** ..." otherwise, followed
by "**Read first:** ..." when the front matter has `read`. At the end it writes
the official-solution link when the front matter has `answer`. The URLs come
from materials/<course>/MANIFEST.csv. Running it again changes nothing.
"""
import argparse
import csv
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
TOP_PREFIXES = ("**Original problem:**", "**Based on:**")
FOOTER_PREFIX = "**Official solution:**"


def load_manifest():
    table = {}
    for manifest in sorted((ROOT / "materials").glob("*/MANIFEST.csv")):
        course = manifest.parent.name
        with manifest.open(newline="", encoding="utf-8") as handle:
            for row in csv.DictReader(handle):
                title = re.sub(r"\s*\(PDF[^)]*\)\s*$", "", row["title"]).strip()
                if "-handwritten" in row["path"]:
                    title += " (handwritten)"
                table[f"materials/{course}/{row['path']}"] = (title, row["file_url"])
    return table


def parse_meta(lines):
    meta = {}
    for line in lines:
        key, separator, value = line.partition(":")
        if separator:
            meta[key.strip()] = value.strip()
    return meta


def link(manifest, path):
    title, url = manifest[path]
    return f"[{title}]({url})"


def top_line(meta, number, manifest):
    label = "Original problem" if 11 <= int(number[2:]) <= 19 else "Based on"
    parts = [f"**{label}:** {link(manifest, meta['pdf'])}"]
    reads = [p.strip() for p in meta.get("read", "").split(",") if p.strip()]
    if reads:
        parts.append("**Read first:** " + ", ".join(link(manifest, p) for p in reads))
    return " · ".join(parts)


def footer_line(meta, manifest):
    answer = meta.get("answer", "").strip()
    return f"{FOOTER_PREFIX} {link(manifest, answer)}. Read it after you finish." if answer else None


def process(readme, manifest):
    lines = readme.read_text(encoding="utf-8").splitlines()
    end = lines.index("---", 1)
    meta = parse_meta(lines[1:end])
    number = readme.parent.name[:4]
    h1 = next(i for i in range(end + 1, len(lines)) if lines[i].startswith("# "))

    j = h1 + 1
    while j < len(lines) and not lines[j].strip():
        j += 1
    if j < len(lines) and lines[j].startswith(TOP_PREFIXES):
        del lines[j]
    while h1 + 1 < len(lines) and not lines[h1 + 1].strip():
        del lines[h1 + 1]
    lines[h1 + 1:h1 + 1] = ["", top_line(meta, number, manifest), ""]

    while lines and not lines[-1].strip():
        lines.pop()
    if lines and lines[-1].startswith(FOOTER_PREFIX):
        lines.pop()
        while lines and not lines[-1].strip():
            lines.pop()
        if lines and lines[-1].strip() == "---":
            lines.pop()
        while lines and not lines[-1].strip():
            lines.pop()
    footer = footer_line(meta, manifest)
    if footer:
        lines += ["", "---", "", footer]
    readme.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--all", action="store_true", help="update every exercise README")
    parser.add_argument("paths", nargs="*", help="README.md files to update")
    args = parser.parse_args()
    paths = [pathlib.Path(p).resolve() for p in args.paths]
    if args.all:
        paths = sorted(ROOT.glob("problems/*/[0-9][0-9][0-9][0-9]-*/README.md"))
    if not paths:
        parser.error("give README paths or --all")
    manifest = load_manifest()
    failures = 0
    for readme in paths:
        try:
            process(readme, manifest)
        except (KeyError, ValueError, StopIteration) as error:
            failures += 1
            print(f"{readme.relative_to(ROOT)}: {type(error).__name__}: {error}")
    print(f"updated {len(paths) - failures} of {len(paths)} READMEs")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
