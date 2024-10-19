// WateringSchedule.h
#ifndef WATERINGSCHEDULE_H
#define WATERINGSCHEDULE_H

#include <Arduino.h>

struct WateringSchedule {
    uint8_t startHour;
    uint8_t startMinute;
    uint8_t finishHour;
    uint8_t finishMinute;
};

#endif