/*
 * drivers/video/decon_7580/panels/s6d7aa0x62_lcd_ctrl.c
 *
 * Samsung SoC MIPI LCD CONTROL functions
 *
 * Copyright (c) 2014 Samsung Electronics
 *
 * Jiun Yu, <jiun.yu@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <video/mipi_display.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

#include "../dsim.h"

#include "panel_info.h"

#ifdef CONFIG_PANEL_AID_DIMMING
#include "aid_dimming.h"
#include "dimming_core.h"
#include "ea8061_aid_dimming.h"
#include "dsim_backlight.h"
#endif

#ifdef CONFIG_PANEL_AID_DIMMING

static const unsigned char *ACL_CUTOFF_TABLE_8[ACL_STATUS_MAX] = {SEQ_ACL_OFF, SEQ_ACL_8};
static const unsigned char *ACL_CUTOFF_TABLE_15[ACL_STATUS_MAX] = {SEQ_ACL_OFF, SEQ_ACL_15};

static const unsigned int br_tbl [256] = {
	5, 5, 5, 5, 5, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 19, 20, 21, 22,
	24, 25, 27, 29, 30, 32, 34, 37, 39, 41, 41, 44, 44, 47, 47, 50, 50, 53, 53, 56,
	56, 56, 60, 60, 60, 64, 64, 64, 68, 68, 68, 72, 72, 72, 72, 77, 77, 77, 82,
	82, 82, 82, 87, 87, 87, 87, 93, 93, 93, 93, 98, 98, 98, 98, 98, 105, 105,
	105, 105, 111, 111, 111, 111, 111, 111, 119, 119, 119, 119, 119, 126, 126, 126,
	126, 126, 126, 134, 134, 134, 134, 134, 134, 134, 143, 143, 143, 143, 143, 143,
	152, 152, 152, 152, 152, 152, 152, 152, 162, 162, 162, 162, 162, 162, 162, 172,
	172, 172, 172, 172, 172, 172, 172, 183, 183, 183, 183, 183, 183, 183, 183, 183,
	195, 195, 195, 195, 195, 195, 195, 195, 207, 207, 207, 207, 207, 207, 207, 207,
	207, 207, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 234, 234, 234, 234,
	234, 234, 234, 234, 234, 234, 234, 249, 249, 249, 249, 249, 249, 249, 249, 249,
	249, 249, 249, 265, 265, 265, 265, 265, 265, 265, 265, 265, 265, 265, 265, 282,
	282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 300, 300, 300, 300,
	300, 300, 300, 300, 300, 300, 300, 300, 316, 316, 316, 316, 316, 316, 316, 316,
	316, 316, 316, 316, 333, 333, 333, 333, 333, 333, 333, 333, 333, 333, 333, 333, 350,
};

static const short center_gamma[NUM_VREF][CI_MAX] = {
	{0x000, 0x000, 0x000},
	{0x080, 0x080, 0x080},
	{0x080, 0x080, 0x080},
	{0x080, 0x080, 0x080},
	{0x080, 0x080, 0x080},
	{0x080, 0x080, 0x080},
	{0x080, 0x080, 0x080},
	{0x080, 0x080, 0x080},
	{0x080, 0x080, 0x080},
	{0x100, 0x100, 0x100},
};

struct SmtDimInfo ea8061_dimming_info_D[MAX_BR_INFO + 1] = {				// add hbm array
	{.br = 5, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl5nit, .cTbl = D_ctbl5nit, .aid = D_aid9685, .elvAcl = elv0, .elv = elv0},
	{.br = 6, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl6nit, .cTbl = D_ctbl6nit, .aid = D_aid9585, .elvAcl = elv0, .elv = elv0},
	{.br = 7, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl7nit, .cTbl = D_ctbl7nit, .aid = D_aid9523, .elvAcl = elv0, .elv = elv0},
	{.br = 8, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl8nit, .cTbl = D_ctbl8nit, .aid = D_aid9438, .elvAcl = elv0, .elv = elv0},
	{.br = 9, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl9nit, .cTbl = D_ctbl9nit, .aid = D_aid9338, .elvAcl = elv0, .elv = elv0},
	{.br = 10, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl10nit, .cTbl = D_ctbl10nit, .aid = D_aid9285, .elvAcl = elv0, .elv = elv0},
	{.br = 11, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl11nit, .cTbl = D_ctbl11nit, .aid = D_aid9200, .elvAcl = elv0, .elv = elv0},
	{.br = 12, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl12nit, .cTbl = D_ctbl12nit, .aid = D_aid9100, .elvAcl = elv0, .elv = elv0},
	{.br = 13, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl13nit, .cTbl = D_ctbl13nit, .aid = D_aid9046, .elvAcl = elv0, .elv = elv0},
	{.br = 14, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl14nit, .cTbl = D_ctbl14nit, .aid = D_aid8954, .elvAcl = elv0, .elv = elv0},
	{.br = 15, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl15nit, .cTbl = D_ctbl15nit, .aid = D_aid8923, .elvAcl = elv0, .elv = elv0},
	{.br = 16, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl16nit, .cTbl = D_ctbl16nit, .aid = D_aid8800, .elvAcl = elv0, .elv = elv0},
	{.br = 17, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl17nit, .cTbl = D_ctbl17nit, .aid = D_aid8715, .elvAcl = elv0, .elv = elv0},
	{.br = 19, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl19nit, .cTbl = D_ctbl19nit, .aid = D_aid8546, .elvAcl = elv0, .elv = elv0},
	{.br = 20, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl20nit, .cTbl = D_ctbl20nit, .aid = D_aid8462, .elvAcl = elv0, .elv = elv0},
	{.br = 21, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl21nit, .cTbl = D_ctbl21nit, .aid = D_aid8346, .elvAcl = elv1, .elv = elv1},
	{.br = 22, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl22nit, .cTbl = D_ctbl22nit, .aid = D_aid8246, .elvAcl = elv2, .elv = elv2},
	{.br = 24, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl24nit, .cTbl = D_ctbl24nit, .aid = D_aid8085, .elvAcl = elv3, .elv = elv3},
	{.br = 25, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl25nit, .cTbl = D_ctbl25nit, .aid = D_aid7969, .elvAcl = elv4, .elv = elv4},
	{.br = 27, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl27nit, .cTbl = D_ctbl27nit, .aid = D_aid7769, .elvAcl = elv5, .elv = elv5},
	{.br = 29, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl29nit, .cTbl = D_ctbl29nit, .aid = D_aid7577, .elvAcl = elv6, .elv = elv6},

	{.br = 30, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl30nit, .cTbl = D_ctbl30nit, .aid = D_aid7508, .elvAcl = elv7, .elv = elv7},
	{.br = 32, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl32nit, .cTbl = D_ctbl32nit, .aid = D_aid7323, .elvAcl = elv7, .elv = elv7},
	{.br = 34, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl34nit, .cTbl = D_ctbl34nit, .aid = D_aid7138, .elvAcl = elv7, .elv = elv7},
	{.br = 37, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl37nit, .cTbl = D_ctbl37nit, .aid = D_aid6892, .elvAcl = elv7, .elv = elv7},
	{.br = 39, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl39nit, .cTbl = D_ctbl39nit, .aid = D_aid6715, .elvAcl = elv7, .elv = elv7},
	{.br = 41, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl41nit, .cTbl = D_ctbl41nit, .aid = D_aid6531, .elvAcl = elv7, .elv = elv7},
	{.br = 44, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl44nit, .cTbl = D_ctbl44nit, .aid = D_aid6262, .elvAcl = elv7, .elv = elv7},
	{.br = 47, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl47nit, .cTbl = D_ctbl47nit, .aid = D_aid6000, .elvAcl = elv7, .elv = elv7},
	{.br = 50, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl50nit, .cTbl = D_ctbl50nit, .aid = D_aid5731, .elvAcl = elv7, .elv = elv7},
	{.br = 53, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl53nit, .cTbl = D_ctbl53nit, .aid = D_aid5454, .elvAcl = elv7, .elv = elv7},
	{.br = 56, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl56nit, .cTbl = D_ctbl56nit, .aid = D_aid5177, .elvAcl = elv7, .elv = elv7},
	{.br = 60, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl60nit, .cTbl = D_ctbl60nit, .aid = D_aid4800, .elvAcl = elv7, .elv = elv7},
	{.br = 64, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl64nit, .cTbl = D_ctbl64nit, .aid = D_aid4438, .elvAcl = elv7, .elv = elv7},
	{.br = 68, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl68nit, .cTbl = D_ctbl68nit, .aid = D_aid4062, .elvAcl = elv7, .elv = elv7},
	{.br = 72, .refBr = 113, .cGma = gma2p00, .rTbl = D_rtbl72nit, .cTbl = D_ctbl72nit, .aid = D_aid3662, .elvAcl = elv7, .elv = elv7},
	{.br = 77, .refBr = 120, .cGma = gma2p00, .rTbl = D_rtbl77nit, .cTbl = D_ctbl77nit, .aid = D_aid3662, .elvAcl = elv7, .elv = elv7},
	{.br = 82, .refBr = 126, .cGma = gma2p00, .rTbl = D_rtbl82nit, .cTbl = D_ctbl82nit, .aid = D_aid3662, .elvAcl = elv7, .elv = elv7},
	{.br = 87, .refBr = 135, .cGma = gma2p00, .rTbl = D_rtbl87nit, .cTbl = D_ctbl87nit, .aid = D_aid3662, .elvAcl = elv7, .elv = elv7},
	{.br = 93, .refBr = 143, .cGma = gma2p00, .rTbl = D_rtbl93nit, .cTbl = D_ctbl93nit, .aid = D_aid3662, .elvAcl = elv7, .elv = elv7},
	{.br = 98, .refBr = 150, .cGma = gma2p00, .rTbl = D_rtbl98nit, .cTbl = D_ctbl98nit, .aid = D_aid3662, .elvAcl = elv7, .elv = elv7},
	{.br = 105, .refBr = 161, .cGma = gma2p00, .rTbl = D_rtbl105nit, .cTbl = D_ctbl105nit, .aid = D_aid3662, .elvAcl = elv7, .elv = elv7},

	{.br = 111, .refBr = 168, .cGma = gma2p00, .rTbl = D_rtbl111nit, .cTbl = D_ctbl111nit, .aid = D_aid3662, .elvAcl = elv8, .elv = elv8},
	{.br = 119, .refBr = 183, .cGma = gma2p00, .rTbl = D_rtbl119nit, .cTbl = D_ctbl119nit, .aid = D_aid3662, .elvAcl = elv8, .elv = elv8},

	{.br = 126, .refBr = 191, .cGma = gma2p00, .rTbl = D_rtbl126nit, .cTbl = D_ctbl126nit, .aid = D_aid3662, .elvAcl = elv9, .elv = elv9},
	{.br = 134, .refBr = 201, .cGma = gma2p00, .rTbl = D_rtbl134nit, .cTbl = D_ctbl134nit, .aid = D_aid3662, .elvAcl = elv9, .elv = elv9},
	{.br = 143, .refBr = 214, .cGma = gma2p00, .rTbl = D_rtbl143nit, .cTbl = D_ctbl143nit, .aid = D_aid3662, .elvAcl = elv9, .elv = elv9},
	{.br = 152, .refBr = 226, .cGma = gma2p00, .rTbl = D_rtbl152nit, .cTbl = D_ctbl152nit, .aid = D_aid3662, .elvAcl = elv9, .elv = elv9},
	{.br = 162, .refBr = 239, .cGma = gma2p00, .rTbl = D_rtbl162nit, .cTbl = D_ctbl162nit, .aid = D_aid3662, .elvAcl = elv9, .elv = elv9},
	{.br = 172, .refBr = 253, .cGma = gma2p00, .rTbl = D_rtbl172nit, .cTbl = D_ctbl172nit, .aid = D_aid3662, .elvAcl = elv9, .elv = elv9},
	{.br = 183, .refBr = 253, .cGma = gma2p00, .rTbl = D_rtbl183nit, .cTbl = D_ctbl183nit, .aid = D_aid3200, .elvAcl = elv9, .elv = elv9},

	{.br = 195, .refBr = 253, .cGma = gma2p00, .rTbl = D_rtbl195nit, .cTbl = D_ctbl195nit, .aid = D_aid2708, .elvAcl = elv10, .elv = elv10},
	{.br = 207, .refBr = 253, .cGma = gma2p00, .rTbl = D_rtbl207nit, .cTbl = D_ctbl207nit, .aid = D_aid2185, .elvAcl = elv11, .elv = elv11},
	{.br = 220, .refBr = 253, .cGma = gma2p00, .rTbl = D_rtbl220nit, .cTbl = D_ctbl220nit, .aid = D_aid1654, .elvAcl = elv12, .elv = elv12},
	{.br = 234, .refBr = 253, .cGma = gma2p00, .rTbl = D_rtbl234nit, .cTbl = D_ctbl234nit, .aid = D_aid1038, .elvAcl = elv13, .elv = elv13},
	{.br = 249, .refBr = 253, .cGma = gma2p00, .rTbl = D_rtbl249nit, .cTbl = D_ctbl249nit, .aid = D_aid0369, .elvAcl = elv14, .elv = elv14},		// 249 ~ 360nit acl off
	{.br = 265, .refBr = 268, .cGma = gma2p00, .rTbl = D_rtbl265nit, .cTbl = D_ctbl265nit, .aid = D_aid0369, .elvAcl = elv15, .elv = elv15},
	{.br = 282, .refBr = 285, .cGma = gma2p00, .rTbl = D_rtbl282nit, .cTbl = D_ctbl282nit, .aid = D_aid0369, .elvAcl = elv16, .elv = elv16},
	{.br = 300, .refBr = 303, .cGma = gma2p00, .rTbl = D_rtbl300nit, .cTbl = D_ctbl300nit, .aid = D_aid0369, .elvAcl = elv17, .elv = elv17},
	{.br = 316, .refBr = 320, .cGma = gma2p00, .rTbl = D_rtbl316nit, .cTbl = D_ctbl316nit, .aid = D_aid0369, .elvAcl = elv18, .elv = elv18},
	{.br = 333, .refBr = 337, .cGma = gma2p00, .rTbl = D_rtbl333nit, .cTbl = D_ctbl333nit, .aid = D_aid0369, .elvAcl = elv19, .elv = elv19},
	{.br = 350, .refBr = 350, .cGma = gma2p00, .rTbl = D_rtbl350nit, .cTbl = D_ctbl350nit, .aid = D_aid0369, .elvAcl = elv20, .elv = elv20},
	{.br = 350, .refBr = 350, .cGma = gma2p00, .rTbl = D_rtbl350nit, .cTbl = D_ctbl350nit, .aid = D_aid0369, .elvAcl = elv20, .elv = elv20},
};

struct SmtDimInfo ea8061_dimming_info[MAX_BR_INFO + 1] = {				// add hbm array
	{.br = 5, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl5nit, .cTbl = ctbl5nit, .aid = aid9685, .elvAcl = elv0, .elv = elv0},
	{.br = 6, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl6nit, .cTbl = ctbl6nit, .aid = aid9592, .elvAcl = elv0, .elv = elv0},
	{.br = 7, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl7nit, .cTbl = ctbl7nit, .aid = aid9523, .elvAcl = elv0, .elv = elv0},
	{.br = 8, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl8nit, .cTbl = ctbl8nit, .aid = aid9446, .elvAcl = elv0, .elv = elv0},
	{.br = 9, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl9nit, .cTbl = ctbl9nit, .aid = aid9354, .elvAcl = elv0, .elv = elv0},
	{.br = 10, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl10nit, .cTbl = ctbl10nit, .aid = aid9285, .elvAcl = elv0, .elv = elv0},
	{.br = 11, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl11nit, .cTbl = ctbl11nit, .aid = aid9215, .elvAcl = elv0, .elv = elv0},
	{.br = 12, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl12nit, .cTbl = ctbl12nit, .aid = aid9123, .elvAcl = elv0, .elv = elv0},
	{.br = 13, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl13nit, .cTbl = ctbl13nit, .aid = aid9069, .elvAcl = elv0, .elv = elv0},
	{.br = 14, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl14nit, .cTbl = ctbl14nit, .aid = aid8985, .elvAcl = elv0, .elv = elv0},
	{.br = 15, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl15nit, .cTbl = ctbl15nit, .aid = aid8923, .elvAcl = elv0, .elv = elv0},
	{.br = 16, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl16nit, .cTbl = ctbl16nit, .aid = aid8831, .elvAcl = elv0, .elv = elv0},
	{.br = 17, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl17nit, .cTbl = ctbl17nit, .aid = aid8746, .elvAcl = elv0, .elv = elv0},
	{.br = 19, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl19nit, .cTbl = ctbl19nit, .aid = aid8592, .elvAcl = elv0, .elv = elv0},
	{.br = 20, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl20nit, .cTbl = ctbl20nit, .aid = aid8500, .elvAcl = elv0, .elv = elv0},

	{.br = 21, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl21nit, .cTbl = ctbl21nit, .aid = aid8423, .elvAcl = elv1, .elv = elv1},
	{.br = 22, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl22nit, .cTbl = ctbl22nit, .aid = aid8331, .elvAcl = elv2, .elv = elv2},
	{.br = 24, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl24nit, .cTbl = ctbl24nit, .aid = aid8138, .elvAcl = elv3, .elv = elv3},
	{.br = 25, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl25nit, .cTbl = ctbl25nit, .aid = aid8054, .elvAcl = elv4, .elv = elv4},
	{.br = 27, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl27nit, .cTbl = ctbl27nit, .aid = aid7885, .elvAcl = elv5, .elv = elv5},
	{.br = 29, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl29nit, .cTbl = ctbl29nit, .aid = aid7692, .elvAcl = elv6, .elv = elv6},

	{.br = 30, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl30nit, .cTbl = ctbl30nit, .aid = aid7562, .elvAcl = elv7, .elv = elv7},
	{.br = 32, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl32nit, .cTbl = ctbl32nit, .aid = aid7362, .elvAcl = elv7, .elv = elv7},
	{.br = 34, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl34nit, .cTbl = ctbl34nit, .aid = aid7192, .elvAcl = elv7, .elv = elv7},
	{.br = 37, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl37nit, .cTbl = ctbl37nit, .aid = aid6931, .elvAcl = elv7, .elv = elv7},
	{.br = 39, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl39nit, .cTbl = ctbl39nit, .aid = aid6746, .elvAcl = elv7, .elv = elv7},
	{.br = 41, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl41nit, .cTbl = ctbl41nit, .aid = aid6577, .elvAcl = elv7, .elv = elv7},
	{.br = 44, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl44nit, .cTbl = ctbl44nit, .aid = aid6292, .elvAcl = elv7, .elv = elv7},
	{.br = 47, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl47nit, .cTbl = ctbl47nit, .aid = aid6015, .elvAcl = elv7, .elv = elv7},
	{.br = 50, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl50nit, .cTbl = ctbl50nit, .aid = aid5762, .elvAcl = elv7, .elv = elv7},
	{.br = 53, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl53nit, .cTbl = ctbl53nit, .aid = aid5515, .elvAcl = elv7, .elv = elv7},
	{.br = 56, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl56nit, .cTbl = ctbl56nit, .aid = aid5254, .elvAcl = elv7, .elv = elv7},
	{.br = 60, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl60nit, .cTbl = ctbl60nit, .aid = aid4846, .elvAcl = elv7, .elv = elv7},
	{.br = 64, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl64nit, .cTbl = ctbl64nit, .aid = aid4515, .elvAcl = elv7, .elv = elv7},
	{.br = 68, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl68nit, .cTbl = ctbl68nit, .aid = aid4146, .elvAcl = elv7, .elv = elv7},

	{.br = 72, .refBr = 114, .cGma = gma2p00, .rTbl = rtbl72nit, .cTbl = ctbl72nit, .aid = aid3754, .elvAcl = elv7, .elv = elv7},
	{.br = 77, .refBr = 121, .cGma = gma2p00, .rTbl = rtbl77nit, .cTbl = ctbl77nit, .aid = aid3754, .elvAcl = elv7, .elv = elv7},
	{.br = 82, .refBr = 128, .cGma = gma2p00, .rTbl = rtbl82nit, .cTbl = ctbl82nit, .aid = aid3754, .elvAcl = elv7, .elv = elv7},
	{.br = 87, .refBr = 136, .cGma = gma2p00, .rTbl = rtbl87nit, .cTbl = ctbl87nit, .aid = aid3754, .elvAcl = elv7, .elv = elv7},
	{.br = 93, .refBr = 144, .cGma = gma2p00, .rTbl = rtbl93nit, .cTbl = ctbl93nit, .aid = aid3754, .elvAcl = elv7, .elv = elv7},
	{.br = 98, .refBr = 152, .cGma = gma2p00, .rTbl = rtbl98nit, .cTbl = ctbl98nit, .aid = aid3754, .elvAcl = elv7, .elv = elv7},
	{.br = 105, .refBr = 163, .cGma = gma2p00, .rTbl = rtbl105nit, .cTbl = ctbl105nit, .aid = aid3754, .elvAcl = elv7, .elv = elv7},


	{.br = 111, .refBr = 172, .cGma = gma2p00, .rTbl = rtbl111nit, .cTbl = ctbl111nit, .aid = aid3754, .elvAcl = elv7, .elv = elv7},
	{.br = 119, .refBr = 183, .cGma = gma2p00, .rTbl = rtbl119nit, .cTbl = ctbl119nit, .aid = aid3754, .elvAcl = elv8, .elv = elv8},
	{.br = 126, .refBr = 193, .cGma = gma2p00, .rTbl = rtbl126nit, .cTbl = ctbl126nit, .aid = aid3754, .elvAcl = elv8, .elv = elv8},
	{.br = 134, .refBr = 204, .cGma = gma2p00, .rTbl = rtbl134nit, .cTbl = ctbl134nit, .aid = aid3754, .elvAcl = elv9, .elv = elv9},

	{.br = 143, .refBr = 216, .cGma = gma2p00, .rTbl = rtbl143nit, .cTbl = ctbl143nit, .aid = aid3754, .elvAcl = elv10, .elv = elv10},
	{.br = 152, .refBr = 230, .cGma = gma2p00, .rTbl = rtbl152nit, .cTbl = ctbl152nit, .aid = aid3754, .elvAcl = elv11, .elv = elv11},
	{.br = 162, .refBr = 244, .cGma = gma2p00, .rTbl = rtbl162nit, .cTbl = ctbl162nit, .aid = aid3754, .elvAcl = elv12, .elv = elv12},

	{.br = 172, .refBr = 255, .cGma = gma2p00, .rTbl = rtbl172nit, .cTbl = ctbl172nit, .aid = aid3754, .elvAcl = elv12, .elv = elv12},
	{.br = 183, .refBr = 255, .cGma = gma2p00, .rTbl = rtbl183nit, .cTbl = ctbl183nit, .aid = aid3292, .elvAcl = elv12, .elv = elv12},
	{.br = 195, .refBr = 255, .cGma = gma2p00, .rTbl = rtbl195nit, .cTbl = ctbl195nit, .aid = aid2800, .elvAcl = elv13, .elv = elv13},
	{.br = 207, .refBr = 255, .cGma = gma2p00, .rTbl = rtbl207nit, .cTbl = ctbl207nit, .aid = aid2277, .elvAcl = elv14, .elv = elv14},
	{.br = 220, .refBr = 255, .cGma = gma2p00, .rTbl = rtbl220nit, .cTbl = ctbl220nit, .aid = aid1708, .elvAcl = elv14, .elv = elv14},
	{.br = 234, .refBr = 255, .cGma = gma2p00, .rTbl = rtbl234nit, .cTbl = ctbl234nit, .aid = aid1077, .elvAcl = elv14, .elv = elv14},

	{.br = 249, .refBr = 255, .cGma = gma2p00, .rTbl = rtbl249nit, .cTbl = ctbl249nit, .aid = aid0369, .elvAcl = elv15, .elv = elv15},		// 249 ~ 360nit acl off
	{.br = 265, .refBr = 272, .cGma = gma2p00, .rTbl = rtbl265nit, .cTbl = ctbl265nit, .aid = aid0369, .elvAcl = elv15, .elv = elv15},
	{.br = 282, .refBr = 286, .cGma = gma2p00, .rTbl = rtbl282nit, .cTbl = ctbl282nit, .aid = aid0369, .elvAcl = elv16, .elv = elv16},
	{.br = 300, .refBr = 306, .cGma = gma2p00, .rTbl = rtbl300nit, .cTbl = ctbl300nit, .aid = aid0369, .elvAcl = elv17, .elv = elv17},
	{.br = 316, .refBr = 320, .cGma = gma2p00, .rTbl = rtbl316nit, .cTbl = ctbl316nit, .aid = aid0369, .elvAcl = elv18, .elv = elv18},
	{.br = 333, .refBr = 337, .cGma = gma2p00, .rTbl = rtbl333nit, .cTbl = ctbl333nit, .aid = aid0369, .elvAcl = elv19, .elv = elv19},
	{.br = 350, .refBr = 350, .cGma = gma2p00, .rTbl = rtbl350nit, .cTbl = ctbl350nit, .aid = aid0369, .elvAcl = elv20, .elv = elv20},
	{.br = 350, .refBr = 350, .cGma = gma2p00, .rTbl = rtbl350nit, .cTbl = ctbl350nit, .aid = aid0369, .elvAcl = elv20, .elv = elv20},
};

static int init_dimming(struct dsim_device *dsim, u8 *mtp)
{
	int i, j;
	int pos = 0;
	int ret = 0;
	short temp;
	struct dim_data *dimming;
	struct panel_private *panel = &dsim->priv;
	struct SmtDimInfo *diminfo = NULL;

	dimming = (struct dim_data *)kmalloc(sizeof(struct dim_data), GFP_KERNEL);
	if (!dimming) {
		dsim_err("failed to allocate memory for dim data\n");
		ret = -ENOMEM;
		goto error;
	}

	if (dsim->priv.id[2] >= 0x03) {
		diminfo = ea8061_dimming_info_D;
		dsim_info("%s : ID is over 0x03 \n", __func__);
	} else {
		diminfo = ea8061_dimming_info;
	}

	panel->dim_data= (void *)dimming;
	panel->dim_info = (void *)diminfo;

	panel->br_tbl = (unsigned int *)br_tbl;

	if (panel->id[2] >=  0x02)
		panel->acl_cutoff_tbl = (unsigned char **)ACL_CUTOFF_TABLE_15;
	else
		panel->acl_cutoff_tbl = (unsigned char **)ACL_CUTOFF_TABLE_8;

for (j = 0; j < CI_MAX; j++) {
	if (mtp[pos] & 0x01)
		temp = mtp[pos+1] - 256;
	else
		temp = mtp[pos+1];

	dimming->t_gamma[V255][j] = (int)center_gamma[V255][j] + temp;
	dimming->mtp[V255][j] = temp;
	pos += 2;
}

/* if ddi have V0 offset, plz modify to   for (i = V203; i >= V0; i--) {    */
	for (i = V203; i > V0; i--) {
		for (j = 0; j < CI_MAX; j++) {
			temp = ((mtp[pos] & 0x80) ? -1 : 1) * ((mtp[pos] & 0x80) ? 128 - (mtp[pos] & 0x7f) : (mtp[pos] & 0x7f));
			dimming->t_gamma[i][j] = (int)center_gamma[i][j] + temp;
			dimming->mtp[i][j] = temp;
			pos++;
		}
	}
	/* for vt */
	temp = (mtp[pos+1]) << 8 | mtp[pos];

	for(i=0;i<CI_MAX;i++)
		dimming->vt_mtp[i] = (temp >> (i*4)) &0x0f;
