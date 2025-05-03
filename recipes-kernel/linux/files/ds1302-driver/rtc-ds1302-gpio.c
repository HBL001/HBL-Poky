/*
 * Dallas DS1302 RTC GPIO Support
 *
 *  Copyright (C) 2002 David McCullough
 *  Copyright (C) 2003 - 2007 Paul Mundt
 *  Copyright (C) 2018 - Highland Biosciences Ltd
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License version 2. See the file "COPYING" in the main directory of
 * this archive for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/bcd.h>

#define DRV_NAME	"ds1302-gpio"
#define DRV_VERSION	"0.0.1"

#define	RTC_CMD_READ	0x81		/* Read command */
#define	RTC_CMD_WRITE	0x80		/* Write command */

#define	RTC_CMD_WRITE_ENABLE	0x00	/* Write enable */
#define	RTC_CMD_WRITE_DISABLE	0x80	/* Write disable */

#define RTC_ADDR_RAM0	0x20		/* Address of RAM0 */
#define RTC_ADDR_TCR	0x08		/* Address of trickle charge register */
#define	RTC_ADDR_CTRL	0x07		/* Address of control register */
#define	RTC_ADDR_YEAR	0x06		/* Address of year register */
#define	RTC_ADDR_DAY	0x05		/* Address of day of week register */
#define	RTC_ADDR_MON	0x04		/* Address of month register */
#define	RTC_ADDR_DATE	0x03		/* Address of day of month register */
#define	RTC_ADDR_HOUR	0x02		/* Address of hour register */
#define	RTC_ADDR_MIN	0x01		/* Address of minute register */
#define	RTC_ADDR_SEC	0x00		/* Address of second register */

#define TCC_TIME_USEC	4
#define TCH_TIME_USEC	1
#define TCL_TIME_USEC	1
#define TCWH_TIME_USEC	4

#define GPIO_LOW 0
#define GPIO_HIGH 1

struct ds1302_rtc_stc {
    int gpio_ce;
    int gpio_sclk;
    int gpio_io;

    struct rtc_device *rtc;
};

static inline int ds1302_hw_init(struct ds1302_rtc_stc *ds1302_rtc)
{
    if (gpio_is_valid(ds1302_rtc->gpio_ce) &&
        gpio_is_valid(ds1302_rtc->gpio_sclk) &&
        gpio_is_valid(ds1302_rtc->gpio_io)) {
        if (gpio_request(ds1302_rtc->gpio_ce, "ds1302_ce") ||
            gpio_direction_output(ds1302_rtc->gpio_ce, GPIO_LOW)) {
            pr_err("DS1302: CE GPIO init failed\n");
            return -1;
        }
        if (gpio_request(ds1302_rtc->gpio_sclk, "ds1302_sclk") ||
            gpio_direction_output(ds1302_rtc->gpio_sclk, GPIO_LOW)) {
            pr_err("DS1302: SCLK GPIO init failed\n");
            return -1;
        }
        if (gpio_request(ds1302_rtc->gpio_io, "ds1302_io") ||
            gpio_direction_input(ds1302_rtc->gpio_io)) {
            pr_err("DS1302: IO GPIO init failed\n");
            return -1;
        }
        return 0;
    }

    pr_err("DS1302: Invalid GPIOs\n");
    return -1;
}

static inline void ds1302_hw_release(struct ds1302_rtc_stc *ds1302_rtc)
{
    gpio_free(ds1302_rtc->gpio_ce);
    gpio_free(ds1302_rtc->gpio_sclk);
    gpio_free(ds1302_rtc->gpio_io);
}

static inline void ds1302_set_tx(struct ds1302_rtc_stc *ds)
{
    gpio_direction_output(ds->gpio_io, GPIO_LOW);
}

static inline void ds1302_set_rx(struct ds1302_rtc_stc *ds)
{
    gpio_direction_input(ds->gpio_io);
}

static inline void ds1302_clock(struct ds1302_rtc_stc *ds)
{
    gpio_set_value(ds->gpio_sclk, GPIO_HIGH);
    udelay(TCH_TIME_USEC);
    gpio_set_value(ds->gpio_sclk, GPIO_LOW);
    udelay(TCL_TIME_USEC);
}

