# ble-firmware-dc27
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
 * source zephyr-env.sh
 * cd into the BLE firmware checkout
 * mkdir build && cd build
 * cmake -GNinja -DBOARD=dcfurs_bt832a ..
 * ninja

Bluetooth Beacon Format
-----------------------
The Bluetooth beacon format follows the #badgelife BLE communcation spec,
which establishes a GAP beacon format that is approximately standardized
between badge makers. Badges are required to set an appearance code of
0x26DC and include a complete badge name.

| Mfr ID | Magic Byte | Bytes 0-1 | Bytes 2-3     | Bytes 4+ | Description
|--------| -----------|-----------|---------------|----------|-----------
| 0x71ff | 0x00       | Serial No |               |          | Normal Operation
| 0x71ff | 0xa0       | Serial No | Origin Serial | TTL      | Awoo Beacon
| 0x71ff | 0xb3       | Serial No | RGB565 Color  | ASCII    | Emote Beacon
| 0x71ff | 0xc2       | Serial No | RGB565 Color  |          | Color suggestion beacon.