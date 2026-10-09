/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef TIME_PROVIDER_H
#define TIME_PROVIDER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../../03-validation/033-status/status.h"

Status TimeProvider_Read(uint32_t *timestamp);

#ifdef __cplusplus
}
#endif

#endif /* TIME_PROVIDER_H */
