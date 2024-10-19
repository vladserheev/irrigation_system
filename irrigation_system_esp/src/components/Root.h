#ifndef ROOT_H
#define ROOT_H

#include <Arduino.h>
#include <string>
#include <ArduinoJson.h>
#include "System.h"

class Root {
    public:
        System system;
        std::string timestamp;
        Root(const System& system, const std::string& timestamp);
        void print() const;
        JsonDocument toJson();
};  

#endif