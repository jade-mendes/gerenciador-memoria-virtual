// simulador.js
document.addEventListener("DOMContentLoaded", () => {
    // Alerta para o usuário não usar alert() no código.
    // Usaremos uma caixa de mensagem customizada no futuro.
    const originalAlert = window.alert;
    window.alert = function(message) {
        console.warn("Utilize um método de notificação customizado ao invés de window.alert(). Mensagem:", message);
        originalAlert(message);
    };

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

    document.getElementById("send-io").addEventListener("click", enviarInputUsuario);

    // Adicionar suporte para Enter no input
    document.getElementById("io-input").addEventListener("keypress", (e) => {
        if (e.key === 'Enter') enviarInputUsuario();
    });

    document.getElementById("restart").addEventListener("click", () => {
        window.location.href = "/";
    });
});

function atualizarPagina(data) {
    if (!data) return;
    if (data.process_queue) atualizarFilaDeProcessos(data.process_queue);
    if (data.current_process) atualizarProcessoAtual(data.current_process, data.last_msg, data.last_error);
    if (data.cycle !== undefined) atualizarCicloAtual(data.cycle);
    if (data.tlb) atualizarTLB(data.tlb);
    if (data.process_list) atualizarListaDeProcessos(data.process_list);
    if (data['main-memory_usage'] !== undefined && data['secondary-memory_usage'] !== undefined) {
        atualizarMemoria(data['main-memory_usage'], data['secondary-memory_usage']);
    }

    if (data.current_process && data.current_process["remaining-cycles"] !== undefined) {
        document.getElementById("remaining-cycles").textContent =
            data.current_process["remaining-cycles"];
    } else {
        document.getElementById("remaining-cycles").textContent = "N/A";
    }
}

function atualizarFilaDeProcessos(fila) {
    const container = document.querySelector(".process-queue");
    container.innerHTML = "";
    fila.forEach(proc => {
        const div = document.createElement("div");
        div.className = "process-mini";
        div.innerHTML = `
            <img src="${proc.icon || ''}" alt="${proc.name}" class="icon">
            <div class="process-name">${proc.name}</div>
        `;
        container.appendChild(div);
    });
}

function atualizarProcessoAtual(proc, mensagem, erro) {
    const container = document.querySelector(".last-msg");
    let content = '';

    if (proc) {
        content += `
            <div class="process-mini">
                <img src="${proc.icon || ''}" alt="${proc.name}" class="icon">
                <div class="process-name">${proc.name}</div>
            </div>
        `;
    }

    if (mensagem) {
        content += `<div class="message"><span class="text">${mensagem}</span></div>`;
    }

    if (erro) {
        content += `<div class="message error"><span class="text">${erro}</span></div>`;
    }

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

        if (entry.valid) {
            row.classList.add("valid-entry");
        }

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
                    <h4>Tabela de Páginas</h4>
                    <table>
                        <thead>
                            <tr>
                                <th>Virtual</th>
                                <th>Real</th>
                                <th>Dirty</th>
                                <th>Referenced</th>
                            </tr>
                        </thead>
                        <tbody>
                            ${(proc.page_table || []).map(p => `
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

    if(barMain) barMain.style.height = `${mainPercent}%`;
    if(labelMain) labelMain.textContent = `${mainPercent}%`;

    if(barSecondary) barSecondary.style.height = `${secondaryPercent}%`;
    if(labelSecondary) labelSecondary.textContent = `${secondaryPercent}%`;
}

async function enviarInputUsuario() {
    const input = document.getElementById("io-input");
    const texto = input.value.trim();

    if (!texto) return;

    try {
        const response = await fetch("/set-user-input", {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({input: texto})
        });

        if (response.ok) {
            input.value = "";
        } else {
            console.error("Erro ao enviar input:", await response.text());
        }
    } catch (error) {
        console.error("Erro ao enviar input:", error);
    }
}


async function GetMemory() {
    const addressInput = document.getElementById("address-input");
    const sizeInput = document.getElementById("size-input");
    const contentDiv = document.getElementById("memory-content-main");

    const address = addressInput.value.trim();
    if (!address) return;

    const size = parseInt(sizeInput.value.trim(), 10);
    if (isNaN(size) || size <= 0) {
        alert("Por favor, insira um tamanho válido.");
        return;
    }

    for (let i = 0; i < size; i++) {
        try {
            const response = await fetch("/get-data-from-address", {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({address: parseInt(address, 10) + i})
            });
            const data = await response.json();
            if (data.error) {
                alert(`Erro ao acessar memória: ${data.error}`);
                contentDiv.innerHTML = 'Erro!';
                return;
            }
            //contentDiv.innerHTML += String.fromCharCode(data.value);
            const char = String.fromCharCode(data.value);
            // Verifica se é um caractere imprimível (exceto \n, \t, null)
            if (data.value === 0) {
                contentDiv.innerHTML += "\\0";
            } else if (data.value === 10) {
                contentDiv.innerHTML += "\\n";
            } else if (data.value === 9) {
                contentDiv.innerHTML += "\\t";
            } else if (data.value === 13) {
                contentDiv.innerHTML += "\\r";
            } else if (char >= ' ' && char <= '~') {
                contentDiv.innerHTML += char;
            } else {
                contentDiv.innerHTML += "?";
            }
        } catch (error) {
            console.error("Erro ao buscar /get-memory:", error);
            contentDiv.innerHTML = 'Erro na comunicação!';
            return;
        }
    }
}

function SetAddressFromPage(address) {
    document.getElementById("address-input").value = address;
}

function LimparMemoria() {
    document.getElementById("memory-content-main").innerHTML = "";
    //document.getElementById("address-input").value = "";
    //document.getElementById("size-input").value = "";
}
