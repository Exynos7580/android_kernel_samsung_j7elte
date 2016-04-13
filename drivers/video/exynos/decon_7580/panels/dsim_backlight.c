/* linux/drivers/video/exynos_decon/panel/dsim_backlight.c
 *
 * Header file for Samsung MIPI-DSI Backlight driver.
 *
 * Copyright (c) 2013 Samsung Electronics
 * Minwoo Kim <minwoo7945.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/


#include <linux/backlight.h>

#include "../dsim.h"
#include "../decon.h"
#include "dsim_backlight.h"
#include "panel_info.h"
#define BRIGHTNESS_REG 0x51

#ifdef CONFIG_PANEL_AID_DIMMING
#include "aid_dimming.h"
#endif

#ifdef CONFIG_PANEL_AID_DIMMING

static unsigned int get_actual_br_value(struct dsim_device *dsim, int index)
{
	struct panel_private *panel = &dsim->priv;
	struct SmtDimInfo *dimming_info = (struct SmtDimInfo *)panel->dim_info;

	if (dimming_info == NULL) {
		dsim_err("%s : dimming info is NULL\n", __func__);
		goto get_br_err;
	}

	if (index > MAX_BR_INFO)
		index = MAX_BR_INFO;

	return dimming_info[index].br;

get_br_err:
	return 0;
}
static unsigned char *get_gamma_from_index(struct dsim_device *dsim, int index)
{
	struct panel_private *panel = &dsim->priv;
	struct SmtDimInfo *dimming_info = (struct SmtDimInfo *)panel->dim_info;

	if (dimming_info == NULL) {
		dsim_err("%s : dimming info is NULL\n", __func__);
		goto get_gamma_err;
	}

	if (index > MAX_BR_INFO)
		index = MAX_BR_INFO;

	return (unsigned char *)dimming_info[index].gamma;

get_gamma_err:
	return NULL;
}

static unsigned char *get_aid_from_index(struct dsim_device *dsim, int index)
{
	struct panel_private *panel = &dsim->priv;
	struct SmtDimInfo *dimming_info = (struct SmtDimInfo *)panel->dim_info;

	if (dimming_info == NULL) {
		dsim_err("%s : dimming info is NULL\n", __func__);
		goto get_aid_err;
	}

	if (index > MAX_BR_INFO)
		index = MAX_BR_INFO;

	return (u8 *)dimming_info[index].aid;

get_aid_err:
	return NULL;
}

static unsigned char *get_elvss_from_index(struct dsim_device *dsim, int index, int acl)
{
	struct panel_private *panel = &dsim->priv;
	struct SmtDimInfo *dimming_info = (struct SmtDimInfo *)panel->dim_info;

	if (dimming_info == NULL) {
		dsim_err("%s : dimming info is NULL\n", __func__);
		goto get_elvess_err;
	}

	if(acl)
		return (unsigned char *)dimming_info[index].elvAcl;
	else
		return (unsigned char *)dimming_info[index].elv;

get_elvess_err:
	return NULL;
}


static void dsim_panel_gamma_ctrl(struct dsim_device *dsim)
{
	int level = LEVEL_IS_HBM(dsim->priv.auto_brightness), i = 0;
	u8 HBM_W[33] = {0xCA, };
	u8 *gamma = NULL;

	if (level) {
		memcpy(&HBM_W[1], dsim->priv.DB, 21);
		for ( i = 22 ; i <=30 ; i++ )
			HBM_W[i] = 0x80;
		HBM_W[31] = 0x00;
		HBM_W[32] = 0x00;

		if (dsim_write_hl_data(dsim, HBM_W, ARRAY_SIZE(HBM_W)) < 0)
			dsim_err("%s : failed to write gamma \n", __func__);

	} else {
		gamma = get_gamma_from_index(dsim, dsim->priv.br_index);
		if (gamma == NULL) {
			dsim_err("%s :faied to get gamma\n", __func__);
			return;
		}
		if (dsim_write_hl_data(dsim, gamma, GAMMA_CMD_CNT) < 0)
			dsim_err("%s : failed to write gamma \n", __func__);
	}

}

static void dsim_panel_aid_ctrl(struct dsim_device *dsim)
{
	u8 *aid = NULL;
	aid = get_aid_from_index(dsim, dsim->priv.br_index);
	if (aid == NULL) {
		dsim_err("%s : faield to get aid value\n", __func__);
		return;
	}
	if (dsim_write_hl_data(dsim, aid, AID_CMD_CNT) < 0)
		dsim_err("%s : failed to write gamma \n", __func__);
}

static void dsim_panel_set_elvss(struct dsim_device *dsim)
{
	u8 *elvss = NULL;
	u8 B2_W[8] = {0xB2, };
	u8 D4_W[19] = {0xD4, };
	int tset = 0;
	int real_br = get_actual_br_value(dsim, dsim->priv.br_index);
	int level = LEVEL_IS_HBM(dsim->priv.auto_brightness);

	tset = ((dsim->priv.temperature >= 0) ? 0 : BIT(7)) | abs(dsim->priv.temperature);

	elvss = get_elvss_from_index(dsim, dsim->priv.br_index, dsim->priv.acl_enable);
	if (elvss == NULL) {
		dsim_err("%s : failed to get elvss value\n", __func__);
		return;
	}

	if (level) {
		memcpy(&D4_W[1], SEQ_EA8061_ELVSS_SET_HBM_D4, ARRAY_SIZE(SEQ_EA8061_ELVSS_SET_HBM_D4));
		if (dsim->priv.temperature > 0)
			D4_W[3] = 0x48;
		else
			D4_W[3] = 0x4C;

		D4_W[18] = dsim->priv.DB[33];

		if (dsim_write_hl_data(dsim, SEQ_EA8061_ELVSS_SET_HBM, ARRAY_SIZE(SEQ_EA8061_ELVSS_SET_HBM)) < 0)
			dsim_err("%s : failed to SEQ_EA8061_ELVSS_SET_HBMelvss \n", __func__);

		if (dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_F1, ARRAY_SIZE(SEQ_TEST_KEY_ON_F1)) < 0)
			dsim_err("%s : failed to SEQ_TEST_KEY_ON_F1 \n", __func__);

		if (dsim_write_hl_data(dsim, D4_W, EA8061_MTP_D4_SIZE + 1) < 0)
			dsim_err("%s : failed to write elvss \n", __func__);

		if (dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_F1, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F1)) < 0)
			dsim_err("%s : failed to SEQ_TEST_KEY_OFF_F1 \n", __func__);

	} else {

		memcpy(&B2_W[1], dsim->priv.B2, EA8061_MTP_B2_SIZE);
		memcpy(&B2_W[1], SEQ_CAPS_ELVSS_DEFAULT, ARRAY_SIZE(SEQ_CAPS_ELVSS_DEFAULT));
		memcpy(&D4_W[1], dsim->priv.D4, EA8061_MTP_D4_SIZE);

		B2_W[1] = elvss[0];
		B2_W[7] = tset;

		if (dsim->priv.temperature > 0) {
			D4_W[3] = 0x48;
		} else {
			D4_W[3] = 0x4C;
		}

		if ( real_br <= 29 ) {
			if (dsim->priv.temperature > 0) {
				D4_W[18] = elvss[1];
			} else if (dsim->priv.temperature > -20 && dsim->priv.temperature < 0 ) {
				D4_W[18] = elvss[2];
			} else {
				D4_W[18] = elvss[3];
			}
		}
		if (dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_F1, ARRAY_SIZE(SEQ_TEST_KEY_ON_F1)) < 0)
		dsim_err("%s : failed to SEQ_TEST_KEY_ON_F1 \n", __func__);

		if (dsim_write_hl_data(dsim, B2_W, EA8061_MTP_B2_SIZE + 1) < 0)
			dsim_err("%s : failed to write elvss \n", __func__);

		if (dsim_write_hl_data(dsim, D4_W, EA8061_MTP_D4_SIZE + 1) < 0)
			dsim_err("%s : failed to write elvss \n", __func__);

		if (dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_F1, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F1)) < 0)
			dsim_err("%s : failed to SEQ_TEST_KEY_OFF_F1 \n", __func__);
	}

	dsim_info("%s: %d Tset: %x Temp: %d \n", __func__, level, D4_W[3], dsim->priv.temperature);
}


static int dsim_panel_set_acl(struct dsim_device *dsim, int force)
{
	int ret = 0, level = ACL_STATUS_8P;
	struct panel_private *panel = &dsim->priv;

	if (panel == NULL) {
			dsim_err("%s : panel is NULL\n", __func__);
			goto exit;
	}

	if (dsim->priv.siop_enable || LEVEL_IS_HBM(dsim->priv.auto_brightness))  // auto acl or hbm is acl on
		goto acl_update;

	if (!dsim->priv.acl_enable)
		level = ACL_STATUS_0P;

acl_update:
	if(force || dsim->priv.current_acl != panel->acl_cutoff_tbl[level][1]) {

		if((ret = dsim_write_hl_data(dsim, panel->acl_cutoff_tbl[level], 2)) < 0) {
			dsim_err("fail to write acl command.\n");
			goto exit;
		}

		dsim->priv.current_acl = panel->acl_cutoff_tbl[level][1];
		dsim_info("acl: %d, auto_brightness: %d\n", dsim->priv.current_acl, dsim->priv.auto_brightness);
	}
exit:
	if (!ret)
		ret = -EPERM;
	return ret;
}

#if 0
static int dsim_panel_set_tset(struct dsim_device *dsim, int force)
{
	int ret = 0;
	int tset = 0;
	unsigned char SEQ_TSET[TSET_LEN] = {TSET_REG, };

	tset = ((dsim->priv.temperature >= 0) ? 0 : BIT(7)) | abs(dsim->priv.temperature);

	if(force || dsim->priv.tset[0] != tset) {
		dsim->priv.tset[0] = SEQ_TSET[1] = tset;
		dsim->priv.tset[1] = SEQ_TSET[2] = 0x0A;

		if (dsim->priv.panel_type == LCD_EA8064G) {
			if ((ret = dsim_write_hl_data(dsim, SEQ_TSET, ARRAY_SIZE(SEQ_TSET))) < 0) {
				dsim_err("fail to write tset command.\n");
				ret = -EPERM;
			}
		} else {
			if ((ret = dsim_write_hl_data(dsim, SEQ_TSET, ARRAY_SIZE(SEQ_TSET) - 1 )) < 0) {
				dsim_err("fail to write tset command.\n");
				ret = -EPERM;
			}
		}
		dsim_info("temperature: %d, tset: %d\n", dsim->priv.temperature, SEQ_TSET[1]);
	}
	return ret;
}
#endif

#ifdef CONFIG_PANEL_S6E3HA2_DYNAMIC
static int dsim_panel_set_vint(struct dsim_device *dsim, int force)
{
	int ret = 0;
	int nit = 0;
	int i, level;
	int arraySize = ARRAY_SIZE(VINT_DIM_TABLE);
	unsigned char SEQ_VINT[VINT_LEN] = {VINT_REG, 0x8B, 0x00};

	nit = dsim->priv.br_tbl[dsim->priv.bd->props.brightness];

	level = arraySize - 1;

	for (i = 0; i < arraySize; i++) {
		if (nit <= VINT_DIM_TABLE[i]) {
			level = i;
			goto set_vint;
		}
	}
set_vint:
	if(force || dsim->priv.current_vint != VINT_TABLE[level]) {
		SEQ_VINT[VINT_LEN - 1] = VINT_TABLE[level];
		if ((ret = dsim_write_hl_data(dsim, SEQ_VINT, ARRAY_SIZE(SEQ_VINT))) < 0) {
			dsim_err("fail to write vint command.\n");
			ret = -EPERM;
		}
		dsim->priv.current_vint = VINT_TABLE[level];
		dsim_info("vint: %02x\n", dsim->priv.current_vint);
	}
	return ret;
}
#endif

#if 0
static int dsim_panel_set_hbm(struct dsim_device *dsim, int force)
{
	int ret = 0, level = LEVEL_IS_HBM(dsim->priv.auto_brightness);
	struct panel_private *panel = &dsim->priv;

	if (panel == NULL) {
		dsim_err("%s : panel is NULL\n", __func__);
		goto exit;
	}

	if(force || dsim->priv.current_hbm != panel->hbm_tbl[level][1]) {
		dsim->priv.current_hbm = panel->hbm_tbl[level][1];
		if((ret = dsim_write_hl_data(dsim, panel->hbm_tbl[level], ARRAY_SIZE(SEQ_HBM_OFF))) < 0) {
			dsim_err("fail to write hbm command.\n");
			ret = -EPERM;
		}
		dsim_info("hbm: %d, auto_brightness: %d\n", dsim->priv.current_hbm, dsim->priv.auto_brightness);
	}
exit:
	return ret;
}
#endif

static int low_level_set_brightness(struct dsim_device *dsim ,int force)
{
	int ret;

	ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(SEQ_TEST_KEY_ON_F0));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_TEST_KEY_ON_F0\n", __func__);
		goto init_exit;
	}
	ret = dsim_write_hl_data(dsim, SEQ_EA8061_LTPS_STOP, ARRAY_SIZE(SEQ_EA8061_LTPS_STOP));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_EA8061_LTPS_STOP\n", __func__);
		goto init_exit;
	}
	dsim_panel_gamma_ctrl(dsim);

	dsim_panel_aid_ctrl(dsim);

	ret = dsim_write_hl_data(dsim, SEQ_EA8061_LTPS_UPDATE, ARRAY_SIZE(SEQ_EA8061_LTPS_UPDATE));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_EA8061_LTPS_UPDATE\n", __func__);
		goto init_exit;
	}

	dsim_panel_set_elvss(dsim);

#ifdef CONFIG_PANEL_S6E3HA2_DYNAMIC
	dsim_panel_set_vint(dsim, force);
#endif

	dsim_panel_set_acl(dsim, force);

//	dsim_panel_set_tset(dsim, force);

//	dsim_panel_set_hbm(dsim, force);

	if (dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_F0, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0)) < 0)
		dsim_err("%s : fail to write F0 on command\n", __func__);

init_exit:
	return 0;
}

static int get_acutal_br_index(struct dsim_device *dsim, int br)
{
	int i;
	int min;
	int gap;
	int index = 0;
	struct panel_private *panel = &dsim->priv;
	struct SmtDimInfo *dimming_info = panel->dim_info;

	if (dimming_info == NULL) {
		dsim_err("%s : dimming_info is NULL\n", __func__);
		return 0;
	}

	min = MAX_BRIGHTNESS;

	for (i = 0; i < MAX_BR_INFO; i++) {
		if (br > dimming_info[i].br)
			gap = br - dimming_info[i].br;
		else
			gap = dimming_info[i].br - br;

		if (gap == 0) {
			index = i;
			break;
		}

		if (gap < min) {
			min = gap;
			index = i;
		}
	}
	return index;
}

#endif

#ifndef CONFIG_PANEL_AID_DIMMING
#if 0
static int dsim_panel_set_cabc(struct dsim_device *dsim, int force) {
	int ret = 0;
	struct panel_private *panel = &dsim->priv;
	struct decon_device *decon = decon_int_drvdata;

	if (panel == NULL) {
		dsim_err("%s : panel is NULL\n", __func__);
		goto exit;
	}

	if (panel->auto_brightness > 0 && panel->auto_brightness < 6)
		panel->acl_enable = 1;
	else
		panel->acl_enable = 0;

	if ((panel->acl_enable == panel->current_acl) && !force)
		return 0;

	if (panel->acl_enable) { //cabc on
		if (dsim_write_hl_data(dsim, SEQ_CABC_ON, ARRAY_SIZE(SEQ_CABC_ON)) < 0)
			dsim_err("%s : failed to write SEQ_CD_ON \n", __func__);

		if (decon->state == DECON_STATE_ON) //during wake up time, avoid needless delay
			msleep(500); // for CABC transforming dimming , H/W & DDI company guide

		if (dsim_write_hl_data(dsim, SEQ_CD_ON, ARRAY_SIZE(SEQ_CD_ON)) < 0)
			dsim_err("%s : failed to write SEQ_CD_ON \n", __func__);

	} else { //cabc off
		if (dsim_write_hl_data(dsim, SEQ_CD_OFF, ARRAY_SIZE(SEQ_CD_OFF)) < 0)
			dsim_err("%s : failed to write SEQ_CD_ON \n", __func__);

		if (dsim_write_hl_data(dsim, SEQ_CABC_OFF, ARRAY_SIZE(SEQ_CABC_OFF)) < 0)
			dsim_err("%s : failed to write SEQ_CD_ON \n", __func__);
	}

	panel->current_acl = panel->acl_enable;
	dsim_info("cabc: %d, auto_brightness: %d\n", dsim->priv.current_acl, dsim->priv.auto_brightness);

exit:
	if (!ret)
		ret = -EPERM;
	return ret;
}

static int dsim_panel_set_outdoor_mode(struct dsim_device *dsim, int force) {
	int ret = 0;
	struct panel_private *panel = &dsim->priv;

	if (panel == NULL) {
			dsim_err("%s : panel is NULL\n", __func__);
			goto exit;
	}
	if (((panel->current_hbm && panel->auto_brightness == 6)
		|| (!panel->current_hbm && panel->auto_brightness < 6)) && !force)
		return ret;

	if (panel->current_hbm && panel->auto_brightness < 6) {
		sky82896_array_write(SKY82896_eprom_drv_outdoor_off_arr, ARRAY_SIZE(SKY82896_eprom_drv_outdoor_off_arr)); //bl cabc off
	} else if (!panel->current_hbm && panel->auto_brightness == 6) {
		sky82896_array_write(SKY82896_eprom_drv_outdoor_on_arr, ARRAY_SIZE(SKY82896_eprom_drv_outdoor_on_arr)); //bl cabc on
	}

	if (panel->auto_brightness == 6)
		panel->current_hbm = 1;
	else
		panel->current_hbm = 0;

	dsim_info("BL_IC outdoor_mode: %d, auto_brightness: %d\n", dsim->priv.current_hbm, dsim->priv.auto_brightness);

exit:
	if (!ret)
		ret = -EPERM;
	return ret;
}
#endif
#endif

static int get_backlight_level(int brightness)
{
	int backlightlevel;

	switch (brightness) {
	case 0:
		backlightlevel = 0;
		break;
	case 1:
		backlightlevel = 2;
		break;
	case 2 ... 11:
		backlightlevel = 2;
		break;
	case 12 ... 20:
		backlightlevel = brightness - 10;
		break;
	case 21 ... 142:
		backlightlevel = (brightness-21) * 88 / (142 - 20)  + 10 ;
		break;
	case 143:
		backlightlevel = 98;
		break;
	case 144 ... 255:
		backlightlevel = (brightness-144) * 124 / (255 - 143) + 98 ;
		break;
	default:
		backlightlevel = 12;
		break;
	}

	return backlightlevel;
}

int dsim_panel_set_brightness(struct dsim_device *dsim, int force)
{
	struct panel_private *panel = &dsim->priv;
	int ret = 0;
	unsigned char bl_reg[2];

#ifdef CONFIG_PANEL_AID_DIMMING
	int p_br = panel->bd->props.brightness;
	int acutal_br = 0;
	int real_br = 0;
	int prev_index = panel->br_index;
	struct dim_data *dimming;
#endif

	if ( dsim->rev < 0x04 ) {
		if (panel->state != PANEL_STATE_RESUMED) {
			dsim_info("%s : panel is not active state..\n", __func__);
			goto set_br_exit;
		}

		ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(SEQ_TEST_KEY_ON_F0));
		if (ret < 0) {
			dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_ON_F0\n", __func__);
		}

		ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_F1, ARRAY_SIZE(SEQ_TEST_KEY_ON_F1));
		if (ret < 0) {
			dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_ON_F1\n", __func__);
		}

		ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_FC, ARRAY_SIZE(SEQ_TEST_KEY_ON_FC));
		if (ret < 0) {
			dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_ON_FC\n", __func__);
		}

		bl_reg[0] = BRIGHTNESS_REG;
		bl_reg[1] = get_backlight_level(panel->bd->props.brightness);
		if (LEVEL_IS_HBM(panel->auto_brightness) && (panel->bd->props.brightness == 255))
			bl_reg[1] = 0xFF;

		if (panel->state != PANEL_STATE_RESUMED) {
			dsim_info("%s : panel is not active state..\n", __func__);
			return ret;
		}

		dsim_info("%s: platform BL : %d panel BL reg : %d \n", __func__, panel->bd->props.brightness, bl_reg[1]);

		if (dsim_write_hl_data(dsim, bl_reg, ARRAY_SIZE(bl_reg)) < 0)
			dsim_err("%s : fail to write brightness cmd.\n", __func__);

#if 0
		dsim_panel_set_cabc(dsim, force);

		dsim_panel_set_outdoor_mode(dsim, force);
#endif
		ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_FC, ARRAY_SIZE(SEQ_TEST_KEY_OFF_FC));
		if (ret < 0) {
			dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_OFF_FC\n", __func__);
		}

		ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_F1, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F1));
		if (ret < 0) {
			dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_OFF_F1\n", __func__);
		}

		ret = dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_F0, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0));
		if (ret < 0) {
			dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_OFF_F0\n", __func__);
		}
	} else {
		/* for ea8061 */

