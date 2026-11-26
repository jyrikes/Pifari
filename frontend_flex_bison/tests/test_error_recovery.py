#!/usr/bin/env python3
"""Recovery is diagnostic only: every malformed input must remain non-executable."""
from pathlib import Path
import os
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
BINARY = os.environ.get("PIFARI_TEST_BINARY", str(ROOT / "pifari"))

# name, source, diagnostic fragments (including positions where relevant)
INVALID = [
    ("lexical_multiple", "~\n$\n", ["Pifari:1:1:", "Pifari:2:1:"]),
    ("lexical_inside_expression", "print(1 ~ + 2)\n$\n", ["Pifari:1:9:", "Pifari:2:1:"]),
    ("panic_lines", "x =\ny =\n", ["Pifari:1:4:", "Pifari:2:4:"]),
    ("local_calls", "print(1 + )\nprint(2 + )\n", ["Pifari:1:11:", "Pifari:2:11:"]),
    ("local_casts", "int(1 + )\nint(2 + )\n", ["Pifari:1:9:", "Pifari:2:9:"]),
    ("local_groups", "x = (1 + )\ny = (2 + )\n", ["Pifari:1:10:", "Pifari:2:10:"]),
    ("local_lists", "x = [1 + ]\ny = [2 + ]\n", ["Pifari:1:10:", "Pifari:2:10:"]),
    ("same_line", "print(1 + ) print(2 + )\n", ["Pifari:1:11:", "Pifari:1:23:"]),
    ("missing_parenthesis", "print(1\nx =\n", ["Pifari:1:8:", "Pifari:2:4:"]),
    ("missing_bracket", "x = [1\ny =\n", ["Pifari:1:7:", "Pifari:2:4:"]),
    ("closing_brace", "if true { x = }\ny =\n", ["Pifari:1:15:", "Pifari:2:4:"]),
    ("closing_dedent", "if true\n    x =\ny =\n", ["Pifari:2:8:", "Pifari:3:4:"]),
    ("nested_blocks", "if true {\n if true { x = }\n y =\n}\nz =\n", ["Pifari:2:16:", "Pifari:3:5:", "Pifari:5:4:"]),
    ("trailing_comma_call", "print(1,)\nx =\n", ["Pifari:1:8:", "expressão ausente após ','", "Pifari:2:4:"]),
    ("trailing_comma_list", "x = [1,]\ny =\n", ["Pifari:1:7:", "expressão ausente após ','", "Pifari:2:4:"]),
    ("bad_assignment", "1 = 2\nx =\n", ["lado esquerdo", "Pifari:2:4:"]),
    ("unterminated_string", 'x = "texto\ny =\n', ["Pifari:1:5: erro léxico: texto não terminado", "Pifari:2:4:"]),
    ("unterminated_single_string", "x = 'texto\ny =\n", ["texto não terminado", "Pifari:2:4:"]),
    ("unterminated_escape", 'x = "texto\\\ny =\n', ["texto não terminado", "Pifari:2:4:"]),
    ("unterminated_comment", '@""" texto\noutra linha\n', ["Pifari:1:1: erro léxico: comentário multilinha não terminado"]),
    ("mixed_indent", "if true\n \tprint(1)\nx =\n", ["mistura espaços e tabulações", "Pifari:3:4:"]),
    ("inconsistent_dedent", "if true\n    print(1)\n  print(2)\nx =\n", ["nível de indentação inconsistente", "Pifari:4:4:"]),
    ("crlf", "x =\r\ny =\r\n", ["Pifari:1:4:", "Pifari:2:4:"]),
    ("eof", "x =", ["Pifari:1:4:", "erro sintático"]),
    ("eof_call", "print(1 +", ["Pifari:1:10:", "erro sintático"]),
    ("eof_string", 'x = "abc', ["texto não terminado"]),
    ("eof_block", "if true {", ["erro sintático"]),
    ("partial_function", "function f(long_name, other: int", ["erro sintático"]),
]


class RecoveryTests(unittest.TestCase):
    def run_source(self, source, flags=()):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "case.pif"
            path.write_bytes(source.encode("utf-8"))
            result = subprocess.run([BINARY, *flags, str(path)], cwd=ROOT,
                                    capture_output=True, text=True, timeout=5)
        self.assertNotIn("Sanitizer", result.stderr)
        return result

    def test_invalid_programs(self):
        for name, source, messages in INVALID:
            for flags in [(), ("--check",)]:
                with self.subTest(case=name, flags=flags):
                    r = self.run_source(source, flags)
                    self.assertEqual(r.returncode, 1, r.stderr)
                    self.assertEqual(r.stdout, "", r.stdout)
                    for message in messages:
                        self.assertIn(message, r.stderr)

    def test_no_partial_execution(self):
        for _, source, _ in INVALID:
            with self.subTest(source=source):
                r = self.run_source('print("MUST_NOT_EXECUTE")\n' + source)
                self.assertEqual(r.returncode, 1, r.stderr)
                self.assertEqual(r.stdout, "")

    def test_bounded_diagnostics(self):
        for source in ["~" * 1000, "x =\n" * 1000, "print(1,)\n" * 1000]:
            with self.subTest(source=source[:20]):
                r = self.run_source(source)
                self.assertEqual(r.returncode, 1, r.stderr)
                self.assertEqual(len(re.findall(r": erro (?:léxico|sintático):", r.stderr)), 50)
                self.assertEqual(r.stderr.count("limite de 50 erros"), 1)
                self.assertEqual(r.stdout, "")

    def test_indentation_limit(self):
        source = "".join(" " * i + "if true\n" for i in range(257))
        r = self.run_source(source)
        self.assertEqual(r.returncode, 1, r.stderr)
        self.assertIn("blocos aninhados demais", r.stderr)

    def test_valid_regressions(self):
        sources = [
            ("print(2 + 3 * 4)\n", "14\n"),
            ("if true\n    print(1)\n    \n    print(2)\nprint(3)\n", "1\n2\n3\n"),
            ("if true\n    print(1)\n@ comentário sem indentação\n    print(2)\nprint(3)\n", "1\n2\n3\n"),
            ("if true\r\n    print(1)\r\n    \r\n    print(2)\r\nprint(3)\r\n", "1\n2\n3\n"),
            ('@""" texto na abertura\ntexto ignorado\n"""@\nprint(7)\n', "7\n"),
            ('@ comentário\n@"aspas em comentário\nprint(8)\n', "8\n"),
            ('print("texto", \'outro\')\n', "texto outro\n"),
            ("if true\n    print(4)", "4\n"),
            ("print(5)", "5\n"),
            ("", ""),
        ]
        for source, output in sources:
            with self.subTest(source=source):
                r = self.run_source(source)
                self.assertEqual(r.returncode, 0, r.stderr)
                self.assertEqual(r.stdout, output)
                self.assertEqual(r.stderr, "")

    def test_semantic_accumulation(self):
        r = self.run_source("print(ausente_a)\nprint(ausente_b)\n", ("--check",))
        self.assertEqual(r.returncode, 2, r.stderr)
        self.assertEqual(r.stderr.count("erro semântico"), 2)
        self.assertIn("ausente_a", r.stderr)
        self.assertIn("ausente_b", r.stderr)
        self.assertEqual(r.stdout, "")


if __name__ == "__main__":
    unittest.main(verbosity=2)