#ifdef SMART_DIMMING_DEBUG
	dimm_info("Center Gamma Info : \n");
	for(i=0;i<VMAX;i++) {
		dsim_info("Gamma : %3d %3d %3d : %3x %3x %3x\n",
			dimming->t_gamma[i][CI_RED], dimming->t_gamma[i][CI_GREEN], dimming->t_gamma[i][CI_BLUE],
			dimming->t_gamma[i][CI_RED], dimming->t_gamma[i][CI_GREEN], dimming->t_gamma[i][CI_BLUE] );
	}
#endif
	dimm_info("VT MTP : \n");
	dimm_info("Gamma : %3d %3d %3d : %3x %3x %3x\n",
			dimming->vt_mtp[CI_RED], dimming->vt_mtp[CI_GREEN], dimming->vt_mtp[CI_BLUE],
			dimming->vt_mtp[CI_RED], dimming->vt_mtp[CI_GREEN], dimming->vt_mtp[CI_BLUE] );

	dimm_info("MTP Info : \n");
	for(i=0;i<VMAX;i++) {
		dimm_info("Gamma : %3d %3d %3d : %3x %3x %3x\n",
			dimming->mtp[i][CI_RED], dimming->mtp[i][CI_GREEN], dimming->mtp[i][CI_BLUE],
			dimming->mtp[i][CI_RED], dimming->mtp[i][CI_GREEN], dimming->mtp[i][CI_BLUE] );
	}

	ret = generate_volt_table(dimming);
	if (ret) {
		dimm_err("[ERR:%s] failed to generate volt table\n", __func__);
		goto error;
	}

	for (i = 0; i < MAX_BR_INFO - 1; i++) {
		ret = cal_gamma_from_index(dimming, &diminfo[i]);
		if (ret) {
			dsim_err("failed to calculate gamma : index : %d\n", i);
			goto error;
		}
	}
	memcpy(diminfo[i].gamma, SEQ_GAMMA_CONDITION_SET, ARRAY_SIZE(SEQ_GAMMA_CONDITION_SET));
