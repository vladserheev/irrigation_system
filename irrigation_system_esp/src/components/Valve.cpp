#include "Valve.h"

Valve::Valve(int id, const std::string& name, bool status, const std::string& location)
          : id(id), name(name), status(status), location(location) {}

void Valve::print() const {
    Serial.print("Valve ID: "); Serial.println(id);
    Serial.print("Name: "); Serial.println(name.c_str());
    Serial.print("Status: "); Serial.println(status ? "Open" : "Closed");
    Serial.print("Location: "); Serial.println(location.c_str());
}

bool Valve::digitalUpdate(std::map<String, int> pinMap, bool statusValve ) {
    try{
        if (pinMap.find(name.c_str()) != pinMap.end()) {
            int pin = pinMap[name.c_str()];
            ::digitalWrite(pin, statusValve ? HIGH : LOW);
            status = statusValve;
            //Serial.println(" Valve status updated");
            return true;
        } else {
            Serial.println("Pin not found for valve!");
            throw "Pin not found for valve!";
        }
    }catch(String e){
        Serial.print(e);
        return false;
    }
}

JsonDocument Valve::toJson(){
    JsonDocument jsonString;
    jsonString["id"] = id;
    jsonString["name"] = name;
    jsonString["status"] = status;
    jsonString["location"] = location;
    return jsonString;
}