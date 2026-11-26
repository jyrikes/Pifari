#!/usr/bin/env python3
"""Gera catálogo do guia sem incluir segredos, binários ou artefatos compilados."""
from pathlib import Path
import json
from datetime import datetime, timezone
ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent / "catalogo.json"
SKIP = {".git", "build", "__pycache__", "node_modules", ".venv", "venv"}
EXTS = {".c", ".h", ".l", ".y", ".pif", ".py", ".sh", ".md", ".txt"}
SPECIAL = {"Makefile", ".gitignore", ".clang-format", ".editorconfig", ".env.example"}
LIMIT = 180_000
BACKEND = {
"runtime":("Runtime","Motor que representa valores, ambientes, funções e executa construções da linguagem.","É a ponte entre estrutura do programa e comportamento observável.","Interpretador, valores, bindings, escopo, VM, bytecode"),
"ast_ir":("AST e IR","Representa operações em forma explícita para avaliação e análise.","A representação intermediária desacopla otimização da sintaxe superficial.","AST, IR, grafo, avaliação"),
"cache":("Cache","Reaproveita resultados de operações caras quando chave e validade permitem.","Evita repetir trabalho, exigindo cuidado com validade.","Cache, chave, invalidação"),
"devices":("Descoberta de dispositivos","Abstrai identificação de recursos de computação disponíveis.","Separar descoberta da política permite trocar estratégias.","CPU, GPU, abstração"),
"dirac":("Módulo de Dirac","Módulo especializado usado em operações e exemplos relacionados a estados quânticos.","Separação permite estudar e testar essa capacidade isoladamente.","Estado, representação numérica"),
"effects":("Análise de efeitos","Classifica ações de funções, como cálculo puro, leitura, escrita e I/O.","Saber os efeitos ajuda a decidir quais transformações são seguras.","Pureza, efeitos colaterais, JIT"),
"gpu_client":("Cliente GPU","Conecta o runtime ao caminho de computação em GPU.","Isola transporte e seleção do dispositivo da semântica da linguagem.","Cliente, dispositivo, custo"),
"mcp_gateway":("Gateway MCP","Centraliza descoberta e chamada de ferramentas e recursos externos.","Isola protocolo, política e validação do núcleo da linguagem.","MCP, JSON-RPC, ferramentas"),
"mcp_server":("Servidor MCP","Expõe funcionalidades por uma interface de servidor e processa mensagens de protocolo.","A fronteira externa precisa validar mensagens e representar erros.","Servidor, protocolo, I/O"),
"memory_plan":("Planejamento de memória","Analisa a vida útil de temporários para procurar slots reutilizáveis.","Reduz alocações sem sobrescrever valores ainda vivos.","Tempo de vida, slots"),
"native_kernel":("Kernels nativos e JIT","Coordena compilação ou reutilização de operações numéricas nativas compatíveis.","O custo inicial pode compensar em execuções repetidas.","JIT, código nativo, subset, cache"),
"numeric_buffer":("Buffers numéricos","Representa dados numéricos em memória contígua.","Dados contíguos favorecem acesso previsível e integração C.","F64, contiguidade, conversão"),
"operator_engine":("Motor de operadores","Implementa kernels numéricos combinados em uma passagem.","A fusão diminui arrays temporários e tráfego de memória.","Kernel, fusão, workers"),
"optimizer":("Otimizador","Analisa operações e tenta simplificações que preservam o resultado esperado.","Velocidade só é melhoria se a semântica for preservada.","IR, constantes, equivalência"),
"plan":("Planejamento de execução","Representa decisões de execução separadas de sua realização.","Facilita escolher estratégia e observar o plano adotado.","Plano, estratégia"),
"reduce":("Reduções","Combina muitos valores em um resultado por uma operação de redução.","Uma redução paralela define como particionar e combinar resultados.","Redução, partição, paralelismo"),
"reversible":("Transformações reversíveis","Contém operações reversíveis de demonstração, como aplicar uma máscara XOR duas vezes.","Invariantes reversíveis ajudam a testar propriedades matemáticas.","XOR, reversibilidade"),
"types":("Representação de tipos","Implementa aspectos concretos da representação e compatibilidade dos tipos.","Centralizar tipos evita duplicar regras em cada operador.","Tipo, conversão, compatibilidade"),
"typecheck":("Verificação de tipos","Verifica incompatibilidades antes da execução quando a checagem é solicitada.","Detectar erros cedo melhora o diagnóstico.","Análise estática, tipos"),
"agent_runtime":("Runtime multiagente","Coordena tarefas, agentes, dependências, estados e execução de grafos.","Um DAG explicita o que pode executar e o que precisa esperar.","Agente, DAG, dependência, cancelamento")
}
NOTES = {
"frontend/ast.h":("Modelo da AST","Define os tipos de nós e os dados que representam o programa após a análise sintática.","O runtime precisa de uma representação explícita, não de texto bruto.","Structs, enums, nós, memória"),
"frontend/lexer.l":("Analisador léxico Flex","Define padrões que reconhecem nomes, números, palavras reservadas, operadores e pontuação e os transformam em tokens.","O parser trabalha com categorias de símbolos em vez de inspecionar cada caractere.","Expressões regulares, lexema, token, scanner"),
"frontend/parser.y":("Gramática Bison","Define como tokens formam expressões e comandos, precedência, associatividade e ações que constroem a AST.","A gramática decide quais formas são válidas e como agrupar expressões.","Produções, terminais, precedência, shift/reduce"),
"backend/runtime.h":("Contrato do runtime","Declara tipos, estruturas e funções compartilhadas pelo runtime e outros módulos.","Contratos evitam que cada arquivo invente interfaces diferentes.","Header, protótipo, tipo"),
"backend/runtime.c":("Implementação do runtime","Implementa boa parte da avaliação: valores, ambientes, variáveis, funções e integração com formas de execução.","Siga uma função de entrada até a operação que avalia nós; compare argumentos e retornos.","Interpretador, bindings, escopo, frames, bytecode"),
"backend/optimization/optimizer.c":("Implementação do otimizador","Transforma operações com regras que procuram preservar o resultado observável.","A compilação pode antecipar cálculos constantes e reduzir trabalho repetido.","IR, dobra de constantes, equivalência"),
"backend/optimization/optimizer.h":("Interface do otimizador","Expõe funções e tipos do otimizador para o sistema e para testes.","Permite testar o módulo sem depender de seus detalhes internos.","API C, declaração, contrato"),
"backend/optimization/native_kernel.c":("JIT e kernels nativos","Cuida de caminhos em que um subconjunto numérico compatível vira código nativo ou é reaproveitado do cache.","O JIT pode reduzir custo em laços repetidos, mas a compilação inicial também custa.","JIT, subset, cache, código nativo"),
"backend/optimization/operator_engine.c":("Operações numéricas fundidas","Implementa operações combinadas em buffers numéricos, evitando arrays intermediários.","Reduzir leituras, escritas e alocações pode ser tão importante quanto reduzir aritmética.","Fusão, ReLU, vetor, threads"),
"backend/optimization/memory_plan.c":("Plano de memória","Procura reutilizar posições temporárias cujos intervalos de vida não se sobrepõem.","Slots reutilizados economizam alocação sem sobrescrever valores ainda necessários.","Vida útil, slots"),
"backend/integrations/mcp_gateway.c":("Gateway de ferramentas MCP","Abstrai descoberta e invocação de ferramentas e recursos externos.","Um gateway desacopla o núcleo da linguagem do protocolo externo.","MCP, JSON-RPC, política"),
"backend/integrations/agent_runtime.c":("Orquestração multiagente","Gerencia tarefas, dependências, estados e execução.","Um DAG explicita dependências e tarefas liberadas para execução.","Agentes, tarefas, DAG"),
"Makefile":("Receita de compilação","Descreve dependências para gerar parser/lexer, compilar objetos, ligar executável e rodar testes.","Automatiza builds reprodutíveis e separa fontes mantidas à mão de artefatos gerados.","Alvos, dependências, GCC, Flex, Bison"),
"README.md":("Mapa do projeto","Resume como compilar, executar, testar e quais recursos e limitações são declarados.","Leia primeiro o mapa, mas confirme afirmações nos fontes e testes atuais.","Arquitetura, build, exemplos"),
"tests/equiv.sh":("Teste de equivalência","Compara saídas de programas em configurações de otimização diferentes.","Otimizar só é correto se o resultado se mantiver.","Regressão, equivalência"),
"tests/golden.sh":("Testes golden","Compara resultados com saídas de referência mantidas no projeto.","Detecta mudanças acidentais de comportamento.","Saída esperada, regressão"),
"tests/fuzz_parser.py":("Fuzzing do parser","Gera entradas variadas para exercitar caminhos de erro e procurar falhas.","Casos de borda complementam exemplos escritos à mão.","Robustez, entradas inesperadas"),
"tools/pifari_worker.py":("Worker auxiliar","Utilitário Python de apoio a processos e tarefas ao redor do núcleo C.","Adaptadores simplificam integrações sem carregar toda a comunicação para o runtime.","Processo, protocolo, adaptador")
}
EXAMPLE = {
"basico.pif":("Primeiro programa","Reconheça atribuição, valores e saída antes de estudar funções ou concorrência."),
"funcoes.pif":("Funções","Mostra parâmetros e chamadas; conecte a sintaxe aos frames e ambientes do runtime."),
"fluxo_sequencial.pif":("Fluxo sequencial","Exemplo de percurso item a item com o operador de fluxo sequencial."),
"fluxo_paralelo.pif":("Fluxo paralelo","Mostra distribuição de itens e isolamento da variável por worker."),
"fluxo_coleta.pif":("Coleta de resultados","Mostra como os resultados de um fluxo são reunidos numa variável."),
"fluxo_sincronizacao.pif":("Barreira de sincronização","Mostra que o trabalho à esquerda termina antes da expressão à direita."),
"typed_arrays.pif":("Arrays tipados","Demonstra armazenamento e operações sobre dados numéricos tipados."),
"mlp_xor.pif":("Rede neural XOR","Exemplo de MLP com operações numéricas e laços, sem framework externo."),
"reducao_nativa.pif":("Redução nativa","Demonstra agregação de valores e a fronteira entre linguagem e caminho nativo.")
}
def explain(rel):
    if rel in NOTES: return NOTES[rel]
    p=Path(rel); name=p.name; stem=p.stem
    if p.parts[0]=="backend":
        if stem in BACKEND:
            title,summary,why,concepts=BACKEND[stem]
            return title+(" — interface" if p.suffix==".h" else " — implementação"),("A interface pública descreve o contrato. "+summary if p.suffix==".h" else summary),( "Compare com o .c correspondente e veja quem usa a API." if p.suffix==".h" else why),concepts
        return "Módulo do backend: "+name,"Parte do motor de execução escrito em C. Compare com o header de mesmo nome, chamadores e testes.","A separação reduz acoplamento e ajuda a testar responsabilidades menores.","C, função, estado, runtime"
    if p.parts[0]=="examples":
        if name in EXAMPLE:
            title,summary=EXAMPLE[name]; return title,summary,"Execute mentalmente cada linha e relacione a construção com o parser e o runtime.","Sintaxe Pifari, execução, exemplo"
        if "benchmarks" in p.parts: return "Benchmark: "+name,"Programa de medição de desempenho e validação de resultados.","Uma comparação justa faz o mesmo trabalho e declara inicialização/aquecimento.","Mediana, checksum, aquecimento"
        return "Exemplo executável: "+name,"Programa de demonstração ou regressão; o fonte real aparece nesta tela.","Exemplos conectam a teoria ao comportamento observado.","Sintaxe, AST, runtime"
    if p.parts[0]=="tests": return "Teste: "+name,"Verifica um comportamento. Identifique entradas, ação testada e condição de sucesso.","Testes tornam regressões detectáveis e documentam o comportamento esperado.","Asserção, unidade, integração, regressão"
    if p.parts[0]=="docs": return "Documentação: "+name,"Documento de arquitetura, semântica, execução ou medições.","Especificação é intenção documentada, não prova automática de implementação completa.","Arquitetura, semântica, limites"
    if p.parts[0]=="frontend": return "Frontend: "+name,"Participa da transformação do texto em estruturas que a execução consegue processar.","Separar lexer, parser e AST simplifica diagnóstico.","Lexer, parser, AST"
    if name=="Makefile": return NOTES["Makefile"]
    if name=="README.md": return NOTES["README.md"]
    if name.endswith(".h"): return "Cabeçalho C: "+name,"Declara tipos e protótipos compartilhados entre módulos.","O header define o contrato; o .c tende a conter a realização.","Protótipos, structs, enums"
    if name.endswith(".c"): return "Implementação C: "+name,"Contém lógica em C para uma parte do runtime.","Identifique entradas, saídas e invariantes e procure testes relacionados.","Funções, memória, estado"
    if name==".env.example": return "Modelo de configuração","Lista configurações esperadas sem carregar o .env pessoal.","Documenta configuração sem expor credenciais.","Configuração, variáveis de ambiente"
    return "Arquivo auxiliar: "+name,"Arquivo de apoio ao desenvolvimento, configuração ou documentação.","Ajuda a entender build e manutenção do projeto.","Ferramentas, configuração, manutenção"
