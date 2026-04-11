#!/usr/bin/env python3
"""
Rock wiki lint tool.

Checks: frontmatter validity, wiki link resolution, orphan detection,
index coverage, directory sizes, and pending sources.

Usage:
    python3 wikiroot/tools/lint.py [--wikiroot PATH] [--no-colour]

Exit codes: 0 = clean, 1 = issues found, 2 = invocation error.
"""

import re
import sys
import argparse
from pathlib import Path


# ── Configuration ────────────────────────────────────────────────────────────

REQUIRED_FRONTMATTER = {"title", "category", "tags", "sources", "updated", "status"}
VALID_STATUSES = {"current", "draft", "stale"}
DIR_SIZE_THRESHOLD = 15


# ── Colour helpers ────────────────────────────────────────────────────────────

class C:
    RED    = "\033[31m"
    YELLOW = "\033[33m"
    GREEN  = "\033[32m"
    RESET  = "\033[0m"
    BOLD   = "\033[1m"

def disable_colour() -> None:
    C.RED = C.YELLOW = C.GREEN = C.RESET = C.BOLD = ""


# ── Core helpers ──────────────────────────────────────────────────────────────

def page_key(pages_dir: Path, page_path: Path) -> str:
    """Return the wiki-link key for a page file, e.g. 'targets/zxn/zxn-dma'."""
    return str(page_path.relative_to(pages_dir).with_suffix(""))


def parse_frontmatter(content: str) -> dict | None:
    """
    Parse YAML-style frontmatter delimited by '---' on its own line.
    Returns a dict of key→value strings, or None if frontmatter is absent.
    Closing '---' is detected by line-start match to avoid false positives
    from '---' inside field values (e.g. filenames in the sources list).
    """
    lines = content.splitlines()
    if not lines or lines[0].strip() != "---":
        return None

    close_idx = None
    for i, line in enumerate(lines[1:], start=1):
        if line.strip() == "---":
            close_idx = i
            break

    if close_idx is None:
        return None

    fm: dict[str, str] = {}
    for line in lines[1:close_idx]:
        if ":" in line:
            key, _, value = line.partition(":")
            fm[key.strip()] = value.strip()
    return fm


def find_wiki_links(content: str) -> list[tuple[int, str]]:
    """Return all [[target]] links as (line_number, target) pairs."""
    results = []
    for i, line in enumerate(content.splitlines(), start=1):
        for m in re.finditer(r'\[\[([^\]]+)\]\]', line):
            results.append((i, m.group(1)))
    return results


# ── Checks ────────────────────────────────────────────────────────────────────

def check_frontmatter(pages_dir: Path, all_pages: list[Path]) -> list[tuple[str, str, str]]:
    issues: list[tuple[str, str, str]] = []

    print(f"\n{C.BOLD}=== FRONTMATTER ==={C.RESET}")
    failed = []

    for page in all_pages:
        content = page.read_text(encoding="utf-8")
        fm = parse_frontmatter(content)
        key = page_key(pages_dir, page)

        if fm is None:
            failed.append((key, "missing frontmatter block"))
            continue

        missing = REQUIRED_FRONTMATTER - set(fm.keys())
        if missing:
            failed.append((key, f"missing fields: {', '.join(sorted(missing))}"))

        if "status" in fm and fm["status"] not in VALID_STATUSES:
            failed.append((key, f"invalid status value: {fm['status']!r}"))

    if failed:
        for key, msg in failed:
            print(f"{C.RED}FAIL{C.RESET}  {key}  ({msg})")
            issues.append(("frontmatter", key, msg))
    else:
        print(f"{C.GREEN}PASS{C.RESET}  {len(all_pages)}/{len(all_pages)} pages have valid frontmatter")

    return issues


