# Roadmap Incremental — Evolução de Desempenho e Plataforma Multiagente MCP

## 1. Visão Geral e Princípios

A evolução do Pifari segue duas frentes desacopladas em código e integradas na linguagem:
1. **Frente A — Linguagem e Runtime de Alto Desempenho:** Eliminar gargalos reais medidos no interpretador/VM/JIT sem descaracterizar a gramática nem quebrar os 25 testes de integração e 4 binários unitários existentes.
2. **Frente B — Plataforma de Orquestração Multiagente e MCP Gateway:** Implementar os 7 módulos (`Task Engine`, `Agent Manager`, `MCP Gateway`, `Context Store`, `Policy Engine`, `Telemetry Engine`, `Artifact Manager`) em C11 modular, com protocolo JSON-RPC 2.0 real (`stdio` e `http`), segurança default-deny e protótipo vertical reproduzível.

---

## 2. Priorização por Impacto, Risco e Esforço

| Prioridade | Entrega | Impacto | Risco | Esforço | Critério de Aceite |
| :---: | :--- | :---: | :---: | :---: | :--- |
| **P0** | **Correção de Thread-Safety e Pré-anotação de Slots em `FunctionDef` + `BC_EXP` + `IndexAssign` (`A[i] = v`)** | Alto | Baixo | Baixo | Zero data race na AST durante `>\|`; `sigmoid(x)` compilada em bytecode; sintaxe `A[i] = v` funcional na gramática e na VM; redução drástica do tempo de `mlp_xor.pif`. |
| **P0** | **Cache Persistente em Disco para `native_kernel.c` + Suporte Nativo a `V_RANGE` / `V_F64_ARRAY` em `>\|` e `\|>`** | Alto | Baixo | Baixo | Kernels IR reutilizados em `/tmp/pifari_kernel_cache_<uid>`; `range(N) >\| x >\| fn(x)` e `f64_array >\| x >\| fn(x)` executam via kernel nativo C sem boxing intermediário. |
| **P1** | **Módulo `MCP Gateway` (`backend/mcp_gateway.[ch]`) + Servidor MCP Controlado (`tests/mock_mcp_server.py`)** | Crítico | Médio | Médio | Handshake `initialize` / `notifications/initialized`, `tools/list`, `tools/call`, `resources/list`, `resources/read`, timeouts reais via `poll()`, cancelamento e recuperação sobre `stdio` e `http`. |
| **P1** | **Núcleo dos 7 Módulos do Agent Runtime (`backend/agent_runtime.[ch]`)** | Crítico | Baixo | Médio | `Task Engine` (DAG paralelo + dependências + deadlines + cancelamento), `Agent Manager`, `Context Store` versionado, `Policy Engine` default-deny, `Telemetry Engine` e `Artifact Manager` com FNV-1a. |
| **P1** | **Integração na Linguagem Pifari + Protótipo Vertical de 3 Agentes (`examples/prototipo_multiagente_mcp.pif`)** | Crítico | Baixo | Médio | Execução de ponta a ponta com 3 agentes (`coletor`, `analista`, `relator`), 2 tarefas paralelas, 1 tarefa dependente, chamada MCP real, cancelamento testado e persistência de relatório/artefatos. |
| **P2** | **Dashboard de Observabilidade (`dashboard/index.html`) e Documentação Consolidada** | Médio | Baixo | Baixo | Painel HTML/JS autocontido que consome os eventos reais de telemetria, estado do DAG, métricas MCP e artefatos produzidos. |

---

## 3. Checklist de Execução e Pesos de Progresso

| ID | Etapa | Peso | Estado Atual |
| :--- | :--- | ---: | :--- |
| **E1** | Auditoria técnica do código, inicialização Git e baseline de testes/benchmarks | 15% | `CONCLUÍDO` |
| **E2** | Diagnóstico arquitetural (`docs/STATUS_REAL.md` e `docs/ROADMAP_AGENT_RUNTIME.md`) | 10% | `CONCLUÍDO` |
| **E3** | Otimizações do compilador/runtime (`FunctionDef` slots, `BC_EXP`, `A[i] = val`, cache de `native_kernel`, fast-path `V_RANGE`/`V_F64_ARRAY`) | 25% | `EM EXECUÇÃO` |
| **E4** | Implementação do `MCP Gateway` (`stdio`/`http`, JSON-RPC 2.0, timeouts, idempotência) e `tests/mock_mcp_server.py` | 20% | `PENDENTE` |
| **E5** | Implementação dos 7 módulos em `backend/agent_runtime.[ch]` e builtins Pifari | 15% | `PENDENTE` |
| **E6** | Protótipo vertical (`examples/prototipo_multiagente_mcp.pif`), testes C (`test_agent_runtime`), dashboard e docs finais (`ARQUITETURA`, `PLANO_TESTES_MCP`, `BENCHMARKS`) | 15% | `PENDENTE` |
