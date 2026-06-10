const canvas = document.getElementById("chart");
const ctx = canvas.getContext("2d");

function drawChart(data, metric) {
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    if (!data || data.length === 0) {
        return;
    }

    const values = data.map(item => item[metric]);

    const min = Math.min(...values);
    const max = Math.max(...values);

    const paddingLeft = 50;
    const paddingRight = 20;
    const paddingTop = 30;
    const paddingBottom = 40;

    const chartWidth = canvas.width - paddingLeft - paddingRight;
    const chartHeight = canvas.height - paddingTop - paddingBottom;

    ctx.beginPath();
    ctx.moveTo(paddingLeft, paddingTop);
    ctx.lineTo(paddingLeft, canvas.height - paddingBottom);
    ctx.lineTo(canvas.width - paddingRight, canvas.height - paddingBottom);
    ctx.stroke();

    ctx.fillText(max.toFixed(2), 5, paddingTop + 5);
    ctx.fillText(min.toFixed(2), 5, canvas.height - paddingBottom);

    ctx.beginPath();

    values.forEach((value, index) => {
        const x = paddingLeft + index * (chartWidth / (values.length - 1));
        const y = canvas.height - paddingBottom - ((value - min) / (max - min || 1)) * chartHeight;

        if (index === 0) {
            ctx.moveTo(x, y);
        } else {
            ctx.lineTo(x, y);
        }
    });

    ctx.stroke();

    ctx.fillText("Últimas medições", canvas.width / 2 - 45, canvas.height - 10);
}

async function carregarDados() {
    const resposta = await fetch("/api/dados");
    const dados = await resposta.json();

    if (dados.length > 0) {
        const ultimo = dados[dados.length - 1];

        document.getElementById("voltage").textContent = ultimo.voltage.toFixed(2) + " V";
        document.getElementById("current").textContent = ultimo.current.toFixed(2) + " A";
        document.getElementById("power").textContent = ultimo.power.toFixed(2) + " W";
    }

    const metric = document.getElementById("metric").value;

    drawChart(dados, metric);
}

document.getElementById("metric").addEventListener("change", carregarDados);

carregarDados();

setInterval(carregarDados, 2000);