error:
	return ret;

}
#endif


static int ea8061_read_init_info(struct dsim_device *dsim, unsigned char* mtp)
{
	int i = 0;
	int ret;
	struct panel_private *panel = &dsim->priv;
	unsigned char buf[60] = {0, };

	dsim_info("MDD : %s was called\n", __func__);

	dsim_write_hl_data(dsim, SEQ_EA8061_READ_ID, ARRAY_SIZE(SEQ_EA8061_READ_ID));
	ret = dsim_read_hl_data(dsim, EA8061_READ_RX_REG, EA8061_ID_LEN, dsim->priv.id);
	if (ret != EA8061_ID_LEN) {
		dsim_err("%s : can't find connected panel. check panel connection\n",__func__);
		panel->lcdConnected = PANEL_DISCONNEDTED;
		goto read_exit;
	}

	dsim_info("READ ID : ");
	for(i = 0; i < EA8061_ID_LEN; i++)
		dsim_info("%02x, ", dsim->priv.id[i]);
	dsim_info("\n");

	ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(SEQ_TEST_KEY_ON_F0));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_TEST_KEY_ON_F0\n", __func__);
	}

	// mtp SEQ_EA8061_READ_MTP
	dsim_write_hl_data(dsim, SEQ_EA8061_READ_MTP, ARRAY_SIZE(SEQ_EA8061_READ_MTP));
	ret = dsim_read_hl_data(dsim, EA8061_READ_RX_REG, EA8061_MTP_DATE_SIZE, buf);
	if (ret != EA8061_MTP_DATE_SIZE) {
		dsim_err("failed to read mtp, check panel connection\n");
		goto read_fail;
	}
	memcpy(mtp, buf, EA8061_MTP_SIZE);

	//read DB
	dsim_write_hl_data(dsim, SEQ_EA8061_READ_DB, ARRAY_SIZE(SEQ_EA8061_READ_DB));
	ret = dsim_read_hl_data(dsim, EA8061_READ_RX_REG, EA8061_MTP_DB_SIZE, buf);
	if (ret != EA8061_MTP_DB_SIZE) {
		dsim_err("failed to read mtp, check panel connection\n");
		goto read_fail;
	}
	memcpy(panel->DB, buf, EA8061_MTP_DB_SIZE);

	memcpy(dsim->priv.date, &buf[50], ARRAY_SIZE(dsim->priv.date) - 2);
	dsim->priv.coordinate[0] = buf[52] << 8 | buf[53];	/* X */
	dsim->priv.coordinate[1] = buf[54] << 8 | buf[55];

	//read B2
	dsim_write_hl_data(dsim, SEQ_EA8061_READ_B2, ARRAY_SIZE(SEQ_EA8061_READ_B2));
	ret = dsim_read_hl_data(dsim, EA8061_READ_RX_REG, EA8061_MTP_B2_SIZE, buf);
	if (ret != EA8061_MTP_B2_SIZE) {
		dsim_err("failed to read mtp, check panel connection\n");
		goto read_fail;
	}
	memcpy(panel->B2, buf, EA8061_MTP_B2_SIZE);

	//read D4
	dsim_write_hl_data(dsim, SEQ_EA8061_READ_D4, ARRAY_SIZE(SEQ_EA8061_READ_D4));
	ret = dsim_read_hl_data(dsim, EA8061_READ_RX_REG, EA8061_MTP_D4_SIZE, buf);
	if (ret != EA8061_MTP_D4_SIZE) {
		dsim_err("failed to read mtp, check panel connection\n");
		goto read_fail;
	}
	memcpy(panel->D4, buf, EA8061_MTP_D4_SIZE);

