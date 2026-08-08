/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef DEPLOYMENT_CONFIG_H
#define DEPLOYMENT_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* *** Type A: Deployment parameters, do not change during runtime *** *** * * ** */

/* Geographic parameters */
#define CONFIG_LATITUDE_DEG    (-20.0)  /* FAO56, ex.8: 20°S, southern hemisphere */
#define CONFIG_LATITUDE_MIN    (0.0)    /* FAO56, ex.8: 20°S, southern hemisphere */
#define CONFIG_ELEVATION_M     (0.0)    /* Sea level * ** *** * ** * *** * * ** * */

/* Illuminance threshold for binary sunshine counter *** * * ********* * * ***** * *** */
#define CONFIG_BRIGHT_LUX_THRESHOLD (20000.0)  /* Preliminary estimate, not calibrated */

/* Illuminance sensor polling period */
#define CONFIG_SAMPLE_PERIOD_SEC (60U)

/* Anemometer parameters above ground surface * ***** *** ** ***** * * * *** * *** * * * * * **** * */
#define CONFIG_WIND_HEIGHT_WMO_M (10.0)    /* WMO standard: 10 m (standard meteorological stations) */
#define CONFIG_WIND_HEIGHT_FAO_M (2.0)     /* FAO56 standard: 2 m (agrometeorological stations) ** **/

/* Crop coefficient Kc mid: grass reference surface */
#define CONFIG_CROP_KC  (1.00)

#ifdef __cplusplus
}
#endif

#endif /* DEPLOYMENT_CONFIG_H */
