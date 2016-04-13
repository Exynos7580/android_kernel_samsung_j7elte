/* linux/drivers/video/exynos_decon/panel/aid_dimming.h
 *
 * Header file for Samsung AID Dimming Driver.
 *
 * Copyright (c) 2013 Samsung Electronics
 * Minwoo Kim <minwoo7945.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __AID_DIMMING_H__
#define __AID_DIMMING_H__

#if defined(CONFIG_PANEL_S6E3FA2_DYNAMIC)
#include "s6e3fa2_dimming.h"
#elif defined(CONFIG_PANEL_S6E3FA3_DYNAMIC)
#include "s6e3fa3_dimming.h"
#elif defined(CONFIG_PANEL_S6E3HF2_DYNAMIC)
#include "s6e3hf2_wqhd_dimming.h"
#elif defined(CONFIG_PANEL_S6D7AA0X62_DYNAMIC)
#include "ea8061_dimming.h"
#else
#error "ERROR !! Check LCD Panel Header File"
#endif

enum {
	CI_RED = 0,
	CI_GREEN,
	CI_BLUE,
	CI_MAX,
};

struct dim_data {
	int t_gamma[NUM_VREF][CI_MAX];
	int mtp[NUM_VREF][CI_MAX];
	int volt[MAX_GRADATION][CI_MAX];
	int volt_vt[CI_MAX];
	int vt_mtp[CI_MAX];
	int look_volt[NUM_VREF][CI_MAX];
};

struct SmtDimInfo {
	unsigned int br;
	unsigned int refBr;
	const unsigned int *cGma;
	const signed char *rTbl;
	const signed char *cTbl;
	const unsigned char *aid;
	const unsigned char *elvAcl;
	const unsigned char *elv;
	unsigned char gamma[OLED_CMD_GAMMA_CNT];
};

#endif

