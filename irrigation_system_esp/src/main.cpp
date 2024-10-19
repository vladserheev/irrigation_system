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

// Classes
#include "components/Valve.h"
#include "components/Pump.h"
#include "components/Sensor.h"
#include "components/Components.h"
#include "components/System.h"
#include "components/Root.h"
#include "components/WateringSchedule.H"
#include "components/Zone.h"
#include "components/ModeHandler.h"

const int IO = 27;    // DAT
const int SCLK = 14;  // CLK
const int CE = 26; 


#define BUTTON_PIN 21  // GIOP21 pin connected to button
#define LED 22
#define VALVE_MAIN 12
#define VALVE1 5
#define VALVE2 33
#define PUMP 4
#define USE_SERIAL Serial


WiFiMulti WiFiMulti;
SocketIOclient socketIO;
ThreeWire myWire(IO, SCLK, CE);
RtcDS1302<ThreeWire> Rtc(myWire);


std::map<String, int>
pinMap = {
  { "Pump", PUMP },
  { "Valve_1", VALVE_MAIN },
  { "Valve_2", VALVE1 },
  { "Valve_3", VALVE2 },
};

//Mode currentMode;

int lastState = LOW; 
int currentState;

Root root(System(0, Components(), std::vector<Sensor>(), MANUAL), "");
Components neededStateComponets;

void handleBtnActionEmit(DynamicJsonDocument doc) {
  std::string componentName = doc[1]["btnName"];
  std::string btnVal = doc[1]["btnVal"];
  Serial.printf("Handling btn action: \n");
  root.system.components.updateComponentStateByName(pinMap, componentName, btnVal);
}

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

void socketIOEvent(socketIOmessageType_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case sIOtype_DISCONNECT:
      USE_SERIAL.printf("[IOc] Disconnecteddd!\n");
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

void sendEmitJson(String name, JsonDocument jsonString) {
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
  socketIO.begin("192.168.1.103", 8080, query.c_str());

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
    //Serial.println(currentMode);
    // if (currentMode == TIME) {
    //   timeWattering();
      
    // }
  } else if (lastState == LOW && currentState == HIGH) {
    digitalWrite(LED, LOW);
    Serial.println("The button is released");
  }
  lastState = currentState;
}