def check_links(
    pages_dir: Path,
    all_pages: list[Path],
    page_keys: dict[str, Path],
    index_path: Path,
) -> tuple[list[tuple[str, str, str]], int]:
    """Returns (issues, total_link_count)."""
    issues: list[tuple[str, str, str]] = []

    print(f"\n{C.BOLD}=== LINKS ==={C.RESET}")
    broken = []
    total = 0

    files_to_check = list(all_pages) + ([index_path] if index_path.exists() else [])

    for file_path in files_to_check:
        content = file_path.read_text(encoding="utf-8")
        links = find_wiki_links(content)
        total += len(links)

        if file_path == index_path:
            label = "index.md"
        else:
            label = page_key(pages_dir, file_path)

        for lineno, target in links:
            if target not in page_keys:
                broken.append((label, lineno, target))

    if broken:
        for source, lineno, target in broken:
            print(f"{C.RED}BROKEN{C.RESET}  [[{target}]]  (in {source}:{lineno})")
            issues.append(("link", target, f"referenced from {source}:{lineno}"))
        print(f"  {total - len(broken)}/{total} links valid")
    else:
        print(f"{C.GREEN}PASS{C.RESET}  {total}/{total} links valid")

    return issues, total


def check_orphans(
    pages_dir: Path,
    all_pages: list[Path],
    page_keys: dict[str, Path],
    index_path: Path,
) -> list[tuple[str, str, str]]:
    issues: list[tuple[str, str, str]] = []

    print(f"\n{C.BOLD}=== ORPHANS ==={C.RESET}")

    inbound: dict[str, int] = {k: 0 for k in page_keys}
    files_to_check = list(all_pages) + ([index_path] if index_path.exists() else [])

    for file_path in files_to_check:
        content = file_path.read_text(encoding="utf-8")
        for _, target in find_wiki_links(content):
            if target in inbound:
                inbound[target] += 1

    orphans = [k for k, n in sorted(inbound.items()) if n == 0]

    if orphans:
        for key in orphans:
            print(f"{C.YELLOW}ORPHAN{C.RESET}  {key}  (0 inbound links)")
            issues.append(("orphan", key, "0 inbound links"))
    else:
        print(f"{C.GREEN}PASS{C.RESET}  0 orphans")

    return issues


def check_index(
    pages_dir: Path,
    page_keys: dict[str, Path],
    index_path: Path,
) -> list[tuple[str, str, str]]:
    issues: list[tuple[str, str, str]] = []

    print(f"\n{C.BOLD}=== INDEX ==={C.RESET}")

    if not index_path.exists():
        print(f"{C.RED}ERROR{C.RESET}  index.md not found")
        return [("index", "index.md", "file not found")]

    index_content = index_path.read_text(encoding="utf-8")
    index_links = {target for _, target in find_wiki_links(index_content)}

    missing = sorted(k for k in page_keys if k not in index_links)
    extra   = sorted(t for t in index_links if t not in page_keys)

    for key in missing:
        print(f"{C.YELLOW}MISSING FROM INDEX{C.RESET}  {key}")
        issues.append(("index", key, "page not in index.md"))

    for target in extra:
        print(f"{C.RED}EXTRA IN INDEX{C.RESET}  {target}  (file not found)")
        issues.append(("index", target, "index.md entry has no matching file"))

    if not issues:
        print(
            f"{C.GREEN}PASS{C.RESET}  {len(page_keys)}/{len(page_keys)} pages indexed; "
            f"all {len(index_links)} index entries resolve"
        )

    return issues


def check_dir_sizes(pages_dir: Path, all_pages: list[Path]) -> list[tuple[str, str, str]]:
    issues: list[tuple[str, str, str]] = []

    print(f"\n{C.BOLD}=== DIRECTORY SIZES ==={C.RESET}")

    counts: dict[str, int] = {}
    for page in all_pages:
        parent = page.parent
        label = str(parent.relative_to(pages_dir)) if parent != pages_dir else "(root)"
        counts[label] = counts.get(label, 0) + 1

    overcrowded = [(d, n) for d, n in sorted(counts.items()) if n > DIR_SIZE_THRESHOLD]

    if overcrowded:
        for d, n in overcrowded:
            print(f"{C.YELLOW}WARN{C.RESET}  {d}/  {n} pages  (threshold: {DIR_SIZE_THRESHOLD})")
            issues.append(("structure", d, f"{n} pages exceeds threshold {DIR_SIZE_THRESHOLD}"))
        # Still print the full table for context
        print()
        for d, n in sorted(counts.items()):
            marker = f"  {C.YELLOW}<-- overcrowded{C.RESET}" if n > DIR_SIZE_THRESHOLD else ""
            print(f"  {d:<45} {n:>3}{marker}")
    else:
        for d, n in sorted(counts.items()):
            print(f"  {d:<45} {n:>3}")
        print(f"{C.GREEN}PASS{C.RESET}  all directories within threshold ({DIR_SIZE_THRESHOLD})")

    return issues


