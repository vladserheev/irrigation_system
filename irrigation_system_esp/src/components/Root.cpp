#include "Root.h"

Root::Root(const System& system, const std::string& timestamp)
    : system(system), timestamp(timestamp) {}

void Root::print() const{
    Serial.println("--- Root System ---");
    Serial.print("Timestamp: "); Serial.println(timestamp.c_str());
    system.print();
}

JsonDocument Root::toJson(){
    JsonDocument jsonString;
    jsonString["System"] = system.toJson();
    return jsonString;
}