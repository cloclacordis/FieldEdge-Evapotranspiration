/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef TEST_CONFIG_H
#define TEST_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* * ** * * *** ** * * ** ****** * * * ** *** ** * * * * * * ******* * * *** * ** * **** ***
 * FAO56 reference values & test scenario parameters;
 * used ONLY in 06-test/main-test.c;
 * physical model constants remain in the computation module headers
 * * ** * * *** ** * * ** ****** * * * ** *** ** * * * * * * ******* * * *** * ** * **** * */

/* * ** * * *** ** * * ** ****** * * * ** *** ** * * * * * * ******* * * *** * ** * **** ***
 * Tolerances for double value comparison;
 * selected according to the precision stated in FAO56
 * * ** * * *** ** * * ** ****** * * * ** *** ** * * * * * * ******* * * *** * ** * **** * */

#define TOL_AIR_TEMP   (0.01)    /* C,               air temperature * * * *** * *** * * * */
#define TOL_KPA        (0.0001)  /* kPa,             vapour pressure **** * **** * **** ** */
#define TOL_KPA_PER_C  (0.0001)  /* kPa/C,           slope of vapour pressure curve ** *** */
#define TOL_DEGREE     (0.01)    /* degree,          latitude in degrees ***** * **** * ** */
#define TOL_RADIANS    (0.001)   /* rad,             latitude in radians *** * *** * ***** */
#define TOL_ANGLE      (0.005)   /* rad,             astronomical angles **** * *** * **** */
#define TOL_HOURS      (0.05)    /* h,               daylight duration *** *** * *** ** ** */
#define TOL_RA         (0.05)    /* MJ m-2 day-1,    extraterrestrial radiation *** ** *** */
#define TOL_RS         (0.05)    /* MJ m-2 day-1,    solar radiation ***** * * **** * * ** */
#define TOL_RSO        (0.05)    /* MJ m-2 day-1,    clear-sky radiation **** * * * * **** */

/* * ** * * *** ** * * ** ****** * * * ** *** ** * * * * * * ******* * * *** * ** * **** ***
 * TC1-5: air temperature & saturation vapour pressure
 *
 * TC1 (isothermal): single measurement T = 20C;
 * TC5 (min/max): three measurements - 20, 10, 30C;
 * reference: FAO56 annex 2, table 2.3, 2.4
 * * ** * * *** ** * * ** ****** * * * ** *** ** * * * * * * ******* * * *** * ** * **** * */

#define TEST_TEMP_INST_C          (20.0)    /* TC1:     instant sensor reading *** *** *** */
#define TEST_TEMP_MEAN_C          (20.0)    /* TC1/TC5: Tmean = (Tmin + Tmax) / 2 ** *** * */
#define TEST_TEMP_MIN_C           (10.0)    /* TC5:     minimum temperature **** ** ** *** */
#define TEST_TEMP_MAX_C           (30.0)    /* TC5:     maximum temperature **** ** ** *** */
#define TEST_TEMP_OUT_OF_RANGE    (150.0)   /* TC2:     outside valid range **** ** ** *** */
#define TEST_E_TMEAN_EXPECTED     (2.3383)  /* kPa,     e(Tmean = 20C) ***** * * ***** *** */
#define TEST_E_S_EXPECTED         (2.3383)  /* kPa,     es for isothermal scenario *** *** */
#define TEST_DELTA_EXPECTED       (0.1447)  /* kPa/C,   Δ at T = 20C ****** * * ***** * ** */

/* * ** * * *** ** * * ** ****** * * * ** *** ** * * * * * * ******* * * *** * ** * **** ***
 * TC12-13: DMS -> decimal degrees & radians conversion; FAO56 eq. 22, ex. 7
 * * ** * * *** ** * * ** ****** * * * ** *** ** * * * * * * ******* * * *** * ** * **** * */

/* Bangkok: 13°44'N **** * * * ** *** ** * * ***** * * * ** *** ** * * ***** * * * ** **** */
#define TEST_BANGKOK_LAT_DEG          (13.0)     /* degrees ***** * * * ** *** ** * * **** */
#define TEST_BANGKOK_LAT_MIN          (44.0)     /* minutes' **** * * * ** *** ** * * * ** */
#define TEST_BANGKOK_LAT_EXPECTED     (13.7333)  /* degrees *** ***** * * * ** *** ** * ** */
#define TEST_BANGKOK_RAD_EXPECTED     (0.2400)   /* rad ** **** * * * ** *** ** * * * * ** */

