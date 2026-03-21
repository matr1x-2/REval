from __future__ import annotations

import os
import re
import shlex
import subprocess
import tempfile
from dataclasses import dataclass
from pathlib import Path


@dataclass
class CRunResult:
    stdout: str
    stderr: str
    exit_code: int


class CCompileError(RuntimeError):
    pass


class CTraceError(RuntimeError):
    pass


class CSandbox:
    """
    Compile and run C code + harness, then optionally collect execution traces via gdb.

    State values are captured at breakpoint hit time (before executing that line),
    which is consistent and robust for pointer/struct/function-pointer expressions.
    """

    def __init__(
        self,
        code: str,
        harness: str,
        compile_flags: list[str] | None = None,
        entry_file: str = "target.c",
        timeout_sec: int = 20,
    ):
        self.code = code
        self.harness = harness
        self.compile_flags = compile_flags or ["-std=gnu11", "-O0", "-g"]
        self.entry_file = entry_file
        self.timeout_sec = timeout_sec

    def _write_program(self, workdir: Path) -> tuple[Path, Path]:
        src = workdir / self.entry_file
        exe = workdir / "a.out"
        with open(src, "w", encoding="utf-8") as f:
            f.write(self.code)
            if not self.code.endswith("\n"):
                f.write("\n")
            f.write("\n")
            f.write(self.harness)
            if not self.harness.endswith("\n"):
                f.write("\n")
        return src, exe

    def _compile(self, src: Path, exe: Path) -> None:
        cmd = ["gcc", str(src), "-o", str(exe), *self.compile_flags]
        proc = subprocess.run(cmd, capture_output=True, text=True)
        if proc.returncode != 0:
            raise CCompileError(proc.stderr.strip() or proc.stdout.strip())

    def run_program(self) -> CRunResult:
        with tempfile.TemporaryDirectory(prefix="creval_") as td:
            workdir = Path(td)
            src, exe = self._write_program(workdir)
            self._compile(src, exe)
            proc = subprocess.run(
                [str(exe)],
                capture_output=True,
                text=True,
                timeout=self.timeout_sec,
                cwd=workdir,
            )
            return CRunResult(proc.stdout, proc.stderr, proc.returncode)

    def collect_line_trace(self, probe_lines: list[int]) -> list[int]:
        if len(probe_lines) == 0:
            return []
        with tempfile.TemporaryDirectory(prefix="creval_") as td:
            workdir = Path(td)
            src, exe = self._write_program(workdir)
            self._compile(src, exe)
            gdb_script = workdir / "trace.gdb"
            target_name = src.name
            with open(gdb_script, "w", encoding="utf-8") as f:
                f.write("set pagination off\n")
                f.write("set confirm off\n")
                f.write("set breakpoint pending on\n")
                for line in sorted(set(probe_lines)):
                    f.write(f"break {target_name}:{line}\n")
                    f.write("commands\n")
                    f.write("  silent\n")
                    f.write(f"  printf \"DREVAL_HIT {line}\\n\"\n")
                    f.write("  continue\n")
                    f.write("end\n")
                f.write("run\n")
                f.write("quit\n")

            proc = subprocess.run(
                ["gdb", "-q", "-batch", "-x", str(gdb_script), str(exe)],
                capture_output=True,
                text=True,
                timeout=self.timeout_sec,
                cwd=workdir,
            )
            if proc.returncode not in (0, 1):
                raise CTraceError(proc.stderr.strip() or proc.stdout.strip())

            hits = []
            for m in re.finditer(r"DREVAL_HIT\s+(-?\d+)", proc.stdout):
                hits.append(int(m.group(1)))
            return hits

    def collect_expr_values(self, lineno: int, expr: str) -> list[str]:
        """
        Capture expression values whenever breakpoint at lineno is hit.
        Values are gdb-rendered strings.
        """
        with tempfile.TemporaryDirectory(prefix="creval_") as td:
            workdir = Path(td)
            src, exe = self._write_program(workdir)
            self._compile(src, exe)
            gdb_script = workdir / "state.gdb"
            target_name = src.name
            safe_expr = expr.replace('"', '\\"')
            with open(gdb_script, "w", encoding="utf-8") as f:
                f.write("set pagination off\n")
                f.write("set confirm off\n")
                f.write("set breakpoint pending on\n")
                f.write(f"break {target_name}:{lineno}\n")
                f.write("commands\n")
                f.write("  silent\n")
                f.write("  printf \"DREVAL_STATE_BEGIN\\n\"\n")
                f.write(f"  print {safe_expr}\n")
                f.write("  printf \"DREVAL_STATE_END\\n\"\n")
                f.write("  continue\n")
                f.write("end\n")
                f.write("run\n")
                f.write("quit\n")

            proc = subprocess.run(
                ["gdb", "-q", "-batch", "-x", str(gdb_script), str(exe)],
                capture_output=True,
                text=True,
                timeout=self.timeout_sec,
                cwd=workdir,
            )
            if proc.returncode not in (0, 1):
                raise CTraceError(proc.stderr.strip() or proc.stdout.strip())

            values = []
            chunks = proc.stdout.split("DREVAL_STATE_BEGIN")
            for chunk in chunks[1:]:
                seg = chunk.split("DREVAL_STATE_END", 1)[0]
                m = re.search(r"\$\d+\s*=\s*(.*)", seg)
                if m:
                    values.append(m.group(1).strip())
            return values


def normalize_scalar_text(v: str) -> str:
    s = v.strip()
    s = re.sub(r"\s+", " ", s)
    return s
