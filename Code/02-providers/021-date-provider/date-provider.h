/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef DATE_PROVIDER_H
#define DATE_PROVIDER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../../03-validation/033-status/status.h"

typedef struct {
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
} DateData;

Status DateProvider_Read(DateData* date);

#ifdef __cplusplus
}
#endif

#endif /* DATE_PROVIDER_H */
