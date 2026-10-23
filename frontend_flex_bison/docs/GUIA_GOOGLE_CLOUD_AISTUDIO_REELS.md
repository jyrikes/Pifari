# Guia Prático: Destravar Google Cloud (Sem pagar R\$ 170) + Usar AI Studio & Veo para Criar Reels no Pifari

---

## 1. Como resolver o problema dos 2 Perfis do Google Cloud e recuperar seus R\$ 30,00

Pelos alertas da sua conta Google (`JOSE YRIKES DE OLIVEIRA FEITOSA`), existem dois perfis de pagamento criados no Brasil:
- **Perfil 1 (`2380-9537-1366`)** $\rightarrow$ vinculado à conta **pré-paga/manual** `Google Cloud 01F04B-24B84B-38C001`.
- **Perfil 2 (`9727-1579-3282`)** $\rightarrow$ vinculado à conta **automática (cartão)** `Google Cloud 01E8F2-EDCEF0-4EC6F3`.

### Por que os R\$ 30,00 não ativaram a conta `01F04B-24B84B-38C001`?
Quando uma conta do Google Cloud é criada no Brasil usando **Pix, boleto ou cartão pré-pago/débito**, ela entra no regime de **Pagamento Manual (Pré-pago)**. Nesse regime, o sistema do Google exige obrigatoriamente um saldo mínimo inicial de **R\$ 170,00** para liberar o serviço. Os seus **R\$ 30,00 estão guardados como crédito** lá dentro, mas a conta permanece bloqueada esperando mais R\$ 140,00.

### Passo a passo para resolver sem gastar R\$ 170,00:

1. **Destravar o Google Cloud de graça (e ganhar os US\$ 300 / ~R\$ 1.700 de crédito gratuito para gerar vídeos no Veo):**
   - Abra [payments.google.com](https://payments.google.com) e clique no ícone do sino (**Alertas**).
   - No 3º alerta (**"Seu pagamento foi recusado — Google Cloud `01E8F2-EDCEF0-4EC6F3`"**), clique em **Corrigir**.
   - Insira um **cartão de crédito** (ou gere um **Cartão Virtual na função crédito** no app do seu banco com pelo menos R\$ 15,00 de limite disponível e compras online/internacionais ativas).
   - Essa segunda conta (`01E8F2-EDCEF0-4EC6F3`) é pós-paga: ela **não exige depósito de R\$ 170,00**, ativa na hora e libera os **US\$ 300 de créditos gratuitos do Google Cloud** para usar o **Vertex AI / Veo 2 / Cloud Text-to-Speech**.

2. **Pedir o reembolso dos seus R\$ 30,00 da conta `01F04B-24B84B-38C001`:**
   - Acesse [console.cloud.google.com/billing](https://console.cloud.google.com/billing).
   - Clique na conta `01F04B-24B84B-38C001` $\rightarrow$ no menu esquerdo, clique em **Gerenciamento da conta** $\rightarrow$ clique em **Encerrar conta de faturamento**.
   - Depois de encerrar, volte em [payments.google.com](https://payments.google.com), selecione o perfil onde consta o crédito de **R\$ 30,00** e clique em **Solicitar reembolso** — o Google estorna os R\$ 30,00 de volta para sua conta bancária.

---

## 2. Como usar o Google AI Studio gratuitamente agora mesmo (Mesmo antes de destravar o Google Cloud!)

O **Google AI Studio** tem um plano gratuito (*Free of charge*) que **não depende do Google Cloud Billing**:

1. Acesse [aistudio.google.com/apikey](https://aistudio.google.com/apikey).
2. Clique em **Create API key** (escolha criar em um projeto novo sem faturamento vinculado para ficar no plano *Free of charge*).
3. No terminal WSL, exporte sua chave na variável de ambiente (ou copie `.env.example` para `.env`, que já está protegido no `.gitignore`):
   ```bash
   export GEMINI_API_KEY="SUA_CHAVE_AQUI"
   ```
4. Execute o orquestrador multiagente de Reels no Pifari:
   ```bash
   ./pifari examples/orquestrador_reels_aistudio.pif
   ```

---

## 3. Como funciona o Pipeline Multiagente de Reels no Pifari

O arquivo `examples/orquestrador_reels_aistudio.pif` coordena **2 servidores MCP em paralelo**:
- **`srv_pifari` (`examples/servidor_mcp_pifari.pif`):** Servidor MCP nativo escrito em Pifari que calcula a curva de quadros e ritmo (`36.000` pontos F64) em menos de `2 ms`.
- **`srv_reels` (`examples/servidor_mcp_aistudio_reels.py`):** Servidor MCP conectado ao **Google AI Studio (`gemini-2.5-flash`)** que:
  1. Gera o roteiro de Reels vertical `9:16` dividido em **Hook (0–3s)**, **Corpo (3–12s)** e **CTA (12–15s)**;
  2. Gera os prompts visuais `9:16` (`1080x1920`, `24fps`) prontos para o gerador de vídeo **Google Veo 2** (no AI Studio ou Vertex AI);
  3. Gera as legendas `.srt` sincronizadas e a linha de comando `FFmpeg` pronta para renderizar o arquivo final `reel_final_1080x1920.mp4`.
