#include "System.h"

System::System(int id, const Components& components, const std::vector<Sensor>& sensors, Mode mode)
          : id(id), components(components),sensors(sensors), mode(mode) {}

void System::print() const{
    Serial.print("System ID: "); Serial.println(id);
    components.print();
    //sensors.print();
}

String System::modeToString(Mode mode) {
    switch (mode) {
        case MANUAL: return "MANUAL";
        case SENSOR: return "SENSOR";
        case TIMED: return "TIMED";
        default: return "UNKNOWN";
    }
}

JsonDocument System::toJson(){
    JsonDocument jsonString;
    jsonString["Components"] = components.toJson();

    for(Sensor& sensor : sensors){
        jsonString["Sensors"].add(sensor.toJson());
    }
    jsonString["Mode"] = modeToString(mode);
    return jsonString;
}