from flask import Blueprint, request, jsonify, session
from models import User, LoginAttempt
import re
from functools import wraps

auth_bp = Blueprint('auth', __name__)

# Decorator para rotas que requerem login
def login_required(f):
    @wraps(f)
    def decorated_function(*args, **kwargs):
        if 'user_id' not in session:
            return jsonify({'error': 'Autenticação necessária'}), 401
        return f(*args, **kwargs)
    return decorated_function

def validate_email(email):
    """Valida formato de email"""
    pattern = r'^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$'
    return re.match(pattern, email) is not None

def validate_password(password):
    """Valida força da senha"""
    if len(password) < 8:
        return False, "Senha deve ter pelo menos 8 caracteres"
    if not re.search(r'[A-Z]', password):
        return False, "Senha deve conter pelo menos uma letra maiúscula"
    if not re.search(r'[a-z]', password):
        return False, "Senha deve conter pelo menos uma letra minúscula"
    if not re.search(r'[0-9]', password):
        return False, "Senha deve conter pelo menos um número"
    return True, "Senha válida"

@auth_bp.route('/register', methods=['POST'])
def register():
    data = request.get_json()
    
    # Validação dos campos obrigatórios
    required_fields = ['username', 'email', 'password']
    if not all(field in data for field in required_fields):
        return jsonify({'error': 'Campos obrigatórios: username, email, password'}), 400
    
    username = data['username'].strip()
    email = data['email'].strip().lower()
    password = data['password']
    
    # Validações
    if len(username) < 3 or len(username) > 80:
        return jsonify({'error': 'Username deve ter entre 3 e 80 caracteres'}), 400
    
    if not validate_email(email):
        return jsonify({'error': 'Email inválido'}), 400
    
    is_valid_password, password_msg = validate_password(password)
    if not is_valid_password:
        return jsonify({'error': password_msg}), 400
    
    # Verifica se usuário já existe
    if User.find_by_email(email) or User.find_by_username(username):
        return jsonify({'error': 'Usuário ou email já cadastrado'}), 409
    
    try:
        # Cria o usuário
        user = User.create(username, email, password)
        
        return jsonify({
            'message': 'Usuário criado com sucesso',
            'user': user.to_dict()
        }), 201
        
    except Exception as e:
        return jsonify({'error': 'Erro ao criar usuário'}), 500

@auth_bp.route('/login', methods=['POST'])
def login():
    data = request.get_json()
    
    # Validação dos campos obrigatórios
    if not data or 'email' not in data or 'password' not in data:
        return jsonify({'error': 'Email e senha são obrigatórios'}), 400
    
    email = data['email'].strip().lower()
    password = data['password']
    ip_address = request.remote_addr
    
    # Verifica tentativas de login (prevenção de brute force)
    failed_attempts = LoginAttempt.count_failed_attempts(email)
    if failed_attempts >= 5:
        return jsonify({'error': 'Muitas tentativas falhas. Tente novamente mais tarde.'}), 429
    
    # Busca usuário
    user = User.find_by_email(email)
    
    if not user or not user.check_password(password):
        # Registra tentativa falha
        LoginAttempt.register(email, ip_address, False)
        return jsonify({'error': 'Email ou senha inválidos'}), 401
    
    # Login bem sucedido
    LoginAttempt.register(email, ip_address, True)
    user.update_last_login()
    
    # Cria sessão
    session.clear()
    session['user_id'] = user.id
    session['username'] = user.username
    session.permanent = True  # Usa PERMANENT_SESSION_LIFETIME
    
    return jsonify({
        'message': 'Login realizado com sucesso',
        'user': user.to_dict()
    }), 200

@auth_bp.route('/logout', methods=['POST'])
def logout():
    session.clear()
    return jsonify({'message': 'Logout realizado com sucesso'}), 200

@auth_bp.route('/profile', methods=['GET'])
@login_required
def profile():
    user = User.find_by_id(session['user_id'])
    if not user:
        session.clear()
        return jsonify({'error': 'Usuário não encontrado'}), 404
    
    return jsonify({'user': user.to_dict()}), 200

@auth_bp.route('/change-password', methods=['POST'])
@login_required
def change_password():
    data = request.get_json()
    
    if not data or 'current_password' not in data or 'new_password' not in data:
        return jsonify({'error': 'Senha atual e nova senha são obrigatórias'}), 400
    
    user = User.find_by_id(session['user_id'])
    
    if not user.check_password(data['current_password']):
        return jsonify({'error': 'Senha atual incorreta'}), 401
    
    is_valid_password, password_msg = validate_password(data['new_password'])
    if not is_valid_password:
        return jsonify({'error': password_msg}), 400
    
    # Aqui você precisa implementar o método update_password no model User
    user.update_password(data['new_password'])
    
    return jsonify({'message': 'Senha alterada com sucesso'}), 200