read_exit:
	return 0;

read_fail:
	return -ENODEV;
}

static int ea8061_displayon(struct dsim_device *dsim)
{
	int ret = 0;

	dsim_info("MDD : %s was called\n", __func__);

	goto displayon_err;


displayon_err:
	return ret;

}

static int ea8061_exit(struct dsim_device *dsim)
{
	int ret = 0;
	dsim_info("MDD : %s was called\n", __func__);
	ret = dsim_write_hl_data(dsim, SEQ_DISPLAY_OFF, ARRAY_SIZE(SEQ_DISPLAY_OFF));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : DISPLAY_OFF\n", __func__);
		goto exit_err;
	}

	msleep(35);

	ret = dsim_write_hl_data(dsim, SEQ_SLEEP_IN, ARRAY_SIZE(SEQ_SLEEP_IN));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SLEEP_IN\n", __func__);
		goto exit_err;
	}

	msleep(150);

exit_err:
	return ret;
}

static int ea8061_init(struct dsim_device *dsim)
{
	int ret;

	dsim_info("MDD : %s was called\n", __func__);

	usleep_range(5000, 6000);

	ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(SEQ_TEST_KEY_ON_F0));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_TEST_KEY_ON_F0\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_FC, ARRAY_SIZE(SEQ_TEST_KEY_ON_FC));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_TEST_KEY_ON_FC\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, SEQ_EA8061_LTPS_STOP, ARRAY_SIZE(SEQ_EA8061_LTPS_STOP));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_EA8061_LTPS_STOP\n", __func__);
		goto init_exit;
	}

	if (dsim->priv.id[2] >= 0x03) {
		ret = dsim_write_hl_data(dsim, SEQ_EA8061_LTPS_TIMING_REV_D, ARRAY_SIZE(SEQ_EA8061_LTPS_TIMING_REV_D));
		if (ret < 0) {
			dsim_err(":%s fail to write CMD : SEQ_EA8061_LTPS_TIMING_REV_D\n", __func__);
			goto init_exit;
		}
	} else {
		ret = dsim_write_hl_data(dsim, SEQ_EA8061_LTPS_TIMING, ARRAY_SIZE(SEQ_EA8061_LTPS_TIMING));
		if (ret < 0) {
			dsim_err(":%s fail to write CMD : SEQ_EA8061_LTPS_TIMING\n", __func__);
			goto init_exit;
		}
	}

	ret = dsim_write_hl_data(dsim, SEQ_EA8061_LTPS_UPDATE, ARRAY_SIZE(SEQ_EA8061_LTPS_UPDATE));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_EA8061_LTPS_UPDATE\n", __func__);
		goto init_exit;
	}
	ret = dsim_write_hl_data(dsim, SEQ_EA8061_SCAN_DIRECTION, ARRAY_SIZE(SEQ_EA8061_SCAN_DIRECTION));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_EA8061_SCAN_DIRECTION\n", __func__);
		goto init_exit;
	}
	ret = dsim_write_hl_data(dsim, SEQ_EA8061_AID_SET_DEFAULT, ARRAY_SIZE(SEQ_EA8061_AID_SET_DEFAULT));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_EA8061_AID_SET_DEFAULT\n", __func__);
		goto init_exit;
	}
	ret = dsim_write_hl_data(dsim, SEQ_EA8061_SLEW_CONTROL, ARRAY_SIZE(SEQ_EA8061_SLEW_CONTROL));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_EA8061_SLEW_CONTROL\n", __func__);
		goto init_exit;
	}

