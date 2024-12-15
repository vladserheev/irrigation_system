// System.h
#ifndef SYSTEM_H
#define SYSTEM_H

#include "Components.h"
#include "Server.h"
#include <ArduinoJson.h>
#include "Sensor.h"
#include "Mode.h"
#include <vector>
#include <memory>

class System {
  public:
      int id;

      Components components;
      //std::vector<std::unique_ptr<Sensor>> sensors;
      Sensor** sensors;
      size_t sensorCount;
      //std::vector<std::unique_ptr<DHT22Sensor>> sensors;
      Mode mode;
      // System(int id, const Components& components, std::vector<std::unique_ptr<Sensor>>&& sensors, Mode mode);
      System(int id, const Components& components, Sensor* sensorArray[], Mode mode, size_t sensorCount);
      

      void print() const;
      String modeToString(Mode mode);
      JsonDocument toJson();
      void initializeSensors();
      void readAllSensors();
      void updateAllSensors();
      JsonDocument sensorsToJson();
      Sensor* getSensor(size_t index);
      Sensor* getSensorByName(const String& name);
};

#endif