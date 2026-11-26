# Mapa dos arquivos e pastas do Pifari

O fluxo principal é: programa `.pif` → `frontend/lexer.l` (tokens) → `frontend/parser.y` (AST definida em `frontend/ast.h`) → `backend/runtime.c` (execução e seleção das otimizações). O Makefile gera e compila os fontes auxiliares.

## Como ler os nomes

`lexer` significa analisador léxico; `parser`, analisador sintático; `ast`, árvore sintática abstrata; `ir`, representação intermediária; `runtime`, ambiente de execução. Arquivos `.h` declaram contratos e tipos; `.c` implementam esses contratos. Esses nomes convencionais são mantidos porque já são usados nos includes, no Makefile e nas integrações.

## Diretórios e arquivos mantidos à mão

### Raiz

Configuração e arquivos de apoio desta pasta.

| Arquivo | Responsabilidade |
|---|---|
| [.clang-format](../.clang-format) | Arquivo de .: .clang-format. |
| [.editorconfig](../.editorconfig) | Arquivo de .: .editorconfig. |
| [.env.example](../.env.example) | Lista configurações esperadas sem carregar o .env pessoal. |
| [Makefile](../Makefile) | Descreve dependências para gerar parser/lexer, compilar objetos, ligar executável e rodar testes. |
| [README.md](../README.md) | Resume como compilar, executar, testar e quais recursos e limitações são declarados. |

### backend

Execução, tipos, otimizações e integrações.

| Arquivo | Responsabilidade |
|---|---|
| [agent_runtime.c](../backend/integrations/agent_runtime.c) | Gerencia tarefas, dependências, estados e execução. |
| [agent_runtime.h](../backend/integrations/agent_runtime.h) | A interface pública descreve o contrato. Coordena tarefas, agentes, dependências, estados e execução de grafos. |
| [ast_ir.c](../backend/optimization/ast_ir.c) | Representa operações em forma explícita para avaliação e análise. |
| [ast_ir.h](../backend/optimization/ast_ir.h) | A interface pública descreve o contrato. Representa operações em forma explícita para avaliação e análise. |
| [cache.c](../backend/optimization/cache.c) | Cache persistente dos resultados de compilação e kernels. Implementação. |
| [cache.h](../backend/optimization/cache.h) | Cache persistente dos resultados de compilação e kernels. Declarações públicas. |
| [devices.c](../backend/integrations/devices.c) | Detecção e seleção de dispositivos de execução. Implementação. |
| [devices.h](../backend/integrations/devices.h) | Detecção e seleção de dispositivos de execução. Declarações públicas. |
| [dirac.c](../backend/dirac.c) | Operações de álgebra na representação de Dirac. Implementação. |
| [dirac.h](../backend/dirac.h) | Operações de álgebra na representação de Dirac. Declarações públicas. |
| [effects.c](../backend/effects.c) | Classificação dos efeitos das funções para orientar otimizações seguras. Implementação. |
| [effects.h](../backend/effects.h) | Classificação dos efeitos das funções para orientar otimizações seguras. Declarações públicas. |
| [gpu_client.c](../backend/integrations/gpu_client.c) | Comunicação com o serviço de execução GPU. Implementação. |
| [gpu_client.h](../backend/integrations/gpu_client.h) | Comunicação com o serviço de execução GPU. Declarações públicas. |
| [mcp_gateway.c](../backend/integrations/mcp_gateway.c) | Abstrai descoberta e invocação de ferramentas e recursos externos. |
| [mcp_gateway.h](../backend/integrations/mcp_gateway.h) | A interface pública descreve o contrato. Centraliza descoberta e chamada de ferramentas e recursos externos. |
| [mcp_server.c](../backend/integrations/mcp_server.c) | Servidor MCP implementado pelo runtime Pifari. Implementação. |
| [mcp_server.h](../backend/integrations/mcp_server.h) | Servidor MCP implementado pelo runtime Pifari. Declarações públicas. |
| [memory_plan.c](../backend/optimization/memory_plan.c) | Procura reutilizar posições temporárias cujos intervalos de vida não se sobrepõem. |
| [memory_plan.h](../backend/optimization/memory_plan.h) | A interface pública descreve o contrato. Analisa a vida útil de temporários para procurar slots reutilizáveis. |
| [native_kernel.c](../backend/optimization/native_kernel.c) | Cuida de caminhos em que um subconjunto numérico compatível vira código nativo ou é reaproveitado do cache. |
| [native_kernel.h](../backend/optimization/native_kernel.h) | A interface pública descreve o contrato. Coordena compilação ou reutilização de operações numéricas nativas compatíveis. |
| [numeric_buffer.c](../backend/numeric_buffer.c) | Armazenamento e manipulação de buffers numéricos tipados. Implementação. |
| [numeric_buffer.h](../backend/numeric_buffer.h) | Armazenamento e manipulação de buffers numéricos tipados. Declarações públicas. |
| [operator_engine.c](../backend/optimization/operator_engine.c) | Implementa operações combinadas em buffers numéricos, evitando arrays intermediários. |
| [operator_engine.h](../backend/optimization/operator_engine.h) | A interface pública descreve o contrato. Implementa kernels numéricos combinados em uma passagem. |
| [optimizer.c](../backend/optimization/optimizer.c) | Transforma operações com regras que procuram preservar o resultado observável. |
| [optimizer.h](../backend/optimization/optimizer.h) | Expõe funções e tipos do otimizador para o sistema e para testes. |
| [plan.c](../backend/optimization/plan.c) | Seleção e diagnóstico do plano de execução e nível de otimização. Implementação. |
| [plan.h](../backend/optimization/plan.h) | Seleção e diagnóstico do plano de execução e nível de otimização. Declarações públicas. |
| [reduce.c](../backend/optimization/reduce.c) | Compilação nativa de reduções inteiras com verificação de overflow. Implementação. |
| [reduce.h](../backend/optimization/reduce.h) | Compilação nativa de reduções inteiras com verificação de overflow. Declarações públicas. |
| [reversible.c](../backend/reversible.c) | Transformações reversíveis por XOR. Implementação. |
| [reversible.h](../backend/reversible.h) | Transformações reversíveis por XOR. Declarações públicas. |
| [runtime.c](../backend/runtime.c) | Arquivo de backend: runtime. |
| [runtime.h](../backend/runtime.h) | Declara tipos, estruturas e funções compartilhadas pelo runtime e outros módulos. |
| [typecheck.c](../backend/typecheck.c) | Checagem estática dos tipos na árvore sintática. Implementação. |
| [typecheck.h](../backend/typecheck.h) | Checagem estática dos tipos na árvore sintática. Declarações públicas. |
| [types.c](../backend/types.c) | Representação e operações do sistema de tipos. Implementação. |
| [types.h](../backend/types.h) | Representação e operações do sistema de tipos. Declarações públicas. |

