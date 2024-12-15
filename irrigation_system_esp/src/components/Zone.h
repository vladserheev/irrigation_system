#ifndef ZONE_H
#define ZONE_H

#include <Arduino.h>
#include <vector>
#include "WateringSchedule.h"  // Include the WateringSchedule struct
#include "Valve.h"  // Make sure to include the Valve class definition
#include "Mode.h"
#include "ManualSetting.h"   // Include the Mode enum or class if it exists

enum ZoneState {
    IDLE,
    STARTING_ZONE,
    STARTING_MAIN_VALVE,
    STARTING_PUMP,
    RUNNING,
    STOPPING_ZONE,
    STOPPING_MAIN_VALVE,
    STOPPING_PUMP,
    SHUTDOWN
};

class Zone {
public:
    String name;
    std::vector<WateringSchedule> schedules;
    ManualSetting manualSetting;
    Valve valve; 
    bool status;
    ZoneState currentState = IDLE;
    unsigned long previousMillis = 0;
    Mode mode; 
    bool isWatering = false;

    Zone(String zoneName, Valve valve, bool status)
        : name(zoneName), valve(valve), status(status), mode(MANUAL) {}

    void addSchedule(uint8_t startHour, uint8_t startMinute, uint8_t finishHour, uint8_t finishMinute) {
        schedules.push_back({startHour, startMinute, finishHour, finishMinute});
        Serial.println("Schedule Added:");
        print();
    }

    void addMaualSetting(String zoneName,  float humidityAir1Max, float humidityAir1Min, float humidityGround1Max, float humidityGround1Min, float temp1Max, float temp1Min){
        manualSetting.humidityAir1Max = humidityAir1Max;
        manualSetting.humidityAir1Min = humidityAir1Min;
        manualSetting.humidityGround1Max = humidityGround1Max;
        manualSetting.humidityGround1Min = humidityGround1Min;
        manualSetting.temp1Max = temp1Max;
        manualSetting.temp1Min = temp1Min;
        Serial.println("Manual Settings Added:");
        print();
    }

    void setMode(Mode newMode) {
        mode = newMode;
    }

    bool sensorNeedsWatering() {
        // Placeholder for actual sensor logic
        return random(0, 2);  // Randomized for testing (replace with actual sensor logic)
    }

    void print() {
        Serial.print("Name: ");
        Serial.println(name);
        Serial.print("Mode: ");
        Serial.println(mode);
        Serial.print("Valve: ");
        Serial.println(valve.name.c_str());
        
        Serial.println("Schedules:");
        for (const auto& schedule : schedules) {
            Serial.print(schedule.startHour);
            Serial.print(":");
            Serial.print(schedule.startMinute);
            Serial.print(" - ");
            Serial.print(schedule.finishHour);
            Serial.print(":");
            Serial.println(schedule.finishMinute);
        }
        Serial.print("Temp max: ");
        Serial.print(manualSetting.temp1Max);
    }
};

#endif // ZONE_H
