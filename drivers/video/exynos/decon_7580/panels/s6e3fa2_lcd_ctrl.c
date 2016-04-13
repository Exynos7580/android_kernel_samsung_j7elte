/*
 * drivers/video/decon_7580/panels/s6e3fa2_lcd_ctrl.c
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
#include "../dsim.h"

#include "panel_info.h"

#ifdef CONFIG_PANEL_AID_DIMMING
#include "aid_dimming.h"
#include "dimming_core.h"
#include "s6e3fa2_aid_dimming.h"
#include "dsim_backlight.h"
#endif

#ifdef CONFIG_PANEL_AID_DIMMING
static const unsigned char *HBM_TABLE[HBM_STATUS_MAX] = {SEQ_HBM_OFF, SEQ_HBM_ON};
static const unsigned char *ACL_CUTOFF_TABLE[ACL_STATUS_MAX] = {SEQ_ACL_OFF, SEQ_ACL_15};
static const unsigned char *ACL_OPR_TABLE[ACL_OPR_MAX] = {SEQ_ACL_OFF_OPR_AVR, SEQ_ACL_ON_OPR_AVR};

static const unsigned int br_tbl [256] = {
	2,		2,		2,		2,		2,		2,		2,		3,		3,		3,		3,		4,		4,		4,		5,		5,
	5,		6,		6,		7,		7,		8,		8,		9,		9,		10,		10,		11,		11,		12,		13,		13,
	14,		14,		15,		16,		16,		17,		18,		19,		19,		20,		21,		21,		22,		23,		24,		25,
	25,		26,		27,		28,		29,		30,		30,		31,		32,		33,		34,		35,		36,		37,		38,		39,
	40,		41,		42,		43,		44,		45,		46,		47,		48,		49,		50,		51,		52,		53,		54,		55,
	56,		58,		59,		60,		61,		62,		63,		65,		66,		67,		68,		69,		70,		72,		73,		74,
	75,		77,		78,		79,		80,		82,		83,		84,		86,		87,		88,		90,		91,		92,		94,		95,
	96,		98,		99,		101,	102,	103,	105,	106,	108,	109,	111,	112,	113,	115,	116,	118,
	119,	121,	122,	124,	125,	127,	128,	130,	131,	133,	135,	136,	138,	139,	141,	142,
	144,	146,	147,	149,	151,	152,	154,	155,	157,	159,	160,	162,	164,	165,	167,	169,
	171,	172,	174,	176,	177,	179,	181,	183,	184,	186,	188,	190,	191,	193,	195,	197,
	199,	200,	202,	204,	206,	208,	210,	211,	213,	215,	217,	219,	221,	223,	225,	227,
	228,	230,	232,	234,	236,	238,	240,	242,	244,	246,	248,	250,	252,	254,	256,	258,
	260,	262,	264,	266,	268,	270,	272,	274,	276,	278,	280,	282,	284,	286,	288,	290,
	292,	295,	297,	299,	301,	303,	305,	307,	309,	312,	314,	316,	318,	320,	322,	324,
	327,	329,	331,	333,	335,	338,	340,	342,	344,	347,	349,	350,	350,	350,	350,	350,
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

struct SmtDimInfo daisy_dimming_info[MAX_BR_INFO + 1] = {				// add hbm array
	{.br = 2, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl2nit, .cTbl = ctbl2nit, .aid = aid9793, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 3, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl3nit, .cTbl = ctbl3nit, .aid = aid9711, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 4, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl4nit, .cTbl = ctbl4nit, .aid = aid9633, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 5, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl5nit, .cTbl = ctbl5nit, .aid = aid9551, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 6, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl6nit, .cTbl = ctbl6nit, .aid = aid9468, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 7, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl7nit, .cTbl = ctbl7nit, .aid = aid9385, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 8, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl8nit, .cTbl = ctbl8nit, .aid = aid9303, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 9, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl9nit, .cTbl = ctbl9nit, .aid = aid9215, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 10, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl10nit, .cTbl = ctbl10nit, .aid = aid9127, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 11, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl11nit, .cTbl = ctbl11nit, .aid = aid9044, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 12, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl12nit, .cTbl = ctbl12nit, .aid = aid8967, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 13, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl13nit, .cTbl = ctbl13nit, .aid = aid8879, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 14, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl14nit, .cTbl = ctbl14nit, .aid = aid8786, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 15, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl15nit, .cTbl = ctbl15nit, .aid = aid8704, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 16, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl16nit, .cTbl = ctbl16nit, .aid = aid8611, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 17, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl17nit, .cTbl = ctbl17nit, .aid = aid8528, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 19, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl19nit, .cTbl = ctbl19nit, .aid = aid8363, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 20, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl20nit, .cTbl = ctbl20nit, .aid = aid8177, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 21, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl21nit, .cTbl = ctbl21nit, .aid = aid8135, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 22, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl22nit, .cTbl = ctbl22nit, .aid = aid8053, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 24, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl24nit, .cTbl = ctbl24nit, .aid = aid7929, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 25, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl25nit, .cTbl = ctbl25nit, .aid = aid7846, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 27, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl27nit, .cTbl = ctbl27nit, .aid = aid7665, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 29, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl29nit, .cTbl = ctbl29nit, .aid = aid7495, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 30, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl30nit, .cTbl = ctbl30nit, .aid = aid7397, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 32, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl32nit, .cTbl = ctbl32nit, .aid = aid7221, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 34, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl34nit, .cTbl = ctbl34nit, .aid = aid7040, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 37, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl37nit, .cTbl = ctbl37nit, .aid = aid6761, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 39, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl39nit, .cTbl = ctbl39nit, .aid = aid6586, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 41, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl41nit, .cTbl = ctbl41nit, .aid = aid6374, .elvAcl = elvAcl4, .elv = elv4},
	{.br = 44, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl44nit, .cTbl = ctbl44nit, .aid = aid6105, .elvAcl = elvAcl4, .elv = elv4},
	{.br = 47, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl47nit, .cTbl = ctbl47nit, .aid = aid5795, .elvAcl = elvAcl3, .elv = elv3},
	{.br = 50, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl50nit, .cTbl = ctbl50nit, .aid = aid5511, .elvAcl = elvAcl3, .elv = elv3},
	{.br = 53, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl53nit, .cTbl = ctbl53nit, .aid = aid5191, .elvAcl = elvAcl2, .elv = elv2},
	{.br = 56, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl56nit, .cTbl = ctbl56nit, .aid = aid4861, .elvAcl = elvAcl1, .elv = elv1},
	{.br = 60, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl60nit, .cTbl = ctbl60nit, .aid = aid4411, .elvAcl = elvAcl0, .elv = elv0},
	{.br = 64, .refBr = 115, .cGma = gma2p15, .rTbl = rtbl64nit, .cTbl = ctbl64nit, .aid = aid4179, .elvAcl = elvAcl0, .elv = elv0},
	{.br = 68, .refBr = 121, .cGma = gma2p15, .rTbl = rtbl68nit, .cTbl = ctbl68nit, .aid = aid4179, .elvAcl = elvAcl0, .elv = elv0},
	{.br = 72, .refBr = 128, .cGma = gma2p15, .rTbl = rtbl72nit, .cTbl = ctbl72nit, .aid = aid4179, .elvAcl = elvAcl0, .elv = elv0},
	{.br = 77, .refBr = 136, .cGma = gma2p15, .rTbl = rtbl77nit, .cTbl = ctbl77nit, .aid = aid4179, .elvAcl = elvAcl0, .elv = elv0},
	{.br = 82, .refBr = 143, .cGma = gma2p15, .rTbl = rtbl82nit, .cTbl = ctbl82nit, .aid = aid4179, .elvAcl = elvAcl1, .elv = elv1},
	{.br = 87, .refBr = 151, .cGma = gma2p15, .rTbl = rtbl87nit, .cTbl = ctbl87nit, .aid = aid4179, .elvAcl = elvAcl1, .elv = elv1},
	{.br = 93, .refBr = 161, .cGma = gma2p15, .rTbl = rtbl93nit, .cTbl = ctbl93nit, .aid = aid4179, .elvAcl = elvAcl1, .elv = elv1},
	{.br = 98, .refBr = 169, .cGma = gma2p15, .rTbl = rtbl98nit, .cTbl = ctbl98nit, .aid = aid4179, .elvAcl = elvAcl2, .elv = elv2},
	{.br = 105, .refBr = 180, .cGma = gma2p15, .rTbl = rtbl105nit, .cTbl = ctbl105nit, .aid = aid4179, .elvAcl = elvAcl2, .elv = elv2},
	{.br = 111, .refBr = 189, .cGma = gma2p15, .rTbl = rtbl111nit, .cTbl = ctbl111nit, .aid = aid4179, .elvAcl = elvAcl3, .elv = elv3},
	{.br = 119, .refBr = 200, .cGma = gma2p15, .rTbl = rtbl119nit, .cTbl = ctbl119nit, .aid = aid4179, .elvAcl = elvAcl3, .elv = elv3},
	{.br = 126, .refBr = 213, .cGma = gma2p15, .rTbl = rtbl126nit, .cTbl = ctbl126nit, .aid = aid4179, .elvAcl = elvAcl4, .elv = elv4},
	{.br = 134, .refBr = 224, .cGma = gma2p15, .rTbl = rtbl134nit, .cTbl = ctbl134nit, .aid = aid4179, .elvAcl = elvAcl4, .elv = elv4},
	{.br = 143, .refBr = 236, .cGma = gma2p15, .rTbl = rtbl143nit, .cTbl = ctbl143nit, .aid = aid4179, .elvAcl = elvAcl5, .elv = elv5},
	{.br = 152, .refBr = 249, .cGma = gma2p15, .rTbl = rtbl152nit, .cTbl = ctbl152nit, .aid = aid4179, .elvAcl = elvAcl6, .elv = elv6},
	{.br = 162, .refBr = 245, .cGma = gma2p15, .rTbl = rtbl162nit, .cTbl = ctbl162nit, .aid = aid3626, .elvAcl = elvAcl7, .elv = elv7},
	{.br = 172, .refBr = 245, .cGma = gma2p15, .rTbl = rtbl172nit, .cTbl = ctbl172nit, .aid = aid3177, .elvAcl = elvAcl7, .elv = elv7},
	{.br = 183, .refBr = 249, .cGma = gma2p15, .rTbl = rtbl183nit, .cTbl = ctbl183nit, .aid = aid2794, .elvAcl = elvAcl7, .elv = elv7},
	{.br = 195, .refBr = 249, .cGma = gma2p15, .rTbl = rtbl195nit, .cTbl = ctbl195nit, .aid = aid2299, .elvAcl = elvAcl7, .elv = elv7},
	{.br = 207, .refBr = 249, .cGma = gma2p15, .rTbl = rtbl207nit, .cTbl = ctbl207nit, .aid = aid1725, .elvAcl = elvAcl7, .elv = elv7},
	{.br = 220, .refBr = 249, .cGma = gma2p15, .rTbl = rtbl220nit, .cTbl = ctbl220nit, .aid = aid1105, .elvAcl = elvAcl7, .elv = elv7},
	{.br = 234, .refBr = 249, .cGma = gma2p15, .rTbl = rtbl234nit, .cTbl = ctbl234nit, .aid = aid0429, .elvAcl = elvAcl7, .elv = elv7},
	{.br = 249, .refBr = 249, .cGma = gma2p15, .rTbl = rtbl249nit, .cTbl = ctbl249nit, .aid = aid0072, .elvAcl = elvAcl7, .elv = elv7},		// 249 ~ 360nit acl off
	{.br = 265, .refBr = 265, .cGma = gma2p15, .rTbl = rtbl265nit, .cTbl = ctbl265nit, .aid = aid0072, .elvAcl = elvAcl8, .elv = elv8},
	{.br = 282, .refBr = 282, .cGma = gma2p15, .rTbl = rtbl282nit, .cTbl = ctbl282nit, .aid = aid0072, .elvAcl = elvAcl9, .elv = elv9},
	{.br = 300, .refBr = 300, .cGma = gma2p15, .rTbl = rtbl300nit, .cTbl = ctbl300nit, .aid = aid0072, .elvAcl = elvAcl10, .elv = elv10},
	{.br = 316, .refBr = 316, .cGma = gma2p15, .rTbl = rtbl316nit, .cTbl = ctbl316nit, .aid = aid0072, .elvAcl = elvAcl10, .elv = elv10},
	{.br = 333, .refBr = 333, .cGma = gma2p15, .rTbl = rtbl333nit, .cTbl = ctbl333nit, .aid = aid0072, .elvAcl = elvAcl11, .elv = elv11},
	{.br = 350, .refBr = 350, .cGma = gma2p20, .rTbl = rtbl350nit, .cTbl = ctbl350nit, .aid = aid0072, .elvAcl = elvAcl12, .elv = elv12},
	{.br = 350, .refBr = 350, .cGma = gma2p20, .rTbl = rtbl350nit, .cTbl = ctbl350nit, .aid = aid0072, .elvAcl = elvAcl12, .elv = elv12},
};

struct SmtDimInfo EA8064G_daisy_dimming_info_[MAX_BR_INFO + 1] = {				// add hbm array
	{.br = 2, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl2nit, .cTbl = ctbl2nit, .aid = aid9793_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 3, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl3nit, .cTbl = ctbl3nit, .aid = aid9711_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 4, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl4nit, .cTbl = ctbl4nit, .aid = aid9633_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 5, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl5nit, .cTbl = ctbl5nit, .aid = aid9551_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 6, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl6nit, .cTbl = ctbl6nit, .aid = aid9468_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 7, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl7nit, .cTbl = ctbl7nit, .aid = aid9385_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 8, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl8nit, .cTbl = ctbl8nit, .aid = aid9303_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 9, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl9nit, .cTbl = ctbl9nit, .aid = aid9215_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 10, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl10nit, .cTbl = ctbl10nit, .aid = aid9127_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 11, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl11nit, .cTbl = ctbl11nit, .aid = aid9044_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 12, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl12nit, .cTbl = ctbl12nit, .aid = aid8967_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 13, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl13nit, .cTbl = ctbl13nit, .aid = aid8879_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 14, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl14nit, .cTbl = ctbl14nit, .aid = aid8786_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 15, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl15nit, .cTbl = ctbl15nit, .aid = aid8704_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 16, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl16nit, .cTbl = ctbl16nit, .aid = aid8611_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 17, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl17nit, .cTbl = ctbl17nit, .aid = aid8528_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 19, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl19nit, .cTbl = ctbl19nit, .aid = aid8363_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 20, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl20nit, .cTbl = ctbl20nit, .aid = aid8177_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 21, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl21nit, .cTbl = ctbl21nit, .aid = aid8135_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 22, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl22nit, .cTbl = ctbl22nit, .aid = aid8053_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 24, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl24nit, .cTbl = ctbl24nit, .aid = aid7929_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 25, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl25nit, .cTbl = ctbl25nit, .aid = aid7846_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 27, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl27nit, .cTbl = ctbl27nit, .aid = aid7665_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 29, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl29nit, .cTbl = ctbl29nit, .aid = aid7495_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 30, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl30nit, .cTbl = ctbl30nit, .aid = aid7397_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 32, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl32nit, .cTbl = ctbl32nit, .aid = aid7221_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 34, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl34nit, .cTbl = ctbl34nit, .aid = aid7040_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 37, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl37nit, .cTbl = ctbl37nit, .aid = aid6761_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 39, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl39nit, .cTbl = ctbl39nit, .aid = aid6586_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 41, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl41nit, .cTbl = ctbl41nit, .aid = aid6374_ea, .elvAcl = elvAcl4_ea, .elv = elv4_ea},
	{.br = 44, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl44nit, .cTbl = ctbl44nit, .aid = aid6105_ea, .elvAcl = elvAcl4_ea, .elv = elv4_ea},
	{.br = 47, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl47nit, .cTbl = ctbl47nit, .aid = aid5795_ea, .elvAcl = elvAcl3_ea, .elv = elv3_ea},
	{.br = 50, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl50nit, .cTbl = ctbl50nit, .aid = aid5511_ea, .elvAcl = elvAcl3_ea, .elv = elv3_ea},
	{.br = 53, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl53nit, .cTbl = ctbl53nit, .aid = aid5191_ea, .elvAcl = elvAcl2_ea, .elv = elv2_ea},
	{.br = 56, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl56nit, .cTbl = ctbl56nit, .aid = aid4861_ea, .elvAcl = elvAcl1_ea, .elv = elv1_ea},
	{.br = 60, .refBr = 110, .cGma = gma2p15, .rTbl = rtbl60nit, .cTbl = ctbl60nit, .aid = aid4411_ea, .elvAcl = elvAcl0_ea, .elv = elv0_ea},
	{.br = 64, .refBr = 115, .cGma = gma2p15, .rTbl = rtbl64nit, .cTbl = ctbl64nit, .aid = aid4179_ea, .elvAcl = elvAcl0_ea, .elv = elv0_ea},
	{.br = 68, .refBr = 121, .cGma = gma2p15, .rTbl = rtbl68nit, .cTbl = ctbl68nit, .aid = aid4179_ea, .elvAcl = elvAcl0_ea, .elv = elv0_ea},
	{.br = 72, .refBr = 128, .cGma = gma2p15, .rTbl = rtbl72nit, .cTbl = ctbl72nit, .aid = aid4179_ea, .elvAcl = elvAcl0_ea, .elv = elv0_ea},
	{.br = 77, .refBr = 136, .cGma = gma2p15, .rTbl = rtbl77nit, .cTbl = ctbl77nit, .aid = aid4179_ea, .elvAcl = elvAcl0_ea, .elv = elv0_ea},
	{.br = 82, .refBr = 143, .cGma = gma2p15, .rTbl = rtbl82nit, .cTbl = ctbl82nit, .aid = aid4179_ea, .elvAcl = elvAcl1_ea, .elv = elv1_ea},
	{.br = 87, .refBr = 151, .cGma = gma2p15, .rTbl = rtbl87nit, .cTbl = ctbl87nit, .aid = aid4179_ea, .elvAcl = elvAcl1_ea, .elv = elv1_ea},
	{.br = 93, .refBr = 161, .cGma = gma2p15, .rTbl = rtbl93nit, .cTbl = ctbl93nit, .aid = aid4179_ea, .elvAcl = elvAcl1_ea, .elv = elv1_ea},
	{.br = 98, .refBr = 169, .cGma = gma2p15, .rTbl = rtbl98nit, .cTbl = ctbl98nit, .aid = aid4179_ea, .elvAcl = elvAcl2_ea, .elv = elv2_ea},
	{.br = 105, .refBr = 180, .cGma = gma2p15, .rTbl = rtbl105nit, .cTbl = ctbl105nit, .aid = aid4179_ea, .elvAcl = elvAcl2_ea, .elv = elv2_ea},
	{.br = 111, .refBr = 189, .cGma = gma2p15, .rTbl = rtbl111nit, .cTbl = ctbl111nit, .aid = aid4179_ea, .elvAcl = elvAcl3_ea, .elv = elv3_ea},
	{.br = 119, .refBr = 200, .cGma = gma2p15, .rTbl = rtbl119nit, .cTbl = ctbl119nit, .aid = aid4179_ea, .elvAcl = elvAcl3_ea, .elv = elv3_ea},
	{.br = 126, .refBr = 213, .cGma = gma2p15, .rTbl = rtbl126nit, .cTbl = ctbl126nit, .aid = aid4179_ea, .elvAcl = elvAcl4_ea, .elv = elv4_ea},
	{.br = 134, .refBr = 224, .cGma = gma2p15, .rTbl = rtbl134nit, .cTbl = ctbl134nit, .aid = aid4179_ea, .elvAcl = elvAcl4_ea, .elv = elv4_ea},
	{.br = 143, .refBr = 236, .cGma = gma2p15, .rTbl = rtbl143nit, .cTbl = ctbl143nit, .aid = aid4179_ea, .elvAcl = elvAcl5_ea, .elv = elv5_ea},
	{.br = 152, .refBr = 249, .cGma = gma2p15, .rTbl = rtbl152nit, .cTbl = ctbl152nit, .aid = aid4179_ea, .elvAcl = elvAcl6_ea, .elv = elv6_ea},
	{.br = 162, .refBr = 245, .cGma = gma2p15, .rTbl = rtbl162nit, .cTbl = ctbl162nit, .aid = aid3626_ea, .elvAcl = elvAcl7_ea, .elv = elv7_ea},
	{.br = 172, .refBr = 245, .cGma = gma2p15, .rTbl = rtbl172nit, .cTbl = ctbl172nit, .aid = aid3177_ea, .elvAcl = elvAcl7_ea, .elv = elv7_ea},
	{.br = 183, .refBr = 249, .cGma = gma2p15, .rTbl = rtbl183nit, .cTbl = ctbl183nit, .aid = aid2794_ea, .elvAcl = elvAcl7_ea, .elv = elv7_ea},
	{.br = 195, .refBr = 249, .cGma = gma2p15, .rTbl = rtbl195nit, .cTbl = ctbl195nit, .aid = aid2299_ea, .elvAcl = elvAcl7_ea, .elv = elv7_ea},
	{.br = 207, .refBr = 249, .cGma = gma2p15, .rTbl = rtbl207nit, .cTbl = ctbl207nit, .aid = aid1725_ea, .elvAcl = elvAcl7_ea, .elv = elv7_ea},
	{.br = 220, .refBr = 249, .cGma = gma2p15, .rTbl = rtbl220nit, .cTbl = ctbl220nit, .aid = aid1105_ea, .elvAcl = elvAcl7_ea, .elv = elv7_ea},
	{.br = 234, .refBr = 249, .cGma = gma2p15, .rTbl = rtbl234nit, .cTbl = ctbl234nit, .aid = aid0429_ea, .elvAcl = elvAcl7_ea, .elv = elv7_ea},
	{.br = 249, .refBr = 249, .cGma = gma2p15, .rTbl = rtbl249nit, .cTbl = ctbl249nit, .aid = aid0072_ea, .elvAcl = elvAcl7_ea, .elv = elv7_ea},		// 249 ~ 360nit acl off
	{.br = 265, .refBr = 265, .cGma = gma2p15, .rTbl = rtbl265nit, .cTbl = ctbl265nit, .aid = aid0072_ea, .elvAcl = elvAcl8_ea, .elv = elv8_ea},
	{.br = 282, .refBr = 282, .cGma = gma2p15, .rTbl = rtbl282nit, .cTbl = ctbl282nit, .aid = aid0072_ea, .elvAcl = elvAcl9_ea, .elv = elv9_ea},
	{.br = 300, .refBr = 300, .cGma = gma2p15, .rTbl = rtbl300nit, .cTbl = ctbl300nit, .aid = aid0072_ea, .elvAcl = elvAcl10_ea, .elv = elv10_ea},
	{.br = 316, .refBr = 316, .cGma = gma2p15, .rTbl = rtbl316nit, .cTbl = ctbl316nit, .aid = aid0072_ea, .elvAcl = elvAcl10_ea, .elv = elv10_ea},
	{.br = 333, .refBr = 333, .cGma = gma2p15, .rTbl = rtbl333nit, .cTbl = ctbl333nit, .aid = aid0072_ea, .elvAcl = elvAcl11_ea, .elv = elv11_ea},
	{.br = 350, .refBr = 350, .cGma = gma2p20, .rTbl = rtbl350nit, .cTbl = ctbl350nit, .aid = aid0072_ea, .elvAcl = elvAcl12_ea, .elv = elv12_ea},
	{.br = 350, .refBr = 350, .cGma = gma2p20, .rTbl = rtbl350nit, .cTbl = ctbl350nit, .aid = aid0072_ea, .elvAcl = elvAcl12_ea, .elv = elv12_ea},
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

	if (dsim->priv.panel_type == LCD_EA8064G)
		diminfo = EA8064G_daisy_dimming_info_;
	else
		diminfo = daisy_dimming_info;

	panel->dim_data= (void *)dimming;
	panel->dim_info = (void *)diminfo;

	panel->br_tbl = (unsigned int *)br_tbl;
	panel->hbm_tbl = (unsigned char **)HBM_TABLE;
	panel->acl_cutoff_tbl = (unsigned char **)ACL_CUTOFF_TABLE;
	panel->acl_opr_tbl = (unsigned char **)ACL_OPR_TABLE;

	for (j = 0; j < CI_MAX; j++) {
		temp = ((mtp[pos] & 0x01) ? -1 : 1) * mtp[pos+1];
		dimming->t_gamma[V255][j] = (int)center_gamma[V255][j] + temp;
		dimming->mtp[V255][j] = temp;
		pos += 2;
	}

	for (i = V203; i >= V0; i--) {
		for (j = 0; j < CI_MAX; j++) {
			temp = ((mtp[pos] & 0x80) ? -1 : 1) * (mtp[pos] & 0x7f);
			dimming->t_gamma[i][j] = (int)center_gamma[i][j] + temp;
			dimming->mtp[i][j] = temp;
			pos++;
		}
	}
	/* for vt */
	temp = (mtp[pos+1]) << 8 | mtp[pos];

	for(i=0;i<CI_MAX;i++)
		dimming->vt_mtp[i] = 0;
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

