/*
 * drivers/video/decon_7580/panels/s6e3fa3_lcd_ctrl.c
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
#include "s6e3fa3_aid_dimming.h"
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
	327,	329,	331,	333,	335,	338,	340,	342,	344,	347,	349,	350,	350,	360,	360,	360,
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
	{.br = 2, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl2nit, .cTbl = ctbl2nit, .aid = aid9845, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 3, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl3nit, .cTbl = ctbl3nit, .aid = aid9768, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 4, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl4nit, .cTbl = ctbl4nit, .aid = aid9685, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 5, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl5nit, .cTbl = ctbl5nit, .aid = aid9597, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 6, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl6nit, .cTbl = ctbl6nit, .aid = aid9520, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 7, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl7nit, .cTbl = ctbl7nit, .aid = aid9437, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 8, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl8nit, .cTbl = ctbl8nit, .aid = aid9354, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 9, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl9nit, .cTbl = ctbl9nit, .aid = aid9272, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 10, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl10nit, .cTbl = ctbl10nit, .aid = aid9189, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 11, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl11nit, .cTbl = ctbl11nit, .aid = aid9096, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 12, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl12nit, .cTbl = ctbl12nit, .aid = aid8982, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 13, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl13nit, .cTbl = ctbl13nit, .aid = aid8931, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 14, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl14nit, .cTbl = ctbl14nit, .aid = aid8802, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 15, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl15nit, .cTbl = ctbl15nit, .aid = aid8714, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 16, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl16nit, .cTbl = ctbl16nit, .aid = aid8626, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 17, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl17nit, .cTbl = ctbl17nit, .aid = aid8574, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 19, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl19nit, .cTbl = ctbl19nit, .aid = aid8414, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 20, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl20nit, .cTbl = ctbl20nit, .aid = aid8270, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 21, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl21nit, .cTbl = ctbl21nit, .aid = aid8192, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 22, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl22nit, .cTbl = ctbl22nit, .aid = aid8115, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 24, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl24nit, .cTbl = ctbl24nit, .aid = aid7944, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 25, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl25nit, .cTbl = ctbl25nit, .aid = aid7862, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 27, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl27nit, .cTbl = ctbl27nit, .aid = aid7639, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 29, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl29nit, .cTbl = ctbl29nit, .aid = aid7531, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 30, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl30nit, .cTbl = ctbl30nit, .aid = aid7454, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 32, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl32nit, .cTbl = ctbl32nit, .aid = aid7309, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 34, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl34nit, .cTbl = ctbl34nit, .aid = aid7133, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 37, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl37nit, .cTbl = ctbl37nit, .aid = aid6865, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 39, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl39nit, .cTbl = ctbl39nit, .aid = aid6730, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 41, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl41nit, .cTbl = ctbl41nit, .aid = aid6560, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 44, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl44nit, .cTbl = ctbl44nit, .aid = aid6312, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 47, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl47nit, .cTbl = ctbl47nit, .aid = aid6059, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 50, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl50nit, .cTbl = ctbl50nit, .aid = aid5790, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 53, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl53nit, .cTbl = ctbl53nit, .aid = aid5496, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 56, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl56nit, .cTbl = ctbl56nit, .aid = aid5238, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 60, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl60nit, .cTbl = ctbl60nit, .aid = aid4892, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 64, .refBr = 113, .cGma = gma2p15, .rTbl = rtbl64nit, .cTbl = ctbl64nit, .aid = aid4809, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 68, .refBr = 116, .cGma = gma2p15, .rTbl = rtbl68nit, .cTbl = ctbl68nit, .aid = aid4246, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 72, .refBr = 123, .cGma = gma2p15, .rTbl = rtbl72nit, .cTbl = ctbl72nit, .aid = aid4246, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 77, .refBr = 131, .cGma = gma2p15, .rTbl = rtbl77nit, .cTbl = ctbl77nit, .aid = aid4246, .elvAcl = delvAcl0, .elv = delv0},
	{.br = 82, .refBr = 139, .cGma = gma2p15, .rTbl = rtbl82nit, .cTbl = ctbl82nit, .aid = aid4246, .elvAcl = delvAcl1, .elv = delv1},
	{.br = 87, .refBr = 148, .cGma = gma2p15, .rTbl = rtbl87nit, .cTbl = ctbl87nit, .aid = aid4246, .elvAcl = delvAcl1, .elv = delv1},
	{.br = 93, .refBr = 158, .cGma = gma2p15, .rTbl = rtbl93nit, .cTbl = ctbl93nit, .aid = aid4246, .elvAcl = delvAcl1, .elv = delv1},
	{.br = 98, .refBr = 166, .cGma = gma2p15, .rTbl = rtbl98nit, .cTbl = ctbl98nit, .aid = aid4246, .elvAcl = delvAcl2, .elv = delv2},
	{.br = 105, .refBr = 177, .cGma = gma2p15, .rTbl = rtbl105nit, .cTbl = ctbl105nit, .aid = aid4246, .elvAcl = delvAcl2, .elv = delv2},
	{.br = 111, .refBr = 186, .cGma = gma2p15, .rTbl = rtbl111nit, .cTbl = ctbl111nit, .aid = aid4246, .elvAcl = delvAcl2, .elv = delv2},
	{.br = 119, .refBr = 198, .cGma = gma2p15, .rTbl = rtbl119nit, .cTbl = ctbl119nit, .aid = aid4246, .elvAcl = delvAcl3, .elv = delv3},
	{.br = 126, .refBr = 208, .cGma = gma2p15, .rTbl = rtbl126nit, .cTbl = ctbl126nit, .aid = aid4246, .elvAcl = delvAcl3, .elv = delv3},
	{.br = 134, .refBr = 220, .cGma = gma2p15, .rTbl = rtbl134nit, .cTbl = ctbl134nit, .aid = aid4246, .elvAcl = delvAcl4, .elv = delv4},
	{.br = 143, .refBr = 234, .cGma = gma2p15, .rTbl = rtbl143nit, .cTbl = ctbl143nit, .aid = aid4246, .elvAcl = delvAcl4, .elv = delv4},
	{.br = 152, .refBr = 247, .cGma = gma2p15, .rTbl = rtbl152nit, .cTbl = ctbl152nit, .aid = aid4246, .elvAcl = delvAcl5, .elv = delv5},
	{.br = 162, .refBr = 249, .cGma = gma2p15, .rTbl = rtbl162nit, .cTbl = ctbl162nit, .aid = aid3807, .elvAcl = delvAcl5, .elv = delv5},
	{.br = 172, .refBr = 249, .cGma = gma2p15, .rTbl = rtbl172nit, .cTbl = ctbl172nit, .aid = aid3404, .elvAcl = delvAcl5, .elv = delv5},
	{.br = 183, .refBr = 249, .cGma = gma2p15, .rTbl = rtbl183nit, .cTbl = ctbl183nit, .aid = aid2841, .elvAcl = delvAcl5, .elv = delv5},
	{.br = 195, .refBr = 249, .cGma = gma2p15, .rTbl = rtbl195nit, .cTbl = ctbl195nit, .aid = aid2319, .elvAcl = delvAcl5, .elv = delv5},
	{.br = 207, .refBr = 249, .cGma = gma2p15, .rTbl = rtbl207nit, .cTbl = ctbl207nit, .aid = aid1772, .elvAcl = delvAcl6, .elv = delv6},
	{.br = 220, .refBr = 249, .cGma = gma2p15, .rTbl = rtbl220nit, .cTbl = ctbl220nit, .aid = aid1193, .elvAcl = delvAcl6, .elv = delv6},
	{.br = 234, .refBr = 249, .cGma = gma2p15, .rTbl = rtbl234nit, .cTbl = ctbl234nit, .aid = aid0584, .elvAcl = delvAcl6, .elv = delv6},
	{.br = 249, .refBr = 249, .cGma = gma2p15, .rTbl = rtbl249nit, .cTbl = ctbl249nit, .aid = aid0093, .elvAcl = delvAcl7, .elv = delv7},		// 249 ~ 360nit acl off
	{.br = 265, .refBr = 265, .cGma = gma2p15, .rTbl = rtbl265nit, .cTbl = ctbl265nit, .aid = aid0093, .elvAcl = delvAcl7, .elv = delv7},
	{.br = 282, .refBr = 282, .cGma = gma2p15, .rTbl = rtbl282nit, .cTbl = ctbl282nit, .aid = aid0093, .elvAcl = delvAcl8, .elv = delv8},
	{.br = 300, .refBr = 300, .cGma = gma2p15, .rTbl = rtbl300nit, .cTbl = ctbl300nit, .aid = aid0093, .elvAcl = delvAcl9, .elv = delv9},
	{.br = 316, .refBr = 316, .cGma = gma2p15, .rTbl = rtbl316nit, .cTbl = ctbl316nit, .aid = aid0093, .elvAcl = delvAcl9, .elv = delv9},
	{.br = 333, .refBr = 333, .cGma = gma2p15, .rTbl = rtbl333nit, .cTbl = ctbl333nit, .aid = aid0093, .elvAcl = delvAcl10, .elv = delv10},
	{.br = 360, .refBr = 360, .cGma = gma2p20, .rTbl = rtbl360nit, .cTbl = ctbl360nit, .aid = aid0093, .elvAcl = delvAcl11, .elv = delv11},
	{.br = 360, .refBr = 360, .cGma = gma2p20, .rTbl = rtbl360nit, .cTbl = ctbl360nit, .aid = aid0093, .elvAcl = delvAcl11, .elv = delvAcl11},
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

static int s6e3fa3_read_init_info(struct dsim_device *dsim, unsigned char* mtp)
{
	int i = 0;
	int ret;
	struct panel_private *panel = &dsim->priv;
	unsigned char bufForCoordi[S6E3FA3_COORDINATE_LEN] = {0,};
	unsigned char buf[S6E3FA3_MTP_DATE_SIZE] = {0, };

	dsim_info("MDD : %s was called\n", __func__);

	ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(SEQ_TEST_KEY_ON_F0));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_ON_F0\n", __func__);
	}

	// id
	ret = dsim_read_hl_data(dsim, S6E3FA3_ID_REG, S6E3FA3_ID_LEN, dsim->priv.id);
	if (ret != S6E3FA3_ID_LEN) {
		dsim_err("%s : can't find connected panel. check panel connection\n",__func__);
		panel->lcdConnected = PANEL_DISCONNEDTED;
		goto read_exit;
	}

	dsim_info("READ ID : ");
	for(i = 0; i < S6E3FA3_ID_LEN; i++)
		dsim_info("%02x, ", dsim->priv.id[i]);
	dsim_info("\n");

	// mtp
	ret = dsim_read_hl_data(dsim, S6E3FA3_MTP_ADDR, S6E3FA3_MTP_DATE_SIZE, buf);
	if (ret != S6E3FA3_MTP_DATE_SIZE) {
		dsim_err("failed to read mtp, check panel connection\n");
		goto read_fail;
	}
	memcpy(mtp, buf, S6E3FA3_MTP_SIZE);
	memcpy(dsim->priv.date, &buf[40], ARRAY_SIZE(dsim->priv.date));
	dsim_info("READ MTP SIZE : %d\n", S6E3FA3_MTP_SIZE);
	dsim_info("=========== MTP INFO =========== \n");
	for (i = 0; i < S6E3FA3_MTP_SIZE; i++)
		dsim_info("MTP[%2d] : %2d : %2x\n", i, mtp[i], mtp[i]);

	// coordinate
	ret = dsim_read_hl_data(dsim, S6E3FA3_COORDINATE_REG, S6E3FA3_COORDINATE_LEN, bufForCoordi);
	if (ret != S6E3FA3_COORDINATE_LEN) {
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
	ret = dsim_read_hl_data(dsim, S6E3FA3_CODE_REG, S6E3FA3_CODE_LEN, dsim->priv.code);
	if (ret != S6E3FA3_CODE_LEN) {
		dsim_err("fail to read code on command.\n");
		goto read_fail;
	}
	dsim_info("READ code : ");
	for(i = 0; i < S6E3FA3_CODE_LEN; i++)
		dsim_info("%x, ", dsim->priv.code[i]);
	dsim_info("\n");

#if 0
	// tset
	ret = dsim_read_hl_data(dsim, TSET_REG, TSET_LEN - 1, dsim->priv.tset);
	if (ret < TSET_LEN - 1) {
		dsim_err("fail to read code on command.\n");
		goto read_fail;
	}
	dsim_info("READ tset : ");
	for(i = 0; i < TSET_LEN - 1; i++)
		dsim_info("%x, ", dsim->priv.tset[i]);
	dsim_info("\n");

	// elvss
	ret = dsim_read_hl_data(dsim, ELVSS_REG, ELVSS_LEN - 1, dsim->priv.elvss_set);
	if (ret < ELVSS_LEN - 1) {
		dsim_err("fail to read elvss on command.\n");
		goto read_fail;
	}
	dsim_info("READ elvss : ");
	for(i = 0; i < ELVSS_LEN - 1; i++)
		dsim_info("%x \n", dsim->priv.elvss_set[i]);
#endif

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

static int s6e3fa3_probe(struct dsim_device *dsim)
{
	int ret = 0;
	struct panel_private *panel = &dsim->priv;
	unsigned char mtp[S6E3FA3_MTP_SIZE] = {0, };

	dsim_info("MDD : %s was called\n", __func__);

	panel->dim_data = (void *)NULL;
	panel->lcdConnected = PANEL_CONNECTED;
	panel->panel_type = 0;

	ret = s6e3fa3_read_init_info(dsim, mtp);
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


static int s6e3fa3_displayon(struct dsim_device *dsim)
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

static int s6e3fa3_exit(struct dsim_device *dsim)
{
	int ret = 0;
	dsim_info("MDD : %s was called\n", __func__);
	ret = dsim_write_hl_data(dsim, SEQ_DISPLAY_OFF, ARRAY_SIZE(SEQ_DISPLAY_OFF));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : DISPLAY_OFF\n", __func__);
		goto exit_err;
	}

	msleep(10);

	ret = dsim_write_hl_data(dsim, SEQ_SLEEP_IN, ARRAY_SIZE(SEQ_SLEEP_IN));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SLEEP_IN\n", __func__);
		goto exit_err;
	}

	msleep(150);

exit_err:
	return ret;
}

static int s6e3fa3_init(struct dsim_device *dsim)
{
	int ret = 0;

	dsim_info("MDD : %s was called\n", __func__);

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

	ret = dsim_write_hl_data(dsim, SEQ_ACL_OFF_OPR_AVR, ARRAY_SIZE(SEQ_ACL_OFF_OPR_AVR));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_ACL_OFF_OPR_AVR\n", __func__);
		goto init_exit;
	}
	ret = dsim_write_hl_data(dsim, SEQ_ACL_OFF, ARRAY_SIZE(SEQ_ACL_OFF));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_ACL_OFF\n", __func__);
		goto init_exit;
	}

#endif
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


struct dsim_panel_ops s6e3fa3_panel_ops = {
	.early_probe = NULL,
	.probe		= s6e3fa3_probe,
	.displayon	= s6e3fa3_displayon,
	.exit		= s6e3fa3_exit,
	.init		= s6e3fa3_init,
};

struct dsim_panel_ops *dsim_panel_get_priv_ops(struct dsim_device *dsim)
{
	return &s6e3fa3_panel_ops;
}
