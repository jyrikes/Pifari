# SPEC_SEMANTICA — semântica observável do Pifari

Versão 1.0 (F0). Normativo. Complementa `SPEC_CONCORRENCIA.md` (fluxos, tarefas,
dispositivos) e a `Definicao_Pifari_v3.tex` (visão geral).

**Regra superior:** o otimizador só pode mudar **tempo**. Saída, ordem de efeitos,
mensagens de erro e código de saída são observáveis e não podem mudar entre
`PIFARI_OPT=0` e `PIFARI_OPT=2`.

---

## 1. Ordem de avaliação

1. Expressões binárias avaliam o operando esquerdo antes do direito.
2. `and` e `or` têm curto-circuito; o direito pode não ser avaliado.
3. Argumentos de chamada são avaliados da esquerda para a direita, antes da chamada.
4. Instruções de um bloco executam na ordem textual.
5. `return` encerra a função imediatamente; expressões já avaliadas mantêm seus efeitos.
6. Um erro interrompe a execução daquele caminho; o processo termina com código 1
   (ou 0 em sucesso), depois de imprimir a mensagem com arquivo/linha/coluna.

## 2. Modelo inteiro

- `int` é **64 bits com sinal**, complemento de dois.
- **Overflow é erro de execução**, não wrap: `"overflow de inteiro na soma"` (idem
  subtração e multiplicação), na linha do operador. É o que a spec §180 recomenda
  ("erro detectável, sem comportamento indefinido").
- `//` é divisão inteira com **truncamento em direção a zero** (como C), para
  `-7 // 2 == -3`. Vale igualmente no interpretador, na VM de bytecode e no JIT.
- `%` satisfaz `a == (a // b) * b + a % b`; com `a = -7, b = 2` dá `-1`.
- Divisão (`/`, `//`, `%`) por zero é erro de execução.
- `**` com base/expoente incompatíveis exige conversão explícita (`float(...)`).

`int(...)`, `float(...)`, `str(...)`, `bool(...)` são conversões explícitas. Não existe
conversão implícita de `bool` para número nem de número para `bool`.

## 3. Modelo de ponto flutuante

- `float` é IEEE-754 `double` de 64 bits.
- **Sem reassociação e sem fast-math**: `(a + b) + c` não vira `a + (b + c)`.
- `NaN` e `Inf` propagam; comparações com `NaN` seguem IEEE (todas falsas, `!=` verdadeiro).
- `/` sempre produz `float`, mesmo com dois inteiros (`4 / 2` é `2.0`).
- Não há garantia de bit-exatidão entre CPU e GPU: kernels em `float` (a Iris Xe não
  tem `cl_khr_fp64`) têm tolerância documentada em `BENCHMARKS_GPU.md`.

## 4. Listas, arrays e textos

- `list` é **valor**: atribuir copia a referência observável; escrever por índice altera
  o binding alvo, não os aliases já existentes (semântica atual, preservada).
- `f64_array` idem; `array_set(a, i, v)` e `a[i] = v` são equivalentes.
- `texto` (`str`/`txt`) é imutável; `+` concatena.
- `range(n)` é preguiçoso, de `0` a `n-1`, limitado a 1.000.000.
- `null` é ausente; em aritmética vale `0`, mas **não** é `false` em condição (ver §5).

## 5. Condições e booleanos

`if`, `while` e filtros exigem `bool`. A implementação atual aceita "truthiness"
(0, "", lista vazia = falso) por compatibilidade; a verificação estrita entra na F1
junto com o `--check`, atrás de `PIFARI_STRICT=1` para não quebrar programas existentes.
Enquanto isso, o otimizador **não pode** assumir que a condição é `bool`.

## 6. Erros

Categorias (spec §388): léxico, sintático, semântico, execução, ligação, concorrência.
Formato: `Pifari:<linha>:<coluna>: erro de execução: <mensagem>` em stderr, saída 1.
Erro de tarefa preserva a mensagem original: `tarefa falhou: <mensagem>`.
Erro semântico de construção ausente: `"diretiva 'Import' não é suportada nesta versão"`.

## 7. Construções ausentes (erro explícito, nunca silêncio)

| Construção | Hoje |
|---|---|
| `:=` (classe) | parseia, executa com erro semântico |
| `import` | erro semântico |
| `#include` | erro semântico |
| `Link to` | erro semântico |
| `dict` | removido do léxico; `--check` (F1) rejeita como tipo desconhecido |
| closures/lambdas | ausentes; `->` reservado para tipo de retorno |
| módulos | ausentes |

## 8. Otimização permitida

Um plano de execução só pode ser trocado por outro se, para toda entrada:
1. a saída em stdout for byte a byte igual;
2. a ordem dos efeitos (`print`, I/O, escrita em `compartilhado`) for a mesma;
3. erros ocorrerem no mesmo ponto lógico e com a mesma mensagem;
4. o código de saída for o mesmo.

O que **não** é observável e pode mudar: tempo, memória, uso de threads/processos,
dispositivo (CPU/GPU), forma de compilação (AST, bytecode, JIT, kernel nativo).

Guardas e deopts (F3) devem ser invisíveis: cair para o caminho lento no meio é
permitido, mudar resultado não é.

## 9. Modos de execução

| Variável | Efeito |
|---|---|
| `PIFARI_OPT=0` | desliga otimizações; referência de comparação |
| `PIFARI_OPT=1` | conservador (padrão) |
| `PIFARI_OPT=2` | agressivo, ainda sob o contrato do §8 |
| `PIFARI_OPT_DEBUG=1` | loga escolha de plano e deopts em stderr |
| `PIFARI_CHECK=1` | só análise estática, não executa (F1) |
| `PIFARI_STRICT=1` | condições exigem `bool` (F1) |
| `PIFARI_WARMUP=off` | ignora o cache de aquecimento (F4) |

## 10. Divergências conhecidas (registradas, com etapa)

| Divergência | Etapa |
|---|---|
| Truthiness em condição em vez de `bool` estrito | F1 |
| Tipo de anotação `=:` não verificado | F1 |
| Classe/`import`/`#include`/`Link to` não executam | F0 (erro explícito) |
| GPU em `float` diverge de `double` do CPU | E4 (documentado, tolerância) |
