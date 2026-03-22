import argparse
import datetime as dt
import json
import os
import random
import re
import shutil
import subprocess
from collections import defaultdict
from dataclasses import dataclass

import lizard


SCRIPT_VERSION = "3.0.0"
DEFAULT_SRC_DIR = "./linux"
DEFAULT_OUTPUT_DIR = "./C-REval_Dataset"

TARGETS = {
    "1_High_Complexity": 50,
    "2_Goto_Paths": 50,
    "3_Kernel_Macros": 30,
    "4_Multi_Pointers": 40,
    "5_Concurrency_Locks": 30,
}

PROFILE_RULES = {
    "linux": {
        "macro_keywords": [
            "list_for_each_entry",
            "container_of",
        ],
        "lock_pairs": [
            ("spin_lock", "spin_unlock"),
        ],
    },
    "generic": {
        "macro_keywords": [
            "list_for_each",
            "list_for_each_entry",
            "container_of",
            "LIST_ENTRY",
            "TAILQ_FOREACH",
        ],
        "lock_pairs": [
            ("spin_lock", "spin_unlock"),
            ("mutex_lock", "mutex_unlock"),
            ("read_lock", "read_unlock"),
            ("write_lock", "write_unlock"),
            ("pthread_mutex_lock", "pthread_mutex_unlock"),
        ],
    },
}

POINTER_REBIND_HINTS = (
    r"\*\s*[A-Za-z_][A-Za-z0-9_]*\s*=\s*&\s*[A-Za-z_][A-Za-z0-9_]*",
    r"\*\s*[A-Za-z_][A-Za-z0-9_]*\s*=\s*(kmalloc|kzalloc|kcalloc|krealloc|vmalloc|vzalloc|malloc|calloc|realloc)\s*\(",
    r"\*\s*[A-Za-z_][A-Za-z0-9_]*\s*=\s*NULL\s*;",
    r"\*\s*[A-Za-z_][A-Za-z0-9_]*\s*=\s*[A-Za-z_][A-Za-z0-9_]*\s*;",
)


@dataclass
class Candidate:
    category: str
    repo_name: str
    source_abs: str
    source_rel: str
    subsystem: str
    function_name: str
    signature: str
    start_line: int
    end_line: int
    nloc: int
    ccn: int
    code: str
    goto_count: int
    has_macro: bool
    has_lock_pair: bool
    has_double_pointer: bool
    has_pointer_rebind: bool
    includes: list
    context_before: str
    context_after: str


def resolve_src_dir(src_dir):
    return os.path.abspath(src_dir)


def parse_source_roots(args):
    roots = []

    if args.src_root:
        for item in args.src_root:
            if "=" not in item:
                raise ValueError(f"--src-root 格式错误: {item}，应为 repo=path")
            repo_name, raw_path = item.split("=", 1)
            repo_name = repo_name.strip()
            repo_path = resolve_src_dir(raw_path.strip())
            if not repo_name:
                raise ValueError(f"--src-root repo 不能为空: {item}")
            roots.append({"repo": repo_name, "path": repo_path})
    else:
        src_dir = resolve_src_dir(args.src_dir)
        repo_name = os.path.basename(src_dir.rstrip(os.sep)) or "repo"
        roots.append({"repo": repo_name, "path": src_dir})

    dedup = {}
    for root in roots:
        dedup[root["repo"]] = root["path"]
    result = [{"repo": k, "path": v} for k, v in dedup.items()]

    for root in result:
        if not os.path.isdir(root["path"]):
            raise ValueError(f"源码目录不存在: {root['repo']}={root['path']}")

    return result


def setup_directories(output_dir):
    if os.path.exists(output_dir):
        shutil.rmtree(output_dir)
    os.makedirs(output_dir, exist_ok=True)
    for category in TARGETS:
        os.makedirs(os.path.join(output_dir, category), exist_ok=True)
    os.makedirs(os.path.join(output_dir, "metadata"), exist_ok=True)
    os.makedirs(os.path.join(output_dir, "splits"), exist_ok=True)


def read_file_lines(filepath):
    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
        return f.readlines()


def extract_function_code(lines, start_line, end_line):
    if start_line < 1 or end_line > len(lines) or start_line > end_line:
        return ""
    return "".join(lines[start_line - 1:end_line])


def extract_includes(lines, max_includes):
    includes = []
    for line in lines:
        stripped = line.strip()
        if stripped.startswith("#include"):
            includes.append(stripped)
            if len(includes) >= max_includes:
                break
    return includes


