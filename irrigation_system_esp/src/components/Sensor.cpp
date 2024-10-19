#include "Sensor.h"

Sensor::Sensor(int id, const std::string& name, float value, const std::string& unit, const std::string& location)
          : id(id), name(name), value(value), unit(unit), location(location) {}

void Sensor::print() const {
    Serial.print("Sensor ID: "); Serial.println(id);
    Serial.print("Name: "); Serial.println(name.c_str());
    Serial.print("Value: "); Serial.print(value); Serial.print(" "); Serial.println(unit.c_str());
    Serial.print("Location: "); Serial.println(location.c_str());
}

JsonDocument Sensor::toJson(){
    JsonDocument jsonString;
    jsonString["id"] = id;
    jsonString["name"] = name;
    jsonString["value"] = value;
    jsonString["unit"] = unit;
    jsonString["location"] = location;
    return jsonString;
}