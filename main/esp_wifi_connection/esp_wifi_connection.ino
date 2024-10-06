#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <string>
#include <map>
#include <ArduinoJson.h>
#include <SocketIOclient.h>
#include "DHT.h"

#define DHTPIN 5
#define DHTTYPE DHT11
#define BUTTON_PIN 21  // GIOP21 pin connected to button
#define LED 22
#define VALVE_MAIN 12
#define VALVE1 26
#define VALVE2 33
#define PUMP 4
#define USE_SERIAL Serial

DHT dht(DHTPIN, DHTTYPE);
WiFiMulti WiFiMulti;
SocketIOclient socketIO;

enum Mode {
  MANUAL,
  DIRECT,
  TIME
};

class TimeSettings {
  public:
    String zone1TimeStart;
    String zone1TimeEnd;
    String zone2TimeStart;
    String zone2TimeEnd;
};

std::map<String, int>
  pinMap = {
    { "pumpBtn", PUMP },
    { "mainValveBtn", VALVE_MAIN },
    { "valve1Btn", VALVE1 },
    { "valve2Btn", VALVE2 },
  };

Mode currentMode;
String currenttime = "15:40";
TimeSettings currentTimeSettings;

int lastState = LOW;  // the previous state from the input pin
int currentState;
bool isSendedMasterEmit = false;


const char *jsonString = R"({
    "timestamp": "2024-10-03T18:40:49.988Z",
    "data": {
      "timestamp": "2024-09-10T16:18:48.755Z",
      "data": {
        "system": {
          "id": 0,
          "components": {
            "valves": [
              {
                "id": 1,
                "name": "Valve 1",
                "status": false,
                "location": "zone 1"
              },
              {
                "id": 2,
                "name": "Valve 2",
                "status": false,
                "location": "zone 2"
              },
              {
                "id": 3,
                "name": "Valve 3",
                "status": false,
                "location": "zone 3"
              }
            ],
            "pump": {
              "id": 1,
              "name": "Water Pump",
              "status": false
            },
            "sensors": {
              "soilMoistureSensors": [
                {
                  "id": 1,
                  "name": "Soil Moisture Sensor 1",
                  "value": 30,
                  "unit": "%",
                  "location": "zone 1"
                },
                {
                  "id": 2,
                  "name": "Soil Moisture Sensor 2",
                  "value": 28.5,
                  "unit": "%",
                  "location": "zone 2"
                }
              ],
              "temperatureSensor": {
                "id": 1,
                "name": "Temperature Sensor",
                "value": 22,
                "unit": "°C"
              },
              "waterLevelSensor": {
                "id": 1,
                "name": "Water Level Sensor",
                "value": 75,
                "unit": "%",
                "location": "tank"
              },
              "airHumiditySensor": {
                "id": 1,
                "name": "Air Humidity Sensor",
                "value": 60,
                "unit": "%"
              }
            }
          },
          "timestamp": "2024-09-10T12:00:00Z"
        }
      }
    }
})";

void handleBtnActionEmit(DynamicJsonDocument doc) {
  String componentName = doc[1]["btnName"];
  String btnVal = doc[1]["btnVal"];
  auto it = pinMap.find(componentName);
  if (it != pinMap.end()) {
    int pinNumber = it->second;
    if (btnVal == "ON") {
      digitalWrite(pinNumber, HIGH);
      USE_SERIAL.printf("%s set to HIGH (ON)\n", componentName.c_str());
    } else if (btnVal == "OFF") {
      digitalWrite(pinNumber, LOW);
      USE_SERIAL.printf("%s set to LOW (OFF)\n", componentName.c_str());
    }
  } else {
    USE_SERIAL.printf("Unknown component: %s\n", componentName.c_str());
  }
}

void timeSetting(DynamicJsonDocument doc) {
  currentMode = TIME;

  currentTimeSettings.zone1TimeStart = doc[1]["zone1Start"].as<String>();
  currentTimeSettings.zone1TimeEnd = doc[1]["zone1End"].as<String>();
  currentTimeSettings.zone2TimeStart = doc[1]["zone2Start"].as<String>();
  currentTimeSettings.zone2TimeEnd = doc[1]["zone2End"].as<String>();
}

void timeWattering() {
  Serial.println("time waterring....");
  Serial.println(currentTimeSettings.zone1TimeStart);
}

void handlingSocketEvent(String eventName, DynamicJsonDocument doc){
  if (eventName == "btnAction") {
      handleBtnActionEmit(doc);
  } else if (eventName == "manualSettings") {
      Serial.println("Manual Setting");
  } else if (eventName == "timeSettings") {
      Serial.println("Time Setting");
      timeSetting(doc);
  }
}