static int s6e3fa2_read_init_info(struct dsim_device *dsim, unsigned char* mtp)
{
	int i = 0;
	int ret;
	struct panel_private *panel = &dsim->priv;
	unsigned char bufForCoordi[S6E3FA2_COORDINATE_LEN] = {0,};
	unsigned char buf[S6E3FA2_MTP_DATE_SIZE] = {0, };

	dsim_info("MDD : %s was called\n", __func__);

	ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(SEQ_TEST_KEY_ON_F0));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_ON_F0\n", __func__);
	}

	// id
	ret = dsim_read_hl_data(dsim, S6E3FA2_ID_REG, S6E3FA2_ID_LEN, dsim->priv.id);
	if (ret != S6E3FA2_ID_LEN) {
		dsim_err("%s : can't find connected panel. check panel connection\n",__func__);
		panel->lcdConnected = PANEL_DISCONNEDTED;
		goto read_exit;
	}

	dsim_info("READ ID : ");
	for(i = 0; i < S6E3FA2_ID_LEN; i++)
		dsim_info("%02x, ", dsim->priv.id[i]);
	dsim_info("\n");

	if (dsim->priv.id[S6E3FA2_ID_LEN-1] >> 6) {
		dsim->priv.panel_type = LCD_EA8064G;
		dsim->lcd_info.ddi_type = TYPE_OF_MAGNA_DDI;

		ACL_OPR_TABLE[0] = SEQ_ACL_OFF_OPR_AVR_EA8064G;
		ACL_OPR_TABLE[1] = SEQ_ACL_ON_OPR_AVR_EA8064G;

		dsim_info("Panel : EA8064G (Magna Chip)\n");
	} else {
		dsim->priv.panel_type = LCD_S6E3FA2;
		dsim_info("Panel : S6E3FA2 (S.LSI Chip)\n");
	}

	// mtp
	ret = dsim_read_hl_data(dsim, S6E3FA2_MTP_ADDR, S6E3FA2_MTP_DATE_SIZE, buf);
	if (ret != S6E3FA2_MTP_DATE_SIZE) {
		dsim_err("failed to read mtp, check panel connection\n");
		goto read_fail;
	}
	memcpy(mtp, buf, S6E3FA2_MTP_SIZE);
	memcpy(dsim->priv.date, &buf[40], ARRAY_SIZE(dsim->priv.date));
	dsim_info("READ MTP SIZE : %d\n", S6E3FA2_MTP_SIZE);
	dsim_info("=========== MTP INFO =========== \n");
	for (i = 0; i < S6E3FA2_MTP_SIZE; i++)
		dsim_info("MTP[%2d] : %2d : %2x\n", i, mtp[i], mtp[i]);

	//elvss

	ret = dsim_read_hl_data(dsim, ELVSS_REG, ELVSS_LEN, dsim->priv.elvss);
	if (ret != ELVSS_LEN) {
		dsim_err("failed to read mtp, check panel connection\n");
		goto read_fail;
	}

	// coordinate
	ret = dsim_read_hl_data(dsim, S6E3FA2_COORDINATE_REG, S6E3FA2_COORDINATE_LEN, bufForCoordi);
	if (ret != S6E3FA2_COORDINATE_LEN) {
		dsim_err("fail to read coordinate on command.\n");
		goto read_fail;
	}
	dsim->priv.coordinate[0] = bufForCoordi[0] << 8 | bufForCoordi[1];	/* X */
	dsim->priv.coordinate[1] = bufForCoordi[2] << 8 | bufForCoordi[3];	/* Y */
	dsim_info("READ coordi : ");
	for(i = 0; i < 2; i++)
		dsim_info("%d, ", dsim->priv.coordinate[i]);
	dsim_info("\n");


	// code
	ret = dsim_read_hl_data(dsim, S6E3FA2_CODE_REG, S6E3FA2_CODE_LEN, dsim->priv.code);
	if (ret != S6E3FA2_CODE_LEN) {
		dsim_err("fail to read code on command.\n");
		goto read_fail;
	}
	dsim_info("READ code : ");
	for(i = 0; i < S6E3FA2_CODE_LEN; i++)
		dsim_info("%x, ", dsim->priv.code[i]);
	dsim_info("\n");

	ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_F0, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_OFF_F0\n", __func__);
		goto read_fail;
	}
