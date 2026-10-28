/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef SERVICE_CONSOLE_H
#define SERVICE_CONSOLE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Polled UART console; Console_Poll() must be called regularly */
void Console_Init(void);
void Console_Poll(void);
void Console_PrintCurrentTime(void);

#ifdef __cplusplus
}
#endif

#endif /* SERVICE_CONSOLE_H */