#if 1
	/* control el_on by manual */

	/*Manual ON = 1*/
	ret = dsim_write_hl_data(dsim, SEQ_GP_01, ARRAY_SIZE(SEQ_GP_01));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_GP_01\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, SEQ_F3_ON, ARRAY_SIZE(SEQ_F3_ON));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_F3_ON\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, SEQ_SLEEP_OUT, ARRAY_SIZE(SEQ_SLEEP_OUT));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_SLEEP_OUT\n", __func__);
		goto init_exit;
	}

	msleep(20);

	/*EL_ON=1*/
	ret = dsim_write_hl_data(dsim, SEQ_GP_02, ARRAY_SIZE(SEQ_GP_02));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_GP_02\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, SEQ_F3_08, ARRAY_SIZE(SEQ_F3_08));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_F3_08\n", __func__);
		goto init_exit;
	}

	usleep_range(150, 160);

	/*EL_ON=0*/
	ret = dsim_write_hl_data(dsim, SEQ_GP_02, ARRAY_SIZE(SEQ_GP_02));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_GP_02\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, SEQ_F3_OFF, ARRAY_SIZE(SEQ_F3_OFF));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_F3_OFF\n", __func__);
		goto init_exit;
	}


	usleep_range(100, 110);


	/*EL_ON=1*/
	ret = dsim_write_hl_data(dsim, SEQ_GP_02, ARRAY_SIZE(SEQ_GP_02));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_GP_02\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, SEQ_F3_08, ARRAY_SIZE(SEQ_F3_08));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_F3_08\n", __func__);
		goto init_exit;
	}

	usleep_range(150, 160);

	/*EL_ON=0*/
	ret = dsim_write_hl_data(dsim, SEQ_GP_02, ARRAY_SIZE(SEQ_GP_02));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_GP_02\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, SEQ_F3_OFF, ARRAY_SIZE(SEQ_F3_OFF));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_F3_OFF\n", __func__);
		goto init_exit;
	}


	usleep_range(100, 110);


	ret = dsim_write_hl_data(dsim, SEQ_SLEEP_IN, ARRAY_SIZE(SEQ_SLEEP_IN));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_SLEEP_IN\n", __func__);
		goto init_exit;
	}

	/*Manual ON = 0*/
	ret = dsim_write_hl_data(dsim, SEQ_GP_01, ARRAY_SIZE(SEQ_GP_01));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_GP_01\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, SEQ_F3_OFF, ARRAY_SIZE(SEQ_F3_OFF));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_F3_OFF\n", __func__);
		goto init_exit;
	}

