/*
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT microchip_mcp4725

#include <zephyr.h>
#include <drivers/i2c.h>
#include <drivers/dac.h>
#include <logging/log.h>
#include <dt-bindings/dac/mcp4725.h>

LOG_MODULE_REGISTER(dac_mcp4725, CONFIG_DAC_LOG_LEVEL);

/* Information in this file comes from MCP4725 datasheet revision D
 * found at https://ww1.microchip.com/downloads/en/DeviceDoc/22039d.pdf
 */

/* Bits that can't be set when an application wants to write to dac */
#define MCP4725_VALUE_DONT_SET_BITS			0x07FFF000

/* Defines for field values in MCP4725 DAC register */
#define MCP4725_WRITE_MODE_Mask				0x7
#define MCP4725_POWER_DOWN_Mask				0x3
#define MCP4725_DAC_VAL_Mask                            0xFFF

#define MCP4725_FAST_MODE_POWER_DOWN_Pos		4
#define MCP4725_FAST_MODE_DAC_UPPER_VAL_Pos		8
#define MCP4725_FAST_MODE_DAC_UPPER_VAL_Mask		0xF
#define MCP4725_FAST_MODE_DAC_LOWER_VAL_Mask		0xFF

#define MCP4725_WRITE_DAC_MODE_WRITE_MODE_Pos		5
#define MCP4725_WRITE_DAC_MODE_POWER_DOWN_Pos		1
#define MCP4725_WRITE_DAC_MODE_DAC_UPPER_VAL_Pos	4
#define MCP4725_WRITE_DAC_MODE_DAC_UPPER_VAL_Mask	0xFF
#define MCP4725_WRITE_DAC_MODE_DAC_LOWER_VAL_Pos	4
#define MCP4725_WRITE_DAC_MODE_DAC_LOWER_VAL_Mask	0xF0

#define MCP4725_READ_RDY_Pos				7
#define MCP4725_READ_RDY_Mask				0x1

/* After writing eeprom, the MCP4725 can be in a busy state for 25 - 50ms
 * See section 1.0 of MCP4725 datasheet, 'Electrical Characteristics'
 */
#define MCP4725_BUSY_TIMEOUT_MS                         60

struct mcp4725_config {
	const char *i2c_master_dev_name;
	uint16_t i2c_slave_addr;
};

struct mcp4725_data {
	const struct device *i2c_master;
};

/* Read mcp4725 and check RDY status bit */
static int mcp4725_wait_until_ready(const struct device *dev, uint16_t slave_addr)
{
	uint8_t rxData[5];
	bool mcp4725_ready = false;
        int32_t retVal;
	int32_t timeout = k_uptime_get_32() + MCP4725_BUSY_TIMEOUT_MS;
        int32_t startTime = k_uptime_get_32();

	/* Wait until RDY bit is set or return error if timer exceeds MCP4725_BUSY_TIMEOUT_MS */
	while (!mcp4725_ready) {
		retVal = i2c_read(dev, rxData, sizeof(rxData), slave_addr);

		if (retVal == 0) {
			mcp4725_ready = (rxData[0] >> MCP4725_READ_RDY_Pos) & MCP4725_READ_RDY_Mask;
		} else {
			/* I2C error */
			return retVal;
		}

		if (k_uptime_get_32() > timeout) {
			return -ETIMEDOUT;
		}
	}

	return 0;
}

/* MCP4725 is a single channel 12 bit DAC */
static int mcp4725_channel_setup(const struct device *dev,
				   const struct dac_channel_cfg *channel_cfg)
{
	if (channel_cfg->channel_id != 0) {
		return -EINVAL;
	}

	if (channel_cfg->resolution != 12) {
		return -ENOTSUP;
	}

	return 0;
}

