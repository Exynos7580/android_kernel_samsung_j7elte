#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/hrtimer.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/wakelock.h>
#include <linux/io.h>
#include <mach/map.h>
#include <linux/hrtimer.h>
#include "imm_vib.h"

static DEFINE_SEMAPHORE(sem);
static int imm_vib_process(struct imm_vib_dev *dev);
static inline int imm_vib_lock(struct semaphore *lock)
{
	return (lock->count) != 1;
}

static enum hrtimer_restart imm_vib_timer(struct hrtimer *timer)
{
	struct imm_vib_dev *dev
		= container_of(timer, struct imm_vib_dev, timer);

	hrtimer_forward_now(timer, dev->timer_ms);

	if (dev->timer_started) {
		if (imm_vib_lock(dev->sem))
			up(dev->sem);
	}

	return HRTIMER_RESTART;
}

static void imm_vib_start_timer(struct imm_vib_dev *dev)
{
	int i;
	int res;

	dev->watchdog_cnt = 0;

	if (!dev->timer_started) {
		if (!imm_vib_lock(dev->sem))
			res = down_interruptible(dev->sem);

		dev->timer_started = true;

		hrtimer_start(&dev->timer, dev->timer_ms, HRTIMER_MODE_REL);

		for (i = 0; i < dev->num_actuators; i++) {
			if ((dev->actuator[i].samples[0].size)
				|| (dev->actuator[i].samples[1].size)) {
				dev->actuator[i].output = 0;
				return;
			}
		}
	}

	if (0 != imm_vib_process(dev))
		return;

	res = down_interruptible(dev->sem);
	if (res != 0)
		printk(KERN_DEBUG
			"[VIB] %s down_interruptible interrupted by a signal.\n",
			__func__);
}

static void imm_vib_stop_timer(struct imm_vib_dev *dev)
{
	int i;

	if (dev->timer_started) {
		dev->timer_started = false;
		hrtimer_cancel(&dev->timer);
	}

	/* Reset samples buffers */
	for (i = 0; i < dev->num_actuators; i++) {
		dev->actuator[i].playing = -1;
		dev->actuator[i].samples[0].size = 0;
		dev->actuator[i].samples[1].size = 0;
	}
	dev->stop_requested = false;
	dev->is_playing = false;
}

static int imm_vib_disable(struct imm_vib_dev *dev, int index)
{
	if (dev->amp_enabled) {
		dev->amp_enabled = false;
		dev->chip_en(dev->private_data, false);
		if (dev->motor_enabled == 1)
			dev->motor_enabled = 0;
	}

	return 0;
}

static int imm_vib_enable(struct imm_vib_dev *dev, u8 index)
{
	if (!dev->amp_enabled) {
		dev->amp_enabled = true;
		dev->motor_enabled = 1;
		dev->chip_en(dev->private_data, true);
	}

	return 0;
}

static int imm_vib_setsamples(struct imm_vib_dev *dev,
	int index, u16 depth, u16 size, char *buff)
{
	int8_t force;

	switch (depth) {
	case 8:
		if (size != 1) {
			printk(KERN_ERR "[VIB] %s size : %d\n", __func__, size);
			return IMM_VIB_FAIL;
		}
		force = buff[0];
		break;

	case 16:
		if (size != 2)
			return IMM_VIB_FAIL;
		force = ((int16_t *)buff)[0] >> 8;
		break;

	default:
		return IMM_VIB_FAIL;
	}

	dev->set_force(dev->private_data, index, force);

	return 0;
}

static int imm_vib_get_name(struct imm_vib_dev *dev,
	u8 index, char *szDevName, int nSize)
{
	return 0;
}

