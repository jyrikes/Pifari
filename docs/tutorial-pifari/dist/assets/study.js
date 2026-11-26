const deadline = new Date('2026-11-08T12:00:00-03:00').getTime();
function updateClock() { const mins=Math.ceil((deadline-Date.now())/60000); document.querySelector('#remaining').textContent=mins>0?`${mins} minutos restantes até 12h`:'O prazo de hoje encerrou. Continue no seu ritmo.'; }
updateClock();setInterval(updateClock,30000);
document.querySelectorAll('[data-step]').forEach(box=>{const key='pifari-sprint-'+box.dataset.step;try{box.checked=localStorage.getItem(key)==='1'}catch{}box.addEventListener('change',()=>{try{localStorage.setItem(key,box.checked?'1':'0')}catch{}})});
