# Interpretador Pifari — núcleo inicial

## Organização mínima do compilador

- `frontend/`: tokens, gramática e árvore sintática (`lexer.l`, `parser.y`, `ast.h`).
- `backend/`: execução e semântica; tipos, efeitos, buffers e módulos C auxiliares.
- `backend/optimization/`: IR, otimizador, kernels nativos, cache e planejamento.
- `backend/integrations/`: agentes, MCP, dispositivos e comunicação GPU.
- `tests/`: testes; `examples/`: programas Pifari; `tools/`: ferramentas auxiliares.
- `docs/`: documentação e tutoriais; `dashboard/`: painel e telemetria.
- `build/`: arquivos gerados; `pifari`: executável usado pelas integrações existentes.

Cada par `.c`/`.h` fica junto. O diretório identifica a área; o nome do arquivo identifica o módulo.


Implementação em C com Flex (lexer) e Bison (parser). O executável interpreta a AST.

Consulte o [mapa de todos os arquivos e pastas](docs/MAPA_ARQUIVOS.md) para navegar pelo código.

## Compilar e executar no WSL

```bash
make clean
make
./pifari examples/fluxo_sequencial.pif
./pifari examples/fluxo_paralelo.pif
./pifari examples/fluxo_coleta.pif
./pifari examples/fluxo_sincronizacao.pif
./pifari examples/nova_sintaxe_fluxo.pif
```

Dependências: gcc, flex, bison, make e pthreads (POSIX, incluído no Linux).

## Arquitetura

O projeto é separado em duas camadas:

- `frontend/`: AST e dados de compilação (`ast.h`), gramática Bison (`parser.y`) e lexer Flex (`lexer.l`). O frontend apenas transforma o código-fonte em uma AST.
- `backend/`: API e implementação do runtime (`runtime.h` e `runtime.c`). O backend recebe a AST e executa interpretação, bytecode, listas e fluxos.
- `parser.tab.c`, `parser.tab.h` e `lex.yy.c`: artefatos gerados em `build/generated/`; não devem ser editados à mão.

O `Makefile` compila cada camada em objetos separados (`build/obj/frontend/*.o` e `build/obj/backend/*.o`) e só depois faz o link do executável. Isso permite testar ou substituir o backend sem misturar sua implementação com o parser.

### Módulos de otimização (novo)

- `backend/optimization/optimizer.h` / `optimizer.c`: grafo de expressões numéricas (IR), dobra de constantes seguras e avaliação/mapeamento por elemento sem arrays intermediários por operação. Identidades que possam alterar NaN ou zero com sinal não são aplicadas sem um futuro modo fast-math explícito.
- `backend/optimization/operator_engine.h` / `operator_engine.c`: kernels nativos fundidos para arrays; atualmente oferece `map_affine_relu`, que calcula `max(0, escala*x + deslocamento)` em uma passagem e usa threads para vetores grandes quando solicitadas.
- `backend/optimization/memory_plan.h` / `memory_plan.c`: planejador simples de slots temporários reutilizáveis a partir dos intervalos de vida dos valores.
- `backend/reversible.h` / `reversible.c`: exemplo didático de transformação reversível por XOR; aplicar a mesma máscara duas vezes restaura os bits originais.
- `tests/test_optimizer.c`: testes unitários isolados dos módulos, executados por `make test`.

Exemplo de kernel nativo no Pifari:

```pifari
valores = [-2, 0, 3, 10]
resultado = map_affine_relu(valores, 2, -1)
print(resultado)
```

A fusão evita criar arrays separados para multiplicação, soma e ReLU dentro do kernel. No runtime atual, listas genéricas ainda são convertidas para buffers `double` antes do kernel e convertidas de volta no resultado; arrays numéricos tipados e contíguos são uma próxima etapa para eliminar também essas conversões.

- `make clean && make`: regenera o frontend e compila as duas camadas.
- `make test`: executa os exemplos de integração.