#ifdef CONFIG_PANEL_AID_DIMMING
		dimming = (struct dim_data *)panel->dim_data;
		if ((dimming == NULL) || (panel->br_tbl == NULL)) {
			dsim_info("%s : this panel does not support dimming\n", __func__);
			return ret;
		}

		mutex_lock(&panel->lock);

		acutal_br = panel->br_tbl[p_br];
		panel->br_index = get_acutal_br_index(dsim, acutal_br);
		real_br = get_actual_br_value(dsim, panel->br_index);
		panel->acl_enable = ACL_IS_ON(real_br);

		if (LEVEL_IS_HBM(panel->auto_brightness)) {
			panel->br_index = HBM_INDEX;
			panel->acl_enable = 1;				// hbm is acl on
		}
		if(panel->siop_enable)					// check auto acl
			panel->acl_enable = 1;

		if (panel->state != PANEL_STATE_RESUMED) {
			dsim_info("%s : panel is not active state..\n", __func__);
			goto set_br_exit;
		}

		dsim_info("%s : platform : %d, : mapping : %d, real : %d, index : %d\n",
			__func__, p_br, acutal_br, real_br, panel->br_index);

		if (!force && panel->br_index == prev_index)
			goto set_br_exit;

		if ((acutal_br == 0) || (real_br == 0))
			goto set_br_exit;

		ret = low_level_set_brightness(dsim, force);

		if (ret) {
			dsim_err("%s failed to set brightness : %d\n", __func__, acutal_br);
		}
	}
