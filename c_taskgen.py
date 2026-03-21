"""
Generate C evaluation tasks from data/CREval_data.jsonl.

Input record schema (minimal):
{
  "task_id": "CREval/0",
  "entry_point": "foo",
  "code": "... C source ...",
  "inputs": [
    {
      "harness": "int main(){ ... }",
      "output": "optional expected stdout"
    }
  ]
}

Optional per-input keys:
- probe_lines: [int, ...]
- probe_exprs: [{"lineno": int, "expr": "..."}, ...]
- output_pred: string
"""

from __future__ import annotations

import json
import re

import pandas as pd
from tqdm import tqdm

from c_dataset import CReval, validate_record_shape

COMMENT_LINE = re.compile(r"^\s*//")
ASSIGN = re.compile(r"=" )
IDENT = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")


def _strip_block_comments(code: str) -> str:
    return re.sub(r"/\*.*?\*/", "", code, flags=re.DOTALL)


def _is_candidate_stmt(line: str) -> bool:
    s = line.strip()
    if s == "" or COMMENT_LINE.match(s):
        return False
    if s in ("{", "}"):
        return False
    if s.startswith("#"):
        return False
    if not s.endswith(";"):
        return False
    return True


def _extract_probe_expr(line: str) -> str | None:
    s = line.strip().rstrip(";")
    if ASSIGN.search(s) and "==" not in s and "!=" not in s and ">=" not in s and "<=" not in s:
        lhs = s.split("=", 1)[0].strip()
        lhs = re.sub(r"\b(const|volatile|static|register|extern|unsigned|signed|long|short|struct|union|enum)\b", "", lhs)
        lhs = re.sub(r"\s+", " ", lhs).strip()
        parts = IDENT.findall(lhs)
        if parts:
            return parts[-1]
    if s.startswith("return "):
        expr = s[len("return "):].strip()
        if expr != "":
            return expr
    return None


def _auto_probes(code: str) -> list[dict[str, object]]:
    code = _strip_block_comments(code)
    probes: list[dict[str, object]] = []
    for idx, line in enumerate(code.splitlines(), start=1):
        if not _is_candidate_stmt(line):
            continue
        expr = _extract_probe_expr(line)
        if expr is None:
            continue
        probes.append({"lineno": idx, "expr": expr})
    return probes


def process_dataset(data_path="data/CREval_data.jsonl", out_path="data/CREval_tasks.jsonl"):
    with open(data_path, "r", encoding="utf-8") as f:
        df = pd.read_json(f, lines=True).to_dict(orient="records")

    out = []
    for ridx, rec in enumerate(tqdm(df)):
        validate_record_shape(rec)
        item = {
            "task_id": rec.get("task_id", f"CREval/{ridx}"),
            "idx": ridx,
            "tasks": [],
        }

        auto = _auto_probes(rec["code"])
        for input_idx, inp in enumerate(rec["inputs"]):
            if len(item["tasks"]) >= CReval.MAX_INPUTS:
                break

            probes = inp.get("probe_exprs")
            if probes is None:
                if "probe_lines" in inp:
                    probe_lines = set(inp["probe_lines"])
                    probes = [p for p in auto if p["lineno"] in probe_lines]
                else:
                    probes = auto

            task = [{"lineno": int(p["lineno"]), "var": str(p["expr"])} for p in probes]
            if len(task) == 0:
                continue

            output_pred = inp.get("output_pred")
            if output_pred is None:
                output_pred = f"/* Fill expected result */ {rec['entry_point']}(... ) == ??"

            item["tasks"].append(
                {
                    "input_idx": input_idx,
                    "task": task,
                    "output_pred": output_pred,
                }
            )

        out.append(item)

    with open(out_path, "w", encoding="utf-8") as f:
        for row in out:
            f.write(json.dumps(row, ensure_ascii=False) + "\n")


if __name__ == "__main__":
    process_dataset()
