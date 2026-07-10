# Medidor Inteligente de Energia AC

Projeto final de Internet das Coisas voltado ao monitoramento e controle de consumo energético em circuitos de corrente alternada (AC).

O sistema funciona como uma tomada inteligente: o medidor é conectado à rede elétrica e o equipamento monitorado é conectado ao medidor. Durante o funcionamento da carga, o dispositivo mede grandezas elétricas, exibe dados em tempo real, envia informações para um banco de dados e permite análise por dashboards.

## Visão geral

O projeto utiliza um ESP32 como unidade principal de controle. Ele é responsável pela leitura dos sensores, comunicação Wi-Fi, servidor web local, envio de dados via MQTT e integração com os demais módulos do sistema.

A medição elétrica é feita pelo módulo PZEM-004T v3.0, que permite obter dados de tensão, corrente, potência, energia consumida, frequência e fator de potência em circuitos AC.

Além da medição, o projeto também conta com persistência dos dados em banco PostgreSQL, visualização pelo Grafana, fluxo de integração pelo Node-RED, controle de relé e integração com Matter.

## Funcionalidades principais

* Medição de tensão, corrente, potência, energia, frequência e fator de potência.
* Servidor web local no ESP32 para visualização dos dados.
* Gráficos em tempo real de tensão, corrente e potência.
* Configuração de Wi-Fi pelo próprio dispositivo.
* Envio das medições via MQTT.
* Tratamento dos dados pelo Node-RED.
* Armazenamento histórico em banco PostgreSQL.
* Dashboards no Grafana para análise das medições.
* Controle da tomada por relé.
* Integração com Matter para automação residencial.
* PCB própria para organização do circuito.
* Gabinete 3D para proteção e montagem do sistema.

## Funcionamento do dispositivo

O ESP32 possui dois modos principais de funcionamento: modo Access Point e modo Wi-Fi.

### Modo Access Point

Quando o dispositivo ainda não possui uma rede Wi-Fi salva, o ESP32 inicia em modo Access Point. Nesse modo, ele cria uma rede própria para configuração inicial.

Ao acessar o servidor gerado pelo ESP32, o usuário encontra uma página para cadastrar o nome e a senha da rede Wi-Fi. Essas informações ficam salvas na memória do dispositivo.

Assim, na próxima vez que o sistema for ligado, o ESP32 tenta se conectar automaticamente à rede cadastrada.

### Modo Wi-Fi

Quando já existe uma rede Wi-Fi salva, o ESP32 se conecta automaticamente a essa rede.

Após a conexão, o usuário pode acessar a página local do dispositivo, que apresenta os valores medidos e gráficos em tempo real. Essa página permite acompanhar rapidamente o comportamento da carga conectada ao medidor.

## Medição elétrica

O módulo PZEM-004T v3.0 realiza as medições do circuito AC e se comunica com o ESP32 por UART.

As grandezas medidas são:

* Tensão RMS;
* Corrente RMS;
* Potência ativa;
* Energia consumida;
* Frequência da rede;
* Fator de potência.

Esses dados são utilizados tanto na interface local quanto no envio para o sistema externo de armazenamento e visualização.

## Servidor web local

O servidor web executado no ESP32 permite visualizar os dados diretamente pelo navegador.

A interface local mostra os valores medidos e gráficos em função do tempo. Essa visualização é mais simples e voltada ao acompanhamento imediato das medições.

Também existe uma página de configuração Wi-Fi, usada quando o dispositivo está em modo Access Point.

## Comunicação MQTT

As medições feitas pelo ESP32 são enviadas via MQTT.

O MQTT permite que o dispositivo publique os dados para outros serviços do sistema, possibilitando a integração com Node-RED, banco de dados e dashboards.

Entre os dados enviados estão:

* Tensão;
* Corrente;
* Potência;
* Energia;
* Frequência;
* Fator de potência;
* Estado do relé;
* Identificação da tomada.

## Node-RED

O Node-RED atua como intermediário entre o ESP32 e o banco de dados.

Ele recebe os dados publicados via MQTT, trata as informações e organiza os parâmetros para envio ao PostgreSQL.

No projeto, o Node-RED é responsável por:

* Receber os dados do broker MQTT;
* Interpretar as mensagens recebidas;
* Validar os campos das medições;
* Enviar os dados tratados para o banco PostgreSQL.

## PostgreSQL

O PostgreSQL é utilizado para armazenar o histórico das medições.

Enquanto a página local do ESP32 mostra dados recentes, o banco permite manter registros ao longo do tempo. Isso possibilita análises posteriores do consumo energético da carga monitorada.