read_exit:
	return 0;

read_fail:
	return -ENODEV;

}

static int s6e3fa2_probe(struct dsim_device *dsim)
{
	int ret = 0;
	struct panel_private *panel = &dsim->priv;
	unsigned char mtp[S6E3FA2_MTP_SIZE] = {0, };

	dsim_info("MDD : %s was called\n", __func__);

	panel->dim_data = (void *)NULL;
	panel->lcdConnected = PANEL_CONNECTED;
	panel->panel_type = 0;

	ret = s6e3fa2_read_init_info(dsim, mtp);
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


static int s6e3fa2_displayon(struct dsim_device *dsim)
{
	int ret = 0;
#ifdef CONFIG_LCD_ALPM
	struct panel_private *panel = &dsim->priv;
#endif

	dsim_info("MDD : %s was called\n", __func__);

	ret = dsim_write_hl_data(dsim, SEQ_DISPLAY_ON, ARRAY_SIZE(SEQ_DISPLAY_ON));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : DISPLAY_ON\n", __func__);
 		goto displayon_err;
	}

displayon_err:
	return ret;

}

static int s6e3fa2_exit(struct dsim_device *dsim)
{
	int ret = 0;
	dsim_info("MDD : %s was called\n", __func__);
	ret = dsim_write_hl_data(dsim, SEQ_DISPLAY_OFF, ARRAY_SIZE(SEQ_DISPLAY_OFF));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : DISPLAY_OFF\n", __func__);
		goto exit_err;
	}

	ret = dsim_write_hl_data(dsim, SEQ_SLEEP_IN, ARRAY_SIZE(SEQ_SLEEP_IN));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SLEEP_IN\n", __func__);
		goto exit_err;
	}

	msleep(120);

