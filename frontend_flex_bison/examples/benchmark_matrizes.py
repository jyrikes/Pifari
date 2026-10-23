"""Compara matrizes por laços interpretados, primitiva nativa e Python puro."""
import re
import subprocess
from statistics import median
from time import perf_counter

N = 60
REPETITIONS = 3


def construir():
    a = [[i + j for j in range(N)] for i in range(N)]
    b = [[i - j for j in range(N)] for i in range(N)]
    return a, b


def multiplicar(a, b):
    c = [[0] * N for _ in range(N)]
    for i in range(N):
        for j in range(N):
            total = 0
            for k in range(N):
                total += a[i][k] * b[k][j]
            c[i][j] = total
    return c


def medir_python():
    a, b = construir()
    tempos = []
    checksum = None
    for _ in range(REPETITIONS):
        inicio = perf_counter()
        c = multiplicar(a, b)
        tempos.append((perf_counter() - inicio) * 1000)
        checksum = c[0][0] + c[N - 1][N - 1]
    return tempos, checksum


def medir_pifari():
    tempos_puros, tempos_nativos = [], []
    checksum_puro = checksum_nativo = None
    padrao_puro = re.compile(
        r"PIFARI matrix interpretado N=\s*(\d+)\s+ms=\s*([0-9.]+)\s+checksum=\s*(-?\d+)"
    )
    padrao_nativo = re.compile(
        r"PIFARI matrix nativo N=\s*(\d+)\s+ms=\s*([0-9.]+)\s+checksum=\s*(-?\d+)"
    )
    for _ in range(REPETITIONS):
        proc = subprocess.run(
            ["./pifari", "examples/benchmark_matrizes.pif"],
            check=True, capture_output=True, text=True,
        )
        puro = padrao_puro.search(proc.stdout)
        nativo = padrao_nativo.search(proc.stdout)
        if not puro or not nativo:
            raise RuntimeError("Saída inesperada do benchmark de matriz:\n" + proc.stdout)
        if int(puro.group(1)) != N or int(nativo.group(1)) != N:
            raise RuntimeError("Dimensão divergente no benchmark Pifari")
        tempos_puros.append(float(puro.group(2)))
        tempos_nativos.append(float(nativo.group(2)))
        checksum_puro = int(puro.group(3))
        checksum_nativo = int(nativo.group(3))
    if checksum_puro != checksum_nativo:
        raise AssertionError(f"Pifari puro/nativo divergiram: {checksum_puro} != {checksum_nativo}")
    return tempos_puros, tempos_nativos, checksum_puro


if __name__ == "__main__":
    pifari_puros, pifari_nativos, checksum_pifari = medir_pifari()
    python_tempos, python_checksum = medir_python()
    print(f"Pifari matriz interpretada N={N}: mediana {median(pifari_puros):.3f} ms; amostras {[round(x, 3) for x in pifari_puros]}")
    print(f"Pifari matmul nativo N={N}: mediana {median(pifari_nativos):.3f} ms; amostras {[round(x, 3) for x in pifari_nativos]}")
    print(f"Python matriz pura N={N}: mediana {median(python_tempos):.2f} ms; amostras {[round(x, 3) for x in python_tempos]}")
    assert checksum_pifari == python_checksum, (
        f"Checksum divergente: Pifari={checksum_pifari}, Python={python_checksum}"
    )
    print(f"Checksum validado: {python_checksum}")
