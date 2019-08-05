/*
 * Copyright (c) 2019 DEFCON Furs
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <misc/printk.h>
#include <misc/util.h>
#include <drivers/hwinfo.h>
#include <kernel.h>
#include <console.h>
#include <sys/crc.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

/* For UICR/Rabies tag programming */
#include <nrf.h>
#include <system_nrf51.h>

#include "badge.h"

static void
do_set(int argc, char **argv)
{
    int i;
    for (i = 0; i < argc; i++) {
        /* In case the value is optional. */
        char *name = argv[i];
        char *value = strchr(argv[i], '=');
        if (value) *value++ = '\0';

        if (strcmp(name, "cooldown") == 0) {
            //dc27_emote_cooldown = strtoul(value, NULL, 0);
            //dc27_emote_reset_cooldowns();
        }
        if (strcmp(name, "serial") == 0) {
            dc27_i2c_regs.status.serial = strtoul(value, NULL, 0);
            dc27_beacon_refresh();
        }
    }
}

static void
do_tx(int argc, char **argv)
{
    int i;
     for (i = 0; i < argc; i++) {
        /* In case the value is optional. */
        char *name = argv[i];
        char *value = strchr(argv[i], '=');
        if (value) *value++ = '\0';

        if (strcmp(name, "awoo") == 0) {
            //dc27_awoo_cooldown = k_uptime_get() + K_SECONDS(300);
            //dc27_start_awoo(64, dc27_i2c_status.serial);
        }
    }
}

static void
do_echo(int argc, char **argv)
{
    int i;
    printk("echo:");
    for (i = 0; i < argc; i++) {
        printk(" %s", argv[i]);
    }
            printk("\n");
}

void
main(void)
{
    int err;

    /* Setup the initial state of the I2C register space. */
    hwinfo_get_device_id(dc27_i2c_regs.status.devid, sizeof(dc27_i2c_regs.status.devid));
    dc27_i2c_regs.status.serial = crc16_ccitt(0, dc27_i2c_regs.status.devid, sizeof(dc27_i2c_regs.status.devid));
    strcpy(dc27_i2c_regs.config.name, "DEFCON Furs");
    dc27_i2c_regs.config.cooldown = 60;

    console_getline_init();
    i2c_slave_init(&dc27_i2c_regs);

    printk("Starting DC27 Badge Comms\n");
    err = dc27_beacon_init();
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

    /* Run the console to handle input from the host. */
    while (1) {
        char *s = console_getline();
        char *name;
        char *args[16];
        int i;

        /* Each line shouold conform to the sytax action: name=value ... */
        name = strtok(s, ":");
        if (!name) {
            continue;
        }
        for (i = 0; i < sizeof(args)/sizeof(args[0]); i++) {
            args[i] = strtok(NULL, " ");
            if (!args[i]) break;
        }
        /* Some functions that we might care to run. */
        if (strcmp(name, "echo") == 0) {
            do_echo(i, args);
        }
        else if (strcmp(name, "set") == 0) {
            do_set(i, args);
            dc27_beacon_refresh();
        }
        else if (strcmp(name, "tx") == 0) {
            do_tx(i, args);
        }
    }
}
