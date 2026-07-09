# Medidor de consumo de energia - integração com `servidor_novo.cpp`

Esta versão refaz a integração desconsiderando totalmente o `servidor.cpp` antigo. A base de rede, servidor HTTP, AP de fallback, NVS e MQTT agora vem do `servidor_novo.cpp`.

## Arquivos integrados

- `servidor_novo.cpp`: base principal do firmware.
- `iot.cpp`: controle do relé, identificação lógica da tomada e comandos locais/Matter.
- `measurement.cpp`: serialização das medições em JSON.
- `test_main.cpp`: teste unitário da serialização JSON.
- `teste_PZEM.cpp.txt`: leitura real do sensor PZEM-004T.

## Arquivos finais

- `src/main.cpp`: firmware integrado com um único `setup()` e um único `loop()`.
- `src/measurement.cpp`: conversão das medições em JSON.
- `include/measurement.h`: struct `Measurement`.
- `test/test_main.cpp`: testes unitários atualizados.

## Decisões de integração

1. O `servidor.cpp` antigo foi ignorado.
2. O MQTT usado agora é o do `servidor_novo.cpp`, com `WiFiClient` + biblioteca `MQTT.h`.
3. O MQTT seguro com `WiFiClientSecure`, `PubSubClient` e `certificados.h` do `iot.cpp` não foi mantido nesta versão para evitar duas bibliotecas MQTT concorrendo no mesmo firmware.
4. A função `gerarDadosFake()` foi removida da lógica principal: `/api/dados` agora retorna leituras reais do PZEM guardadas em histórico.
5. O `setup()` e o `loop()` do teste do PZEM foram transformados em funções internas de leitura, sem duplicar `setup()`/`loop()`.
6. A aplicação continua funcionando em modo AP quando não há WiFi salvo no NVS.

## Fluxo atual do firmware

1. Inicializa Serial, relé e LittleFS.
2. Lê SSID/senha salvos no NVS.
3. Se conseguir conectar, entra em modo WiFi, inicia MQTT e Matter.
4. Se não conseguir conectar, abre AP `MonitoramentoESP32` com senha `12345678`.
5. Sobe o servidor HTTP.
6. A cada 2 segundos, lê o PZEM.
7. Se a leitura for válida, salva no histórico e publica via MQTT.
8. Permite controlar o relé por HTTP, MQTT, Serial e Matter.

## Endpoints HTTP

- `GET /api/status`: status geral do ESP32.
- `GET /api/dados`: histórico das últimas medições válidas.
- `GET /api/medidas`: alias de `/api/dados`.
- `GET /api/ultima`: última medição, válida ou inválida.
- `POST /api/rele/liga`: liga o relé.
- `POST /api/rele/desliga`: desliga o relé.
- `POST /api/rele/toggle`: alterna o relé.
- `POST /api/pzem/reset-energy`: zera o contador de energia do PZEM.
- `POST /api/configurar-wifi`: salva `ssid` e `senha` no NVS e reinicia.

## MQTT

Base mantida do `servidor_novo.cpp`:

- Broker: `broker.emqx.io`
- Porta: `1883`
- Cliente: `ESP32_MONITORAMENTO_01`

Tópicos:

- Publicação de medições: `dados_sensores`
- Publicação de estado da tomada: `esp32/estado`
- Recebimento de comandos: `esp32/comandos`

Comandos aceitos em `esp32/comandos`:

Texto simples:

- `liga`, `ligar`, `on`, `1`
- `desliga`, `desligar`, `off`, `0`
- `toggle`, `alternar`, `t`
- `reset_energy`, `resetar_energia`

JSON:

```json
{"rele": true}
```

```json
{"ligado": false}
```

```json
{"comando": "toggle"}
```

## Matter

O código mantém a integração Matter do `iot.cpp`, mas de forma não bloqueante.

No topo de `src/main.cpp` existe:

```cpp
#ifndef HABILITAR_MATTER
#define HABILITAR_MATTER 1
#endif
```

Se a sua placa/core não tiver a biblioteca Matter, altere para:

```cpp
#define HABILITAR_MATTER 0
```

ou compile definindo `HABILITAR_MATTER=0`.

## Pontos importantes de hardware

1. `PIN_RELE = 10`: em muitas placas ESP32, GPIO 6 a 11 podem estar ligados à memória flash. Se a placa travar ou o relé não responder, troque para GPIO 16, 17, 18, 19, 21, 22, 23, 25, 26 ou 27.
2. `PZEM_TX_PIN = 36`: no ESP32 clássico, GPIO36 costuma ser somente entrada. Para TX do ESP32 para RX do PZEM, use um pino com saída, como GPIO17, GPIO25, GPIO26 ou GPIO27.
3. O PZEM trabalha com medição em rede elétrica. Faça os testes com isolamento e cuidado.
4. Os arquivos de interface web (`index.html`, `wifi.html`, CSS e JS) precisam ser enviados para o LittleFS.

## Bibliotecas necessárias

- `PZEM004Tv30`
- `MQTT` de Joel Gaehwiler / 256dpi, usada como `#include <MQTT.h>`
- `ArduinoJson`
- `LittleFS`
- `Preferences`
- `WebServer`
- `ESPmDNS`
- Biblioteca Matter compatível com o seu core ESP32, se `HABILITAR_MATTER = 1`
- `Unity`, apenas para testes

## Teste unitário

O teste unitário valida apenas a serialização JSON de `Measurement`, pois o restante depende de hardware ESP32/PZEM/WiFi.

No PlatformIO, o teste fica em:

```text
test/test_main.cpp
```