- Variáveis dinâmicas com `nome = expressão`.
- Acesso em listas aninhadas, por exemplo `A[i][j]`. A atribuição por índice ainda não faz parte da gramática C/Flex/Bison.
- Declarações com tipo e inicializador, por exemplo `idade =: int(20)`.
- Valores: inteiros, reais, booleanos, textos e `Null` / `null` / `None`.
- Listas literais, indexação e iteração `for item in lista` ou sobre texto.
- Expressões aritméticas, comparações, igualdade e operadores lógicos.
- Condicionais explícitas `if condição`, `else` e uma forma `elseif`.
- Funções definidas pelo usuário com `function` ou `func` (forma existente) e a nova forma `nome : (parâmetros) { ... }`; na nova forma, a última expressão do bloco é o retorno implícito. `return` explícito continua disponível.
- JIT nativo opcional por função para um subconjunto seguro de funções numéricas de um argumento, com atribuições aritméticas e laços `for` sobre `range(parâmetro)`. O código C é compilado com GCC `-O3 -march=native`, carregado dinamicamente e armazenado em cache em `/tmp`; funções não suportadas continuam no runtime de bytecode/AST. A primeira execução fria pode pagar o custo de compilação; as seguintes reutilizam o cache.
- Chamadas nativas `print`, `println`, `len`, `range(n)` (intervalo preguiçoso de inteiros de `0` a `n-1`, limite de 1.000.000), `matmul(A, B)` (multiplicação nativa de matrizes 2D numéricas retangulares), `sleep_ms(milissegundos)`, `agora_ms()`/`agora_ns()` (relógio monotônico), `thread_id()` (identificador da thread atual) e `exp(x)` (exponencial), além dos casts `int`, `long`, `short`, `float`, `double`, `str`, `txt`, `bool`.
- `|>`: fluxo iterável sequencial.
- `>| `: distribuição paralela com pool limitado a quatro workers; cada worker tem bindings isolados e a saída de `print` é protegida contra mistura entre threads.
- `|< nome`: coleta em uma variável dos valores retornados por um fluxo paralelo.
- `<| continuação`: barreira; a expressão à esquerda termina (incluindo a junção de todos os workers) antes de avaliar a expressão à direita.
- Erros de execução com posição de linha e coluna para variáveis desconhecidas, divisão por zero, casts inválidos, índices inválidos e tipos não iteráveis.

## Exemplos

Fluxo sequencial:

```pifari
numeros = [1, 2, 3, 4]
numeros |> numero |> print(numero)
```

Distribuição paralela:

```pifari
numeros = [1, 2, 3, 4, 5, 6, 7, 8]
numeros >| numero >| print(numero)
```

Coletar resultados (o exemplo usa o cast nativo `int`):

```pifari
numeros = [1, 2, 3, 4]
numeros >| numero >| int(numero) |< resultados
print(resultados)
```

Sincronizar antes de continuar:

```pifari
numeros = [1, 2, 3, 4]
numeros >| numero >| print(numero) <| print("todas as tarefas terminaram")
```

## Benchmark de CPU: Pifari x Python

Os arquivos `examples/benchmarks/benchmark_cpu.pif` e `examples/benchmarks/benchmark_cpu.py` calculam a soma dos quadrados para quatro tarefas de 200.000 iterações cada. O Python mede a mediana de cinco execuções sequenciais, com quatro threads e com quatro processos, tanto com o pool aquecido quanto incluindo a inicialização dos processos. Para executar ambos, com cinco amostras de CPU e três de matrizes por runtime:

```bash
make benchmark
```

