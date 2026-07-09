#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <ESPmDNS.h>

#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <ArduinoJson.h>
#include <PZEM004Tv30.h>

#include <vector>
#include <cmath>
#include <cstring>

#include "measurement.h"
#include "certificados.h"

// =====================================================
// Matter
// =====================================================
// Deixe 1 para usar o Matter como no iot.cpp.
// Se a sua placa/core não tiver suporte à biblioteca Matter, altere para 0.

#ifndef HABILITAR_MATTER
#define HABILITAR_MATTER 1
#endif

#if HABILITAR_MATTER
#include <Matter.h>
MatterOnOffPlugin tomadaMatter;
#endif

// =====================================================
// Hardware
// =====================================================

#define PIN_RELE 10
#define RELE_LIGA HIGH
#define RELE_DESLIGA LOW

// Pinos vindos do teste_PZEM.cpp.txt.
// Atenção: em muitos ESP32, GPIO36 é somente entrada. Se o PZEM não responder,
// troque PZEM_TX_PIN para um GPIO com saída, por exemplo 17, 25, 26 ou 27.
#define PZEM_RX_PIN 35  // ESP32 RX  <- PZEM TX
#define PZEM_TX_PIN 36  // ESP32 TX  -> PZEM RX

// =====================================================
// Identificação lógica da tomada / medidor
// =====================================================

#define TOMADA_ID "tomada_sala_01"
#define TOMADA_NOME "Tomada da sala"
#define TOMADA_COMODO "Sala"
#define TOMADA_POSICAO_X 35
#define TOMADA_POSICAO_Y 62

// =====================================================
// Servidor / NVS - base do servidor_novo.cpp
// =====================================================

WebServer server(80);
Preferences prefs;

const char* apSSID = "MonitoramentoESP32";
const char* apPassword = "12345678";

bool usandoAP = false;

// =====================================================
// MQTT - base do servidor_novo.cpp
// =====================================================

WiFiClientSecure conexaoSegura;
MQTTClient mqtt(512);



const char* mqttServer = "mqtt.janks.dev.br";
const int mqttPort = 8883;
const char* mqttUser = "aula";
const char* mqttPass = "zowmad-tavQez";
const char* mqttID = "vaidarcertooo"; 

const char* mqttTopicComandos = "casa/tomadas/sala/estado";
const char* mqttTopicDados = "casa/tomadas/sala/estado";
const char* mqttTopicEstado = "casa/tomadas/sala/estado";

// =====================================================
// PZEM / medições
// =====================================================

PZEM004Tv30 pzem(Serial2, PZEM_RX_PIN, PZEM_TX_PIN);

Measurement ultimaMedicao;
std::vector<Measurement> historico;

const size_t MAX_HISTORICO = 60;
const unsigned long INTERVALO_LEITURA_PZEM_MS = 2000;
const unsigned long INTERVALO_MQTT_PUBLICACAO_MS = 2000;
const unsigned long INTERVALO_REPUBLICAR_ESTADO_MS = 30000;

unsigned long lastMQTTReconnect = 0;
unsigned long lastPzemRead = 0;
unsigned long lastMQTTPublish = 0;
unsigned long lastEstadoPublish = 0;

// =====================================================
// Relé / estado
// =====================================================

bool releLigado = false;
bool matterIniciado = false;
bool pairingMatterImpresso = false;

unsigned long lastSend = 0;

// =====================================================
// Utilitários JSON/HTTP
// =====================================================

String getContentType(String filename)
{
    if (filename.endsWith(".html")) return "text/html";
    if (filename.endsWith(".css")) return "text/css";
    if (filename.endsWith(".js")) return "application/javascript";
    if (filename.endsWith(".json")) return "application/json";
    if (filename.endsWith(".png")) return "image/png";
    if (filename.endsWith(".jpg")) return "image/jpeg";
    if (filename.endsWith(".jpeg")) return "image/jpeg";
    if (filename.endsWith(".ico")) return "image/x-icon";
    return "text/plain";
}

void adicionarHistorico(const Measurement& m)
{
    if (!m.valid) return;

    historico.push_back(m);
    if (historico.size() > MAX_HISTORICO) {
        historico.erase(historico.begin());
    }
}

// =====================================================
// PZEM
// =====================================================

