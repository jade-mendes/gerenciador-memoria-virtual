// simulador.js

document.addEventListener("DOMContentLoaded", () => {
    document.getElementById("next-cycle").addEventListener("click", async () => {
        try {
            const response = await fetch("/next-cycle", {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({null: null})
            });
            const data = await response.json();
            atualizarPagina(data);
        } catch (error) {
            console.error("Erro ao buscar /next-cycle:", error);
        }
    });
});

function atualizarPagina(data) {
    atualizarFilaDeProcessos(data.process_queue);
    atualizarProcessoAtual(data.current_process);
    atualizarCicloAtual(data.cycle);
    atualizarTLB(data.tlb);
    atualizarListaDeProcessos(data.process_list);
    atualizarMemoria(data.memory_usage);
}

function atualizarFilaDeProcessos(fila) {
    const container = document.querySelector(".process-queue");
    container.innerHTML = "";
    fila.forEach(proc => {
        const div = document.createElement("div");
        div.className = "process-mini";
        div.innerHTML = `
            <img src="${proc.icon}" alt="${proc.name}" class="icon">
            <div class="process-name">${proc.name}</div>
        `;
        container.appendChild(div);
    });
}

function atualizarProcessoAtual(proc) {
    const container = document.querySelector(".last-msg");
    container.innerHTML = `
        <div class="process-mini">
            <img src="${proc.icon}" alt="${proc.name}" class="icon">
            <div class="process-name">${proc.name}</div>
        </div>
        <div class="message">
            <span class="text">${proc.last_msg}</span>
        </div>
    `;
}

function atualizarCicloAtual(ciclo) {
    document.getElementById("current-cycle").textContent = `Ciclo atual: ${ciclo}`;
    document.getElementById("remaining-cycles").textContent = `${Math.max(0, 100 - ciclo)}`;
}

function atualizarTLB(tlb) {
    const tbl = document.querySelector(".memory-panel .tbl table thead");
    tbl.innerHTML = `
        <tr><th>Virtual</th><th>Real</th><th>Dirty</th><th>Referenced</th><th>Modified</th></tr>
    `;
    tlb.forEach(entry => {
        const row = document.createElement("tr");
        row.innerHTML = `
            <th>${entry.virtual}</th><th>${entry.real}</th><th>${entry.dirty}</th><th>${entry.referenced}</th><th>${entry.modified}</th>
        `;
        tbl.appendChild(row);
    });
}

function atualizarListaDeProcessos(lista) {
    const container = document.querySelector(".process-panel-list");
    container.innerHTML = "";
    lista.forEach(proc => {
        const panel = document.createElement("div");
        panel.className = "process-panel";
        panel.innerHTML = `
            <h3>${proc.name}: (${proc.pid})</h3>
            <div class="tables">
                <div class="table-box">
                    <table>
                        <thead><tr><th>Virtual</th><th>Real</th></tr></thead>
                        <tbody>
                            ${proc.page_table.map(p => `<tr><td>${p.virtual}</td><td>${p.real}</td></tr>`).join("")}
                        </tbody>
                    </table>
                </div>
                <div class="info-box">
                    <div class="process-info">
                        <p><strong>PID:</strong> ${proc.pid}</p>
                        <p><strong>Estado:</strong> ${proc.state}</p>
                    </div>
                </div>
            </div>
        `;
        container.appendChild(panel);
    });
}

function atualizarMemoria(uso) {
    const bar = document.querySelector(".usage-bar .fill");
    const label = document.querySelector(".usage-bar .usage-label");
    bar.style.height = `${uso}%`;
    label.textContent = `${uso}%`;
}