exit_err:
	return ret;
}

static int s6e3fa2_init(struct dsim_device *dsim)
{
	int ret = 0;

	dsim_info("MDD : %s was called\n", __func__);

	if (dsim->priv.panel_type == LCD_EA8064G) {
		ret = dsim_panel_set_brightness(dsim, 1);
		if (ret) {
			dsim_err("%s : fail to set brightness\n", __func__);
		}
	}

	ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(SEQ_TEST_KEY_ON_F0));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_ON_F0\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_FC, ARRAY_SIZE(SEQ_TEST_KEY_ON_FC));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_ON_FC\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, SEQ_SLEEP_OUT, ARRAY_SIZE(SEQ_SLEEP_OUT));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_SLEEP_OUT\n", __func__);
		goto init_exit;
	}

	msleep(20);

#ifndef CONFIG_PANEL_AID_DIMMING
	/* Brightness Setting */
	ret = dsim_write_hl_data(dsim, SEQ_GAMMA_CONDITION_SET, ARRAY_SIZE(SEQ_GAMMA_CONDITION_SET));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_GAMMA_CONDITION_SET\n", __func__);
		goto init_exit;
	}
	ret = dsim_write_hl_data(dsim, SEQ_AID_SET, ARRAY_SIZE(SEQ_AID_SET));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_AID_SET\n", __func__);
		goto init_exit;
	}
	ret = dsim_write_hl_data(dsim, SEQ_ELVSS_SET, ARRAY_SIZE(SEQ_ELVSS_SET));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_ELVSS_SET\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, SEQ_GAMMA_UPDATE, ARRAY_SIZE(SEQ_GAMMA_UPDATE));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_GAMMA_UPDATE\n", __func__);
		goto init_exit;
	}
	ret = dsim_write_hl_data(dsim, SEQ_ACL_SET, ARRAY_SIZE(SEQ_ACL_SET));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_ACL_SET\n", __func__);
		goto init_exit;
	}
	ret = dsim_write_hl_data(dsim, SEQ_ACL_ON_OPR_AVR, ARRAY_SIZE(SEQ_ACL_ON_OPR_AVR));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_ACL_ON_OPR_AVR\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, SEQ_TSET_GLOBAL, ARRAY_SIZE(SEQ_TSET_GLOBAL));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_TSET_GLOBAL\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, SEQ_TSET, ARRAY_SIZE(SEQ_TSET));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_TSET\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, SEQ_CAPS_ELVSS_SET, ARRAY_SIZE(SEQ_CAPS_ELVSS_SET));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_CAPS_ELVSS_SET\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, SEQ_ERR_FG, ARRAY_SIZE(SEQ_ERR_FG));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_ERR_FG\n", __func__);
		goto init_exit;
	}

