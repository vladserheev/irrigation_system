#ifndef MODENANDLER_H
#define MODEHANDLER_H
#include <Arduino.h>
#include <RtcDS1302.h>
#include <vector>
#include "IEventListener.h"
#include "Mode.h"
#include "Valve.h"
#include "WateringSchedule.h"
#include "testIEventListener.h"
#include <queue>
#include <string> 

struct ZoneUpdate {
    String zoneName;
    bool turnOn;
};

class ModeHandler : public testIEventListener {
   private:
    std::vector<Zone> zones;
    RtcDS1302<ThreeWire> &rtc;
    Root &root;
    std::queue<std::pair<Zone*, std::string>> zoneUpdateQueue;
    std::map<String, int> pinMap;
    unsigned long previousMillis = 0;
    const unsigned long delayTime = 2000;
    

   public:
    ModeHandler(RtcDS1302<ThreeWire> &rtcRef, Root &rootRef, std::map<String, int> pinMap) : rtc(rtcRef), root(rootRef), pinMap(pinMap) {}

    void onEvent(const std::string &event) override {
        Log.notice("Modehandler: New event: %s" CR, event.c_str());
    }

    void onButtonAction(const std::string &event, DynamicJsonDocument doc) override {
        setZonesMode(MANUAL);
        setModeToAllValves(IDLE);
        std::string componentName = doc[1]["btnName"];
        std::string btnVal = doc[1]["btnVal"];
        Log.notice("ModeHandler: ButtonAction: %s -- %s" CR, componentName.c_str(), btnVal.c_str());
        root.system.components.updateComponentStateByName(pinMap, componentName, btnVal);
    }

    void onTimedMode(const std::string &event, DynamicJsonDocument doc) override {
        Log.notice("ModeHandler: Time settings event!" CR);
        addNewTimeSettingsToZones(doc);
    }

    void addZone(String zoneName, Valve valve, bool status) {
        zones.push_back(Zone(zoneName, valve, status));
    }

    void setTimedModeForZone(String zoneName, uint8_t startHour, uint8_t startMinute, uint8_t finishHour, uint8_t finishMinute) {
        Log.notice("ModeHandler: setTimedModeForZone:" CR);
        for (auto &zone : zones) {
            if (zone.name == zoneName) {
                zone.addSchedule(startHour, startMinute, finishHour, finishMinute);
                break;
            }
        }
    }

    void setModeForZone(String zoneName, Mode mode) {
        for (auto &zone : zones) {
            if (zone.name == zoneName) {
                zone.setMode(mode);
                break;
            }
        }
    }

    void addRoot(Root &rootref) { root = rootref; }

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

    void processQueue() {
          if (!zoneUpdateQueue.empty()) {
              auto currentPair = zoneUpdateQueue.front();  // Получаем первую зону и статус в очереди
              Zone* currentZone = currentPair.first;
              std::string status = currentPair.second;

              bool turnOn = (status == "ON");  // Определяем, включить или выключить
              Log.notice("ProccesQueue: Zone: Current state: ");
              Serial.print(currentZone->currentState);
              Serial.println("");
              updateZoneState(*currentZone, turnOn);  // Обновляем состояние зоны

              if (currentZone->currentState == IDLE || currentZone->currentState == RUNNING) {
                  Log.notice("ProccesQueue: Zone: %s deleted from queue with status: ", currentZone->name);
                  Serial.print(currentZone->currentState);  
                  Serial.print("");
                  zoneUpdateQueue.pop();  // Удаляем зону из очереди, когда процесс завершён
              }
          }
    }

