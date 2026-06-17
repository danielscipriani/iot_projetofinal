async function salvarWifi()
{
    const ssid =
        document.getElementById("ssid").value.trim();

    const senha =
        document.getElementById("senha").value;

    const status =
        document.getElementById("status");

    if (!ssid)
    {
        status.innerText =
            "Informe o nome da rede.";

        return;
    }

    status.innerText =
        "Salvando configuração...";

    try
    {
        const response = await fetch(
            "/api/configurar-wifi",
            {
                method: "POST",
                headers:
                {
                    "Content-Type":
                        "application/x-www-form-urlencoded"
                },
                body:
                    `ssid=${encodeURIComponent(ssid)}&senha=${encodeURIComponent(senha)}`
            }
        );

        const texto =
            await response.text();

        status.innerText = texto;
    }
    catch (erro)
    {
        console.error(erro);

        status.innerText =
            "Erro ao enviar configuração.";
    }
}

async function carregarStatus()
{
    try
    {
        const response =
            await fetch("/api/status");

        const data =
            await response.json();

        const ipInfo =
            document.getElementById("ipInfo");

        ipInfo.innerText =
            `Modo: ${data.modo} | IP: ${data.ip}`;
    }
    catch (erro)
    {
        console.error(erro);
    }
}

window.onload = carregarStatus;