/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <stddef.h>
#include "time-provider.h"

#define SMOKE_TEST_TIMESTAMP  ((uint32_t)1791460800UL)  /* 2026-10-08 12:00:00 UTC */

Status TimeProvider_Read(uint32_t *timestamp)
{
    if (timestamp == NULL) {
        return STATUS_NULL_POINTER;
    }

    *timestamp = SMOKE_TEST_TIMESTAMP;

    return STATUS_OK;
}
