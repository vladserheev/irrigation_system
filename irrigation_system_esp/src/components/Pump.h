#ifndef PUMP_H
#define PUMP_H

#include <Arduino.h>
#include <string>
#include <ArduinoJson.h>
#include <map>

// Assuming pinMap is defined somewhere accessible or passed as needed
//extern std::map<String, int> pinMap;

class Pump {
public:
    int id;
    std::string name;
    bool status;

    Pump(int id, const std::string& name, bool status);
    void print() const;
    void digitalUpdate(std::map<String, int> pinMap, bool statusValve);
    JsonDocument toJson();
};

#endif
