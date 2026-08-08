/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include "value-source.h"

const char* SensorValueSource_ToString(const SensorValueSource source) {
    switch (source) {
        case SENSOR_VALUE_MEASURED:
            return "MEASURED";
        case SENSOR_VALUE_DEFAULT:
            return "DEFAULT";
        default:
            return "UNKNOWN";
    }
}