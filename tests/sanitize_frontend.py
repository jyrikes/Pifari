#!/usr/bin/env python3
"""Instrument lexer/parser/type checker; abort if a rejected program reaches runtime.

Run after make. This does not instrument or certify the execution backend.
"""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="pifari-frontend-asan-") as tmp:
    tmp = Path(tmp)
    stub = tmp / "runtime_guard.c"
    stub.write_text('#include "frontend/ast.h"\n'
                    'int runtime_execute(Node *root) { (void)root; abort(); }\n')
    binary = tmp / "pifari-frontend-asan"
    subprocess.run([
        "gcc", "-std=c11", "-O1", "-g", "-no-pie",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
        "-I.", "-Ibuild/generated", "build/generated/parser.tab.c",
        "build/generated/lex.yy.c", "backend/typecheck.c", "backend/types.c",
        str(stub), "-o", str(binary),
    ], cwd=ROOT, check=True)
    env = dict(os.environ, PIFARI_TEST_BINARY=str(binary),
               ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
               UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    methods = ["test_invalid_programs", "test_no_partial_execution",
               "test_bounded_diagnostics", "test_indentation_limit",
               "test_semantic_accumulation"]
    subprocess.run([sys.executable, "tests/test_error_recovery.py",
                    *("RecoveryTests." + method for method in methods)],
                   cwd=ROOT, env=env, check=True)
    # Also exercise successful parser teardown and start-symbol ownership.
    source = tmp / "valid.pif"
    source.write_text("x = 2\nif true\n    print(x + 1)\n")
    subprocess.run([str(binary), "--check", str(source)],
                   cwd=ROOT, env=env, check=True)
print("frontend ASan/UBSan/LSan: all selected checks passed")
