/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef VALUE_SOURCE_H
#define VALUE_SOURCE_H

#ifdef __cplusplus
extern "C" {
#endif

/* To track the source of an input value - measurement or fallback */
typedef enum {
    SENSOR_VALUE_MEASURED = 0,
    SENSOR_VALUE_DEFAULT
} SensorValueSource;

const char* SensorValueSource_ToString(SensorValueSource source);

#ifdef __cplusplus
}
#endif

#endif /* VALUE_SOURCE_H */
