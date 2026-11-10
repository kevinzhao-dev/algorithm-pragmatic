#!/usr/bin/env python3
"""Download slides/notes and assignments for the three MIT OCW courses.

Only PDFs and ZIP code bundles are fetched; videos are never requested. Files
land in materials/<course>/ and each course gets a MANIFEST.csv recording the
source URL, size and SHA-256. Re-running skips files that still match.

Usage:
    python3 -I tools/fetch_materials.py            # download whatever is missing
    python3 -I tools/fetch_materials.py --verify   # check local files only
"""

from __future__ import annotations

import argparse
import concurrent.futures as futures
import csv
import hashlib
import html as html_lib
import pathlib
import re
import shutil
import sys
import time
import urllib.error
import urllib.request
import zipfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
MATERIALS = ROOT / "materials"
BASE = "https://ocw.mit.edu"
USER_AGENT = "ocw-course-mirror/1.0"
TIMEOUT_SECONDS = 120
RETRIES = 4
WORKERS = 6
MANIFEST_FIELDS = ["path", "title", "resource_url", "file_url", "bytes", "sha256"]


def classify_6006(name: str) -> str | None:
    if m := re.fullmatch(r"mit6_006s20_lec(\d+)(?:_pdf)?", name):
        return f"lectures/lec{int(m[1]):02d}.pdf"
    if m := re.fullmatch(r"mit6_006s20_r(\d+)", name):
        return f"recitations/rec{int(m[1]):02d}.pdf"
    if m := re.fullmatch(r"mit6_006s20_ps(\d+)[-_]questions", name):
        return f"psets/ps{int(m[1])}/questions.pdf"
    if m := re.fullmatch(r"mit6_006s20_ps(\d+)[-_]solutions", name):
        return f"psets/ps{int(m[1])}/solutions.pdf"
    if m := re.fullmatch(r"mit6_006s20_ps(\d+)[-_]template", name):
        return f"psets/ps{int(m[1])}/template.zip"
    return None


def classify_6046(name: str) -> str | None:
    if name == "latex-template-for-problem-sets-zip":
        return None
    if m := re.fullmatch(r"c?lecture-(\d+)([ab]?)-notes(-handwritten)?", name):
        suffix = "-handwritten" if m[3] else ""
        return f"lectures/lec{int(m[1]):02d}{m[2]}{suffix}.pdf"
    if m := re.fullmatch(r"recitation-(\d+)-notes", name):
        return f"recitations/rec{int(m[1]):02d}.pdf"
    if m := re.fullmatch(r"solutions-to-problem-set-(\d+)", name):
        return f"psets/ps{int(m[1]):02d}/solutions.pdf"
    if m := re.fullmatch(r"problem-set-(\d+)", name):
        return f"psets/ps{int(m[1]):02d}/questions.pdf"
    return None


def classify_6172(name: str) -> str | None:
    if m := re.fullmatch(r"mit6_172f18_lec(\d+)", name):
        return f"lectures/lec{int(m[1]):02d}.pdf"
    if m := re.fullmatch(r"lecture-(\d+)-.*", name):
        return f"lectures/lec{int(m[1]):02d}.pdf"
    if m := re.fullmatch(r"homework-(\d+)-materials-zip", name):
        return f"homework/hw{int(m[1]):02d}/materials.zip"
    if m := re.fullmatch(r"homework-(\d+)-.*", name):
        return f"homework/hw{int(m[1]):02d}/handout.pdf"
    if m := re.fullmatch(r"project-(\d+)-materials-zip", name):
        return f"projects/p{int(m[1])}/materials.zip"
    if m := re.fullmatch(r"project-(\d+)-.*", name):
        return f"projects/p{int(m[1])}/handout.pdf"
    if name == "leiserchess":
        return "projects/p4/leiserchess-rules.pdf"
    if m := re.fullmatch(r"recitation-(\d+)-solutions", name):
        return f"recitations/r{int(m[1])}/solutions.pdf"
    if m := re.fullmatch(r"recitation-(\d+)", name):
        return f"recitations/r{int(m[1])}/problems.pdf"
    return None


COURSES = {
    "6.006": {
        "slug": "6-006-introduction-to-algorithms-spring-2020",
        "pages": ["lecture-notes", "assignments"],
        "classify": classify_6006,
    },
    "6.046j": {
        "slug": "6-046j-design-and-analysis-of-algorithms-spring-2015",
        "pages": ["lecture-notes", "recitation-notes", "assignments"],
        "classify": classify_6046,
    },
    "6.172": {
        "slug": "6-172-performance-engineering-of-software-systems-fall-2018",
        "pages": ["lecture-slides", "assignments", "projects", "recitation-problems"],
        "classify": classify_6172,
    },
}


def http_get(url: str) -> bytes:
    request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    last_error: Exception | None = None
    for attempt in range(RETRIES):
        try:
            with urllib.request.urlopen(request, timeout=TIMEOUT_SECONDS) as response:
                return response.read()
        except (urllib.error.URLError, TimeoutError, ConnectionError) as error:
            last_error = error
            time.sleep(2**attempt)
    raise RuntimeError(f"GET {url} failed after {RETRIES} attempts: {last_error}")


def list_resources(slug: str, page: str) -> list[tuple[str, str]]:
    """Return (resource_name, title) pairs linked from a course page, in page order."""
    html = http_get(f"{BASE}/courses/{slug}/pages/{page}/").decode("utf-8", "replace")
    section = re.search(r'id="course-content-section"(.*?)<footer', html, re.S)
    body = section.group(1) if section else html
    found: dict[str, str] = {}
    for href, inner in re.findall(r'<a[^>]+href="([^"]+)"[^>]*>(.*?)</a>', body, re.S):
        match = re.search(rf"/courses/{re.escape(slug)}/resources/([^/\"?#]+)/?$", href)
        if match:
            title = html_lib.unescape(re.sub(r"<[^>]+>", "", inner)).strip()
            found.setdefault(match[1], title)
    return list(found.items())


