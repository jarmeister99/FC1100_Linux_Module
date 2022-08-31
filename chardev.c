#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/iomap.h>
#include <linux/mutex.h>
 
#include "chardev.h"

#define MAX_DEV 1
#define DEV_MAJOR 1

static struct mutex etx_mutex;

static int fc1100_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	add_uevent_var(env, "DEVMODE=%#o", 0666);

	return 0;
}

static int fc1100_dev_open(struct inode *inode, struct file *file);
static int fc1100_dev_release(struct inode *inode, struct file *file);
static long fc1100_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static ssize_t fc1100_dev_read(struct file *file, char __user *buf, size_t count, loff_t *offset);
static ssize_t fc1100_dev_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);
static loff_t fc1100_llseek(struct file *file, loff_t offset, int whence);

static const struct file_operations fc1100_dev_fops = {
	.owner      = THIS_MODULE,
	.open       = fc1100_dev_open,
	.release    = fc1100_dev_release,
	.unlocked_ioctl = fc1100_dev_ioctl,
	.read       = fc1100_dev_read,
	.write       = fc1100_dev_write,
	.llseek	 = fc1100_llseek,
};

struct fc1100_device_data {
	struct device* fc100_dev;
	struct cdev cdev;
};

struct fc1100_device_private {
	uint8_t chnum;
	struct fc1100_driver* drv;
};

static int dev_major = 0;
static struct class *fc1100_class = NULL;
static struct fc1100_device_data fc1100_dev_data[MAX_DEV];
static struct fc1100_driver* drv_access = NULL;

int create_char_devs(struct fc1100_driver* drv)
{
	int err, i;
	dev_t dev;

	mutex_init(&etx_mutex);

	err = alloc_chrdev_region(&dev, 0, MAX_DEV, "fc1100");

	dev_major = MAJOR(dev);

	fc1100_class = class_create(THIS_MODULE, "fc1100-dev");

	fc1100_class->dev_uevent = fc1100_uevent;

	for (i = 0; i < MAX_DEV; i++) {
		cdev_init(&fc1100_dev_data[i].cdev, &fc1100_dev_fops);
		fc1100_dev_data[i].cdev.owner = THIS_MODULE;
		cdev_add(&fc1100_dev_data[i].cdev, MKDEV(dev_major, i), 1);

		fc1100_dev_data[i].fc100_dev = device_create(fc1100_class, NULL, MKDEV(dev_major, i), NULL, "fc1100-%d", i);
	}

	drv_access = drv;

	return 0;
}

int destroy_char_devs(void)
{
	int i;

	for (i = 0; i < MAX_DEV; i++) {
		device_destroy(fc1100_class, MKDEV(dev_major, i));
	}

	class_unregister(fc1100_class);
	class_destroy(fc1100_class);
	unregister_chrdev_region(MKDEV(dev_major, 0), MINORMASK);

	return 0;
}

static int fc1100_dev_open(struct inode *inode, struct file *file)
{
	struct fc1100_device_private* fc1100_device_priv;
	unsigned int minor = iminor(inode);

	fc1100_device_priv = kzalloc(sizeof(struct fc1100_device_private), GFP_KERNEL);
	fc1100_device_priv->drv = drv_access;

	fc1100_device_priv->chnum = minor;

	file->private_data = fc1100_device_priv;

	return 0;
}

static int fc1100_dev_release(struct inode *inode, struct file *file)
{
	struct fc1100_device_private* priv = file->private_data;

	kfree(priv);

	priv = NULL;

	return 0;
}

// TODO: implement ioctl commands
static long fc1100_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static ssize_t fc1100_dev_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
	struct fc1100_device_private* priv = file->private_data;
	struct fc1100_driver* drv = priv->drv;
	int i;

	uint8_t* ret_data = kzalloc(count, GFP_ATOMIC);
	uint8_t data;

	void *read_address = drv->hwmem + 0x1000 + ((*offset) * sizeof(uint8_t));

	for (i = 0; i < count; i++) {
		mutex_lock(&etx_mutex);
		data = ioread8(read_address++);
		mutex_unlock(&etx_mutex);
		ret_data[i] = data;
		(*offset)++;
	}
	if (copy_to_user(buf, ret_data, count)) {
		return -EFAULT;
	}
	return count;
}

// implement a write function that writes to the device
static ssize_t fc1100_dev_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
	struct fc1100_device_private* priv = file->private_data;
	struct fc1100_driver* drv = priv->drv;
	int i;

	uint8_t* data = kzalloc(count, GFP_ATOMIC);

	void *write_address = drv->hwmem + 0x1000 + ((*offset) * sizeof(uint8_t));

	if (copy_from_user(data, buf, count)) {
		return -EFAULT;
	}

	for (i = 0; i < count; i++) {
		mutex_lock(&etx_mutex);
		iowrite8(data[i], write_address++);
		mutex_unlock(&etx_mutex);
		(*offset)++;
	}

	kfree(data);

	return 0;
}

// TODO: fix SEEK_END
static loff_t fc1100_llseek(struct file *file, loff_t offset, int whence)
{
	loff_t newpos;

	switch(whence) {
		case 0: /* SEEK_SET */
			newpos = offset;
			break;

		case 1: /* SEEK_CUR */
			newpos = file->f_pos + offset;
			break;

		case 2: /* SEEK_END */
			newpos = 0x1000 + offset;
			break;

		default: /* can't happen */
			return -EINVAL;
	}
	if (newpos < 0) return -EINVAL;
	file->f_pos = newpos;
	return newpos;
}