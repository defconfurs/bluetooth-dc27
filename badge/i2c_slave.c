/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <drivers/i2c.h>
#include <dt-bindings/i2c/i2c.h>
#include <nrfx_twis.h>

#include "badge.h"

#define LOG_DOMAIN "i2c_nrfx_twis"
#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(i2c_nrfx_twis);

#define I2C_REGADDR_TEXT 96

struct i2c_dcfurs_data {
    nrfx_twis_t twis;
    nrfx_twis_config_t config;
    /* Register data */
    struct i2c_regs *regs;
    const char      *text;
    size_t          text_length;
    /* Pretend to operate as an I2C EEPROM with 8-bit addressing. */
    uint8_t  bufaddr;
    uint8_t  buffer[512];
};

/* Undocumented text blurb. */
static const char i2c_text_data[] = 
    "This device complies with part 15 of the Furry Communications\n"
    "Commission Rules. Operation is subject to the following two\n"
    "conditions: (1) This device may not cause harmful awoos, and\n"
    "(2) this device must accept any hugs and scritches, including\n"
    "those that may cause undesired attention.\n";

/* I2C slave hardware configuration. */
static struct i2c_dcfurs_data i2c_data = {
    .twis = NRFX_TWIS_INSTANCE(0),
    .config = {
        .addr     = {0x42, 0},
        .scl      = DT_NORDIC_NRF_I2C_I2C_0_SCL_PIN,
        .sda      = DT_NORDIC_NRF_I2C_I2C_0_SDA_PIN,
        .scl_pull = NRF_GPIO_PIN_NOPULL,
        .sda_pull = NRF_GPIO_PIN_NOPULL,
        .interrupt_priority = NRFX_TWIS_DEFAULT_CONFIG_IRQ_PRIORITY,
    },
    .text = i2c_text_data,
    .text_length = sizeof(i2c_text_data),
    .bufaddr = 0,
    .buffer = {0},
};

/* Fill up the read buffer. */
static void fill_buffer(struct i2c_dcfurs_data *data)
{
    const uint8_t *indata = (const void *)data->regs;
    int in = data->bufaddr;
    int out = 0;

    /* Get bytes from the register map */
    while (in < sizeof(struct i2c_regs)) {
        data->buffer[out++] = indata[in++];
    }

    /* Get bytes from the text region. */
    while (in < I2C_REGADDR_TEXT) {
        data->buffer[out++] = 0x00;
        in++;
    }
    while ((in - I2C_REGADDR_TEXT) < data->text_length) {
        if (out >= sizeof(data->buffer)) return;
        data->buffer[out++] = data->text[in - I2C_REGADDR_TEXT];
        in++;
    }
    while (out < sizeof(data->buffer)) {
        data->buffer[out++] = 0;
    }
}

/* Handle any writes to the rw section. */
static void flush_write(struct i2c_dcfurs_data *data, uint32_t length)
{
    uint8_t *outdata = (void *)data->regs;
    int out = data->bufaddr;
    int in = 0;
    int end;

    /* Require at least one byte for the address, and at least one payload byte. */
    if (length < 2) return;
    length--;
    if (length > sizeof(data->buffer)) length = sizeof(data->buffer);
    end = data->bufaddr + length;

    /* Skip any bytes in the RO registers. */
    if (out < offsetof(struct i2c_regs, config)) {
        in += (offsetof(struct i2c_regs, config) - out);
        out = offsetof(struct i2c_regs, config);
    }
    /* Ignore any bytes passed the end of the RW registers. */
    if (end > sizeof(struct i2c_regs)) {
        end = sizeof(struct i2c_regs);
    }
    if (in >= end) return;

    /* Any remaining bytes must be in the RW registers, so copy them. */
    while (in < end) {
        outdata[out++] = data->buffer[in++];
    }
    /* If the txlen register was written, begin a beacon transmit. */
    if ( (data->bufaddr <= offsetof(struct i2c_regs, config.txlen)) &&
         (end > offsetof(struct i2c_regs, config.txlen)) ) {
        dc27_start_transmit();
    }
    /* Otherwise, signal that the beacon may need to be reset. */
    else {
        dc27_beacon_refresh();
    }
}

static void event_handler(nrfx_twis_evt_t const *p_event)
{
    struct i2c_dcfurs_data *data = &i2c_data;

	switch (p_event->type) {
	case NRFX_TWIS_EVT_READ_REQ:
        /* Read requested - setup the transmit buffer. */
        fill_buffer(data);
        nrfx_twis_tx_prepare(&data->twis, &data->buffer, sizeof(data->buffer));
        break;

	case NRFX_TWIS_EVT_READ_DONE:
        /* Advance the buffer address by the number of bytes read. */
        data->bufaddr += p_event->data.tx_amount;
        break;
        
	case NRFX_TWIS_EVT_WRITE_REQ:
        /* Write requested - setup the receive buffer for both the payload and the address. */
        data->bufaddr = 0;
        nrfx_twis_rx_prepare(&data->twis, &data->bufaddr, sizeof(data->bufaddr) + sizeof(data->buffer));
        break;
    
    case NRFX_TWIS_EVT_WRITE_DONE:
        /* Write completed - examine the address and copy the data out to its final home. */
        flush_write(data, p_event->data.rx_amount);
        if (p_event->data.rx_amount) {
            data->bufaddr += p_event->data.rx_amount - 1;
        }
        break;

	case NRFX_TWIS_EVT_READ_ERROR:
    case NRFX_TWIS_EVT_WRITE_ERROR:
    case NRFX_TWIS_EVT_GENERAL_ERROR:
    default:
        /* Data overrun of some kind occurred. */
		break;
	}
}

/* Attach and start I2C as an slave */
int i2c_slave_init(struct i2c_regs *regs)
{
    struct i2c_dcfurs_data *data = &i2c_data;
	nrfx_err_t result;

    i2c_data.regs = regs;

    IRQ_CONNECT(DT_NORDIC_NRF_I2C_I2C_0_IRQ_0,
            DT_NORDIC_NRF_I2C_I2C_0_IRQ_0_PRIORITY,
            nrfx_isr, nrfx_twis_0_irq_handler, 0);
                
    result = nrfx_twis_init(&data->twis, &data->config, event_handler);
	if (result != NRFX_SUCCESS) {
		LOG_ERR("Failed to initialize i2c slave device");
		return -EBUSY;
	}
	nrfx_twis_enable(&data->twis);

	return 0;
}
