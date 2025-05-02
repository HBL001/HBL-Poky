// SPDX-License-Identifier: GPL-2.0-only
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/of.h>

static int ft800_probe(struct spi_device *spi)
{
    dev_info(&spi->dev, "FT800 SPI device probed\n");
    return 0;
}

static int ft800_remove(struct spi_device *spi)
{
    dev_info(&spi->dev, "FT800 SPI device removed\n");
    return 0;
}

static const struct of_device_id ft800_dt_ids[] = {
    { .compatible = "bridgetek,ft800" },
    { /* sentinel */ }
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
MODULE_DESCRIPTION("Stub driver for Bridgetek FT800 SPI graphics controller");
MODULE_LICENSE("GPL");

