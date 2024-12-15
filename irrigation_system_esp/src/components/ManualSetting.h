// WateringSchedule.h
#ifndef MANUALSETTINGTRUCT_H
#define MANUALSETTINGTRUCT_H

#include <Arduino.h>

// Structure to store manual settings
class ManualSetting {
    public:
    float humidityAir1Max;      // Maximum air humidity
    float humidityAir1Min;      // Minimum air humidity
    float humidityGround1Max;   // Maximum ground humidity
    float humidityGround1Min;   // Minimum ground humidity
    float temp1Max;             // Maximum temperature
    float temp1Min;             // Minimum temperature
};

#endif