/* Rio de Janeiro: 22°54'S **** * * * ** *** ** * * ***** * * * ** *** ** * * ***** * * ** */
#define TEST_RIO_LAT_DEG              (-22.0)    /* degrees *** ***** * * * ** *** ** * ** */
#define TEST_RIO_LAT_MIN              (54.0)     /* minutes' **** * * * ** *** ** * * **** */
#define TEST_RIO_LAT_EXPECTED         (-22.9000) /* degrees * ** **** ** * * * *** ** **** */
#define TEST_RIO_RAD_EXPECTED         (-0.4000)  /* rad * * *** * ** *** ** * * * **** *** */

/* * ** * * *** ** * * ** ****** * * * ** *** ** * * * * * * ******* * * *** * ** * **** ***
 * TC14: extraterrestrial radiation Ra, FAO56 ex. 8 & ex. 9;
 * location: 20°S, sea level; day: J = 246 (3 September);
 * reference values from FAO56 ex. 8 (Ra) & ex. 9 (N)
 * * ** * * *** ** * * ** ****** * * * ** *** ** * * * * * * ******* * * *** * ** * **** * */

#define TEST_EX8_J                    (246U)     /* day J **** * * * ****** * * ***** **** */
#define TEST_EX8_LAT_DEG              (-20.0)    /* degrees *** ** * * ****** *** * **** * */
#define TEST_EX8_LAT_MIN              (0.0)      /* minutes' * ** *** ** * * * *** ** **** */
#define TEST_ELEVATION_SEA_LEVEL      (0.0)      /* m, used in TC14, TC20 *** * * ***** ** */
#define TEST_EX8_DR_EXPECTED          (0.985)    /* inverse relative distance Earth-Sun ** */
#define TEST_EX8_DELTA_RAD_EXPECTED   (0.120)    /* solar declination δ, rad **** * **** * */
#define TEST_EX8_OMEGA_S_EXPECTED     (1.527)    /* sunset hour angle ωs, rad **** *** *** */
#define TEST_EX8_N_EXPECTED           (11.7)     /* h daylight duration * ** *** ** * * ** */
#define TEST_EX8_RA_EXPECTED          (32.2)     /* MJ m-2 day-1 * ** *** ** * * * *** *** */

/* * ** * * *** ** * * ** ****** * * * ** *** ** * * * * * * ******* * * *** * ** * **** ***
 * TC15: polar night, FAO56 ex. 8 (extension);
 * location: 80°N; day: J = 355 (winter solstice);
 * during polar night: ωs = 0, N = 0, Ra = 0
 * * ** * * *** ** * * ** ****** * * * ** *** ** * * * * * * ******* * * *** * ** * **** * */
 
#define TEST_POLAR_LAT_DEG            (80.0)     /* degrees * ** **** ** * * * *** ** **** */
#define TEST_POLAR_J                  (355U)     /* day J * ** *** ** * * * *** ** **** ** */
#define TEST_POLAR_OMEGA_S_EXPECTED   (0.0)      /* rad * ** *** ** * * * *** ** **** **** */
#define TEST_POLAR_N_EXPECTED         (0.0)      /* h * ** *** ** * * * *** ** **** * * ** */
#define TEST_POLAR_RA_EXPECTED        (0.0)      /* MJ m-2 day-1 * * *** ** * * * *** ** * */

/* * ** * * *** ** * * ** ****** * * * ** *** ** * * * * * * ******* * * *** * ** * **** ***
 * TC20: solar radiation Rs, Rso, FAO56 ex. 10;
 * location: Rio de Janeiro 22°54'S; day: J = 135 (15 May);
 * data: 220 sunshine hours for May (31 days) -> n = 220/31 = 7.1 h/day
 * * ** * * *** ** * * ** ****** * * * ** *** ** * * * * * * ******* * * *** * ** * **** * */

