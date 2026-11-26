"""Parser recursivo descendente: constrói AST, não executa código."""

from dataclasses import dataclass, field
from lexer import lex


class ParseError(SyntaxError):
    pass


@dataclass
class Node:
    kind: str
    value: object = None
    children: list = field(default_factory=list)
    line: int = 0
    column: int = 0


class Parser:
    PRECEDENCE = {
        "OR": 1,
        "AND": 2,
        "EQ": 3,
        "NE": 3,
        "LT": 3,
        "LE": 3,
        "GT": 3,
        "GE": 3,
        "PIPE": 4,
        "APPLY": 4,
        "FANOUT": 4,
        "JOIN": 4,
        "PLUS": 5,
        "MINUS": 5,
        "STAR": 6,
        "SLASH": 6,
        "IDIV": 6,
        "PERCENT": 6,
        "POWER": 7,
    }

    def __init__(self, tokens):
        self.tokens = tokens
        self.i = 0

    @property
    def tok(self):
        return self.tokens[self.i]

    def at(self, *kinds):
        return self.tok.kind in kinds

    def take(self, kind=None):
        t = self.tok
        if kind and t.kind != kind:
            raise ParseError(
                f"linha {t.line}, coluna {t.column}: esperado {kind}, encontrado {t.kind}"
            )
        self.i += 1
        return t

    def accept(self, kind):
        if self.at(kind):
            return self.take()
        return None

    def skip_newlines(self):
        while self.accept("NEWLINE"):
            pass

    def parse(self):
        body = []
        self.skip_newlines()
        while not self.at("EOF"):
            if self.accept("DEDENT"):
                continue
            body.append(self.statement())
            self.skip_newlines()
        return Node("Program", children=body, line=1, column=1)

    def statement(self):
        t = self.tok
        if self.at("IF"):
            return self.if_stmt()
        if self.at("TYPED_DECL"):
            return self.member_stmt()
        if self.at("FOR"):
            return self.for_stmt()
        if self.at("CLASS_DEF"):
            return self.class_stmt()
        if self.at("IMPORT", "LINK", "INCLUDE"):
            return self.directive()
        if self.at("RETURN"):
            self.take()
            value = None if self.at("NEWLINE", "DEDENT", "EOF") else self.expression()
            self.end_statement()
            return Node(
                "Return",
                children=[] if value is None else [value],
                line=t.line,
                column=t.column,
            )
        if self.at("BREAK", "CONTINUE"):
            op = self.take()
            self.end_statement()
            return Node(op.value.title(), line=op.line, column=op.column)
        if self.at("IDENT") and self.tokens[self.i + 1].kind in {
            "ASSIGN",
            "TYPED_DECL",
        }:
            name = self.take()
            if self.accept("ASSIGN"):
                val = self.expression()
                self.end_statement()
                return Node("Assign", name.value, [val], name.line, name.column)
            self.take("TYPED_DECL")
            typ = self.type_expr()
            val = None
            if self.accept("LPAREN"):
                if not self.at("RPAREN"):
                    val = self.expression()
                self.take("RPAREN")
            self.end_statement()
            return Node(
                "TypedDecl",
                {"name": name.value, "type": typ.value},
                [] if val is None else [val],
                name.line,
                name.column,
            )
        expr = self.expression()
        if (
            self.at("NEWLINE")
            and self.i + 1 < len(self.tokens)
            and self.tokens[self.i + 1].kind == "INDENT"
        ):
            self.take("NEWLINE")
            yes = self.block()
            no = []
            self.skip_newlines()
            if self.accept("ELSE"):
                self.header_end()
                no = self.block()
            return Node(
                "ImplicitIf",
                children=[
                    expr,
                    Node("Block", children=yes),
                    Node("Block", children=no),
                ],
                line=t.line,
                column=t.column,
            )
        self.end_statement()
        if (
            expr.kind == "Binary"
            and expr.value == "|>"
            and expr.children[0].kind == "Binary"
            and expr.children[0].value == "|>"
            and expr.children[0].children[1].kind == "Name"
        ):
            inner = expr.children[0]
            return Node(
                "ForEachPipe",
                {"name": inner.children[1].value},
                [inner.children[0], expr.children[1]],
                t.line,
                t.column,
            )
        return Node(
            "ExpressionStatement", children=[expr], line=t.line, column=t.column
        )

    def end_statement(self):
        if self.at("NEWLINE"):
            self.take()
        elif not self.at("DEDENT", "RBRACE", "EOF"):
            t = self.tok
            raise ParseError(
                f"linha {t.line}, coluna {t.column}: esperado fim da instrução"
            )

    def header_end(self):
        if self.at("COLON"):
            self.take()
        if self.at("NEWLINE"):
            self.take()
        else:
            raise ParseError(f"linha {self.tok.line}: esperado fim de cabeçalho")

    def block(self):
        if self.accept("LBRACE"):
            self.skip_newlines()
            out = []
            while not self.at("RBRACE", "EOF"):
                while self.accept("INDENT") or self.accept("DEDENT"):
                    pass
                if self.at("RBRACE", "EOF"):
                    break
                out.append(self.statement())
                self.skip_newlines()
            self.take("RBRACE")
            return out
        self.skip_newlines()
        self.take("INDENT")
        out = []
        self.skip_newlines()
        while not self.at("DEDENT", "EOF"):
            out.append(self.statement())
            self.skip_newlines()
        self.take("DEDENT")
        return out

    def if_stmt(self):
        t = self.take("IF")
        cond = self.expression()
        self.header_end()
        yes = self.block()
        no = []
        self.skip_newlines()
        if self.accept("ELSEIF"):
            self.i -= 1
            no = [self.if_stmt()]
        elif self.accept("ELSE"):
            self.header_end()
            no = self.block()
        return Node(
            "If",
            children=[cond, Node("Block", children=yes), Node("Block", children=no)],
            line=t.line,
            column=t.column,
        )

    def for_stmt(self):
        t = self.take("FOR")
        name = self.take("IDENT")
        self.take("IN")
        iterable = self.expression()
        self.header_end()
        return Node(
            "For",
            {"name": name.value},
            [iterable, Node("Block", children=self.block())],
            t.line,
            t.column,
        )

    def class_stmt(self):
        t = self.take("CLASS_DEF")
        name = self.take("IDENT")
        self.header_end()
        return Node("Class", {"name": name.value}, self.block(), t.line, t.column)

    def member_stmt(self):
        t = self.take("TYPED_DECL")
        name = self.take("IDENT")
        if self.accept("LPAREN"):
            params = []
            if not self.at("RPAREN"):
                params.append(self.take("IDENT").value)
                while self.accept("COMMA"):
                    params.append(self.take("IDENT").value)
            self.take("RPAREN")
            if self.at("TYPE"):
                self.take()
            self.header_end()
            body = self.block()
            return Node(
                "Method", {"name": name.value, "params": params}, body, t.line, t.column
            )
        typ = self.type_expr()
        value = None
        if self.accept("ASSIGN"):
            value = self.expression()
        self.end_statement()
        return Node(
            "Attribute",
            {"name": name.value, "type": typ.value},
            [] if value is None else [value],
            t.line,
            t.column,
        )

    def directive(self):
        t = self.take()
        if t.kind == "IMPORT":
            name = self.take()
            if name.kind not in {"IDENT", "STRING"}:
                raise ParseError(f"linha {name.line}: esperado módulo após import")
            self.end_statement()
            return Node("Import", name.value, line=t.line, column=t.column)
        if t.kind == "INCLUDE":
            if self.accept("LT"):
                parts = []
                while not self.at("GT", "EOF"):
                    parts.append(self.take().value)
                self.take("GT")
                target = "".join(parts)
            else:
                target = self.take("STRING").value
            self.end_statement()
            return Node("Include", target, line=t.line, column=t.column)
        self.take("TO")
        target = self.take()
        if target.kind not in {"STRING", "IDENT"}:
            raise ParseError(
                f"linha {target.line}: esperado biblioteca/caminho após Link to"
            )
        self.end_statement()
        return Node("LinkTo", target.value, line=t.line, column=t.column)

    def type_expr(self):
        t = self.take()
        if t.kind not in {"TYPE", "IDENT"}:
            raise ParseError(f"linha {t.line}: esperado tipo")
        parts = [t.value]
        while self.at("TYPE", "IDENT") and parts[-1] in {
            "unsigned",
            "signed",
            "long",
            "short",
        }:
            parts.append(self.take().value)
        if self.accept("LBRACKET"):
            args = [self.type_expr()]
            while self.accept("COMMA"):
                args.append(self.type_expr())
            self.take("RBRACKET")
            return Node("Type", parts[0], args, t.line, t.column)
        return Node("Type", " ".join(parts), line=t.line, column=t.column)

    def expression(self, min_prec=0):
        left = self.prefix()
        while True:
            if self.at("LPAREN"):
                left = self.call(left)
                continue
            if self.accept("DOT"):
                name = self.take("IDENT")
                left = Node("Member", name.value, [left], name.line, name.column)
                continue
            if self.accept("LBRACKET"):
                idx = self.expression()
                self.take("RBRACKET")
                left = Node(
                    "Index", children=[left, idx], line=left.line, column=left.column
                )
                continue
            op = self.tok
            prec = self.PRECEDENCE.get(op.kind, -1)
            if prec < min_prec:
                break
            self.take()
            right = self.expression(prec if op.kind == "POWER" else prec + 1)
            left = Node("Binary", op.value, [left, right], op.line, op.column)
        return left

    def prefix(self):
        t = self.tok
        if self.accept("NOT"):
            return Node("Unary", "not", [self.expression(7)], t.line, t.column)
        if self.accept("PIPE"):
            return Node("FlowEmit", "|>", [self.expression(7)], t.line, t.column)
        if self.accept("MINUS"):
            return Node("Unary", "-", [self.expression(7)], t.line, t.column)
        if self.accept("PLUS"):
            return Node("Unary", "+", [self.expression(7)], t.line, t.column)
        if self.accept("LPAREN"):
            e = self.expression()
            self.take("RPAREN")
            return e
        if self.at("INT", "FLOAT", "STRING", "BOOL", "NULL", "NONE"):
            self.take()
            if t.kind == "INT":
                v = int(t.value)
            elif t.kind == "FLOAT":
                v = float(t.value)
            elif t.kind == "BOOL":
                v = t.value == "true"
            elif t.kind == "NULL":
                v = None
            elif t.kind == "NONE":
                v = "None"
            else:
                v = t.value
            return Node("Literal", v, line=t.line, column=t.column)
        if self.at("IDENT", "TYPE"):
            self.take()
            return Node("Name", t.value, line=t.line, column=t.column)
        if self.accept("LBRACKET"):
            vals = []
            if not self.at("RBRACKET"):
                vals.append(self.expression())
                while self.accept("COMMA"):
                    vals.append(self.expression())
            self.take("RBRACKET")
            return Node("List", children=vals, line=t.line, column=t.column)
        raise ParseError(
            f"linha {t.line}, coluna {t.column}: expressão inesperada {t.kind} ({t.value!r})"
        )

    def call(self, callee):
        self.take("LPAREN")
        args = []
        if not self.at("RPAREN"):
            args.append(self.expression())
            while self.accept("COMMA"):
                args.append(self.expression())
        self.take("RPAREN")
        return Node(
            "Call", children=[callee] + args, line=callee.line, column=callee.column
        )


def parse(source):
    return Parser(lex(source)).parse()


def dump(node, level=0):
    out = [
        "  " * level + node.kind + ("" if node.value is None else f"({node.value!r})")
    ]
    for child in node.children:
        out.extend(dump(child, level + 1).splitlines())
    return "\n".join(out)
