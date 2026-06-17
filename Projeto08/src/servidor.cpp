#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <ESPmDNS.h>

#include <vector>

#include "measurement.h"

WebServer server(80);
Preferences prefs;

// =========================
// AP de fallback
// =========================

const char* apSSID = "MonitoramentoESP32";
const char* apPassword = "12345678";

bool usandoAP = false;

// =========================
// Dados de teste
// =========================

std::vector<Measurement> gerarDadosFake()
{
    std::vector<Measurement> data;

    for (int i = 0; i < 10; i++)
    {
        Measurement m;

        m.timestamp = millis() + i * 1000;
        m.voltage = 127.0 + random(-200, 200) / 100.0;
        m.current = random(50, 300) / 100.0;
        m.power = m.voltage * m.current;

        data.push_back(m);
    }

    return data;
}

// =========================
// Tipos MIME
// =========================

String getContentType(String filename)
{
    if (filename.endsWith(".html")) return "text/html";
    if (filename.endsWith(".css")) return "text/css";
    if (filename.endsWith(".js")) return "application/javascript";
    if (filename.endsWith(".json")) return "application/json";

    return "text/plain";
}

// =========================
// Arquivos estáticos
// =========================

void handleFile(String path)
{
    if (path == "/")
    {
        if (usandoAP)
            path = "/wifi.html";
        else
            path = "/index.html";
    }

    Serial.println("Arquivo solicitado: " + path);

    if (!LittleFS.exists(path))
    {
        Serial.println("Arquivo nao encontrado: " + path);

        server.send(
            404,
            "text/plain",
            "Arquivo nao encontrado"
        );

        return;
    }

    File file = LittleFS.open(path, "r");

    if (!file)
    {
        server.send(
            500,
            "text/plain",
            "Erro ao abrir arquivo"
        );

        return;
    }

    server.streamFile(
        file,
        getContentType(path)
    );

    file.close();
}

// =========================
// API de dados
// =========================

void handleApiDados()
{
    auto data = gerarDadosFake();

    std::string json =
        measurementsToJson(data);

    server.send(
        200,
        "application/json",
        json.c_str()
    );
}

// =========================
// API status
// =========================

void handleStatus()
{
    String json = "{";

    json += "\"modo\":\"";
    json += usandoAP ? "AP" : "WIFI";
    json += "\",";

    json += "\"ip\":\"";

    if (usandoAP)
        json += WiFi.softAPIP().toString();
    else
        json += WiFi.localIP().toString();

    json += "\"}";

    server.send(
        200,
        "application/json",
        json
    );
}

// =========================
// Salvar WiFi
// =========================

void handleSalvarWiFi()
{
    if (!server.hasArg("ssid") ||
        !server.hasArg("senha"))
    {
        server.send(
            400,
            "text/plain",
            "Parametros invalidos"
        );

        return;
    }

    String ssid = server.arg("ssid");
    String senha = server.arg("senha");

    Serial.println();
    Serial.println("Salvando credenciais:");
    Serial.println(ssid);

    if (!prefs.begin("wifi", false))
    {
        server.send(
            500,
            "text/plain",
            "Erro ao abrir NVS"
        );

        return;
    }

    prefs.putString("ssid", ssid);
    prefs.putString("senha", senha);

    prefs.end();

    server.send(
        200,
        "text/plain",
        "Configuracao salva. Reiniciando..."
    );

    delay(1000);

    ESP.restart();
}

// =========================
// Conectar WiFi
// =========================

bool conectarWiFi()
{
    Serial.println("Abrindo Preferences...");

    bool ok = prefs.begin("wifi", true);

    Serial.print("prefs.begin = ");
    Serial.println(ok);

    if (!ok)
    {
        Serial.println("Falha ao abrir NVS");
        return false;
    }

    String ssid = prefs.getString("ssid", "");
    String senha = prefs.getString("senha", "");

    prefs.end();

    Serial.print("SSID encontrado: ");
    Serial.println(ssid);

    if (ssid.isEmpty())
    {
        Serial.println("Nenhuma rede configurada.");
        return false;
    }

    Serial.println();
    Serial.println("Tentando conectar ao WiFi:");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);

    WiFi.begin(
        ssid.c_str(),
        senha.c_str()
    );

    unsigned long inicio = millis();

    while (
        WiFi.status() != WL_CONNECTED &&
        millis() - inicio < 15000
    )
    {
        Serial.print(".");
        delay(500);
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("WiFi conectado!");

        Serial.print("Conectado na rede: ");
        Serial.println(WiFi.SSID());

        Serial.print("IP: ");
        Serial.println(WiFi.localIP());

        if (MDNS.begin("monitoramento"))
        {
            Serial.println("mDNS iniciado");
            Serial.println("http://monitoramento.local");
        }

        return true;
    }

    Serial.println("Falha ao conectar.");

    return false;
}