void socketIOEvent(socketIOmessageType_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case sIOtype_DISCONNECT:
      USE_SERIAL.printf("[IOc] Disconnected!\n");
      break;
    case sIOtype_CONNECT:
      USE_SERIAL.printf("[IOc] Connected to url: %s\n", payload);
      // join default namespace (no auto join in Socket.IO V3)
      socketIO.send(sIOtype_CONNECT, "/");
      sendEmit("master");
      break;
    case sIOtype_EVENT:
      {
        char *sptr = NULL;
        int id = strtol((char *)payload, &sptr, 10);
        USE_SERIAL.printf("[IOc] get event: %s id: %d\n", payload, id);
        if (id) {
          payload = (uint8_t *)sptr;
        }
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload, length);
        if (error) {
          USE_SERIAL.print(F("deserializeJson() failed: "));
          USE_SERIAL.println(error.c_str());
          return;
        }

        String eventName = doc[0];
        
        serializeJson(doc, Serial);
        handlingSocketEvent(eventName, doc);

        USE_SERIAL.printf("[IOc] event name: %s\n", eventName.c_str());

        // Message Includes a ID for a ACK (callback)
        if (id) {
          // creat JSON message for Socket.IO (ack)
          DynamicJsonDocument docOut(1024);
          JsonArray array = docOut.to<JsonArray>();

          // add payload (parameters) for the ack (callback function)
          JsonObject param1 = array.createNestedObject();
          param1["now"] = millis();

          // JSON to String (serializion)
          String output;
          output += id;
          serializeJson(docOut, output);

          // Send event
          socketIO.send(sIOtype_ACK, output);
        }
        break;
      }
    case sIOtype_ACK:
      USE_SERIAL.printf("[IOc] get ack: %u\n", length);
      break;
    case sIOtype_ERROR:
      USE_SERIAL.printf("[IOc] get error: %u\n", length);
      break;
    case sIOtype_BINARY_EVENT:
      USE_SERIAL.printf("[IOc] get binary: %u\n", length);
      break;
    case sIOtype_BINARY_ACK:
      USE_SERIAL.printf("[IOc] get binary ack: %u\n", length);
      break;
  }
}

void sendEmit(String name) {
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();
  array.add(name);

  JsonObject param1 = array.createNestedObject();
  param1["val"] = true;

  String output;
  serializeJson(doc, output);

  socketIO.sendEVENT(output);

  USE_SERIAL.println(output);
}

void sendEmitJson(String name) {
  DynamicJsonDocument doc(1024);          // Create a JSON document
  JsonArray array = doc.to<JsonArray>();  // Create an array in the document

  // Add event name to the array
  array.add(name);  // Use the 'name' parameter as the event name

  // Create an object to hold parameters
  JsonObject param1 = array.createNestedObject();
  param1["val"] = jsonString;  // Add your JSON string to the parameters

  // Serialize the JSON document to a String
  String output;
  serializeJson(doc, output);  // Serialize the document into the output string

  // Send event
  socketIO.sendEVENT(output);  // Send the output string through Socket.IO

  // Print JSON for debugging
  USE_SERIAL.println(output);
}

unsigned long messageTimestamp = 0;

void setup() {
  USE_SERIAL.begin(115200);
  USE_SERIAL.setDebugOutput(true);

  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();

  pinMode(PUMP, OUTPUT);
  pinMode(VALVE_MAIN, OUTPUT);
  pinMode(VALVE1, OUTPUT);
  pinMode(VALVE2, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  for (uint8_t t = 4; t > 0; t--) {
    USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }

  WiFiMulti.addAP("DESKTOP_VLADGOG", "ajyzfajyz");

  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(100);
  }

  String ip = WiFi.localIP().toString();
  USE_SERIAL.printf("[SETUP] WiFi Connected %s\n", ip.c_str());

  socketIO.begin("192.168.137.1", 8080, "/socket.io/?EIO=4");

  socketIO.onEvent(socketIOEvent);
}

void loop() {
  socketIO.loop();
  unsigned long time = millis();

  currentState = digitalRead(BUTTON_PIN);
  if (lastState == HIGH && currentState == LOW) {
    sendEmitJson("kefteme");
    digitalWrite(LED, HIGH);
    Serial.println("The button is pressed");
    Serial.println(currentMode);
    if (currentMode == TIME) {
      timeWattering();
    }
  } else if (lastState == LOW && currentState == HIGH) {
    digitalWrite(LED, LOW);
    Serial.println("The button is released");
  }
  lastState = currentState;
}