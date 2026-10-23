# Plano e Evidências de Testes do MCP Gateway e Agent Runtime

**Data:** 2026-10-23  
**Servidor MCP de Teste Controlado:** `tests/mock_mcp_server.py`  
**Suíte C:** `tests/test_agent_runtime.c` (`./test_agent_runtime`)  
**Protótipo Vertical Pifari:** `examples/prototipo_multiagente_mcp.pif`

---

## 1. Matriz de Cobertura de Testes MCP e Multiagente

| ID | Cenário | Módulo Alvo | Resultado Esperado | Onde é Verificado |
|---|---|---|---|---|
| **T-MCP-01** | Handshake `initialize` + `notifications/initialized` | `mcp_gateway.c` | `protocolVersion == "2025-03-26"`, `connected == 1`, `capabilities_tools == 1` | `tests/test_agent_runtime.c` + `prototipo_multiagente_mcp.pif` |
| **T-MCP-02** | Descoberta de ferramentas (`tools/list`) e anotações de efeito colateral | `mcp_gateway.c` | 6 ferramentas descobertas; `write_audit_entry` e `restricted_op` marcadas com `is_side_effect = 1` | `tests/test_agent_runtime.c` |
| **T-MCP-03** | Descoberta e leitura de recursos (`resources/list`, `resources/read`) | `mcp_gateway.c` | 2 recursos listados; leitura de `resource://pifari/runtime_spec` retorna JSON válido | `tests/test_agent_runtime.c` + `prototipo_multiagente_mcp.pif` |
| **T-MCP-04** | Bloqueio Default-Deny pelo Policy Engine | `agent_runtime.c` | Chamada a `restricted_op` sem permissão retorna `MCP_STATUS_ERR_POLICY_DENIED` e gera evento `POLICY_DENY` | `tests/test_agent_runtime.c` + `prototipo_multiagente_mcp.pif` |
| **T-MCP-05** | Bloqueio de ferramenta mutável quando `allow_side_effects == 0` | `agent_runtime.c` | `write_audit_entry` negada até que `allow_side_effects == 1` seja explicitamente concedido | `tests/test_agent_runtime.c` |
| **T-MCP-06** | Proteção contra chamadas duplicadas com efeito colateral | `mcp_gateway.c` | Segunda invocação idêntica de `write_audit_entry` bloqueada com `MCP_STATUS_ERR_DUPLICATE_CALL` | `tests/test_agent_runtime.c` |
| **T-MCP-07** | Timeout real e envio de `notifications/cancelled` | `mcp_gateway.c` + `agent_runtime.c` | `slow_op` (`delay_ms: 350`) com `timeout_ms: 70/80` transita tarefa para `CANCELLED` sem corromper o pipe `stdio` subsequente | `tests/test_agent_runtime.c` + `prototipo_multiagente_mcp.pif` |
| **T-MCP-08** | Cancelamento explícito pelo coordenador (`task_cancel`) | `agent_runtime.c` | `task_explicit_cancel` transita imediatamente para `CANCELLED` e incrementa `cancelled_tasks` | `tests/test_agent_runtime.c` |
| **T-MCP-09** | Execução paralela de tarefas independentes + barreira de dependência no DAG | `agent_runtime.c` | `task_collect` e `task_analyze` executam em paralelo (`dag_run(2)`); `task_report` só inicia após ambas completarem | `tests/test_agent_runtime.c` + `prototipo_multiagente_mcp.pif` |
| **T-MCP-10** | Context Store versionado + interpolação `{{ctx:key}}` + No-Op invariance | `agent_runtime.c` | Escrita idêntica preserva `version`; escrita nova incrementa `version`; `task_report` recebe resultados estruturados dos dois agentes | `tests/test_agent_runtime.c` + `prototipo_multiagente_mcp.pif` |
| **T-MCP-11** | Persistência de artefato com checksum FNV-1a e exportação de telemetria JSON | `agent_runtime.c` | `dashboard/latest_report.txt` e `dashboard/telemetry_latest.json` gravados com checksum FNV-1a de 16 dígitos hex | `tests/test_agent_runtime.c` + `prototipo_multiagente_mcp.pif` |

---

## 2. Como Executar

No ambiente Linux/WSL (`Ubuntu-24.04`):

```bash
# Executar toda a suíte C (incluindo test_agent_runtime) + todos os exemplos .pif (incluindo prototipo_multiagente_mcp.pif)
make test

# Executar apenas o teste unitário/integração C do Agent Runtime + MCP Gateway
./test_agent_runtime

# Executar apenas o protótipo vertical multiagente em Pifari
./pifari examples/prototipo_multiagente_mcp.pif
```
