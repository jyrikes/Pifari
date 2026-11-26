"""Verifica os requisitos mínimos da disciplina; execute após make."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
cases = [
    ("precedencia", "print(2 + 3 * 4)\nprint(2 ** 3 ** 2)\n", [], 0, "14\n512"),
    ("variaveis", "x = 2\nx = x + 5\nprint(x)\n", [], 0, "7"),
    ("condicional", "x = 2\nif x > 1 {\n print(10)\n} else {\n print(20)\n}\n", [], 0, "10"),
    ("repeticao", "s = 0\nfor i in range(5) {\n s = s + i\n}\nprint(s)\n", [], 0, "10"),
    ("lexico", "print(1 ~ 2)\n", [], 1, "caractere inesperado"),
    ("sintatico", "print(1 + )\n", [], 1, "erro sintático"),
    ("nome_inexistente", "print(inexistente)\n", ["--check"], 2, "erro semântico"),
    ("nome_execucao", "print(inexistente)\n", [], 1, "erro"),
    ("tipo_incompativel", 'x =: int("texto")\n', ["--check"], 2, "erro semântico"),
    ("divisao_zero", "print(1 / 0)\n", [], 1, "zero"),
]
failed = 0
with tempfile.TemporaryDirectory() as tmp:
    for name, source, flags, expected_rc, expected in cases:
        path = Path(tmp) / (name + ".pif")
        path.write_text(source, encoding="utf-8")
        result = subprocess.run([str(ROOT / "pifari"), *flags, str(path)], cwd=ROOT,
                                capture_output=True, text=True, timeout=15)
        output = (result.stdout + result.stderr).strip()
        ok = result.returncode == expected_rc and (output == expected if expected_rc == 0 else expected in output)
        print(f"{'PASS' if ok else 'FAIL'} {name}: rc={result.returncode} {output!r}")
        failed += not ok
print(f"{len(cases)-failed}/{len(cases)} casos aprovados")
raise SystemExit(bool(failed))
