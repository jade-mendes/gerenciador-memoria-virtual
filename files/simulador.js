// simulador.js
document.addEventListener("DOMContentLoaded", () => {
    document.getElementById("next-cycle").addEventListener("click", async () => {
        try {
            const response = await fetch("/next-cycle", {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({null: null})
            });
            const data = await response.json(); //important_cycle
            atualizarPagina(data);
        } catch (error) {
            console.error("Erro ao buscar /next-cycle:", error);
        }
    });
    // botão direito
    document.getElementById("next-cycle").addEventListener("contextmenu", async (e) => {
        e.preventDefault(); // Previne o menu de contexto padrão
        const audio_normal = new Audio('/next-cycle.wav');
        const audio_important = new Audio('/next-cycle-important.mp3');
        try {
            let importantCycle = false;
            while (!importantCycle) {
                const response = await fetch("/next-cycle", {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({null: null})
                });
                const data = await response.json();
                atualizarPagina(data);
                importantCycle = data.important_cycle; // Verifica se é um ciclo importante
                // await new Promise(resolve => setTimeout(resolve, 1000)); // Espera 1 segundo antes de continuar
                if (importantCycle) {
                    await audio_important.play();
                }
                else {
                    await audio_normal.play();
                }
            }
        } catch (error) {
            console.error("Erro ao buscar /next-cycle:", error);
        }
    });

    document.querySelector(".input-block button").addEventListener("click", enviarInputUsuario);

    // Adicionar suporte para Enter no input
    document.querySelector(".input-block input").addEventListener("keypress", (e) => {
        if (e.key === 'Enter') enviarInputUsuario();
    });

    document.getElementById("restart").addEventListener("click", () => {
        window.location.href = "/";
    });
});

function atualizarPagina(data) {
    atualizarFilaDeProcessos(data.process_queue);
    atualizarProcessoAtual(data.current_process, data.last_msg, data.last_error);
    atualizarCicloAtual(data.cycle);
    atualizarTLB(data.tlb);
    atualizarListaDeProcessos(data.process_list);
    atualizarMemoria(data['main-memory_usage'], data['secondary-memory_usage']);

    // Atualizar remaining-cycles se houver processo atual
    if (data.current_process && data.current_process["remaining-cycles"] !== undefined) {
        document.getElementById("remaining-cycles").textContent =
            data.current_process["remaining-cycles"];
    }
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

function atualizarProcessoAtual(proc, mensagem, erro) {
    const container = document.querySelector(".last-msg");
    let content = '';

    // Exibir processo atual se existir
    if (proc) {
        content += `
            <div class="process-mini">
                <img src="${proc.icon}" alt="${proc.name}" class="icon">
                <div class="process-name">${proc.name}</div>
            </div>
        `;
    }

    // Exibir mensagem principal
    if (mensagem) {
        content += `<div class="message"><span class="text">${mensagem}</span></div>`;
    }

    // Exibir mensagem de erro se existir
    if (erro) {
        content += `<div class="message error"><span class="text">${erro}</span></div>`;
    }

    document.getElementById("remaining-cycles").textContent = `${proc ? proc["remaining-cycles"] : "N/A"}`;

    container.innerHTML = content;
}

function atualizarCicloAtual(ciclo) {
    document.getElementById("current-cycle").textContent = `Ciclo atual: ${ciclo}`;
}

function atualizarTLB(tlb) {
    const tbody = document.getElementById("tlb-table");
    tbody.innerHTML = "";

    tlb.forEach(entry => {
        const row = document.createElement("tr");

        // Destacar linhas válidas
        if (entry.valid) {
            row.classList.add("valid-entry");
        }

        // Converter valores booleanos para texto
        const validText = entry.valid ? "Sim" : "Não";
        const referencedText = entry.referenced ? "Sim" : "Não";

        row.innerHTML = `
            <td>${entry.index}</td>
            <td>${validText}</td>
            <td>${entry.virtual}</td>
            <td>${entry.real}</td>
            <td>${entry.last_used}</td>
            <td>${referencedText}</td>
        `;
        tbody.appendChild(row);
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
                        <h4>Tabela de Páginas</h4>
                        <thead>
                            <tr>
                                <th>Virtual</th>
                                <th>Real</th>
                                <th>Dirty</th>
                                <th>Referenced</th>
                            </tr>
                        </thead>
                        <tbody>
                            ${proc.page_table.map(p => `
                                <tr>
                                    <td>${p.virtual}</td>
                                    <td onclick="SetAddressFromPage(${p.real})">${p.real}</td>
                                    <td>${p.dirty}</td>
                                    <td>${p.referenced}</td>
                                </tr>
                            `).join("")}
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

function atualizarMemoria(mainUsage, secondaryUsage) {
    const mainPercent = Math.round(mainUsage * 100);
    const secondaryPercent = Math.round(secondaryUsage * 100);

    const barMain = document.querySelector("#memory-fill-main");
    const labelMain = document.querySelector("#memory-usage-label-main");
    const barSecondary = document.querySelector("#memory-fill-secondary");
    const labelSecondary = document.querySelector("#memory-usage-label-secondary");

    barMain.style.height = `${mainPercent}%`;
    labelMain.textContent = `${mainPercent}%`;

    barSecondary.style.height = `${secondaryPercent}%`;
    labelSecondary.textContent = `${secondaryPercent}%`;
}

// Nova função para enviar input do usuário
async function enviarInputUsuario() {
    const input = document.querySelector(".input-block input");
    const texto = input.value.trim();

    if (!texto) return;

    try {
        const response = await fetch("/set-user-input", {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({input: texto})
        });

        if (response.ok) {
            input.value = ""; // Limpa o campo após envio bem-sucedido
        } else {
            console.error("Erro ao enviar input:", await response.text());
        }
    } catch (error) {
        console.error("Erro ao enviar input:", error);
    }
}


async function GetMemory() {
    const address = document.getElementById("address-input").value.trim();
    if (!address) return;
    const size = parseInt(document.getElementById("size-input").value.trim());
    if (isNaN(size) || size <= 0) {
        alert("Por favor, insira um tamanho válido.");
        return;
    }

    const conteudo = document.getElementById("memory-content-main");

    for (let i = 0; i < size; i++) {
        try {
            const response = await fetch("/get-data-from-address", {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({address: parseInt(address) + i})
            });
            const data = await response.json();
            if (data.error) {
                alert(`Erro ao acessar memória: ${data.error}`);
                return;
            }
            console.log(`Endereço ${parseInt(address) + i}: ${data.value}`);
            conteudo.innerHTML += String.fromCharCode(data.value);
        } catch (error) {
            console.error("Erro ao buscar /get-memory:", error);
        }
    }
}

function SetAddressFromPage(address) {
    document.getElementById("address-input").value = address;
}

function LimparMemoria() {
    document.getElementById("memory-content-main").innerHTML = "";
    document.getElementById("address-input").value = "";
}