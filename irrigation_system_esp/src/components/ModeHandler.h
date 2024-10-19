#ifndef MODENANDLER_H
#define MODEHANDLER_H

#include <Arduino.h>
#include <vector>
#include "WateringSchedule.h"  // Include the WateringSchedule struct
#include "Valve.h"  // Make sure to include the Valve class definition
#include "Mode.h"
#include "IEventListener.h"

class ModeHandler{
  private:
      std::vector<Zone> zones;  // List of zones
      RtcDS1302<ThreeWire> &rtc;

  public:
    

        // void onButtonAction(const String& componentName, const String& btnVal) override {
        //     Log.notice("Handling button action for %s: %s\n", componentName.c_str(), btnVal.c_str());
        //     // Ваш код для обработки нажатий кнопок
        // }

        // // Реализация метода для обработки настроек сенсоров
        // void onSensorSettings(const DynamicJsonDocument& doc) override {
        //     Log.notice("Handling sensor settings event.");
        //     // Ваш код для обработки настроек сенсоров
        // }

        // // Реализация метода для обработки временных настроек
        // void onTimeSettings(const DynamicJsonDocument& doc) override {
        //     Log.notice("Handling time settings event.");
        //     //addNewTimeSettingsToZones(doc);
        // }

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

#endif