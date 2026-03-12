# Smartlab 2026 (CSILab)

Uma plataforma de infraestrutura IoT segura para gerenciamento de dispositivos inteligentes em laboratórios. Fornece um sistema backend com autenticação, mensageria em tempo real via MQTT e armazenamento persistente de dados com segurança de nível empresarial usando TLS/SSL em todas as camadas de comunicação.

## 🚀 Visão Geral

O Smartlab 2026 é uma plataforma IoT completa que integra:
- **API REST segura** com Flask e autenticação JWT
- **Broker MQTT** com suporte a TLS para comunicação em tempo real
- **Banco de dados PostgreSQL** com criptografia SSL
- **Proxy reverso Nginx** com terminação TLS/SSL
- **Containerização completa** com Docker e Docker Compose

## 🛠️ Tecnologias Utilizadas

- **Backend:** Flask 2.3.3, Python 3.12, Gunicorn
- **Autenticação:** Flask-JWT-Extended, JWT com expiração de 15 minutos
- **Banco de Dados:** PostgreSQL 15 com SSL obrigatório
- **Mensageria:** Eclipse Mosquitto 2.0 com TLS
- **Proxy:** Nginx com terminação SSL
- **Infraestrutura:** Docker & Docker Compose

## 🏗️ Arquitetura

```
┌─────────────────────────────────────────────────────────────┐
│                    nginx (Reverse Proxy)                    │
│                    TLS/SSL Termination                       │
│                 Port 80 → 443 (HTTP Redirect)                │
└──────────┬──────────────────────────────────────────────────┘
           │
           ├─────────────────────────────────────────┐
           │                                         │
    ┌──────▼──────┐                          ┌──────▼──────┐
    │ Flask API   │                          │   MQTT      │
    │ Port: 5000  │                          │ Port: 8883  │
    │  (Internal) │                          │ (TLS/MQTT)  │
    │             │ (Client Certs)           │ Port: 9001  │
    │ - Auth      │◄──────────────────────────► (WebSocket) │
    │ - JWT       │   SSL Connection         │             │
    │ - Users     │                          │ Eclipse     │
    │ - Dashboard │                          │ Mosquitto   │
    └──────┬──────┘                          └─────────────┘
           │
    ┌──────▼─────────────────┐
    │   PostgreSQL 15        │
    │   Port: 5432           │
    │   (TLS Connection)     │
    │                        │
    │ - users table          │
    │ - login_attempts       │
    │ - encrypted data       │
    └────────────────────────┘
```

## 🔒 Recursos de Segurança

- **Criptografia End-to-End TLS/SSL** em todos os serviços
- **Autenticação JWT** com tokens de acesso e refresh
- **Proteção contra força bruta** com limite de tentativas de login
- **Requisitos de senha** fortes (8+ caracteres, maiúscula, minúscula, número)
- **Autenticação baseada em certificados** entre serviços
- **Modo SSL obrigatório** no PostgreSQL

## 📋 Pré-requisitos

- Docker & Docker Compose instalados
- Ambiente Linux/UNIX (ou WSL no Windows)
- Acesso root/sudo para geração de certificados
- OpenSSL disponível

## 🚀 Instalação e Configuração

### 1. Clonar o Repositório
```bash
git clone https://github.com/acJoaog/csilab_2026.git
cd csilab_2026
```

### 2. Gerar Certificados TLS
```bash
cd scripts
chmod +x *.sh
sudo ./generate-certs.sh
```

Este script gera:
- Autoridade Certificadora (CA)
- Certificados TLS para MQTT, PostgreSQL, Nginx e Flask API
- Validade de 3650 dias para CA, 825 dias para certificados de serviço
- SANs configurados para IP 192.168.66.11 e localhost

### 3. Implantar Serviços
```bash
sudo ./deploy.sh
```

Este script:
- Para containers em execução
- Reconstrói todas as imagens
- Inicia todos os serviços em modo detached

