# CAN Bootloader

[![Build Status](https://travis-ci.org/cvra/can-bootloader.svg?branch=master)](https://travis-ci.org/cvra/can-bootloader)

This repository contains source the code for the Controller Area Network (CAN) bootloader,
which is running on every microcontroller in our robots.
It allows us to quickly update the firmware on all (>20) boards
without disassembly or additional electrical connections.

# Configuration pages

The bootloader is the first program to be executed on an embedded sytem upon startup.
The bootloader code is followed by two flash pages,
each containing a bootloader configuration struct.
The two pages are for redundancy.
A checksum (CRC32) is present at the beginning of each page
and checked by the bootloader.
In case an invalid page is detected,
it's content is replaced by the redundant page.

After the bootloader has updated one of the two configuration pages,
it verifies it before proceeding to the second one.
This ensures that there is always a valid configuration page to prevent bricking a board.

The config contains the following informations,
stored as a [MessagePack](https://msgpack.org/) map:

* **ID**: Unique node identifier. Value ranges from 1 to 127.
* **name**: Human readable node name (e.g. "arms.left.shoulder"). Maximum length: 64 chars.
* **device_class**: Board model and revision (e.g. "CVRA.MotorController.v1"). Maximum length: 64 chars.
* **application_crc**: Application checksum. If the checksum matches the image, the bootloader will boot into the application after a timeout.
* **application_size**: Needed for checksum calculation.
* **update_count**: Number of firmware updates so far. Used for diagnostics and lifespan estimation. Can explicitly be set when updating the config, otherwise it's incremented by the bootloader when flashing a firmware image.

# Performance considerations

Assuming :

* CAN speed is 1 Mb/s
* CAN overhead is about 50%
* Protocol overhead + data drop is about 10%.
    The size of a write chunk might affect this since invalid data is dropped by chunk.
* Binary size: 1 MB
* The bottleneck is in the CAN network, not in the CRC computation or in the flash write speed.
* The time to check the CRC of each board can be neglected.
* We use multicast write commands to lower bandwith usage.

We can flash a whole board (1MB) in about 20 seconds.
If all board in the robot run the same firmware, this is the time required to do a full system update!

# Safety features

The bootloader is expected to be one of the safest part of the robot firmware.
Correcting a bug in the bootloader could be very complicated,
requiring disassembly of the robot in the worst cases.
Therefore, when implementing the bootloader or the associated protocol,
the following safety points must be taken into account:

* The bootloader must *never* erase itself or its configuration page.
* It should never write to flash if the device class does not match. Doing so might result in the wrong firmware being written to the board, which is dangerous.
* If the application CRC does not match, the bootloader should not boot it.
* On power up the bootloader should wait enough time for the user to input commands before jumping to the application code.

# How to build

1. Run [CVRA's packager script](https://github.com/cvra/packager): `packager`.
2. Build libopencm3: `pushd libopencm3 && make && popd`.
3. Build your desired platform: `cd platform/motor-board-v1 && make`.
4. Flash the resulting binary to your board: `make flash`.

# Protocol

The protocol is described in [PROTOCOL.markdown](./PROTOCOL.markdown).

# License

See [LICENSE.md](./LICENSE.md).
