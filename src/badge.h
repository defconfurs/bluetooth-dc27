
/* Status registers we export via I2C. */
struct i2c_status_regs {
    /* Static identifiers */
    uint8_t devid[8];
    /* Dynamic data received via BLE */
    int8_t      rssi;       /* Average beacon RSSI. */
    uint8_t     magic;      /* Current magic byte. */
    uint16_t    color;      /* Last received color point. */
    uint8_t     emote[8];   /* Received emote string. */
};

/* Configurable registers we export via I2C. */
struct i2c_config_regs {
    char        name[16];   /* Configure the BLE advertisement name */
};

int i2c_slave_init(struct i2c_status_regs *status, struct i2c_config_regs *config);
