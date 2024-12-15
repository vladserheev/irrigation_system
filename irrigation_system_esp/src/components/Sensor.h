#ifndef DHTSENSOR_H
#define DHTSENSOR_H

#include <ArduinoJson.h>

class Sensor {
protected:
    String sensorName;

public:
    Sensor(const String& name) : sensorName(name) {}

    // Виртуальный метод для получения данных с сенсора
    virtual void readData() = 0;
    virtual void update() = 0;

    virtual std::vector<float> getData() = 0;
    virtual JsonDocument toJson() = 0;

    // Метод для получения имени сенсора
    String getName() const {
        return sensorName;
    }

    // Метод, который будет переопределен в DHTSensor для инициализации
    virtual void begin() {}

    virtual ~Sensor() {}
};

#endif