static inline void ds1302_chip_enable_on(struct ds1302_rtc_stc *ds)
{
    gpio_set_value(ds->gpio_ce, GPIO_HIGH);
    udelay(TCC_TIME_USEC);
}

static inline void ds1302_chip_enable_off(struct ds1302_rtc_stc *ds)
{
    gpio_set_value(ds->gpio_ce, GPIO_LOW);
    udelay(TCWH_TIME_USEC);
}

static inline void ds1302_txbit(struct ds1302_rtc_stc *ds, int bit)
{
    gpio_set_value(ds->gpio_io, bit ? GPIO_HIGH : GPIO_LOW);
}

static inline int ds1302_rxbit(struct ds1302_rtc_stc *ds)
{
    return gpio_get_value(ds->gpio_io);
}

static void ds1302_sendbits(struct ds1302_rtc_stc *ds, unsigned int val)
{
    int i;
    ds1302_set_tx(ds);
    for (i = 0; i < 8; i++, val >>= 1) {
        ds1302_txbit(ds, val & 1);
        ds1302_clock(ds);
    }
}

static unsigned int ds1302_recvbits(struct ds1302_rtc_stc *ds)
{
    unsigned int val = 0;
    int i;
    ds1302_set_rx(ds);
    for (i = 0; i < 8; i++) {
        val |= ds1302_rxbit(ds) << i;
        ds1302_clock(ds);
    }
    return val;
}

static unsigned int ds1302_readbyte(struct ds1302_rtc_stc *ds, unsigned int addr)
{
    unsigned int val;
    ds1302_chip_enable_on(ds);
    ds1302_sendbits(ds, ((addr & 0x3f) << 1) | RTC_CMD_READ);
    val = ds1302_recvbits(ds);
    ds1302_chip_enable_off(ds);
    return val;
}

static void ds1302_writebyte(struct ds1302_rtc_stc *ds, unsigned int addr, unsigned int val)
{
    ds1302_chip_enable_on(ds);
    ds1302_sendbits(ds, ((addr & 0x3f) << 1) | RTC_CMD_WRITE);
    ds1302_sendbits(ds, val);
    ds1302_chip_enable_off(ds);
}

static int ds1302_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
    struct ds1302_rtc_stc *ds = dev_get_drvdata(dev);

    tm->tm_sec  = bcd2bin(ds1302_readbyte(ds, RTC_ADDR_SEC));
    tm->tm_min  = bcd2bin(ds1302_readbyte(ds, RTC_ADDR_MIN));
    tm->tm_hour = bcd2bin(ds1302_readbyte(ds, RTC_ADDR_HOUR));
    tm->tm_wday = bcd2bin(ds1302_readbyte(ds, RTC_ADDR_DAY));
    tm->tm_mday = bcd2bin(ds1302_readbyte(ds, RTC_ADDR_DATE));
    tm->tm_mon  = bcd2bin(ds1302_readbyte(ds, RTC_ADDR_MON)) - 1;
    tm->tm_year = bcd2bin(ds1302_readbyte(ds, RTC_ADDR_YEAR));
    if (tm->tm_year < 70)
        tm->tm_year += 100;

    return rtc_valid_tm(tm);
}

