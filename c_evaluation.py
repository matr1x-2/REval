from __future__ import annotations

import argparse
import json
import os
import re
from datetime import datetime

import pandas as pd
from tqdm import tqdm

from c_dataset import CReval, validate_record_shape
from c_dynamics import CSandbox, normalize_scalar_text
from c_inference import APIModel
from c_prompt import build_cot_prompt, build_direct_prompt


def now_tag() -> str:
    return datetime.now().strftime("%y-%m-%d-%H-%M")


def parse_answer_block(text: str) -> str:
    s = text.strip()
    m = re.search(r"\[ANSWER\](.*?)\[/ANSWER\]", s, flags=re.DOTALL | re.IGNORECASE)
    if m:
        return m.group(1).strip()
    return s


class CTaskBase:
    def __init__(self, name: str, cfg: dict):
        self.name = name
        self.cfg = cfg
        self.prompt_type = cfg.get("prompt_type", "direct")
        self.model = APIModel(
            model_id=cfg["model_id"],
            api_key=cfg.get("api_key"),
            api_base=cfg.get("api_base"),
            temperature=float(cfg.get("temperature", 0.0)),
            max_tokens=int(cfg.get("max_tokens", 768)),
            system_prompt=cfg.get("system_prompt"),
        )
        self.data_path = cfg.get("data_path", "data/CREval_data.jsonl")
        self.task_path = cfg.get("task_path", "data/CREval_tasks.jsonl")
        self.save_dir = cfg.get("save_dir", "model_generations_c")

        self.data = pd.read_json(self.data_path, lines=True).to_dict("records")
        self.tasks = pd.read_json(self.task_path, lines=True).to_dict("records")
        for rec in self.data:
            validate_record_shape(rec)

        self.rows = []

    def _prompt(self, **kwargs) -> str:
        if self.prompt_type == "cot":
            return build_cot_prompt(self.name, **kwargs)
        return build_direct_prompt(self.name, **kwargs)

    def _infer(self, p: str) -> tuple[str, str]:
        raw = self.model.infer(p)
        return parse_answer_block(raw), raw

    def _record_dir(self) -> str:
        d = os.path.join(self.save_dir, f"{self.name}@{self.model.info}")
        os.makedirs(d, exist_ok=True)
        return d


class CoverageTask(CTaskBase):
    def __init__(self, cfg: dict):
        super().__init__("coverage", cfg)
        self.correct = 0
        self.total = 0

    def _yes(self, ans: str) -> bool:
        s = ans.strip().upper()
        if s.startswith("YES"):
            return True
        if s.startswith("NO"):
            return False
        return False

    def run(self):
        for task_row in tqdm(self.tasks):
            idx = task_row["idx"]
            rec = self.data[idx]
            code = rec["code"]
            item_logs = {"task_id": task_row["task_id"], "generation": []}
            for pair in task_row["tasks"]:
                input_idx = pair["input_idx"]
                inp = rec["inputs"][input_idx]
                sandbox = CSandbox(
                    code=code,
                    harness=inp["harness"],
                    compile_flags=rec.get("compile_flags", ["-std=gnu11", "-O0", "-g"]),
                    timeout_sec=int(self.cfg.get("exec_timeout", 20)),
                )
                probe_lines = [t["lineno"] for t in pair["task"]]
                trace = sandbox.collect_line_trace(probe_lines)
                hits = set(trace)

                results = []
                for t in pair["task"]:
                    line = t["lineno"]
                    codeline = code.splitlines()[line - 1] if line - 1 < len(code.splitlines()) else ""
                    p = self._prompt(
                        code=code,
                        invocation=inp["harness"],
                        invocation_abbr="the C harness",
                        line=line,
                        codeline=codeline,
                    )
                    ans, raw = self._infer(p)
                    pred = self._yes(ans)
                    expected = line in hits
                    self.total += 1
                    if pred == expected:
                        self.correct += 1
                    results.append({"generated": raw, "response": pred, "expected": expected})
                item_logs["generation"].append({"input_idx": input_idx, "results": results})
            self.rows.append(item_logs)

        metric = {"acc": self.correct / self.total if self.total else 0.0}
        self.rows.append(metric)
        out = os.path.join(self._record_dir(), f"{now_tag()}.jsonl")
        with open(out, "w", encoding="utf-8") as f:
            for r in self.rows:
                f.write(json.dumps(r, ensure_ascii=False) + "\n")
        print(metric)


