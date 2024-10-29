#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <string>
#include <map>
#include <ArduinoJson.h>
#include "DHT.h"
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <ArduinoLog.h>

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
#include "components/SocketHandler.h"
#include "components/EventEmiter.h"
#include "components/testIEventListener.h"

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

const char* ssid = "KEFTEME";
const char* pass = "spokospoko";
const char* socketioIp = "192.168.1.103";
uint16_t socketioPort = 8080;
const char* socketioQuery = "/socket.io/?EIO=4";

const long interval = 600000;

WiFiMulti WiFiMulti;

ThreeWire myWire(IO, SCLK, CE);
RtcDS1302<ThreeWire> Rtc(myWire);


EventEmitter eventEmitter;
SocketHandler socketHandler(eventEmitter, socketioIp, socketioPort, socketioQuery);

Root root(System(0, Components(), std::vector<Sensor>(), MANUAL), "");

std::map<String, int>
pinMap = {
  { "Pump", PUMP },
  { "Valve_1", VALVE_MAIN },
  { "Valve_2", VALVE1 },
  { "Valve_3", VALVE2 },
};  
ModeHandler modeHandler(Rtc, root, pinMap);


// Глобальные переменные


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
    modeHandler.addZone("Zone 1", valves[1], false);  // Zone 1 controls valve 5
    modeHandler.addZone("Zone 2", valves[2], false);
  
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




int lastState = LOW; 
int currentState;

void setup() {
  USE_SERIAL.begin(115200);
  USE_SERIAL.setDebugOutput(true);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);

  Rtc.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  //printDateTime(compiled);
  Serial.println();

  if (!Rtc.IsDateTimeValid()) {
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

  Log.notice(( "******************************************" CR));                  
  Log.notice(  "******* Irrigation System Logging ********" CR);              
  Log.notice(F("******************************************" CR));

  pinMode(PUMP, OUTPUT);
  pinMode(VALVE_MAIN, OUTPUT);
  pinMode(VALVE1, OUTPUT);
  pinMode(VALVE2, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  for (uint8_t t = 4; t > 0; t--) {
    Log.notice("[SETUP] BOOT WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }

  // Connnection to WiFi
  WiFiMulti.addAP(ssid, pass);
  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(100);
  }
  
  String ip = WiFi.localIP().toString();
  Log.notice("[SETUP] WiFi Connected %s\n", ip.c_str());

  // Connecting to socketIO server
  socketHandler.initializeSocket();
  
  // Adding listener
  eventEmitter.addListener(&modeHandler);

  // initialization Root
  Root root = initializeSystemComponents();
  modeHandler.addRoot(root);
}


unsigned long previousMillis = 0;
unsigned long lastModeCheck = 0;  // Для проверки режимов
unsigned long lastZoneUpdate = 0; // Для обновления состояний зон
const unsigned long modeInterval = 60000;  // Интервал в одну минуту
const unsigned long updateInterval = 2000;

void loop() {
  socketHandler.loop();
  unsigned long currentMillis = millis();

  currentState = digitalRead(BUTTON_PIN);
  if (lastState == HIGH && currentState == LOW) {
    Log.notice("Button press on oin: %d" CR, BUTTON_PIN);
    socketHandler.sendEmitJson("kefteme", root.toJson());
    //eventEmitter.emitEvent("button_click");
    modeHandler.runMode();
    
    digitalWrite(LED, HIGH);
  } else if (lastState == LOW && currentState == HIGH) {
    digitalWrite(LED, LOW);
  }
  lastState = currentState;


  // running modes 

  if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        Log.notice("Timer: %ls has passed!"CR, interval/1000);
    }

  //  if (currentMillis - lastModeCheck >= modeInterval) {
  //   lastModeCheck = currentMillis;
  //   modeHandler.runMode();  // Проверка состояния всех зон
  // }
  

  modeHandler.processQueue();
}