// Sensor.h
#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include <string>
#include <ArduinoJson.h>

class Sensor {
public:
    Sensor(int id, const std::string& name, float value, const std::string& unit, const std::string& location);
    void print() const;
    JsonDocument toJson();
private: 
    int id;
    std::string name;
    float value;
    std::string unit;
    std::string location;
};

#endif
