/*
 * Copyright (c) 2019 DEFCON Furs
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

/* #badgelife/DCFurs beacon constants */
#define DC27_MAGIC_NONE     0x00
#define DC27_MAGIC_AWOO     0xa0
#define DC27_MAGIC_EMOTE    0xb2
#define DC27_MAGIC_COLOR    0xc3
#define DC27_MAGIC_ACCESS   0x6c
#define DC27_MAGIC_REPLY    0x7e
#define DC27_APPEARANCE     0x27dc
#define DCFURS_MFGID_BADGE  0x71ff
#define DCFURS_MFGID_BEACON 0x0DCF

#define LED_PORT DT_ALIAS_LED0_GPIOS_CONTROLLER
#define LED	DT_ALIAS_LED0_GPIOS_PIN

#define BLINK_TIME  K_SECONDS(1)
#define SLEEP_TIME 	K_SECONDS(5)

/* TODO: Put these configuration knobs somewhere. */
#define EMOTE_COLOR	0
#define BEACON_NAME "DCFurs Fox"
#define ACCESS_CODE "WylDf0x"
#define ACCESS_SUCCESS "qKAypz5uoJH6Ezk1MzMvLJkf"
#define ACCESS_DENIED  "Access Denied"

/* Timer/Workqueue handlers for updating beacon contents. */
static void dc27_beacon_reset(struct k_work *work)
{
    struct bt_le_adv_param adv_param = {
        .options = BT_LE_ADV_OPT_USE_IDENTITY,
        .interval_min = BT_GAP_ADV_SLOW_INT_MIN,
        .interval_max = BT_GAP_ADV_SLOW_INT_MAX,
    };
    u8_t adv_data[] = {
		(DCFURS_MFGID_BEACON >> 0) & 0xff,
		(DCFURS_MFGID_BEACON >> 8) & 0xff,
		DC27_MAGIC_EMOTE,
        0x00, /* TODO: Color selection */
        /* 'Q', 'X' */
	};
    struct bt_data adv[] = {
        BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, (DC27_APPEARANCE & 0x00ff) >> 0, (DC27_APPEARANCE & 0xff00) >> 8),
        BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
        BT_DATA(BT_DATA_NAME_COMPLETE, BEACON_NAME, strlen(BEACON_NAME)),
        BT_DATA(BT_DATA_MANUFACTURER_DATA, adv_data, sizeof(adv_data)),
    };

    /* Stop advertising and update the beacon payload. */
    bt_le_adv_stop();
    bt_le_adv_start(&adv_param, adv, ARRAY_SIZE(adv), NULL, 0);
}
K_WORK_DEFINE(dc27_beacon_worker, dc27_beacon_reset);

static void dc27_beacon_expire(struct k_timer *timer_id) { k_work_submit(&dc27_beacon_worker); }
K_TIMER_DEFINE(dc27_beacon_timer, dc27_beacon_expire, NULL);

static void dc27_beacon_access(const u8_t *token, u8_t length)
{
    static s64_t cooldown = 0;

    struct bt_le_adv_param adv_param = {
        .options = BT_LE_ADV_OPT_USE_IDENTITY,
        .interval_min = BT_GAP_ADV_FAST_INT_MIN_1,
        .interval_max = BT_GAP_ADV_FAST_INT_MAX_1,
    };
    struct bt_data adv[] = {
        BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, (DC27_APPEARANCE & 0x00ff) >> 0, (DC27_APPEARANCE & 0xff00) >> 8),
        BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
        BT_DATA(BT_DATA_NAME_COMPLETE, BEACON_NAME, strlen(BEACON_NAME)),
    };
    u8_t rsp_data[31];
    u8_t rsp_length = 0;
    s64_t now = k_uptime_get();

    /* Rate limit to one response every 3 seconds. */
    if (cooldown > now) {
        return;
    }
    cooldown = now + K_SECONDS(3);
    k_timer_start(&dc27_beacon_timer, K_SECONDS(1), 0);

    /* Build the access reply beacon payload. */
	rsp_data[rsp_length++] = (DCFURS_MFGID_BEACON >> 0) & 0xff;
	rsp_data[rsp_length++] = (DCFURS_MFGID_BEACON >> 8) & 0xff;
	rsp_data[rsp_length++] = DC27_MAGIC_REPLY;
	if (length == strlen(ACCESS_CODE) && (memcmp(token, ACCESS_CODE, length) == 0)) {
        /* Access is granted - send the ciphertext. */
        memcpy(rsp_data + rsp_length, ACCESS_SUCCESS, strlen(ACCESS_SUCCESS));
        rsp_length += strlen(ACCESS_SUCCESS);
    }
    else {
        /* Access is denied - send a nack message. */
        memcpy(rsp_data + rsp_length, ACCESS_DENIED, strlen(ACCESS_DENIED));
        rsp_length += strlen(ACCESS_DENIED);
    }
    
    /* Stop advertising and update the beacon payload. */
    struct bt_data rsp[] = { BT_DATA(BT_DATA_MANUFACTURER_DATA, rsp_data, rsp_length) };
    bt_le_adv_stop();
    bt_le_adv_start(&adv_param, adv, ARRAY_SIZE(adv), rsp, ARRAY_SIZE(rsp));
}

struct dc27_scan_data {
    char name[32];
    uint16_t appearance;
    uint16_t mfgid;
    uint8_t magic;
    uint8_t length;
    uint8_t payload[16];
};

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
            scan->length = ad->data_len - 3;
            if (scan->length) {
                if (scan->length >= sizeof(scan->payload)) {
                    scan->length = sizeof(scan->payload);
                }
                memcpy(scan->payload, &ad->data[3], scan->length);
            }
            break;
    }
    return true;
}

static void scan_cb(const bt_addr_le_t *addr, s8_t rssi, u8_t adv_type, struct net_buf_simple *buf)
{
    struct dc27_scan_data data;

    /* Parse the BLE advertisemnet. */
    memset(&data, 0, sizeof(data));
    bt_data_parse(buf, dc27_scan_parse, &data);
    if (data.appearance != DC27_APPEARANCE) {
        /* Not a part of this year's convention. */
        return;
    }

    /* Handle the badge challenge data */
    if (data.magic == DC27_MAGIC_ACCESS) {
        dc27_beacon_access(data.payload, data.length);
    }
}

void main(void)
{
	struct device *dev = device_get_binding(LED_PORT);

    struct bt_le_scan_param scan_param = {
        .type       = BT_HCI_LE_SCAN_PASSIVE,
        .filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_DISABLE,
        .interval   = 0x0010,
        .window     = 0x0010,
    };

    /* Initialize the Bluetooth Subsystem */
    int err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }
    err = bt_le_scan_start(&scan_param, scan_cb);
    if (err != 0) {
        printk("Starting scanning failed (err %d)\n", err);
        return;
    }
    else {
        /* Start advertising */
        k_work_submit(&dc27_beacon_worker);
    }

	/* Set LED pin as output */
	gpio_pin_configure(dev, LED, GPIO_DIR_OUT);
	gpio_pin_write(dev, LED, 0);

	while (1) {
		gpio_pin_write(dev, LED, 1);
		k_sleep(BLINK_TIME);

		gpio_pin_write(dev, LED, 0);
		k_sleep(SLEEP_TIME);
	}
}
