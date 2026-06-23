/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef FAO56_STATUS_H
#define FAO56_STATUS_H

#ifdef __cplusplus
extern "C" {
#endif

/* A set of states for use in all modules */
typedef enum {
    STATUS_OK = 0,
    STATUS_NULL_POINTER,
    STATUS_INVALID_VALUE,
    STATUS_UNAVAILABLE,
    STATUS_INTERNAL_ERROR
} Status;

const char* Status_ToString(Status status);

#ifdef __cplusplus
}
#endif

#endif /* FAO56_STATUS_H */
