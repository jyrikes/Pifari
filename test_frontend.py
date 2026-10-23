import unittest
from lexer import lex, LexError
from parser import parse

class FrontendTests(unittest.TestCase):
    def test_all_flow_operators(self):
        kinds={t.kind for t in lex("a |> b |< c <| d >| e")}
        self.assertTrue({"PIPE","JOIN","APPLY","FANOUT"} <= kinds)
    def test_comment_and_quotes(self):
        ts=lex('x = "olá" @ comentário\ny = \'oi\'\n')
        self.assertTrue(any(t.kind=="STRING" and t.value=="olá" for t in ts))
        self.assertTrue(any(t.kind=="STRING" and t.value=="oi" for t in ts))
    def test_typed_dynamic_declarations(self):
        tree=parse("idade = 10\npontos =: int(20)\n")
        self.assertEqual([n.kind for n in tree.children],["Assign","TypedDecl"])
    def test_concise_if_else(self):
        tree=parse('test = true\ntest\n    print("olá")\nelse\n    |> 0\n')
        self.assertEqual(tree.children[1].kind,"ImplicitIf")
        self.assertEqual(len(tree.children[1].children[2].children),1)
    def test_class_and_imports(self):
        tree=parse(':= Pessoa\n    =: idade int\nimport math\nLink to "mylib"\n')
        self.assertEqual(tree.children[0].kind,"Class")
        self.assertEqual(tree.children[1].kind,"Import")
        self.assertEqual(tree.children[2].kind,"LinkTo")
    def test_multiline_comment(self):
        ts=lex('a = 1\n@""" comentário\nignorando tokens\n"""@\nb = 2\n')
        self.assertIn("a", [t.value for t in ts]); self.assertIn("b", [t.value for t in ts])
    def test_unclosed_string(self):
        with self.assertRaises(LexError): lex('x = "oops\n')

if __name__=="__main__": unittest.main(verbosity=2)