static int imm_vib_process(struct imm_vib_dev *dev)
{
	int i = 0;
	int not_playing = 0;

	for (i = 0; i < dev->num_actuators; i++) {
		struct actuator_data *current_actuator = &(dev->actuator[i]);

		if (-1 == current_actuator->playing) {
			not_playing++;
			if ((dev->num_actuators == not_playing) &&
				((++dev->watchdog_cnt) > WATCHDOG_TIMEOUT)) {
				char buff[1] = {0};
				imm_vib_setsamples(dev, i, 8, 1, buff);
				imm_vib_disable(dev, i);
				imm_vib_stop_timer(dev);
				dev->watchdog_cnt = 0;
			}
		} else {
			int index = (int)current_actuator->playing;
			if (IMM_VIB_FAIL == imm_vib_setsamples(dev,
				current_actuator->samples[index].index,
				current_actuator->samples[index].bit_detpth,
				current_actuator->samples[index].size,
				current_actuator->samples[index].data))
				hrtimer_forward_now(&dev->timer, dev->timer_ms);

			current_actuator->output += current_actuator->samples[index].size;

			if (current_actuator->output >= current_actuator->samples[index].size) {
				current_actuator->samples[index].size = 0;

				(current_actuator->playing) ^= 1;
				current_actuator->output = 0;

				if (dev->stop_requested) {
					current_actuator->playing = -1;
					imm_vib_disable(dev, i);
				}
			}
		}
	}

	if (dev->stop_requested) {
		imm_vib_stop_timer(dev);
		dev->watchdog_cnt = 0;
		if (imm_vib_lock(dev->sem))
			up(dev->sem);
		return 1;
	}

	return 0;
}

static int imm_vib_open(struct inode *inode, struct file *file)
{
	printk(KERN_DEBUG "[VIB] %s\n", __func__);
	if (!try_module_get(THIS_MODULE))
		return -ENODEV;

	return 0;
}

static int imm_vib_release(struct inode *inode, struct file *file)
{
	struct miscdevice *miscdev = file->private_data;
	struct imm_vib_dev *dev = container_of(miscdev,
		struct imm_vib_dev,miscdev);

	printk(KERN_DEBUG "[VIB] %s\n", __func__);
	imm_vib_stop_timer(dev);
	file->private_data = (void *)NULL;
	module_put(THIS_MODULE);

	return 0;
}

static ssize_t imm_vib_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	struct miscdevice *miscdev = file->private_data;
	struct imm_vib_dev *dev = container_of(miscdev,
		struct imm_vib_dev,miscdev);
	const size_t size = (dev->cch_device_name > (size_t)(*ppos)) ?
		min(count, dev->cch_device_name - (size_t)(*ppos)) : 0;

	if (0 == size)
		return 0;

	if (0 != copy_to_user(buf, dev->device_name + (*ppos), size))	{
		printk(KERN_ERR "[VIB] copy_to_user failed.\n");
		return 0;
	}

	*ppos += size;
	return size;
}

static ssize_t imm_vib_write(struct file *file, const char *buf, size_t count,
	loff_t *ppos)
{
	struct miscdevice *miscdev = file->private_data;
	struct imm_vib_dev *dev = container_of(miscdev,
		struct imm_vib_dev,miscdev);
	int i = 0;

	*ppos = 0;

	if (file->private_data != (void *)IMM_VIB_MAGIC_NUMBER) {
		printk(KERN_ERR "[VIB] unauthorized write.\n");
		return 0;
	}

	if ((count <= SPI_HEADER_SIZE) || (count > SPI_BUFFER_SIZE)) {
		printk(KERN_ERR "[VIB] invalid write buffer size.\n");
		return 0;
	}

	if (0 != copy_from_user(dev->write_buff, buf, count)) {
		printk(KERN_ERR "[VIB] copy_from_user failed.\n");
		return 0;
	}

	while (i < count) {
		struct samples_data* input =
			(struct samples_data *)(&dev->write_buff[i]);
		int buff = 0;

		if ((i + SPI_HEADER_SIZE) >= count) {
			printk(KERN_EMERG "[VIB] invalid buffer index.\n");
			return 0;
		}

		if (8 != input->bit_detpth)
			printk(KERN_WARNING
				"[VIB] invalid bit depth. Use default value (8)\n");

		if ((i + SPI_HEADER_SIZE + input->size) > count) {
			printk(KERN_EMERG "[VIB] invalid data size.\n");
			return 0;
		}

		if (dev->num_actuators <= input->index) {
			printk(KERN_ERR "[VIB] invalid actuator index.\n");
			i += (SPI_HEADER_SIZE + input->size);
			continue;
		}

		if (0 == dev->actuator[input->index].samples[0].size)
			buff = 0;
		else if (0 == dev->actuator[input->index].samples[1].size)
			buff = 1;
		else {
			printk(KERN_ERR
				"[VIB] no room to store new samples.\n");
			return 0;
		}

		memcpy(&(dev->actuator[input->index].samples[buff]),
			&dev->write_buff[i],
			(SPI_HEADER_SIZE + input->size));

		if (-1 == dev->actuator[input->index].playing) {
			dev->actuator[input->index].playing = buff;
			dev->actuator[input->index].output = 0;
		}

		i += (SPI_HEADER_SIZE + input->size);
	}

	dev->is_playing = true;
	imm_vib_start_timer(dev);

	return count;
}