Measurement lerPZEM()
{
    Measurement m;
    m.timestamp = millis();

    m.voltage = pzem.voltage();
    m.current = pzem.current();
    m.power = pzem.power();
    m.energy = pzem.energy();
    m.frequency = pzem.frequency();
    m.powerFactor = pzem.pf();

    m.valid =
        !isnan(m.voltage) &&
        !isnan(m.current) &&
        !isnan(m.power) &&
        !isnan(m.energy) &&
        !isnan(m.frequency) &&
        !isnan(m.powerFactor);

    return m;
}

void imprimirMedicaoSerial(const Measurement& m)
{
    if (!m.valid) {
        Serial.println("[PZEM] Erro ao ler uma ou mais grandezas.");
        return;
    }

    Serial.println("--------------------");
    Serial.print("[PZEM] Tensao: "); Serial.print(m.voltage); Serial.println(" V");
    Serial.print("[PZEM] Corrente: "); Serial.print(m.current); Serial.println(" A");
    Serial.print("[PZEM] Potencia: "); Serial.print(m.power); Serial.println(" W");
    Serial.print("[PZEM] Energia: "); Serial.print(m.energy, 3); Serial.println(" kWh");
    Serial.print("[PZEM] Frequencia: "); Serial.print(m.frequency); Serial.println(" Hz");
    Serial.print("[PZEM] FP: "); Serial.println(m.powerFactor);
}

void atualizarMedicao()
{
    if (millis() - lastPzemRead < INTERVALO_LEITURA_PZEM_MS) return;
    lastPzemRead = millis();

    ultimaMedicao = lerPZEM();
    imprimirMedicaoSerial(ultimaMedicao);
    adicionarHistorico(ultimaMedicao);
}

// =====================================================
// Relé / tomada
// =====================================================

void publicarEstadoTomada();

void aplicarEstadoRele(bool ligado)
{
    releLigado = ligado;
    digitalWrite(PIN_RELE, ligado ? RELE_LIGA : RELE_DESLIGA);

    Serial.print("[RELE] ");
    Serial.println(ligado ? "LIGADO" : "DESLIGADO");

   // publicarEstadoTomada();
}

#if HABILITAR_MATTER
bool receberComandoMatter(bool ligado)
{
    aplicarEstadoRele(ligado);
    return true;
}
#endif

void definirEstadoTomada(bool ligado)
{
#if HABILITAR_MATTER
    if (matterIniciado) {
        tomadaMatter.setOnOff(ligado);
    }
#endif

    if (releLigado != ligado) {
        aplicarEstadoRele(ligado);
    } else {
       // publicarEstadoTomada();
    }
}

void alternarEstadoTomada()
{
    definirEstadoTomada(!releLigado);
}

void resetarEnergiaPZEM()
{
    Serial.println("[PZEM] Resetando contador de energia...");
    pzem.resetEnergy();
}

// =====================================================
// MQTT
// =====================================================

void tratarComandoTexto(String comando)
{
    comando.trim();
    comando.toLowerCase();

    if (comando == "1" || comando == "on" || comando == "liga" || comando == "ligar") {
        definirEstadoTomada(true);
    } else if (comando == "0" || comando == "off" || comando == "desliga" || comando == "desligar") {
        definirEstadoTomada(false);
    } else if (comando == "toggle" || comando == "alternar" || comando == "t") {
        alternarEstadoTomada();
    } else if (comando == "reset_energy" || comando == "resetar_energia") {
        resetarEnergiaPZEM();
    } else {
        Serial.println("[MQTT] Comando desconhecido: " + comando);
    }
}

void mensagemRecebida(String &topic, String &payload)
{
    Serial.println("================================");
    Serial.println("[MQTT] RECEBIDO");
    Serial.println("TOPIC: " + topic);
    Serial.println("PAYLOAD: " + payload);
    Serial.println("================================");

    JsonDocument doc;
    DeserializationError erro = deserializeJson(doc, payload);

    if (!erro) {
        if (doc["rele"].is<bool>()) {
            definirEstadoTomada(doc["rele"].as<bool>());
            return;
        }

        if (doc["ligado"].is<bool>()) {
            definirEstadoTomada(doc["ligado"].as<bool>());
            return;
        }

        if (doc["comando"].is<const char*>()) {
            tratarComandoTexto(String(doc["comando"].as<const char*>()));
            return;
        }
    }

    tratarComandoTexto(payload);
}

