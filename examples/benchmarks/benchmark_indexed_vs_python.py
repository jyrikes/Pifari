#!/usr/bin/env python3
"""Compara indexação aritmética em Pifari com Python, usando o mesmo vetor."""

from __future__ import annotations
import re
import statistics
import subprocess
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
N = 50_000
PIFARI_RUNS = 3
PYTHON_RUNS = 7


def pifari_sample() -> tuple[float, float, float, list[str]]:
    proc = subprocess.run(
        ["./pifari", "examples/benchmarks/benchmark_indexed_native.pif"],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )
    if proc.returncode:
        raise RuntimeError(
            f"Pifari falhou (exit {proc.returncode}):\n{proc.stdout}\n{proc.stderr}"
        )
    cold = re.search(r"cold_ns\s+(\d+)", proc.stdout)
    native = re.search(r"native_ns\s+(\d+)", proc.stdout)
    interpreted = re.search(r"interpreted_ns\s+(\d+)", proc.stdout)
    if not cold or not native or not interpreted:
        raise RuntimeError(
            f"Não encontrei os tempos no output do Pifari:\n{proc.stdout}"
        )
    expected = [
        f"length {N}",
        "first 3",
        f"last {2 * (N - 1) + 3}",
        "same true",
        "parallel_first 3",
        f"parallel_last {2 * (N - 1) + 3}",
    ]
    for line in expected:
        if line not in proc.stdout:
            raise RuntimeError(
                f"Validação falhou: esperado {line!r}. Output:\n{proc.stdout}"
            )
    return (
        int(cold.group(1)) / 1e6,
        int(native.group(1)) / 1e6,
        int(interpreted.group(1)) / 1e6,
        proc.stdout.splitlines(),
    )


def python_loop(data: list[float]) -> list[float]:
    out = [0.0] * len(data)
    for i in range(len(data)):
        out[i] = data[i] * 2 + 3
    return out


def python_comprehension(data: list[float]) -> list[float]:
    return [data[i] * 2 + 3 for i in range(len(data))]


def median_ms(fn, data: list[float]) -> float:
    samples = []
    expected_first, expected_last = 3, 2 * (len(data) - 1) + 3
    for _ in range(PYTHON_RUNS):
        start = time.perf_counter_ns()
        result = fn(data)
        samples.append((time.perf_counter_ns() - start) / 1e6)
        if (
            result[0] != expected_first
            or result[-1] != expected_last
            or len(result) != len(data)
        ):
            raise RuntimeError("Resultado Python incorreto")
    return statistics.median(samples)


def main() -> None:
    p_cold, p_native, p_interpreted = [], [], []
    for _ in range(PIFARI_RUNS):
        cold, native, interpreted, _ = pifari_sample()
        p_cold.append(cold)
        p_native.append(native)
        p_interpreted.append(interpreted)
    p_cold_ms = statistics.median(p_cold)
    p_native_ms = statistics.median(p_native)
    p_interpreted_ms = statistics.median(p_interpreted)

    data = [float(i) for i in range(N)]
    py_loop_ms = median_ms(python_loop, data)
    py_comp_ms = median_ms(python_comprehension, data)

    print(
        f"Benchmark de indexação nativa — {N:,} elementos; mediana de {PIFARI_RUNS} processos Pifari e {PYTHON_RUNS} repetições Python"
    )
    print("Todas as implementações calculam out[i] = data[i] * 2 + 3.")
    print(f"Pifari primeira chamada (compilação + execução): {p_cold_ms:.3f} ms")
    print(f"Pifari JIT indexado (kernel aquecido): {p_native_ms:.3f} ms")
    print(f"Pifari interpretado (índice + array_set): {p_interpreted_ms:.3f} ms")
    print(f"Python for + lista pré-alocada:       {py_loop_ms:.3f} ms")
    print(f"Python list comprehension:            {py_comp_ms:.3f} ms")
    print(
        f"Speedup JIT vs Pifari interpretado:   {p_interpreted_ms / p_native_ms:.2f}x"
    )
    print(f"Speedup JIT vs Python for:             {py_loop_ms / p_native_ms:.2f}x")
    print(f"Speedup JIT vs Python comprehension:  {py_comp_ms / p_native_ms:.2f}x")
    print(
        "Nota: a medição aquecida exclui a compilação; a primeira chamada é reportada separadamente. Python usa float/listas built-in, sem NumPy."
    )


if __name__ == "__main__":
    main()
