# Release acadêmica — 26/11/2026

Modalidade: interpretador em C11, com Flex e Bison. Escopo estável pretendido: núcleo da linguagem; JIT, GPU, MCP e concorrência avançada são extensões.

## Reprodução
Na pasta Pifari, em Linux/WSL com GCC, Flex, Bison, Make e Python 3:
```sh
make clean
make
python3 tests/test_academic.py
make test-core check-core test-conc test-equiv test-golden
```

A recompilação e os 10 casos de conformidade acadêmica passaram em Ubuntu 24.04 em 26/11/2026. Os casos incluem precedência, associatividade, atribuições, saída, condicionais, repetição, erros léxicos, sintáticos, nomes não declarados, incompatibilidade de tipos e divisão por zero.

## Arquitetura
lexer.l -> parser.y -> AST (ast.h) -> análise semântica (--check, typecheck.c) ou execução (runtime.c). TcScope registra nomes, tipos e escopos. A análise estática é um modo separado; a execução também verifica erros dinamicamente. A gramática Bison declara sete conflitos shift/reduce conhecidos; estes devem ser explicados na apresentação.

## Limites e entrega
A suíte acadêmica não certifica todas as extensões nem a correção de todos os programas possíveis. A aprovação da proposta, a apresentação individual, a disponibilização do Git ao professor e a declaração completa de IA dependem da equipe. A documentação principal está em especificacao/Definicao_Pifari_v3.tex e docs/SPEC_SEMANTICA.md; o código e os testes determinam as construções efetivamente implementadas.

## Uso de IA
ChatGPT foi utilizado nesta rodada para revisão do histórico Git, elaboração dos testes acadêmicos e preparação da release. A equipe deve complementar a declaração com as demais ferramentas e atividades efetivamente utilizadas ao longo do projeto.