def resolve_file_url(slug: str, resource: str) -> str:
    page = http_get(f"{BASE}/courses/{slug}/resources/{resource}/").decode("utf-8", "replace")
    pattern = rf'href="(/courses/{re.escape(slug)}/[^"/]+\.(?:pdf|zip))"'
    match = re.search(pattern, page)
    if not match:
        raise RuntimeError(f"no downloadable PDF/ZIP found on resource page: {resource}")
    return BASE + match[1]


def looks_valid(data: bytes, relative_path: str) -> bool:
    if relative_path.endswith(".pdf"):
        return data[:5] == b"%PDF-"
    if relative_path.endswith(".zip"):
        return data[:2] == b"PK"
    return False


def load_manifest(course_dir: pathlib.Path) -> dict[str, dict[str, str]]:
    path = course_dir / "MANIFEST.csv"
    if not path.exists():
        return {}
    with path.open(newline="", encoding="utf-8") as handle:
        return {row["path"]: row for row in csv.DictReader(handle)}


def sha256_of(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def fetch_one(course_dir, slug, relative_path, resource, title, known) -> dict[str, str]:
    target = course_dir / relative_path
    previous = known.get(relative_path)
    if previous and target.exists() and sha256_of(target) == previous["sha256"]:
        return previous
    file_url = resolve_file_url(slug, resource)
    data = http_get(file_url)
    if not looks_valid(data, relative_path):
        raise RuntimeError(f"{file_url} is not a valid {pathlib.Path(relative_path).suffix} file")
    target.parent.mkdir(parents=True, exist_ok=True)
    partial = target.with_suffix(target.suffix + ".part")
    partial.write_bytes(data)
    partial.replace(target)
    return {
        "path": relative_path,
        "title": title,
        "resource_url": f"{BASE}/courses/{slug}/resources/{resource}/",
        "file_url": file_url,
        "bytes": str(len(data)),
        "sha256": hashlib.sha256(data).hexdigest(),
    }


def extract_zip(archive: pathlib.Path) -> int:
    """Extract next to the archive into a fresh code/ directory; returns file count."""
    destination = archive.parent / "code"
    if destination.exists():
        shutil.rmtree(destination)
    destination.mkdir()
    root = destination.resolve()
    count = 0
    with zipfile.ZipFile(archive) as bundle:
        for info in bundle.infolist():
            name = info.filename
            parts = pathlib.PurePosixPath(name).parts
            if info.is_dir() or "__MACOSX" in parts or parts[-1] in (".DS_Store", ".gitignore"):
                continue
            # Regular files only: symlink entries carry the S_IFLNK bit in the high bits.
            if (info.external_attr >> 16) & 0o170000 == 0o120000:
                continue
            output = (destination / name).resolve()
            if root not in output.parents:
                raise RuntimeError(f"unsafe path in {archive}: {name}")
            output.parent.mkdir(parents=True, exist_ok=True)
            with bundle.open(info) as source, output.open("wb") as sink:
                shutil.copyfileobj(source, sink)
            count += 1
    return count


def download_course(course: str, config: dict) -> tuple[int, int]:
    course_dir = MATERIALS / course
    course_dir.mkdir(parents=True, exist_ok=True)
    known = load_manifest(course_dir)
    slug = config["slug"]
    jobs: dict[str, tuple[str, str]] = {}
    for page in config["pages"]:
        for resource, title in list_resources(slug, page):
            relative_path = config["classify"](resource)
            if relative_path is None:
                print(f"  [skip] {course}: {resource}")
                continue
            jobs[relative_path] = (resource, title)

    rows: dict[str, dict[str, str]] = {}
    failures = 0
    with futures.ThreadPoolExecutor(max_workers=WORKERS) as pool:
        pending = {
            pool.submit(fetch_one, course_dir, slug, path, resource, title, known): path
            for path, (resource, title) in jobs.items()
        }
        for done in futures.as_completed(pending):
            path = pending[done]
            try:
                rows[path] = done.result()
            except Exception as error:  # keep going so one bad file does not lose the rest
                failures += 1
                print(f"  [FAIL] {course}/{path}: {error}", file=sys.stderr)

    with (course_dir / "MANIFEST.csv").open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=MANIFEST_FIELDS)
        writer.writeheader()
        for path in sorted(rows):
            writer.writerow(rows[path])

    for path in sorted(rows):
        if path.endswith(".zip"):
            extracted = extract_zip(course_dir / path)
            print(f"  [zip] {course}/{path}: {extracted} files")

    total_bytes = sum(int(row["bytes"]) for row in rows.values())
    print(f"{course}: {len(rows)} files, {total_bytes / 1e6:.1f} MB, {failures} failed")
    return len(rows), failures


def verify_course(course: str) -> int:
    course_dir = MATERIALS / course
    known = load_manifest(course_dir)
    bad = 0
    for path, row in sorted(known.items()):
        target = course_dir / path
        if not target.exists():
            print(f"  [missing] {course}/{path}")
            bad += 1
        elif sha256_of(target) != row["sha256"]:
            print(f"  [changed] {course}/{path}")
            bad += 1
    print(f"{course}: {len(known)} files checked, {bad} problems")
    return bad


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--verify", action="store_true", help="only check local files")
    args = parser.parse_args()
    if args.verify:
        return 1 if sum(verify_course(course) for course in COURSES) else 0
    failures = sum(download_course(course, config)[1] for course, config in COURSES.items())
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