class PathTask(CTaskBase):
    def __init__(self, cfg: dict):
        super().__init__("path", cfg)
        self.correct = 0
        self.total = 0

    def _to_line(self, ans: str) -> int:
        s = ans.strip()
        m = re.search(r"-?\d+", s)
        if not m:
            return -2
        return int(m.group(0))

    def _next_lines(self, trace: list[int], line: int) -> set[int]:
        idxs = [i for i, x in enumerate(trace) if x == line]
        if len(idxs) == 0:
            return {-1}
        out = set()
        for i in idxs:
            out.add(trace[i + 1] if i + 1 < len(trace) else -1)
        return out

    def run(self):
        for task_row in tqdm(self.tasks):
            idx = task_row["idx"]
            rec = self.data[idx]
            code = rec["code"]
            item_logs = {"task_id": task_row["task_id"], "generation": []}
            for pair in task_row["tasks"]:
                input_idx = pair["input_idx"]
                inp = rec["inputs"][input_idx]
                sandbox = CSandbox(
                    code=code,
                    harness=inp["harness"],
                    compile_flags=rec.get("compile_flags", ["-std=gnu11", "-O0", "-g"]),
                    timeout_sec=int(self.cfg.get("exec_timeout", 20)),
                )
                probe_lines = [t["lineno"] for t in pair["task"]]
                trace = sandbox.collect_line_trace(probe_lines)

                results = []
                for t in pair["task"]:
                    line = t["lineno"]
                    codeline = code.splitlines()[line - 1] if line - 1 < len(code.splitlines()) else ""
                    p = self._prompt(
                        code=code,
                        invocation=inp["harness"],
                        invocation_abbr="the C harness",
                        line=line,
                        codeline=codeline,
                    )
                    ans, raw = self._infer(p)
                    pred = self._to_line(ans)
                    expected = sorted(self._next_lines(trace, line))
                    self.total += 1
                    if pred in expected:
                        self.correct += 1
                    results.append({"generated": raw, "response": pred, "expected": expected})
                item_logs["generation"].append({"input_idx": input_idx, "results": results})
            self.rows.append(item_logs)

        metric = {"acc": self.correct / self.total if self.total else 0.0}
        self.rows.append(metric)
        out = os.path.join(self._record_dir(), f"{now_tag()}.jsonl")
        with open(out, "w", encoding="utf-8") as f:
            for r in self.rows:
                f.write(json.dumps(r, ensure_ascii=False) + "\n")
        print(metric)


class StateTask(CTaskBase):
    def __init__(self, cfg: dict):
        super().__init__("state", cfg)
        self.correct = 0
        self.total = 0

    def _eq(self, ans: str, expected: list[str]) -> bool:
        a = normalize_scalar_text(ans)
        e = [normalize_scalar_text(x) for x in expected]
        return a in e

    def run(self):
        for task_row in tqdm(self.tasks):
            idx = task_row["idx"]
            rec = self.data[idx]
            code = rec["code"]
            item_logs = {"task_id": task_row["task_id"], "generation": []}
            for pair in task_row["tasks"]:
                input_idx = pair["input_idx"]
                inp = rec["inputs"][input_idx]
                sandbox = CSandbox(
                    code=code,
                    harness=inp["harness"],
                    compile_flags=rec.get("compile_flags", ["-std=gnu11", "-O0", "-g"]),
                    timeout_sec=int(self.cfg.get("exec_timeout", 20)),
                )

                results = []
                for t in pair["task"]:
                    line = t["lineno"]
                    var = t["var"]
                    codeline = code.splitlines()[line - 1] if line - 1 < len(code.splitlines()) else ""
                    p = self._prompt(
                        code=code,
                        invocation=inp["harness"],
                        invocation_abbr="the C harness",
                        line=line,
                        codeline=codeline,
                        var=var,
                    )
                    ans, raw = self._infer(p)
                    expected_vals = sandbox.collect_expr_values(line, var)
                    ok = self._eq(ans, expected_vals)
                    self.total += 1
                    if ok:
                        self.correct += 1
                    results.append({"generated": raw, "response": ans, "expected": expected_vals, "eq": ok})
                item_logs["generation"].append({"input_idx": input_idx, "results": results})
            self.rows.append(item_logs)

        metric = {"acc": self.correct / self.total if self.total else 0.0}
        self.rows.append(metric)
        out = os.path.join(self._record_dir(), f"{now_tag()}.jsonl")
        with open(out, "w", encoding="utf-8") as f:
            for r in self.rows:
                f.write(json.dumps(r, ensure_ascii=False) + "\n")
        print(metric)