def get_context(lines, start_line, end_line, context_lines):
    before_start = max(0, start_line - 1 - context_lines)
    before_end = max(0, start_line - 1)
    after_start = min(len(lines), end_line)
    after_end = min(len(lines), end_line + context_lines)
    before = "".join(lines[before_start:before_end]).strip("\n")
    after = "".join(lines[after_start:after_end]).strip("\n")
    return before, after


def get_subsystem(source_rel):
    parts = source_rel.split(os.sep)
    return parts[0] if parts else "unknown"


def has_pointer_rebind(func_signature, func_code):
    if "**" not in func_signature and "**" not in func_code:
        return False, False

    has_double_pointer = True

    for pattern in POINTER_REBIND_HINTS:
        if re.search(pattern, func_code):
            return has_double_pointer, True

    double_ptr_params = re.findall(r"\*\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)", func_signature)
    for param in double_ptr_params:
        param_assign = rf"\*\s*{re.escape(param)}\s*=\s*[^;]+;"
        if re.search(param_assign, func_code):
            return has_double_pointer, True

    return has_double_pointer, False


def has_token(code, token):
    return re.search(rf"\b{re.escape(token)}\b", code) is not None


def detect_category(func, func_code, profile):
    rules = PROFILE_RULES.get(profile, PROFILE_RULES["linux"])
    macro_keywords = rules["macro_keywords"]
    lock_pairs = rules["lock_pairs"]

    goto_count = len(re.findall(r"\bgoto\b", func_code))
    has_macro = any(has_token(func_code, keyword) for keyword in macro_keywords)
    has_lock_pair = any(has_token(func_code, lk) and has_token(func_code, ulk) for lk, ulk in lock_pairs)
    has_double_pointer, has_pointer_rebind_flag = has_pointer_rebind(func.long_name, func_code)

    # 类别优先级：锁 -> 宏 -> 指针别名 -> goto -> 高复杂度
    if has_lock_pair:
        category = "5_Concurrency_Locks"
    elif has_macro:
        category = "3_Kernel_Macros"
    elif has_double_pointer and has_pointer_rebind_flag:
        category = "4_Multi_Pointers"
    elif goto_count >= 3:
        category = "2_Goto_Paths"
    elif func.cyclomatic_complexity > 20 and 100 <= func.nloc <= 300:
        category = "1_High_Complexity"
    else:
        category = None

    return category, goto_count, has_macro, has_lock_pair, has_double_pointer, has_pointer_rebind_flag


def collect_candidates(source_roots, min_nloc, max_nloc, context_lines, max_includes, candidate_quotas, max_c_files, seed, profile):
    candidates_by_category = defaultdict(list)
    scanned_c_files = 0
    scanned_functions = 0
    done = False
    rng = random.Random(seed)

    c_files = []
    for source_root in source_roots:
        repo_name = source_root["repo"]
        repo_path = source_root["path"]
        for root, _, files in os.walk(repo_path):
            for file in files:
                if file.endswith(".c"):
                    source_abs = os.path.join(root, file)
                    source_rel_in_repo = os.path.relpath(source_abs, repo_path)
                    c_files.append((repo_name, repo_path, source_abs, source_rel_in_repo))

    rng.shuffle(c_files)

    for repo_name, repo_path, source_abs, source_rel_in_repo in c_files:
        if done:
            break

        if max_c_files > 0 and scanned_c_files >= max_c_files:
            break

        scanned_c_files += 1
        source_rel = f"{repo_name}/{source_rel_in_repo}"

        try:
            lines = read_file_lines(source_abs)
            includes = extract_includes(lines, max_includes)
            analysis = lizard.analyze_file(source_abs)
        except Exception:
            continue

        for func in analysis.function_list:
            scanned_functions += 1
            if func.nloc < min_nloc or func.nloc > max_nloc:
                continue

            func_code = extract_function_code(lines, func.start_line, func.end_line)
            if not func_code:
                continue

            category, goto_count, has_macro, has_lock_pair, has_double_pointer, has_pointer_rebind_flag = detect_category(func, func_code, profile)
            if not category:
                continue

            context_before, context_after = get_context(lines, func.start_line, func.end_line, context_lines)

            if len(candidates_by_category[category]) < candidate_quotas.get(category, 0):
                candidates_by_category[category].append(
                    Candidate(
                        category=category,
                        repo_name=repo_name,
                        source_abs=source_abs,
                        source_rel=source_rel,
                        subsystem=get_subsystem(source_rel_in_repo),
                        function_name=func.name,
                        signature=func.long_name,
                        start_line=func.start_line,
                        end_line=func.end_line,
                        nloc=func.nloc,
                        ccn=func.cyclomatic_complexity,
                        code=func_code,
                        goto_count=goto_count,
                        has_macro=has_macro,
                        has_lock_pair=has_lock_pair,
                        has_double_pointer=has_double_pointer,
                        has_pointer_rebind=has_pointer_rebind_flag,
                        includes=includes,
                        context_before=context_before,
                        context_after=context_after,
                    )
                )

            if all(len(candidates_by_category[c]) >= candidate_quotas[c] for c in TARGETS):
                done = True
                break

    return candidates_by_category, scanned_c_files, scanned_functions


