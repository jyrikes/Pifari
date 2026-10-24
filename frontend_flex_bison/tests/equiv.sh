#!/bin/sh
# Teste diferencial do contrato de otimização (docs/SPEC_SEMANTICA.md §8):
# a saída e o código de saída precisam ser idênticos com PIFARI_OPT=0 e PIFARI_OPT=2.
#
# Programas cuja própria saída é não determinística (tempo, thread id, print dentro
# de fluxo paralelo) são detectados rodando o mesmo programa duas vezes em OPT=0:
# se já divergem entre si, não servem como oráculo e são pulados.
#
# NUNCA inclua examples/orquestrador_reels_aistudio.pif aqui: ele chama a API do
# Gemini e consome quota.
#
# Uso: tests/equiv.sh examples/a.pif examples/b.pif
cd "$(dirname "$0")/.." || exit 1
fail=0
skip=0
ok=0
for f in "$@"; do
  [ -f "$f" ] || continue
  PIFARI_OPT=0 ./pifari "$f" >/tmp/pifari_eq_a1.txt 2>/dev/null
  e1=$?
  PIFARI_OPT=0 ./pifari "$f" >/tmp/pifari_eq_a2.txt 2>/dev/null
  e2=$?
  if [ "$e1" -ne "$e2" ] || ! cmp -s /tmp/pifari_eq_a1.txt /tmp/pifari_eq_a2.txt; then
    echo "SKIP nao-deterministico: $f"
    skip=$((skip + 1))
    continue
  fi
  PIFARI_OPT=2 ./pifari "$f" >/tmp/pifari_eq_b.txt 2>/dev/null
  b=$?
  if [ "$e1" -ne "$b" ] || ! cmp -s /tmp/pifari_eq_a1.txt /tmp/pifari_eq_b.txt; then
    echo "DIVERGE $f (exit $e1 vs $b)"
    diff /tmp/pifari_eq_a1.txt /tmp/pifari_eq_b.txt | head -4
    fail=1
  else
    ok=$((ok + 1))
  fi
done
echo "iguais=$ok pulados=$skip"
[ $fail -eq 0 ] && echo EQUIV_OK
exit $fail
