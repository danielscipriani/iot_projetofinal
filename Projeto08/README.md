# Projeto 08 — Monitoramento de Circuitos AC com ESP32

Este projeto consiste em um sistema IoT para monitoramento de circuitos em corrente alternada (AC). A proposta é medir grandezas elétricas de uma carga conectada ao dispositivo, como tensão, corrente, potência e energia consumida, permitindo visualizar os dados em tempo real e armazená-los para análises posteriores.

O sistema utiliza um ESP32 como unidade principal de controle e comunicação. As medições elétricas são realizadas com o módulo PZEM-004T, responsável por medir os principais parâmetros do circuito AC.

## Funcionamento geral

O dispositivo funciona como um intermediário entre a tomada e o equipamento monitorado. Dessa forma, o equipamento é ligado ao medidor, e o medidor é ligado à rede elétrica.

Durante o funcionamento da carga, o ESP32 coleta os dados do PZEM-004T, disponibiliza informações em um servidor web local e também envia os dados por MQTT para armazenamento e visualização externa.

## Modos de operação do ESP32

O ESP32 possui dois modos principais de funcionamento.

### Modo Access Point

Quando não existe nenhuma rede Wi-Fi salva, o ESP32 inicia em modo Access Point.

Nesse modo, o próprio ESP32 cria uma rede local chamada para configuração inicial. Ao acessar o endereço do servidor gerado pelo ESP32, o usuário encontra uma página para cadastrar o nome e a senha da rede Wi-Fi.

Essas informações são salvas na memória do ESP32 usando Preferences/NVS. Assim, na próxima inicialização, o dispositivo tenta se conectar automaticamente à rede cadastrada.

### Modo Wi-Fi

Quando já existe uma rede Wi-Fi salva, o ESP32 se conecta automaticamente a essa rede.

Após a conexão, o servidor local passa a exibir uma página de monitoramento com gráficos e valores das grandezas medidas. A interface permite visualizar, em tempo real, o comportamento de tensão, corrente e potência ao longo do tempo.

## Interface web local

A interface web do ESP32 é armazenada no LittleFS e é composta por arquivos HTML, CSS e JavaScript.

A página principal apresenta:

* valor atual de tensão;
* valor atual de corrente;
* valor atual de potência;
* gráfico em função do tempo;
* seleção da grandeza exibida no gráfico.

Também existe uma página de configuração Wi-Fi, usada quando o ESP32 está em modo Access Point.

## Medições elétricas

O módulo PZEM-004T é utilizado para medir as grandezas elétricas do circuito AC.

Entre os valores medidos estão:

* tensão;
* corrente;
* potência;
* energia consumida;
* frequência;
* fator de potência.

Essas grandezas são utilizadas tanto para visualização local quanto para envio ao sistema externo via MQTT.

## Comunicação MQTT

Além da página local, o ESP32 também envia as medições por MQTT.

Essa comunicação permite integrar o dispositivo com outros serviços do projeto. Os dados publicados pelo ESP32 podem ser recebidos pelo Node-RED, tratados e encaminhados para armazenamento em banco de dados.

## Node-RED e PostgreSQL

O projeto utiliza Node-RED como intermediário entre o broker MQTT e o banco de dados.

O fluxo do Node-RED recebe os dados enviados pelo ESP32, organiza as informações e salva as medições no PostgreSQL.

Com isso, o sistema mantém um histórico das medições, permitindo análises em intervalos maiores do que aqueles mostrados diretamente na página local do ESP32.

## Grafana

Os dados armazenados no PostgreSQL são utilizados para criação de dashboards no Grafana.

Enquanto a página local do ESP32 oferece uma visualização simples e imediata, o Grafana permite análises mais completas, com gráficos históricos e seleção de intervalos maiores de tempo.

## Integração com Matter e relé

O projeto também prevê integração com Matter, permitindo que o dispositivo seja registrado em uma rede de automação residencial.

Essa etapa está relacionada ao controle de um relé, possibilitando ligar e desligar a tomada monitorada. O estado da tomada também pode ser enviado ao sistema e exibido no dashboard.

## Estrutura do projeto

```text
Projeto08/
├── data/
│   ├── index.html
│   ├── script.js
│   ├── style.css
│   ├── wifi.html
│   └── wifi.js
│
├── src/
│   ├── servidor.cpp
│   ├── measurement.cpp
│   ├── measurement.h
│   └── certificados.h
│
├── flows.json
├── partitions.csv
└── platformio.ini
```

## Principais arquivos

`src/servidor.cpp`

Arquivo principal do servidor no ESP32. Ele configura o Wi-Fi, cria o modo Access Point quando necessário, inicializa o LittleFS, configura as rotas HTTP e serve as páginas web.

`src/measurement.cpp` e `src/measurement.h`

Arquivos responsáveis pela estrutura das medições e pela conversão dos dados para JSON.

`data/index.html`

Página principal de monitoramento, com cards de tensão, corrente e potência, além do gráfico em função do tempo.

`data/wifi.html`

Página usada para configurar a rede Wi-Fi quando o ESP32 está em modo Access Point.

`flows.json`

Fluxo do Node-RED usado para receber os dados MQTT e encaminhar as informações para o PostgreSQL e/ou dashboards.

`platformio.ini`

Arquivo de configuração do PlatformIO, com a placa utilizada, bibliotecas e configurações de compilação.


## Objetivo final

O objetivo final é entregar um dispositivo funcional de monitoramento de energia AC, capaz de medir grandezas elétricas, exibir dados em tempo real, armazenar histórico de medições e se integrar com ferramentas de IoT e automação residencial.

O projeto combina aquisição de dados elétricos, comunicação MQTT, servidor web embarcado, banco de dados, dashboards no Grafana, modelagem de caixa 3D, PCB e integração com Matter.
