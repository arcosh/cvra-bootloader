
This folder implements the program bootloader_dummy.
The runs on a PC and mocks all capabilities of a regular bootloader,
which runs on a microcontroller.
A local file called "flash.bin" is used as a virtual "flash memory".
With the CAN bus it interfaces
via SocketCAN.

Later on it shall also support SLCAN-capable adapters via a serial port.
 