static int mcp4725_write_value(const struct device *dev, uint8_t channel,
				uint32_t value)
{
	struct mcp4725_data *data = (struct mcp4725_data *)dev->data;
	const struct mcp4725_config *config = (struct mcp4725_config *)dev->config;
	uint8_t writeMode;
	uint8_t powerDownBits;
	uint16_t dacVal;
	uint32_t retVal;

	if (channel != 0) {
		return -EINVAL;
	}

	/* Check value isn't over 12 bits */
	if ((value & MCP4725_VALUE_DONT_SET_BITS) != 0) {
		return -ENOTSUP;
	}
	writeMode = (value >> MCP4725_WRITE_MODE_BIT_SHIFT) & MCP4725_WRITE_MODE_Mask;
	powerDownBits = (value >> MCP4725_POWER_DOWN_BIT_SHIFT) & MCP4725_POWER_DOWN_Mask;
	dacVal = value & MCP4725_DAC_VAL_Mask;

	/* In fast mode only the DAC value is written to the DAC register */
	if (writeMode == MCP4725_WRITE_MODE_FAST) {
		uint8_t txData[2];

		/* WRITE_MODE_FAST message format (2 bytes):
		 *
		 * ||     15 14     |      13 12      |    11 10 9 8    || 7 6 5 4 3 2 1 0 ||
		 * || Fast mode (0) | Power-down bits | DAC value[11:8] || DAC value[7:0]  ||
		 */
		txData[0] = (powerDownBits << MCP4725_FAST_MODE_POWER_DOWN_Pos) |
			((dacVal >> MCP4725_FAST_MODE_DAC_UPPER_VAL_Pos) &
                        MCP4725_FAST_MODE_DAC_UPPER_VAL_Mask);
		txData[1] = (dacVal & MCP4725_FAST_MODE_DAC_LOWER_VAL_Mask);
		retVal = i2c_write(data->i2c_master, txData, sizeof(txData), config->i2c_slave_addr);
	}
	/* In write DAC mode, the DAC value and power down select bits are written to the DAC
	 * register. In write DAC and EEPROM mode, both DAC and EEPROM are written to.
	 */
	else if (writeMode == MCP4725_WRITE_MODE_DAC ||
			writeMode == MCP4725_WRITE_MODE_DAC_AND_EEPROM) {
		uint8_t txData[3];

		/* WRITE_MODE_DAC and WRITE_MODE_DAC_AND_EEPROM message format (3 bytes):
		 *
		 * ||  23 22 21  |   20 19    |      18 17      |     16     ||
		 * || Write mode | Don't care | Power-down bits | Don't care ||
		 *
		 * | 15 14 13 12 11 10 9 8 ||    7 6 5 4     |  3 2 1 0   ||
		 * |    DAC value[11:3]    || DAC value[3:0] | Don't care ||
		 */
		txData[0] = (writeMode << MCP4725_WRITE_DAC_MODE_WRITE_MODE_Pos) |
			(powerDownBits << MCP4725_WRITE_DAC_MODE_POWER_DOWN_Pos);
		txData[1] = (dacVal >> MCP4725_WRITE_DAC_MODE_DAC_UPPER_VAL_Pos) &
			MCP4725_WRITE_DAC_MODE_DAC_UPPER_VAL_Mask;
		txData[2] = (dacVal << MCP4725_WRITE_DAC_MODE_DAC_LOWER_VAL_Pos) &
			MCP4725_WRITE_DAC_MODE_DAC_LOWER_VAL_Mask;
		if (!mcp4725_wait_until_ready(data->i2c_master, config->i2c_slave_addr)) {
			retVal = i2c_write(data->i2c_master, txData, sizeof(txData),
                                config->i2c_slave_addr);
		} else {
			/* RDY bit was never set by MCP4725 */
			retVal -EBUSY;
		}
	} else {
		/* Invalid write mode command */
		retVal = -EINVAL;
	}

	return retVal;
}

static int dac_mcp4725_init(const struct device *dev)
{
	struct mcp4725_data *data = dev->data;
	const struct mcp4725_config *config = dev->config;

	data->i2c_master = device_get_binding(
		config->i2c_master_dev_name);
	if (!data->i2c_master) {
		LOG_ERR("I2C master not found: %s",
			    config->i2c_master_dev_name);
		return -EINVAL;
	}

	/* Check we can read a 'RDY' bit from this device */
	if (mcp4725_wait_until_ready(data->i2c_master, config->i2c_slave_addr)) {
		return -EBUSY;
	}

	return 0;
}

static const struct dac_driver_api mcp4725_driver_api = {
	.channel_setup = mcp4725_channel_setup,
	.write_value = mcp4725_write_value,
};


#define INST_DT_MCP4725(index)						\
	static struct mcp4725_data mcp4725_data_##index;		\
	static const struct mcp4725_config mcp4725_config_##index = {	\
		.i2c_master_dev_name = DT_INST_BUS_LABEL(index),	\
		.i2c_slave_addr = DT_INST_REG_ADDR(index)		\
		};							\
									\
	DEVICE_DT_INST_DEFINE(index, dac_mcp4725_init,			\
			    device_pm_control_nop,			\
			    &mcp4725_data_##index,			\
			    &mcp4725_config_##index, POST_KERNEL,	\
			    CONFIG_DAC_MCP4725_INIT_PRIORITY,		\
			    &mcp4725_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INST_DT_MCP4725);