### dashboard

Painel e relatórios de telemetria.

| Arquivo | Responsabilidade |
|---|---|
| [index.html](../dashboard/index.html) | Arquivo de dashboard: index. |
| [latest_report.txt](../dashboard/latest_report.txt) | Arquivo de dashboard: latest report. |
| [telemetry_latest.json](../dashboard/telemetry_latest.json) | Arquivo de dashboard: telemetry latest. |

### docs

Documentação técnica e material didático.

| Arquivo | Responsabilidade |
|---|---|
| [ARQUITETURA_AGENT_RUNTIME.md](../docs/ARQUITETURA_AGENT_RUNTIME.md) | Documento de arquitetura, semântica, execução ou medições. |
| [BENCHMARK_SERVIDOR_MCP_PIFARI_VS_PYTHON.md](../docs/BENCHMARK_SERVIDOR_MCP_PIFARI_VS_PYTHON.md) | Documento de arquitetura, semântica, execução ou medições. |
| [BENCHMARKS.md](../docs/BENCHMARKS.md) | Documento de arquitetura, semântica, execução ou medições. |
| [BENCHMARKS_GPU.md](../docs/BENCHMARKS_GPU.md) | Documento de arquitetura, semântica, execução ou medições. |
| [ESTRUTURA_PROJETO.md](../docs/ESTRUTURA_PROJETO.md) | Documento de arquitetura, semântica, execução ou medições. |
| [EXECUCAO.md](../docs/EXECUCAO.md) | Documento de arquitetura, semântica, execução ou medições. |
| [GUIA_GOOGLE_CLOUD_AISTUDIO_REELS.md](../docs/GUIA_GOOGLE_CLOUD_AISTUDIO_REELS.md) | Documento de arquitetura, semântica, execução ou medições. |
| [OTIMIZACOES.md](../docs/OTIMIZACOES.md) | Documento de arquitetura, semântica, execução ou medições. |
| [PLANO_OTIMIZACAO_RUNTIME.md](../docs/PLANO_OTIMIZACAO_RUNTIME.md) | Documento de arquitetura, semântica, execução ou medições. |
| [PLANO_TESTES_MCP.md](../docs/PLANO_TESTES_MCP.md) | Documento de arquitetura, semântica, execução ou medições. |
| [ROADMAP_AGENT_RUNTIME.md](../docs/ROADMAP_AGENT_RUNTIME.md) | Documento de arquitetura, semântica, execução ou medições. |
| [SPEC_CONCORRENCIA.md](../docs/SPEC_CONCORRENCIA.md) | Documento de arquitetura, semântica, execução ou medições. |
| [SPEC_SEMANTICA.md](../docs/SPEC_SEMANTICA.md) | Documento de arquitetura, semântica, execução ou medições. |
| [STATUS_REAL.md](../docs/STATUS_REAL.md) | Documento de arquitetura, semântica, execução ou medições. |
| [V1_READINESS.md](../docs/V1_READINESS.md) | Documentação: Pifari — prontidão para V1. |