O benchmark de CPU reporta o tempo do kernel JIT de Pifari, o tempo total do processo (incluindo inicialização e compilação quando o cache está frio) e os tempos de kernel Python sequencial/threads/processos; valida a igualdade dos resultados. O benchmark de matrizes usa N=60 e valida o checksum para três casos: laços Pifari interpretados, matmul nativo em C e três laços em Python puro. A comparação entre matmul nativo e Python mede a vantagem da primitiva nativa; a comparação dos laços interpretados mostra o custo do interpretador. Tempos são diagnósticos do ambiente local, não garantias universais.

Para instrumentar uma execução e contar nós AST, buscas de variáveis, cópias de valores e operações de bytecode (a instrumentação adiciona overhead):

```bash
PIFARI_PROFILE=1 ./pifari examples/benchmarks/benchmark_matrizes.pif
```

`range(n)` cria um intervalo preguiçoso de inteiros de `0` a `n-1` (limite de 1.000.000), sem materializar uma lista. Ele funciona com `for`, `len` e indexação; quando impresso, é exibido como uma lista. Isso reduz bastante o custo de memória e alocação em laços numéricos. Resultados variam conforme CPU, carga do sistema e ambiente WSL.

## MLP do zero: Pifari vs Python

O exemplo `examples/mlp_xor.pif` treina uma rede neural MLP 2–4–1 para XOR com backpropagation e sigmoid, usando somente loops, arrays F64 e o builtin matemático `exp`; não depende de bibliotecas de machine learning. `examples/mlp_xor.py` implementa o mesmo algoritmo em Python puro (somente biblioteca padrão). O benchmark `python3 examples/benchmarks/benchmark_mlp_xor.py` mede treinamento em cinco execuções Pifari e compara medianas, verificando também a acurácia de treino e teste. O conjunto de teste usa quatro coordenadas diferentes das amostras de treino; é um teste didático pequeno, não uma avaliação estatística de generalização.

## Semântica e limites atuais

O runtime é compilado com `-O3 -flto`. A busca de variáveis usa uma tabela hash por ambiente, os laços reutilizam o binding da variável de iteração, e a AST pré-calcula os códigos dos operadores e os valores dos literais numéricos. Chamadas de função usam frames contíguos de slots locais indexados por hash, evitando clonar e reconstruir toda a lista de bindings em cada chamada; os slots são inicializados com os valores visíveis no ambiente chamador para preservar o isolamento atual. Expressões aritméticas suportadas (`+`, `-`, `*`, `/`, `//`, `%`, `**` e negação numérica) compilam uma vez por nó da AST e reutilizam o bytecode; comparações, lógica, chamadas, strings e fluxos usam fallback interpretado. Após a primeira compilação, a verificação do bytecode usa leitura atômica sem mutex no caminho quente; a compilação inicial continua sincronizada. A VM usa uma pilha local de 32 slots e só aloca pilha no heap para expressões maiores. A AST também dobra constantes aritméticas seguras, evitando interpretar subexpressões constantes em tempo de execução. A indexação de matrizes/listas aninhadas copia apenas o elemento selecionado, e `+` concatena listas. No fluxo paralelo, a distribuição de itens usa contador atômico para não bloquear a cada tarefa. Um pool persistente de até quatro workers é iniciado sob demanda e reutilizado por todos os fluxos `>|` dentro da execução; ele é encerrado ao final do runtime. Para listas, o buffer da sequência é transferido para o trabalho em vez de duplicar profundamente todos os itens. Os workers usam bindings rasos de leitura compartilhada para o ambiente base e um slot local para a variável de iteração, enquanto funções usam frames locais isolados. `range(n)` é preguiçoso e não materializa uma lista. Essas otimizações preservam a semântica atual; o ganho depende do programa e da carga do sistema. Veja `docs/PLANO_OTIMIZACAO_RUNTIME.md` para as próximas fases e os critérios de medição.