#endif

	ret = dsim_write_hl_data(dsim, SEQ_SLEEP_OUT, ARRAY_SIZE(SEQ_SLEEP_OUT));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_SLEEP_OUT\n", __func__);
		goto init_exit;
	}

	msleep(120);

	dsim_panel_set_brightness(dsim, 1);

#if 0
	ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_F1, ARRAY_SIZE(SEQ_TEST_KEY_ON_F1));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_TEST_KEY_ON_F1\n", __func__);
		goto init_exit;
	}
	ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_FC, ARRAY_SIZE(SEQ_TEST_KEY_ON_FC));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_TEST_KEY_ON_FC\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, SEQ_MONITOR_GLOBAL, ARRAY_SIZE(SEQ_MONITOR_GLOBAL));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_MONITOR_GLOBAL\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, SEQ_MONITOR_0, ARRAY_SIZE(SEQ_MONITOR_0));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_MONITOR_0\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, SEQ_MONITOR_1, ARRAY_SIZE(SEQ_MONITOR_1));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_MONITOR_1\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_F1, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F1));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_TEST_KEY_OFF_F1\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_FC, ARRAY_SIZE(SEQ_TEST_KEY_OFF_FC));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_TEST_KEY_OFF_FC\n", __func__);
		goto init_exit;
	}

