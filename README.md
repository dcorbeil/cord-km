# cord-km

Modules were written while following [this presentation](https://www.youtube.com/watch?v=XoYkHUnmpQo)
on Linux device driver development. The presenter's source code is available
[here](https://www.dropbox.com/scl/fi/a7b52z7shcntwdjvak1wd/ldd.tar.bz2?dl=0&e=23&rlkey=zqmtdrterhlicetijanwivg0x).

Target board: [BeagleBone Black](https://www.beagleboard.org/boards/beaglebone-black), with an LED
wired on `GPIO0 31` (pin 13 on P8 connector).

## LED toggle implementations

Five versions of a GPIO LED toggle driver, in increasing order of complexity and correctness:

| # | Module | Description |
|---|--------|-------------|
| 1 | [simplecdrv](simplecdrv/README.md) | Simulated LED via a char device. No real hardware, just prints ON/OFF |
| 2 | [mmiocdrv](mmiocdrv/README.md) | Real GPIO controlled via raw memory-mapped I/O register access |
| 3 | [gpiolibled](gpiolibled/README.md) | GPIO abstracted through GPIOlib, without the LED framework |
| 4 | [ledframework](ledframework/README.md) | GPIOlib + LED framework for proper sysfs integration and trigger support |
| 5 | [dtled](dtled/README.md) | GPIOlib + LED framework + device tree, no hardcoded GPIO. The right way |

