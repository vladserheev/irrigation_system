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
    { "Pump", PUMP },
    { "Valve 1", VALVE_MAIN },
    { "Valve 2", VALVE1 },
    { "Valve 3", VALVE2 },
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
          }
        }
    }
})";

class Valve {
public:
    int id;
    std::string name;
    bool status;
    std::string location;

    Valve(int id, const std::string& name, bool status, const std::string& location)
        : id(id), name(name), status(status), location(location) {}

    void print() const {
    Serial.print("Valve ID: "); Serial.println(id);
    Serial.print("Name: "); Serial.println(name.c_str());
    Serial.print("Status: "); Serial.println(status ? "Open" : "Closed");
    Serial.print("Location: "); Serial.println(location.c_str());
    }
};

class Pump {
public:
    int id;
    std::string name;
    bool status;

    Pump(int id, const std::string& name, bool status)
        : id(id), name(name), status(status) {}

    void print() {
    Serial.print("Pump ID: "); Serial.println(id);
    Serial.print("Name: "); Serial.println(name.c_str());
    Serial.print("Status: "); Serial.println(status ? "On" : "Off");
    }
};

class Sensor {
public:
    int id;
    std::string name;
    float value;
    std::string unit;
    std::string location;

    Sensor(int id, const std::string& name, float value, const std::string& unit, const std::string& location)
        : id(id), name(name), value(value), unit(unit), location(location) {}

    void print() const {
        Serial.print("Sensor ID: "); Serial.println(id);
        Serial.print("Name: "); Serial.println(name.c_str());
        Serial.print("Value: "); Serial.print(value); Serial.print(" "); Serial.println(unit.c_str());
        Serial.print("Location: "); Serial.println(location.c_str());
    }
};

class Components {
public:
    std::vector<Valve> valves;
    Pump pump;
    Components() : valves(), pump(0, "", false) {}

    Components(const std::vector<Valve>& valves, const Pump& pump)
        : valves(valves), pump(pump) {}

    void print() {
        Serial.println("--- Valves ---");
        for (const auto& valve : valves) {
            valve.print();
            Serial.println();  // Print newline after each valve
        }
        Serial.println("--- Pump ---");
        pump.print();
    }
};

class Sensors {
public:
    std::vector<Sensor> sensors;

    Sensors(const std::vector<Sensor>& sensors)
        : sensors(sensors) {}

    void print() {
        Serial.println("--- Sensors ---");
        for (const auto& sensor : sensors) {
            sensor.print();
            Serial.println();  // Print newline after each sensor
        }
    }
};

class System {
public:
    int id;
    Components components;
    Sensors sensors;

    System(int id, const Components& components, const Sensors& sensors)
        : id(id), components(components),sensors(sensors) {}

    void print() {
        Serial.print("System ID: "); Serial.println(id);
        components.print();
        sensors.print();
    }
};

class Root {
public:
    System system;
    std::string timestamp;

    Root(const System& system, const std::string& timestamp)
        : system(system), timestamp(timestamp) {}
    void print() {
        Serial.println("--- Root System ---");
        Serial.print("Timestamp: "); Serial.println(timestamp.c_str());
        system.print();
    }
};  

// class CurrentState {
// public:
//     bool valve1Status;
//     bool valve2Status;
//     bool valve3Status;
//     bool pumpStatus;

//     CurrentState(bool v1, bool v2, bool v3, bool p)
//         : valve1Status(v1), valve2Status(v2), valve3Status(v3), pumpStatus(p) {}
// };

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

Root initializeSystemComponents() {
    std::vector<Valve> valves = {
        Valve(1, "Valve 1", false, "zone 1"),
        Valve(2, "Valve 2", false, "zone 2"),
        Valve(3, "Valve 3", false, "zone 3")
    };
    Pump pump(1, "Pump", false);

    std::vector<Sensor> sensors = {
        Sensor (1, "Temperature Sensor", 1, "cel", "main"),
        Sensor (2, "Water Level Sensor", 1, "cel", "main"),
        Sensor (3, "Air Humidity Sensor", 1, "cel", "main"),
        Sensor(4, "Soil Humidity Sensor 1", 1, "cel", "main"),
        Sensor(5, "Soil Humidity Sensor 2", 1, "cel", "main")
    };

    Components components(valves, pump);


    System system(0, components, sensors);
    std::string timestamp = "2024-10-03T18:40:49.988Z";
    Root root(system, timestamp);

    return root;
}

Components initializeNeededStateComponents(){
      std::vector<Valve> valves = {
        Valve(1, "Valve 1", false, "zone 1"),
        Valve(2, "Valve 2", false, "zone 2"),
        Valve(3, "Valve 3", false, "zone 3")
    };
    Pump pump(1, "Pump", true);
    Components components(valves, pump);
    return components;
}

void refresPhysicalComponentsAccordingToNeededState(Components neededState, Components currentState){
    for (const auto& neededValve : neededState.valves) {
        // Find the current valve corresponding to the needed valve
        for (const auto& currentValve : currentState.valves) {
            if (currentValve.id == neededValve.id) {
                // Compare the statuses
                if (neededValve.status != currentValve.status) {
                    // Write to the corresponding pin
                    digitalWrite(pinMap[neededValve.name.c_str()], neededValve.status ? HIGH : LOW);
                    Serial.print("Status of "); 
                    //Serial.print(neededValve.name); 
                    Serial.print(" changed to "); 
                    Serial.println(neededValve.status ? "OPEN" : "CLOSED");
                }
                break; // No need to check other valves once found
            }
        }
    }

    // Update pump state
    if (neededState.pump.status != currentState.pump.status) {
        digitalWrite(pinMap[currentState.pump.name.c_str()], neededState.pump.status ? HIGH : LOW);
        Serial.print("Status of "); 
        //Serial.print(currentState.pump.name); 
        Serial.print(" changed to "); 
        Serial.println(neededState.pump.status ? "ON" : "OFF");
    }
}


unsigned long messageTimestamp = 0;
//Root root(System(0, Components({Valve(0, "", false)}, Pump(0, "", false))),Sensors({Sensor(0, "" ,"", "")})), "");
Root root(System(0, Components(), Sensors({})), "");
Components neededStateComponets;
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

  root = initializeSystemComponents();
  neededStateComponets = initializeNeededStateComponents();
}



void loop() {
  socketIO.loop();
  unsigned long time = millis();

  currentState = digitalRead(BUTTON_PIN);
  if (lastState == HIGH && currentState == LOW) {
    sendEmitJson("kefteme");
    digitalWrite(LED, HIGH);
    refresPhysicalComponentsAccordingToNeededState(neededStateComponets, root.system.components);
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