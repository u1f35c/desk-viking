# Desk Viking

This is an STM32F103 debug tool inspired by the Bus Pirate. The aim is to have something that's easier to extend (I hate the PIC ecosystem) and a bit beefier - e.g. by having a proper USB connection instead of a USB<->UART bridge.

At present the code presents the device via 2 USB ACM devices. The first is the CLI interface, the second provides debugging information during development. The longer term plan is to present 2 interfaces (or 3 when debug is enabled) with the additional interface being a UART bridge to one of the device UARTs.

## User interface

The user interface is designed to be compatible (though not identical to) the Bus Pirate interface; primarily because I'm already familiar with that and it provides a good set of initial functionality.

## Supported protocols

The protocols currently supported are:

 * 1-Wire
 * I2C

## Building

desk-viking uses the lightweight Chopstx RT thread library. You'll need to get it as follows:

`git clone https://salsa.debian.org/gnuk-team/chopstx/chopstx.git ../chopstx`

Then configure the board you're using. My development is currently done using a Maple Mini, so I do:

`ln -s ../chopstx/board/board-maple-mini.h board.h`

If you're building on Debian you can get an appropriate cross compile via:

`sudo apt install gcc-arm-none-eabi`

After that you can build with a simple:

`make`

You'll get a `build/desk-viking.bin` file which can be flashed over a serial connection to the device's UART1 using [stm32flash](https://sourceforge.net/p/stm32flash/wiki/Home/):

`stm32flash -w build/desk-viking.bin -v /dev/ttyUSB0`

## Pinouts

The pinout configuration can be configured in `include/gpio.h`. The default maps as follows:

| Name | STM32 GPIO | Maple Mini pin |
|------|------------|----------------|
| AUX  | PB8        | 18             |
| CLK  | PB13       | 30             |
| CS   | PB12       | 31             |
| MISO | PB14       | 29             |
| MOSI | PB15       | 28             |

These pins have been chosen as they map to SPI2, which should allow for the STM32 hardware SPI engine to be used for accelerating SPI access, rather than having to bitbang it.

## TODO

This is a fledgling project and there is much to do.

### Access methods

The intent is to implement the Bus Pirate binary access method, allowing the use of tools which already support this protocol to use the Desk Viking without modification. Examples of tools that it is aimed to be compatible with include [AVRDUDE](https://www.nongnu.org/avrdude/), [flashrom](https://www.flashrom.org/Flashrom) and [OpenOCD](http://openocd.org/).

It should also be possible to support [CCLib](https://github.com/wavesoft/CCLib) in order to allow debugging of Texas Instruments' CCxxxx chips.

### Protocols

 * CC.Debugger
 * JTAG/SWD
 * SPI

## Author

* [Jonathan McDowell](https://www.earth.li/~noodles/blog/)

## License

This project is licensed under the GPL 3+ license, see [COPYING](COPYING) for details.