O pool persistente de `>| ` usa até quatro threads e espera todas terminarem antes de devolver a lista de resultados. O ambiente base é compartilhado apenas para leitura por bindings rasos privados de cada worker; a variável de iteração e variáveis locais de funções usam slots isolados. Alterações locais não são compartilhadas com outras tarefas nem com o escopo chamador. A saída de `print` é sincronizada. Isso é paralelismo real no runtime. Funções definidas pelo usuário também podem ser chamadas dentro do fluxo paralelo; cada chamada e cada worker usam ambientes isolados.

`|<` não é um operador genérico de retorno/fechamento: na versão atual, ele guarda a lista de resultados do fluxo à esquerda em um identificador. `<|` avalia o fluxo à esquerda até sua conclusão e só então avalia a expressão à direita. `|>` permanece sequencial.

Esta é uma fatia executável, não uma implementação completa da especificação. A gramática ainda tem conflitos shift/reduce conhecidos. A sintaxe condicional implícita sem `if` ainda não funciona: por exemplo, a forma `idade >= 18` seguida por bloco indentado ainda gera erro sintático; por isso `examples/runtime_basico.pif` e `examples/condicao_implicita.pif` são exemplos de uma funcionalidade pendente e não fazem parte do alvo `make test`. O alvo `make test` executa os exemplos atualmente suportados, incluindo funções e os quatro fluxos.

Limites atuais das funções: não há closures, parâmetros nomeados, valores padrão ou declarações de tipo nos parâmetros/retorno. Cada chamada recebe uma cópia do ambiente; portanto, atribuições dentro da função não alteram variáveis do chamador, e funções não podem ser registradas dinamicamente dentro de outras funções.

Ainda não implementados: métodos/instâncias e semântica completa de classes `:=` / `=:`; análise estática e validação de tipos nas declarações; carregamento real de `import`, `#include` e `Link to`; interoperabilidade real com C/Python/.NET; agentes, cancelamento cooperativo, memória compartilhada entre workers e escalonador configurável.

Execute um arquivo com `./pifari caminho/arquivo.pif`. O código de saída 0 significa sucesso e diferente de zero indica erro.


### Operações numéricas nativas e complexidade

A biblioteca padrão agora inclui:

- `sort(lista)`: ordenação numérica por `qsort`, com complexidade média/garantida pela biblioteca C tipicamente `O(n log n)` e memória auxiliar dependente da libc. Retorna uma nova lista e não altera a original. Aceita apenas inteiros e floats.
- `binary_search(lista_ordenada, alvo)`: busca binária em lista numérica já ordenada, `O(log n)` tempo e `O(1)` memória extra. Retorna o índice encontrado ou `-1`. Não verifica se a lista está ordenada.
- `solve_linear(A, b)`: resolve `A*x=b` usando eliminação de Gauss com pivoteamento parcial, `O(n^3)` tempo e `O(n^2)` memória. Rejeita matrizes não quadradas, elementos não numéricos, valores não finitos e matrizes singulares. Por usar `double`, é uma implementação numérica clássica, não um solver simbólico exato.
- `matmul(A, B)`: multiplicação nativa de matrizes, `O(r*k*c)` tempo e `O(r*c)` espaço para a saída.
- `map_affine_relu(lista, escala, deslocamento, workers=1)`: kernel fundido `max(0, escala*x + deslocamento)`, `O(n)` trabalho total; com workers, divide a saída em blocos independentes. Para listas genéricas ainda há conversão de/para `double`.

O grafo em `backend/optimizer.[ch]` agora é avaliado iterativamente em ordem topológica, com um buffer temporário reutilizado por chamada (sem recursão por nó), e oferece `pifari_graph_map_parallel`. A execução paralela é habilitada a partir de 16.384 elementos para reduzir o overhead de criar threads. Essa IR ainda é uma API interna em C: o compilador Pifari não transforma automaticamente expressões arbitrárias do programa nesse grafo nem gera JIT nativo para ele.

Exemplo:

```pifari
v = [9, 1, 5, 1, 3]
ordenados = sort(v)
print(ordenados)
print(binary_search(ordenados, 5))

A = [[3, 2], [1, 2]]
b = [18, 14]
print(solve_linear(A, b))
```


