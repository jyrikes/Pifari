"""Lexer de referência para o subconjunto definido da linguagem Pifari."""

from dataclasses import dataclass


@dataclass(frozen=True)
class Token:
    kind: str
    value: str
    line: int
    column: int


class LexError(SyntaxError):
    pass


KEYWORDS = {
    "if": "IF",
    "else": "ELSE",
    "elseif": "ELSEIF",
    "for": "FOR",
    "while": "WHILE",
    "break": "BREAK",
    "continue": "CONTINUE",
    "return": "RETURN",
    "import": "IMPORT",
    "Link": "LINK",
    "to": "TO",
    "agent": "AGENT",
    "task": "TASK",
    "async": "ASYNC",
    "await": "AWAIT",
    "spawn": "SPAWN",
    "send": "SEND",
    "receive": "RECEIVE",
    "close": "CLOSE",
    "try": "TRY",
    "catch": "CATCH",
    "finally": "FINALLY",
    "throw": "THROW",
    "raise": "RAISE",
    "match": "MATCH",
    "case": "CASE",
    "const": "CONST",
    "dynamic": "DYNAMIC",
    "true": "BOOL",
    "false": "BOOL",
    "and": "AND",
    "or": "OR",
    "not": "NOT",
    "in": "IN",
    "Null": "NULL",
    "null": "NULL",
    "None": "NONE",
}
TYPES = {
    "void",
    "bool",
    "char",
    "byte",
    "short",
    "int",
    "long",
    "float",
    "double",
    "signed",
    "unsigned",
    "txt",
    "Text",
    "Int",
    "Float",
    "Bool",
    "List",
    "Map",
    "Set",
    "Tuple",
    "Range",
    "Task",
    "Agent",
}
OPERATORS = [
    ("|<", "JOIN"),
    ("<|", "APPLY"),
    (">|", "FANOUT"),
    ("|>", "PIPE"),
    (":=", "CLASS_DEF"),
    ("=:", "TYPED_DECL"),
    ("->", "ARROW"),
    ("==", "EQ"),
    ("!=", "NE"),
    ("<=", "LE"),
    (">=", "GE"),
    ("//", "IDIV"),
    ("**", "POWER"),
    ("+", "PLUS"),
    ("-", "MINUS"),
    ("*", "STAR"),
    ("/", "SLASH"),
    ("%", "PERCENT"),
    ("=", "ASSIGN"),
    ("<", "LT"),
    (">", "GT"),
    (".", "DOT"),
    (",", "COMMA"),
    (":", "COLON"),
    (";", "SEMICOLON"),
    ("(", "LPAREN"),
    (")", "RPAREN"),
    ("[", "LBRACKET"),
    ("]", "RBRACKET"),
    ("{", "LBRACE"),
    ("}", "RBRACE"),
]


class Lexer:
    def __init__(self, source):
        self.source = source.replace("\r\n", "\n").replace("\r", "\n")
        self.tokens = []
        self.indents = [0]

    def tokenize(self):
        lines = self.source.split("\n")
        in_block_comment = False
        for line_no, raw in enumerate(lines, 1):
            # Suporte multilinha: @""" inicia e """@ encerra; conteúdo é ignorado.
            if in_block_comment:
                if '"""@' in raw:
                    raw = raw.split('"""@', 1)[1]
                    in_block_comment = False
                else:
                    continue
            if '@"""' in raw:
                before, after = raw.split('@"""', 1)
                if '"""@' in after:
                    raw = before + after.split('"""@', 1)[1]
                else:
                    raw = before
                    in_block_comment = True
            self._scan_line(raw, line_no)
        while len(self.indents) > 1:
            self.indents.pop()
            self.tokens.append(Token("DEDENT", "", len(lines), 1))
        self.tokens.append(Token("EOF", "", len(lines), 1))
        return self.tokens

    def _scan_line(self, raw, line_no):
        prefix = len(raw) - len(raw.lstrip(" \t"))
        text = raw[prefix:]
        if not text.strip() or text.startswith("@"):
            return
        indent_text = raw[:prefix]
        if " " in indent_text and "\t" in indent_text:
            raise LexError(
                f"linha {line_no}, coluna 1: não misture espaços e tabulações"
            )
        indent = sum(4 if c == "\t" else 1 for c in indent_text)
        if indent > self.indents[-1]:
            self.indents.append(indent)
            self.tokens.append(Token("INDENT", "", line_no, 1))
        elif indent < self.indents[-1]:
            while len(self.indents) > 1 and indent < self.indents[-1]:
                self.indents.pop()
                self.tokens.append(Token("DEDENT", "", line_no, 1))
            if indent != self.indents[-1]:
                raise LexError(
                    f"linha {line_no}: indentação não corresponde a bloco anterior"
                )
        i, n = prefix, len(raw)
        while i < n:
            c = raw[i]
            if raw.startswith("#include", i):
                self.tokens.append(Token("INCLUDE", "#include", line_no, i + 1))
                i += 8
                continue
            if c in " \t":
                i += 1
                continue
            if c == "@":
                break
            if c in "\"'":
                quote, start = c, i
                i += 1
                value = ""
                while i < n:
                    if raw[i] == "\\" and i + 1 < n:
                        esc = raw[i + 1]
                        value += {
                            "n": "\n",
                            "t": "\t",
                            "r": "\r",
                            "\\": "\\",
                            quote: quote,
                        }.get(esc, esc)
                        i += 2
                    elif raw[i] == quote:
                        i += 1
                        break
                    else:
                        value += raw[i]
                        i += 1
                else:
                    raise LexError(
                        f"linha {line_no}, coluna {start + 1}: string não terminada"
                    )
                self.tokens.append(Token("STRING", value, line_no, start + 1))
                continue
            if c.isdigit():
                start = i
                while i < n and raw[i].isdigit():
                    i += 1
                kind = "INT"
                if i + 1 < n and raw[i] == "." and raw[i + 1].isdigit():
                    i += 1
                    while i < n and raw[i].isdigit():
                        i += 1
                    kind = "FLOAT"
                self.tokens.append(Token(kind, raw[start:i], line_no, start + 1))
                continue
            if c.isalpha() or c == "_":
                start = i
                i += 1
                while i < n and (raw[i].isalnum() or raw[i] == "_"):
                    i += 1
                value = raw[start:i]
                kind = KEYWORDS.get(value, "TYPE" if value in TYPES else "IDENT")
                self.tokens.append(Token(kind, value, line_no, start + 1))
                continue
            match = next(
                ((op, kind) for op, kind in OPERATORS if raw.startswith(op, i)), None
            )
            if match:
                op, kind = match
                self.tokens.append(Token(kind, op, line_no, i + 1))
                i += len(op)
                continue
            raise LexError(
                f"linha {line_no}, coluna {i + 1}: caractere inesperado {c!r}"
            )
        if (
            self.tokens
            and self.tokens[-1].line == line_no
            and self.tokens[-1].kind not in {"NEWLINE", "INDENT", "DEDENT"}
        ):
            self.tokens.append(Token("NEWLINE", "", line_no, n + 1))


def lex(source):
    return Lexer(source).tokenize()