#endif

	ret = dsim_write_hl_data(dsim, SEQ_DISPLAY_ON, ARRAY_SIZE(SEQ_DISPLAY_ON));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_EA8061_MPS_SET_MAX\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_FC, ARRAY_SIZE(SEQ_TEST_KEY_OFF_FC));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_OFF_FC\n", __func__);
	}

	ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_F0, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_OFF_F0\n", __func__);
	}

init_exit:
	return ret;
}

static int ea8061_probe(struct dsim_device *dsim)
{
	int ret = 0;
	struct panel_private *panel = &dsim->priv;
	unsigned char mtp[EA8061_MTP_SIZE] = {0, };

	dsim_info("MDD : %s was called\n", __func__);

	panel->dim_data = (void *)NULL;
	panel->lcdConnected = PANEL_CONNECTED;
	panel->panel_type = 0;

	ret = ea8061_read_init_info(dsim, mtp);
	if (panel->lcdConnected == PANEL_DISCONNEDTED) {
		dsim_err("dsim : %s lcd was not connected\n", __func__);
		goto probe_exit;
	}

#ifdef CONFIG_PANEL_AID_DIMMING
	ret = init_dimming(dsim, mtp);
	if (ret) {
		dsim_err("%s : failed to generate gamma tablen\n", __func__);
	}
#endif

probe_exit:
	return ret;
}

static int ea8061_early_probe(struct dsim_device *dsim)
{
	int ret = 0;

	return ret;
}

struct dsim_panel_ops ea8061_panel_ops = {
	.early_probe = ea8061_early_probe,
	.probe		= ea8061_probe,
	.displayon	= ea8061_displayon,
	.exit		= ea8061_exit,
	.init		= ea8061_init,
};

