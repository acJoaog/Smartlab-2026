#!/bin/bash
set -e

# =========================================================
# PATHS
# =========================================================
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

CA_DIR="$SCRIPT_DIR/ca"
EXPORT_DIR="$ROOT_DIR/export"

MQTT_CERTS="$ROOT_DIR/mqtt/certs"
POSTGRES_CERTS="$ROOT_DIR/postgres/certs"
NGINX_CERTS="$ROOT_DIR/nginx/certs"
FLASK_CERTS="$ROOT_DIR/flask-api/certs"

DAYS_CA=3650
DAYS_CERT=365

# =========================================================
# SUBJECTS
# =========================================================
SUBJ_CA="/C=BR/ST=MG/L=SRS/O=CSILAB/CN=Smartlab CA"
SUBJ_MQTT="/C=BR/ST=MG/L=SRS/O=CSILAB/CN=192.168.66.11"
SUBJ_POSTGRES="/C=BR/ST=MG/L=SRS/O=CSILAB/CN=postgres-db"
SUBJ_NGINX="/C=BR/ST=MG/L=SRS/O=CSILAB/CN=192.168.66.11"
SUBJ_CLIENT="/C=BR/ST=MG/L=SRS/O=CSILAB/CN=iot-client"
SUBJ_CLIENT_SMARTLAB="/C=BR/ST=MG/L=SRS/O=CSILAB/CN=smartlab"

# =========================================================
# HELPERS
# =========================================================
mk() { mkdir -p "$1"; }
gen_key() { openssl genrsa -out "$1" 2048; }
gen_csr() { openssl req -new -key "$1" -out "$2" -subj "$3"; }
sign() {
  openssl x509 -req \
    -in "$1" \
    -CA "$CA_DIR/ca.crt" \
    -CAkey "$CA_DIR/ca.key" \
    -CAcreateserial \
    -out "$2" \
    -days "$DAYS_CERT" \
    -sha256
}

# =========================================================
# DIRS
# =========================================================
echo "📁 Criando diretórios..."
mk "$CA_DIR"
mk "$MQTT_CERTS"
mk "$POSTGRES_CERTS"
mk "$NGINX_CERTS"
mk "$EXPORT_DIR"
mk "$FLASK_CERTS"

# =========================================================
# CA
# =========================================================
echo "🔐 Gerando CA..."
gen_key "$CA_DIR/ca.key"
openssl req -new -x509 \
  -key "$CA_DIR/ca.key" \
  -out "$CA_DIR/ca.crt" \
  -days "$DAYS_CA" \
  -subj "$SUBJ_CA"

chmod 600 "$CA_DIR/ca.key"
chmod 644 "$CA_DIR/ca.crt"

# =========================================================
# MQTT
# =========================================================
echo "🔐 MQTT..."
gen_key "$MQTT_CERTS/server.key"
gen_csr "$MQTT_CERTS/server.key" "$MQTT_CERTS/server.csr" "$SUBJ_MQTT"
sign "$MQTT_CERTS/server.csr" "$MQTT_CERTS/server.crt"
rm "$MQTT_CERTS/server.csr"

cp "$CA_DIR/ca.crt" "$MQTT_CERTS/ca.crt"

chmod 600 "$MQTT_CERTS/server.key"
chmod 644 "$MQTT_CERTS/server.crt"
chmod 644 "$MQTT_CERTS/ca.crt"

# =========================================================
# POSTGRES
# =========================================================
echo "🔐 PostgreSQL..."
gen_key "$POSTGRES_CERTS/server.key"
gen_csr "$POSTGRES_CERTS/server.key" "$POSTGRES_CERTS/server.csr" "$SUBJ_POSTGRES"
sign "$POSTGRES_CERTS/server.csr" "$POSTGRES_CERTS/server.crt"
rm "$POSTGRES_CERTS/server.csr"

cp "$CA_DIR/ca.crt" "$POSTGRES_CERTS/ca.crt"

chmod 600 "$POSTGRES_CERTS/server.key"
chmod 644 "$POSTGRES_CERTS/server.crt"
chmod 644 "$POSTGRES_CERTS/ca.crt"

# =========================================================
# NGINX
# =========================================================
echo "🔐 NGINX ..."
gen_key "$NGINX_CERTS/server.key"
gen_csr "$NGINX_CERTS/server.key" "$NGINX_CERTS/server.csr" "$SUBJ_NGINX"
sign "$NGINX_CERTS/server.csr" "$NGINX_CERTS/server.crt"
rm "$NGINX_CERTS/server.csr"

cp "$CA_DIR/ca.crt" "$NGINX_CERTS/ca.crt"

chmod 600 "$NGINX_CERTS/server.key"
chmod 644 "$NGINX_CERTS/server.crt"
chmod 644 "$NGINX_CERTS/ca.crt"

# =========================================================
# CLIENTS (EXPORT)
# =========================================================
echo "🔐 Clients smartlab (export)..."

# client.key / client.crt
gen_key "$EXPORT_DIR/client.key"
gen_csr "$EXPORT_DIR/client.key" "$EXPORT_DIR/client.csr" "$SUBJ_CLIENT"
sign "$EXPORT_DIR/client.csr" "$EXPORT_DIR/client.crt"
rm "$EXPORT_DIR/client.csr"

# smartlab-client.key / smartlab-client.crt
gen_key "$EXPORT_DIR/smartlab-client.key"
gen_csr "$EXPORT_DIR/smartlab-client.key" "$EXPORT_DIR/smartlab-client.csr" "$SUBJ_CLIENT_SMARTLAB"
sign "$EXPORT_DIR/smartlab-client.csr" "$EXPORT_DIR/smartlab-client.crt"
rm "$EXPORT_DIR/smartlab-client.csr"

cp "$EXPORT_DIR/smartlab-client.crt" "$FLASK_CERTS/client.crt"
cp "$EXPORT_DIR/smartlab-client.key" "$FLASK_CERTS/client.key"

cp "$CA_DIR/ca.crt" "$EXPORT_DIR/ca.crt"

chmod 600 "$EXPORT_DIR"/*.key
chmod 644 "$EXPORT_DIR"/*.crt

echo "✅ Certificados gerados com sucesso"
