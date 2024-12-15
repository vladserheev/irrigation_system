#include "System.h"

// System::System(int id, const Components& components, std::vector<std::unique_ptr<Sensor>>&& sensors, Mode mode)
//           : id(id), components(components),sensors(std::move(sensors)), mode(mode) {}

System::System(int id, const Components& components, Sensor* sensorArray[], Mode mode, size_t count)
          : id(id), components(components),sensors(sensorArray), mode(mode),sensorCount(count) {}



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

    // for(Sensor& sensor : sensors){
    //     jsonString["Sensors"].add(sensor.toJson());
    // }

    for (size_t i = 0; i < sensorCount; i++) {
        // Инициализируем каждый сенсор
        jsonString["Sensors"].add(sensors[i]->toJson());
        
    }
    jsonString["Mode"] = modeToString(mode);
    return jsonString;
}

    void System::initializeSensors() {
        Serial.begin(115200);
        for (size_t i = 0; i < sensorCount; i++) {
            // Инициализируем каждый сенсор
            sensors[i]->begin();
        }
    }

    // Метод для чтения данных со всех сенсоров
    void System::readAllSensors() {
        Serial.begin(115200);
        for (size_t i = 0; i < sensorCount; i++) {
            sensors[i]->readData();
        }
    }

    void System::updateAllSensors() {
        Serial.begin(115200);
        for (size_t i = 0; i < sensorCount; i++) {
            sensors[i]->update();
        }
    }

    JsonDocument System::sensorsToJson(){
        JsonDocument jsonString;
        // float temperature = 
        // jsonString["temperature"]=getSensorByName("DHT22Sensor")->
        // jsonString["soilHumidity"]=
        // jsonString["airHumidity"]=
        for (size_t i = 0; i < sensorCount; i++) {
            jsonString.add(sensors[i]->toJson());
        } 
        return jsonString; 
    };

    // Метод для получения сенсора по индексу
    Sensor* System::getSensor(size_t index) {
        if (index < sensorCount) {
            return sensors[index];
        }
        return nullptr; // Возвращаем nullptr, если индекс вне диапазона
    }

    // Метод для получения сенсора по имени
    Sensor* System::getSensorByName(const String& name) {
        for (size_t i = 0; i < sensorCount; i++) {
            if (sensors[i]->getName() == name) {
                return sensors[i];
            }
        }
        return nullptr; // Возвращаем nullptr, если сенсор с таким именем не найден
    }

//     ~System() {
//     for (int i = 0; i < sensorCount; ++i) {
//         delete sensors[i];
//     }
// }
