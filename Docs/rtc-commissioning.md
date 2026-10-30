# RTC commissioning and time-setting

## Time model

The STM32F446RE RTC uses the external 32.768 kHz LSE source in 24-hour mode. The prescalers `PREDIV_A = 127` and `PREDIV_S = 255` provide a 1 Hz calendar clock.

The RTC stores UTC. `DateProvider_Read()` derives the station calendar date using `CONFIG_STATION_UTC_OFFSET_MIN`. The host time zone and daylight-saving rules do not affect the result.

`RTC_Init()` initializes and checks the RTC hardware. It does not set the date, overwrite a running calendar, or reset the backup domain.

The RTC reports one of four states:

* `RTC_STATE_CLOCK_FAULT` — the RTC clock is unavailable, stopped, or configured with an incompatible clock source.  
* `RTC_STATE_NOT_SET` — the RTC calendar has not been initialized, or the backup-domain state was lost.  
* `RTC_STATE_UNCONFIRMED` — calendar data exists, but one or more trust checks failed, including configuration, validity, or rollback checks.  
* `RTC_STATE_TRUSTED` — the calendar was explicitly set, verified by hardware read-back, and passed the firmware’s trust checks.

`RTC_Init()` returning `STATUS_OK` does not necessarily mean that the time is set. Until the state is `RTC_STATE_TRUSTED`, timestamp reads return `STATUS_UNAVAILABLE`, and the `RUN` calculation command is blocked.

The supported application time range is 2001–2099.

* * *

## Initial time-setting on STM32F446RE

1. Set the correct time on the host computer and ensure that its clock is synchronized to UTC. Network access is not required on the MCU.

2. Build and flash the firmware (see `README`’s [`“Build and run”`](../README.md#build-and-run) and [`Flashing and Debugging Workflow via OpenOCD and GDB`](flashing-routine.md)). Connect the board through its ST-LINK USB connector. The virtual COM port uses USART2 at 115200 baud, 8-N-1, without hardware flow control.

3. Create a Python virtual environment and install the utility dependency. On Linux:

   ```bash
   python3 -m venv .venv
   .venv/bin/python -m pip install -r Tools/RTC/requirements.txt
   ```

   If `venv` is unavailable, install the appropriate package for the host distribution, e.g. `python3-venv` on Debian or Ubuntu.

4. Close Minicom and any other application using the serial port. Run the utility from the repository root:

   ```bash
   .venv/bin/python Tools/RTC/rtc-set.py --port /dev/ttyACM0
   ```

   Replace `/dev/ttyACM0` with the actual device name. On Windows, use the appropriate COM port.

   The utility reads the host UTC time, sends `SET-UTC`, checks the device response and reads the RTC back. It reports an error if the RTC does not reach `RTC_STATE_TRUSTED` or if the difference from the host exceeds three seconds.

5. After the utility exits, open the serial port in a terminal and send:

   ```
   TIME
   ```

   Confirm that the response contains `RTC_STATE_TRUSTED` and a current UTC timestamp.

6. Send `RUN` in a terminal to run the daily calculation.

The Python utility and Minicom must not use the serial port simultaneously.

The MCU does not echo received command characters. Depending on the terminal’s local-echo setting, typed commands may not appear on the screen.

* * *

## Manual time-setting

The time can also be set from a UART terminal:

```
SET-UTC YYYY-MM-DDTHH:MM:SSZ
TIME
```

Replace the placeholder with the current UTC date and time. Do not enter local time without converting it to UTC.

`SET-UTC` can be used again to correct the clock or after loss of the RTC backup-domain state.

* * *

## Reset and power behavior

A normal MCU reset does not reset the RTC backup domain. If the backup domain remains powered, the time and trust marker should survive the reset.

The time and trust marker cannot be retained if both VDD and VBAT are lost. Do not assume that a Nucleo board preserves RTC time after complete power removal. A field device must provide suitable VBAT backup or set the time again after loss of the backup domain.

LSE provides a continuing time base, not absolute time synchronization. The RTC does not automatically compensate for oscillator drift. Timekeeping accuracy depends on the crystal characteristics, board design, and operating temperature. Periodic correction may be required depending on the application’s time-accuracy requirements.

* * *

## Tests

The host-side `fao56_time_test` suite checks calendar conversion, timestamp boundaries, leap years, station UTC offset, and unavailable time sources. These tests do not replace a hardware check on the target board.

On the MCU, verify initialization, explicit time-setting, and time retention across a normal MCU reset. With the UART terminal connected to USART2, first confirm that `TIME` reports `RTC_STATE_TRUSTED`, then enter:

```
RUN
```

Verify that the daily calculation executes and reports its result.

* * *

## Related documents

* [`Contracts and conventions`](contracts-and-conventions.md).  
* [`Doxygen source documentation`](https://cloclacordis.github.io/FieldEdge-Evapotranspiration/files.html).  
* [`Software architecture diagram`](software-architecture-diagram.md).  
* [`README`](../README.md).