def balanced_select(candidates, target, seed, per_file_cap, max_repo_ratio):
    rng = random.Random(seed)
    by_bucket = defaultdict(list)
    for cand in candidates:
        bucket = f"{cand.repo_name}:{cand.subsystem}"
        by_bucket[bucket].append(cand)

    for bucket in by_bucket:
        rng.shuffle(by_bucket[bucket])

    buckets = sorted(by_bucket.keys())
    rng.shuffle(buckets)

    selected = []
    used = set()
    file_counts = defaultdict(int)
    repo_counts = defaultdict(int)
    repo_cap = max(1, int(target * max_repo_ratio + 0.9999))

    # 第一轮：轮转各 bucket(仓库+子系统)，避免单仓库或单子系统主导
    progress = True
    while len(selected) < target and progress:
        progress = False
        for bucket in buckets:
            while by_bucket[bucket]:
                cand = by_bucket[bucket].pop()
                key = (cand.source_rel, cand.function_name, cand.start_line)
                if key in used:
                    continue
                file_key = f"{cand.repo_name}:{cand.source_rel}"
                if file_counts[file_key] >= per_file_cap:
                    continue
                if repo_counts[cand.repo_name] >= repo_cap:
                    continue
                selected.append(cand)
                used.add(key)
                file_counts[file_key] += 1
                repo_counts[cand.repo_name] += 1
                progress = True
                break
            if len(selected) >= target:
                break

    # 第二轮：如果还不够，放宽每文件上限补齐
    if len(selected) < target:
        leftovers = []
        for bucket in buckets:
            leftovers.extend(by_bucket[bucket])
        rng.shuffle(leftovers)
        for cand in leftovers:
            key = (cand.source_rel, cand.function_name, cand.start_line)
            if key in used:
                continue
            selected.append(cand)
            used.add(key)
            repo_counts[cand.repo_name] += 1
            if len(selected) >= target:
                break

    return selected[:target]


def to_comment_block(title, content):
    if not content:
        return f"/* {title}: <empty> */\n"
    lines = content.splitlines()
    block = [f"/* {title}"]
    for line in lines:
        block.append(f" * {line}")
    block.append(" */")
    return "\n".join(block) + "\n"


def write_samples(output_dir, selected_by_category, anonymize, include_context):
    manifest_rows = []
    sample_counter = 0

    for category in TARGETS:
        category_dir = os.path.join(output_dir, category)
        selected = selected_by_category.get(category, [])

        for idx, cand in enumerate(selected, start=1):
            sample_counter += 1
            sample_id = f"{category}_{idx:03d}"
            if anonymize:
                filename = f"{sample_id}.c"
            else:
                filename = f"{os.path.basename(cand.source_rel)}_{cand.function_name}.c"

            save_path = os.path.join(category_dir, filename)

            with open(save_path, "w", encoding="utf-8") as f:
                f.write("/* C-REval Sample */\n")
                f.write(f"/* Sample-ID: {sample_id} */\n")
                f.write(f"/* Category: {category} */\n")
                f.write(f"/* Repo: {cand.repo_name} */\n")
                f.write(f"/* Cyclomatic Complexity: {cand.ccn} */\n")
                f.write(f"/* NLOC: {cand.nloc} */\n")
                f.write(f"/* Subsystem: {cand.subsystem} */\n")
                if not anonymize:
                    f.write(f"/* Source: {cand.source_rel} */\n")
                    f.write(f"/* Function: {cand.function_name} */\n")

                if include_context:
                    f.write(to_comment_block("Includes", "\n".join(cand.includes)))
                    f.write(to_comment_block("Context-Before", cand.context_before))

                f.write(cand.code)
                if not cand.code.endswith("\n"):
                    f.write("\n")

                if include_context:
                    f.write(to_comment_block("Context-After", cand.context_after))

            manifest_rows.append(
                {
                    "sample_id": sample_id,
                    "category": category,
                    "sample_path": os.path.relpath(save_path, output_dir),
                    "repo": cand.repo_name,
                    "source_rel": cand.source_rel,
                    "function_name": cand.function_name,
                    "subsystem": cand.subsystem,
                    "start_line": cand.start_line,
                    "end_line": cand.end_line,
                    "nloc": cand.nloc,
                    "ccn": cand.ccn,
                    "goto_count": cand.goto_count,
                    "has_macro": cand.has_macro,
                    "has_lock_pair": cand.has_lock_pair,
                    "has_double_pointer": cand.has_double_pointer,
                    "has_pointer_rebind": cand.has_pointer_rebind,
                }
            )

    return manifest_rows, sample_counter


