/*
 * CORERIVER TOUCHCORE TC3XX touchkey driver
 *
 * Copyright (C) 2013 Samsung Electronics Co.Ltd
 * Author: Junkyeong Kim <jk0430.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */

#ifndef __LINUX_TC360_H
#define __LINUX_TC360_H

#ifdef CONFIG_SAMSUNG_LPM_MODE
extern int poweroff_charging;
#endif

#define TC300K_DEVICE	"sec_touchkey" //"tc300k"

extern struct class *sec_class;
extern int touch_is_pressed;
extern int system_rev;

struct tc300k_platform_data {
	u8	enable;
	u32	gpio_scl;
	u32	gpio_sda;
	u32	gpio_int;
	u32 gpio_en;
	u32 irq_gpio_flags;
	u32 sda_gpio_flags;
	u32 scl_gpio_flags;
	u32 vcc_gpio_flags;
	int	udelay;
	int	num_key;
	int sensing_ch_num;
	int	*keycodes;
	u8	suspend_type;
	u8	exit_flag;
	bool firmup;
	const char *vcc_en_ldo_name;
	const char *vdd_led_ldo_name;
};
#endif /* __LINUX_TC360_H */
