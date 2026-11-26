const lessons = window.LESSONS;
let done = new Set();
try {
  done = new Set(JSON.parse(localStorage.getItem("pifari-progress") || "[]"));
} catch {}
const esc = (s) =>
  String(s).replace(
    /[&<>"']/g,
    (c) =>
      ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" })[
        c
      ],
  );
const main = document.querySelector("#main");
function navigation() {
  const query = document
    .querySelector("#search")
    .value.toLocaleLowerCase("pt-BR");
  document.querySelector("#count").textContent =
    done.size + " / " + lessons.length;
  document.querySelector("#progress").value = done.size;
  let html = "";
  for (const level of ["Básico", "Intermediário", "Avançado"]) {
    const found = lessons.filter(
      (l) =>
        l.level === level &&
        (l.title + " " + l.intro + " " + l.body)
          .toLocaleLowerCase("pt-BR")
          .includes(query),
    );
    if (found.length)
      html +=
        "<h3>" +
        level +
        "</h3>" +
        found
          .map(
            (l) =>
              `<a href="#aula-${l.id}" ${location.hash === "#aula-" + l.id ? 'class="active" aria-current="page"' : ""}><span class="num">${done.has(l.id) ? "✓" : String(l.id + 1).padStart(2, "0")}</span>${esc(l.title)}</a>`,
          )
          .join("");
  }
  document.querySelector("#nav").innerHTML =
    html || "<p>Nenhuma aula encontrada.</p>";
}
function render() {
  navigation();
  const match = location.hash.match(/^#aula-(\d+)$/);
  const lesson = match && lessons[Number(match[1])];
  if (!lesson) {
    main.innerHTML = `<section class="hero"><div class="eyebrow">Uma linguagem. Muitas possibilidades.</div><h1>Do primeiro <em>print</em><br>ao que acontece<br>por trás do código.</h1><p class="lead">Aprenda Pifari com calma, exemplos reais e um caminho claro. Dos fundamentos aos fluxos, tarefas e otimizações do runtime.</p><div class="actions"><a class="btn" href="#aula-0">Começar a aprender ↗</a><a class="btn secondary" href="#aula-${lessons.find((l) => !done.has(l.id))?.id || 0}">Continuar meu percurso</a></div><div class="stats"><div><b>18</b><span>AULAS GUIADAS</span></div><div><b>03</b><span>TRILHAS DE ESTUDO</span></div><div><b>.pif</b><span>EXEMPLOS REAIS</span></div></div></section><h2>Seu caminho pela linguagem</h2><div class="tracks">${[
      [
        "01",
        "Básico",
        "Pense em valores, decisões, listas e funções.",
        0,
        "5 aulas",
      ],
      [
        "02",
        "Intermediário",
        "Transforme dados com matrizes, fluxos e arrays.",
        5,
        "6 aulas",
      ],
      [
        "03",
        "Avançado",
        "Explore tarefas, MCP e os caminhos do runtime.",
        11,
        "7 aulas",
      ],
    ]
      .map(
        (t) =>
          `<a class="track" href="#aula-${t[3]}"><span class="tag">TRILHA ${t[0]}</span><b>${t[1]} ↗</b><p>${t[2]}</p><small>${t[4]} · exemplos + exercícios</small></a>`,
      )
      .join(
        "",
      )}</div><div class="notice"><strong>Uma fotografia do Pifari atual.</strong> O curso usa os fontes e exemplos locais revisados em novembro de 2026. Recursos experimentais e integrações têm seus limites explicados nas aulas. Você lê e baixa exemplos aqui; a execução acontece no seu ambiente local.</div>`;
    return;
  }
  const l = lesson;
  main.innerHTML = `<div class="eyebrow">${l.level} / AULA ${String(l.id + 1).padStart(2, "0")} DE 18</div><h1>${esc(l.title)}</h1><p class="lead">${esc(l.intro)}</p><h2>Entenda a ideia</h2><p>${esc(l.body)}</p>${l.id === 0 ? '<div class="notice"><strong>Preparar o ambiente (Ubuntu / WSL)</strong><pre>sudo apt install build-essential flex bison\nmake\n./pifari aula.pif</pre>Rode make na pasta frontend_flex_bison do seu projeto.</div>' : ""}<div class="codebox"><div class="codehead"><span>${l.file ? "examples/" + esc(l.file) : "Referência de estudo"}</span><div><button id="copy">Copiar</button> ${l.file ? '<button id="download">Baixar .pif ↓</button>' : ""}</div></div><pre><code>${esc(l.code)}</code></pre></div><p style="font-size:12px;color:#718077">${l.file ? "Código do projeto atual. Execute com ./pifari examples/" + esc(l.file) + ". O site não executa esse código no navegador." : "Referência para estudo; os caminhos são relativos à pasta frontend_flex_bison."}</p><section class="exercise"><span class="eyebrow">Sua vez</span><h2>Experimente antes de continuar</h2><p>${esc(l.exercise)}</p><details><summary>Conferir a explicação</summary><p>${esc(l.answer)}</p></details></section><div class="actions"><button class="btn" id="complete">${done.has(l.id) ? "✓ Aula concluída · desfazer" : "Marcar aula como concluída"}</button><span id="feedback" role="status"></span></div><div class="bottomnav"><a href="${l.id ? "#aula-" + (l.id - 1) : "#inicio"}">← ${l.id ? "Aula anterior" : "Início"}</a><a href="${l.id < 17 ? "#aula-" + (l.id + 1) : "#inicio"}">${l.id < 17 ? "Próxima aula" : "Voltar ao percurso"} →</a></div>`;
  document.querySelector("#complete").onclick = () => {
    done.has(l.id) ? done.delete(l.id) : done.add(l.id);
    try {
      localStorage.setItem("pifari-progress", JSON.stringify([...done]));
    } catch {}
    render();
  };
  document.querySelector("#copy").onclick = async (e) => {
    try {
      await navigator.clipboard.writeText(l.code);
      e.target.textContent = "Copiado ✓";
    } catch {
      document.querySelector("#feedback").textContent =
        "Selecione o código e copie com Ctrl+C.";
    }
  };
  if (l.file)
    document.querySelector("#download").onclick = () => {
      const url = URL.createObjectURL(
        new Blob([l.code], { type: "text/plain;charset=utf-8" }),
      );
      const a = document.createElement("a");
      a.href = url;
      a.download = l.file;
      a.click();
      setTimeout(() => URL.revokeObjectURL(url), 1000);
    };
}
document.querySelector("#search").addEventListener("input", navigation);
document.querySelector("#menu").onclick = (e) => {
  const open = document.body.classList.toggle("menu-open");
  e.currentTarget.setAttribute("aria-expanded", String(open));
};
window.addEventListener("hashchange", () => {
  document.body.classList.remove("menu-open");
  document.querySelector("#menu").setAttribute("aria-expanded", "false");
  render();
  window.scrollTo(0, 0);
});
render();
