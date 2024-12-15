#ifndef SOILMOISTURESENSOR_H
#define SOILMOISTURESENSOR_H

#include "Sensor.h"

class SoilMoistureSensor : public Sensor {
private:
    uint8_t analogPin;  // Аналоговый пин
    float moisturePercent;  // Уровень влажности

public:
    SoilMoistureSensor(const String& name, uint8_t pin, int min, int max)
        : Sensor(name), analogPin(pin), moisturePercent(0) {}

    void readData() override {
        Serial.println(sensorName + " - Moisture Level: " + String(moisturePercent) + "%");
    }

    void update() override {
        int moistureLevel = analogRead(analogPin); // Чтение данных с аналогового пина
        // Преобразование в процентное значение
        moisturePercent = map(moistureLevel, 4000, 0, 0, 100); 
    }

    std::vector<float> getData() override{
        return {moisturePercent};
    }

    JsonDocument toJson() override {
        JsonDocument doc;

        // Clear any previous data in the JsonDocument
        doc.clear();

        // Populate the JsonDocument
        doc["sensorName"] = sensorName;
        doc["value"] = moisturePercent;

        return doc;
    }
};
#endif