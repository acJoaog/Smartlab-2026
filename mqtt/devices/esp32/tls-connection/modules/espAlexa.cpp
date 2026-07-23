#include <Arduino.h>
#include <WiFiClientSecure.h>

#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif

#include "fauxmoESP.h"
#include <PubSubClient.h>

// =========================
// WIFI
// =========================
const char* ssid = "CSI-Lab";
const char* password = "In@teLCS&I";

// =========================
// WIFI / MQTT
// =========================
const char* MQTT_HOST = "192.168.66.11";
const uint16_t MQTT_PORT = 8883;
const char* MQTT_USER = "smartlab";
const char* MQTT_PASS = "WhoAmI#2024";
const char* MQTT_CLIENT_ID = "alexa_mqtt_gateway";

// =========================
// INSTANCES
// =========================
WiFiClientSecure secureClient;
PubSubClient client(secureClient);
fauxmoESP fauxmo;

// =========================
// DEVICE MAP
// =========================
struct AlexaDevice {
  const char* name;
  const char* topic;
  const char* payloadOn;
  const char* payloadOff;
};

// Dispositivos expostos para Alexa
AlexaDevice devices[] = {
  {"Lâmpada 1",        "smartlab/lampada/set/1",      "on",      "off"},
  {"Lâmpada 2",        "smartlab/lampada/set/2",      "on",      "off"},
  {"Lâmpada 3",        "smartlab/lampada/set/3",      "on",      "off"},
  {"Lâmpada 4",        "smartlab/lampada/set/4",      "on",      "off"},
  {"Lâmpada 5",        "smartlab/lampada/set/5",      "on",      "off"},
  {"Lâmpada 6",        "smartlab/lampada/set/6",      "on",      "off"},

  {"TV Samsung",    "smartlab/tv/samsung/set/1", "power",   "power"},
  {"TV LG",         "smartlab/tv/lg/set/1",      "power",   "power"},

  {"Cortina 1",       "smartlab/cortina/set/1",    "open",    "close"},
  {"Cortina 2",       "smartlab/cortina/set/2",    "open",    "close"},
  {"Cortina 3",       "smartlab/cortina/set/3",    "open",    "close"},
  {"Cortina 4",       "smartlab/cortina/set/4",    "open",    "close"},
  {"Cortina 5",       "smartlab/cortina/set/5",    "open",    "close"},
  {"Cortina 6",       "smartlab/cortina/set/6",    "open",    "close"},

  {"AC 17",         "smartlab/arcond/set/1",         "17",      "17"},
  {"AC 18",         "smartlab/arcond/set/1",         "18",      "18"},
  {"AC 19",         "smartlab/arcond/set/1",         "19",      "19"},
  {"AC 20",         "smartlab/arcond/set/1",         "20",      "20"},
  {"AC 21",         "smartlab/arcond/set/1",         "21",      "21"},
  {"AC 22",         "smartlab/arcond/set/1",         "22",      "22"},
  {"AC 23",         "smartlab/arcond/set/1",         "23",      "23"},
  {"AC 24",         "smartlab/arcond/set/1",         "24",      "24"},

  {"AC2 17",        "smartlab/arcond/set/2",         "17",      "17"},
  {"AC2 18",        "smartlab/arcond/set/2",         "18",      "18"},
  {"AC2 19",        "smartlab/arcond/set/2",         "19",      "19"},
  {"AC2 20",        "smartlab/arcond/set/2",         "20",      "20"},
  {"AC2 21",        "smartlab/arcond/set/2",         "21",      "21"},
  {"AC2 22",        "smartlab/arcond/set/2",         "22",      "22"},
  {"AC2 23",        "smartlab/arcond/set/2",         "23",      "23"},
  {"AC2 24",        "smartlab/arcond/set/2",        "24",      "24"},
};

const size_t DEVICE_COUNT = sizeof(devices) / sizeof(devices[0]);

// =========================
// WIFI
// =========================
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("[WiFi] Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("[WiFi] Connected: ");
  Serial.println(WiFi.localIP());
}

// =========================
// MQTT
// =========================
void connectMQTT() {
  while (!client.connected()) {
    Serial.print("[MQTT] Connecting... ");

    if (client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

// =========================
// MQTT CALLBACK
// =========================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char msg[64];
  length = min(length, sizeof(msg) - 1);
  memcpy(msg, payload, length);
  msg[length] = '\0';

  Serial.printf("[MQTT] %s => %s\n", topic, msg);
}

// =========================
// SETUP
// =========================
void setup() {
  Serial.begin(115200);
  Serial.println();

  secureClient.setInsecure();

  connectWiFi();

  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setCallback(mqttCallback);
  connectMQTT();

  fauxmo.createServer(true);
  fauxmo.setPort(80);
  fauxmo.enable(true);

  for (size_t i = 0; i < DEVICE_COUNT; i++) {
    fauxmo.addDevice(devices[i].name);
  }

  fauxmo.onSetState([](unsigned char id, const char* name, bool state, unsigned char value) {
    Serial.printf("[Alexa] %s -> %s\n", name, state ? "ON" : "OFF");

    for (size_t i = 0; i < DEVICE_COUNT; i++) {
      if (strcmp(name, devices[i].name) == 0) {
        client.publish(
          devices[i].topic,
          state ? devices[i].payloadOn : devices[i].payloadOff
        );
        break;
      }
    }
  });
}

// =========================
// LOOP
// =========================
void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!client.connected()) connectMQTT();

  fauxmo.handle();
  client.loop();
}