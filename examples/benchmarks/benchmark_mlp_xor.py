"""Compare pure-scalar MLP training in Pifari and Python (no ML/numeric third-party libs)."""

import re
import statistics
import subprocess
import sys

PIFARI_RUNS = 5


def run_pifari():
    samples = []
    last = ""
    for _ in range(PIFARI_RUNS):
        p = subprocess.run(
            ["./pifari", "examples/mlp_xor.pif"],
            check=True,
            capture_output=True,
            text=True,
        )
        last = p.stdout
        m = re.search(r"train_ms\s+([0-9.]+)", last)
        if not m:
            raise RuntimeError("Pifari output missing train_ms:\n" + last)
        if "train_accuracy_pct 100" not in last or "test_accuracy_pct 100" not in last:
            raise RuntimeError("Pifari MLP failed accuracy validation:\n" + last)
        samples.append(float(m.group(1)))
    return statistics.median(samples), samples, last


def run_python():
    p = subprocess.run(
        [sys.executable, "examples/mlp_xor.py"],
        check=True,
        capture_output=True,
        text=True,
    )
    out = p.stdout
    m = re.search(r"train_ms_median\s+([0-9.]+)", out)
    if (
        not m
        or "train_accuracy_pct 100" not in out
        or "test_accuracy_pct 100" not in out
    ):
        raise RuntimeError("Python MLP failed validation:\n" + out)
    return float(m.group(1)), out


def main():
    pif_ms, samples, pif_out = run_pifari()
    py_ms, py_out = run_python()
    print("MLP XOR 2-4-1 | 3000 epochs | scalar loops | no third-party libraries")
    print("Pifari training medians (ms):", ", ".join(f"{x:.3f}" for x in samples))
    print(f"Pifari median training: {pif_ms:.3f} ms")
    print(f"Python median training: {py_ms:.3f} ms")
    if pif_ms > 0 and py_ms > 0:
        ratio = pif_ms / py_ms
        print(f"Python/Pifari time ratio: {ratio:.2f}x")
        if ratio >= 1:
            print(f"Python speedup: {ratio:.2f}x")
        else:
            print(f"Pifari speedup: {1 / ratio:.2f}x")
    print("\n--- Pifari metrics ---")
    for line in pif_out.splitlines():
        if line.startswith(("epochs ", "train_accuracy_pct ", "test_accuracy_pct ")):
            print(line)
    print("\n--- Python metrics ---")
    for line in py_out.splitlines():
        if line.startswith(("epochs ", "train_accuracy_pct ", "test_accuracy_pct ")):
            print(line)
    print(
        "\nValidation: both implementations reached 100% on the four training and four held-out coordinate samples."
    )


if __name__ == "__main__":
    main()
