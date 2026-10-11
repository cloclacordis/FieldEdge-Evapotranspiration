/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <stddef.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>

/* newlib time source for the STM32 smoke test */
int _gettimeofday(struct timeval *tv, void *tz) {
    (void)tz;

    if (tv == NULL) {
        errno = EINVAL;  /* Invalid argument */
        return -1;
    }

    /* Smoke test only: 2026-10-08 12:00:00 UTC; replace with RTC later */
    tv->tv_sec = (time_t)1791460800L;  /* Signed */
    tv->tv_usec = 0;

    return 0;
}
