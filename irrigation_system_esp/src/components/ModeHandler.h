#ifndef MODEHANDLER_H
#define MODEHANDLER_H

#include <Arduino.h>
#include <RtcDS1302.h>
#include <vector>
#include <queue>
#include <string>
#include <map>
#include <utility>  // for std::pair

#include "IEventListener.h"
#include "Mode.h"
#include "Valve.h"
#include "WateringSchedule.h"
#include "testIEventListener.h"

// Struct to hold zone update details
struct ZoneUpdate {
    String zoneName;
    bool turnOn;
};

class WateringRecord {    
    public: 
    String name;       // Имя зоны
    String timestampStart;  // Время начала
    String timestampFinish;
    
    JsonDocument toJson(){
        JsonDocument jsonString;
        jsonString["name"] = name;
        jsonString["timestampStart"] = timestampStart;
        jsonString["timestampFinish"] = timestampFinish;
        return jsonString;
    } // Время окончания
};

class ModeHandler : public testIEventListener {
   private:
    std::vector<Zone> zones;
    RtcDS1302<ThreeWire> &rtc;
    Root &root;
    std::vector<WateringRecord> startedWateringLog; // started but not finished waterings
    std::queue<std::pair<Zone*, std::string>> zoneUpdateQueue;
    std::map<String, int> pinMap;
    unsigned long previousMillis = 0;
    const unsigned long delayTime = 2000;

   public:
   std::vector<WateringRecord> wateringLog;