void reconectarMQTT()
{
    if (usandoAP) return;
    if (WiFi.status() != WL_CONNECTED) return;
    if (mqtt.connected()) return;

    if (millis() >= lastMQTTReconnect + 5000) return;
    lastMQTTReconnect = millis();

    Serial.println();
    Serial.println("[MQTT] Desconectado");
    Serial.println("[MQTT] Tentando reconectar...");

    bool ok;
    ok = mqtt.connect(mqttID, mqttUser, mqttPass);

    if (ok) {
        Serial.println("[MQTT] CONECTADO");
        mqtt.subscribe(mqttTopicComandos);
    } else {
        Serial.print("[MQTT] FALHOU, erro = ");
        Serial.println(mqtt.lastError());
    }
}

void iniciarMQTT()
{
    Serial.println("[MQTT] Inicializando...");
    mqtt.begin(mqttServer, mqttPort, conexaoSegura);
    mqtt.onMessage(mensagemRecebida);
    mqtt.setKeepAlive(10);
    reconectarMQTT();
}

void publicarEstadoTomada()
{
    if (usandoAP || WiFi.status() != WL_CONNECTED) return;
    if (!mqtt.connected()) return;

    JsonDocument doc;
    doc["tomada_id"] = TOMADA_ID;
    doc["nome"] = TOMADA_NOME;
    doc["comodo"] = TOMADA_COMODO;
    doc["rele"] = releLigado;
    doc["estado"] = releLigado ? "ligada" : "desligada";
    doc["x"] = TOMADA_POSICAO_X;
    doc["y"] = TOMADA_POSICAO_Y;
    doc["ip"] = WiFi.localIP().toString();

    JsonObject med = doc["medicao"].to<JsonObject>();
    med["valid"] = ultimaMedicao.valid;
    med["voltage"] = ultimaMedicao.valid ? ultimaMedicao.voltage : 0.0f;
    med["current"] = ultimaMedicao.valid ? ultimaMedicao.current : 0.0f;
    med["power"] = ultimaMedicao.valid ? ultimaMedicao.power : 0.0f;
    med["energy"] = ultimaMedicao.valid ? ultimaMedicao.energy : 0.0f;
    med["frequency"] = ultimaMedicao.valid ? ultimaMedicao.frequency : 0.0f;
    med["powerFactor"] = ultimaMedicao.valid ? ultimaMedicao.powerFactor : 0.0f;

    String payload;
    serializeJson(doc, payload);

    Serial.println("[MQTT] Estado: " + payload);
    mqtt.publish(mqttTopicEstado, payload);
}

void enviarMedicaoMQTT()
{
    if (usandoAP || WiFi.status() != WL_CONNECTED) return;
    if (!mqtt.connected()) return;
    if (!ultimaMedicao.valid) return;
    if (millis() - lastMQTTPublish < INTERVALO_MQTT_PUBLICACAO_MS) return;
    lastMQTTPublish = millis();

    JsonDocument doc;
    doc["tomada_id"] = TOMADA_ID;
    doc["nome"] = TOMADA_NOME;
    doc["comodo"] = TOMADA_COMODO;
    doc["rele"] = releLigado;
    doc["timestamp"] = ultimaMedicao.timestamp;
    doc["voltage"] = ultimaMedicao.voltage;
    doc["current"] = ultimaMedicao.current;
    doc["power"] = ultimaMedicao.power;
    doc["energy"] = ultimaMedicao.energy;
    doc["frequency"] = ultimaMedicao.frequency;
    doc["powerFactor"] = ultimaMedicao.powerFactor;
    doc["valid"] = ultimaMedicao.valid;

    String payload;
    serializeJson(doc, payload);

    Serial.println("[MQTT] Dados: " + payload);
    mqtt.publish(mqttTopicDados, payload);
}

// void republicarEstadoPeriodicamente()
// {
//     if (millis() - lastEstadoPublish < INTERVALO_REPUBLICAR_ESTADO_MS) return;
//     lastEstadoPublish = millis();
//     publicarEstadoTomada();
// }

// =====================================================
// Matter
// =====================================================

void iniciarMatterSePossivel()
{
#if HABILITAR_MATTER
    if (usandoAP || matterIniciado || WiFi.status() != WL_CONNECTED) return;

    tomadaMatter.begin(releLigado);
    tomadaMatter.onChange(receberComandoMatter);
    Matter.begin();
    matterIniciado = true;

    Serial.println("[MATTER] Inicializado");
#endif
}

void manterMatter()
{
#if HABILITAR_MATTER
    if (!matterIniciado) return;

    if (!Matter.isDeviceCommissioned() && !pairingMatterImpresso) {
        pairingMatterImpresso = true;
        Serial.println("[MATTER] Dispositivo ainda nao comissionado");
        Serial.printf("[MATTER] Codigo de pareamento: %s\r\n", Matter.getManualPairingCode().c_str());
        Serial.printf("[MATTER] QR code: %s\r\n", Matter.getOnboardingQRCodeUrl().c_str());
    }
#endif
}