#define TEST_EX10_J                   (135U)     /* day J ** ** * * * *** ** **** **** *** */
#define TEST_EX10_N_HOURS             (7.1)      /* h actual sunshine duration **** ** *** */
#define TEST_EX10_RA_EXPECTED         (25.1)     /* MJ m-2 day-1 ** ** * * * *** ** **** * */
#define TEST_EX10_N_DAYLIGHT_EXPECTED (10.9)     /* h daylight duration ** ** * * * *** ** */
#define TEST_EX10_RS_EXPECTED         (14.5)     /* MJ m-2 day-1 ** ** * * * *** *** *** * */
#define TEST_EX10_RSO_EXPECTED        (18.8)     /* MJ m-2 day-1 ** ** * ** * ** ** **** * */

/* * ** * * *** ** * * ** ****** * * * ** *** ** * * * * * * ******* * * *** * ** * **** ***
 * TC27: real-time lux sensor polling emulation
 * * ** * * *** ** * * ** ****** * * * ** *** ** * * * * * * ******* * * *** * ** * **** * */

#define TEST_EMUL_SAMPLES             (6U)       /* number of samples ** ** * * * *** ** * */
#define TEST_EMUL_DELAY_MS            (200U)     /* delay between samples, ms ** ** * * ** */
#define TEST_EMUL_LUX_BRIGHT          (50000.0)  /* lux, "clear" ** ** ** * * *** ** ***** */
#define TEST_EMUL_LUX_DARK            (500.0)    /* lux, "overcast" *** ** * * * *** ** ** */

/* * ** * * *** ** * * ** ****** * * * ** *** ** * * * * * * ******* * * *** * ** * **** ***
 * TC29-32: net radiation Rns, Rnl, Rn, FAO56 ex. 11 & ex. 12
 *
 * TC29: Rio de Janeiro, May
 *
 *   input data from ex. 11 & ex. 10:
 *     Tmax = 25.1C; Tmin = 19.1C, ea = 2.1 kPa;
 *     Rs = 14.5; Rso = 18.8 MJ m-2 day-1
 *
 *   reference (ex. 11, ex. 12):
 *     Rns = (1 - 0.23) * 14.5 = 11.1 MJ m-2 day-1;
 *     Rnl = 3.5 MJ m-2 day-1; Rn  = 7.6 MJ m-2 day-1
 * * ** * * *** ** * * ** ****** * * * ** *** ** * * * * * * ******* * * *** * ** * **** * */

#define TEST_EX11_TMAX_C              (25.1)    /* C, Tmax for Rnl ** *** * * * *** ** *** */
#define TEST_EX11_TMIN_C              (19.1)    /* C, Tmin for Rnl *** ** * * * *** ** *** */
#define TEST_EX11_EA_KPA              (2.1)     /* kPa, actual vapour pressure ** *** * ** */
#define TEST_EX11_RNL_EXPECTED        (3.5)     /* MJ m-2 day-1 ** ** * * * *** ** **** ** */
#define TEST_EX12_RNS_EXPECTED        (11.1)    /* MJ m-2 day-1 ** ** * * * *** ** ***** * */
#define TEST_EX12_RN_EXPECTED         (7.6)     /* MJ m-2 day-1 ** ** * * * *** *** **** * */

/* Tolerances * * *** ** **** ****** *** * *** ** **** ****** *** * *** ** **** ****** *** */
#define TOL_RNS  (0.10)   /* MJ m-2 day-1, eq. 38 ** ** * * * *** ** **** ****** ** * * ** */
#define TOL_RNL  (0.15)   /* MJ m-2 day-1, eq. 39 (T⁴ accumulates rounding)  *** * * ***** */
#define TOL_RN   (0.20)   /* MJ m-2 day-1, eq. 40 (sum of two roundings) *** **** * * * ** */

/* * ** * * *** ** * * ** ****** * * * ** *** ** * * * * * * ******* * * *** * ** * **** ***
 * TC33-TC43: air humidity, atmospheric pressure, psychrometrics, vapour pressure
 *
 * TC33: AirHumidity_Update            - accumulation of min/max/mean (from ex. 5 values)
 * TC34: AirHumidity_Update            - NaN
 * TC35: AirHumidity_Update            - Infinity
 * TC36: AirHumidity_Update            - Out of range
 * TC37: Calc_PressureFromElevation    - sea level
 * TC38: Calc_PressureFromElevation    - FAO56 ex. 2 (z = 1800 m)
 * TC39: Calc_AtmosphericParameters    - sea level
 * TC40: Calc_AtmosphericParameters    - FAO56 ex. 2
 * TC41: NULL pointer                  - ... ... ...
 * TC42: Calc_SaturationVapourPressure - FAO56 ex. 3
 * TC43: Calc_ActualVapourPressure     - FAO56 ex. 5
 * * ** * * *** ** * * ** ****** * * * ** *** ** * * * * * * ******* * * *** * ** * **** * */