static int ds1302_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
    struct ds1302_rtc_stc *ds = dev_get_drvdata(dev);

    /* Enable writes */
    ds1302_writebyte(ds, RTC_ADDR_CTRL, RTC_CMD_WRITE_ENABLE);
    /* Stop RTC */
    ds1302_writebyte(ds, RTC_ADDR_SEC,
        ds1302_readbyte(ds, RTC_ADDR_SEC) | 0x80);

    ds1302_writebyte(ds, RTC_ADDR_SEC,  bin2bcd(tm->tm_sec));
    ds1302_writebyte(ds, RTC_ADDR_MIN,  bin2bcd(tm->tm_min));
    ds1302_writebyte(ds, RTC_ADDR_HOUR, bin2bcd(tm->tm_hour));
    ds1302_writebyte(ds, RTC_ADDR_DAY,  bin2bcd(tm->tm_wday));
    ds1302_writebyte(ds, RTC_ADDR_DATE, bin2bcd(tm->tm_mday));
    ds1302_writebyte(ds, RTC_ADDR_MON,  bin2bcd(tm->tm_mon + 1));
    ds1302_writebyte(ds, RTC_ADDR_YEAR, bin2bcd(tm->tm_year % 100));

    /* Start RTC */
    ds1302_writebyte(ds, RTC_ADDR_SEC,
        ds1302_readbyte(ds, RTC_ADDR_SEC) & ~0x80);

    /* Disable writes */
    ds1302_writebyte(ds, RTC_ADDR_CTRL, RTC_CMD_WRITE_DISABLE);

    return 0;
}

static int ds1302_rtc_ioctl(struct device *dev, unsigned int cmd, unsigned long arg)
{
#ifdef RTC_SET_CHARGE
    if (cmd == RTC_SET_CHARGE) {
        int tcs_val;
        struct ds1302_rtc_stc *ds = dev_get_drvdata(dev);
        if (copy_from_user(&tcs_val, (int __user *)arg, sizeof(tcs_val)))
            return -EFAULT;
        ds1302_writebyte(ds, RTC_ADDR_TCR, (0xa0 | tcs_val * 0xf));
        return 0;
    }
#endif
    return -ENOIOCTLCMD;
}

static const struct rtc_class_ops ds1302_rtc_ops = {
    .read_time   = ds1302_rtc_read_time,
    .set_time    = ds1302_rtc_set_time,
    .ioctl       = ds1302_rtc_ioctl,
};

static int __init ds1302_rtc_probe(struct platform_device *pdev)
{
    struct ds1302_rtc_stc *ds;
    const char *name = DRV_NAME;

    ds = devm_kzalloc(&pdev->dev, sizeof(*ds), GFP_KERNEL);
    if (!ds)
        return -ENOMEM;

    ds->gpio_ce   = of_get_named_gpio(pdev->dev.of_node, "ce-gpios", 0);
    ds->gpio_sclk = of_get_named_gpio(pdev->dev.of_node, "sclk-gpios", 0);
    ds->gpio_io   = of_get_named_gpio(pdev->dev.of_node, "io-gpios", 0);

    pr_info("DS1302_RTC: CE %d, SCLK %d, IO %d\n",
            ds->gpio_ce, ds->gpio_sclk, ds->gpio_io);

    if (ds1302_hw_init(ds)) {
        dev_err(&pdev->dev, "GPIO init failed\n");
        return -EINVAL;
    }

    /* Probe by writing/reading RAM0 */
    ds1302_writebyte(ds, RTC_ADDR_RAM0, 0x42);
    if (ds1302_readbyte(ds, RTC_ADDR_RAM0) != 0x42) {
        dev_err(&pdev->dev, "Probe failed\n");
        ds1302_hw_release(ds);
        return -ENODEV;
    }

    platform_set_drvdata(pdev, ds);
    ds->rtc = devm_rtc_device_register(&pdev->dev, name,
                                       &ds1302_rtc_ops, THIS_MODULE);
    if (IS_ERR(ds->rtc)) {
        ds1302_hw_release(ds);
        return PTR_ERR(ds->rtc);
    }

    return 0;
}

static const struct of_device_id ds1302_dt_ids[] = {
    { .compatible = "maxim,ds1302-gpio", },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ds1302_dt_ids);

static struct platform_driver ds1302_rtc_driver = {
    .driver = {
        .name           = DRV_NAME,
        .of_match_table = of_match_ptr(ds1302_dt_ids),
    },
};
module_platform_driver_probe(ds1302_rtc_driver, ds1302_rtc_probe);

MODULE_DESCRIPTION("Dallas DS1302 RTC GPIO driver");
MODULE_VERSION(DRV_VERSION);
MODULE_AUTHOR("Paul Mundt, David McCullough, Highland Biosciences Ltd");
MODULE_LICENSE("GPL v2");

