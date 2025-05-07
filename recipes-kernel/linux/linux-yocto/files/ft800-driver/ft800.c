// SPDX-License-Identifier: GPL-2.0
/**
 * @file ft800.c
 * @brief SPI + display initialization logic for the Bridgetek FT800 graphics controller
 */

#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/of.h>
#include "ft800.h"

/* Register definitions */
#define FT800_REG_ID           0x102400
#define FT800_REG_HCYCLE       0x102428
#define FT800_REG_HOFFSET      0x10242C
#define FT800_REG_HSYNC0       0x102430
#define FT800_REG_HSYNC1       0x102434
#define FT800_REG_VCYCLE       0x102438
#define FT800_REG_VOFFSET      0x10243C
#define FT800_REG_VSYNC0       0x102440
#define FT800_REG_VSYNC1       0x102444
#define FT800_REG_HSIZE        0x102448
#define FT800_REG_VSIZE        0x10244C
#define FT800_REG_PCLK_POL     0x102450
#define FT800_REG_SWIZZLE      0x102454
#define FT800_REG_DLSWAP       0x10245C
#define FT800_REG_CSPREAD      0x102464
#define FT800_REG_GPIO_DIR     0x102490
#define FT800_REG_GPIO         0x102094
#define FT800_REG_PCLK         0x10246C

#define FT800_CMD_ACTIVE       0x00
#define FT800_EXPECTED_ID      0x7C

/**
 * @brief Send a host command to the FT800
 */
static int ft800_send_host_command(struct spi_device *spi, u8 cmd)
{
	u8 tx[3] = { cmd, 0x00, 0x00 };
	return spi_write(spi, tx, sizeof(tx));
}

/**
 * @brief Read 8-bit register from FT800
 */
int ft800_spi_read_u8(struct ft800_device *ft800, u32 addr, u8 *val)
{
	u8 tx[4] = {
		(addr >> 16) & 0x3F,
		(addr >> 8) & 0xFF,
		addr & 0xFF,
		0
	};
	u8 rx;
	int ret = spi_write_then_read(ft800->spi, tx, sizeof(tx), &rx, 1);
	if (ret == 0)
		*val = rx;
	return ret;
}
EXPORT_SYMBOL_GPL(ft800_spi_read_u8);

/**
 * @brief Write 8-bit register to FT800
 */
int ft800_spi_write_u8(struct ft800_device *ft800, u32 addr, u8 val)
{
	u8 tx[4] = {
		(addr >> 16) & 0x3F,
		(addr >> 8) & 0xFF,
		addr & 0xFF,
		val
	};
	return spi_write(ft800->spi, tx, sizeof(tx));
}
EXPORT_SYMBOL_GPL(ft800_spi_write_u8);

/**
 * @brief Write 16-bit register to FT800
 */
int ft800_spi_write16(struct ft800_device *ft800, u32 addr, u16 val)
{
	u8 tx[5] = {
		(addr >> 16) & 0x3F,
		(addr >> 8) & 0xFF,
		addr & 0xFF,
		val & 0xFF,
		(val >> 8) & 0xFF
	};
	return spi_write(ft800->spi, tx, sizeof(tx));
}
EXPORT_SYMBOL_GPL(ft800_spi_write16);

/**
 * @brief SPI probe function - initializes FT800
 */
static int ft800_probe(struct spi_device *spi)
{
	struct ft800_device *ft800;
	u8 id = 0;
	int ret, i;

	ft800 = devm_kzalloc(&spi->dev, sizeof(*ft800), GFP_KERNEL);
	if (!ft800)
		return -ENOMEM;

	ft800->spi = spi;
	spi_set_drvdata(spi, ft800);

	dev_info(&spi->dev, "FT800 probe: sending CMD_ACTIVE\n");
	ret = ft800_send_host_command(spi, FT800_CMD_ACTIVE);
	if (ret)
		return dev_err_probe(&spi->dev, ret, "CMD_ACTIVE failed\n");

	msleep(20);

	for (i = 0; i < 100; i++) {
		ft800_spi_read_u8(ft800, FT800_REG_ID, &id);
		if (id == FT800_EXPECTED_ID)
			break;
		msleep(10);
	}

	if (id != FT800_EXPECTED_ID)
		return dev_err_probe(&spi->dev, -ENODEV, "FT800 REG_ID mismatch: 0x%02x\n", id);

	dev_info(&spi->dev, "FT800 detected, REG_ID = 0x%02x\n", id);

	/* Display register configuration (for 480x272 LCD) */
	ft800_spi_write16(ft800, FT800_REG_HCYCLE, 548);
	ft800_spi_write16(ft800, FT800_REG_HOFFSET, 43);
	ft800_spi_write16(ft800, FT800_REG_HSYNC0, 0);
	ft800_spi_write16(ft800, FT800_REG_HSYNC1, 41);
	ft800_spi_write16(ft800, FT800_REG_VCYCLE, 292);
	ft800_spi_write16(ft800, FT800_REG_VOFFSET, 12);
	ft800_spi_write16(ft800, FT800_REG_VSYNC0, 0);
	ft800_spi_write16(ft800, FT800_REG_VSYNC1, 10);
	ft800_spi_write16(ft800, FT800_REG_HSIZE, 480);
	ft800_spi_write16(ft800, FT800_REG_VSIZE, 272);
	ft800_spi_write16(ft800, FT800_REG_SWIZZLE, 0);
	ft800_spi_write16(ft800, FT800_REG_PCLK_POL, 1);
	ft800_spi_write16(ft800, FT800_REG_CSPREAD, 1);
	ft800_spi_write16(ft800, FT800_REG_DLSWAP, 2);
	ft800_spi_write16(ft800, FT800_REG_GPIO_DIR, 0x80);
	ft800_spi_write16(ft800, FT800_REG_GPIO, 0x80);
	ft800_spi_write16(ft800, FT800_REG_PCLK, 5); // Enable pixel clock

	ret = ft800_register_chrdev(ft800);
	if (ret)
		return dev_err_probe(&spi->dev, ret, "Failed to register /dev/ft800\n");

	dev_info(&spi->dev, "FT800 /dev/ft800 ready\n");
	return 0;
}

/**
 * @brief SPI remove function
 */
static void ft800_remove(struct spi_device *spi)
{
	struct ft800_device *ft800 = spi_get_drvdata(spi);
	ft800_unregister_chrdev(ft800);
	dev_info(&spi->dev, "FT800 driver removed\n");
}

static const struct of_device_id ft800_dt_ids[] = {
	{ .compatible = "bridgetek,ft800" },
	{ }
};
MODULE_DEVICE_TABLE(of, ft800_dt_ids);

static struct spi_driver ft800_driver = {
	.driver = {
		.name = "ft800",
		.of_match_table = ft800_dt_ids,
	},
	.probe = ft800_probe,
	.remove = ft800_remove,
};

module_spi_driver(ft800_driver);

MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("FT800 SPI Display Driver with Char Device Interface");
MODULE_LICENSE("GPL");