class OutputTask(CTaskBase):
    def __init__(self, cfg: dict):
        super().__init__("output", cfg)
        self.correct = 0
        self.total = 0

    def _eq(self, ans: str, expected: str) -> bool:
        return normalize_scalar_text(ans) == normalize_scalar_text(expected)

    def run(self):
        for task_row in tqdm(self.tasks):
            idx = task_row["idx"]
            rec = self.data[idx]
            code = rec["code"]
            item_logs = {"task_id": task_row["task_id"], "generation": []}
            for pair in task_row["tasks"]:
                input_idx = pair["input_idx"]
                inp = rec["inputs"][input_idx]
                expected = inp.get("output")
                if expected is None:
                    sandbox = CSandbox(
                        code=code,
                        harness=inp["harness"],
                        compile_flags=rec.get("compile_flags", ["-std=gnu11", "-O0", "-g"]),
                        timeout_sec=int(self.cfg.get("exec_timeout", 20)),
                    )
                    run_res = sandbox.run_program()
                    expected = run_res.stdout

                p = self._prompt(code=code, invocation=inp["harness"])
                ans, raw = self._infer(p)
                ok = self._eq(ans, expected)
                self.total += 1
                if ok:
                    self.correct += 1
                item_logs["generation"].append(
                    {
                        "input_idx": input_idx,
                        "results": [
                            {
                                "generated": raw,
                                "response": ans,
                                "expected": expected,
                                "pass": ok,
                            }
                        ],
                    }
                )
            self.rows.append(item_logs)

        metric = {"acc": self.correct / self.total if self.total else 0.0}
        self.rows.append(metric)
        out = os.path.join(self._record_dir(), f"{now_tag()}.jsonl")
        with open(out, "w", encoding="utf-8") as f:
            for r in self.rows:
                f.write(json.dumps(r, ensure_ascii=False) + "\n")
        print(metric)


TASKS = {
    "coverage": CoverageTask,
    "path": PathTask,
    "state": StateTask,
    "output": OutputTask,
}


class ConsistencyTask:
    def __init__(self, cfg: dict):
        self.cfg = cfg
        self.save_dir = cfg.get("save_dir", "model_generations_c")
        self.model_info = APIModel(
            model_id=cfg["model_id"],
            api_key=cfg.get("api_key"),
            api_base=cfg.get("api_base"),
            temperature=float(cfg.get("temperature", 0.0)),
            max_tokens=int(cfg.get("max_tokens", 768)),
            system_prompt=cfg.get("system_prompt"),
        ).info

    def _latest(self, task_name: str) -> str:
        import glob

        path = os.path.join(self.save_dir, f"{task_name}@{self.model_info}", "*.jsonl")
        files = glob.glob(path)
        if len(files) == 0:
            raise FileNotFoundError(f"No generation logs found for task: {task_name}")
        return max(files, key=os.path.getctime)

    def _load_atomic(self, task_name: str) -> list[bool]:
        path = self._latest(task_name)
        rows = pd.read_json(path, lines=True).to_dict("records")
        out = []
        for i, row in enumerate(rows):
            if i == len(rows) - 1:
                break
            for g in row["generation"]:
                for r in g["results"]:
                    if task_name == "coverage":
                        out.append(r["response"] == r["expected"])
                    elif task_name == "path":
                        out.append(r["response"] in r["expected"])
                    elif task_name == "state":
                        out.append(bool(r.get("eq", False)))
                    elif task_name == "output":
                        out.append(bool(r.get("pass", False)))
                    else:
                        raise ValueError(task_name)
        return out

    def run(self):
        cov = self._load_atomic("coverage")
        path = self._load_atomic("path")
        state = self._load_atomic("state")
        out = self._load_atomic("output")

        n = min(len(cov), len(path), len(state), len(out))
        if n == 0:
            print({"consistency": 0.0, "n": 0})
            return

        score = 0.0
        for i in range(n):
            if cov[i] and state[i] and path[i] and out[i]:
                score += 1.0
            elif cov[i] and state[i] and path[i] and not out[i]:
                score += 0.5
            elif cov[i] and state[i] and not path[i] and not out[i]:
                score += 0.25
            elif cov[i] and not state[i] and not path[i] and not out[i]:
                score += 0.125

        print({"consistency": score / n, "n": n})


TASKS["consistency"] = ConsistencyTask


def load_cfg(path: str) -> dict:
    if not os.path.exists(path):
        raise FileNotFoundError(path)
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def save_cfg_template(path: str):
    cfg = {
        "task": "coverage",
        "prompt_type": "direct",
        "model_id": "YOUR_MODEL_NAME",
        "api_key": "YOUR_API_KEY",
        "api_base": "https://your-openai-compatible-endpoint/v1",
        "temperature": 0.0,
        "max_tokens": 768,
        "exec_timeout": 20,
        "data_path": "data/CREval_data.jsonl",
        "task_path": "data/CREval_tasks.jsonl",
        "save_dir": "model_generations_c",
        "system_prompt": "You are an expert in C systems programming, low-level execution semantics, and debugging.",
    }
    with open(path, "w", encoding="utf-8") as f:
        json.dump(cfg, f, indent=2)
    print(f"Saved config template to {path}")


def main():
    parser = argparse.ArgumentParser(description="Run C REval-like evaluation")
    parser.add_argument("command", nargs="?", default="run", choices=["run", "init-config"])
    parser.add_argument("-i", "--input", default=".c_eval_config.json")
    args = parser.parse_args()

    if args.command == "init-config":
        save_cfg_template(args.input)
        return

    cfg = load_cfg(args.input)
    task_name = cfg.get("task", "coverage")
    if task_name not in TASKS:
        raise ValueError(f"Unsupported task: {task_name}")
    task = TASKS[task_name](cfg)
    task.run()


if __name__ == "__main__":
    main()
