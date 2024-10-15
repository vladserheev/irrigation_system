#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <string>
#include <map>
#include <ArduinoJson.h>
#include <SocketIOclient.h>
#include "DHT.h"
#include <ThreeWire.h>
#include <RtcDS1302.h>

const int IO = 27;    // DAT
const int SCLK = 14;  // CLK
const int CE = 26; 

#define DHTPIN 5
#define DHTTYPE DHT11
#define BUTTON_PIN 21  // GIOP21 pin connected to button
#define LED 22
#define VALVE_MAIN 12
#define VALVE1 5
#define VALVE2 33
#define PUMP 4
#define USE_SERIAL Serial

DHT dht(DHTPIN, DHTTYPE);
WiFiMulti WiFiMulti;
SocketIOclient socketIO;
ThreeWire myWire(IO, SCLK, CE);
RtcDS1302<ThreeWire> Rtc(myWire);

enum Mode {
  MANUAL,
  SENSOR,
  TIMED
};

class TimeSettings {
  public:
    int zone1TimeStartHour;
    int zone1TimeStartMinute;
    int zone1TimeEndHour;
    int zone1TimeEndMinute;
    int zone2TimeStartHour;
    int zone2TimeStartMinute;
    int zone2TimeEndHour;
    int zone2TimeEndMinute;
};

std::map<String, int>
pinMap = {
  { "Pump", PUMP },
  { "Valve_1", VALVE_MAIN },
  { "Valve_2", VALVE1 },
  { "Valve_3", VALVE2 },
};

Mode currentMode;
String currenttime = "15:40";
TimeSettings currentTimeSettings;

int lastState = LOW; 
int currentState;

