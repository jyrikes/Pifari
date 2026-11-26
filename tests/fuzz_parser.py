#!/usr/bin/env python3
"""Deterministic smoke-fuzzer for parser/runtime robustness; signal exits fail."""

import os
import random
import subprocess
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BINARY = os.path.join(ROOT, "pifari")
RNG = random.Random(20261019)
TOKENS = [
    "x",
    "y",
    "0",
    "1",
    "2.5",
    '"text"',
    "true",
    "false",
    "+",
    "-",
    "*",
    "/",
    "==",
    "(",
    ")",
    "[",
    "]",
    "{",
    "}",
    ",",
    ":",
    "=",
    "if",
    "else",
    "for",
    "in",
    "return",
    "print",
    ">|",
    "|>",
    "and",
    "or",
    "\n",
    "    ",
    "# fuzz",
]


def generate_case():
    return (
        "".join(
            RNG.choice(TOKENS) + ("" if RNG.random() < 0.35 else " ")
            for _ in range(RNG.randint(1, 45))
        )
        + "\n"
    )


def main():
    for index in range(120):
        source = generate_case()
        with tempfile.NamedTemporaryFile(
            "w", suffix=".pif", delete=False, encoding="utf-8"
        ) as file:
            file.write(source)
            path = file.name
        try:
            result = subprocess.run(
                [BINARY, path],
                cwd=ROOT,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                timeout=2,
            )
            if result.returncode < 0:
                raise SystemExit(f"crash/signal on case {index}: {source!r}")
        except subprocess.TimeoutExpired:
            raise SystemExit(f"timeout on case {index}: {source!r}")
        finally:
            os.unlink(path)
    print("parser fuzz smoke: 120 inputs, no crashes/timeouts")


if __name__ == "__main__":
    main()