// =====================================================
// Serial local
// =====================================================

void lerComandosSerial()
{
    if (!Serial.available()) return;

    char comando = Serial.read();

    if (comando == '1' || comando == 'l' || comando == 'L') {
        definirEstadoTomada(true);
    } else if (comando == '0' || comando == 'd' || comando == 'D') {
        definirEstadoTomada(false);
    } else if (comando == 't' || comando == 'T') {
        alternarEstadoTomada();
    } else if (comando == 'r' || comando == 'R') {
        resetarEnergiaPZEM();
    }
}

// =====================================================
// LittleFS / rotas HTTP - base do servidor_novo.cpp
// =====================================================

void handleFile(String path)
{
    if (path == "/") {
        path = usandoAP ? "/wifi.html" : "/index.html";
    }

    Serial.println("[HTTP] Arquivo: " + path);

    if (!LittleFS.exists(path)) {
        server.send(404, "text/plain", "Arquivo nao encontrado");
        return;
    }

    File file = LittleFS.open(path, "r");
    if (!file) {
        server.send(500, "text/plain", "Erro ao abrir arquivo");
        return;
    }

    server.streamFile(file, getContentType(path));
    file.close();
}

void handleApiDados()
{
    std::string output = measurementsToJson(historico);
    server.send(200, "application/json", output.c_str());
}

void handleApiUltima()
{
    std::string output = measurementToJson(ultimaMedicao);
    server.send(200, "application/json", output.c_str());
}

void handleStatus()
{
    JsonDocument doc;
    doc["modo"] = usandoAP ? "AP" : "WIFI";
    doc["ip"] = usandoAP ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
    doc["wifiConectado"] = WiFi.status() == WL_CONNECTED;
    doc["mqttConectado"] = mqtt.connected();
    doc["matterHabilitado"] = static_cast<bool>(HABILITAR_MATTER);
    doc["matterIniciado"] = matterIniciado;
    doc["rele"] = releLigado;
    doc["tomada_id"] = TOMADA_ID;
    doc["nome"] = TOMADA_NOME;
    doc["historico"] = historico.size();

    JsonObject med = doc["ultimaMedicao"].to<JsonObject>();
    med["valid"] = ultimaMedicao.valid;
    med["timestamp"] = ultimaMedicao.timestamp;
    med["voltage"] = ultimaMedicao.valid ? ultimaMedicao.voltage : 0.0f;
    med["current"] = ultimaMedicao.valid ? ultimaMedicao.current : 0.0f;
    med["power"] = ultimaMedicao.valid ? ultimaMedicao.power : 0.0f;
    med["energy"] = ultimaMedicao.valid ? ultimaMedicao.energy : 0.0f;
    med["frequency"] = ultimaMedicao.valid ? ultimaMedicao.frequency : 0.0f;
    med["powerFactor"] = ultimaMedicao.valid ? ultimaMedicao.powerFactor : 0.0f;

    String output;
    serializeJson(doc, output);
    server.send(200, "application/json", output);
}

void handleSalvarWiFi()
{
    String ssid = server.arg("ssid");
    String senha = server.arg("senha");

    if (ssid.length() == 0) {
        server.send(400, "text/plain", "SSID vazio");
        return;
    }

    Serial.println("[WIFI] Salvando SSID: " + ssid);

    prefs.begin("wifi", false);
    prefs.putString("ssid", ssid);
    prefs.putString("senha", senha);
    prefs.end();

    server.send(200, "text/plain", "Salvo. Reiniciando...");
    delay(1000);
    ESP.restart();
}

void handleLiga()
{
    definirEstadoTomada(true);
    handleStatus();
}

void handleDesliga()
{
    definirEstadoTomada(false);
    handleStatus();
}

void handleToggle()
{
    alternarEstadoTomada();
    handleStatus();
}

void handleResetEnergia()
{
    resetarEnergiaPZEM();
    server.send(200, "application/json", "{\"ok\":true,\"acao\":\"resetEnergy\"}");
}

