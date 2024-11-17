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

// Include necessary component headers
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

// Pin and network settings
#define BUTTON_PIN 21
#define LED 22
#define VALVE_MAIN 12
#define VALVE1 5
#define VALVE2 33
#define PUMP 4

const char* ssid = "KEFTEME";
const char* pass = "spokospoko";
const char* socketioIp = "192.168.1.103";
uint16_t socketioPort = 8080;
const char* socketioQuery = "/socket.io/?EIO=4";

const long interval = 60000;
bool isClocksSynchronized = false;
class IrrigationSystem {
private:
    WiFiMulti wifiMulti;
    ThreeWire myWire;
    RtcDS1302<ThreeWire> rtc;
    EventEmitter eventEmitter;
    SocketHandler socketHandler;
    ModeHandler modeHandler;
    Root root;
    unsigned long previousMillis;
    int lastState;

public:
    IrrigationSystem()
        : myWire(27, 14, 26), rtc(myWire),
          root(initializeSystemComponents()),
          socketHandler(eventEmitter, socketioIp, socketioPort, socketioQuery),  // Initialize Root first
          modeHandler(rtc, root, {
              { "pump", PUMP },
              { "valve1", VALVE_MAIN },
              { "valve2", VALVE1 },
              { "valve3", VALVE2 }
          }),
          previousMillis(0), lastState(0) { }

    void connectWiFi() {
        wifiMulti.addAP(ssid, pass);
        while (wifiMulti.run() != WL_CONNECTED) {
            delay(100);
        }
        Log.notice("Connected to WiFi: %s\n"CR, WiFi.localIP().toString().c_str());
    }

    Root initializeSystemComponents() {
        std::vector<Valve> valves = {
            Valve(1, "valve1", false, "zone 1"),
            Valve(2, "valve2", false, "zone 2"),
            Valve(3, "valve3", false, "zone 3")
        };
        Pump pump(1, "pump", false);
        std::vector<Sensor> sensors = {
            Sensor(1, "Temperature Sensor", 25, "°", "main"),
            Sensor(2, "Water Level Sensor", 20, "cm", "main"),
            Sensor(3, "Air Humidity Sensor", 60, "%", "main"),
            Sensor(4, "Soil Humidity Sensor 1", 25, "%", "main"),
            Sensor(5, "Soil Humidity Sensor 2", 35, "%", "main")
        };

        Components components(valves, pump);
        System system(0, components, sensors, MANUAL);
        Root root(system, "2024-10-03T18:40:49.988Z");

        // Adding zones
        modeHandler.addZone("zone1", valves[1], false);
        modeHandler.addZone("zone2", valves[2], false);

        return root;
    }

    
    String getDateTime(RtcDateTime now) {
    // Create a String to hold the formatted date and time
      String dateTimeString = "";

      // Build the date string
      dateTimeString += String(now.Year()) + "/";
      dateTimeString += String(now.Month()) + "/";
      dateTimeString += String(now.Day()) + " ";

      // Build the time string
      dateTimeString += String(now.Hour()) + ":";
      dateTimeString += String(now.Minute()) + ":";
      dateTimeString += String(now.Second());

      // Return the formatted date and time string
      return dateTimeString;
    }

    void setup() {
        Log.begin(LOG_LEVEL_VERBOSE, &Serial);
        rtc.Begin();
        rtc.SetDateTime(RtcDateTime(__DATE__, __TIME__));

        Serial.println("");
        Serial.println("");
        Serial.println("");
        Serial.println("");
        Log.notice(( "******************************************" CR));                  
        Log.notice(  "******* Irrigation System Logging ********" CR);              
        Log.notice(F("******************************************" CR));

        //rtc.Begin(); // Initialize your RTC
        RtcDateTime now = rtc.GetDateTime();
        Log.notice("Current rtc date/time: %s"CR, getDateTime(now).c_str());
        //Serial.print(getDateTime(now)); // Get current time from RTC


  
        connectWiFi();
        socketHandler.initializeSocket();
        eventEmitter.addListener(&modeHandler);

        pinMode(PUMP, OUTPUT);
        pinMode(VALVE_MAIN, OUTPUT);
        pinMode(VALVE1, OUTPUT);
        pinMode(VALVE2, OUTPUT);
        pinMode(LED, OUTPUT);
        pinMode(BUTTON_PIN, INPUT_PULLUP);

        Log.notice("Setup Complete, ESP32 Ready"CR);
    }

    void loop() {
        // Check WiFi status
        if (WiFi.status() != WL_CONNECTED) connectWiFi();

        socketHandler.loop();

        // Button handling with debounce
        unsigned long currentMillis = millis();
        int currentState = digitalRead(BUTTON_PIN);
        static unsigned long lastButtonPress = 0;
        
        if (lastState == HIGH && currentState == LOW && (currentMillis - lastButtonPress > 50)) {  // 50ms debounce
            lastButtonPress = currentMillis;
            Log.verbose("Button Press Detected"CR);
            socketHandler.sendEmitJson("kefteme", root.toJson());
            modeHandler.runMode();
            Log.verbose("Time when pressing the button: %s"CR, getDateTime(rtc.GetDateTime()).c_str());
            digitalWrite(LED, HIGH);
        } else if (lastState == LOW && currentState == HIGH) {
            digitalWrite(LED, LOW);
        }
        lastState = currentState;

        // Clocks synchronization
        if(!isClocksSynchronized && rtc.GetDateTime().Minute() != 0 && rtc.GetDateTime().Second() == 0){
          Log.notice("The clocks has successfully synchronized"CR);
          isClocksSynchronized=true;
          previousMillis = currentMillis;
        }

        if(isClocksSynchronized){
          if (currentMillis - previousMillis >= interval) {
              previousMillis = currentMillis;
              Log.notice("Interval Passed: %l seconds"CR, interval / 1000);
              modeHandler.runMode();
          }
        }

        modeHandler.processQueue();
    }
};

IrrigationSystem irrigationSystem;

void setup() {
    Serial.begin(115200);
    irrigationSystem.setup();
}

void loop() {
    irrigationSystem.loop();
}

