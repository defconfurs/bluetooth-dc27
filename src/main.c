/* main.c - Application main entry point */

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

/* Generate and update beacons */
static void dc27_beacon_reset(struct i2c_status_regs *regs);
static void dc27_start_awoo(uint8_t ttl, uint16_t origin);

#define DC27_MAGIC_NONE     0x00
#define DC27_MAGIC_AWOO     0xa0
#define DC27_MAGIC_EMOTE    0xb2
#define DC27_APPEARANCE     0x27dc
#define DCFURS_MFGID_BADGE  0x71ff
#define DCFURS_MFGID_BEACON 0x0DCF

struct dc27_scan_data {
    char name[32];
    uint16_t appearance;
    uint16_t mfgid;
    uint16_t serial;
    uint8_t magic;
    uint8_t length;
    uint8_t payload[16];
};

static struct i2c_status_regs dc27_i2c_status;
static struct i2c_config_regs dc27_i2c_config;

static uint16_t dc27_badge_serial = 0xffff;

/* Update the idle beacon content. */
static void dc27_beacon_reset(struct i2c_status_regs *status)
{
    const struct bt_le_adv_param adv_param = {
        .options = 0,
        .interval_min = BT_GAP_ADV_SLOW_INT_MIN,
        .interval_max = BT_GAP_ADV_SLOW_INT_MAX,
    };

    /* Standard normally operating beacons. */
    u8_t adv_data[] = {
        (DCFURS_MFGID_BADGE >> 0) & 0xff, /* DCFurs Cheaty Vendor ID */
        (DCFURS_MFGID_BADGE >> 8) & 0xff,
        status->magic,
        (dc27_badge_serial >> 0) & 0xff, /* Serial */
        (dc27_badge_serial >> 8) & 0xff,
    };
    struct bt_data adv[] = {
        BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, (DC27_APPEARANCE & 0x00ff) >> 0, (DC27_APPEARANCE & 0xff00) >> 8),
        BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
        BT_DATA_BYTES(BT_DATA_NAME_COMPLETE, 'D', 'E', 'F', 'C', 'O', 'N', 'F', 'u', 'r', 's'),
        BT_DATA(BT_DATA_MANUFACTURER_DATA, adv_data, sizeof(adv_data))
    };

    bt_le_adv_stop();
    bt_le_adv_start(&adv_param, adv, ARRAY_SIZE(adv), NULL, 0);
}

/* Timer/Workqueue handlers for clearing special beacons. */
static void dc27_magic_work(struct k_work *work) { dc27_beacon_reset(&dc27_i2c_status); }
K_WORK_DEFINE(dc27_magic_worker, dc27_magic_work);
static void dc27_magic_expire(struct k_timer *timer_id) { k_work_submit(&dc27_magic_worker); }
K_TIMER_DEFINE(dc27_magic_timer, dc27_magic_expire, NULL);

/* Start generating an Awoo beacon */
static void dc27_start_awoo(uint8_t ttl, uint16_t origin)
{
    if (!ttl) {
        return;
    }

    const struct bt_le_adv_param awoo_param = {
        .options = 0,
        .interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
        .interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
    };

    /* Awoo beacons */
    u8_t awoo_data[] = {
        (DCFURS_MFGID_BADGE >> 0) & 0xff,
        (DCFURS_MFGID_BADGE >> 8) & 0xff,
        DC27_MAGIC_AWOO,        /* Magic */
        (dc27_badge_serial >> 0) & 0xff, /* Serial */
        (dc27_badge_serial >> 8) & 0xff,
        ttl - 1,                /* TTL */
        (origin >> 0) & 0xff,   /* Origin */
        (origin >> 8) & 0xff,
    };
    struct bt_data awoo[] = {
        BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, (DC27_APPEARANCE & 0x00ff) >> 0, (DC27_APPEARANCE & 0xff00) >> 8),
        BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
        BT_DATA_BYTES(BT_DATA_NAME_COMPLETE, 'D', 'E', 'F', 'C', 'O', 'N', 'F', 'u', 'r', 's'),
        BT_DATA(BT_DATA_MANUFACTURER_DATA, awoo_data, sizeof(awoo_data))
    };

    bt_le_adv_stop();
    bt_le_adv_start(&awoo_param, awoo, ARRAY_SIZE(awoo), NULL, 0);
    k_timer_start(&dc27_magic_timer, K_SECONDS(5), 0);
}

/* Average RSSI Tracking */
#define DC27_RSSI_INIT			-80
#define DC27_RSSI_HISTORY_LEN	512
static s32_t dc27_rssi_sum = DC27_RSSI_INIT * DC27_RSSI_HISTORY_LEN;