### docs/benchmarks

Relatórios históricos de desempenho.

| Arquivo | Responsabilidade |
|---|---|
| [BENCHMARK_FUSAO_CADEIAS_2026-10-19.md](../docs/benchmarks/BENCHMARK_FUSAO_CADEIAS_2026-10-19.md) | Documento de arquitetura, semântica, execução ou medições. |
| [BENCHMARK_INDEXACAO_NATIVA_2026-10-19.md](../docs/benchmarks/BENCHMARK_INDEXACAO_NATIVA_2026-10-19.md) | Documento de arquitetura, semântica, execução ou medições. |
| [BENCHMARK_MLP_XOR_2026-10-19.md](../docs/benchmarks/BENCHMARK_MLP_XOR_2026-10-19.md) | Documento de arquitetura, semântica, execução ou medições. |
| [BENCHMARKS_2026-10-19.md](../docs/benchmarks/BENCHMARKS_2026-10-19.md) | Documento de arquitetura, semântica, execução ou medições. |

### docs/curso-lexer

Aplicação React do curso de análise léxica.

| Arquivo | Responsabilidade |
|---|---|
| [.gitignore](../docs/curso-lexer/.gitignore) | Arquivo de docs/curso-lexer: .gitignore. |
| [docker-compose.yml](../docs/curso-lexer/docker-compose.yml) | Arquivo de docs/curso-lexer: docker-compose. |
| [Dockerfile](../docs/curso-lexer/Dockerfile) | Arquivo de docs/curso-lexer: Dockerfile. |
| [Dockerfile.dockerignore](../docs/curso-lexer/Dockerfile.dockerignore) | Arquivo de docs/curso-lexer: Dockerfile. |
| [index.html](../docs/curso-lexer/index.html) | Arquivo de docs/curso-lexer: index. |
| [nginx.conf](../docs/curso-lexer/nginx.conf) | Arquivo de docs/curso-lexer: nginx. |
| [package-lock.json](../docs/curso-lexer/package-lock.json) | Arquivo de docs/curso-lexer: package-lock. |
| [package.json](../docs/curso-lexer/package.json) | Arquivo de docs/curso-lexer: package. |
| [README.md](../docs/curso-lexer/README.md) | Documentação: Oficina do Lexer Pifari. |
| [vite.config.js](../docs/curso-lexer/vite.config.js) | Arquivo de docs/curso-lexer: vite.config. |

### docs/curso-lexer/src

Componentes, lições e estilos do curso.

| Arquivo | Responsabilidade |
|---|---|
| [App.jsx](../docs/curso-lexer/src/App.jsx) | Arquivo de docs/curso-lexer/src: App. |
| [lessons.js](../docs/curso-lexer/src/lessons.js) | Arquivo de docs/curso-lexer/src: lessons. |
| [main.jsx](../docs/curso-lexer/src/main.jsx) | Arquivo de docs/curso-lexer/src: main. |
| [styles.css](../docs/curso-lexer/src/styles.css) | Arquivo de docs/curso-lexer/src: styles. |