## Pipeline experimental AST → IR → kernel nativo

O caminho novo fica separado em módulos com contratos pequenos:

- `backend/ast_ir.[ch]`: converte funções de um parâmetro cujo corpo é uma expressão aritmética pura (`+`, `-`, `*`, `/` e negação unária) para `PifariExprGraph`. Variáveis livres, chamadas e controle de fluxo não são aceitos nessa IR.
- `backend/optimizer.[ch]`: dobra constantes seguras e calcula slots temporários por tempo de vida; `pifari_graph_workspace_slots` expõe a quantidade de slots planejados. O mapa serial/paralelo reutiliza esse workspace por worker.
- `backend/native_kernel.[ch]`: emite C somente a partir de opcodes validados, compila um shared object com GCC e carrega a função nativa. O kernel é criado sob demanda e reutilizado pela definição da função durante a execução do programa.
- `backend/numeric_buffer.[ch]`: buffer F64 contíguo, alinhado a 64 bytes, com checagem de overflow e inicialização/liberação explícitas.
- `backend/optimization/operator_engine.c`: o kernel affine-ReLU declara independência dos elementos para permitir vetorização do compilador; a divisão de trabalho usa faixas de saída disjuntas.
- `backend/dirac.[ch]`: álgebra simbólica experimental de bras, kets, operadores, soma, composição, adjunto e produto interno. É uma API C; ainda não é sintaxe da linguagem nem um sistema de álgebra linear simbólica completo.
- `backend/reversible.[ch]`: programa de gates XOR-mask, NOT e SWAP; `pifari_reversible_uncompute` aplica os gates inversos na ordem reversa. O compilador ainda não infere automaticamente inversas de programas Pifari arbitrários.

### Uso e fallback

`map_fused(funcao, lista, workers_opcional)` transforma a função elegível em IR e tenta compilar um kernel nativo fundido. O fluxo paralelo `lista >| item >| funcao(item)` tenta a mesma otimização automaticamente quando a função é numérica, unária e compatível com a IR. Se o kernel nativo não puder ser criado, o runtime pode usar o grafo interpretado; expressões fora do subconjunto elegível continuam no caminho normal do runtime.

Exemplo executável: `examples/runtime_map_fused.pif`. Testes específicos: `make test`; smoke-fuzz determinístico do parser/runtime: `make test-fuzz`.

### Limites que permanecem explícitos

Os arrays de variáveis ainda são listas genéricas no modelo de valores do runtime: o caminho fundido usa buffers F64 temporários e converte entrada/saída na fronteira. Ainda não há array tipado persistente na sintaxe Pifari, inferência de tipos geral, JIT para indexação/matrizes/loops arbitrários, fusion pass para todas as cadeias do programa, parser de notação de Dirac ou uncomputation automática sobre AST. O fuzz smoke testa robustez básica, não substitui libFuzzer/AFL, sanitizers em toda a suíte, injeção sistemática de falhas de alocação ou stress concorrente prolongado.


### Benchmarks reproduzíveis (2026-10-19)

Resultados medidos, ambiente, medianas, amostras, razões de desempenho e comandos para repetir estão em [BENCHMARKS_2026-10-19.md](docs/benchmarks/BENCHMARKS_2026-10-19.md). Execute `make benchmark` para repetir os testes de CPU e multiplicação de matrizes. Os resultados indicam claramente a diferença entre o caminho nativo e os loops interpretados; os ganhos são específicos das cargas testadas, e o fluxo paralelo pode perder para o sequencial em tarefas pequenas.


## VM de bytecode: indexação e comparações

A VM de expressões e a VM de funções agora compilam comparações (==, !=, <,
<=, >, >=) e indexação de listas/ranges (A[i]), inclusive indexação aninhada
de matrizes (A[i][j]). Índices negativos são normalizados e índices inválidos
continuam gerando erro de execução. Quando um nó não é compatível com o bytecode,
o runtime mantém o caminho interpretado como fallback.

