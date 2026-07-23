// ============================================================
// CORTINA AUTOMATIZADA - Refatorado
// Comandos MQTT: 3=subir | 2=descer | 4=parar (manual ou fim de curso)
// ============================================================

#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <Ultrasonic.h>

// --- Rede WiFi ---
const char* ssid     = "CSI-Lab";
const char* password = "In@teLCS&I";

// --- Broker MQTT ---
const char* mqttServer   = "192.168.66.11";
const char* mqttUser     = "smartlab";
const char* mqttPassword = "WhoAmI#2024";
const int   mqttPort     = 8883;
const char* ID           = "GECO_CORTINA";

const char* TOPIC_SUB    = "smartlab/cortina/lab1/1";
const char* TOPIC_STATUS = "GECO/estado";

// --- Pinos corrigidos ---
// Motor
#define IN1  32   // era 22 (I2C SCL)
#define IN2  33   // era 23 (I2C SDA)

// Sensor 1 — fim de curso TOPO
#define TRIG_PIN_01  27   // era 19 (SPI MISO — ok, mas padronizando)
#define ECHO_PIN_01  14   // era 18 (SPI CLK — ok, mas padronizando)

// Sensor 2 — fim de curso BASE
#define TRIG_PIN_02  26   // era 4  (ADC2 — BLOQUEADO pelo WiFi!)
#define ECHO_PIN_02  25   // era 2  (boot pin + LED onboard — CRITICO!)

// --- Limiar de distância do sensor (cm) ---
// Ajuste conforme a instalação física.
// Quando a cortina está no fim de curso (cima ou baixo),
// o objeto fica a menos de DIST_THRESHOLD do sensor.
#define DIST_THRESHOLD 12.0f

// --- Estados do motor ---
enum EstadoMotor { PARADO = 0, SUBINDO = 1, DESCENDO = 2 };

// --- Posição detectada pelos sensores ---
// INDEFINIDO: leitura inconsistente (sensores discordantes ou ruído)
enum PosicaoCortina { INDEFINIDA = -1, MEIO = 0, CIMA = 1, BAIXO = 2 };

EstadoMotor   estadoMotor   = PARADO;
PosicaoCortina posicaoCortina = INDEFINIDA;

Ultrasonic sensor1(TRIG_PIN_01, ECHO_PIN_01);
Ultrasonic sensor2(TRIG_PIN_02, ECHO_PIN_02);

WiFiClientSecure secureClient;
PubSubClient client(secureClient);

// ============================================================
// Controle do motor
// ============================================================
void pararMotor(const char* motivo = nullptr) {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  estadoMotor = PARADO;
  if (motivo) {
    client.publish(TOPIC_STATUS, motivo);
    Serial.println(String("[MOTOR] Parado: ") + motivo);
  }
}

void subirCortina() {
  if (posicaoCortina == CIMA) {
    Serial.println("[MOTOR] Já está em cima, ignorado.");
    return;
  }
  digitalWrite(IN2, LOW);
  digitalWrite(IN1, HIGH);
  estadoMotor = SUBINDO;
  client.publish(TOPIC_STATUS, "Subindo");
  Serial.println("[MOTOR] Subindo");
}

void descerCortina() {
  if (posicaoCortina == BAIXO) {
    Serial.println("[MOTOR] Já está embaixo, ignorado.");
    return;
  }
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  estadoMotor = DESCENDO;
  client.publish(TOPIC_STATUS, "Descendo");
  Serial.println("[MOTOR] Descendo");
}

// ============================================================
// Callback MQTT
// Comandos esperados: "3" = subir | "2" = descer | "4" = parar
// ============================================================
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  int cmd = atoi((char*)payload);

  Serial.printf("[MQTT] Tópico: %s | CMD: %d\n", topic, cmd);

  if (strcmp(topic, TOPIC_SUB) == 0) {
    switch (cmd) {
      case 3: subirCortina();            break;
      case 2: descerCortina();           break;
      case 4: pararMotor("Parado - CMD manual"); break;
      default:
        Serial.printf("[MQTT] Comando desconhecido: %d\n", cmd);
        break;
    }
  }
}

// ============================================================
// Leitura dos sensores ultrassônicos
//
// Lógica:
//   sensor1 = fim de curso CIMA  (instalado no trilho superior)
//   sensor2 = fim de curso BAIXO (instalado no trilho inferior)
//
//   Quando a cortina está no fim de curso, ela se aproxima
//   do sensor => distância MENOR que DIST_THRESHOLD.
//
//   d1 < threshold  => cortina chegou ao topo  (sensor1 disparado)
//   d2 < threshold  => cortina chegou ao fundo (sensor2 disparado)
//   ambos >= threshold => cortina no meio
//   ambos <  threshold => leitura inválida/ruído => INDEFINIDA
// ============================================================
void verificarCortina() {
  float d1 = sensor1.read(CM);  // sensor do TOPO
  float d2 = sensor2.read(CM);  // sensor da BASE

  Serial.printf("[SENSOR] d1=%.1f cm | d2=%.1f cm\n", d1, d2);

  bool fim1 = (d1 < DIST_THRESHOLD);  // topo coberto
  bool fim2 = (d2 < DIST_THRESHOLD);  // base coberta

  if      (!fim1 && !fim2) posicaoCortina = CIMA;
  else if ( fim1 && !fim2) posicaoCortina = MEIO;
  else if ( fim1 &&  fim2) posicaoCortina = BAIXO;
  else {
    // fim2 ativo sem fim1 — impossível fisicamente, ignora leitura
    Serial.println("[SENSOR] Leitura inválida (base sem topo), mantendo estado anterior.");
  }
}

// ============================================================
// Parada automática por fim de curso
// Chamado no loop após verificarCortina()
// ============================================================
void verificarFimDeCurso() {
  if (posicaoCortina == CIMA && estadoMotor == SUBINDO) {
    pararMotor("Parado - fim de curso CIMA");
  }
  if (posicaoCortina == BAIXO && estadoMotor == DESCENDO) {
    pararMotor("Parado - fim de curso BAIXO");
  }
}

// ============================================================
// Conexão MQTT
// ============================================================
void conectarMQTT() {
  while (!client.connected()) {
    Serial.println("[MQTT] Conectando ao broker...");
    if (client.connect(ID, mqttUser, mqttPassword)) {
      Serial.println("[MQTT] Conectado.");
      client.subscribe(TOPIC_SUB);
    } else {
      Serial.printf("[MQTT] Falha (state=%d), tentando em 2s...\n", client.state());
      delay(2000);
    }
  }
}

// ============================================================
// Setup
// ============================================================
void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pararMotor();  // garante motor desligado na inicialização

  secureClient.setInsecure();

  Serial.printf("[WIFI] Conectando a %s...\n", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.printf("\n[WIFI] Conectado. IP: %s\n", WiFi.localIP().toString().c_str());

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  conectarMQTT();
}

// ============================================================
// Loop
// ============================================================
void loop() {
  if (!client.connected()) {
    conectarMQTT();
  }
  client.loop();

  verificarCortina();
  verificarFimDeCurso();

  delay(100);
}