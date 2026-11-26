# Benchmark MLP XOR — Pifari vs Python (2026-10-19)

## Modelo e protocolo
- MLP totalmente conectado 2–4–1, sigmoid nas duas camadas, backpropagation e SGD.
- Tarefa XOR: quatro exemplos de treinamento; teste em quatro coordenadas diferentes, próximas aos quatro cantos.
- 3.000 épocas, learning rate 0,7, pesos iniciais idênticos.
- Implementação escalar, sem NumPy, PyTorch, TensorFlow ou bibliotecas de ML. Python usa apenas a biblioteca padrão; Pifari usa seu runtime e o builtin matemático `exp`.
- O tempo medido cobre somente o laço de treinamento. O benchmark executa cinco processos Pifari, calcula a mediana dos tempos internos, e o script Python calcula a mediana de cinco treinos.

## Resultado medido no ambiente WSL local
- Pifari: mediana de **134,106 ms**; amostras: 170,445; 132,494; 125,870; 136,254; 134,106 ms.
- Python puro: mediana de **14,005 ms**.
- Python foi **9,58× mais rápido** neste teste escalar interpretado.
- Ambos chegaram a **100% de acurácia** nos quatro exemplos de treinamento e nos quatro pontos de teste.
- Probabilidades Pifari aproximadas no teste: 0,104; 0,967; 0,967; 0,096.

Reproduzir com `python3 examples/benchmarks/benchmark_mlp_xor.py`. Os tempos são específicos desse ambiente e podem variar com a carga do sistema. O MLP é pequeno e a amostra de teste tem apenas quatro pontos; isto é uma validação didática, não uma avaliação estatística de generalização. Não extrapole o resultado para kernels nativos especializados do Pifari nem para Python com bibliotecas vetorizadas.
