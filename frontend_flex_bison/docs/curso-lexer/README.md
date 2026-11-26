# Oficina do Lexer Pifari

Curso local em React para estudar `frontend/lexer.l` com explicações curtas, autômato interativo e exercícios de escrita.

## Executar

Abra um terminal na pasta `docs/curso-lexer` e rode:

```powershell
npm install
npm run dev
```

Abra o endereço local mostrado pelo Vite. O servidor está vinculado a `127.0.0.1`. Para gerar uma versão estática, use `npm run build`; os arquivos saem em `dist/`.

## Executar com Docker

Na pasta `docs/curso-lexer`, rode:

```powershell
docker compose up --build -d
```

Depois abra <http://127.0.0.1:4173>. O Compose compila o React em uma imagem Node e serve os arquivos estáticos com Nginx. Para acompanhar o estado: `docker compose ps`; para parar: `docker compose down`.

O build usa a raiz do repositório como contexto para importar `frontend/lexer.l`; ele não compila nem modifica o interpretador C.

O curso importa o lexer original com `?raw`, então o painel linha por linha acompanha o fonte real. O material didático não compila regras Flex digitadas na página; para isso, use o Makefile e o Flex do ambiente do projeto.