void configurarRotas()
{
    server.on("/", HTTP_GET, []() {
        handleFile("/");
    });

    server.on("/api/dados", HTTP_GET, handleApiDados);
    server.on("/api/medidas", HTTP_GET, handleApiDados);
    server.on("/api/ultima", HTTP_GET, handleApiUltima);
    server.on("/api/status", HTTP_GET, handleStatus);
    server.on("/api/configurar-wifi", HTTP_POST, handleSalvarWiFi);

    server.on("/api/rele/liga", HTTP_POST, handleLiga);
    server.on("/api/rele/desliga", HTTP_POST, handleDesliga);
    server.on("/api/rele/toggle", HTTP_POST, handleToggle);
    server.on("/api/pzem/reset-energy", HTTP_POST, handleResetEnergia);

    server.onNotFound([]() {
        Serial.println("[HTTP] 404: " + server.uri());
        handleFile(server.uri());
    });
}

// =====================================================
// WiFi / AP - base do servidor_novo.cpp
// =====================================================

bool conectarWiFi()
{
    Serial.println("[WIFI] Lendo configuracao...");

    prefs.begin("wifi", true);

    String ssid  = prefs.getString("ssid", "");
    String senha = prefs.getString("senha", "");

    prefs.end();

    Serial.print("[WIFI] SSID salvo: ");
    Serial.println(ssid);

    if (ssid.isEmpty())
    {
        Serial.println("[WIFI] Nenhuma rede salva.");
        return false;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), senha.c_str());

    unsigned long inicio = millis();

    Serial.print("[WIFI] Conectando");

    while (WiFi.status() != WL_CONNECTED &&
           millis() - inicio < 15000)
    {
        Serial.print(".");
        delay(500);
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
        usandoAP = false;

        Serial.println("[WIFI] Conectado!");
        Serial.print("[WIFI] IP: ");
        Serial.println(WiFi.localIP());

        if (MDNS.begin("monitoramento"))
        {
            Serial.println("[mDNS] http://monitoramento.local");
        }

        return true;
    }

    Serial.println("[WIFI] Falha ao conectar.");

    return false;
}

void criarAP()
{
    Serial.println("[AP] Iniciando...");

    usandoAP = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID, apPassword);

    Serial.println("[AP] OK");
    Serial.println(WiFi.softAPIP());
}

// =====================================================
// Setup / loop únicos da aplicação integrada
// =====================================================

void setup()
{
    Serial.begin(115200);
    delay(2000);

    Serial.println("================================");
    Serial.println("ESP32 MEDIDOR INTEGRADO");
    Serial.println("servidor_novo + iot + PZEM + measurement");
    Serial.println("================================");

    pinMode(PIN_RELE, OUTPUT);
    releLigado = false;
    digitalWrite(PIN_RELE, RELE_DESLIGA);

    if (!LittleFS.begin()) {
        Serial.println("[LittleFS] Falha ao iniciar. O servidor funciona, mas sem arquivos estaticos.");
    } else {
        Serial.println("[LittleFS] OK");
    }

    if (!conectarWiFi()) {
        criarAP();
    }

    if (!usandoAP) {
        conexaoSegura.setCACert(certificado1);
        iniciarMQTT();
        iniciarMatterSePossivel();
    }

    configurarRotas();
    server.begin();

    Serial.println("[HTTP] Servidor iniciado");

    Serial.println();
    Serial.println("================================");

    if (usandoAP)
    {
        Serial.println("MODO: ACCESS POINT");
        Serial.print("SSID: ");
        Serial.println(apSSID);

        Serial.print("Senha: ");
        Serial.println(apPassword);

        Serial.print("Acesse: http://");
        Serial.println(WiFi.softAPIP());

        Serial.print("Status: http://");
        Serial.println(WiFi.softAPIP().toString() + "/api/status");
    }
    else
    {
        Serial.println("MODO: WIFI");

        Serial.print("IP: ");
        Serial.println(WiFi.localIP());

        Serial.print("Pagina: http://");
        Serial.println(WiFi.localIP());

        Serial.print("Status: http://");
        Serial.println(WiFi.localIP().toString() + "/api/status");

        Serial.println("mDNS: http://monitoramento.local");
    }

    Serial.println("================================");
    Serial.println("[Serial] 1/l = liga | 0/d = desliga | t = alterna | r = reset energia PZEM");

    lastSend = millis();
}

void loop()
{
    
    server.handleClient();
    

    lerComandosSerial();
    atualizarMedicao();

    if (!usandoAP) {
        reconectarMQTT();
        if (mqtt.connected()) {
            mqtt.loop();
            if (millis() >= lastSend + 5000) {
                lastSend = millis();
                Serial.println("5 seg");
                publicarEstadoTomada();
                enviarMedicaoMQTT();
                // republicarEstadoPeriodicamente();
            }
            // manterMatter();
        }
    }

    delay(10);
}