def write_manifest_and_metadata(output_dir, manifest_rows, stats, args, source_roots):
    manifest_path = os.path.join(output_dir, "metadata", "manifest.jsonl")
    with open(manifest_path, "w", encoding="utf-8") as f:
        for row in manifest_rows:
            f.write(json.dumps(row, ensure_ascii=False) + "\n")

    public_manifest_path = os.path.join(output_dir, "metadata", "manifest_public.jsonl")
    with open(public_manifest_path, "w", encoding="utf-8") as f:
        for row in manifest_rows:
            public_row = {
                "sample_id": row["sample_id"],
                "category": row["category"],
                "sample_path": row["sample_path"],
                "repo": row["repo"],
                "subsystem": row["subsystem"],
                "nloc": row["nloc"],
                "ccn": row["ccn"],
                "goto_count": row["goto_count"],
                "has_macro": row["has_macro"],
                "has_lock_pair": row["has_lock_pair"],
                "has_double_pointer": row["has_double_pointer"],
                "has_pointer_rebind": row["has_pointer_rebind"],
            }
            f.write(json.dumps(public_row, ensure_ascii=False) + "\n")

    public_source_roots = [
        {
            "repo": root["repo"],
            "path_hint": os.path.basename(root["path"].rstrip(os.sep)) or root["repo"],
        }
        for root in source_roots
    ]

    metadata = {
        "script_version": SCRIPT_VERSION,
        "generated_at": dt.datetime.utcnow().isoformat() + "Z",
        "source_roots": public_source_roots,
        "repo_commits": {root["repo"]: get_git_commit(root["path"]) for root in source_roots},
        "seed": args.seed,
        "targets": TARGETS,
        "parameters": {
            "min_nloc": args.min_nloc,
            "max_nloc": args.max_nloc,
            "context_lines": args.context_lines,
            "max_includes": args.max_includes,
            "per_file_cap": args.per_file_cap,
            "max_repo_ratio": args.max_repo_ratio,
            "anonymize": args.anonymize,
            "include_context": args.include_context,
        },
        "stats": stats,
    }

    metadata_path = os.path.join(output_dir, "metadata", "dataset_metadata.json")
    with open(metadata_path, "w", encoding="utf-8") as f:
        json.dump(metadata, f, ensure_ascii=False, indent=2)


def split_dataset(output_dir, manifest_rows, seed, train_ratio=0.8, dev_ratio=0.1):
    rng = random.Random(seed)
    by_category = defaultdict(list)
    for row in manifest_rows:
        by_category[row["category"]].append(row)

    train, dev, test = [], [], []
    for category in TARGETS:
        rows = by_category.get(category, [])
        rng.shuffle(rows)

        n = len(rows)
        n_train = int(n * train_ratio)
        n_dev = int(n * dev_ratio)
        n_test = n - n_train - n_dev

        train.extend(r["sample_path"] for r in rows[:n_train])
        dev.extend(r["sample_path"] for r in rows[n_train:n_train + n_dev])
        test.extend(r["sample_path"] for r in rows[n_train + n_dev:n_train + n_dev + n_test])

    for split_name, items in (("train.txt", train), ("dev.txt", dev), ("test.txt", test)):
        split_path = os.path.join(output_dir, "splits", split_name)
        with open(split_path, "w", encoding="utf-8") as f:
            for item in items:
                f.write(item + "\n")

    return {"train": len(train), "dev": len(dev), "test": len(test)}


def get_git_commit(src_dir):
    try:
        out = subprocess.check_output(
            ["git", "-C", src_dir, "rev-parse", "HEAD"],
            stderr=subprocess.DEVNULL,
            text=True,
        ).strip()
        return out
    except Exception:
        return "unknown"


def print_category_stats(selected_by_category):
    print("\n当前收集进度:")
    for category in TARGETS:
        count = len(selected_by_category.get(category, []))
        print(f"- {category}: {count}/{TARGETS[category]}")