// Classes //

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

      void digitalUpdate(bool statusValve) {
        if (pinMap.find(name.c_str()) != pinMap.end()) {
          int pin = pinMap[name.c_str()];
          ::digitalWrite(pin, statusValve ? HIGH : LOW);
          status = statusValve;
          Serial.println(" Valve tatus updated");
        } else {
            Serial.println("Pin not found for valve!");
        }
      }

      StaticJsonDocument<200> toJson(){
        StaticJsonDocument<200> jsonString;
        jsonString["id"] = id;
        jsonString["name"] = name;
        jsonString["status"] = status;
        jsonString["location"] = location;
        return jsonString;
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
      
      void digitalUpdate(bool statusValve) {
        if (pinMap.find(name.c_str()) != pinMap.end()) {
          int pin = pinMap[name.c_str()];
          ::digitalWrite(pin, statusValve ? HIGH : LOW);
          status = statusValve;
          Serial.println("Pump status updated");
        } else {
            Serial.println("Pin not found for pump!");
        }
      }
      StaticJsonDocument<200> toJson(){
        StaticJsonDocument<200> jsonString;
        jsonString["id"] = id;
        jsonString["name"] = name;
        jsonString["status"] = status;
        return jsonString;
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

      StaticJsonDocument<200> toJson(){
        StaticJsonDocument<200> jsonString;
        jsonString["id"] = id;
        jsonString["name"] = name;
        jsonString["value"] = value;
        jsonString["unit"] = unit;
        jsonString["location"] = location;
        return jsonString;
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

      bool canTurnOnPump() {
        bool valve1Open = false, valve2Open = false, valve3Open = false;

        // Check the status of each valve by name
        for (const auto& valve : valves) {
            if (valve.name == "Valve_1"){valve1Open = valve.status;}
            if (valve.name == "Valve_2"){valve2Open = valve.status;}
            if (valve.name == "Valve_3"){valve3Open = valve.status;}
        }
        // The pump can be turned on if both Valve1 && Valve2 are open, or any one valve is open
        return (valve1Open && valve2Open) || (valve1Open || valve3Open);
      }
      
      // Method to check if a valve can be closed
      bool canCloseValves() {
        bool valve1Open = false, valve2Open = false, valve3Open = false;

          // Check the status of each valve by name
          for (const Valve& valve : valves) {
              if (valve.name == "Valve_1") valve1Open = valve.status;
              if (valve.name == "Valve_2") valve2Open = valve.status;
              if (valve.name == "Valve_3") valve3Open = valve.status;
          }
          // Valves cannot be closed if the pump is running
          return !pump.status && (!(valve1Open && valve2Open) || !(valve1Open || valve3Open));
      }

      void updateComponentStateByName(std::string componentName, std::string componentStatus){
          bool componentStatusBool = (componentStatus == "ON");

          if (componentName == pump.name) {
              if (componentStatusBool && !pump.status) { 
                  Serial.println("---- canTurnonPump -----");
                  Serial.println(canTurnOnPump());
                  Serial.println("----");// If trying to turn on the pump
                  if (canTurnOnPump()) {
                      pump.digitalUpdate(true);  // Turn on the pump
                  } else {
                      Serial.println("Cannot turn on the pump: No valves are open.");
                  }
              } else if (!componentStatusBool && pump.status) {  // If trying to turn off the pump
                  pump.digitalUpdate(false);  // Turn off the pump
              }
          } else {
              // Update the status of a valve
              for (Valve& valve : valves) {
                  if (valve.name == componentName) {
                    Serial.println(canCloseValves());
                      // if (!componentStatusBool && canCloseValves()) {
                        if(false){
                          Serial.println("Cannot close valve: Pump is running.");
                      } else {
                          valve.digitalUpdate(componentStatusBool);
                      }
                  }
              }
          }
      }

      StaticJsonDocument<200> toJson(){
        StaticJsonDocument<200> jsonString;

        for(Valve valve : valves){
          jsonString["Valves"].add(valve.toJson());
        }
        jsonString["Pump"]=pump.toJson();
        return jsonString;
      }
};

class System {
  public:
      int id;
      Components components;
      std::vector<Sensor> sensors;
      Mode mode;

      System(int id, const Components& components, const std::vector<Sensor>& sensors, Mode mode)
          : id(id), components(components),sensors(sensors), mode(mode) {}

      void print() {
          Serial.print("System ID: "); Serial.println(id);
          components.print();
          //sensors.print();
      }

      String modeToString(Mode mode) {
        switch (mode) {
          case MANUAL: return "MANUAL";
          case SENSOR: return "SENSOR";
          case TIMED: return "TIMED";
          default: return "UNKNOWN";
        }
      }

      StaticJsonDocument<200> toJson(){
        StaticJsonDocument<200> jsonString;
        jsonString["Components"] = components.toJson();
        
        for(Sensor& sensor : sensors){
          jsonString["Sensors"].add(sensor.toJson());
        }
        jsonString["Mode"] = modeToString(mode);
        return jsonString;
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

      StaticJsonDocument<200> toJson(){
        StaticJsonDocument<200> jsonString;
        jsonString["System"] = system.toJson();
        return jsonString;
      }
};  

Root root(System(0, Components(), std::vector<Sensor>(), MANUAL), "");
Components neededStateComponets;

struct WateringSchedule {
    uint8_t startHour;
    uint8_t startMinute;
    uint8_t finishHour;
    uint8_t finishMinute;
};

class Zone {
  public:
      String name;
      std::vector<WateringSchedule> schedules;
      Valve valve; 
      Mode mode; 
      bool isWatering = false;

      Zone(String zoneName, Valve valve) : name(zoneName), valve(valve), mode(MANUAL) {}

      void addSchedule(uint8_t startHour, uint8_t startMinute, uint8_t finishHour, uint8_t finishMinute) {
          schedules.push_back({startHour, startMinute, finishHour, finishMinute});
          Serial.println("ScheduleAdded: ");
          print();
      }

      void setMode(Mode newMode) {
          mode = newMode;
      }

      // Placeholder for reading a sensor (e.g., soil moisture)
      bool sensorNeedsWatering() {
          // Implement sensor logic here (return true if watering is needed)
          return random(0, 2);  // Randomized for testing (replace with actual sensor logic)
      }

      void print(){
        Serial.print("Name: ");
        Serial.println(name);
        Serial.print("Mode: ");
        Serial.println(mode);
        Serial.print("Valve: ");
        Serial.println(valve.name.c_str());
        for(auto &schedule : schedules){
          Serial.print(schedule.startHour);
          Serial.print(":");
          Serial.print(schedule.startMinute);
          Serial.print(" - ");
          Serial.print(schedule.finishHour);
          Serial.print(":");
          Serial.println(schedule.finishMinute);
        }
      }
};

class ModeHandler {
  private:
      std::vector<Zone> zones;  // List of zones
      RtcDS1302<ThreeWire> &rtc;

  public:
      ModeHandler(RtcDS1302<ThreeWire> &rtcRef) : rtc(rtcRef) {}

      // Add a new watering zone with associated valves
      void addZone(String zoneName, Valve valve) {
          zones.push_back(Zone(zoneName, valve));
      }

      // Set watering schedule for a specific zone
      void setTimedModeForZone(String zoneName, uint8_t startHour, uint8_t startMinute, uint8_t finishHour, uint8_t finishMinute) {
        Serial.println("setTimedModeForZone:");
          for (auto &zone : zones) {
              if (zone.name == zoneName) {
                  zone.addSchedule(startHour, startMinute, finishHour, finishMinute);
                  break;
              }
          }
      }

      // Set the mode for a specific zone (Timed or Sensor-based)
      void setModeForZone(String zoneName, Mode mode) {
          for (auto &zone : zones) {
              if (zone.name == zoneName) {
                  zone.setMode(mode);
                  break;
              }
          }
      }

      void runMode() {
        for (auto &zone : zones) {
          switch (zone.mode) {
            case MANUAL:
                runManualMode(zone);
                break;
            case SENSOR:
                runSensorMode(zone);
                break;
            case TIMED:
                runTimedMode(zone);
                break;
          }
        }
      }

      void addNewTimeSettingsToZones(DynamicJsonDocument doc){
        if (doc[1]["zones"].is<JsonArray>()) {
          JsonArray zonesArray = doc[1]["zones"].as<JsonArray>();
          for (const auto& zoneJson : zonesArray) {
              String zoneName = zoneJson["name"];

              setModeForZone(zoneName, TIMED);
              uint8_t startHour = zoneJson["startHour"];
              uint8_t startMinute = zoneJson["startMinute"];
              uint8_t finishHour = zoneJson["finishHour"];
              uint8_t finishMinute = zoneJson["finishMinute"];

              setTimedModeForZone(zoneName, startHour, startMinute, finishHour, finishMinute);
              
          }
        } else {
          Serial.println("Zone data is not an array.");
        }
          //Serial.print(doc);
      }

      void print(){
        for (auto &zone : zones) {
          zone.print();
        }
      }

      private:
      // Manual mode logic (for manual control)
      void runManualMode(Zone &zone) {
          Serial.print("Manual mode for ");
          Serial.println(zone.name);
          // Add manual control logic here (e.g., via buttons)
      }

      // Sensor-based mode logic (watering based on sensor readings)
      void runSensorMode(Zone &zone) {
          Serial.print("Sensor mode for");
          Serial.println(zone.name);
      }

      // Timed mode logic (watering based on schedules)
      void runTimedMode(Zone &zone) {
          Serial.print("Timed mode for");
          Serial.println(zone.name);
      }
};

void handleBtnActionEmit(DynamicJsonDocument doc) {
  std::string componentName = doc[1]["btnName"];
  std::string btnVal = doc[1]["btnVal"];
  Serial.printf("Handling btn action: \n");
  root.system.components.updateComponentStateByName(componentName, btnVal);
}

// void timeSetting(DynamicJsonDocument doc) {
//   currentMode = TIME;

//   currentTimeSettings.zone1TimeStartHour = doc[1]["zone1StartHour"];
//   currentTimeSettings.zone1TimeStartMinute = doc[1]["zone1StartMinute"];
//   currentTimeSettings.zone1TimeEndHour = doc[1]["zone1EndHour"];
//   currentTimeSettings.zone1TimeEndMinute = doc[1]["zone1EndMinute"];

//   currentTimeSettings.zone2TimeStartHour = doc[1]["zone2StartHour"];
//   currentTimeSettings.zone2TimeStartMinute = doc[1]["zone2StartMinute"];
//   currentTimeSettings.zone2TimeEndHour = doc[1]["zone2EndHour"];
//   currentTimeSettings.zone2TimeEndMinute = doc[1]["zone2EndMinute"];


// }

// void timeWatering() {
//   Serial.println("time waterring....");
  
//   RtcDateTime now = Rtc.GetDateTime();

//   int currentHour = now.Hour();
//   int currentMinute = now.Minute();
  
//   Serial.println(currentHour);
//   Serial.print(currentMinute);
//   // Zone 1
//   if ((currentHour > currentTimeSettings.zone1TimeStartHour || 
//        (currentHour == currentTimeSettings.zone1TimeStartHour && currentMinute >= currentTimeSettings.zone1TimeStartMinute)) &&
//       (currentHour < currentTimeSettings.zone1TimeEndHour || 
//        (currentHour == currentTimeSettings.zone1TimeEndHour && currentMinute < currentTimeSettings.zone1TimeEndMinute))) {
//     // Turn on Valve 1 and the pump
//     Serial.println("Zone 1 watering");
//     root.system.components.updateComponentStateByName("Valve_1", "ON");
//     root.system.components.updateComponentStateByName("Valve_2", "ON");
//     root.system.components.updateComponentStateByName("Pump", "ON");
//   } else {
//     // Turn off Valve 1 and the pump
//     Serial.println("Zone 1 not watering");
//     root.system.components.updateComponentStateByName("Valve_2", "OFF");
//   }

//   // Zone 2
//   if ((currentHour > currentTimeSettings.zone2TimeStartHour || 
//        (currentHour == currentTimeSettings.zone2TimeStartHour && currentMinute >= currentTimeSettings.zone2TimeStartMinute)) &&
//       (currentHour < currentTimeSettings.zone2TimeEndHour || 
//        (currentHour == currentTimeSettings.zone2TimeEndHour && currentMinute < currentTimeSettings.zone2TimeEndMinute))) {
//     // Turn on Valve 2 and the pump
//     Serial.println("Zone 2 watering");
//     root.system.components.updateComponentStateByName("Valve_1", "ON");
//     root.system.components.updateComponentStateByName("Valve_3", "ON");
//     root.system.components.updateComponentStateByName("Pump", "ON");
//   } else {
//     // Turn off Valve 2 and the pump
//     Serial.println("Zone 2 not watering");
//     root.system.components.updateComponentStateByName("Valve_3", "OFF");
//     // Serial.print(root.system.components.valves[2].status);
//   }
//   // Serial.print(root.system.components.valves[1].status);
//   // Serial.print(root.system.components.valves[2].status);
//   // Turn off the pump if both zones are off
//   if (!root.system.components.valves[1].status && !root.system.components.valves[2].status) {
//     root.system.components.updateComponentStateByName("Pump", "OFF");
//     root.system.components.updateComponentStateByName("Valve_1", "OFF");
//     Serial.println("Pump turned off");
//   }
// }

ModeHandler modeHandler(Rtc);

void handlingSocketEvent(String eventName, DynamicJsonDocument doc){
  Serial.printf("Handling socket event: %s \n", eventName);

  if (eventName == "btnAction") {
      root.system.mode = MANUAL;
      Serial.println("Sensor Control!");
      handleBtnActionEmit(doc);
  } else if (eventName == "sensorSettings") {
      root.system.mode = SENSOR;
      Serial.println("Sensor Setting!");
  } else if (eventName == "timeSettings") {
      root.system.mode = TIMED;
      Serial.println("Time Setting!");
      modeHandler.addNewTimeSettingsToZones(doc);
  }
}

// IO //

void socketIOEvent(socketIOmessageType_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case sIOtype_DISCONNECT:
      USE_SERIAL.printf("[IOc] Disconnected!\n");
      break;
    case sIOtype_CONNECT:
      USE_SERIAL.printf("[IOc] Connected to url: %s\n", payload);
      socketIO.send(sIOtype_CONNECT, "/");
      sendEmit("master", "true");
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
        
        handlingSocketEvent(eventName, doc);

        USE_SERIAL.printf("[IOc] event name: %s\n", eventName.c_str());

        // Message Includes a ID for a ACK (callback)
        if (id) {
          // creat JSON message for Socket.IO (ack)
          DynamicJsonDocument docOut(1024);
          JsonArray array = docOut.to<JsonArray>();

          // add payload (parameters) for the ack (callback function);
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

void sendEmit(String name, String val) {
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();
  array.add(name);

  JsonObject param1 = array.createNestedObject();
  param1["val"] = val;

  String output;
  serializeJson(doc, output);

  socketIO.sendEVENT(output);

  USE_SERIAL.println(output);
}

void sendEmitJson(String name, StaticJsonDocument<200> jsonString) {
  DynamicJsonDocument doc(1024);          // Create a JSON document
  JsonArray array = doc.to<JsonArray>();  // Create an array in the document

  // // Add event name to the array
  array.add(name);  // Use the 'name' parameter as the event name

  // // Create an object to hold parameters
  JsonObject param1 = array.createNestedObject();
  param1["val"] = jsonString;  // Add your JSON string to the parameters

  // // Serialize the JSON document to a String
  String output;
  serializeJson(doc, output);  // Serialize the document into the output string

  // // Send event
  socketIO.sendEVENT(output);  // Send the output string through Socket.IO

  // // Print JSON for debugging
  USE_SERIAL.println(output);
}

// system initialization //

Root initializeSystemComponents() {
    std::vector<Valve> valves = {
        Valve(1, "Valve_1", false, "zone 1"),
        Valve(2, "Valve_2", false, "zone 2"),
        Valve(3, "Valve_3", false, "zone 3")
    };
    Pump pump(1, "Pump", false);

    std::vector<Sensor> sensors = {
        Sensor(1, "Temperature Sensor", 25, "°", "main"),
        Sensor(2, "Water Level Sensor", 20, "cm", "main"),
        Sensor(3, "Air Humidity Sensor", 60, "%", "main"),
        Sensor(4, "Soil Humidity Sensor 1", 25, "%", "main"),
        Sensor(5, "Soil Humidity Sensor 2", 35, "%", "main")
    };

    Components components(valves, pump);

    System system(0, components, sensors, MANUAL);

     // Define watering zones with assigned valves
    modeHandler.addZone("Zone 1", valves[1]);  // Zone 1 controls valve 5
    modeHandler.addZone("Zone 2", valves[2]);
  
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

unsigned long messageTimestamp = 0;

void setup() {
  USE_SERIAL.begin(115200);
  USE_SERIAL.setDebugOutput(true);

  Rtc.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  //printDateTime(compiled);
  Serial.println();

  if (!Rtc.IsDateTimeValid()) {
    // Common Causes:
    //    1) first time you ran and the device wasn't running yet
    //    2) the battery on the device is low or even missing

    Serial.println("RTC lost confidence in the DateTime!");
    Rtc.SetDateTime(compiled);
  }

  if (Rtc.GetIsWriteProtected()) {
    Serial.println("RTC was write protected, enabling writing now");
    Rtc.SetIsWriteProtected(false);
  }

  if (!Rtc.GetIsRunning()) {
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled) {
    Serial.println("RTC is older than compile time!  (Updating DateTime)");
    Rtc.SetDateTime(compiled);
  } else if (now > compiled) {
    Serial.println("RTC is newer than compile time. (this is expected)");
  } else if (now == compiled) {
    Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }

  Rtc.SetDateTime(compiled);
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

  WiFiMulti.addAP("KEFTEME", "spokospoko");

  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(100);
  }
  
  String ip = WiFi.localIP().toString();
  USE_SERIAL.printf("[SETUP] WiFi Connected %s\n", ip.c_str());

  String query = "/socket.io/?EIO=4";  // Here you specify the role as master
  socketIO.begin("192.168.1.100", 8080, query.c_str());

  socketIO.onEvent(socketIOEvent);

  root = initializeSystemComponents();
  neededStateComponets = initializeNeededStateComponents();
}


unsigned long last_time=0;

void loop() {
  socketIO.loop();
  unsigned long time = millis();

  currentState = digitalRead(BUTTON_PIN);
  if (lastState == HIGH && currentState == LOW) {
    sendEmitJson("kefteme", root.toJson());
    digitalWrite(LED, HIGH);
    Serial.println(currentMode);
    // if (currentMode == TIME) {
    //   timeWattering();
      
    // }
  } else if (lastState == LOW && currentState == HIGH) {
    digitalWrite(LED, LOW);
    Serial.println("The button is released");
  }
  lastState = currentState;
}