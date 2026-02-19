import psycopg
from werkzeug.security import generate_password_hash, check_password_hash
from config import Config

class Database:
    @staticmethod
    def get_connection():
        return psycopg.connect(**Config.DB_CONFIG)
    
    @staticmethod
    def init_db():
        """Cria as tabelas necessárias se não existirem"""
        with Database.get_connection() as conn:
            with conn.cursor() as cur:
                # Tabela de usuários
                cur.execute("""
                    CREATE TABLE IF NOT EXISTS users (
                        id SERIAL PRIMARY KEY,
                        username VARCHAR(80) UNIQUE NOT NULL,
                        email VARCHAR(120) UNIQUE NOT NULL,
                        password_hash VARCHAR(200) NOT NULL,
                        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                        last_login TIMESTAMP,
                        is_active BOOLEAN DEFAULT TRUE
                    )
                """)
                
                # Índices para melhor performance
                cur.execute("CREATE INDEX IF NOT EXISTS idx_users_email ON users(email)")
                cur.execute("CREATE INDEX IF NOT EXISTS idx_users_username ON users(username)")
                
                # Tabela para gerenciar tentativas de login (prevenção de brute force)
                cur.execute("""
                    CREATE TABLE IF NOT EXISTS login_attempts (
                        id SERIAL PRIMARY KEY,
                        email VARCHAR(120),
                        ip_address VARCHAR(45),
                        attempt_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                        success BOOLEAN
                    )
                """)
                
                conn.commit()

class User:
    def __init__(self, id, username, email, password_hash):
        self.id = id
        self.username = username
        self.email = email
        self.password_hash = password_hash
    
    @staticmethod
    def create(username, email, password):
        """Cria um novo usuário"""
        password_hash = generate_password_hash(password)
        
        with Database.get_connection() as conn:
            with conn.cursor() as cur:
                cur.execute("""
                    INSERT INTO users (username, email, password_hash)
                    VALUES (%s, %s, %s)
                    RETURNING id, username, email, password_hash
                """, (username, email, password_hash))
                
                user_data = cur.fetchone()
                conn.commit()
                
                return User(*user_data)
    
    @staticmethod
    def find_by_email(email):
        """Busca usuário por email"""
        with Database.get_connection() as conn:
            with conn.cursor() as cur:
                cur.execute("""
                    SELECT id, username, email, password_hash 
                    FROM users 
                    WHERE email = %s AND is_active = TRUE
                """, (email,))
                
                user_data = cur.fetchone()
                
                if user_data:
                    return User(*user_data)
                return None
    
    @staticmethod
    def find_by_username(username):
        """Busca usuário por username"""
        with Database.get_connection() as conn:
            with conn.cursor() as cur:
                cur.execute("""
                    SELECT id, username, email, password_hash 
                    FROM users 
                    WHERE username = %s AND is_active = TRUE
                """, (username,))
                
                user_data = cur.fetchone()
                
                if user_data:
                    return User(*user_data)
                return None
    
    @staticmethod
    def find_by_id(user_id):
        """Busca usuário por ID"""
        with Database.get_connection() as conn:
            with conn.cursor() as cur:
                cur.execute("""
                    SELECT id, username, email, password_hash, created_at
                    FROM users 
                    WHERE id = %s AND is_active = TRUE
                """, (user_id,))
                
                user_data = cur.fetchone()
                
                if user_data:
                    user = User(user_data[0], user_data[1], user_data[2], user_data[3])
                    user.created_at = user_data[4]
                    return user
                return None
    
    def check_password(self, password):
        """Verifica se a senha está correta"""
        return check_password_hash(self.password_hash, password)
    
    def update_password(self, new_password):
        """Atualiza a senha do usuário"""
        new_password_hash = generate_password_hash(new_password)
        
        with Database.get_connection() as conn:
            with conn.cursor() as cur:
                cur.execute("""
                    UPDATE users 
                    SET password_hash = %s 
                    WHERE id = %s
                """, (new_password_hash, self.id))
                conn.commit()
        
        self.password_hash = new_password_hash
    
    def update_last_login(self):
        """Atualiza timestamp do último login"""
        with Database.get_connection() as conn:
            with conn.cursor() as cur:
                cur.execute("""
                    UPDATE users 
                    SET last_login = CURRENT_TIMESTAMP 
                    WHERE id = %s
                """, (self.id,))
                conn.commit()
    
    def to_dict(self):
        """Retorna representação do usuário sem dados sensíveis"""
        return {
            'id': self.id,
            'username': self.username,
            'email': self.email,
            'created_at': self.created_at.isoformat() if hasattr(self, 'created_at') and self.created_at else None
        }

class LoginAttempt:
    @staticmethod
    def register(email, ip_address, success):
        """Registra uma tentativa de login"""
        with Database.get_connection() as conn:
            with conn.cursor() as cur:
                cur.execute("""
                    INSERT INTO login_attempts (email, ip_address, success)
                    VALUES (%s, %s, %s)
                """, (email, ip_address, success))
                conn.commit()
    
    @staticmethod
    def count_failed_attempts(email, minutes=15):
        """Conta tentativas falhas nos últimos X minutos"""
        with Database.get_connection() as conn:
            with conn.cursor() as cur:
                cur.execute("""
                    SELECT COUNT(*) FROM login_attempts
                    WHERE email = %s 
                    AND success = FALSE
                    AND attempt_time > (CURRENT_TIMESTAMP - INTERVAL '%s minutes')
                """, (email, minutes))
                
                return cur.fetchone()[0]