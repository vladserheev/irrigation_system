#ifndef DHTSENSOR_H
#define DHTSENSOR_H

#include <Sensor.h>
#include <ArduinoLog.h>

#include <DHT.h>
//#include <DHT_U.h>

// Define the pin where the DHT22 is connected
// #define DHTPIN 4  // GPIO pin for the DHT22 sensor

// // Define the type of DHT sensor
// #define DHTTYPE DHT22

class DhtSensor : public Sensor {
private:
    DHT dht;
    float temperature;
    float humidity;

public:
    DHTSensor(uint8_t pin, uint8_t type) : dht(pin, type), temperature(0.0), humidity(0.0) {}

    void begin() override {
        dht.begin();
        Serial.println("Initializing DHT22 sensor...");
    }

    void readData() override {
        Serial.begin(115200);
        temperature = dht.readTemperature();
        humidity = dht.readHumidity();

        if (isnan(temperature) || isnan(humidity)) {
            Serial.println("Failed to read from DHT sensor!");
        } else {
            Serial.print("Temperature: ");
            Serial.print(temperature);
            Serial.println(" °C");
            Serial.print("Humidity: ");
            Serial.print(humidity);
            Serial.println(" %");
        }
    }
};

#endif