### docs/guia-didatico

Guia navegável com catálogo dos fontes.

| Arquivo | Responsabilidade |
|---|---|
| [abrir_guia.ps1](../docs/guia-didatico/abrir_guia.ps1) | Arquivo de docs/guia-didatico: abrir guia. |
| [catalogo.json](../docs/guia-didatico/catalogo.json) | Arquivo de docs/guia-didatico: catalogo. |
| [gerar_catalogo.py](../docs/guia-didatico/gerar_catalogo.py) | Documento de arquitetura, semântica, execução ou medições. |
| [index.html](../docs/guia-didatico/index.html) | Arquivo de docs/guia-didatico: index. |
| [README.md](../docs/guia-didatico/README.md) | Documento de arquitetura, semântica, execução ou medições. |

### docs/superpowers/plans

Planos de implementação documentados.

| Arquivo | Responsabilidade |
|---|---|
| [2026-10-31-curso-interativo-lexer-pifari.md](../docs/superpowers/plans/2026-10-31-curso-interativo-lexer-pifari.md) | Documentação: Curso Interativo do Lexer Pifari Implementation Plan. |

### docs/superpowers/specs

Especificações de design documentadas.

| Arquivo | Responsabilidade |
|---|---|
| [2026-10-30-curso-interativo-lexer-pifari-design.md](../docs/superpowers/specs/2026-10-30-curso-interativo-lexer-pifari-design.md) | Documentação: Curso Interativo do Lexer Pifari — Design. |

### examples

Exemplos executáveis da linguagem.