### 4. Verificar Implantação
```bash
docker ps  # Verificar containers em execução
docker logs smartlab-api  # Verificar logs da API Flask
docker logs smartlab-postgres  # Verificar logs do PostgreSQL
```

## 📖 Uso

### Endpoints da API

**Base URL:** `https://localhost` (via proxy reverso Nginx)

#### Autenticação (`/auth`)

| Método | Endpoint | Descrição | Autenticação |
|--------|----------|-----------|--------------|
| POST | `/auth/register` | Registrar novo usuário | Não |
| POST | `/auth/login` | Login do usuário, retorna tokens JWT | Não |
| POST | `/auth/refresh` | Renovar token de acesso | Sim (Refresh JWT) |
| POST | `/auth/logout` | Logout (descartar token no cliente) | Sim |
| GET | `/auth/profile` | Obter perfil do usuário autenticado | Sim |
| POST | `/auth/change-password` | Alterar senha do usuário | Sim |

#### Sistema

| Método | Endpoint | Descrição | Autenticação |
|--------|----------|-----------|--------------|
| GET | `/` | Verificação de saúde | Não |
| GET | `/db-check` | Verificação de conexão e SSL do banco | Não |
| GET | `/dashboard` | Dashboard de exemplo protegido | Sim |

### Exemplos de Uso

#### Registro de Usuário
```bash
curl -X POST https://localhost/auth/register \
  -H "Content-Type: application/json" \
  -d '{
    "username": "john_doe",
    "email": "john@example.com",
    "password": "SecurePass123"
  }' --insecure
```

#### Login
```bash
curl -X POST https://localhost/auth/login \
  -H "Content-Type: application/json" \
  -d '{
    "email": "john@example.com",
    "password": "SecurePass123"
  }' --insecure
```

Armazene o `access_token` retornado.

#### Acessar Endpoint Protegido
```bash
curl -X GET https://localhost/dashboard \
  -H "Authorization: Bearer {access_token}" \
  --insecure
```

#### Verificar Conexão do Banco e SSL
```bash
curl https://localhost/db-check --insecure
```

#### Conexão MQTT
```bash
# Instalar mqtt-cli ou mosquitto-clients
mqtt sub -h 192.168.66.11 -p 8883 -t "lab/#" --cafile mqtt/certs/ca.crt

# Publicar mensagem
mqtt pub -h 192.168.66.11 -p 8883 -t "lab/sensor1" -m '{"temp": 25.5}' --cafile mqtt/certs/ca.crt
```

## ⚙️ Configuração

### Flask ([config.py](flask-api/config.py))
- Chave secreta JWT (configurável via ambiente)
- Expiração do token de acesso: 900 segundos (15 minutos)
- Modo SSL do banco: Obrigatório
- Localização do token: Headers de Authorization apenas

### Nginx ([nginx/nginx.conf](nginx/nginx.conf))
- Porta 80 redireciona para 443
- TLS 1.2 e 1.3 habilitados
- Proxy para API Flask em `http://flask-api:5000`

### MQTT ([mqtt/mosquitto.conf](mqtt/mosquitto.conf))
- Persistência habilitada
- Porta 8883 para MQTT sobre TLS
- Porta 9001 para WebSocket sobre TLS
- Máximo de conexões: 100
- Acesso anônimo desabilitado

### PostgreSQL
- Usuário padrão: `smartlab`
- Senha padrão: `admin` (alterar em produção)
- Banco: `smartlab_db`
- Porta: 5432
- Modo SSL obrigatório

## 🚀 Implantação em Produção

1. **Atualizar Configuração de Certificados:**
   - Modificar variável `IP_BROKER` em [scripts/generate-certs.sh](scripts/generate-certs.sh)
   - Atualizar SANs conforme necessário

2. **Proteger Variáveis de Ambiente:**
   - Definir valores de produção para `DB_PASSWORD`, `JWT_SECRET_KEY`
   - Usar Docker Secrets ou gerenciamento externo de segredos

