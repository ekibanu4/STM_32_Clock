#ifndef APP_TYPES_H_
#define APP_TYPES_H_

#include <stdint.h>

#include "board.h"

typedef enum {
    DISPLAY_TIME = 0,
    DISPLAY_DATE,
    DISPLAY_ENVIRONMENT,
    DISPLAY_ALARM
} DisplayMode_t;

typedef enum {
    EDIT_NONE = 0,
    EDIT_MINUTES,
    EDIT_HOURS,
    EDIT_DAY,
    EDIT_MONTH,
    EDIT_YEAR,
    EDIT_ALARM_HOURS,
    EDIT_ALARM_MINUTES
} EditTarget_t;

typedef struct {
    uint8_t configured;
    uint8_t enabled;
    uint8_t hour;
    uint8_t minute;
} AlarmSlot_t;

#endif /* APP_TYPES_H_ */
