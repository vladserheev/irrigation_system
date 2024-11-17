#ifndef VALVE_H
#define VALVE_H

#include <Arduino.h>
#include <string>
#include <ArduinoJson.h>
#include <map>

class Valve {
public:
    int id;
    std::string name;
    bool status;
    std::string location;

    Valve(int id, const std::string& name, bool status, const std::string& location);
    void print() const;
    bool digitalUpdate(std::map<String, int> pinMap, bool statusValve);
    JsonDocument toJson();
};

#endif
