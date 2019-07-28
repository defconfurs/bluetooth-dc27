#ifndef DCFURS_INCLUDE_BADGE_H_
#define DCFURS_INCLUDE_BADGE_H_

#include <sys/types.h>
#include <stdint.h>

/* Status registers we export via I2C. */
struct i2c_regs {
    /* Status Registers - treated as read-only. */
    struct {
        /* Static identifiers */
        uint8_t     devid[8];
        uint16_t    serial;
        /* Dynamic data received via BLE */
        uint8_t     magic;      /* Outgoing magic byte. */
        uint8_t     flags;      /* DC27_FLAG_xxxx */
        uint16_t    color;      /* Suggested color point (rgb565). */
        uint16_t    duration;   /* Suggested color duration (msec). */
        int8_t      rssi;       /* Average beacon RSSI. */
        uint8_t     ttl;        /* TTL for routed messages. */
        uint16_t    origin;     /* Originator for current messages. */
        uint8_t     emote[2];   /* Received emote string. */
        uint8_t     __pad0[10];
    } status;

    /* Config Registers - treated as read-write. */
    struct {
        char        name[12];   /* BLE advertisement name */
        uint8_t     __pad1[2];
        uint8_t     txlen;
        uint8_t     txmagic;
        uint8_t     txdata[32];
    } config;
};

/* Status Register Flags */
#define DC27_FLAG_EMOTE     (1 << 0)
#define DC27_FLAG_COLOR     (1 << 1)

/* #badgelife/DCFurs beacon constants */
#define DC27_MAGIC_NONE     0x00
#define DC27_MAGIC_AWOO     0xa0
#define DC27_MAGIC_EMOTE    0xb2
#define DC27_MAGIC_COLOR    0xc3
#define DC27_APPEARANCE     0x27dc
#define DCFURS_MFGID_BADGE  0x71ff
#define DCFURS_MFGID_BEACON 0x0DCF

void dc27_beacon_refresh(void);
void dc27_start_transmit(void);
void dc27_start_awoo(uint8_t ttl, uint16_t origin);
int dc26_beacon_init(void);
int i2c_slave_init(struct i2c_regs *regs);

extern struct i2c_regs dc27_i2c_regs;

#endif /* DCFURS_INCLUDE_BADGE_H_ */
