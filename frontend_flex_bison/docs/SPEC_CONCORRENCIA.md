# SPEC_CONCORRENCIA — Semântica normativa de fluxos, tarefas e dispositivos

Versão 1.0 — decisão de projeto que fecha a divergência entre `Definicao_Pifari_v3.tex`
(§224–246) e a implementação de referência (`backend/runtime.c`).

Convenção de termos: **DEVE** (requisito), **NÃO DEVE** (proibição), **PODE** (opcional),
igual à §46 da definição.

---

## 1. Princípio único

Um programa Pifari DEVE produzir o mesmo resultado observável quando executado de forma
sequencial ou paralela. Concorrência NÃO DEVE alterar silenciosamente a semântica
sequencial. Toda diferença entre os modos de execução é **erro de implementação**, não
recurso.

Corolário: o modo de distribuição (`threads` ou `processos`) e o dispositivo de execução
(CPU, GPU local, worker remoto) são decisões de **runtime** e NÃO DEVEM mudar o resultado.

---

## 2. Operadores de fluxo

| Operador | Nome | Semântica | Resultado |
|---|---|---|---|
| `|>` | fluxo sequencial | itera na ordem, um item por vez | `Null` (efeito colateral) |
| `>|` | distribuição paralela | divide os itens entre trabalhadores; ordem de aplicação é livre; ordem de coleta é a de entrada | `List` (resultados por item) |
| `|<` | barreira de junção | espera o fluxo à esquerda terminar e agrega os resultados em ordem de entrada | ver §2.1 |
| `<|` | continuação | avalia o fluxo à esquerda até a conclusão e só então avalia a direita | valor da direita |

### 2.1 A regra do `|<` (resolve a divergência spec ↔ implementação)

`|<` é **uma barreira que produz a lista agregada**, e não uma instrução de atribuição.
Por compatibilidade com os programas existentes, vale esta regra única:

- se o operando à direita for um **identificador ainda não ligado** no escopo corrente,
  ele recebe a lista agregada (forma de ligação); a expressão `|<` NÃO DEVE ser usada
  como valor nesse caso;
- caso contrário, a expressão `|<` **é** a lista agregada e PODE ser usada como valor.

```
resultados = [1, 2, 3] >| x >| dobro(x) |<    # forma de expressão (nova)
[1, 2, 3] >| x >| dobro(x) |< resultados      # forma de ligação (existente)
```

O agregado DEVE estar na ordem de entrada, nunca na ordem de término das tarefas.

### 2.2 Alvo do fluxo

O lado direito de `|>` e `>|` DEVE ser uma expressão arbitrária avaliada uma vez por item,
com a variável de iteração ligada. A implementação PODE reconhecer a forma `Call` de
função de usuário pura e usar um caminho nativo, mas o comportamento observável DEVE ser
idêntico ao da avaliação genérica.

### 2.3 Aninhamento

Um fluxo DENTRO de outro fluxo PODE ser executado serialmente no trabalhador que o
contém. Aninhamento NÃO DEVE causar deadlock nem resultado diferente do sequencial.

---

## 3. Escopo e isolamento

1. A variável de iteração vive em um **frame próprio do fluxo** e NÃO DEVE sobreviver a ele.
2. O ambiente externo é visível **somente para leitura** dentro do fluxo.
3. `Assign` ou `IndexAssign` que resolva para um binding fora do frame do fluxo DEVE
   falhar em tempo de execução, com linha e coluna:

   ```
   escrita em '<nome>' dentro de fluxo paralelo não é compartilhada; use compartilhado()
   ```

4. Chamadas de função continuam com frames próprios: escrever em local de função nunca
   propaga para o chamador (regra já vigente, §"Limites atuais das funções" do README).
5. Compartilhamento entre trabalhadores SÓ PODE ocorrer por `compartilhado()` (§6).

---

## 4. Falha, cancelamento e ciclo de vida

1. Falha fica associada ao **item** que falhou. Itens independentes continuam.
2. `>|` DEVE devolver uma lista com o resultado de cada item; itens que falharam são
   contabilizados e a barreira `|<` DEVE reportar o **primeiro** erro com índice e mensagem.
