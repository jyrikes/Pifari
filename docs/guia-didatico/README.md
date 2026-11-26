# Guia didático do Pifari

## Abrir o site

No Windows, execute no PowerShell, a partir da raiz do repositório:

```powershell
powershell -ExecutionPolicy Bypass -File .\docs\guia-didatico\abrir_guia.ps1
```

O script gera/atualiza `catalogo.json`, inicia um servidor HTTP local e abre o Edge. A porta escolhida é 18765 para não conflitar com outros serviços locais. Não é necessário pagar por uma API nem conectar o site à internet.

## Como o site foi implementado

- `index.html`: layout, estilo responsivo, aulas teóricas, glossário, quiz e interface de leitura do código.
- `gerar_catalogo.py`: percorre o repositório e constrói um catálogo dos arquivos-fonte, exemplos, testes e documentos, incluindo um resumo pedagógico por arquivo.
- `catalogo.json`: saída gerada pelo script; contém as fontes textuais exibidas no navegador.
- `abrir_guia.ps1`: atualiza o catálogo e inicia o site local.

A interface é HTML/CSS/JavaScript simples, sem framework ou dependência de API paga. O navegador carrega o catálogo local por HTTP; isso evita a restrição de segurança que impede páginas abertas como `file://` de ler outros arquivos locais.

## Segurança e escopo

O gerador exclui `.env`, diretórios `.git`, `build/`, caches e binários. Arquivos acima do limite de tamanho configurado também não entram no catálogo. O guia oferece explicações conceituais e descrições por arquivo; ele não substitui ler os testes nem verificar a implementação quando uma afirmação depende da semântica exata.
