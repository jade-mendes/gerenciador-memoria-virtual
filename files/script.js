document.getElementById('configForm').addEventListener('submit', function (e) {
  e.preventDefault();

  const config = {
    pageSize: parseInt(document.getElementById('pageSize').value),
    mpSize: parseInt(document.getElementById('mpSize').value),
    logicalBits: parseInt(document.getElementById('logicalBits').value),
    tlbLines: parseInt(document.getElementById('tlbLines').value),
    policy: document.getElementById('policy').value
  };

  // Enviar JSON para o backend
  fetch('/start-simulation', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify(config)
  })
      .then(response => {
        if (!response.ok) throw new Error('Erro na simulação');
        return response;
      })
      .then(data => {
        console.log('Simulação iniciada com sucesso:', data);

        // Redirecionar para a próxima página

      })
      .catch(error => {
        console.error('Erro:', error);
        alert('Erro ao iniciar a simulação.');
      });
});
