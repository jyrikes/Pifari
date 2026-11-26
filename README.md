# Pifari — linguagem e implementação

A implementação principal fica em [frontend_flex_bison](frontend_flex_bison/README.md). Consulte o [mapa de cada arquivo e pasta](frontend_flex_bison/docs/MAPA_ARQUIVOS.md).

| Arquivo ou pasta | Finalidade |
|---|---|
| `frontend_flex_bison/` | Interpretador em C, frontend Flex/Bison, otimizações, testes, exemplos e documentação. |
| `lexer.py` | Protótipo Python do analisador léxico. |
| `parser.py` | Protótipo Python do analisador sintático. |
| `test_frontend.py` | Testes do frontend Python. |
| `README_Frontend_Pifari.txt` | Instruções históricas do protótipo. |
| `Definicao_Pifari.tex` | Especificação original em LaTeX. |
| `Definicao_Pifari_v2.tex` | Segunda versão da especificação. |
| `Definicao_Pifari_v3.tex` | Terceira versão da especificação. |
| `Definicao_Pifari*.pdf` | Documentos renderizados das especificações. |
| `*.aux`, `*.toc`, `*.out`, `*.log` | Arquivos auxiliares gerados pelo LaTeX. |
| `.vscode/` | Configuração local do editor. |
| `.gitignore` | Regras de exclusão de arquivos no Git. |
| `__pycache__/` | Cache de bytecode Python. |

Para começar, leia `frontend/lexer.l`, `frontend/parser.y`, `frontend/ast.h` e `backend/runtime.c`, nessa ordem, dentro da implementação principal. As especificações descrevem a linguagem; os testes e o código determinam o que esta versão executa.