def main():
    parser = argparse.ArgumentParser(description="科研版 C-REval 抽取器：去泄漏、均衡采样、上下文增强、可复现")
    parser.add_argument("--src-dir", default=DEFAULT_SRC_DIR, help="Linux 源码目录，例如 /path/to/linux")
    parser.add_argument("--src-root", action="append", help="多仓库输入，可重复传入，格式 repo=path")
    parser.add_argument("--output-dir", default=DEFAULT_OUTPUT_DIR, help="输出目录")
    parser.add_argument("--seed", type=int, default=42, help="随机种子")
    parser.add_argument("--profile", choices=["linux", "generic"], default="linux", help="规则配置：linux 或 generic")
    parser.add_argument("--min-nloc", type=int, default=15, help="函数最小 NLOC")
    parser.add_argument("--max-nloc", type=int, default=350, help="函数最大 NLOC")
    parser.add_argument("--context-lines", type=int, default=20, help="前后文行数")
    parser.add_argument("--max-includes", type=int, default=20, help="最多保留 include 条目")
    parser.add_argument("--per-file-cap", type=int, default=2, help="每个源文件在同一类别最多选中样本数")
    parser.add_argument("--max-repo-ratio", type=float, default=0.7, help="单仓库在单类别内的最高占比，默认 0.7")
    parser.add_argument("--candidate-multiplier", type=int, default=8, help="候选冗余倍数，越大分布越均衡但扫描更慢")
    parser.add_argument("--max-c-files", type=int, default=0, help="最多扫描 C 文件数，0 表示不限制")
    parser.add_argument("--anonymize", action="store_true", default=True, help="匿名化输出（默认开启）")
    parser.add_argument("--no-anonymize", dest="anonymize", action="store_false", help="关闭匿名化")
    parser.add_argument("--include-context", action="store_true", default=True, help="在样本中附带 include/前后文注释（默认开启）")
    parser.add_argument("--no-context", dest="include_context", action="store_false", help="不附带上下文")
    args = parser.parse_args()

    output_dir = os.path.abspath(args.output_dir)

    try:
        source_roots = parse_source_roots(args)
    except ValueError as e:
        print(f"[错误] {e}")
        return

    print("开始扫描目录:")
    for root in source_roots:
        print(f"- {root['repo']}: {root['path']}")
    setup_directories(output_dir)

    candidate_quotas = {
        category: TARGETS[category] * max(1, args.candidate_multiplier)
        for category in TARGETS
    }

    candidates_by_category, scanned_c_files, scanned_functions = collect_candidates(
        source_roots=source_roots,
        min_nloc=args.min_nloc,
        max_nloc=args.max_nloc,
        context_lines=args.context_lines,
        max_includes=args.max_includes,
        candidate_quotas=candidate_quotas,
        max_c_files=args.max_c_files,
        seed=args.seed,
        profile=args.profile,
    )

    selected_by_category = {}
    for category, target in TARGETS.items():
        candidates = candidates_by_category.get(category, [])
        selected = balanced_select(
            candidates=candidates,
            target=target,
            seed=args.seed + sum(ord(ch) for ch in category),
            per_file_cap=args.per_file_cap,
            max_repo_ratio=args.max_repo_ratio,
        )
        selected_by_category[category] = selected

    manifest_rows, total_samples = write_samples(
        output_dir=output_dir,
        selected_by_category=selected_by_category,
        anonymize=args.anonymize,
        include_context=args.include_context,
    )

    split_stats = split_dataset(output_dir, manifest_rows, args.seed)

    category_counts = {category: len(selected_by_category.get(category, [])) for category in TARGETS}
    coverage = {
        category: round((category_counts[category] / TARGETS[category]) * 100, 2)
        if TARGETS[category] > 0 else 0.0
        for category in TARGETS
    }

    stats = {
        "scanned_c_files": scanned_c_files,
        "scanned_functions": scanned_functions,
        "total_samples": total_samples,
        "category_counts": category_counts,
        "coverage_percent": coverage,
        "split_counts": split_stats,
        "repos": sorted(list({row["repo"] for row in manifest_rows})),
    }

    write_manifest_and_metadata(output_dir, manifest_rows, stats, args, source_roots)

    print("\n抽取完成")
    print(f"扫描统计: C文件={scanned_c_files}, 函数={scanned_functions}")
    print_category_stats(selected_by_category)
    print(f"\n切分统计: train={split_stats['train']}, dev={split_stats['dev']}, test={split_stats['test']}")
    print(f"元数据: {os.path.join(output_dir, 'metadata', 'dataset_metadata.json')}")
    print(f"清单(内部): {os.path.join(output_dir, 'metadata', 'manifest.jsonl')}")
    print(f"清单(公开): {os.path.join(output_dir, 'metadata', 'manifest_public.jsonl')}")


if __name__ == "__main__":
    main()