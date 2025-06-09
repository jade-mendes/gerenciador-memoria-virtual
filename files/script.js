document.getElementById('configForm').addEventListener('submit', function (e) {
    e.preventDefault();

    const form = e.target;

    // Unit conversion factors
    const units = {
        'B': 1,
        'KB': 1024,
        'MB': 1048576
    };

    // Convert values to bytes
    const pageSize = parseFloat(form.elements['PAGE_SIZE'].value) *
        units[form.elements['PAGE_SIZE_DIM'].value];

    const mpSize = parseFloat(form.elements['MP_SIZE'].value) *
        units[form.elements['MP_SIZE_DIM'].value];

    const msSize = parseFloat(form.elements['MS_SIZE'].value) *
        units[form.elements['MS_SIZE_DIM'].value];

    // Map policy value
    let policyValue = form.elements['policy'].value;
    if (policyValue === 'relogio') {
        policyValue = 'CLOCK';
    }

    // Prepare config with byte values
    const config = {
        PAGE_SIZE: Math.round(pageSize),
        MP_SIZE: Math.round(mpSize),
        MS_SIZE: Math.round(msSize),
        TLB_SIZE: parseInt(form.elements['TLB_SIZE'].value),
        BITS_LOGICAL_ADDRESS: parseInt(form.elements['BITS_LOGICAL_ADDRESS'].value),
        SUB_POLICY_TYPE: policyValue
    };

    // Send JSON to backend
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
            window.location.href = '/simulador.html';
        })
        .catch(error => {
            console.error('Erro:', error);
            alert('Erro ao iniciar a simulação.');
        });
});