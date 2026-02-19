from flask import Blueprint, request, jsonify
from flask_jwt_extended import create_access_token, create_refresh_token, jwt_required, get_jwt_identity, get_jwt
from models import User, LoginAttempt
import re
from datetime import timedelta
import os
import logging

# Configurar logging
logger = logging.getLogger(__name__)

# PRIMEIRO: Criar o blueprint
auth_bp = Blueprint('auth', __name__)

def validate_email(email):
    pattern = r'^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$'
    return re.match(pattern, email) is not None

def validate_password(password):
    if len(password) < 8:
        return False, "Senha deve ter pelo menos 8 caracteres"
    if not re.search(r'[A-Z]', password):
        return False, "Senha deve conter pelo menos uma letra maiúscula"
    if not re.search(r'[a-z]', password):
        return False, "Senha deve conter pelo menos uma letra minúscula"
    if not re.search(r'[0-9]', password):
        return False, "Senha deve conter pelo menos um número"
    return True, "Senha válida"

# DEPOIS: Usar o blueprint para definir as rotas
@auth_bp.route('/register', methods=['POST'])
def register():
    data = request.get_json()
    
    required_fields = ['username', 'email', 'password']
    if not all(field in data for field in required_fields):
        return jsonify({'error': 'Campos obrigatórios: username, email, password'}), 400
    
    username = data['username'].strip()
    email = data['email'].strip().lower()
    password = data['password']
    
    if len(username) < 3 or len(username) > 80:
        return jsonify({'error': 'Username deve ter entre 3 e 80 caracteres'}), 400
    
    if not validate_email(email):
        return jsonify({'error': 'Email inválido'}), 400
    
    is_valid_password, password_msg = validate_password(password)
    if not is_valid_password:
        return jsonify({'error': password_msg}), 400
    
    if User.find_by_email(email) or User.find_by_username(username):
        return jsonify({'error': 'Usuário ou email já cadastrado'}), 409
    
    try:
        user = User.create(username, email, password)
        return jsonify({
            'message': 'Usuário criado com sucesso',
            'user': user.to_dict()
        }), 201
    except Exception as e:
        logger.error(f"Erro no registro: {e}")
        return jsonify({'error': 'Erro ao criar usuário'}), 500

@auth_bp.route('/login', methods=['POST'])
def login():
    data = request.get_json()
    
    if not data or 'email' not in data or 'password' not in data:
        return jsonify({'error': 'Email e senha são obrigatórios'}), 400
    
    email = data['email'].strip().lower()
    password = data['password']
    ip_address = request.remote_addr
    
    # Prevenção de brute force
    failed_attempts = LoginAttempt.count_failed_attempts(email)
    if failed_attempts >= 5:
        return jsonify({'error': 'Muitas tentativas falhas. Tente novamente mais tarde.'}), 429
    
    user = User.find_by_email(email)
    
    if not user or not user.check_password(password):
        LoginAttempt.register(email, ip_address, False)
        return jsonify({'error': 'Email ou senha inválidos'}), 401
    
    LoginAttempt.register(email, ip_address, True)
    user.update_last_login()
    
    # Log para debug
    logger.info(f"Login bem-sucedido para user {user.id}")
    
    # Criar token JWT
    access_token = create_access_token(
        identity=user.id,
        additional_claims={
            'username': user.username,
            'email': user.email
        },
        expires_delta=timedelta(minutes=15)
    )
    
    refresh_token = create_refresh_token(identity=user.id)
    
    return jsonify({
        'message': 'Login realizado com sucesso',
        'access_token': access_token,
        'refresh_token': refresh_token,
        'token_type': 'Bearer',
        'expires_in': 900,
        'user': user.to_dict()
    }), 200

@auth_bp.route('/refresh', methods=['POST'])
@jwt_required(refresh=True)
def refresh():
    """Endpoint para renovar o access token usando refresh token"""
    current_user_id = get_jwt_identity()
    new_access_token = create_access_token(identity=current_user_id)
    return jsonify({'access_token': new_access_token}), 200

@auth_bp.route('/logout', methods=['POST'])
@jwt_required()
def logout():
    """Para JWT, o logout é feito no cliente (descartar o token)"""
    return jsonify({'message': 'Logout realizado com sucesso. Descarte o token.'}), 200

@auth_bp.route('/profile', methods=['GET'])
@jwt_required()
def profile():
    current_user_id = get_jwt_identity()
    claims = get_jwt()
    
    user = User.find_by_id(current_user_id)
    if not user:
        return jsonify({'error': 'Usuário não encontrado'}), 404
    
    return jsonify({
        'user': user.to_dict(),
        'token_claims': {
            'username': claims.get('username'),
            'email': claims.get('email')
        }
    }), 200

@auth_bp.route('/change-password', methods=['POST'])
@jwt_required()
def change_password():
    data = request.get_json()
    
    if not data or 'current_password' not in data or 'new_password' not in data:
        return jsonify({'error': 'Senha atual e nova senha são obrigatórias'}), 400
    
    current_user_id = get_jwt_identity()
    user = User.find_by_id(current_user_id)
    
    if not user.check_password(data['current_password']):
        return jsonify({'error': 'Senha atual incorreta'}), 401
    
    is_valid_password, password_msg = validate_password(data['new_password'])
    if not is_valid_password:
        return jsonify({'error': password_msg}), 400
    
    user.update_password(data['new_password'])
    
    return jsonify({'message': 'Senha alterada com sucesso'}), 200