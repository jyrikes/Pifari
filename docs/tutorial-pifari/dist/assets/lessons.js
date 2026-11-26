window.LESSONS = [
  {
    id: 0,
    title: "Comece por aqui",
    level: "Básico",
    file: "basico.pif",
    intro:
      "Pifari é uma linguagem cuja implementação principal combina um frontend Flex/Bison com um runtime em C. Neste curso, você aprende a escrever programas e depois descobre como eles são executados.",
    body: "Instale GCC, Flex, Bison e Make no Linux ou WSL. Na pasta frontend_flex_bison, rode make. Salve o código em aula.pif e execute ./pifari aula.pif. Execute a partir dessa pasta para que os caminhos dos exemplos funcionem.",
    exercise: "Troque a idade por 16 e observe qual mensagem aparece.",
    answer: "O programa escolhe o ramo else e imprime menor de idade.",
    code: '@ Exemplo básico de Pifari\nidade =: int(20)\nnome = "Yngrid"\nif idade >= 18\n    print("maior de idade")\nelse\n    print("menor de idade")\n',
  },
  {
    id: 1,
    title: "Valores, nomes e tipos",
    level: "Básico",
    file: "basico.pif",
    intro:
      "Uma atribuição associa um nome a um valor. Strings usam aspas; inteiros representam contagens. O exemplo também mostra a forma idade =: int(20).",
    body: "Leia o programa de cima para baixo. nome recebe uma string; idade recebe o inteiro 20. O símbolo @ inicia um comentário nos exemplos. Use quatro espaços para tornar os blocos fáceis de ler.",
    exercise: "Adicione print(nome) depois da atribuição de nome.",
    answer: "A saída inclui Yngrid antes do resultado da condição.",
    code: '@ Exemplo básico de Pifari\nidade =: int(20)\nnome = "Yngrid"\nif idade >= 18\n    print("maior de idade")\nelse\n    print("menor de idade")\n',
  },
  {
    id: 2,
    title: "Decisões com if e else",
    level: "Básico",
    file: "basico.pif",
    intro:
      "Uma condição compara valores e escolhe um bloco. A indentação delimita o corpo de if e else.",
    body: "idade >= 18 verifica se a idade é maior ou igual a 18. Apenas um dos dois blocos é executado. Use if explicitamente: a forma de condição implícita presente em rascunhos antigos não é suportada.",
    exercise: "Teste as idades 17, 18 e 19.",
    answer: "17 entra no else; 18 e 19 entram no if.",
    code: '@ Exemplo básico de Pifari\nidade =: int(20)\nnome = "Yngrid"\nif idade >= 18\n    print("maior de idade")\nelse\n    print("menor de idade")\n',
  },
  {
    id: 3,
    title: "Listas e repetição",
    level: "Básico",
    file: "runtime_lista.pif",
    intro:
      "Uma lista agrupa valores em ordem. O laço for percorre esses valores e dá um nome ao elemento atual.",
    body: "A cada volta, numero recebe um elemento da lista. print(numero * 2) calcula o dobro sem modificar a lista. len(numeros) informa quantos elementos existem.",
    exercise: "Inclua 5 na lista. Quantos valores serão impressos pelo laço?",
    answer: "Cinco valores: 2, 4, 6, 8 e 10. O comprimento passa a ser 5.",
    code: 'numeros = [1, 2, 3, 4]\nfor numero in numeros\n    print(numero * 2)\nprint("quantidade", len(numeros))\n',
  },
  {
    id: 4,
    title: "Funções e recursão",
    level: "Básico",
    file: "funcoes.pif",
    intro:
      "Uma função reúne instruções sob um nome. Parâmetros recebem os argumentos e return entrega o resultado ao chamador.",
    body: "soma tem dois parâmetros. dobro reaproveita soma. fatorial chama a si mesma com um valor menor; n <= 1 é a condição que encerra a recursão. O exemplo também usa um fluxo, que será estudado na próxima trilha.",
    exercise: "Calcule soma(10, 7) e fatorial(4).",
    answer:
      "Os resultados são 17 e 24. Sem o caso de parada, a recursão não teria um término definido.",
    code: "function soma(a, b)\n    return a + b\n\nfunction dobro(x)\n    return soma(x, x)\n\nprint(soma(2, 3))\nprint(dobro(8))\nvalores = [1, 2, 3, 4]\nvalores >| valor >| print(dobro(valor))\n\nfunction fatorial(n)\n    if n <= 1\n        return 1\n    else\n        return n * fatorial(n - 1)\n\nprint(fatorial(5))\n",
  },
  {
    id: 5,
    title: "Índices e matrizes",
    level: "Intermediário",
    file: "runtime_matriz.pif",
    intro:
      "Listas podem conter outras listas. Dois índices acessam primeiro a linha e depois a coluna.",
    body: "A[0] é a primeira linha. A[0][1] acessa o segundo elemento dessa linha. Os índices começam em zero.",
    exercise: "Acesse o número 4.",
    answer: "Use print(A[1][1]).",
    code: "A = [[1, 2], [3, 4]]\nprint(A[0][1])\nprint(A[1][0])\n",
  },
  {
    id: 6,
    title: "Fluxos sequenciais",
    level: "Intermediário",
    file: "fluxo_sequencial.pif",
    intro:
      "O operador |> expressa um fluxo sequencial sobre uma coleção. A variável intermediária dá nome ao item.",
    body: "Leia numeros |> numero |> print(numero) como: percorra numeros, chame cada item de numero e imprima-o. A forma do exemplo é parte da gramática atual; não confunda com o pipe de um shell.",
    exercise: "Troque print(numero) por print(numero * 3).",
    answer: "Para [1, 2, 3, 4], os resultados são 3, 6, 9 e 12.",
    code: "numeros = [1, 2, 3, 4]\nnumeros |> numero |> print(numero)\n",
  },
  {
    id: 7,
    title: "Fluxos paralelos",
    level: "Intermediário",
    file: "fluxo_paralelo.pif",
    intro:
      "Fluxos paralelos distribuem trabalho. Eles são úteis quando os itens podem ser processados de forma independente.",
    body: "Observe o operador usado no código. Uma execução concorrente não deve depender da ordem de mensagens impressas. Comece com operações puras, sem escrita compartilhada, e compare com a versão sequencial.",
    exercise:
      "Explique por que comparar apenas a ordem dos prints pode produzir um falso erro.",
    answer:
      "O escalonamento pode mudar a ordem observada sem mudar os resultados de cada cálculo.",
    code: "numeros = [1, 2, 3, 4, 5, 6, 7, 8]\nnumeros >| numero >| print(numero)\n",
  },
  {
    id: 8,
    title: "Coletar resultados",
    level: "Intermediário",
    file: "fluxo_coleta.pif",
    intro:
      "Processar itens e coletar os resultados são etapas diferentes. Este exemplo mostra a sintaxe de coleta implementada.",
    body: "Acompanhe a coleção de origem, a transformação e o destino. Compare com um fluxo que apenas imprime: a coleta permite que o programa use os resultados em uma etapa posterior.",
    exercise: "Modifique a transformação para multiplicar cada item por 2.",
    answer:
      "Confira os valores coletados e compare-os com os dobros dos valores de entrada.",
    code: "numeros = [1, 2, 3, 4]\nsoma : (x){\n    x*2\n}\nnumeros >| numero >| soma(numero) |< resultados\nprint(resultados)\n",
  },
  {
    id: 9,
    title: "Arrays numéricos tipados",
    level: "Intermediário",
    file: "typed_arrays.pif",
    intro:
      "f64_array cria armazenamento numérico contíguo de dupla precisão. É uma base para os caminhos otimizados.",
    body: "f64_array(4) cria quatro posições. array_set altera uma posição. alias = valores compartilha o mesmo array: uma mudança por um nome pode ser observada pelo outro. O exemplo também demonstra índice negativo e atribuição indexada.",
    exercise: "Por que alias[1] mostra 20?",
    answer:
      "alias e valores referenciam o mesmo array, modificado por array_set(valores, 1, 20).",
    code: 'valores = f64_array([1, 2, 3.5, 4])\nprint(valores)\nprint(len(valores))\nprint(valores[0])\nprint(valores[-1])\n\nalias = valores\narray_set(valores, 1, 20)\nprint(alias[1])\nprint(valores)\n\nzeros = f64_array(4)\narray_set(zeros, 0, 7)\narray_set(zeros, 1, 8.5)\narray_set(zeros, 2, zeros[0] + zeros[1])\nzeros[3] = zeros[2] + 4.5\nprint(zeros)\nprint(len(zeros))\n\nsoma = 0\nfor x in valores\n    soma = soma + x\nprint(soma)\n\nfunction dobro(x)\n    return x * 2\n\nfunction soma_tres(x)\n    return x + 3\n\ntransformado = map_fused_chain(["dobro", "soma_tres"], valores)\nprint(transformado)\nprint(transformado[0])\n',
  },
  {
    id: 10,
    title: "Fundir transformações",
    level: "Intermediário",
    file: "runtime_fused_chain.pif",
    intro:
      "Uma cadeia fundida combina transformações numéricas compatíveis para reduzir trabalho intermediário.",
    body: "Leia as funções da cadeia e a ordem em que são aplicadas. A otimização depende do subconjunto aceito pelo runtime; ela não transforma qualquer programa arbitrário em código nativo.",
    exercise:
      "Compare a composição f(g(x)) com g(f(x)) usando duas funções diferentes.",
    answer:
      "A ordem geralmente altera o resultado: dobrar e depois somar 3 difere de somar 3 e depois dobrar.",
    code: 'function dobro(x)\n    return x * 2\n\nfunction soma_tres(x)\n    return x + 3\n\nfunction quadrado(x)\n    return x * x\n\ndados = [1, 2, 3, 4, 5]\netapas = dados >| x >| dobro(x)\netapas >| x >| soma_tres(x) |< sequencial\nfundido = map_fused_chain(["dobro", "soma_tres"], dados)\nprint(sequencial)\nprint(fundido)\nprint(map_fused_chain(["dobro", "soma_tres", "quadrado"], dados))\n',
  },
  {
    id: 11,
    title: "Tarefas e espera",
    level: "Avançado",
    file: "conc_tarefas.pif",
    intro:
      "spawn inicia uma tarefa e retorna seu identificador. await espera seu resultado. cancelar solicita cancelamento cooperativo.",
    body: "As duas somas podem avançar independentemente. O resultado só é necessário no await. Cancelamento cooperativo significa que a tarefa para em um ponto seguro, não necessariamente na mesma instrução em que o pedido é feito.",
    exercise: "Calcule manualmente soma_ate(10).",
    answer: "range(10) percorre 0 até 9; a soma é 45.",
    code: '@ Tarefas: spawn / await / cancelar (docs/SPEC_CONCORRENCIA.md §5)\n@ spawn inicia imediatamente e devolve o identificador; await espera o valor.\n@ A falha de uma tarefa propaga no ponto de espera (caso C10 do SPEC) — para não\n@ deixar este exemplo com saída de erro, ele cobre só o caminho feliz.\n\nfunction soma_ate(n)\n  total = 0\n  for i in range(n)\n    total = total + i\n  return total\n\nt1 = spawn soma_ate(1000)\nt2 = spawn soma_ate(10)\nprint("ids:", t1, t2)\n\nprint("resultados:", await t1, await t2)\n\n@ Cancelamento cooperativo: a tarefa para no próximo ponto seguro.\nfunction lenta()\n  total = 0\n  for bloco in range(20)\n    for i in range(900000)\n      total = total + 1\n  return total\n\nt3 = spawn lenta()\nsleep_ms(5)\ncancelar(t3)\nprint("cancelada antes de terminar")\n',
  },
  {
    id: 12,
    title: "Reduções nativas",
    level: "Avançado",
    file: "reducao_nativa.pif",
    intro:
      "Certos laços de redução inteira podem ser compilados para C com aritmética checada. O objetivo é preservar os resultados e reduzir o tempo.",
    body: "O formato elegível possui acumulador inteiro, laço sobre range do parâmetro, atualização aritmética e retorno. O exemplo media usa acumulador float e fica fora desse formato. Overflow continua sendo erro; não é uma licença para alterar a semântica.",
    exercise: "Por que a redução de soma_ate(10) retorna 45, e não 55?",
    answer:
      "O limite superior de range não é incluído. Para somar de 0 a 10, seria preciso percorrer 11 valores.",
    code: '@ Redução nativa de laço puro (docs/SPEC_SEMANTICA.md §8, etapa F3).\n@ As quatro primeiras funções têm exatamente a forma\n@   acc = <inteiro>; for i in range(<parametro>); acc = acc <op> expr(i); return acc\n@ e podem ser executadas por um kernel C com aritmética inteira checada.\n@ A última usa acumulador float: fica fora do formato e segue no interpretador.\n\nfunction soma_ate(n)\n  total = 0\n  for i in range(n)\n    total = total + i\n  return total\n\nfunction soma_quadrados(n)\n  total = 0\n  for i in range(n)\n    total = total + i * i\n  return total\n\nfunction saldo(n)\n  total = 100\n  for i in range(n)\n    total = total - i * 2\n  return total\n\nfunction fatorial(n)\n  total = 1\n  for i in range(n)\n    total = total * (i + 1)\n  return total\n\nfunction media(n)\n  total = 0.0\n  for i in range(n)\n    total = total + i\n  return total\n\nprint("soma_ate(10):", soma_ate(10))\nprint("soma_ate(200000):", soma_ate(200000))\nprint("soma_quadrados(1000):", soma_quadrados(1000))\nprint("saldo(10):", saldo(10))\nprint("fatorial(10):", fatorial(10))\nprint("media(4):", media(4))\nprint("repeticao:", soma_ate(1000))\nprint("repeticao:", soma_ate(1000))\n',
  },
  {
    id: 13,
    title: "Gateway MCP e agentes",
    level: "Avançado",
    file: "prototipo_multiagente_mcp.pif",
    intro:
      "O runtime possui um protótipo de agentes, tarefas e ferramentas MCP. Este exemplo usa um servidor Python local de teste.",
    body: "Primeiro registra e descobre ferramentas; depois cria agentes e permissões. policy_allow autoriza operações específicas. Task cria trabalhos e task_add_dep registra dependências. dag_run executa o grafo. O modelo local-deterministic deste exemplo não representa uma chamada a uma IA remota.",
    exercise:
      "Identifique quais tarefas precisam terminar antes de t_sintese_dependente.",
    answer:
      "t_coleta_paralela e t_analise_paralela. Ambas foram adicionadas como dependências.",
    code: '@ Prototipo Vertical Multiagente & MCP do Pifari\n@ Demonstra:\n@ 1. MCP Gateway (stdio JSON-RPC 2.0, descoberta de ferramentas e recursos)\n@ 2. 3 Agentes com papeis distintos (Coletor, Analista, Relator)\n@ 3. Policy Engine (default-deny + bloqueio de ferramenta restrita)\n@ 4. Task Engine & DAG Paralelo (2 tarefas independentes em paralelo + 1 tarefa dependente)\n@ 5. Context Store (passagem estruturada e versionada de resultados entre agentes)\n@ 6. Cancelamento por timeout de tarefa lenta\n@ 7. Artifact Manager (checksum FNV-1a) e Telemetry Engine (JSON persistido)\n\nok_srv = mcp_register("mcp_lab", "stdio", "python3 tests/mock_mcp_server.py", 2000, 4)\nnum_tools = mcp_discover_tools("mcp_lab")\nnum_res = mcp_discover_resources("mcp_lab")\nspec = mcp_read_resource("mcp_lab", "resource://pifari/runtime_spec")\n\nprint("MCP conectado:", ok_srv, "tools:", num_tools, "resources:", num_res)\nprint("Recurso MCP:", spec)\n\n@ Registro dos 3 agentes com papeis e limites proprios\na1 = Agent("ag_coletor", "Coletor Cientifico", "Coletar series temporais do MCP", "local-deterministic", 2, 1500)\na2 = Agent("ag_analista", "Analista Estatistico", "Extrair metricas e validar anomalias", "local-deterministic", 2, 1500)\na3 = Agent("ag_relator", "Relator Executivo", "Consolidar artefato final auditavel", "local-deterministic", 1, 1500)\n\n@ Permissoes explicitas no Policy Engine (default-deny)\npolicy_allow("ag_coletor", "mcp_lab", "collect_dataset", false)\npolicy_allow("ag_coletor", "mcp_lab", "slow_op", false)\npolicy_allow("ag_analista", "mcp_lab", "analyze_series", false)\npolicy_allow("ag_relator", "mcp_lab", "build_report", false)\n\n@ Teste de bloqueio de seguranca pelo Policy Engine\nbloqueio = mcp_call("ag_coletor", "mcp_lab", "restricted_op", "{}")\nprint("Teste Policy Engine (esperado ERR_POLICY_DENIED):", bloqueio)\n\n@ Contexto inicial compartilhado\nv_ctx = ctx_put("missao.escopo", "Telemetria Pifari Multiagente", "ag_coletor", "bootstrap")\n\n@ Tarefas independentes que executam em paralelo no DAG\nTask("t_coleta_paralela", "ag_coletor", "mcp_lab", "collect_dataset", \'{"source":"sensor_alpha"}\', "coleta.dados", 10, 1500)\nTask("t_analise_paralela", "ag_analista", "mcp_lab", "analyze_series", \'{"dataset":"sensor_alpha_window"}\', "analise.metricas", 10, 1500)\n\n@ Tarefa dependente que aguarda a conclusao das duas tarefas paralelas acima\nTask("t_sintese_dependente", "ag_relator", "mcp_lab", "build_report", \'{"collector_summary":"{{ctx:coleta.dados}}","analysis_summary":"{{ctx:analise.metricas}}"}\', "relatorio.final", 5, 1500)\ntask_add_dep("t_sintese_dependente", "t_coleta_paralela")\ntask_add_dep("t_sintese_dependente", "t_analise_paralela")\n\n@ Tarefa lenta com timeout curto (80ms para operacao de 350ms) para validar cancelamento\nTask("t_lenta_cancelada", "ag_coletor", "mcp_lab", "slow_op", \'{"delay_ms":350}\', "", 1, 80)\n\n@ Executa o grafo de tarefas (DAG) com ate 2 workers paralelos\nconcluidas = dag_run(2)\nprint("Tarefas concluidas com sucesso no DAG:", concluidas)\nprint("Status t_coleta_paralela:", task_status("t_coleta_paralela"))\nprint("Status t_analise_paralela:", task_status("t_analise_paralela"))\nprint("Status t_sintese_dependente:", task_status("t_sintese_dependente"))\nprint("Status t_lenta_cancelada:", task_status("t_lenta_cancelada"))\n\nrelatorio = ctx_get("relatorio.final")\nprint("Resultado no Context Store:", relatorio)\n\n@ Persistencia de artefato e relatorio de telemetria\nchecksum = artifact_save("art_relatorio_executivo", "dashboard/latest_report.txt", relatorio, "ag_relator", "t_sintese_dependente")\nprint("Artefato salvo com checksum FNV-1a:", checksum)\nprint("Progresso final da missao (%):", telemetry_progress())\nok_tel = telemetry_export("dashboard/telemetry_latest.json")\nprint("Telemetria JSON exportada:", ok_tel)\n',
  },
  {
    id: 14,
    title: "Construir um servidor MCP",
    level: "Avançado",
    file: "servidor_mcp_pifari.pif",
    intro:
      "Além de consumir ferramentas, o projeto possui um exemplo de servidor MCP escrito em Pifari.",
    body: "Este é um programa de integração: leia o registro de ferramentas e os handlers antes de executá-lo. Um servidor pode esperar mensagens de entrada; use o cliente de teste do repositório para exercitar o protocolo.",
    exercise: "Localize no código o nome de uma ferramenta e seu handler.",
    answer:
      "Compare o registro com a função que produz o resultado; o cliente deve chamar o mesmo nome registrado.",
    code: '@ Servidor MCP Nativo escrito em Pifari (Protocolo MCP 2025-03-26)\nmcp_server_init("pifari-native-mcp", "1.0.0")\n\n@ 1. Ferramenta de baixa latencia (echo/diagnostico JSON-RPC)\nfunction ping_echo(msg)\n    return "PONG:" + str(msg)\n\n@ 2. Ferramenta numerica escalar intensiva (compilada por JIT/VM no Pifari)\nfunction soma_quadrados(limite)\n    soma = 0\n    for i in range(limite)\n        soma = soma + (i * i - i // 2)\n    return soma\n\n@ 3. Ferramenta vetorial cientifica F64 com fusao nativa de cadeias\nfunction etapa_afim(x)\n    return x * 1.75 + 2.5\n\nfunction etapa_quad(x)\n    return x * x * 0.001 + x\n\nfunction processar_vetor_f64(tamanho)\n    arr = f64_array(range(int(tamanho)))\n    out = map_fused_chain(["etapa_afim", "etapa_quad"], arr)\n    return out[-1]\n\n@ 4. Ferramenta de algebra linear (multiplicacao de matrizes matmul)\nfunction matmul_bench(dim)\n    n = int(dim)\n    A = []\n    B = []\n    for i in range(n)\n        rowA = []\n        rowB = []\n        for j in range(n)\n            rowA = rowA + [(i + j + 1) * 0.5]\n            rowB = rowB + [(i * 2 + j + 1) * 0.25]\n        A = A + [rowA]\n        B = B + [rowB]\n    C = matmul(A, B)\n    return C[n - 1][n - 1]\n\n@ Registro declarativo das ferramentas e recursos MCP\nmcp_server_register_tool("ping_echo", "Ecoa mensagem para medir latencia pura JSON-RPC", "ping_echo", ["msg"], true)\nmcp_server_register_tool("soma_quadrados", "Calcula soma quadratica intensiva em CPU", "soma_quadrados", ["limite"], true)\nmcp_server_register_tool("processar_vetor_f64", "Executa cadeia vetorial F64 fundida em codigo nativo", "processar_vetor_f64", ["tamanho"], true)\nmcp_server_register_tool("matmul_bench", "Executa multiplicacao nativa de matrizes NxN", "matmul_bench", ["dim"], true)\n\nmcp_server_register_resource("resource://pifari/manifest", "Manifesto do Servidor MCP Pifari", "application/json", \'{"server":"pifari-native-mcp","language":"Pifari","protocol":"2025-03-26","features":["jit","f64_array","fused_chain","matmul"]}\')\n\n@ Inicia o loop principal JSON-RPC 2.0 sobre stdio\nmcp_server_run_stdio()\n',
  },
  {
    id: 15,
    title: "Do texto à execução",
    level: "Avançado",
    file: null,
    intro:
      "O compilador e o runtime formam um percurso: texto → tokens → árvore sintática → execução. Otimizações entram apenas quando o programa é compatível.",
    body: "frontend/lexer.l reconhece tokens e indentação. frontend/parser.y constrói a AST definida em frontend/ast.h. backend/runtime.c executa o programa e coordena caminhos como bytecode e kernels. backend/optimization/ast_ir.c e optimizer.c trabalham com a representação intermediária numérica.",
    exercise: "Em qual etapa investigar um erro de reconhecimento de símbolo?",
    answer:
      "Comece pelo lexer. Para combinação inválida de tokens, examine a gramática; para comportamento incorreto de execução, investigue o runtime.",
    code: "frontend/lexer.l\n    ↓ tokens\nfrontend/parser.y + frontend/ast.h\n    ↓ AST\nbackend/runtime.c\n    ↓\ninterpretação · bytecode · kernels compatíveis",
  },
  {
    id: 16,
    title: "Testar e medir",
    level: "Avançado",
    file: null,
    intro:
      "Uma otimização precisa preservar comportamento antes de ser avaliada pela velocidade. O repositório oferece testes centrais, golden e equivalência.",
    body: "make test-core executa o núcleo. make test-golden compara saídas de referência. make test-equiv compara modos de execução. Nos benchmarks, separe compilação, aquecimento e execução; mantenha entradas e ambiente iguais. Não generalize uma medição para toda a linguagem.",
    exercise:
      "Por que medir só a primeira execução pode distorcer a comparação?",
    answer:
      "Ela pode incluir compilação e preparação de caches que não se repetem nas execuções aquecidas.",
    code: "make test-core\nmake test-golden\nmake test-equiv",
  },
  {
    id: 17,
    title: "Limites e próximos estudos",
    level: "Avançado",
    file: null,
    intro:
      "O projeto é experimental. A especificação da linguagem é mais ampla que o conjunto atualmente executável.",
    body: "Classes reconhecidas na gramática não implicam suporte completo a objetos e métodos. Os módulos Dirac e reversível têm APIs C e testes, mas isso não significa sintaxe disponível em .pif. GPU depende do ambiente e da integração. Consulte código e testes atuais ao usar relatórios históricos.",
    exercise:
      "Escolha um recurso e encontre seu exemplo, implementação e teste.",
    answer:
      "Uma boa verificação cruza os três: sintaxe demonstrada, caminho real no runtime e comportamento coberto por teste.",
    code: "frontend/lexer.l\n    ↓ tokens\nfrontend/parser.y + frontend/ast.h\n    ↓ AST\nbackend/runtime.c\n    ↓\ninterpretação · bytecode · kernels compatíveis",
  },
];
