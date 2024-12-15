#ifndef DHT22SENSOR_H
#define DHT22SENSOR_H

#include "Sensor.h"
#include <DHT.h>
#include <ArduinoJson.h>

class DHTSensor : public Sensor {
private:
    DHT dht;              // Объект библиотеки DHT
    float temperature;    // Температура
    float humidity;       // Влажность

public:
    DHTSensor(const String& name, uint8_t pin, uint8_t type)
        : Sensor(name), dht(pin, type), temperature(0.0), humidity(0.0) {}

    void begin() override {
        dht.begin();
        Serial.print("DHT22: Begin! \n");
    }

    void readData() override {
        if (isnan(temperature) || isnan(humidity)) {
            Serial.println("DHT22: Failed to read data!");
        } else {
            Serial.println(sensorName + " - Temperature: " + String(temperature) +
                           "°C, Humidity: " + String(humidity) + "%");
        }
    }

    void update() override {
        temperature = dht.readTemperature();
        humidity = dht.readHumidity();
        Serial.print("update!!");
    }

    std::vector<float> getData() override{
        return {temperature, humidity};
    }

    JsonDocument toJson() override {
        JsonDocument doc;

        // Clear any previous data in the JsonDocument
        doc.clear();

        // Populate the JsonDocument
        doc["sensorName"] = sensorName;
        doc["tempValue"] = temperature;
        doc["humValue"] = humidity;

        return doc;
    }
};
#endif