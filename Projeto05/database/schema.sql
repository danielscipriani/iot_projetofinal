CREATE TABLE IF NOT EXISTS tomadas (
  tomada_id TEXT PRIMARY KEY,
  nome TEXT NOT NULL,
  comodo TEXT NOT NULL,
  posicao_x INTEGER NOT NULL,
  posicao_y INTEGER NOT NULL,
  criado_em TIMESTAMPTZ NOT NULL DEFAULT now(),
  atualizado_em TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS tomada_estado (
  id BIGSERIAL PRIMARY KEY,
  recebido_em TIMESTAMPTZ NOT NULL DEFAULT now(),
  tomada_id TEXT NOT NULL REFERENCES tomadas(tomada_id),
  estado TEXT NOT NULL CHECK (estado IN ('ligada', 'desligada')),
  rele BOOLEAN NOT NULL,
  payload JSONB NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_tomada_estado_recebido_em
  ON tomada_estado (recebido_em DESC);

CREATE INDEX IF NOT EXISTS idx_tomada_estado_tomada_id_recebido_em
  ON tomada_estado (tomada_id, recebido_em DESC);

CREATE OR REPLACE VIEW tomada_estado_atual AS
SELECT DISTINCT ON (te.tomada_id)
  te.tomada_id,
  t.nome,
  t.comodo,
  te.estado,
  te.rele,
  te.mac,
  te.recebido_em
FROM tomada_estado te
JOIN tomadas t ON t.tomada_id = te.tomada_id
ORDER BY te.tomada_id, te.recebido_em DESC;
