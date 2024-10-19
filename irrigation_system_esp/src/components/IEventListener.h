#ifndef IEVENTLISTENER_H
#define IEVENTLISTENER_H

#include <Arduino.h>
#include <ArduinoJson.h>

class IEventListener {
public:
    virtual void onButtonAction(const String& componentName, const String& btnVal) = 0;
    virtual void onSensorSettings(const DynamicJsonDocument& doc) = 0;
    virtual void onTimeSettings(const DynamicJsonDocument& doc) = 0;
};

#endif
