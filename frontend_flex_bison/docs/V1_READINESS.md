# Pifari — prontidão para V1

Data da estabilização: 2026-10-27 10:02:46
Commit-base: 3cc0b2e

## Estado desta rodada
- SUITE COMPLETA PASSOU nesta execução.
- Correção JIT: funções com vários parâmetros são desviadas para o interpretador até existir mapeamento explícito de slots/parâmetros.
- Código C gerado do JIT só copia argumento zero se o ponteiro existir e na > 0; removeu leituras de a[1..7] e escritas especulativas em v[1..7].
- Log completo da suíte: C:\Users\Yngrid Kalinne\Documents\Pifari_V1_stabilizacao_20261027_094946\testes_v1.log
- Snapshot prévio do diff: C:\Users\Yngrid Kalinne\Documents\Pifari_V1_stabilizacao_20261027_094946\03-working-tree.patch
- Cópia de backend/runtime.c antes da correção: C:\Users\Yngrid Kalinne\Documents\Pifari_V1_stabilizacao_20261027_094946\backend_runtime_before_v1.c

## Escopo V1 proposto
Interpretador, sintaxe documentada, tratamento de erros, build reprodutível e testes de núcleo são o contrato estável. JIT é opcional e deve recorrer ao interpretador quando não elegível. GPU, MCP/agentes, kernels nativos e paralelismo permanecem experimentais até terem contratos e testes dedicados.

## Antes de declarar 1.0 final
1. Adicionar regressão explícita para função de um parâmetro com variáveis locais e para chamada com múltiplos parâmetros, comparando otimização 0 e padrão.
2. Executar sanitizadores/valgrind onde suportados e validar overflow, divisão por zero e limites de arrays.
3. Auditar a gramática pública, especificação e exemplos.
4. Revisar o diff completo: o repositório já possuía alterações pendentes antes desta rodada.
5. Verificar autenticação Gemini CLI; a tentativa atual falhou com erro de API key inválida.

Esta página é um registro de prontidão, não uma release publicada.