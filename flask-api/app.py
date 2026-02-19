from flask import Flask, jsonify, session, request
import jwt as pyjwt  # Import para debug
from flask_jwt_extended import JWTManager, jwt_required, get_jwt_identity, get_jwt
from config import Config
from models import Database
from auth_jwt import auth_bp  # Importe a nova versão com JWT
import os

app = Flask(__name__)
app.config.from_object(Config)

# Configurações adicionais do JWT
app.config['JWT_SECRET_KEY'] = os.getenv('JWT_SECRET_KEY', 'fallback-secret-key')
app.config['JWT_ACCESS_TOKEN_EXPIRES'] = int(os.getenv('JWT_ACCESS_TOKEN_EXPIRES', 900))
app.config['JWT_TOKEN_LOCATION'] = ['headers']  # APENAS headers
app.config['JWT_HEADER_NAME'] = 'Authorization'
app.config['JWT_HEADER_TYPE'] = 'Bearer'
app.config['JWT_IDENTITY_CLAIM'] = 'sub'  # Claim que identifica o usuário
app.config['JWT_CSRF_PROTECT'] = False  # Desabilita CSRF para API
app.config['JWT_VERIFY_SUB'] = False  # Desabilita verificação do subject

# Inicializa o JWT
jwt = JWTManager(app)

# Registra o blueprint
app.register_blueprint(auth_bp, url_prefix='/auth')

# Callbacks para melhor tratamento de erros JWT
@jwt.expired_token_loader
def expired_token_callback(jwt_header, jwt_payload):
    return jsonify({
        'error': 'token_expired',
        'message': 'O token expirou. Faça login novamente.'
    }), 401

@jwt.invalid_token_loader
def invalid_token_callback(error):
    return jsonify({
        'error': 'invalid_token',
        'message': 'Token inválido.'
    }), 401

@jwt.unauthorized_loader
def missing_token_callback(error):
    return jsonify({
        'error': 'authorization_required',
        'message': 'Token de acesso é obrigatório.'
    }), 401

@app.before_request
def before_request():
    """Inicializa o banco de dados antes da primeira requisição"""
    if not hasattr(app, 'db_initialized'):
        Database.init_db()
        app.db_initialized = True

@app.route("/")
def health():
    return {"status": "ok", "message": "API funcionando"}

@app.route("/db-check")
def db_check():
    try:
        with Database.get_connection() as conn:
            with conn.cursor() as cur:
                # Verifica SSL
                cur.execute("SHOW ssl;")
                ssl_on = cur.fetchone()[0]
                
                # Cipher
                cipher = None
                try:
                    cur.execute("SELECT ssl_cipher();")
                    result = cur.fetchone()
                    if result:
                        cipher = result[0]
                except:
                    cipher = "não disponível"
                
                # Versão SSL
                ssl_version = None
                try:
                    cur.execute("SELECT ssl_version();")
                    result = cur.fetchone()
                    if result:
                        ssl_version = result[0]
                except:
                    ssl_version = "não disponível"
                
                # Contagem de usuários
                cur.execute("SELECT COUNT(*) FROM users;")
                user_count = cur.fetchone()[0]

        return {
            "status": "connected",
            "ssl": ssl_on,
            "cipher": cipher,
            "ssl_version": ssl_version,
            "user_count": user_count
        }
    except Exception as e:
        return {
            "status": "error",
            "message": str(e)
        }, 500

# Rota protegida de exemplo
@app.route("/dashboard")
@jwt_required()
def dashboard():
    current_user_id = get_jwt_identity()
    claims = get_jwt()
    return jsonify({
        "message": "Bem-vindo ao dashboard!",
        "user_id": current_user_id,
        "username": claims.get('username')
    })