/* Return the average RSSI */
static s8_t dc27_rssi_average(void)
{
    return dc27_rssi_sum / DC27_RSSI_HISTORY_LEN;
}

/* Update the average RSSI */
static void dc27_rssi_insert(s8_t rssi)
{
    static unsigned int idx = 0;
    static s8_t history[DC27_RSSI_HISTORY_LEN] = {
        [0 ... (DC27_RSSI_HISTORY_LEN-1)] = DC27_RSSI_INIT
	};

    /* Update the RSSI history. */
    s8_t prev = history[idx];
    history[idx] = rssi;
    idx = (idx + 1) % DC27_RSSI_HISTORY_LEN;

    /* Update the sum */
    dc27_rssi_sum -= prev;
    dc27_rssi_sum += rssi;

    /* Update the exported RSSI measurement. */
    dc27_i2c_status.rssi = dc27_rssi_sum / DC27_RSSI_HISTORY_LEN;
}

/* Cooldown timer between emotes. */
static s64_t dc27_emote_cooldown = 60000;
static s64_t dc27_emote_short_cooldown = 0;     /* For beacons with very high RSSI */
static s64_t dc27_emote_medium_cooldown = 0;    /* For beacons with average RSSI */
static s64_t dc27_emote_long_cooldown = 0;      /* For beacons with very weak RSSI */
static s64_t dc27_awoo_cooldown = 0;            /* For beacons starting a howl */

static s64_t dc27_emote_jitter(unsigned int max)
{
    return (max/2) + sys_rand32_get() % (max/2);
}

static void dc27_emote_reset_cooldowns(void)
{
    s64_t now = k_uptime_get();
    dc27_emote_short_cooldown = now + dc27_emote_jitter(dc27_emote_cooldown);
    dc27_emote_medium_cooldown = now + dc27_emote_jitter(dc27_emote_cooldown << 1);
    dc27_emote_long_cooldown = now + dc27_emote_jitter(dc27_emote_cooldown << 2);
}

static unsigned int dc27_emote_test_cooldowns(s8_t rssi)
{
    s64_t now = k_uptime_get();
    if (((rssi + 10) < dc27_rssi_average()) && (dc27_emote_long_cooldown > now)) {
        /* Beacon is 10dB below the average RSSI, and the long cooldown has not elapsed. */
        return 0; /* Long cooldown has not elapsed */
    }
    if ((rssi < -75) && (dc27_emote_medium_cooldown > now)) {
        return 0; /* Medium cooldown has not elapsed */
    }
    if (dc27_emote_short_cooldown > now) {
        return 0; /* Short cooldown has not elapsed */
    }

    /* Reset the cooldowns */
    dc27_emote_reset_cooldowns();
    return 1;
}

#if 0
/* bloom filter of blacklisted addresses */
static uint8_t 	dc27_blacklist[1024 / 8];

static void dc27_blacklist_insert(const bt_addr_le_t *addr)
{
    uint16_t ansi = crc16_ansi(addr->a.val, sizeof(addr->a.val));
    uint16_t ccitt = crc16_ccitt(addr->a.val, sizeof(addr->a.val));
    dc27_blacklist[(ansi >> 3) % sizeof(dc27_blacklist)] |= (1 << (ansi & 0x7));
    dc27_blacklist[(ccitt >> 3) % sizeof(dc27_blacklist)] |= (1 << (ccitt & 0x7));
}

static void dc27_blacklist_check(const bt_addr_le_t *addr)
{
    uint16_t ansi = crc16_ansi(addr->a.val, sizeof(addr->a.val));
    uint16_t ccitt = crc16_ccitt(addr->a.val, sizeof(addr->a.val));
    return (dc27_blacklist[(ansi >> 3) % sizeof(dc27_blacklist)] & (1 << (ansi & 0x7)) && 
            dc27_blacklist[(ccitt >> 3) % sizeof(dc27_blacklist)] & (1 << (ccitt & 0x7)));
}
#endif

