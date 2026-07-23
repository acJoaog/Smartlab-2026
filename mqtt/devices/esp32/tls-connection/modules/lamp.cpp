#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <time.h>
#include <strings.h>

// =========================
// WIFI
// =========================
const char* ssid = "CSI-Lab";
const char* password = "In@teLCS&I";

// =========================
// MQTT
// =========================
const char* mqttServer = "192.168.66.11";
const char* mqttUser = "smartlab";
const char* mqttPassword = "WhoAmI#2024";
const int mqttPort = 8883;

const char* mqttTopicSub = "smartlab/#";
const char* ID = "smartlab-ir";

// =========================
// INSTÂNCIAS
// =========================
WiFiClientSecure secureClient;
PubSubClient client(secureClient);

#define lamp8 2
#define lamp7 4
#define lamp6 5
#define lamp5 18
#define lamp4 19
#define lamp3 21
#define lamp2 22
#define lamp1 23

// =========================
// SINCRONIZA TEMPO
// =========================
void setClock() {
  configTime(-3 * 3600, 0, "pool.ntp.org");

  Serial.print("Sincronizando tempo");
  time_t now = time(nullptr);

  while (now < 1700000000) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("\nTempo OK");
}

// =========================
// PUBLICA ESTADO MQTT
// =========================
void publishLampState(int lamp, String state) {
  String topic = "smartlab/lampada/state/" + String(lamp);
  client.publish(topic.c_str(), state.c_str(), true); // retain=true
  Serial.printf("Estado publicado [%s]: %s\n", topic.c_str(), state.c_str());
}

void publishAllLampStates() {
  publishLampState(1, digitalRead(lamp1) == LOW ? "ON" : "OFF");
  publishLampState(2, digitalRead(lamp2) == LOW ? "ON" : "OFF");
  publishLampState(3, digitalRead(lamp3) == LOW ? "ON" : "OFF");
  publishLampState(4, digitalRead(lamp4) == LOW ? "ON" : "OFF");
  publishLampState(5, digitalRead(lamp5) == LOW ? "ON" : "OFF");
  publishLampState(6, digitalRead(lamp6) == LOW ? "ON" : "OFF");
  publishLampState(7, digitalRead(lamp7) == LOW ? "ON" : "OFF");
  publishLampState(8, digitalRead(lamp8) == LOW ? "ON" : "OFF");
}

// =========================
// CONTROLE DAS LÂMPADAS
// =========================
void processLamp(int lamp, String state) {
  if (lamp < 0 || lamp > 8) {
    Serial.println("ID da lâmpada inválido.");
    return;
  }

  if (state != "ON" && state != "OFF") {
    Serial.println("Estado inválido. Use 'ON' ou 'OFF'.");
    return;
  }

  int x = (state == "ON") ? LOW : HIGH;

  switch (lamp) {
    case 0:
      digitalWrite(lamp1, x); publishLampState(1, state);
      digitalWrite(lamp2, x); publishLampState(2, state);
      digitalWrite(lamp3, x); publishLampState(3, state);
      digitalWrite(lamp4, x); publishLampState(4, state);
      digitalWrite(lamp5, x); publishLampState(5, state);
      digitalWrite(lamp6, x); publishLampState(6, state);
      digitalWrite(lamp7, x); publishLampState(7, state);
      digitalWrite(lamp8, x); publishLampState(8, state);
      break;

    case 1: digitalWrite(lamp1, x); publishLampState(1, state); break;
    case 2: digitalWrite(lamp2, x); publishLampState(2, state); break;
    case 3: digitalWrite(lamp3, x); publishLampState(3, state); break;
    case 4: digitalWrite(lamp4, x); publishLampState(4, state); break;
    case 5: digitalWrite(lamp5, x); publishLampState(5, state); break;
    case 6: digitalWrite(lamp6, x); publishLampState(6, state); break;
    case 7: digitalWrite(lamp7, x); publishLampState(7, state); break;
    case 8: digitalWrite(lamp8, x); publishLampState(8, state); break;
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
  const String topicBase = "smartlab/lampada/set/";

  if (topicStr.startsWith(topicBase)) {
    Serial.println("Tópico válido.");

    String deviceIdStr = topicStr.substring(topicBase.length());
    int deviceId = deviceIdStr.toInt();

    if (deviceIdStr.length() == 0) {
      Serial.println("ID inválido no tópico.");
      return;
    }

    processLamp(deviceId, String(message));
  } else {
    Serial.println("Tópico inválido.");
  }
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
      Serial.printf("Inscrito no tópico: %s\n", mqttTopicSub);

      publishAllLampStates(); // sincroniza estado ao reconectar
    } else {
      Serial.print("Falha, rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(lamp1, OUTPUT);
  pinMode(lamp2, OUTPUT);
  pinMode(lamp3, OUTPUT);
  pinMode(lamp4, OUTPUT);
  pinMode(lamp5, OUTPUT);
  pinMode(lamp6, OUTPUT);
  pinMode(lamp7, OUTPUT);
  pinMode(lamp8, OUTPUT);

  // inicia todas apagadas
  digitalWrite(lamp1, HIGH);
  digitalWrite(lamp2, HIGH);
  digitalWrite(lamp3, HIGH);
  digitalWrite(lamp4, HIGH);
  digitalWrite(lamp5, HIGH);
  digitalWrite(lamp6, HIGH);
  digitalWrite(lamp7, HIGH);
  digitalWrite(lamp8, HIGH);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  setClock();

  secureClient.setInsecure();

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  connectMQTT();

  ArduinoOTA.begin();
  Serial.println("OTA pronto");
}

void loop() {
  ArduinoOTA.handle();
  client.loop();

  if (!client.connected()) {
    connectMQTT();
  }
}