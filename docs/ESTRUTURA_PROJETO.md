# Estrutura do Pifari

## Organização mínima do compilador

- `frontend/`: tokens, gramática e árvore sintática (`lexer.l`, `parser.y`, `ast.h`).
- `backend/`: execução e semântica; tipos, efeitos, buffers e módulos C auxiliares.
- `backend/optimization/`: IR, otimizador, kernels nativos, cache e planejamento.
- `backend/integrations/`: agentes, MCP, dispositivos e comunicação GPU.
- `tests/`: testes; `examples/`: programas Pifari; `tools/`: ferramentas auxiliares.
- `docs/`: documentação e tutoriais; `dashboard/`: painel e telemetria.
- `build/`: arquivos gerados; `pifari`: executável usado pelas integrações existentes.

Cada par `.c`/`.h` fica junto. O diretório identifica a área; o nome do arquivo identifica o módulo.

Compile com `make`; valide com `make test-core`, `make test-golden` e `make test-equiv`.
