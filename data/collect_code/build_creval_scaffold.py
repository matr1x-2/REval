"""
Build CREval_data.jsonl scaffolds from C-REval sampled code files.

This script generates draft `harness` and `stubs` for each sample.
For samples with unresolved function arguments, it emits TODO markers.
"""

from __future__ import annotations

import argparse
import json
import os
import re
from typing import Optional

COMMON_STUBS = r'''#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#ifndef __user
#define __user
#endif
#ifndef __iomem
#define __iomem
#endif
#ifndef __init
#define __init
#endif
#ifndef __must_check
#define __must_check
#endif

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif
#ifndef likely
#define likely(x)   (x)
#endif
#ifndef unlikely
#define unlikely(x) (x)
#endif

#ifndef ASSERT_OK
#define ASSERT_OK(x) do { if ((x) != 0) { printf("ASSERT_OK fail at %s:%d\\n", __FILE__, __LINE__); return -1; } } while (0)
#endif
#ifndef ASSERT_EQ
#define ASSERT_EQ(a, b) do { if ((a) != (b)) { printf("ASSERT_EQ fail at %s:%d\\n", __FILE__, __LINE__); return -1; } } while (0)
#endif
#ifndef ASSERT_NEQ
#define ASSERT_NEQ(a, b) do { if ((a) == (b)) { printf("ASSERT_NEQ fail at %s:%d\\n", __FILE__, __LINE__); return -1; } } while (0)
#endif
#ifndef ASSERT_GT
#define ASSERT_GT(a, b) do { if (!((a) > (b))) { printf("ASSERT_GT fail at %s:%d\\n", __FILE__, __LINE__); return -1; } } while (0)
#endif
#ifndef ASSERT_GE
#define ASSERT_GE(a, b) do { if (!((a) >= (b))) { printf("ASSERT_GE fail at %s:%d\\n", __FILE__, __LINE__); return -1; } } while (0)
#endif
#ifndef ASSERT_FALSE
#define ASSERT_FALSE(x, ...) do { if ((x)) { printf("ASSERT_FALSE fail at %s:%d\\n", __FILE__, __LINE__); return -1; } } while (0)
#endif
#ifndef ASSERT_OK_PTR
#define ASSERT_OK_PTR(x, ...) ((x) != NULL)
#endif

#ifndef TEST_IMPL
#define TEST_IMPL(name) int test_##name(void)
#endif
'''


def _read(path: str) -> str:
    with open(path, "r", encoding="utf-8", errors="ignore") as f:
        return f.read()


def _load_manifest(dataset_dir: str) -> list[dict]:
    path = os.path.join(dataset_dir, "metadata", "manifest.jsonl")
    rows = []
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            rows.append(json.loads(line))
    return rows


def _infer_entry_point(code: str) -> tuple[Optional[str], str]:
    m = re.search(r"\bTEST_IMPL\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{", code)
    if m:
        return f"test_{m.group(1)}", "test_impl"

    m = re.search(
        r"^\s*(?:static\s+)?(?:inline\s+)?[A-Za-z_][A-Za-z0-9_\s\*]*?\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(([^;{}]*)\)\s*\{",
        code,
        flags=re.MULTILINE,
    )
    if not m:
        return None, "unknown"

    name = m.group(1)
    args = m.group(2).strip()
    if args in ("", "void"):
        return name, "no_args"
    return name, "has_args"


def _harness_for(entry: Optional[str], kind: str) -> str:
    if entry is None:
        return (
            "int main(void) {\n"
            "  /* TODO: Could not infer callable symbol automatically. */\n"
            "  printf(\"TODO\\n\");\n"
            "  return 0;\n"
            "}\n"
        )
    if kind in ("test_impl", "no_args"):
        return (
            "int main(void) {\n"
            f"  int rc = {entry}();\n"
            "  printf(\"%d\\n\", rc);\n"
            "  return 0;\n"
            "}\n"
        )

    return (
        "int main(void) {\n"
        f"  /* TODO: Provide concrete arguments for {entry}(...) */\n"
        f"  /* Example: int rc = {entry}(arg1, arg2); */\n"
        "  printf(\"TODO_ARGS\\n\");\n"
        "  return 0;\n"
        "}\n"
    )


def build_scaffold_records(dataset_dir: str, limit: int) -> list[dict]:
    rows = _load_manifest(dataset_dir)
    out = []
    for idx, row in enumerate(rows):
        if limit > 0 and idx >= limit:
            break

        sample_path = os.path.join(dataset_dir, row["sample_path"])
        code = _read(sample_path)
        entry, kind = _infer_entry_point(code)
        harness = _harness_for(entry, kind)

        out.append(
            {
                "task_id": f"CREval/{idx}",
                "entry_point": entry or "unknown",
                "category": row.get("category"),
                "sample_id": row.get("sample_id"),
                "source_rel": row.get("source_rel"),
                "compile_flags": ["-std=gnu11", "-O0", "-g", "-pthread"],
                "code": code,
                "inputs": [
                    {
                        "stubs": COMMON_STUBS,
                        "harness": harness,
                        "output": "",
                        "notes": (
                            "auto-generated scaffold; review stubs/harness before running."
                            if kind != "no_args"
                            else "auto-generated scaffold"
                        ),
                    }
                ],
            }
        )
    return out


def main():
    parser = argparse.ArgumentParser(description="Generate CREval scaffold with draft harness/stubs")
    parser.add_argument("--dataset-dir", default="data/collect_code/C-REval_MultiRepo_v1")
    parser.add_argument("--output", default="data/CREval_data.scaffold.jsonl")
    parser.add_argument("--limit", type=int, default=0, help="0 means all samples")
    args = parser.parse_args()

    records = build_scaffold_records(args.dataset_dir, args.limit)

    with open(args.output, "w", encoding="utf-8") as f:
        for rec in records:
            f.write(json.dumps(rec, ensure_ascii=False) + "\n")

    print(f"Wrote {len(records)} scaffold records to {args.output}")


if __name__ == "__main__":
    main()