## Grafana

O Grafana utiliza os dados armazenados no PostgreSQL para criar dashboards mais completos.

Com ele, é possível visualizar gráficos históricos e escolher intervalos maiores de análise. Dessa forma, o usuário pode acompanhar o comportamento do consumo energético ao longo do tempo.

## Matter e controle da tomada

O projeto também possui integração com Matter, permitindo que o dispositivo seja reconhecido em uma rede de automação residencial.

Essa integração está associada ao controle de um relé, permitindo ligar e desligar a tomada monitorada. O estado da tomada também pode ser enviado ao sistema e exibido nos dashboards.

## PCB

Foi desenvolvida uma PCB própria para o projeto.

A placa substitui a montagem inicial em protoboard e organiza melhor as conexões entre ESP32, PZEM, relé e demais componentes. Essa etapa tornou a montagem mais limpa, confiável e adequada para a integração final.

## Gabinete 3D

Também foi modelado um gabinete 3D para acomodar o circuito.

O gabinete foi projetado para receber a PCB, o ESP32, o PZEM, o transformador de corrente e os cabos de entrada e saída. Ele protege o circuito e deixa o projeto mais próximo de um produto final.

## Arquitetura do sistema

O sistema é organizado em etapas integradas. Primeiro, o PZEM-004T realiza as medições elétricas da carga AC. Em seguida, o ESP32 recebe esses dados, exibe as informações na página web local e também publica as medições via MQTT.

A partir do MQTT, o Node-RED recebe os dados, trata as informações e envia os registros para o PostgreSQL. Com os dados persistidos no banco, o Grafana é utilizado para criar dashboards históricos e visualizações mais completas.

Paralelamente, o ESP32 também se comunica com o relé e com a integração Matter, permitindo o controle inteligente da tomada.

## Divisão de responsabilidades

O projeto foi desenvolvido por três integrantes: Filipe Reis, David Benech e Daniel.

### Filipe Reis

Responsável pela parte de medição, comunicação dos dados e desenvolvimento físico do circuito.

Atividades realizadas:

* Montagem do circuito de medição;
* Programação do ESP32 para leitura de tensão e corrente;
* Integração com o sensor PZEM-004T;
* Envio dos dados via MQTT;
* Integração com banco de dados e Grafana;
* Desenvolvimento da PCB;
* Participação na integração final do sistema.

### David Benech

Responsável pela interface local do ESP32 e pela modelagem física do dispositivo.

Atividades realizadas:

* Desenvolvimento do servidor web em modo Access Point;
* Página de configuração Wi-Fi;
* Página local de monitoramento;
* Gráficos dos dados no servidor do ESP32;
* Alternância entre modo AP e modo Wi-Fi;
* Modelagem da caixa 3D;
* Refinamento da interface web;
* Participação na integração final.

### Daniel

Responsável pela integração com automação residencial e controle da tomada.

Atividades realizadas:

* Integração com Matter;
* Registro do dispositivo na rede;
* Implementação dos comandos de ligar e desligar o relé;
* Envio do estado da tomada;
* Visualização do estado das tomadas no Grafana;
* Testes do sistema;
* Participação na montagem e integração final.

## Cronograma de metas

### Filipe Reis

* Meta 1: Montagem do circuito e programação do ESP32 para medir corrente e tensão.
* Meta 2: Envio dos dados por MQTT, banco de dados e Grafana.
* Meta 3: Integração com o grupo.
* Meta 4: Desenvolvimento da PCB.
* Meta 5: Integração final.

### David Benech

* Meta 1: Servidor Web AP no ESP32 com gráficos dos dados.
* Meta 2: Ajustes na página e alternância entre AP/Wi-Fi.
* Meta 3: Integração com o grupo.
* Meta 4: Caixa 3D e refino da página.
* Meta 5: Integração final.

### Daniel

* Meta 1: Conexão com Matter e comandos para ligar/desligar o relé.
* Meta 2: Envio do estado da tomada e Grafana com mapa das tomadas.
* Meta 3: Integração com o grupo.
* Meta 4: Testes e montagem final.
* Meta 5: Integração final.

## Resultado final

O projeto resultou em um sistema funcional de monitoramento e controle de energia AC.

O dispositivo é capaz de medir grandezas elétricas, exibir dados em tempo real, enviar medições por MQTT, armazenar histórico em banco de dados, gerar dashboards no Grafana e controlar a tomada por meio de relé e integração Matter.

O projeto reúne conceitos de sistemas embarcados, eletrônica, IoT, banco de dados, visualização de dados, automação residencial, PCB e modelagem 3D.
