#!/bin/bash

echo "=== Corrigindo permissões dos certificados ==="
if [ -d "/etc/postgresql/certs" ]; then
    chown -R postgres:postgres /etc/postgresql/certs
    chmod 700 /etc/postgresql/certs
    
    if [ -f "/etc/postgresql/certs/server.key" ]; then
        chmod 600 /etc/postgresql/certs/server.key
    fi
    
    if [ -f "/etc/postgresql/certs/server.crt" ]; then
        chmod 644 /etc/postgresql/certs/server.crt
    fi
    
    if [ -f "/etc/postgresql/certs/ca.crt" ]; then
        chmod 644 /etc/postgresql/certs/ca.crt
    fi
fi

set -e

echo "=== Inicializando banco de dados SmartLab ==="

# Criar usuário de aplicação
psql -v ON_ERROR_STOP=1 --username "$POSTGRES_USER" --dbname "$POSTGRES_DB" <<-EOSQL
    CREATE TABLE IF NOT EXISTS users (
        id SERIAL PRIMARY KEY,
        username VARCHAR(80) UNIQUE NOT NULL,
        email VARCHAR(120) UNIQUE NOT NULL,
        password_hash VARCHAR(200) NOT NULL,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        last_login TIMESTAMP,
        is_active BOOLEAN DEFAULT TRUE
    );

    -- Criar usuário smartlab
    DO \$\$
    BEGIN
        IF NOT EXISTS (SELECT FROM pg_catalog.pg_user WHERE usename = 'smartlab') THEN
            CREATE USER smartlab WITH PASSWORD 'admin';
        END IF;
    END
    \$\$;
    
    GRANT CONNECT ON DATABASE smartlab_db TO smartlab;
    GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO smartlab;
    GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO smartlab;
    GRANT USAGE ON SCHEMA public TO smartlab;

    -- Criar usuário admin padrão
    INSERT INTO users (username, email, password_hash) 
    VALUES ('admin', 'admin@iot.com', 'admin123_hash')
    ON CONFLICT (username) DO NOTHING;
EOSQL

echo "=== Banco de dados inicializado com sucesso! ==="