// SPDX-License-Identifier: GPL-2.0
/**
 * @file ft800.h
 * @brief Internal header for the FT800 SPI + char device kernel driver
 */

#ifndef FT800_H
#define FT800_H

#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/spi/spi.h>
#include <linux/cdev.h>

#define FT800_NAME "ft800"

/** @brief ioctl magic number */
#define FT800_IOC_MAGIC     'f'

/** @brief ioctl to send a 4-byte co-processor command */
#define FT800_IOC_SEND_CMD  _IOW(FT800_IOC_MAGIC, 1, struct ft800_cmd)

/** @brief ioctl to read an 8-bit FT800 register */
#define FT800_IOC_READ_REG  _IOWR(FT800_IOC_MAGIC, 2, struct ft800_reg)

/** @brief ioctl to write an 8-bit FT800 register */
#define FT800_IOC_WRITE_REG _IOW(FT800_IOC_MAGIC, 3, struct ft800_reg)

/** @brief User-space co-processor command */
struct ft800_cmd {
	u32 addr;
	u8 data[4];
};

/** @brief User-space FT800 register access structure */
struct ft800_reg {
	u32 addr;
	u32 value;
};

/** @brief Driver state structure shared across SPI and ioctl */
struct ft800_device {
	struct spi_device *spi;
	struct mutex lock;
	struct cdev cdev;
	dev_t devt;
	struct class *class;
	struct device *device;
};

/* SPI helpers */
int ft800_spi_write_u8(struct ft800_device *ft800, u32 addr, u8 val);
int ft800_spi_write16(struct ft800_device *ft800, u32 addr, u16 val);
int ft800_spi_read_u8(struct ft800_device *ft800, u32 addr, u8 *val);

/* char device registration */
int ft800_register_chrdev(struct ft800_device *ft800);
void ft800_unregister_chrdev(struct ft800_device *ft800);

#endif /* FT800_H */

