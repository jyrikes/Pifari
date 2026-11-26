FROM node:22-alpine AS build

WORKDIR /workspace
COPY docs/curso-lexer/package.json docs/curso-lexer/package-lock.json* ./docs/curso-lexer/
COPY frontend/lexer.l ./frontend/lexer.l

WORKDIR /workspace/docs/curso-lexer
RUN npm install --no-audit --no-fund

COPY docs/curso-lexer/ ./
RUN npm run build

FROM nginx:1.27-alpine
COPY docs/curso-lexer/nginx.conf /etc/nginx/conf.d/default.conf
COPY --from=build /workspace/docs/curso-lexer/dist/ /usr/share/nginx/html/
EXPOSE 8080