static long imm_vib_ioctl(struct file *file, unsigned int cmd,
	unsigned long arg)
{
	struct miscdevice *miscdev = file->private_data;
	struct imm_vib_dev *dev = container_of(miscdev,
		struct imm_vib_dev,miscdev);

#if 0
	printk(KERN_DEBUG "[VIB] ioctl cmd[0x%x].\n", cmd);
#endif
	switch (cmd) {
	case IMM_VIB_STOP_KERNEL_TIMER:
		if (true == dev->is_playing)
			dev->stop_requested = true;
		imm_vib_process(dev);
		break;

	case IMM_VIB_MAGIC_NUMBER:
		file->private_data = (void *)IMM_VIB_MAGIC_NUMBER;
		break;

	case IMM_VIB_ENABLE_AMP:
		wake_lock(&dev->wlock);
		imm_vib_enable(dev, arg);
		break;

	case IMM_VIB_DISABLE_AMP:
		if (!dev->stop_requested)
			imm_vib_disable(dev, arg);
		wake_unlock(&dev->wlock);
		break;

	case IMM_VIB_GET_NUM_ACTUATORS:
		return dev->num_actuators;
	}

	return 0;
}

static const struct file_operations imm_vib_fops = {
	.owner = THIS_MODULE,
	.read = imm_vib_read,
	.write = imm_vib_write,
	.unlocked_ioctl = imm_vib_ioctl,
	.open = imm_vib_open,
	.release = imm_vib_release,
	.llseek =	default_llseek
};

int imm_vib_register(struct imm_vib_dev *dev)
{
	int ret = 0, i = 0;

	printk(KERN_DEBUG "[VIB] %s\n", __func__);

	dev->miscdev.minor = MISC_DYNAMIC_MINOR;
	dev->miscdev.name = MODULE_NAME;
	dev->miscdev.fops = &imm_vib_fops;

	ret = misc_register(&dev->miscdev);
	if (ret) {
		printk(KERN_ERR "[VIB] misc_register failed.\n");
		return ret;
	}

	hrtimer_init(&dev->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);

	dev->amp_enabled = false;
	dev->motor_enabled = 0;
	dev->timer_ms = ktime_set(0, 5000000);
	dev->timer.function = imm_vib_timer;
	dev->sem = &sem;

	for (i = 0; i < dev->num_actuators; i++) {
		char *name = dev->device_name;
		imm_vib_get_name(dev, i, name, IMM_VIB_NAME_SIZE);

		strcat(name, VERSION_STR);
		dev->cch_device_name += strlen(name);
		dev->actuator[i].playing = -1;
		dev->actuator[i].samples[0].size = 0;
		dev->actuator[i].samples[1].size = 0;
	}

	wake_lock_init(&dev->wlock, WAKE_LOCK_SUSPEND, "vib_present");
	return 0;
}

void imm_vib_unregister(struct imm_vib_dev *dev)
{
	int i = 0;

	printk(KERN_DEBUG "[VIB] %s\n", __func__);

	imm_vib_stop_timer(dev);
	if (imm_vib_lock(dev->sem))
		up(dev->sem);

	for (i = 0; dev->num_actuators > i; i++)
		imm_vib_disable(dev, i);

	wake_lock_destroy(&dev->wlock);

	misc_deregister(&dev->miscdev);
}

MODULE_DESCRIPTION("Immersion TouchSense Kernel Module");
MODULE_LICENSE("GPL v2");