O teste de regressão examples/runtime_vm_index_compare.pif cobre comparações,
índices negativos, matriz 2D e uma função com for range que acumula valores
por indexação. Ele faz parte de make test.

Limites atuais: operadores lógicos com curto-circuito, atribuição a elementos de
listas, compilação nativa de funções que recebem arrays e otimização de laços
arbitrários ainda não foram generalizados. O JIT C existente continua restrito
a padrões numéricos suportados; o caminho de bytecode cobre mais casos sem
forçar conversões ou compilação nativa insegura.

## Fusão nativa de cadeias numéricas

O builtin `map_fused_chain(["funcao_a", "funcao_b"], dados[, workers])` compõe grafos IR de funções aritméticas puras e compila uma única cadeia nativa. O kernel fica em cache no processo; a primeira chamada inclui compilação, chamadas seguintes reutilizam o kernel. Aceita listas numéricas e `range(N)`; o resultado é uma lista genérica.

Exemplo: `examples/runtime_fused_chain.pif`. Benchmark medido e ressalvas: `docs/benchmarks/BENCHMARK_FUSAO_CADEIAS_2026-10-19.md`.

Isso ainda não é fusão geral de qualquer AST nem arrays tipados de primeira classe. Funções com efeitos colaterais, condicionais, loops ou indexação de matrizes não entram no IR nativo atual. O buffer F64 é contíguo e alinhado, mas listas genéricas continuam sendo convertidas na fronteira de chamadas.


## Arrays tipados F64 de primeira classe

A linguagem agora oferece `f64_array(valor)`, também disponível como `array_f64(valor)`. Com um inteiro não negativo, aloca um array de doubles contíguo e inicializado com zeros; com uma lista/range numérico, aloca e converte os valores para F64. O objeto é um valor de primeira classe, pode ser atribuído a variáveis e passado a funções/built-ins, e cópias de referência compartilham o mesmo buffer com contagem de referências.

```pif
valores = f64_array([1, 2, 3.5, 4])
print(valores[0])
print(valores[-1])
array_set(valores, 1, 20)
print(valores)

function dobro(x)
    return x * 2

resultado = map_fused_chain(["dobro"], valores)
print(resultado)
```

- `len(array)` retorna o número de elementos.
- `array[i]` é suportado pela VM de bytecode com acesso direto ao buffer contíguo F64; índices negativos são normalizados e os limites são validados.
- `array_set(array, indice, valor)` grava no buffer existente; aliases observam a alteração.
- `for x in array` itera diretamente pelos elementos.
- `map_fused_chain` aceita arrays F64, lê o buffer sem converter cada elemento em um `Value` genérico e devolve outro array F64. O kernel aritmético fundido é compilado para código nativo.

Limites atuais: arrays tipados disponíveis são F64 unidimensionais; não há ainda shape/strides para arrays multidimensionais, tipos F32/I64, slicing ou sintaxe de atribuição `array[i] = x` (use `array_set`). A leitura indexada é executada pelo opcode de bytecode especializado e compilado junto ao runtime nativo; ela ainda não é uma operação de indexação arbitrária incorporada ao IR JIT de funções. O JIT aritmético permanece limitado às funções puras de um argumento suportadas pelo IR.


## JIT nativo de funções com indexação F64

O runtime também oferece `map_indexed_native("nome_da_funcao", array_f64[, workers])`. A função Pifari deve receber dois parâmetros — o array e o índice — e retornar uma expressão aritmética pura que pode conter `array[indice]`. O compilador reconhece esse acesso e gera um kernel C cujo laço lê diretamente `input[i]`; o índice é limitado pelo próprio intervalo `0..len(array)-1` do laço nativo.