#endif

	if (dsim->priv.panel_type == LCD_EA8064G) {
		ret = dsim_write_hl_data(dsim, SEQ_SOURCE_CONTROL, ARRAY_SIZE(SEQ_SOURCE_CONTROL));
		if (ret < 0) {
			dsim_err(":%s fail to write CMD : SEQ_SOURCE_CONTROL\n", __func__);
			goto init_exit;
		}
	}

	ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_FC, ARRAY_SIZE(SEQ_TEST_KEY_OFF_FC));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_TEST_KEY_OFF_FC\n", __func__);
		goto init_exit;
	}

	msleep(120);

	ret = dsim_write_hl_data(dsim, SEQ_TE_OUT, ARRAY_SIZE(SEQ_TE_OUT));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_TE_OUT\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_F0, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_OFF_F0\n", __func__);
		goto init_exit;
	}

init_exit:
	return ret;
}


struct dsim_panel_ops s6e3fa2_panel_ops = {
	.early_probe = NULL,
	.probe		= s6e3fa2_probe,
	.displayon	= s6e3fa2_displayon,
	.exit		= s6e3fa2_exit,
	.init		= s6e3fa2_init,
};

struct dsim_panel_ops *dsim_panel_get_priv_ops(struct dsim_device *dsim)
{
	return &s6e3fa2_panel_ops;
}
