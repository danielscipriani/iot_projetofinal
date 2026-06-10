#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <vector>

#include "measurement.h"

char* ssid = "MonitoramentoESP32";
char* password = "12345678";

WebServer server(80);

std::vector<Measurement> gerarDadosFake() {
    std::vector<Measurement> data;

    for (int i = 0; i < 10; i++) {
        Measurement m;

        m.timestamp = millis() + i * 1000;
        m.voltage = 127.0 + random(-200, 200) / 100.0;
        m.current = random(50, 300) / 100.0;
        m.power = m.voltage * m.current;

        data.push_back(m);
    }

    return data;
}

String getContentType(String filename) {
    if (filename.endsWith(".html")) return "text/html";
    if (filename.endsWith(".css")) return "text/css";
    if (filename.endsWith(".js")) return "application/javascript";
    if (filename.endsWith(".json")) return "application/json";

    return "text/plain";
}

void handleFile(String path) {
    if (path == "/") {
        path = "/index.html";
    }

    Serial.println("Caminho:"+ String(path));

    // File arquivo = LittleFS.open("/index.html", "r");
    if (!LittleFS.exists(path)) {
        Serial.println("Caminho:"+ String(path));
        server.send(404, "text/plain", "Arquivo nao encontrado");
        return;
    }

    File file = LittleFS.open(path, "r");
    server.streamFile(file, getContentType(path));
    file.close();
}

void handleApiDados() {
    std::vector<Measurement> data = gerarDadosFake();

    std::string json = measurementsToJson(data);

    server.send(200, "application/json", json.c_str());
}

void setup() {
    Serial.begin(115200);

    randomSeed(esp_random());

    if (!LittleFS.begin()) {
        Serial.println("Erro ao iniciar LittleFS");
        return;
    }

    WiFi.softAP(ssid, password);

    Serial.println();
    Serial.println("Access Point criado");
    Serial.print("Nome da rede: ");
    Serial.println(ssid);
    Serial.print("Senha: ");
    Serial.println(password);
    Serial.print("IP do ESP32: ");
    Serial.println(WiFi.softAPIP());

    server.on("/api/dados", handleApiDados);

    server.onNotFound([]() {
        handleFile(server.uri());
    });

    server.begin();

    Serial.println("Servidor iniciado");
}

void loop() {
    server.handleClient();
}