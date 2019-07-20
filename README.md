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