    ModeHandler(RtcDS1302<ThreeWire> &rtcRef, Root &rootRef, std::map<String, int> pinMap)
        : rtc(rtcRef), root(rootRef), pinMap(pinMap) {}

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
        Log.notice("ModeHandler: Time mode event!" CR);
        updateTimeSettings(doc);
    }

    void onManualMode(const std::string &event, DynamicJsonDocument doc) override {
        Log.notice("ModeHandler: Manual mode event!" CR);
        //updateTimeSettings(doc);
        updateManualSettings(doc);
    }

    void onZonesConfig(const std::string &event, DynamicJsonDocument doc) override {
        Log.notice("ModeHandler: Zones config event!" CR);
        if (doc[1]["zones"].is<JsonArray>()) {
            for (const auto &zoneJson : doc[1]["zones"].as<JsonArray>()) {
                configureZone(zoneJson);
            }
        } else {
            Log.error("ModeHandler: Zones config is not a JSON array!" CR);
        }
    }

    void addZone(String zoneName, Valve valve, bool status) {
        zones.push_back(Zone(zoneName, valve, status));
    }

    void setTimedModeForZone(String zoneName, uint8_t startHour, uint8_t startMinute, uint8_t finishHour, uint8_t finishMinute) {
        Log.verbose("Setting timed mode for zone: %s"CR, zoneName);
        for (auto &zone : zones) {
            if (zone.name == zoneName) {
                //zone.mode = TIMED;
                setModeForZone(zoneName, TIMED);
                zone.addSchedule(startHour, startMinute, finishHour, finishMinute);
                zone.print();
                break;
            }
        }
    }

    void setManualSettingForZone (String zoneName,  float humidityAir1Max, float humidityAir1Min, float humidityGround1Max, float humidityGround1Min, float temp1Max, float temp1Min){
        Log.verbose("Setting manual settings for zone: %s"CR, zoneName);
        for (auto &zone : zones) {
            if (zone.name == zoneName) {
                //zone.mode = TIMED;
                setModeForZone(zoneName, SENSOR);
                zone.addMaualSetting(zoneName, humidityAir1Max,  humidityAir1Min,  humidityGround1Max,  humidityGround1Min,  temp1Max,  temp1Min);
                zone.print();
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

    void clearZoneSchedule(String zoneName) {
        for (auto &zone : zones) {
            if (zone.name == zoneName) {
                zone.schedules.clear();
                break;
            }
        }
    }

    void runMode(float temperature, float airHumidity, float soilMoistureZone1) {
        for (auto &zone : zones) {
            switch (zone.mode) {
                case MANUAL: runManualMode(zone); break;
                case SENSOR: runSensorMode(zone, temperature, airHumidity, soilMoistureZone1); break;
                case TIMED: runTimedMode(zone); break;
            }
        }
    }

    void processQueue() {
        if (zoneUpdateQueue.empty()) return;

        auto [currentZone, status] = zoneUpdateQueue.front();
        updateZoneState(*currentZone, status == "ON");
        
        if (currentZone->currentState == IDLE || currentZone->currentState == RUNNING) {
            zoneUpdateQueue.pop();
            RtcDateTime now = rtc.GetDateTime();

            addRecordToWateringLog(currentZone->name, getDateTime(now), status == "ON");
            
            // WateringRecord record;
            // record.name = currentZone->name;
            // if (status == "ON") {
            //     Serial.print("------------------------------------------------------------------------------------------------------------------------------------------------");
            //     record.timestampStart = getDateTime(now);
            //     addRecordToWateringLog(currentZone->name, getDateTime(now), status == "ON");
            //     wateringLog.push_back(record);  // Сохранение времени начала
            // } else {
            //     Serial.print(record)
            //     record.timestampFinish = getDateTime(now); // Сохранение времени окончания
            //     wateringLog.push_back(record); // Добавление в общий лог
            // }
        }
    }

    void addRecordToWateringLog(String zoneName, String dateTime, bool status){
        if(status){
            WateringRecord record;
            record.name=zoneName;
            record.timestampStart=dateTime;
            record.timestampFinish="undefined";
            startedWateringLog.push_back(record);
        }else{
            for(auto &record : startedWateringLog){
                if(record.name == zoneName && record.timestampFinish=="undefined"){
                    record.timestampFinish=dateTime;
                    wateringLog.push_back(record);
                }
            }
        }
    }

    void print() {
        for (auto &zone : zones) {
            zone.print();
        }
    }

    // void printWateringRecords(){
    //     for (auto &record : wateringLog) {
    //         Log.notice(Lo)
    //     }
    // }

    float getZoneManualSetting(String zoneName){
        for (auto &zone : zones) {
            if (zone.name == zoneName) {
                return zone.manualSetting.humidityAir1Max;
                break;
            }
        }
    }

   private:
    void configureZone(const JsonObject& zoneJson) {
        String zoneName = zoneJson["name"];
        if (zoneJson["mode"] == "TIMED") {
            for (const auto &schedule : zoneJson["schedules"].as<JsonArray>()) {
                setTimedModeForZone(zoneName, schedule["startHour"], schedule["startMinute"], schedule["finishHour"], schedule["finishMinute"]);
            }
        }
    }

    void updateManualSettings(DynamicJsonDocument& doc) {
        Log.verbose("ModeHandler: updateding manual settings!"CR);

        if (doc[1]["zones"].is<JsonArray>()) {
            JsonArray zonesArray = doc[1]["zones"].as<JsonArray>();
            for (const auto &zoneJson : zonesArray) {
                String zoneName = zoneJson["name"].as<String>();
                setModeForZone(zoneName, SENSOR);
                Log.verbose("ModeHandler: manualSettings: zone name: %s"CR, zoneName);
                Serial.print(zoneJson["settings"].as<String>());
                float humidityAir1Max = zoneJson["settings"]["humidityAir1Max"];
                float humidityAir1Min = zoneJson["settings"]["humidityAir1Min"];
                float humidityGround1Max = zoneJson["settings"]["humidityGround1Max"];
                float humidityGround1Min = zoneJson["settings"]["humidityGround1Min"];
                float temp1Max = zoneJson["settings"]["temp1Max"];
                float temp1Min = zoneJson["settings"]["temp1Min"];

// Set the manual settings for the zone
                setManualSettingForZone(zoneName, humidityAir1Max, humidityAir1Min, humidityGround1Max, humidityGround1Min, temp1Max, temp1Min);
            }
        }
    }

    void updateTimeSettings(DynamicJsonDocument& doc) {
        if (doc[1]["zones"].is<JsonArray>()) {
            JsonArray zonesArray = doc[1]["zones"].as<JsonArray>();
            
            for (const auto &zoneJson : zonesArray) {
                String zoneName = zoneJson["name"].as<String>();
                setModeForZone(zoneName, TIMED);
                Log.verbose("ModeHandler: timeSetting: zone name: %s"CR, zoneName);

                // Make sure to clear the schedule before adding new times
                clearZoneSchedule(zoneName);

                if (zoneJson["schedules"].is<JsonArray>()) {
                    JsonArray schedules = zoneJson["schedules"].as<JsonArray>();

                    for (const auto &schedule : schedules) {
                        // Add checks to confirm these values exist before accessing
                        if (schedule.containsKey("startHour") && schedule.containsKey("startMinute") &&
                            schedule.containsKey("finishHour") && schedule.containsKey("finishMinute")) {
                            
                            uint8_t startHour = schedule["startHour"].as<uint8_t>();
                            uint8_t startMinute = schedule["startMinute"].as<uint8_t>();
                            uint8_t finishHour = schedule["finishHour"].as<uint8_t>();
                            uint8_t finishMinute = schedule["finishMinute"].as<uint8_t>();

                            setTimedModeForZone(zoneName, startHour, startMinute, finishHour, finishMinute);
                        } else {
                            Log.error("ModeHandler: Schedule data is incomplete for zone %s!" CR, zoneName.c_str());
                        }
                    }
                } else {
                    Log.error("ModeHandler: 'schedules' is not a JsonArray for zone %s!" CR, zoneName.c_str());
                }
            }
        } else {
            Log.error("ModeHandler: Zone data is not an array!" CR);
        }
    }


    void runManualMode(Zone &zone) {
        Serial.print("Manual mode for ");
        Serial.println(zone.name);
        zone.print();
    }

    void runSensorMode(Zone &zone, float temperature, float airHumidity, float soilMoistureZone1) {
        Log.notice("Sensor mode for %s" CR, zone.name.c_str());
        Log.notice("Sensors info: Temperature: %F, Air Humidity: %F, Soil Moisture: %F" CR, temperature, airHumidity, soilMoistureZone1);

        bool isNeedToUpdate = false;

        // Check soil moisture threshold
        if (soilMoistureZone1 < zone.manualSetting.humidityGround1Min) {
            Log.verbose("Sensor mode: Soil moisture below threshold. Zone needs activation." CR);
            isNeedToUpdate = true;

            if (!root.system.components.getValveStatusByName(zone.valve.name.c_str())) {
                Log.verbose("Sensor mode: Activating zone." CR);
                scheduleZoneUpdate(zone, "ON");
            }
        } else if(soilMoistureZone1 > zone.manualSetting.humidityGround1Max) {
            Log.verbose("Sensor mode: Soil moisture is above threshold. Zone needs deactivation" CR);

            if (root.system.components.getValveStatusByName(zone.valve.name.c_str())) {
                isNeedToUpdate = false;
                Log.verbose("Sensor mode: Deactivating zone." CR);
                scheduleZoneUpdate(zone, "OFF");
            }
        }else{
           Log.verbose("Sensor mode: Zone is in valid range!" CR); 

           if (root.system.components.getValveStatusByName(zone.valve.name.c_str())) {
                isNeedToUpdate = false;
                Log.verbose("Sensor mode: Deactivating zone." CR);
                scheduleZoneUpdate(zone, "OFF");
            }
        }

        // Optional: Temperature and air humidity logic
        if (temperature > zone.manualSetting.temp1Max) {
            Log.warning("Sensor mode: Temperature exceeds maximum threshold!" CR);
        }
        if (airHumidity < zone.manualSetting.humidityAir1Max) {
            Log.warning("Sensor mode: Air humidity below minimum threshold!" CR);
        }

        if (!isNeedToUpdate) {
            Log.notice("Sensor mode: Zone %s does not need an update." CR, zone.name.c_str());
        }
    }


    void runTimedMode(Zone &zone) {
        Log.notice("Timed mode for %s" CR, zone.name.c_str());
        zone.print();
        bool isNeedToUpdate;
        // bool updatedValue;
        RtcDateTime now = rtc.GetDateTime();
        uint16_t currentTimeInMinutes = now.Hour() * 60 + now.Minute();

        for (const auto &schedule : zone.schedules) {
            if (isTimeInRange(currentTimeInMinutes, schedule.startHour * 60 + schedule.startMinute, schedule.finishHour * 60 + schedule.finishMinute)) {
                Log.verbose("Time mode: time in range"CR);
                if (!root.system.components.getValveStatusByName(zone.valve.name.c_str())) {
                    Log.verbose("Time mode: activating zone"CR);
                    isNeedToUpdate=true;
                    scheduleZoneUpdate(zone, "ON");
                }
                return;
            }else{
                 if (root.system.components.getValveStatusByName(zone.valve.name.c_str())) {
                    isNeedToUpdate=false;
                    scheduleZoneUpdate(zone, "OFF");
                }   
            }
        }

        
        
        if(!isNeedToUpdate){ Log.notice("Time mode: Zone: %s do not need to update"CR, zone.name.c_str());};
    }

    bool isTimeInRange(uint16_t current, uint16_t start, uint16_t end) const {
        return current >= start && current < end;
    }

    void updateZoneState(Zone &zone, bool turnOn) {
        //Log.verbose("Time mode: Started activating... zone!"CR);
        if (!isDelayOver()) return;

        unsigned long currentMillis = millis();

        switch (zone.currentState) {
            case IDLE:
                if (turnOn) transitionState(zone, STARTING_MAIN_VALVE, currentMillis);
                break;
            case STARTING_MAIN_VALVE:
                root.system.components.updateComponentStateByName(pinMap, "valve1", "ON");
                transitionState(zone, STARTING_ZONE, currentMillis);
                break;
            case STARTING_ZONE:
                root.system.components.updateComponentStateByName(pinMap, zone.valve.name.c_str(), "ON");
                transitionState(zone, STARTING_PUMP, currentMillis);
                break;
            case STARTING_PUMP:
                root.system.components.updateComponentStateByName(pinMap, "pump", "ON");
                zone.currentState = RUNNING;
                break;
            case RUNNING:
                if (!turnOn) transitionState(zone, STOPPING_PUMP, currentMillis);
                break;
            case STOPPING_PUMP:
                if (!isAnyZoneRunning()) root.system.components.updateComponentStateByName(pinMap, "pump", "OFF");
                transitionState(zone, STOPPING_ZONE, currentMillis);
                break;
            case STOPPING_ZONE:
                root.system.components.updateComponentStateByName(pinMap, zone.valve.name.c_str(), "OFF");
                transitionState(zone, STOPPING_MAIN_VALVE, currentMillis);
                break;
            case STOPPING_MAIN_VALVE:
                if (!isAnyZoneRunning()) root.system.components.updateComponentStateByName(pinMap, "valve1", "OFF");
                zone.currentState = IDLE;
                break;
        }
    }

    void transitionState(Zone &zone, ZoneState newState, unsigned long currentMillis) {
        zone.currentState = newState;
        previousMillis = currentMillis;
    }

    void scheduleZoneUpdate(Zone &zone, std::string status) {
        zoneUpdateQueue.push(std::make_pair(&zone, status));
        Log.verbose("ModeHandler: Zone %s added to queue with status %s" CR, zone.name.c_str(), status.c_str());
    }

    void setZonesMode(Mode mode) {
        for (auto &zone : zones) {
            zone.mode = mode;
        }
        Log.verbose("ModeHandler: Set all zones to MANUAL mode!" CR);
    }

    void setModeToAllValves(ZoneState state) {
        for (auto &zone : zones) {
            zone.currentState = state;
        }
        Log.verbose("ModeHandler: Set all valves to IDLE state!" CR);
    }

    bool isAnyZoneRunning() const {
        for (const auto &zone : zones) {
            if (zone.currentState == RUNNING) return true;
       
        }
        return false;
    }

    bool isDelayOver() const {
        return millis() - previousMillis >= delayTime;
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
};

#endif  // MODEHANDLER_H