set_br_exit:
	mutex_unlock(&panel->lock);
	return ret;
}

#else
	}
set_br_exit:
	return ret;
}
#endif

static int panel_get_brightness(struct backlight_device *bd)
{
	return bd->props.brightness;
}


static int panel_set_brightness(struct backlight_device *bd)
{
	int ret = 0;
	int brightness = bd->props.brightness;
	struct panel_private *priv = bl_get_data(bd);
	struct dsim_device *dsim;

	dsim = container_of(priv, struct dsim_device, priv);

	if (brightness < UI_MIN_BRIGHTNESS || brightness > UI_MAX_BRIGHTNESS) {
		printk(KERN_ALERT "Brightness should be in the range of 0 ~ 255\n");
		ret = -EINVAL;
		goto exit_set;
	}

	ret = dsim_panel_set_brightness(dsim, 0);
	if (ret) {
		dsim_err("%s : fail to set brightness\n", __func__);
		goto exit_set;
	}
exit_set:
	return ret;

}

static const struct backlight_ops panel_backlight_ops = {
	.get_brightness = panel_get_brightness,
	.update_status = panel_set_brightness,
};


int dsim_backlight_probe(struct dsim_device *dsim)
{
	int ret = 0;
	struct panel_private *panel = &dsim->priv;

	panel->bd = backlight_device_register("panel", dsim->dev, &dsim->priv,
					&panel_backlight_ops, NULL);
	if (IS_ERR(panel->bd)) {
		dsim_err("%s:failed register backlight\n", __func__);
		ret = PTR_ERR(panel->bd);
	}

	panel->bd->props.max_brightness = UI_MAX_BRIGHTNESS;
	panel->bd->props.brightness = UI_DEFAULT_BRIGHTNESS;

#ifdef CONFIG_LCD_HMT
	panel->hmt_on = HMT_OFF;
	panel->hmt_brightness = DEFAULT_HMT_BRIGHTNESS;
	panel->hmt_br_index = 0;
#endif
	return ret;
}
