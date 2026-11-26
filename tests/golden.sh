#!/bin/sh
# Golden outputs (etapa F5): congela a saída dos programas determinísticos do
# test-core e compara nas execuções seguintes.
#
# Uso:
#   sh tests/golden.sh examples/a.pif ...   # compara (sai 1 se divergir)
#   sh tests/golden.sh --update examples/*  # regenera os arquivos
#
# Programas cuja saída varia entre duas execuções do mesmo modo são pulados:
# não servem como oráculo (tempo, thread_id, print dentro de fluxo paralelo).
cd "$(dirname "$0")/.." || exit 1
update=0
if [ "$1" = "--update" ]; then update=1; shift; fi
mkdir -p tests/golden
fail=0
skip=0
ok=0
for f in "$@"; do
  [ -f "$f" ] || continue
  # determina se o programa é um oráculo estável: 3 execuções idênticas
  PIFARI_OPT=1 ./pifari "$f" >/tmp/pifari_g1.txt 2>/dev/null
  e1=$?
  stable=1
  for attempt in 2 3; do
    PIFARI_OPT=1 ./pifari "$f" >/tmp/pifari_gx.txt 2>/dev/null
    ex=$?
    if [ "$e1" -ne "$ex" ] || ! cmp -s /tmp/pifari_g1.txt /tmp/pifari_gx.txt; then
      stable=0
      break
    fi
  done
  if [ $stable -eq 0 ]; then
    echo "SKIP nao-deterministico: $f"
    skip=$((skip + 1))
    continue
  fi
  name=$(basename "$f" .pif)
  golden="tests/golden/$name.txt"
  if [ $update -eq 1 ]; then
    cp /tmp/pifari_g1.txt "$golden"
    echo "$e1" >"tests/golden/$name.exit"
    ok=$((ok + 1))
    continue
  fi
  if [ ! -f "$golden" ]; then
    # programa estável mas ainda sem golden congelado: não bloqueia, só avisa
    echo "SKIP sem golden: $f (rode --update)"
    skip=$((skip + 1))
    continue
  elif ! cmp -s /tmp/pifari_g1.txt "$golden"; then
    echo "MUDOU $f"
    diff "$golden" /tmp/pifari_g1.txt | head -4
    fail=1
  else
    ok=$((ok + 1))
  fi
done
if [ $update -eq 1 ]; then
  echo "gravados=$ok pulados=$skip"
  exit 0
fi
echo "iguais=$ok pulados=$skip"
[ $fail -eq 0 ] && echo GOLDEN_OK
exit $fail
