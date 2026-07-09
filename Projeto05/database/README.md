# Fluxo ESP32 -> MQTT -> PostgreSQL -> Grafana

Este projeto usa o MQTT como transporte dos estados da tomada e o PostgreSQL como base que o Grafana consulta.

## Arquivos

- `schema.sql`: cria as tabelas e a view usadas pelo Grafana.
- `../src/mqtt_para_postgres.py`: assina o MQTT e salva os estados no PostgreSQL.
- `../src/publicar_teste_mqtt.py`: publica uma mensagem de teste, simulando o ESP32.
- `../src/requirements.txt`: dependencias Python.

## 1. Instalar dependencias

Execute dentro da pasta `Projeto05/src`:

```bash
pip install -r requirements.txt
```

## 2. Rodar a ponte MQTT -> PostgreSQL

```bash
python mqtt_para_postgres.py
```

Ela cria o schema automaticamente usando `database/schema.sql`, assina:

```txt
casa/tomadas/+/estado
```

e salva cada mensagem recebida.

## 3. Publicar um teste sem ESP32

Em outro terminal, ainda em `Projeto05/src`:

```bash
python publicar_teste_mqtt.py ligada
python publicar_teste_mqtt.py desligada
```

## 4. Consulta no Grafana: estado atual

Use a view:

```sql
SELECT
  tomada_id,
  nome,
  comodo,
  estado,
  rele,
  posicao_x,
  posicao_y,
  recebido_em
FROM tomada_estado_atual;
```

## 5. Consulta no Grafana: historico

```sql
SELECT
  recebido_em AS time,
  CASE WHEN rele THEN 1 ELSE 0 END AS estado
FROM tomada_estado
WHERE tomada_id = 'tomada_sala_01'
ORDER BY recebido_em;
```
