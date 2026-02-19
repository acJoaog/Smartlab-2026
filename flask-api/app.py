from flask import Flask, jsonify, session  # Adicione session aqui
from config import Config
from models import Database
from auth import auth_bp, login_required
import os

app = Flask(__name__)
app.config.from_object(Config)

# Registra o blueprint de autenticação
app.register_blueprint(auth_bp, url_prefix='/auth')

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
@login_required
def dashboard():
    return jsonify({
        "message": "Bem-vindo ao dashboard!",
        "user": session.get('username')  # Agora session está importado
    })