```pif
function transformar(A, i)
    return A[i] * 2 + 3

dados = f64_array(range(50000))
resultado = map_indexed_native("transformar", dados)
print(resultado[0])
print(resultado[-1])
```

O kernel e o IR indexado são cacheados na definição da função durante a execução. O primeiro uso inclui a compilação nativa; chamadas seguintes reutilizam o kernel. Por segurança e previsibilidade, o subset atual só aceita `A[i]` com o nome exato dos dois parâmetros e expressões aritméticas puras (`+`, `-`, `*`, `/`, unário `-`). Acesso com offsets (`A[i+1]`), múltiplos arrays, escritas indexadas dentro do kernel, efeitos colaterais e indexação de arrays multidimensionais ainda não fazem parte desse JIT.

Para comparar com Python sem incluir o tempo de inicialização do processo nos tempos internos de Pifari, execute `python3 examples/benchmarks/benchmark_indexed_vs_python.py`. O script mede o kernel Pifari aquecido, o loop interpretado Pifari e duas variantes Python com o mesmo cálculo `out[i] = data[i] * 2 + 3`, valida resultados e reporta medianas.

---

## Atribuição indexada direta (`A[i] = valor` e `A[i][j] = valor`)

A gramática e o runtime agora suportam atribuição direta por índice tanto em `f64_array` quanto em listas 1D e matrizes aninhadas (`A[i][j] = valor`), sem adicionar novos conflitos shift/reduce ao parser (`%expect 7`):

```pif
zeros = f64_array(4)
zeros[0] = 7
zeros[1] = 8.5
zeros[2] = zeros[0] + zeros[1]
zeros[3] = zeros[2] + 4.5
print(zeros)
```

---

## Plataforma de Orquestração Multiagente e MCP Gateway (Novo)

O runtime inclui uma plataforma modular em C (`backend/agent_runtime.[ch]` e `backend/mcp_gateway.[ch]`) com 7 subsistemas integrados:

1. **Task Engine & DAG Paralelo:** `Task(id, agent_id, server_id, tool_name, args_json, output_ctx_key, priority, timeout_ms)`, `task_add_dep(task_id, dep_id)`, `task_cancel(task_id, motivo)`, `task_status(task_id)`, `task_result(task_id)` e `dag_run(max_parallel)`.
2. **Agent Manager:** `Agent(id, role, goal, adapter, max_concurrency, timeout_ms)`.
3. **MCP Gateway (`stdio` e `http` JSON-RPC 2.0, protocolo `2025-03-26`):** `mcp_register`, `mcp_discover_tools`, `mcp_discover_resources`, `mcp_read_resource` e `mcp_call`.
4. **Context Store (compartilhado, versionado e com interpolação `{{ctx:chave}}`):** `ctx_put` e `ctx_get`.
5. **Policy Engine (Default-Deny e separação leitura vs. efeito colateral):** `policy_allow(agent_id, server_id, tool_name, allow_side_effects)`.
6. **Artifact Manager (persistência com checksum FNV-1a 64-bit):** `artifact_save(id, path, content, agent_id, task_id)`.
7. **Telemetry Engine & Observabilidade:** `telemetry_progress()`, `telemetry_export(path)` e painel visual em `dashboard/index.html`.

### Documentação Técnica Adicional

- [docs/STATUS_REAL.md](docs/STATUS_REAL.md) — Diagnóstico arquitetural completo do compilador e runtime.
- [docs/ROADMAP_AGENT_RUNTIME.md](docs/ROADMAP_AGENT_RUNTIME.md) — Plano incremental priorizado por impacto, risco e esforço.
- [docs/ARQUITETURA_AGENT_RUNTIME.md](docs/ARQUITETURA_AGENT_RUNTIME.md) — Especificação dos 7 módulos e integração MCP.
- [docs/PLANO_TESTES_MCP.md](docs/PLANO_TESTES_MCP.md) — Matriz de cobertura de testes unitários e de integração MCP.
- [docs/BENCHMARKS.md](docs/BENCHMARKS.md) — Resultados comparativos medidos antes e depois das otimizações.


