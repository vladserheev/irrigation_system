// Components.h
#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <Arduino.h>
#include <vector>
#include "Valve.h"
#include "Pump.h"
#include <ArduinoJson.h>
#include "ArduinoLog.h"
class Components {
public:
    Components();
    Components(const std::vector<Valve>& valves, const Pump& pump);
    void print() const;
    bool canTurnOnPump();
    bool canCloseValves();
    bool getValveStatusByName(String name);
    JsonDocument toJson();
    void updateComponentStateByName(std::map<String, int> pinMap, std::string componentName, std::string componentStatus);
private:
    std::vector<Valve> valves;
    Pump pump;
};

#endif