/* TC33-36: humidity accumulation (values from FAO56 ex. 5) ** ** * * * *** ** *** **** ** */
#define TEST_EX5_RH_MAX               (82.0)    /* % ** ** * * ** *** ** *** **** *** * ** */
#define TEST_EX5_RH_MIN               (54.0)    /* % ** ** * * * *** ** **** ***** ** * ** */
#define TEST_RH_MEAN_EXPECTED         (68.0)    /* %, (82 + 54) / 2 *** * * **** * * * *** */
#define TEST_RH_OUT_OF_RANGE          (150.0)   /* TC36: outside valid range *** ** ** *** */

/* TC37-38: Calc_PressureFromElevation ** **** * * * *** * **** * * * ***** * * * ***** ** */
#define TEST_P_SEA_LEVEL_EXPECTED     (101.3)   /* kPa **** * * * ***** * * * ***** * * ** */
#define TEST_EX2_ELEVATION_M          (1800.0)  /* m *** * ***** * * * ******* * * * ***** */
#define TEST_EX2_P_EXPECTED           (81.8)    /* kPa ****** * * * **** **** * * * ****** */

/* TC39-40: Calc_AtmosphericParameters ** **** * * * *** * **** * * * ***** * * * ***** ** */
#define TEST_GAMMA_SEA_LEVEL_EXPECTED (0.0674)  /* kPa/C *** * ***** * * * ******* * ***** */
#define TEST_EX2_GAMMA_EXPECTED       (0.054)   /* kPa/C **** * *** * ***** * * * ***** ** */

/* TC42: Calc_SaturationVapourPressure, FAO56 ex. 3 ** **** * * * *** * **** * * * ***** * */
#define TEST_EX3_TMAX_C               (24.5)    /* C ** ** ***** * * * ******* * ***** * * */
#define TEST_EX3_TMIN_C               (15.0)    /* C **** *** * ***** * * * ***** * * * ** */
#define TEST_EX3_E_TMAX_EXPECTED      (3.075)   /* kPa ******* * * * **** ** * ***** * * * */
#define TEST_EX3_E_TMIN_EXPECTED      (1.705)   /* kPa ** * * ***** ** * ***** * * * ***** */

/* TC43: Calc_ActualVapourPressure, FAO56 ex. 5 ** ** * ***** * * * ******* * ***** * * ** */
#define TEST_EX5_TMIN_C               (18.0)    /* C * * *** ** ***** ****** *** * *** *** */
#define TEST_EX5_TMAX_C               (25.0)    /* C *** * ** * **** ** **** ****** ** *** */
#define TEST_EX5_EA_EXPECTED          (1.70)    /* kPa * * *** ** **** ****** *** * *** ** */

/* Tolerances * * *** ** **** ****** *** * *** ** **** ****** *** * *** ** **** ****** *** */
#define TOL_P      (0.10)   /* kPa * * *** ** **** ****** *** * *** ** **** ****** *** * * */
#define TOL_GAMMA  (0.002)  /* kPa/C * * *** ** **** ****** *** ** *** ** **** ****** **** */
#define TOL_E_SAT  (0.005)  /* kPa *** * * * *** ** **** ****** ** ** * * *** ** **** **** */
#define TOL_EA     (0.010)  /* kPa * * ****** * *** * * *** ** **** ****** *** * *** ** ** */

/* * ** * * *** ** * * ** ****** * * * ** *** ** * * * * * * ******* * * *** * ** * **** ***
 * TC44-TC49: wind speed, conversion to u2 (FAO56 eq. 47)
 *
 * TC44: Calc_WindSpeedAt2m - normal path, uz = 3.2 m/s, z = 10 m -> u2 = 2.393 m/s
 * TC45: Calc_WindSpeedAt2m - z = 2 m, conversion factor ≈ 1.0
 * TC46: WindSpeed_Init + Update * 3 -> min/max/mean, then Calc_WindSpeedAt2m from mean
 * TC47: NULL pointer - WindSpeed_Init, WindSpeed_Update, Calc_WindSpeedAt2m
 * TC48: STATUS_INVALID_VALUE - speed < 0, NaN, INFINITY; height outside [0.1, 200] m
 * TC49: STATUS_INVALID_VALUE - height mismatch on subsequent Update
 * ** * * *** ** * * ** ****** * * * ** *** ** * * * * * * ******* * * *** * ** * **** *** */

