#!/usr/bin/env python3
"""
Detect Lakos levelization violations in Zancle utility modules.

The rule: a module at level N may only `#include` from modules at levels
strictly less than N. Same-level cross-module includes are violations
(modules at the same level are siblings, not a dependency chain).

Multimedia modules (Audio, GLUtils, Graphics, ImGui, Network, Window) live
above the utility hierarchy. They can freely include any utility module;
strict ordering between them is not enforced here (the .clang-format
priorities for that range are alphabetical, not topological).

Usage:
    scripts/check_levelization.py
    scripts/check_levelization.py --root /path/to/repo
"""

import argparse
import re
import sys
from pathlib import Path

# Utility module levels. Keep this in sync with `.clang-format`.
UTILITY_LEVELS = {
    # Level 7
    "Err":         7,
    "Lifetime":    7,
    "Scn":         7,
    # Level 6
    "Fmt":         6,
    "IO":          6,
    "Reflection":  6,
    # Level 5
    "Concurrency": 5,
    "Mixin":       5,
    "Random":      5,
    "String":      5,
    # Level 4
    "Algorithm":   4,
    "Chrono":      4,
    "Container":   4,
    "Geometry":    4,
    # Level 3
    "Vocabulary":  3,
    # Level 2
    "Math":        2,
    # Level 1
    "Base":        1,
    # Level 0
    "Trait":       0,
}

# Multimedia / framework modules. Outside the strict utility hierarchy --
# any utility module is fair game. Cross-multimedia checks are not enforced
# (would require its own levelization decision).
MULTIMEDIA_MODULES = {
    "Audio", "GLUtils", "Graphics", "ImGui", "Network", "Window",
}

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*"Zancle/([^/"]+)/')


def module_of_file(path: Path, repo_root: Path) -> str | None:
    """Return the Zancle/<Module> name a file belongs to, or None if outside."""
    try:
        rel = path.relative_to(repo_root)
    except ValueError:
        return None
    parts = rel.parts
    # include/Zancle/<Module>/... or src/Zancle/<Module>/...
    if len(parts) < 3:
        return None
    if parts[0] not in ("include", "src") or parts[1] != "Zancle":
        return None
    return parts[2]


def find_files(repo_root: Path):
    """All Zancle C++ source files we care about."""
    roots = [repo_root / "include" / "Zancle", repo_root / "src" / "Zancle"]
    exts = {".hpp", ".cpp", ".inl"}
    for root in roots:
        if not root.exists():
            continue
        for f in root.rglob("*"):
            if f.is_file() and f.suffix in exts:
                yield f


def check_file(path: Path, owner_module: str) -> list[tuple[int, str, str]]:
    """
    Inspect `path` and return a list of (line_number, included_module, reason)
    tuples for each levelization violation.
    """
    owner_level = UTILITY_LEVELS.get(owner_module)
    violations: list[tuple[int, str, str]] = []

    # Files in multimedia modules are not subject to the utility-level
    # check on what they include (multimedia is above all utility levels).
    is_multimedia_owner = owner_module in MULTIMEDIA_MODULES

    try:
        with path.open("r", encoding="utf-8", errors="replace") as f:
            for line_no, line in enumerate(f, start=1):
                m = INCLUDE_RE.match(line)
                if not m:
                    continue
                inc_module = m.group(1)

                # Own-module includes are always fine.
                if inc_module == owner_module:
                    continue

                # Utility module including a multimedia module is forbidden
                # (utility cannot depend on the multimedia layer).
                if not is_multimedia_owner and inc_module in MULTIMEDIA_MODULES:
                    violations.append((
                        line_no, inc_module,
                        f"utility module '{owner_module}' depends on multimedia '{inc_module}'"
                    ))
                    continue

                # Cross-utility checks only apply when the owner IS a utility module.
                if owner_level is None:
                    continue

                inc_level = UTILITY_LEVELS.get(inc_module)
                if inc_level is None:
                    # Unknown utility module (typo, or newly added but not registered).
                    continue

                if inc_level >= owner_level:
                    rel_owner = f"{owner_module}(level {owner_level})"
                    rel_inc = f"{inc_module}(level {inc_level})"
                    if inc_level == owner_level:
                        reason = f"same-level sibling include: {rel_owner} <- {rel_inc}"
                    else:
                        reason = f"upward include: {rel_owner} <- {rel_inc}"
                    violations.append((line_no, inc_module, reason))
    except OSError as e:
        print(f"warning: could not read {path}: {e}", file=sys.stderr)

    return violations


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=Path,
        default=Path(__file__).resolve().parent.parent,
        help="Repository root (default: parent of scripts/)",
    )
    args = parser.parse_args()

    repo_root = args.root.resolve()
    if not (repo_root / "include" / "Zancle").exists():
        print(f"error: {repo_root} does not look like a Zancle repo", file=sys.stderr)
        return 2

    total_violations = 0
    files_with_violations = 0

    for path in find_files(repo_root):
        owner = module_of_file(path, repo_root)
        if owner is None:
            continue
        # Skip files in unknown modules entirely.
        if owner not in UTILITY_LEVELS and owner not in MULTIMEDIA_MODULES:
            continue

        violations = check_file(path, owner)
        if not violations:
            continue

        rel = path.relative_to(repo_root)
        files_with_violations += 1
        total_violations += len(violations)
        for line_no, _, reason in violations:
            print(f"{rel}:{line_no}: {reason}")

    print()
    if total_violations == 0:
        print("OK: no levelization violations found.")
        return 0

    print(f"FAIL: {total_violations} violation(s) across {files_with_violations} file(s).")
    return 1


if __name__ == "__main__":
    sys.exit(main())
