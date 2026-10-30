## Flashing and Debugging Workflow via OpenOCD and GDB

1. Start the OpenOCD server with the following command:
   
```bash
openocd -f board/st_nucleo_f4.cfg
```

2. In a separate terminal, start GDB with one of the following commands:

```bash
arm-none-eabi-gdb
```

or:

```bash
gdb-multiarch
```

3. Connect to the MCU via GDB:

```
(gdb) target remote localhost:3333
```

4. Reset and initialize the MCU:

```bash
(gdb) monitor reset init
```

5. Flash the firmware image to the MCU:

```bash
(gdb) monitor flash write_image erase build-mcu/fao56_mcu.elf
```

6. After flashing, perform another reset:

```bash
(gdb) monitor reset init
```

7. Resume execution of the firmware on the MCU:

```bash
(gdb) monitor resume
```