#define TEST_WIND_UZ_MS               (3.2)     /* m/s,  speed at z = 10 m (WMO standard)  */
#define TEST_WIND_HEIGHT_M            (10.0)    /* m,    measurement height (WMO standard) */
#define TEST_WIND_U2_EXPECTED         (2.393)   /* m/s,  u2 = 3.2 * (4.87 / ln(672.58)) ** */
#define TEST_WIND_AT2M_UZ_MS          (5.0)     /* m/s,  speed measured at 2 m *** * * * * */
#define TEST_WIND_AT2M_EXPECTED       (5.001)   /* m/s,  u2 ≈ uz at z = 2 m (factor ≈ 1) * */
#define TEST_WIND_SAMPLE_MS_LOW       (2.0)     /* m/s,  minimum sample TC46 **** * **** * */
#define TEST_WIND_SAMPLE_MS_MID       (3.0)     /* m/s,  middle  sample TC46 *** * *** * * */
#define TEST_WIND_SAMPLE_MS_HIGH      (4.0)     /* m/s,  maximum sample TC46 ***** * ***** */
#define TEST_WIND_MEAN_EXPECTED       (3.0)     /* m/s,  (2 + 4 + 3) / 3 * * * * ***** *** */
#define TEST_WIND_MEAN_U2_EXPECTED    (2.244)   /* m/s,  u2 at umean = 3.0, z = 10 m *** * */
#define TEST_WIND_BAD_HEIGHT_LOW_M    (0.05)    /* m,    below minimum 0.1 m ***** * ** ** */
#define TEST_WIND_BAD_HEIGHT_HIGH_M   (300.0)   /* m,    above maximum 200 m * * * *** *** */
#define TEST_WIND_HEIGHT_SECOND_M     (2.0)     /* m,    different height for TC49 *** *** */
#define TOL_WIND_MS                   (0.005)   /* m/s,  tolerance ***** * * * ** ****** * */

/* * ** * * *** ** * * ** ****** * * * ** *** ** * * * * * * ******* * * *** * ** * **** ***
 * TC50-TC56: reference evapotranspiration ETo (eq. 6) & ETc (eq. 56)
 *
 * TC50-TC53 use a composite scenario from verified FAO56 examples:
 *   delta = 0.1447 kPa/C (TC1, T = 20C);   Rn    = 7.6 MJ m-2 day-1 (TC29);
 *   gamma = 0.0674 kPa/C (TC36);            Tmean = 20.0C;
 *   es    = 2.3383 kPa (TC1);                ea    = 1.70 kPa (TC40), u2 = 2.0 m/s;
 *
 * TC50: G = 0 (daily), u2 = 2.0 -> ETo = 2.764 mm/day.
 * Manual calc: num = (0.408 * 0.1447 * 7.6) + (0.0674 * (900 / 293) * 2.0 * 0.6383)
 *                  = 0.4487 + 0.2643 = 0.7130
 *              den = 0.1447 + 0.0674 * 1.68 = 0.2579
 *              ETo = 0.7130 / 0.2579 = 2.764 mm/day.
 *
 * TC51: G  =  1.0 (non-zero soil heat flux)       -> ETo = 2.535 mm/day
 * TC52: u2 =  0.0 (calm)   -> only radiation term -> ETo = 2.115 mm/day
 * TC53: Rn = -5.0 (winter) -> ETo < 0             -> clamped to 0.0 mm/day
 * TC54: STATUS_NULL_POINTER & STATUS_INVALID_VALUE for Calc_ETo
 * TC55: Calc_ETo, NaN on each parameter
 * TC56: ETc = Kc * ETo = 1.15 * 2.764 = 3.179 mm/day
 * TC57: STATUS_NULL_POINTER & STATUS_INVALID_VALUE for Calc_ETc
 * TC58: Calc_ETc, NaN on each parameter
 * * ** * * *** ** * * ** ****** * * * ** *** ** * * * * * * ******* * * *** * ** * **** * */