    void addNewTimeSettingsToZones(DynamicJsonDocument doc) {
        if (doc[1]["zones"].is<JsonArray>()) {
            JsonArray zonesArray = doc[1]["zones"].as<JsonArray>();
            for (const auto &zoneJson : zonesArray) {
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
    }

    void print() {
        for (auto &zone : zones) {
            zone.print();
        }
    }

   private:
    void runManualMode(Zone &zone) {
        Serial.print("Manual mode for ");
        Serial.println(zone.name);
    }

    void runSensorMode(Zone &zone) {
        Serial.print("Sensor mode for");
        Serial.println(zone.name);
    }

    void runTimedMode(Zone &zone) {
        Log.notice("Timed mode for %s" CR, zone.name.c_str());
        RtcDateTime now = rtc.GetDateTime();
        Log.notice("Current RTC time: %s" CR, getRtcDateTimeString(now).c_str());
        
        bool shouldTurnOn = false;
        uint16_t currentTimeInMinutes = now.Hour() * 60 + now.Minute();

        for (auto &schedule : zone.schedules) {
            uint16_t startTimeInMinutes = schedule.startHour * 60 + schedule.startMinute;
            uint16_t endTimeInMinutes = schedule.finishHour * 60 + schedule.finishMinute;

            // Проверяем, попадает ли текущее время в любое расписание
            if (currentTimeInMinutes >= startTimeInMinutes && currentTimeInMinutes < endTimeInMinutes) {
                shouldTurnOn = true;
                break;  // Если хотя бы одно расписание активно, выходим из цикла
            }
        }

        String componentName = zone.valve.name.c_str();
        bool isValveOn = root.system.components.getValveStatusByName(componentName.c_str());

        if (shouldTurnOn) {
            if (!isValveOn) {  // Включаем, если клапан ещё не активен
                Log.notice("Watering zone %s - Valve ON" CR, zone.name.c_str());
                scheduleZoneUpdate(zone, "ON");
            } else {
                Log.notice("Watering zone %s is already open!" CR, zone.name.c_str());
            }
        } else {
            if (isValveOn) {  // Отключаем, если клапан ещё включен
                Log.notice("Zone: %s - Valve OFF" CR, zone.name.c_str());
                scheduleZoneUpdate(zone, "OFF");
            } else {
                Log.notice("Watering zone %s is already closed!" CR, zone.name.c_str());
            }
        }
    }

    void updateZoneState(Zone &zone, bool turnOn) {
        unsigned long currentMillis = millis();
        if (!isDelayOver()) return;

        switch (zone.currentState) {
            case IDLE:
                if (turnOn) {
                    Log.notice("Preparing to start zone %s" CR, zone.name.c_str());
                    root.system.components.print();
                    zone.currentState = STARTING_MAIN_VALVE; // Переход к включению valve_main
                    previousMillis = currentMillis;
                }
                break;

            case STARTING_MAIN_VALVE:
                //if(!isDelayOver()) return
                Log.notice("Starting main valve" CR);
                root.system.components.updateComponentStateByName(pinMap, "Valve_1", "ON"); // Включение valve_main
                zone.currentState = STARTING_ZONE; // Переход к включению zone.valve
                previousMillis = currentMillis;
                break;

            case STARTING_ZONE:
                //if(!isDelayOver()) return
                Log.notice("Starting zone %s" CR, zone.name.c_str());
                root.system.components.updateComponentStateByName(pinMap, zone.valve.name.c_str(), "ON"); // Включение zone.valve
                zone.currentState = STARTING_PUMP; // Переход к включению насоса
                previousMillis = currentMillis;
            
                break;

            case STARTING_PUMP:
                //if(!isDelayOver()) return
                Log.notice("Starting main pump" CR);
                root.system.components.updateComponentStateByName(pinMap, "Pump", "ON"); // Включение насоса
                zone.currentState = RUNNING; // Переход к RUNNING
                previousMillis = currentMillis;
                break;

            case RUNNING:
                if (!turnOn) {
                    Log.notice("Preparing to stop zone %s" CR, zone.name.c_str());
                    zone.currentState = STOPPING_PUMP; // Переход к выключению насоса
                    previousMillis = currentMillis;
                }
                break;

            case STOPPING_PUMP:
                //if(!isDelayOver()) return
                Log.notice("Stopping pump: isAnyZoneRunning: %t" CR, isAnyZoneRunning());
                if (!isAnyZoneRunning()) {
                    Log.notice("Stopping main pump" CR);
                    root.system.components.updateComponentStateByName(pinMap, "Pump", "OFF"); // Выключение насоса
                    zone.currentState = STOPPING_ZONE; // Переход к выключению zone.valve
                    previousMillis = currentMillis;
                } else {
                    zone.currentState = STOPPING_ZONE;
                }
                break;

            case STOPPING_ZONE:
                //if(!isDelayOver()) return
                Log.notice("Stopping zone %s" CR, zone.name.c_str());
                root.system.components.updateComponentStateByName(pinMap, zone.valve.name.c_str(), "OFF"); // Выключение zone.valve
                zone.currentState = STOPPING_MAIN_VALVE; // Переход к выключению valve_main
                previousMillis = currentMillis;
                break;

            case STOPPING_MAIN_VALVE:
                //if(!isDelayOver()) return
                if(!isAnyZoneRunning()){
                    Log.notice("Stopping main valve" CR);
                    root.system.components.updateComponentStateByName(pinMap, "Valve_1", "OFF"); // Выключение valve_main
                    zone.currentState = IDLE; // Переход к IDLE
                }else{
                    zone.currentState = IDLE;
                }
                break;
        }
    }

    void scheduleZoneUpdate(Zone &zone, std::string status) {
        Log.notice("ModeHandler: Zone: %s has added to queue"CR, zone.name);
        zoneUpdateQueue.push(std::make_pair(&zone, status)) ;
        Serial.println(zoneUpdateQueue.size()); // Добавляем зону в очередь на обработку
    }

    String getRtcDateTimeString(RtcDateTime now) {
        String dateTimeStr = String(now.Hour()) + ":" + String(now.Minute()) + ":" + String(now.Second());
        return dateTimeStr;
    }

    void setZonesMode(Mode mode){
        for (auto &zone : zones) {
            zone.mode = mode;
        }
        Log.notice("ModeHandler: ButtonAction: setted MANUAL mode to all zones!"CR);
    }

    void setModeToAllValves(ZoneState zoneState){
        for (auto &zone : zones) {
            zone.currentState = zoneState;
        }
        Log.notice("ModeHandler: ButtonAction: setted IDLE state to all valves!"CR);
    }
    
    bool isAnyZoneRunning() {
        for (const auto &zone : zones) {
            if (zone.currentState == RUNNING) {
                Log.notice("Exist one active zone!");
                return true; // Нашли активную зону
            }
        }
        return false; // Нет активных зон
    }

    bool isDelayOver() {
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= delayTime) {
            previousMillis = currentMillis;  // Обновляем предыдущую отметку времени
            return true;
        }
        return false;
    }
};

#endif