3. **Gerenciamento de Certificados:**
   ```bash
   sudo ./scripts/generate-certs.sh  # Gera/renova certificados
   ```

4. **Implantar Infraestrutura:**
   ```bash
   cd scripts
   sudo ./deploy.sh
   ```

5. **Verificar Serviços:**
   ```bash
   # Verificar saúde
   curl https://your-domain/
   
   # Verificar banco
   curl https://your-domain/db-check
   
   # Verificar logs
   docker logs smartlab-api
   docker logs smartlab-postgres
   docker logs smartlab-mqtt-broker
   ```

## 🧪 Desenvolvimento e Testes

### Desenvolvimento Local
- Use `--insecure` com curl para ignorar certificados auto-assinados
- Monitore logs: `docker logs -f {nome_container}`
- Acesso direto ao banco: `psql -h localhost -U smartlab -d smartlab_db`

### Testes de Endpoints
- Verificação de saúde em `/` (sem autenticação)
- Verificação de banco em `/db-check` mostra cipher SSL e versão
- Todos os endpoints de auth têm validação de entrada e mensagens de erro em português

### Credenciais Padrão (Desenvolvimento)
- PostgreSQL: `smartlab` / `admin`
- MQTT: Configurado via arquivo credentials
- API admin padrão: `admin@iot.com`

## 📁 Estrutura do Projeto

```
csilab_2026/
├── docker-compose.yml          # Orquestração de serviços
├── flask-api/                  # API Flask
│   ├── app.py                  # Aplicação principal
│   ├── auth_jwt.py             # Blueprint de autenticação
│   ├── config.py               # Configurações
│   ├── models.py               # Modelos do banco
│   ├── requirements.txt        # Dependências Python
│   ├── Dockerfile              # Containerização da API
│   └── ssl-config.sh           # Configuração SSL
├── mqtt/                       # Broker MQTT
│   ├── Dockerfile              # Containerização MQTT
│   ├── mosquitto.conf          # Configuração Mosquitto
│   └── credentials             # Credenciais MQTT
├── nginx/                      # Proxy reverso
│   └── nginx.conf              # Configuração Nginx
├── postgres/                   # Banco de dados
│   ├── Dockerfile              # Containerização PostgreSQL
│   ├── init-db.sh              # Inicialização do banco
│   ├── setup-ssl.sh            # Configuração SSL
│   └── conf/                   # Configurações PostgreSQL
├── scripts/                    # Scripts de automação
│   ├── deploy.sh               # Script de implantação
│   └── generate-certs.sh       # Geração de certificados
└── README.md                   # Este arquivo
```

## 🤝 Contribuição

1. Fork o projeto
2. Crie uma branch para sua feature (`git checkout -b feature/AmazingFeature`)
3. Commit suas mudanças (`git commit -m 'Add some AmazingFeature'`)
4. Push para a branch (`git push origin feature/AmazingFeature`)
5. Abra um Pull Request

## 📄 Licença

Este projeto está sob a licença MIT. Veja o arquivo `LICENSE` para mais detalhes.

## ⚠️ Considerações Adicionais

- **CORS:** Adicionar configuração CORS no Flask se clientes frontend precisarem acessar a API
- **Logs:** Logs básicos; recomendado logging estruturado para produção
- **Monitoramento:** Considerar Prometheus/Grafana para monitoramento
- **Backup:** Configurar estratégia de backup do PostgreSQL
- **Renovação SSL:** Automatizar renovação de certificados antes da expiração (825 dias)
- **Gerenciamento de Segredos:** Usar Docker Secrets ou cofres externos para dados sensíveis
- **Limitação de Taxa:** Implementar limitação global de taxa junto com proteção contra força bruta

---

**Nota:** Esta é uma plataforma IoT pronta para produção com segurança de nível empresarial, arquitetura escalável containerizada e mecanismos abrangentes de autenticação/autorização.