| Arquivo | Responsabilidade |
|---|---|
| [basico.pif](../examples/basico.pif) | Reconheça atribuição, valores e saída antes de estudar funções ou concorrência. |
| [carga_fluxo_puro.pif](../examples/carga_fluxo_puro.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [conc_fluxos.pif](../examples/conc_fluxos.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [conc_processos.pif](../examples/conc_processos.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [conc_shared.pif](../examples/conc_shared.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [conc_tarefas.pif](../examples/conc_tarefas.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [condicao_implicita.pif](../examples/condicao_implicita.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [diretivas.pif](../examples/diretivas.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [efeitos.pif](../examples/efeitos.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [fluxo.pif](../examples/fluxo.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [fluxo_coleta.pif](../examples/fluxo_coleta.pif) | Mostra como os resultados de um fluxo são reunidos numa variável. |
| [fluxo_paralelo.pif](../examples/fluxo_paralelo.pif) | Mostra distribuição de itens e isolamento da variável por worker. |
| [fluxo_sequencial.pif](../examples/fluxo_sequencial.pif) | Exemplo de percurso item a item com o operador de fluxo sequencial. |
| [fluxo_sincronizacao.pif](../examples/fluxo_sincronizacao.pif) | Mostra que o trabalho à esquerda termina antes da expressão à direita. |
| [funcoes.pif](../examples/funcoes.pif) | Mostra parâmetros e chamadas; conecte a sintaxe aos frames e ambientes do runtime. |
| [gpu_affine.pif](../examples/gpu_affine.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [gpu_dispositivos.pif](../examples/gpu_dispositivos.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [mlp_xor.pif](../examples/mlp_xor.pif) | Exemplo de MLP com operações numéricas e laços, sem framework externo. |
| [mlp_xor.py](../examples/mlp_xor.py) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [mlp_xor_fluxo.pif](../examples/mlp_xor_fluxo.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [nova_sintaxe_fluxo.pif](../examples/nova_sintaxe_fluxo.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [orquestrador_reels_aistudio.pif](../examples/orquestrador_reels_aistudio.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [paralelismo_assincrono.pif](../examples/paralelismo_assincrono.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [prototipo_multiagente_mcp.pif](../examples/prototipo_multiagente_mcp.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [reducao_erro.pif](../examples/reducao_erro.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [reducao_nativa.pif](../examples/reducao_nativa.pif) | Demonstra agregação de valores e a fronteira entre linguagem e caminho nativo. |
| [runtime_basico.pif](../examples/runtime_basico.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [runtime_fluxo.pif](../examples/runtime_fluxo.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [runtime_fused_chain.pif](../examples/runtime_fused_chain.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [runtime_fusion.pif](../examples/runtime_fusion.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [runtime_indexed_native.pif](../examples/runtime_indexed_native.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [runtime_lista.pif](../examples/runtime_lista.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [runtime_map_fused.pif](../examples/runtime_map_fused.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [runtime_matmul.pif](../examples/runtime_matmul.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [runtime_matriz.pif](../examples/runtime_matriz.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [runtime_numeric_tools.pif](../examples/runtime_numeric_tools.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [runtime_operadores.pif](../examples/runtime_operadores.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [runtime_otimizacoes.pif](../examples/runtime_otimizacoes.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [runtime_slots_pool.pif](../examples/runtime_slots_pool.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [runtime_vm.pif](../examples/runtime_vm.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [runtime_vm_index_compare.pif](../examples/runtime_vm_index_compare.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [servidor_mcp_aistudio_reels.py](../examples/servidor_mcp_aistudio_reels.py) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [servidor_mcp_pifari.pif](../examples/servidor_mcp_pifari.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [servidor_mcp_python_ref.py](../examples/servidor_mcp_python_ref.py) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [test.pif](../examples/test.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [test_pesado.pif](../examples/test_pesado.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [teste_cliente_servidor_mcp_pifari.pif](../examples/teste_cliente_servidor_mcp_pifari.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [teste_range_preguicoso.pif](../examples/teste_range_preguicoso.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [tipos.pif](../examples/tipos.pif) | Programa de demonstração ou regressão; o fonte real aparece nesta tela. |
| [typed_arrays.pif](../examples/typed_arrays.pif) | Demonstra armazenamento e operações sobre dados numéricos tipados. |

### examples/benchmarks

Programas e scripts de medição de desempenho.

| Arquivo | Responsabilidade |
|---|---|
| [benchmark_cpu.pif](../examples/benchmarks/benchmark_cpu.pif) | Programa de medição de desempenho e validação de resultados. |
| [benchmark_cpu.py](../examples/benchmarks/benchmark_cpu.py) | Programa de medição de desempenho e validação de resultados. |
| [benchmark_fused_chain.pif](../examples/benchmarks/benchmark_fused_chain.pif) | Programa de medição de desempenho e validação de resultados. |
| [benchmark_indexed_native.pif](../examples/benchmarks/benchmark_indexed_native.pif) | Programa de medição de desempenho e validação de resultados. |
| [benchmark_indexed_vs_python.py](../examples/benchmarks/benchmark_indexed_vs_python.py) | Programa de medição de desempenho e validação de resultados. |
| [benchmark_matrizes.pif](../examples/benchmarks/benchmark_matrizes.pif) | Programa de medição de desempenho e validação de resultados. |
| [benchmark_matrizes.py](../examples/benchmarks/benchmark_matrizes.py) | Programa de medição de desempenho e validação de resultados. |
| [benchmark_mcp_pifari_vs_python.py](../examples/benchmarks/benchmark_mcp_pifari_vs_python.py) | Programa de medição de desempenho e validação de resultados. |
| [benchmark_mlp_xor.py](../examples/benchmarks/benchmark_mlp_xor.py) | Programa de medição de desempenho e validação de resultados. |
| [README.md](../examples/benchmarks/README.md) | Programa de medição de desempenho e validação de resultados. |

### examples/benchmarks/legacy

Benchmarks antigos preservados.

| Arquivo | Responsabilidade |
|---|---|
| [bench_fluxo.pif](../examples/benchmarks/legacy/bench_fluxo.pif) | Programa de medição de desempenho e validação de resultados. |
| [bench_for.pif](../examples/benchmarks/legacy/bench_for.pif) | Programa de medição de desempenho e validação de resultados. |
| [bench_python.py](../examples/benchmarks/legacy/bench_python.py) | Programa de medição de desempenho e validação de resultados. |

### frontend

Análise léxica, sintática e definição da AST.

| Arquivo | Responsabilidade |
|---|---|
| [ast.h](../frontend/ast.h) | Define os tipos de nós e os dados que representam o programa após a análise sintática. |
| [lexer.l](../frontend/lexer.l) | Define padrões que reconhecem nomes, números, palavras reservadas, operadores e pontuação e os transformam em tokens. |
| [parser.y](../frontend/parser.y) | Define como tokens formam expressões e comandos, precedência, associatividade e ações que constroem a AST. |

### tests

Testes automatizados e suas referências.

| Arquivo | Responsabilidade |
|---|---|
| [equiv-skip.txt](../tests/equiv-skip.txt) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [equiv.sh](../tests/equiv.sh) | Compara saídas de programas em configurações de otimização diferentes. |
| [fuzz_parser.py](../tests/fuzz_parser.py) | Gera entradas variadas para exercitar caminhos de erro e procurar falhas. |
| [golden.sh](../tests/golden.sh) | Compara resultados com saídas de referência mantidas no projeto. |
| [mock_mcp_server.py](../tests/mock_mcp_server.py) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [test_agent_runtime.c](../tests/test_agent_runtime.c) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [test_ast_ir.c](../tests/test_ast_ir.c) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [test_dirac.c](../tests/test_dirac.c) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [test_native_kernel.c](../tests/test_native_kernel.c) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [test_optimizer.c](../tests/test_optimizer.c) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |

### tests/golden

Saídas esperadas (.txt) e códigos de retorno (.exit).

| Arquivo | Responsabilidade |
|---|---|
| [conc_fluxos.exit](../tests/golden/conc_fluxos.exit) | Código de saída esperado do exemplo `conc_fluxos`. |
| [conc_fluxos.txt](../tests/golden/conc_fluxos.txt) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [conc_processos.exit](../tests/golden/conc_processos.exit) | Código de saída esperado do exemplo `conc_processos`. |
| [conc_processos.txt](../tests/golden/conc_processos.txt) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [conc_tarefas.exit](../tests/golden/conc_tarefas.exit) | Código de saída esperado do exemplo `conc_tarefas`. |
| [conc_tarefas.txt](../tests/golden/conc_tarefas.txt) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [fluxo_coleta.exit](../tests/golden/fluxo_coleta.exit) | Código de saída esperado do exemplo `fluxo_coleta`. |
| [fluxo_coleta.txt](../tests/golden/fluxo_coleta.txt) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [fluxo_sequencial.exit](../tests/golden/fluxo_sequencial.exit) | Código de saída esperado do exemplo `fluxo_sequencial`. |
| [fluxo_sequencial.txt](../tests/golden/fluxo_sequencial.txt) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [gpu_dispositivos.exit](../tests/golden/gpu_dispositivos.exit) | Código de saída esperado do exemplo `gpu_dispositivos`. |
| [gpu_dispositivos.txt](../tests/golden/gpu_dispositivos.txt) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [prototipo_multiagente_mcp.exit](../tests/golden/prototipo_multiagente_mcp.exit) | Código de saída esperado do exemplo `prototipo_multiagente_mcp`. |
| [prototipo_multiagente_mcp.txt](../tests/golden/prototipo_multiagente_mcp.txt) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [reducao_nativa.exit](../tests/golden/reducao_nativa.exit) | Código de saída esperado do exemplo `reducao_nativa`. |
| [reducao_nativa.txt](../tests/golden/reducao_nativa.txt) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [runtime_fluxo.exit](../tests/golden/runtime_fluxo.exit) | Código de saída esperado do exemplo `runtime_fluxo`. |
| [runtime_fluxo.txt](../tests/golden/runtime_fluxo.txt) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [runtime_fused_chain.exit](../tests/golden/runtime_fused_chain.exit) | Código de saída esperado do exemplo `runtime_fused_chain`. |
| [runtime_fused_chain.txt](../tests/golden/runtime_fused_chain.txt) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [runtime_fusion.exit](../tests/golden/runtime_fusion.exit) | Código de saída esperado do exemplo `runtime_fusion`. |
| [runtime_fusion.txt](../tests/golden/runtime_fusion.txt) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [runtime_indexed_native.exit](../tests/golden/runtime_indexed_native.exit) | Código de saída esperado do exemplo `runtime_indexed_native`. |
| [runtime_indexed_native.txt](../tests/golden/runtime_indexed_native.txt) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [runtime_lista.exit](../tests/golden/runtime_lista.exit) | Código de saída esperado do exemplo `runtime_lista`. |
| [runtime_lista.txt](../tests/golden/runtime_lista.txt) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [runtime_matmul.exit](../tests/golden/runtime_matmul.exit) | Código de saída esperado do exemplo `runtime_matmul`. |
| [runtime_matmul.txt](../tests/golden/runtime_matmul.txt) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [runtime_matriz.exit](../tests/golden/runtime_matriz.exit) | Código de saída esperado do exemplo `runtime_matriz`. |
| [runtime_matriz.txt](../tests/golden/runtime_matriz.txt) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [runtime_numeric_tools.exit](../tests/golden/runtime_numeric_tools.exit) | Código de saída esperado do exemplo `runtime_numeric_tools`. |
| [runtime_numeric_tools.txt](../tests/golden/runtime_numeric_tools.txt) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [runtime_operadores.exit](../tests/golden/runtime_operadores.exit) | Código de saída esperado do exemplo `runtime_operadores`. |
| [runtime_operadores.txt](../tests/golden/runtime_operadores.txt) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [runtime_otimizacoes.exit](../tests/golden/runtime_otimizacoes.exit) | Código de saída esperado do exemplo `runtime_otimizacoes`. |
| [runtime_otimizacoes.txt](../tests/golden/runtime_otimizacoes.txt) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [runtime_slots_pool.exit](../tests/golden/runtime_slots_pool.exit) | Código de saída esperado do exemplo `runtime_slots_pool`. |
| [runtime_slots_pool.txt](../tests/golden/runtime_slots_pool.txt) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [runtime_vm.exit](../tests/golden/runtime_vm.exit) | Código de saída esperado do exemplo `runtime_vm`. |
| [runtime_vm.txt](../tests/golden/runtime_vm.txt) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [runtime_vm_index_compare.exit](../tests/golden/runtime_vm_index_compare.exit) | Código de saída esperado do exemplo `runtime_vm_index_compare`. |
| [runtime_vm_index_compare.txt](../tests/golden/runtime_vm_index_compare.txt) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [teste_cliente_servidor_mcp_pifari.exit](../tests/golden/teste_cliente_servidor_mcp_pifari.exit) | Código de saída esperado do exemplo `teste_cliente_servidor_mcp_pifari`. |
| [teste_cliente_servidor_mcp_pifari.txt](../tests/golden/teste_cliente_servidor_mcp_pifari.txt) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [teste_range_preguicoso.exit](../tests/golden/teste_range_preguicoso.exit) | Código de saída esperado do exemplo `teste_range_preguicoso`. |
| [teste_range_preguicoso.txt](../tests/golden/teste_range_preguicoso.txt) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |
| [typed_arrays.exit](../tests/golden/typed_arrays.exit) | Código de saída esperado do exemplo `typed_arrays`. |
| [typed_arrays.txt](../tests/golden/typed_arrays.txt) | Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso. |

### tools

Utilitários auxiliares.

| Arquivo | Responsabilidade |
|---|---|
| [pifari_worker.py](../tools/pifari_worker.py) | Utilitário Python de apoio a processos e tarefas ao redor do núcleo C. |

## Artefatos e configuração local

- `build/generated/`: lexer/parser C e cabeçalhos produzidos por Flex/Bison.
- `build/obj/`: objetos `.o` e dependências `.d` da compilação.
- `build/bin/`: executáveis dos testes unitários.
- `pifari`: executável principal, mantido na raiz para compatibilidade MCP.
- `.env`: configuração local; não faz parte do catálogo de conteúdo.
- `__pycache__/`, `node_modules/` e `dist/`: caches, dependências e saídas geradas; não editar para mudar o compilador.

## Legibilidade e validação

Os fontes C usam quatro espaços de indentação, limite de 100 colunas e comandos curtos expandidos, conforme `.clang-format`. A ordem dos includes é preservada. Não aplique um formatador C diretamente às gramáticas `.l` e `.y`.

No WSL, execute `make test-core`, `make test-golden` e `make test-equiv`. Para verificar a formatação, use `clang-format --dry-run --Werror backend/*.c backend/*.h frontend/ast.h tests/*.c`.
