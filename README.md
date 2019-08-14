# bluetooth-dc27
Bluetooth Firmware for the DC27 DEFCON Furs badge

On my Ubuntu 16.04 development machine, I needed to install the following
prerequisites to build the BLE firmware:
 * pip3 install --upgrade cmake
 * apt-get install ninja
 * apt-get install gperf
 * sudo ln -s /usr/bin/ninja /usr/sbin/ninja
 * export GNUARMEMB_TOOLCHAIN_PATH=/usr
 * export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
 * Download and install device-tree-compiler package from Ubuntu cosmic.

Build the badge BLE firmware:
 * export GNUARMEMB_TOOLCHAIN_PATH=/usr
 * ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
 * cd into your zephyr source checkout
 * apply add-i2c-slave-hal.patch to your zephyr sources.
 * source zephyr-env.sh
 * cd into the BLE firmware checkout
 * mkdir build && cd build
 * cmake -GNinja ..
 * ninja

This code is not exactly my finest work, it was the product of a last minute
hackathon, as per usual with all DEFCON projects. My appologies for any
bugs or missing documentation.

Bluetooth Beacon Format
-----------------------
The Bluetooth beacon format follows the #badgelife BLE communcation spec,
which establishes a GAP beacon format that is approximately standardized
between badge makers. Badges are required to set an appearance code of
0x27DC and include a complete badge name.

| Mfr ID | Magic Byte | Byte 0    | Byte 1    | Bytes 3   | Description
|--------| -----------|-----------|-----------|-----------|-----------
| 0x71ff | 0x00       | Badge ID  | Badge ID  |           | Normal Operation
| 0x71ff | 0xa0       | Origin ID | Origin ID |           | Awoo Beacon
| 0x71ff | 0xb3       | Color Hue | ASCII     | ASCII     | Emote Beacon
| 0x71ff | 0xc2       | Color Hue | Time msec | Time msec | Color suggestion beacon.

The badge ID is calculated as an 16-bit CCITT CRC of the bluetooth module's
hardware device ID.

The color hue is in units of 3 degrees (eg: 0=red, 40=green, 80=blue, 120=red
and etc), with the values 0x7e reserved for white, and 0x7f to let the badge
select a random color.

The color suggestion beacon was planned for coordinated lighting effects from
the DJ console, but was never implemented in the badge and animation codebase
because we ran out of time.
