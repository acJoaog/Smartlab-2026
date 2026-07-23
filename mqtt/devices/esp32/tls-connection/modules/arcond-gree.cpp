// =========================
// Bibliotecas
// =========================
#include <Arduino.h>
#include <IRsend.h>
#include <ir_Gree.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WiFi.h>

// =========================
// WIFI
// =========================
const char* ssid     = "CSI-Lab";
const char* password = "In@teLCS&I";

// =========================
// MQTT
// =========================
const char* mqttServer   = "192.168.66.11";
const char* mqttUser     = "smartlab";
const char* mqttPassword = "WhoAmI#2024";
const int   mqttPort     = 8883;

const char* mqttTopicSub = "smartlab/arcond/set/#";
const char* ID           = "smartlab-CTIoT-34";

// =========================
// IR - AR CONDICIONADO
// =========================
const uint16_t IRLed4 = 33;
const uint16_t IRLed3 = 18;

IRGreeAC ac3(IRLed3);  // AR 3
IRGreeAC ac4(IRLed4);  // AR 4

uint8_t temp3 = 23;
uint8_t temp4 = 23;

const int COMANDO_DESLIGAR = 7;

// =========================
// INSTÂNCIAS
// =========================
WiFiClientSecure secureClient;
PubSubClient client(secureClient);

// =========================
// WIFI
// =========================
void connectWiFi() {
  Serial.print("Conectando ao WiFi");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado!");
  Serial.println(WiFi.localIP());
}

// =========================
// PRINT ESTADO DOS ARES
// =========================
void printState() {
  Serial.println("=== Estado atual dos ares ===");

  Serial.print("[AR 3] ");
  Serial.println(ac3.toString().c_str());

  Serial.print("[AR 4] ");
  Serial.println(ac4.toString().c_str());
}

// =========================
// COMANDOS AR 3
// =========================
void processAC3Command(int valor) {
  if (valor == COMANDO_DESLIGAR) {
    ac3.off();
    ac3.send();
    Serial.println("AR 3 DESLIGADO");
  }
  else if (valor >= 16 && valor <= 30) {
    temp3 = (uint8_t)valor;
    ac3.on();
    ac3.setTemp(temp3);
    ac3.send();
    Serial.printf("AR 3 - LIGADO, temperatura ajustada para: %d\n", temp3);
  }
  else {
    Serial.printf("AR 3 - Comando invalido: %d\n", valor);
  }
}

// =========================
// COMANDOS AR 4
// =========================
void processAC4Command(int valor) {
  if (valor == COMANDO_DESLIGAR) {
    ac4.off();
    ac4.send();
    Serial.println("AR 4 DESLIGADO");
  }
  else if (valor >= 16 && valor <= 30) {
    temp4 = (uint8_t)valor;
    ac4.on();
    ac4.setTemp(temp4);
    ac4.send();
    Serial.printf("AR 4 - LIGADO, temperatura ajustada para: %d\n", temp4);
  }
  else {
    Serial.printf("AR 4 - Comando invalido: %d\n", valor);
  }
}

// =========================
// CALLBACK MQTT
// =========================
void callback(char* topic, byte* payload, unsigned int length) {
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';

  Serial.printf("Mensagem recebida [%s]: %s\n", topic, message);

  String topicStr = String(topic);
  const String topicBase = "smartlab/arcond/set/";

  if (!topicStr.startsWith(topicBase)) {
    Serial.println("Topico invalido.");
    return;
  }

  // Valida se o payload e numerico (aceita negativo, embora nao usado)
  bool isNumeric = (length > 0);
  for (unsigned int i = 0; i < length; i++) {
    if (!isDigit(message[i]) && !(i == 0 && message[i] == '-')) {
      isNumeric = false;
      break;
    }
  }

  if (!isNumeric) {
    Serial.println("Payload nao e um numero valido.");
    return;
  }

  int deviceId = topicStr.substring(topicBase.length()).toInt();
  int valor    = atoi(message);

  if (deviceId == 3) {
    processAC3Command(valor);
  }
  else if (deviceId == 4) {
    processAC4Command(valor);
  }
  else {
    // Mensagens para AR 1 e 2 sao ignoradas por este firmware
    return;
  }

  printState();
}

// =========================
// CONEXÃO MQTT
// =========================
void connectMQTT() {
  while (!client.connected()) {
    Serial.println("Conectando ao Broker MQTT...");

    if (client.connect(ID, mqttUser, mqttPassword)) {
      Serial.println("Conectado!");
      client.subscribe(mqttTopicSub);
      Serial.printf("Inscrito no topico: %s\n", mqttTopicSub);
    } else {
      Serial.print("Falha, rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

// =========================
// SETUP
// =========================
void setup() {
  Serial.begin(115200);

  ac3.begin();
  ac4.begin();

  ac3.setFan(0);
  ac3.setMode(kGreeCool);
  ac3.setTemp(temp3);

  ac4.setFan(0);
  ac4.setMode(kGreeCool);
  ac4.setTemp(temp4);

  connectWiFi();

  secureClient.setInsecure(); // ajuste se houver certificado configurado
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  Serial.println("Sistema pronto - AR 3 e AR 4 via MQTT");
  Serial.println("Topicos: smartlab/arcond/set/3 e smartlab/arcond/set/4");
  Serial.println("Comandos: 16-30=Ligar/Ajustar temperatura, 7=Desligar");
}

// =========================
// LOOP
// =========================
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!client.connected()) {
    connectMQTT();
  }

  client.loop();
} 