// =========================
// Criar AP
// =========================

void criarAP()
{
    Serial.println("Executando criarAP()");

    usandoAP = true;

    Serial.println();
    Serial.println("Criando Access Point...");

    WiFi.mode(WIFI_AP);

    if (!WiFi.softAP(
            apSSID,
            apPassword))
    {
        Serial.println("Falha ao criar AP.");
        return;
    }

    Serial.println("AP criado com sucesso.");

    Serial.print("SSID: ");
    Serial.println(apSSID);

    Serial.print("Senha: ");
    Serial.println(apPassword);

    Serial.print("IP AP: ");
    Serial.println(WiFi.softAPIP());
}

// =========================
// Rotas
// =========================

void configurarRotas()
{
    Serial.println("Configurando rotas...");

    server.on("/teste", HTTP_GET, []()
    {
        Serial.println("Rota /teste chamada");
        server.send(200, "text/plain", "FUNCIONOU");
    });

    server.on("/api/status", HTTP_GET, []()
    {
        Serial.println("Rota /api/status chamada");

        String json =
            "{\"modo\":\"" +
            String(usandoAP ? "AP" : "WIFI") +
            "\"}";

        server.send(
            200,
            "application/json",
            json
        );
    });

    server.on("/wifi", HTTP_GET, []()
    {
        Serial.println("Rota /wifi chamada");
        handleFile("/wifi.html");
    });

    server.on("/api/dados", HTTP_GET, handleApiDados);

    server.on(
        "/api/configurar-wifi",
        HTTP_POST,
        handleSalvarWiFi
    );

    server.onNotFound([]()
    {
        Serial.print("404 -> ");
        Serial.println(server.uri());

        handleFile(server.uri());
    });

    Serial.println("Rotas configuradas");
}

// =========================
// Setup
// =========================

void setup()
{
    Serial.begin(115200);

    delay(5000);

    Serial.println("################################");
    Serial.println("SETUP EXECUTANDO");
    Serial.println("################################");

    randomSeed(esp_random());

    Serial.println("Iniciando LittleFS...");

    if (!LittleFS.begin())
    {
        File root = LittleFS.open("/");

        File file = root.openNextFile();

        while(file)
{
    Serial.print("Arquivo: ");
    Serial.println(file.name());

    file = root.openNextFile();
}
        Serial.println("ERRO AO INICIAR LITTLEFS");
        return;
    }

    Serial.println("LittleFS OK");

    File root = LittleFS.open("/");

File file = root.openNextFile();

while (file)
{
    Serial.print("Arquivo encontrado: ");
    Serial.println(file.name());

    file = root.openNextFile();
}

if (!conectarWiFi())
    {
        Serial.println("Entrando em modo AP");

        criarAP();

        Serial.println();
        Serial.println("Modo configuracao ativo");
        Serial.println("Acesse:");
        Serial.println("http://192.168.4.1");
    }

    configurarRotas();

    server.begin();

    Serial.println();
    Serial.println("Servidor HTTP iniciado.");

    if (usandoAP)
    {
        Serial.println("Modo AP ativo");
    }
    else
    {
        Serial.println("Modo WiFi ativo");
    }
}

// =========================
// Loop
// =========================

void loop()
{
    server.handleClient();

    static unsigned long ultimoCheck = 0;

    if (!usandoAP &&
        millis() - ultimoCheck > 10000)
    {
        ultimoCheck = millis();

        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("WiFi desconectado.");

            WiFi.reconnect();

            unsigned long inicio = millis();

            while (
                WiFi.status() != WL_CONNECTED &&
                millis() - inicio < 5000)
            {
                delay(100);
            }

            if (WiFi.status() == WL_CONNECTED)
            {
                Serial.println("Reconectado.");
            }
            else
            {
                Serial.println("Falha na reconexao.");
            }
        }
    }
}