def check_pending(new_dir: Path) -> list[tuple[str, str, str]]:
    issues: list[tuple[str, str, str]] = []

    print(f"\n{C.BOLD}=== PENDING SOURCES ==={C.RESET}")

    if not new_dir.exists():
        print(f"{C.GREEN}PASS{C.RESET}  new/ not present")
        return issues

    pending = sorted(
        f for f in new_dir.iterdir()
        if not f.name.startswith(".") and f.name != ".gitkeep"
    )

    if pending:
        for f in pending:
            print(f"{C.YELLOW}PENDING{C.RESET}  {f.name}")
            issues.append(("pending", f.name, "awaiting ingest"))
    else:
        print(f"{C.GREEN}PASS{C.RESET}  no pending sources")

    return issues


# ── Entry point ───────────────────────────────────────────────────────────────

def run_lint(wikiroot: Path) -> int:
    pages_dir  = wikiroot / "pages"
    index_path = wikiroot / "index.md"
    new_dir    = wikiroot / "new"

    all_pages = sorted(pages_dir.rglob("*.md"))
    page_keys = {page_key(pages_dir, p): p for p in all_pages}

    print(f"Wiki root : {wikiroot}")
    print(f"Pages     : {len(all_pages)}")

    all_issues: list[tuple[str, str, str]] = []

    all_issues += check_frontmatter(pages_dir, all_pages)
    link_issues, total_links = check_links(pages_dir, all_pages, page_keys, index_path)
    all_issues += link_issues
    all_issues += check_orphans(pages_dir, all_pages, page_keys, index_path)
    all_issues += check_index(pages_dir, page_keys, index_path)
    all_issues += check_dir_sizes(pages_dir, all_pages)
    all_issues += check_pending(new_dir)

    # ── Summary ───────────────────────────────────────────────────────────────
    print(f"\n{C.BOLD}=== SUMMARY ==={C.RESET}")

    if not all_issues:
        print(
            f"{C.GREEN}CLEAN{C.RESET} — no issues found  "
            f"({len(all_pages)} pages, {total_links} links)"
        )
        return 0

    by_type: dict[str, int] = {}
    for t, _, _ in all_issues:
        by_type[t] = by_type.get(t, 0) + 1

    print(f"{C.RED}{len(all_issues)} issue(s) found:{C.RESET}")
    for t, n in sorted(by_type.items()):
        print(f"  {t:<14} {n}")
    return 1


def main() -> None:
    parser = argparse.ArgumentParser(description="Rock wiki structural lint")
    parser.add_argument(
        "--wikiroot", type=Path, default=None,
        help="Path to wikiroot/ directory (default: auto-detected from script location)"
    )
    parser.add_argument(
        "--no-colour", action="store_true",
        help="Disable ANSI colour output"
    )
    args = parser.parse_args()

    if args.no_colour:
        disable_colour()

    if args.wikiroot:
        wikiroot = args.wikiroot.resolve()
    else:
        # Script lives at wikiroot/tools/lint.py → parent.parent = wikiroot
        wikiroot = Path(__file__).resolve().parent.parent

    if not wikiroot.is_dir():
        print(f"Error: wikiroot not found at {wikiroot}", file=sys.stderr)
        sys.exit(2)

    sys.exit(run_lint(wikiroot))


if __name__ == "__main__":
    main()
