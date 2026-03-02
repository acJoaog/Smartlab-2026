FROM eclipse-mosquitto:2.0

# Copiar configuração
COPY mosquitto.conf /mosquitto/config/mosquitto.conf

# Copiar certificados
COPY  /certs/ /mosquitto/certs/

# Copiar arquivo de senhas
COPY mosquitto.passwd /mosquitto/config/credentials

# Criar diretórios para dados e logs
RUN mkdir -p /mosquitto/data /mosquitto/log

# Ajustar permissões
RUN chown -R mosquitto:mosquitto /mosquitto 

EXPOSE 8883 9001

CMD ["/usr/sbin/mosquitto", "-c", "/mosquitto/config/mosquitto.conf"]