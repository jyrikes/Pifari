"""Benchmarks repetidos de CPU: kernel e custo total do processo."""

from concurrent.futures import ThreadPoolExecutor, ProcessPoolExecutor
from statistics import median
from time import perf_counter
import ast
import re
import subprocess

N = 200_000
TASKS = [N, N, N, N]
REPETITIONS = 5


def soma_quadrados(limite: int) -> int:
    total = 0
    for i in range(limite):
        total += i * i
    return total


def medir_repetido(nome, funcao):
    tempos = []
    resultado = None
    for _ in range(REPETITIONS):
        inicio = perf_counter()
        resultado = funcao()
        tempos.append((perf_counter() - inicio) * 1000)
    print(
        f"{nome}: mediana {median(tempos):.2f} ms; amostras {[round(x, 2) for x in tempos]}"
    )
    return resultado


def medir_processos_frios():
    with ProcessPoolExecutor(max_workers=4) as processos:
        return list(processos.map(soma_quadrados, TASKS))


def medir_pifari():
    tempos_seq, tempos_par, tempos_processo = [], [], []
    resultado = None
    for _ in range(REPETITIONS):
        inicio = perf_counter()
        processo = subprocess.run(
            ["./pifari", "examples/benchmarks/benchmark_cpu.pif"],
            check=True,
            capture_output=True,
            text=True,
        )
        tempos_processo.append((perf_counter() - inicio) * 1000)
        saida = processo.stdout
        seq = re.search(r"Tempo sequencial \(ms\):\s*([0-9.]+)", saida)
        par = re.search(r"Tempo paralelo \(ms\):\s*([0-9.]+)", saida)
        res = re.search(r"Resultados:\s*(\[[^\n]*\])", saida)
        if not seq or not par or not res:
            raise RuntimeError("Saída inesperada do benchmark Pifari:\n" + saida)
        tempos_seq.append(float(seq.group(1)))
        tempos_par.append(float(par.group(1)))
        resultado = ast.literal_eval(res.group(1))
    print(
        f"Pifari kernel sequencial (JIT aquecido): mediana {median(tempos_seq):.3f} ms; amostras {[round(x, 3) for x in tempos_seq]}"
    )
    print(
        f"Pifari kernel paralelo (JIT + pool): mediana {median(tempos_par):.3f} ms; amostras {[round(x, 3) for x in tempos_par]}"
    )
    print(
        f"Pifari processo completo: mediana {median(tempos_processo):.2f} ms; primeira execução {tempos_processo[0]:.2f} ms; amostras {[round(x, 2) for x in tempos_processo]}"
    )
    return resultado


if __name__ == "__main__":
    resultado_pifari = medir_pifari()
    sequencial = medir_repetido(
        "Python sequencial (kernel)",
        lambda: [soma_quadrados(n) for n in TASKS],
    )
    with (
        ThreadPoolExecutor(max_workers=4) as threads,
        ProcessPoolExecutor(max_workers=4) as processos,
    ):
        list(threads.map(soma_quadrados, [1000] * 4))
        list(processos.map(soma_quadrados, [1000] * 4))
        resultado_threads = medir_repetido(
            "Python ThreadPoolExecutor (4 threads, kernel)",
            lambda: list(threads.map(soma_quadrados, TASKS)),
        )
        resultado_processos = medir_repetido(
            "Python ProcessPoolExecutor (4 processos, pool aquecido)",
            lambda: list(processos.map(soma_quadrados, TASKS)),
        )
    processos_frios = medir_repetido(
        "Python ProcessPoolExecutor (4 processos, inicialização + kernel)",
        medir_processos_frios,
    )
    assert (
        resultado_pifari
        == sequencial
        == resultado_threads
        == resultado_processos
        == processos_frios
    ), "Resultados divergentes!"
    print("Resultados:", sequencial)
    print("Validação: todos os resultados são iguais.")
