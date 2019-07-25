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
#include <nrf.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#include "badge.h"

struct i2c_regs dc27_i2c_regs;

static void dc27_magic_expire(struct k_timer *timer_id);
K_TIMER_DEFINE(dc27_magic_timer, dc27_magic_expire, NULL);
static void dc27_emote_expire(struct k_timer *timer_id);
K_TIMER_DEFINE(dc27_emote_timer, dc27_emote_expire, NULL);

/* Timer/Workqueue handlers for updating beacon contents. */
static void dc27_beacon_reset(struct k_work *work)
{
    struct i2c_regs *regs = &dc27_i2c_regs;
    struct bt_le_adv_param adv_param = {
        .options = 0,
        .interval_min = BT_GAP_ADV_SLOW_INT_MIN,
        .interval_max = BT_GAP_ADV_SLOW_INT_MAX,
    };
    u8_t adv_length = 0;
    u8_t adv_data[32];

    /* Stop advertising and update the beacon payload. */
    bt_le_adv_stop();

    /* Fixed #badgelife header format */
    adv_data[adv_length++] = (DCFURS_MFGID_BADGE >> 0) & 0xff;
    adv_data[adv_length++] = (DCFURS_MFGID_BADGE >> 8) & 0xff;
    adv_data[adv_length++] = regs->status.magic;

    /*
     * For any magic beacons, set the advertising rate as fast as possible, and
     * schedule a timer to disable the magic after one second. 
     */
    if (regs->status.magic != DC27_MAGIC_NONE) {
        adv_param.interval_min = BT_GAP_ADV_FAST_INT_MIN_1;
        adv_param.interval_max = BT_GAP_ADV_FAST_INT_MAX_1;
        k_timer_start(&dc27_magic_timer, K_SECONDS(1), 0);
    }

    /* Add any additional beacon contents depending on the magic byte. */
    switch (regs->status.magic) {
        case DC27_MAGIC_NONE:
            adv_data[adv_length++] = (regs->status.serial >> 0) & 0xff;
            adv_data[adv_length++] = (regs->status.serial >> 8) & 0xff;
            break;

        case DC27_MAGIC_AWOO:
            /* Stop propagation when TTL reaches zero. */
            if (regs->status.ttl == 0) return;
            adv_data[adv_length++] = (regs->status.origin >> 0) & 0xff;
            adv_data[adv_length++] = (regs->status.origin >> 8) & 0xff;
            adv_data[adv_length++] = regs->status.ttl - 1;
            break;
        
        case DC27_MAGIC_EMOTE:
            adv_data[adv_length++] = (regs->status.color >> 0) & 0xff;
            adv_data[adv_length++] = (regs->status.color >> 8) & 0xff;
            adv_data[adv_length++] = regs->status.emote[0];
            adv_data[adv_length++] = regs->status.emote[1];
            break;
        
        case DC27_MAGIC_COLOR:
            adv_data[adv_length++] = (regs->status.color >> 0) & 0xff;
            adv_data[adv_length++] = (regs->status.color >> 8) & 0xff;
            adv_data[adv_length++] = (regs->status.duration >> 0) & 0xff;
            adv_data[adv_length++] = (regs->status.duration >> 8) & 0xff;
            break;
    }

    /* Start the advertisement. */
    struct bt_data adv[] = {
        BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, (DC27_APPEARANCE & 0x00ff) >> 0, (DC27_APPEARANCE & 0xff00) >> 8),
        BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
        BT_DATA(BT_DATA_NAME_COMPLETE, regs->config.name, strnlen(regs->config.name, sizeof(regs->config.name))),
        BT_DATA(BT_DATA_MANUFACTURER_DATA, adv_data, adv_length),
    };
    bt_le_adv_start(&adv_param, adv, ARRAY_SIZE(adv), NULL, 0);
}
K_WORK_DEFINE(dc27_beacon_worker, dc27_beacon_reset);

static void do_transmit(struct k_work *work)
{
    struct i2c_regs *regs = &dc27_i2c_regs;
    struct bt_le_adv_param adv_param = {
        .options = 0,
        .interval_min = BT_GAP_ADV_FAST_INT_MIN_1,
        .interval_max = BT_GAP_ADV_FAST_INT_MAX_1,
    };
    u8_t i;
    u8_t adv_length = 0;
    u8_t adv_data[32];

    /* Stop advertising and update the beacon payload. */
    bt_le_adv_stop();
    /* Start a timer to reset the beacon after 1 second. */
    k_timer_start(&dc27_magic_timer, K_SECONDS(1), 0);

    /* Fixed #badgelife header format */
    adv_data[adv_length++] = (DCFURS_MFGID_BADGE >> 0) & 0xff;
    adv_data[adv_length++] = (DCFURS_MFGID_BADGE >> 8) & 0xff;
    adv_data[adv_length++] = regs->config.txmagic;
    /* Copy in the arbitrary payload data. */
    for (i = 0; i < regs->config.txlen; i++) {
        if (adv_length >= sizeof(adv_data)) break;
        adv_data[adv_length++] = regs->config.txdata[i];
    }

    /* Start the advertisement. */
    struct bt_data adv[] = {
        BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, (DC27_APPEARANCE & 0x00ff) >> 0, (DC27_APPEARANCE & 0xff00) >> 8),
        BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
        BT_DATA(BT_DATA_MANUFACTURER_DATA, adv_data, adv_length),
    };
    /* Move the badge name into the scan response to make room for more payload. */
    struct bt_data rsp[] = {
        BT_DATA(BT_DATA_NAME_COMPLETE, regs->config.name, strnlen(regs->config.name, sizeof(regs->config.name))),
    };
    bt_le_adv_start(&adv_param, adv, ARRAY_SIZE(adv), rsp, ARRAY_SIZE(rsp));
}
K_WORK_DEFINE(dc27_transmit_worker, do_transmit);

