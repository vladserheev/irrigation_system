#ifndef TESTIEVENTLISTENER_H
#define TESTIEVENTLISTENER_H

#include <ArduinoJson.h>

class testIEventListener {
public:
    virtual void onEvent(const std::string& event) = 0;
    virtual void onButtonAction(const std::string& event, DynamicJsonDocument doc) = 0;
    virtual void onTimedMode(const std::string& event, DynamicJsonDocument doc) = 0;
    virtual ~testIEventListener() = default;
};

#endif