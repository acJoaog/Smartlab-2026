#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

CA_DIR="$SCRIPT_DIR/ca"
EXPORT_DIR="$ROOT_DIR/export"

MQTT_CERTS="$ROOT_DIR/mqtt/certs"
POSTGRES_CERTS="$ROOT_DIR/postgres/certs"
NGINX_CERTS="$ROOT_DIR/nginx/certs"
FLASK_CERTS="$ROOT_DIR/flask-api/certs"

DAYS_CA=3650
DAYS_CERT=825

IP_BROKER="192.168.66.11"

mk() { mkdir -p "$1"; }

gen_key() { openssl genrsa -out "$1" 2048; }

# =========================================================
# EXT FILE HELPERS
# =========================================================

server_ext() {
cat > "$1" <<EOF
authorityKeyIdentifier=keyid,issuer
basicConstraints=CA:FALSE
keyUsage=digitalSignature,keyEncipherment
extendedKeyUsage=serverAuth
subjectAltName=@alt_names

[alt_names]
IP.1=${IP_BROKER}
DNS.1=localhost
EOF
}

client_ext() {
cat > "$1" <<EOF
authorityKeyIdentifier=keyid,issuer
basicConstraints=CA:FALSE
keyUsage=digitalSignature
extendedKeyUsage=clientAuth
EOF
}

mqtt_server_ext() {
cat > "$1" <<EOF
authorityKeyIdentifier=keyid,issuer
basicConstraints=CA:FALSE
keyUsage=digitalSignature,keyEncipherment
extendedKeyUsage=serverAuth
subjectAltName=IP:${IP_BROKER}
EOF
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

openssl req -x509 -new -nodes \
  -key "$CA_DIR/ca.key" \
  -sha256 \
  -days $DAYS_CA \
  -subj "/C=BR/ST=MG/L=SRS/O=CSILAB/CN=Smartlab CA" \
  -out "$CA_DIR/ca.crt"

chmod 600 "$CA_DIR/ca.key"
chmod 644 "$CA_DIR/ca.crt"

# =========================================================
# SERVER CERT GENERATOR
# =========================================================

gen_server_cert() {
  NAME=$1
  CERT_DIR=$2

  echo "🔐 $NAME..."

  gen_key "$CERT_DIR/server.key"

  openssl req -new \
    -key "$CERT_DIR/server.key" \
    -subj "/C=BR/ST=MG/L=SRS/O=CSILAB/CN=${IP_BROKER}" \
    -out "$CERT_DIR/server.csr"

  if [ "$NAME" = "MQTT" ]; then
    mqtt_server_ext "$CERT_DIR/ext.cnf"
  else
    server_ext "$CERT_DIR/ext.cnf"
  fi

  openssl x509 -req \
    -in "$CERT_DIR/server.csr" \
    -CA "$CA_DIR/ca.crt" \
    -CAkey "$CA_DIR/ca.key" \
    -CAcreateserial \
    -out "$CERT_DIR/server.crt" \
    -days $DAYS_CERT \
    -sha256 \
    -extfile "$CERT_DIR/ext.cnf"

  rm "$CERT_DIR/server.csr"
  rm "$CERT_DIR/ext.cnf"

  cp "$CA_DIR/ca.crt" "$CERT_DIR/ca.crt"

  chmod 600 "$CERT_DIR/server.key"
  chmod 644 "$CERT_DIR/server.crt"
  chmod 644 "$CERT_DIR/ca.crt"
}

# =========================================================
# CLIENT CERT GENERATOR
# =========================================================

gen_client_cert() {
  NAME=$1

  echo "🔐 Client $NAME..."

  gen_key "$EXPORT_DIR/${NAME}.key"

  openssl req -new \
    -key "$EXPORT_DIR/${NAME}.key" \
    -subj "/C=BR/ST=MG/L=SRS/O=CSILAB/CN=${NAME}" \
    -out "$EXPORT_DIR/${NAME}.csr"

  client_ext "$EXPORT_DIR/ext.cnf"

  openssl x509 -req \
    -in "$EXPORT_DIR/${NAME}.csr" \
    -CA "$CA_DIR/ca.crt" \
    -CAkey "$CA_DIR/ca.key" \
    -CAcreateserial \
    -out "$EXPORT_DIR/${NAME}.crt" \
    -days $DAYS_CERT \
    -sha256 \
    -extfile "$EXPORT_DIR/ext.cnf"

  rm "$EXPORT_DIR/${NAME}.csr"
  rm "$EXPORT_DIR/ext.cnf"

  chmod 600 "$EXPORT_DIR/${NAME}.key"
  chmod 644 "$EXPORT_DIR/${NAME}.crt"
}

# =========================================================
# GENERATE SERVERS
# =========================================================

gen_server_cert "MQTT" "$MQTT_CERTS"
gen_server_cert "PostgreSQL" "$POSTGRES_CERTS"
gen_server_cert "NGINX" "$NGINX_CERTS"

# =========================================================
# GENERATE CLIENTS
# =========================================================

gen_client_cert "iot-client"
gen_client_cert "smartlab"

cp "$EXPORT_DIR/smartlab.crt" "$FLASK_CERTS/client.crt"
cp "$EXPORT_DIR/smartlab.key" "$FLASK_CERTS/client.key"
cp "$CA_DIR/ca.crt" "$FLASK_CERTS/ca.crt"

cp "$CA_DIR/ca.crt" "$EXPORT_DIR/ca.crt"

echo "✅ Certificados gerados com SAN + EKU corretamente"