def main():
    entries=[]
    for p in sorted(ROOT.rglob("*")):
        if not p.is_file(): continue
        rel=p.relative_to(ROOT).as_posix()
        if any(part in SKIP for part in p.relative_to(ROOT).parts): continue
        if rel==".env" or p.suffix.lower() in {".pyc",".exe",".dll",".so",".o",".a",".bin",".lock"}: continue
        if p.name not in SPECIAL and p.suffix.lower() not in EXTS: continue
        try:
            size=p.stat().st_size
            if size>LIMIT: continue
            content=p.read_text(encoding="utf-8",errors="replace")
        except OSError: continue
        title,summary,why,concepts=explain(rel)
        entries.append({"path":rel,"name":p.name,"group":p.parts[0] if len(p.parts)>1 else "Raiz","kind":p.suffix.lower().lstrip(".") or ("make" if p.name=="Makefile" else "config"),"bytes":size,"lines":len(content.splitlines()),"title":title,"summary":summary,"why":why,"concepts":concepts,"content":content})
    OUT.write_text(json.dumps({"generatedAt":datetime.now(timezone.utc).isoformat(),"count":len(entries),"files":entries},ensure_ascii=False),encoding="utf-8")
    print("CATALOGO_OK arquivos={} bytes={}".format(len(entries),OUT.stat().st_size))
if __name__=="__main__": main()