## Concorrência, tarefas e dispositivos (2026-10-24)

Semântica normativa em `docs/SPEC_CONCORRENCIA.md`. Coberto por `make test-core`,
`make test-conc` e `make test-gpu`:

- `|>` e `>|` com escopo próprio: a variável de iteração não vaza e a falha fica por item
  (`fluxo paralelo falhou no índice N: ...`), com os demais itens concluindo.
- Fluxo aninhado não trava mais (roda serialmente dentro do trabalhador).
- `modo_processos(n)` / `modo_threads()`: distribuição em processos via `fork()` e contador
  compartilhado; resultado idêntico ao modo threads.
- `spawn expr` / `await id` / `cancelar(id)`: tarefas com ambiente clonado e cancelamento
  cooperativo nos laços de execução.
- `compartilhado(v)` / `ler` / `escrever` / `incrementar`: único caminho de estado compartilhado.
- `dispositivos()` / `usar(id)`: descoberta de CPU e GPU (OpenCL). O kernel roda na Iris Xe,
  mas o transporte WSL↔Windows domina: `docs/BENCHMARKS_GPU.md` registra a GPU ~7–8× mais lenta
  nesses tamanhos, então `auto` permanece CPU e a GPU é escolha explícita.

## Tipos, efeitos e otimização (2026-10-24)

- **Tipos verificados**: `=:` confere o valor (com promoção `int → float`), parâmetros e
  retorno podem ser anotados (`function f(a: int) -> float`) e `pifari --check` valida antes
  de executar. Semântica completa em `docs/SPEC_SEMANTICA.md`.
- **Efeitos**: cada função é classificada em puro/leitura/escrita/io; só `puro` pode virar
  kernel nativo. Consultável em Pifari com `efeitos("nome")`.
- **Planos**: `PIFARI_OPT=0|1|2`; `PIFARI_OPT_DEBUG=1` mostra a decisão. JIT com tiering
  (compila depois de 16 chamadas ou 50 ms) em vez de sempre.
- **Aquecimento**: `~/.cache/pifari/` guarda `.so` e um manifesto por função; a segunda
  execução do mesmo programa entra no JIT na primeira chamada (~78× mais rápida na carga de
  teste). `PIFARI_WARMUP=off` desliga.
- **Prova**: `make test-equiv` compara saída byte a byte entre `OPT=0` e `OPT=2`;
  `make test-golden` congela 23 programas. Números em `docs/OTIMIZACOES.md`.


## Guia didático interativo

O guia local em `docs/guia-didatico/` explica a teoria do lexer, parser, AST, runtime, VM/bytecode, tipos, otimizações, concorrência, MCP e testes. Inclui catálogo pesquisável dos fontes, testes, exemplos e documentos, glossário e quiz.

No Windows, abra com:

```powershell
powershell -ExecutionPolicy Bypass -File .\docs\guia-didatico\abrir_guia.ps1
```

O script regenera o catálogo e abre o site em localhost. O catálogo exclui o `.env` pessoal, binários e arquivos gerados em `build/`.

## Tutorial interativo

[Tutorial do básico ao avançado](docs/tutorial-pifari/dist/index.html) — aulas, exemplos e exercícios. Os arquivos de estilo, navegação e conteúdo ficam em `docs/tutorial-pifari/dist/assets/`.

## Recuperação de erros (hotfix)

O frontend acumula diagnósticos com recuperação léxica, modo pânico por linha/bloco,
recuperação local de expressões e uma produção para vírgula sem argumento.
Programas com erros não são executados. Consulte [técnicas, limites e testes](docs/RECUPERACAO_ERROS.md).
Execute `make test-errors`; o alvo também integra `make test-core`.
