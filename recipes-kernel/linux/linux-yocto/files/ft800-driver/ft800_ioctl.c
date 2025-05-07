// SPDX-License-Identifier: GPL-2.0
/**
 * @file ft800_ioctl.c
 * @brief Character device + ioctl interface for FT800 (/dev/ft800)
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device/class.h>
#include "ft800.h"

static int ft800_open(struct inode *inode, struct file *file)
{
	struct ft800_device *ft800 = container_of(inode->i_cdev, struct ft800_device, cdev);
	file->private_data = ft800;
	return 0;
}

static int ft800_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long ft800_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct ft800_device *ft800 = file->private_data;
	struct ft800_cmd user_cmd;
	struct ft800_reg user_reg;
	u8 val = 0;
	int ret = 0;

	if (_IOC_TYPE(cmd) != FT800_IOC_MAGIC)
		return -ENOTTY;

	mutex_lock(&ft800->lock);

	switch (cmd) {
	case FT800_IOC_SEND_CMD:
		if (copy_from_user(&user_cmd, (void __user *)arg, sizeof(user_cmd))) {
			ret = -EFAULT;
			break;
		}
		ret = spi_write(ft800->spi, user_cmd.data, sizeof(user_cmd.data));
		break;

	case FT800_IOC_READ_REG:
		if (copy_from_user(&user_reg, (void __user *)arg, sizeof(user_reg))) {
			ret = -EFAULT;
			break;
		}
		ret = ft800_spi_read_u8(ft800, user_reg.addr, &val);
		user_reg.value = val;
		if (!ret && copy_to_user((void __user *)arg, &user_reg, sizeof(user_reg)))
			ret = -EFAULT;
		break;

	case FT800_IOC_WRITE_REG:
		if (copy_from_user(&user_reg, (void __user *)arg, sizeof(user_reg))) {
			ret = -EFAULT;
			break;
		}
		ret = ft800_spi_write_u8(ft800, user_reg.addr, user_reg.value & 0xFF);
		break;

	default:
		ret = -ENOTTY;
		break;
	}

	mutex_unlock(&ft800->lock);
	return ret;
}

static const struct file_operations ft800_fops = {
	.owner = THIS_MODULE,
	.open = ft800_open,
	.release = ft800_release,
	.unlocked_ioctl = ft800_ioctl,
};

int ft800_register_chrdev(struct ft800_device *ft800)
{
	int ret;

	mutex_init(&ft800->lock);

	ret = alloc_chrdev_region(&ft800->devt, 0, 1, FT800_NAME);
	if (ret)
		return ret;

	cdev_init(&ft800->cdev, &ft800_fops);
	ft800->cdev.owner = THIS_MODULE;

	ret = cdev_add(&ft800->cdev, ft800->devt, 1);
	if (ret)
		goto unregister;

	ft800->class = class_create(FT800_NAME);
	if (IS_ERR(ft800->class)) {
		ret = PTR_ERR(ft800->class);
		goto del_cdev;
	}

	ft800->device = device_create(ft800->class, NULL, ft800->devt, NULL, FT800_NAME);
	if (IS_ERR(ft800->device)) {
		ret = PTR_ERR(ft800->device);
		goto destroy_class;
	}

	return 0;

destroy_class:
	class_destroy(ft800->class);
del_cdev:
	cdev_del(&ft800->cdev);
unregister:
	unregister_chrdev_region(ft800->devt, 1);
	return ret;
}

void ft800_unregister_chrdev(struct ft800_device *ft800)
{
	device_destroy(ft800->class, ft800->devt);
	class_destroy(ft800->class);
	cdev_del(&ft800->cdev);
	unregister_chrdev_region(ft800->devt, 1);
}

