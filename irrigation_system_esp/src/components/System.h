// System.h
#ifndef SYSTEM_H
#define SYSTEM_H

#include "Components.h"
#include "Server.h"
#include <ArduinoJson.h>
#include "Sensor.h"
#include "Mode.h"

class System {
  public:
    int id;
      Components components;
      std::vector<Sensor> sensors;
      Mode mode;
      System(int id, const Components& components, const std::vector<Sensor>& sensors, Mode mode);

      void print() const;
      String modeToString(Mode mode);
      JsonDocument toJson();    
};

#endif