static bool dc27_scan_parse(struct bt_data *ad, void *user_data)
{
    struct dc27_scan_data *scan = (struct dc27_scan_data *)user_data;

    switch (ad->type) {
        case BT_DATA_GAP_APPEARANCE:
            if (ad->data_len != 2) {
                scan->appearance = 0xffff;
                return false;
            }
            scan->appearance = (ad->data[1] << 8) | ad->data[0];
            break;
		
        case BT_DATA_NAME_COMPLETE:
        case BT_DATA_NAME_SHORTENED:
            if (ad->data_len >= sizeof(scan->name)) {
                scan->appearance = 0xffff;
                return false;
            }
            memcpy(scan->name, ad->data, ad->data_len);
            scan->name[ad->data_len] = '\0';
            break;
		
        case BT_DATA_MANUFACTURER_DATA:
            if (ad->data_len < 3) {
                scan->appearance = 0xffff;
                return false;
            }
            scan->mfgid = (ad->data[1] << 8) | ad->data[0];
            scan->magic = ad->data[2];
            if (ad->data_len >= 5) {
                scan->serial = (ad->data[3] << 8) | ad->data[4];
            }
            if (ad->data_len > 5) {
                scan->length = ad->data_len - 5;
                if (scan->length >= sizeof(scan->payload)) {
                    scan->length = sizeof(scan->payload);
                }
                memcpy(scan->payload, &ad->data[5], scan->length);
            }
            break;
    }
    return true;
}

static void scan_cb(const bt_addr_le_t *addr, s8_t rssi, u8_t adv_type,
                    struct net_buf_simple *buf)
{
    char src[BT_ADDR_LE_STR_LEN];
    struct dc27_scan_data data;

    /* Parse the BLE advertisemnet. */
    memset(&data, 0, sizeof(data));
    data.appearance = 0xffff;
    bt_data_parse(buf, dc27_scan_parse, &data);
    if (data.appearance != DC27_APPEARANCE) {
        /* Not a part of the game - ignore it. */
        return;
    }

    bt_addr_le_to_str(addr, src, sizeof(src));

    if ((data.magic == DC27_MAGIC_EMOTE) && dc27_emote_test_cooldowns(rssi)) {
        /* Emote selection... */
        if (data.length < 2) {
            printk("rx: mfgid=0x%02x emote=random\n", data.mfgid);
        }
        else {
            char emhex[sizeof(data.payload)*2 + 1] = {'\0'};
            for (int i = 0; (i < sizeof(data.payload)) && i < data.length; i++) {
                sprintf(&emhex[i*2], "%02x", data.payload[i]);
            }
            printk("rx: mfgid=0x%02x rssi=%d emote=%s\n", data.mfgid, rssi, emhex);
        }
    }
	
    if ((data.magic == DC27_MAGIC_AWOO) && (data.mfgid == DCFURS_MFGID_BADGE) &&
        (dc27_awoo_cooldown < k_uptime_get()) && (data.length >= 3)) {
        /* Awoo beacon */
        uint8_t ttl = data.payload[0];
        uint16_t origin = (data.payload[2] << 8) | data.payload[1];
        printk("rx: awoo origin=0x%04x\n", origin);

        /* Route the magic beacon another hop */
        dc27_awoo_cooldown = k_uptime_get() + K_SECONDS(300);
        dc27_start_awoo(ttl, origin);
    }
	
#if 0
    else {
        printk("rx: mfgid=0x%02x rssi=%d avg=%d magic=%02x\n", data.mfgid, rssi, dc27_rssi_average(), data.magic);
    }
#endif

    /* Update RSSI tracking statistics */
    dc27_rssi_insert(rssi);
}

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
            dc27_emote_cooldown = strtoul(value, NULL, 0);
            dc27_emote_reset_cooldowns();
        }
        if (strcmp(name, "serial") == 0) {
            dc27_badge_serial = strtoul(value, NULL, 0);
            dc27_beacon_reset(&dc27_i2c_status);
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
            dc27_awoo_cooldown = k_uptime_get() + K_SECONDS(300);
            dc27_start_awoo(64, dc27_badge_serial);
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
    struct bt_le_scan_param scan_param = {
        .type       = BT_HCI_LE_SCAN_PASSIVE,
        .filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_DISABLE,
        .interval   = 0x0010,
        .window     = 0x0010,
    };
    int err;

    /* Setup the initial state of the I2C register space. */
    hwinfo_get_device_id(dc27_i2c_status.devid, sizeof(dc27_i2c_status.devid));
    strcpy(dc27_i2c_config.name, "DEFCON Furs");

    console_getline_init();
    i2c_slave_init(&dc27_i2c_status, &dc27_i2c_config);
    printk("Starting DC27 Badge Comms\n");

    /* Initialize the Bluetooth Subsystem */
    err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

    /* Start advertising */
    dc27_beacon_reset(&dc27_i2c_status);
    dc27_emote_reset_cooldowns();

    err = bt_le_scan_start(&scan_param, scan_cb);
    if (err) {
        printk("Starting scanning failed (err %d)\n", err);
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
            dc27_beacon_reset(&dc27_i2c_status);
        }
        else if (strcmp(name, "tx") == 0) {
            do_tx(i, args);
        }
    }
}
