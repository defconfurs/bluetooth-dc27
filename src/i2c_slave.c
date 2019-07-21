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

struct i2c_dcfurs_data {
    nrfx_twis_t twis;
    nrfx_twis_config_t config;
    /* Register data */
    const uint8_t *ro_regs;
    size_t         ro_length;
    uint8_t *rw_regs;
    size_t   rw_length;
    const char *text;
    size_t      text_length;
    /* Pretend to operate as an I2C EEPROM with 8-bit addressing. */
    uint8_t  bufaddr;
    uint8_t  buffer[256];
};

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
    .text = "Hello World",
    .bufaddr = 0,
    .buffer = {0},
};

/* Fill up the read buffer. */
static void fill_buffer(struct i2c_dcfurs_data *data)
{
    int in = data->bufaddr;
    int out = 0;

    while (in < data->ro_length) {
        data->buffer[out++] = data->ro_regs[in++];
    }
    while (in < data->ro_length + data->rw_length) {
        data->buffer[out++] = data->rw_regs[in++ - data->ro_length];
    }
    while (out < sizeof(data->buffer)) {
        data->buffer[out++] = 0;
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
        data->bufaddr += p_event->data.rx_amount;
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
int i2c_slave_init(struct i2c_status_regs *ro_regs, struct i2c_config_regs *rw_regs)
{
    struct i2c_dcfurs_data *data = &i2c_data;
	nrfx_err_t result;

    i2c_data.ro_regs = (const void *)ro_regs;
    i2c_data.ro_length = sizeof(struct i2c_status_regs);
    i2c_data.rw_regs = (void *)rw_regs;
    i2c_data.rw_length = sizeof(struct i2c_config_regs);

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