/* Timer function to clear out any magic beacons. */
static void dc27_magic_expire(struct k_timer *timer_id)
{
    dc27_i2c_regs.status.magic = DC27_MAGIC_NONE;
    k_work_submit(&dc27_beacon_worker);
}

/* Timer function to clear out any emotes and color suggestions. */
static void dc27_emote_expire(struct k_timer *timer_id)
{
    dc27_i2c_regs.status.color = 0;
    dc27_i2c_regs.status.duration = 0;
    dc27_i2c_regs.status.flags &= ~DC27_FLAG_EMOTE;
    memset(dc27_i2c_regs.status.emote, 0, sizeof(dc27_i2c_regs.status.emote));
}

/* Called when the beacon contents need to be refreshed. */
void dc27_beacon_refresh(void)
{
    k_work_submit(&dc27_beacon_worker);
}

/* Called to begin an arbitrary beacon transmit via the I2C regs. */
void dc27_start_transmit(void)
{
    k_work_submit(&dc27_transmit_worker);
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
    dc27_i2c_regs.status.rssi = dc27_rssi_sum / DC27_RSSI_HISTORY_LEN;
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

struct dc27_scan_data {
    char name[32];
    uint16_t appearance;
    uint16_t mfgid;
    uint16_t serial;
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
    bt_data_parse(buf, dc27_scan_parse, &data);
    if (data.appearance != DC27_APPEARANCE) {
        /* Not a part of this year's convention. */
        return;
    }

    dc27_rssi_insert(rssi);
    bt_addr_le_to_str(addr, src, sizeof(src));
    
    /* Handle special beacons. */
    switch (data.magic) {
        case DC27_MAGIC_EMOTE:
            if (!dc27_emote_test_cooldowns(rssi)) break;
            if (data.length < 2) break;
            dc27_i2c_regs.status.color = (data.payload[1] << 8) | data.payload[0];
            dc27_i2c_regs.status.flags |= DC27_FLAG_EMOTE;
            /* TODO: Parse the desired emote. */
            memset(dc27_i2c_regs.status.emote, 0, sizeof(dc27_i2c_regs.status.emote));
            k_timer_start(&dc27_emote_timer, K_SECONDS(5), 0);
            break;
        
        case DC27_MAGIC_COLOR:
            /* Only accept color suggestion from DCFurs beacons.  */
            /* TODO: Do we want any signing or obfuscation to prevent abuse? */
            /* A cooldown would prevent coordinated lighting effects. */
            if (data.mfgid != DCFURS_MFGID_BEACON) break;
            if (data.length < 4) break;
            else {
                /* Require at least 500ms for color suggestion. */
                uint16_t duration = (data.payload[3] << 8) | data.payload[2];
                if (duration < 500) duration = 500;
                dc27_i2c_regs.status.duration = duration;
                dc27_i2c_regs.status.color = (data.payload[1] << 8) | data.payload[0];
                k_timer_start(&dc27_emote_timer, duration, 0);
            }
            break;
        
        case DC27_MAGIC_AWOO:
            /* Filter out unwanted awoo's */
            if (data.mfgid != DCFURS_MFGID_BADGE) break;
            if (dc27_awoo_cooldown > k_uptime_get()) break;
            if (data.length < 3) break;

            /* Route the magic beacon another hop */
            dc27_awoo_cooldown = k_uptime_get() + K_SECONDS(300);
            dc27_i2c_regs.status.magic = DC27_MAGIC_AWOO;
            dc27_i2c_regs.status.origin = (data.payload[1] << 8) | data.payload[0];
            dc27_i2c_regs.status.ttl = data.payload[0];
            dc27_beacon_refresh();
            break;
    }

#if 0
    printk("rx: mfgid=0x%02x rssi=%d avg=%d magic=%02x\n", data.mfgid, rssi, dc27_rssi_average(), data.magic);
#endif
}

int dc26_beacon_init(void)
{
    struct bt_le_scan_param scan_param = {
        .type       = BT_HCI_LE_SCAN_PASSIVE,
        .filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_DISABLE,
        .interval   = 0x0010,
        .window     = 0x0010,
    };

    /* Initialize the Bluetooth Subsystem */
    int err = bt_enable(NULL);
    if (err) return err;

    err = bt_le_scan_start(&scan_param, scan_cb);
    if (err) {
        printk("Starting scanning failed (err %d)\n", err);
        return err;
    }

    /* Start advertising */
    dc27_emote_reset_cooldowns();
    dc27_beacon_refresh();

    /* Success */
    return 0;
}