/* Common input data for TC50-TC53 ** * * ** ****** * * * ** *** ** * * * * * * ****** * * */
#define TEST_ETO_DELTA_KPA_C         (0.1447)   /* kPa/C, from TC1 ** ** ** * ** **** ** * */
#define TEST_ETO_GAMMA_KPA_C         (0.0674)   /* kPa/C, from TC39 ** ** * * *** **** *** */
#define TEST_ETO_TMEAN_C             (20.0)     /* C ** *** * * ** ****** * ** ** * * ** * */
#define TEST_ETO_RN_MJ               (7.6)      /* MJ m-2 day-1, from TC29 ** ** * * ** ** */
#define TEST_ETO_U2_MS               (2.0)      /* m/s ** ** * * ** ****** * ** ** * * *** */
#define TEST_ETO_ES_KPA              (2.3383)   /* kPa, from TC1 ** ** * * ** **** ** ** * */
#define TEST_ETO_EA_KPA              (1.70)     /* kPa, from TC43 ** ** * * ** *** **** ** */

/* TC50: G = 0, u2 = 2.0 ** * * ** ****** * * * ** *** ** * * * * * * ****** * * ** ****** */
#define TEST_ETO_G_ZERO              (0.0)      /* MJ m-2 day-1 ** * * ** ***** * * * ** * */
#define TEST_ETO_EXPECTED            (2.764)    /* mm/day ** * * ** ****** * * * ** *** ** */

/* TC51: G ≠ 0 ** * * ** ****** * * * ** *** ** * * * * * * ****** * * ** ****** * * * *** */
#define TEST_ETO_G_NON_ZERO          (1.0)      /* MJ m-2 day-1 ** * ** * * ** ***** * * * */
#define TEST_ETO_G_NONZERO_EXPECTED  (2.535)    /* mm/day * * **** * * ** ****** * * * *** */

/* TC52: calm/Stille ** ****** * * * ** *** ** * * * * * * ****** * * ** ****** * * * ** * */
#define TEST_ETO_U2_CALM             (0.0)      /* m/s ** * * ** ****** * * * ** *** ** ** */
#define TEST_ETO_CALM_EXPECTED       (2.115)    /* mm/day ** * * ** ****** * * * ** *** ** */

/* TC53: negative Rn -> clamp ** * * ** ****** * * * ** *** ** * * * * * * ****** * * ** * */
#define TEST_ETO_RN_NEGATIVE         (-5.0)     /* MJ m-2 day-1 * * *** ** * * ** ***** ** */
#define TEST_ETO_NEGATIVE_EXPECTED   (0.0)      /* mm/day, clamped to 0 ** * * ** ****** * */

/* TC56: ETc ** * * ** ****** * * * ** *** ** * * * * * * ****** * * ** ****** * * * ** ** */
#define TEST_ETC_ETO_MM              (2.764)    /* mm/day, ETo from TC50 ** * * ** ***** * */
#define TEST_ETC_KC                  (1.15)     /* Kc, example (mid-season) *** * **** *** */
#define TEST_ETC_EXPECTED            (3.179)    /* mm/day, 2.764 * 1.15 **** * ***** * *** */

/* Tolerances * * *** ** **** ****** *** * *** ** **** ****** *** * *** ** **** ****** *** */
#define TOL_ETO                      (0.005)    /* mm/day ** *** * *** ** **** ****** * ** */
#define TOL_ETC                      (0.005)    /* mm/day ** *** * *** ** **** ****** **** */

/* Other definitions * * *** ** **** ****** *** * *** ** **** *** * *** ** **** ****** *** */
#define TEST_BRIGHT_LUX_THRESHOLD    (20000.0)  /* Illuminance threshold for testing *** * */
#define TEST_SAMPLE_PERIOD_SEC       (60U)      /* Illuminance sensor polling period ** ** */
#define TEST_EPSILON_THRESHOLD       (1e-9)     /* Permissible margin of error (10^-9) *** */
#define TEST_TOLERANCE_VALUE         (0.001)    /* Tolerance value for testing *** * *** * */

#ifdef __cplusplus
}
#endif

#endif /* TEST_CONFIG_H */
