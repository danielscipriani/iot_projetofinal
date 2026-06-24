#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <ESPmDNS.h>

#include <WiFiClient.h>
#include <MQTT.h>
#include <ArduinoJson.h>

#include <vector>
#include "measurement.h"

// =========================
// SERVIDOR / NVS
// =========================

WebServer server(80);
Preferences prefs;

// =========================
// MQTT
// =========================

WiFiClient net;
MQTTClient mqtt(1000);

const char* mqttServer = "broker.emqx.io";
const int mqttPort = 1883;


const char* mqttUser = "";   // se não tiver, deixa vazio
const char* mqttPass = "";
const char* mqttID   = "ESP32_MONITORAMENTO_01";

// =========================
// AP fallback
// =========================

const char* apSSID = "MonitoramentoESP32";
const char* apPassword = "12345678";

bool usandoAP = false;

// =========================
// TIMERS
// =========================

unsigned long lastMQTTReconnect = 0;
unsigned long lastMQTTPublish = 0;

// =========================
// MQTT CALLBACK
// =========================

void mensagemRecebida(String topic, String payload)
{
    Serial.println("================================");
    Serial.println("MQTT RECEBIDO");
    Serial.println("TOPIC: " + topic);
    Serial.println("PAYLOAD: " + payload);
    Serial.println("================================");
}

// =========================
// MQTT CONNECT
// =========================

void reconectarMQTT()
{
    if (mqtt.connected()) return;

    if (millis() - lastMQTTReconnect < 5000) return;
    lastMQTTReconnect = millis();

    Serial.println();
    Serial.println("[MQTT] Desconectado!");
    Serial.println("[MQTT] Tentando reconectar...");

    bool ok;

    if (strlen(mqttUser) > 0)
        ok = mqtt.connect(mqttID, mqttUser, mqttPass);
    else
        ok = mqtt.connect(mqttID);

    if (ok)
    {
        Serial.println("[MQTT] CONECTADO!");
        mqtt.subscribe("esp32/comandos");
    }
    else
    {
        Serial.print("[MQTT] FALHOU, erro = ");
        Serial.println(mqtt.lastError());
    }
}

// =========================
// MQTT INIT
// =========================

void iniciarMQTT()
{
    Serial.println("[MQTT] Inicializando...");

    mqtt.begin(mqttServer, mqttPort, net);
    mqtt.onMessage(mensagemRecebida);
    mqtt.setKeepAlive(10);

    reconectarMQTT();
}

// =========================
// DADOS FAKE
// =========================

std::vector<Measurement> gerarDadosFake()
{
    std::vector<Measurement> data;

    for (int i = 0; i < 5; i++)
    {
        Measurement m;
        m.timestamp = millis();
        m.voltage = 127.0 + random(-200, 200) / 100.0;
        m.current = random(50, 300) / 100.0;
        m.power = m.voltage * m.current;

        data.push_back(m);
    }

    return data;
}

// =========================
// MIME
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
// FILE SERVER
// =========================

void handleFile(String path)
{
    if (path == "/") path = "/index.html";

    Serial.println("[HTTP] Arquivo: " + path);

    if (!LittleFS.exists(path))
    {
        server.send(404, "text/plain", "Arquivo nao encontrado");
        return;
    }

    File file = LittleFS.open(path, "r");
    server.streamFile(file, getContentType(path));
    file.close();
}

// =========================
// API DADOS
// =========================

void handleApiDados()
{
    auto data = gerarDadosFake();

    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();

    for (auto &m : data)
    {
        JsonDocument item;
        item["timestamp"] = m.timestamp;
        item["voltage"] = m.voltage;
        item["current"] = m.current;
        item["power"] = m.power;
        array.add(item);
    }

    String output;
    serializeJson(doc, output);

    server.send(200, "application/json", output);
}

// =========================
// STATUS
// =========================

void handleStatus()
{
    JsonDocument doc;

    doc["modo"] = usandoAP ? "AP" : "WIFI";
    doc["ip"] = usandoAP ? WiFi.softAPIP().toString() : WiFi.localIP().toString();

    String output;
    serializeJson(doc, output);

    server.send(200, "application/json", output);
}

// =========================
// SALVAR WIFI
// =========================

void handleSalvarWiFi()
{
    String ssid = server.arg("ssid");
    String senha = server.arg("senha");

    Serial.println("[WIFI] Salvando SSID: " + ssid);

    prefs.begin("wifi", false);
    prefs.putString("ssid", ssid);
    prefs.putString("senha", senha);
    prefs.end();

    server.send(200, "text/plain", "Salvo. Reiniciando...");
    delay(1000);
    ESP.restart();
}

// =========================
// WIFI CONNECT
// =========================

bool conectarWiFi()
{
    Serial.println("[WIFI] Lendo NVS...");

    prefs.begin("wifi", true);

    String ssid = prefs.getString("ssid", "");
    String senha = prefs.getString("senha", "");

    prefs.end();

    Serial.println("[WIFI] SSID: " + ssid);

    if (ssid.isEmpty())
    {
        Serial.println("[WIFI] Nenhuma rede salva");
        return false;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), senha.c_str());

    Serial.println("[WIFI] Conectando...");

    unsigned long start = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000)
    {
        Serial.print(".");
        delay(500);
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("[WIFI] OK!");
        Serial.println(WiFi.localIP());

        if (MDNS.begin("monitoramento"))
            Serial.println("[mDNS] http://monitoramento.local");

        return true;
    }

    Serial.println("[WIFI] FALHOU");
    return false;
}

// =========================
// AP MODE
// =========================

void criarAP()
{
    Serial.println("[AP] Iniciando...");

    usandoAP = true;

    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID, apPassword);

    Serial.println("[AP] OK");
    Serial.println(WiFi.softAPIP());
}

// =========================
// ROTAS
// =========================

void configurarRotas()
{
    server.on("/", HTTP_GET, []()
    {
        handleFile("/index.html");
    });

    server.on("/api/dados", handleApiDados);
    server.on("/api/status", handleStatus);
    server.on("/api/configurar-wifi", HTTP_POST, handleSalvarWiFi);

    server.onNotFound([]()
    {
        Serial.println("[HTTP] 404: " + server.uri());
        handleFile(server.uri());
    });
}

// =========================
// MQTT SEND
// =========================

void enviarMQTT()
{
    if (!mqtt.connected()) return;

    auto data = gerarDadosFake();

    for (auto &m : data)
    {
        JsonDocument doc;

        doc["voltage"] = m.voltage;
        doc["current"] = m.current;
        doc["power"] = m.power;
        doc["timestamp"] = m.timestamp;

        String payload;
        serializeJson(doc, payload);

        Serial.println("[MQTT] Enviando: " + payload);

        mqtt.publish("dados_sensores", payload);
    }
}

// =========================
// SETUP
// =========================

void setup()
{
    Serial.begin(115200);
    delay(2000);

    Serial.println("================================");
    Serial.println("ESP32 START");
    Serial.println("================================");

    LittleFS.begin();

    if (!conectarWiFi())
    {
        criarAP();
    }

    if (!usandoAP)
    {
        iniciarMQTT();
    }

    configurarRotas();
    server.begin();

    Serial.println("[HTTP] Server iniciado");
}

// =========================
// LOOP
// =========================

void loop()
{
    server.handleClient();

    if (!usandoAP)
    {
        reconectarMQTT();
        mqtt.loop();
    }

    if (!usandoAP && millis() - lastMQTTPublish > 2000)
    {
        lastMQTTPublish = millis();
        enviarMQTT();
    }
}
