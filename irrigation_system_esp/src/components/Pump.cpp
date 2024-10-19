#include "Pump.h"

Pump::Pump(int id, const std::string& name, bool status)
    : id(id), name(name), status(status) {}

void Pump::print() const {
    Serial.print("Pump ID: "); Serial.println(id);
    Serial.print("Name: "); Serial.println(name.c_str());
    Serial.print("Status: "); Serial.println(status ? "On" : "Off");
}

void Pump::digitalUpdate(std::map<String, int> pinMap, bool statusValve) {
    if (pinMap.find(name.c_str()) != pinMap.end()) {
        int pin = pinMap[name.c_str()];
        ::digitalWrite(pin, statusValve ? HIGH : LOW);
        status = statusValve;
        Serial.println("Pump status updated");
    } else {
        Serial.println("Pin not found for pump!");
    }
}

JsonDocument Pump::toJson() {
    JsonDocument jsonString;
    jsonString["id"] = id;
    jsonString["name"] = name;
    jsonString["status"] = status;
    return jsonString;
}
