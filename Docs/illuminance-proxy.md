# Illuminance as a proxy

## Scope

This document specifies what FAO-56’s **sunshine-duration** input actually represents, why this codebase’s illuminance-based measurement is a different physical quantity from that definition, and the current, uncalibrated state of the constants used as proxies for it.

* * *

## The WMO reference quantity

FAO-56, eq. 35 ([`SolarRadiation_Calc()`](https://cloclacordis.github.io/FieldEdge-Evapotranspiration/solar-radiation-calc_8c.html)) consumes `n`, actual sunshine duration, as one of its inputs. The WMO [`defines`](https://library.wmo.int/idviewer/68695/358) this precisely: “Sunshine duration during a given period is defined as the sum of the time for which the direct solar irradiance exceeds 120 W/m^-2” (WMO-No. 8.1 2024: 336). “Direct” is load-bearing here — the reference instrument class (from the 19th-century Campbell–Stokes recorder to modern photoelectric sunshine sensors) measures the sun’s disc through a narrow field of view, excluding diffuse skylight; direct solar irradiance is measured on a plane normal to the direction of the Sun. Even the classical Campbell–Stokes instrument’s actual burn threshold is empirically higher than the modern definition, [`reported`](https://www.mdpi.com/2073-4433/14/2/244) as 279.2 W/m^2 — the 120 W/m^2 figure itself was a 1981 CIMO [`compromise`](https://www.sciencedirect.com/science/article/abs/pii/S0263224121010575) among values proposed between 70 and 280 W/m^2, not a derived physical constant.

* * *

## What `CONFIG_BRIGHT_LUX_THRESHOLD` actually approximates

This codebase “measures” illuminance (lux) with a sensor mocked in `01-measurement/014-sunshine-lux-read`, and classifies each sample as “bright” or not based on `CONFIG_BRIGHT_LUX_THRESHOLD` (`02-providers/022-configurations/deployment-config.h`). Two gaps separate this from the WMO quantity above:

1. **Photometric vs. radiometric.** Lux is weighted by the CIE photopic luminosity function (human-eye sensitivity, peaking near 555 nm, near zero in UV and far-IR); W/m^2 is unweighted radiant power. There is no single accepted conversion factor between them — [`published`](https://www.extrica.com/article/21667) values range roughly **from 21 to 131 lm/W** (or lx per W/m^2) depending on spectral composition, with a [`cited`](https://ambientweather.com/faqs/question/view/id/1452) practical figure for bright sunlight around 120–127 lm/W.  
2. **Global vs. direct.** A standard ambient-light sensor reports global horizontal illuminance — direct beam plus diffuse skylight combined — not the isolated direct-beam quantity WMO’s definition specifies. This is not a minor distinction: diffuse daylight under a heavily overcast summer sky can exceed the direct-beam contribution under low-sun winter conditions. Consequently, a fixed global-illuminance threshold can misclassify sky or sunlight conditions across seasons.

For additional context on the comparison of different methods/algorithms for determining sunshine duration, see [`1`](https://cdn.knmi.nl/knmi/pdf/bibliotheek/knmipubWR/WR2006-06.pdf) and [`2`](https://journals.ametsoc.org/view/journals/atot/24/5/jtech2013_1.xml).

* * *

## Current configuration

| Constant | Value | Represents | Basis |
|---|---|---|---|
| `SENSOR_MOCK_INSTANT_LUX` | 55,000 lux | PC mock “direct sunlight” / “clear sky” reading | Within the commonly cited lux range for direct sunlight; not tied to a specific solar elevation. |
| `CONFIG_BRIGHT_LUX_THRESHOLD` | 20,000 lux | Bright/not-bright decision threshold | Order-of-magnitude cross-check only: 120 W/m^2 * ~125 lm/W (bright-sunlight factor) ≈ 15,000 lux, same order as the configured value. Not derived, not calibrated. Under the same simplified factor it would correspond to roughly 160 W/m^2. |

* * *

## Research in progress

Using a low-cost sensor as a proxy for the standard-specified instrument has precedent in published work in this exact domain. It does not validate any particular threshold value in this project; it establishes that “cheap proxy sensor, calibrated against the reference method” is an accepted path in this field, not an ad hoc decision.

A calibration effort and two candidate algorithmic improvements — normalizing the threshold by the already-computed extraterrestrial radiation `Ra(day)` to correct the seasonal bias described above, and a short-window illuminance-variance discriminator (call it a “dynamic threshold”), largely independent of absolute calibration — are under active investigation, alongside actual sensor-driver development for v0.2.x. Results will be reflected later here and in the release notes for the version they land in.

* * *

## Related documents

* [`README`](../README.md).  
* [`Issues v0.1.x`](issues-v01x.md).  
* [`Data flow specification`](data-flow-specification.md).