3. Uma falha NÃO DEVE encerrar o processo, salvo falha não recuperável ou pedido explícito.
4. Cancelamento é cooperativo, verificado por item e em início de etapa.
5. Estados de tarefa (spec §228): `Created`, `Ready`, `Running`, `Waiting`, `Succeeded`,
   `Failed`, `Cancelled`. Terminais: os três últimos.
6. O programa NÃO DEVE terminar enquanto houver tarefa ou worker pertencente ao seu escopo.

---

## 5. Tarefas

- `spawn expressao` cria e **inicia imediatamente** uma tarefa (regra única, spec §228).
- `await tarefa` espera e devolve o valor; se a tarefa falhou, propaga a falha original.
- `cancelar(tarefa)` solicita cancelamento; a tarefa termina `Cancelled` no próximo ponto
  seguro. Operação nativa não interrompível DEVE declarar a limitação.

---

## 6. Memória compartilhada

`compartilhado(valor)` cria uma célula protegida por mutex. `ler(c)`, `escrever(c, v)` e
`incrementar(c, delta)` são atômicos em relação aos trabalhadores. O runtime protege o
acesso; nenhuma corrida lógica é detectada automaticamente (spec §246).

---

## 7. Dispositivos de execução

1. `cpu:0` é sempre o dispositivo 0 e o **fallback obrigatório**.
2. `dispositivos()` devolve a lista descoberta: `[id, backend, nome, memória_MB, ops]`.
3. `usar(id)` fixa o dispositivo dos fluxos seguintes; `usar("auto")` devolve a escolha
   automática.
4. A escolha automática DEVE ser guiada por medição publicada em `docs/BENCHMARKS_GPU.md`,
   nunca por preferência. Se a GPU não ganhar na faixa testada, `auto` permanece em CPU e
   a GPU só funciona por escolha explícita.
5. Indisponibilidade de dispositivo (worker morto, kernel que não compila, timeout) DEVE
   degradar para CPU com aviso em stderr, sem falhar o fluxo.
6. A linguagem NÃO promete hardware: promete descoberta e colocação determinística.

---

## 8. Matriz de estado

| Recurso | Estado | Etapa |
|---|---|---|
| `|>` / `>|` / `|<` / `<|` | implementado | — |
| Iterador não vaza (ambos os fluxos) | pendente | E1 |
| Falha por índice | pendente | E1 |
| Fluxo aninhado sem deadlock | pendente | E1 |
| `modo_processos(n)` / `modo_threads()` | pendente | E2 |
| `dispositivos()` / `usar()` | pendente | E3 |
| Offload OpenCL (iGPU / remoto) | pendente | E4 |
| `spawn` / `await` / `cancelar` | pendente | E5 |
| `compartilhado` e erro de escrita externa | pendente | E6 |

---

## 9. Testes de conformidade (viram `tests/test_concorrencia.c` e `examples/conc_*.pif`)

| # | Programa | Saída esperada |
|---|---|---|
| C1 | `xs |> n |> f(n)` e depois `print(n)` | erro "não declarada" (iterador não vaza) |
| C2 | mesmo programa com `>|` | idêntico a C1 |
| C3 | `>|` dentro de `>|` | termina; resultado igual ao sequencial |
| C4 | lista com um item que divide por zero | demais itens completos; erro com índice na barreira |
| C5 | `[1,2,3] >| x >| dobro(x) |<` como expressão | `[2,4,6]` |
| C6 | `[1,2,3] >| x >| dobro(x) |< r` | `r == [2,4,6]` |
| C7 | `modo_processos(2)` sobre soma de 1000 itens | idêntico ao modo threads |
| C8 | `compartilhado(0)` + 4×1000 `incrementar` | `4000` |
| C9 | `acc = 0` e fluxo que faz `acc = acc + x` | erro de escrita não compartilhada |
| C10 | `t = spawn lento(200)`; `cancelar(t)`; `await t` | estado `Cancelled` em < 500 ms |
| C11 | `dispositivos()` | contém `cpu:0`; contém `gpu:0` após bootstrap |
| C12 | `usar("gpu:0")` em `[1..N] >| f(x)` | idêntico ao CPU (tolerância 1e‑9 em float) |
