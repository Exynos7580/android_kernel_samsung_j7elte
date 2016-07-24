/*
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/list.h>
#include <linux/dma-mapping.h>
#include <linux/pm_runtime.h>
#include <linux/pm_qos.h>
#include <linux/suspend.h>
#include <linux/debugfs.h>
#include <linux/clk-provider.h>

#include <mach/bts.h>
#include "cal_bts_7580.h"
#include "regs-bts.h"

#define MAX_MIF_IDX		8

#define FHD_BW			(1920 * 1080 * 4 * 60)

#ifdef BTS_DBGGEN
#define BTS_DBG(x...)		pr_err(x)
#else
#define BTS_DBG(x...)		do {} while (0)
#endif

#define BTS_SYSREG_DOMAIN	(BTS_DISPLAY | \
					BTS_DISPLAY_M0 | \
					BTS_DISPLAY_S0 | \
					BTS_DISPLAY_S1 | \
					BTS_DISPLAY_S2 | \
					BTS_ISP0 | \
					BTS_ISP1 | \
					BTS_ISP_M0 | \
					BTS_ISP_S0 | \
					BTS_ISP_S1 | \
					BTS_ISP_S2 | \
					BTS_MFC | \
					BTS_JPEG | \
					BTS_MSCL0 | \
					BTS_MSCL1 | \
					BTS_MFCMSCL_M0 | \
					BTS_JPG_S0 | \
					BTS_MSCL0_S1 | \
					BTS_MSCL1_S2 | \
					BTS_AUD | \
					BTS_FSYS0 | \
					BTS_FSYS1 | \
					BTS_IMEM | \
					BTS_MODAPIF | \
					BTS_CPU | \
					BTS_APL)

enum bts_index {
	BTS_IDX_DISPLAY,
	BTS_IDX_DISPLAY_M0,
	BTS_IDX_DISPLAY_S0,
	BTS_IDX_DISPLAY_S1,
	BTS_IDX_DISPLAY_S2,
	BTS_IDX_ISP0,
	BTS_IDX_ISP1,
	BTS_IDX_ISP_M0,
	BTS_IDX_ISP_S0,
	BTS_IDX_ISP_S1,
	BTS_IDX_ISP_S2,
	BTS_IDX_MFC,
	BTS_IDX_JPEG,
	BTS_IDX_MSCL0,
	BTS_IDX_MSCL1,
	BTS_IDX_MFCMSCL_M0,
	BTS_IDX_JPG_S0,
	BTS_IDX_MSCL0_S1,
	BTS_IDX_MSCL1_S2,
	BTS_IDX_AUD,
	BTS_IDX_FSYS0,
	BTS_IDX_FSYS1,
	BTS_IDX_IMEM,
	BTS_IDX_MODAPIF,
	BTS_IDX_CPU,
	BTS_IDX_APL,
	BTS_IDX_G3D,
	BTS_MAX,
};

enum bts_id {
	BTS_DISPLAY = (1 << BTS_IDX_DISPLAY),
	BTS_DISPLAY_M0 = (1 << BTS_IDX_DISPLAY_M0),
	BTS_DISPLAY_S0 = (1 << BTS_IDX_DISPLAY_S0),
	BTS_DISPLAY_S1 = (1 << BTS_IDX_DISPLAY_S1),
	BTS_DISPLAY_S2 = (1 << BTS_IDX_DISPLAY_S2),
	BTS_ISP0 = (1 << BTS_IDX_ISP0),
	BTS_ISP1 = (1 << BTS_IDX_ISP1),
	BTS_ISP_M0 = (1 << BTS_IDX_ISP_M0),
	BTS_ISP_S0 = (1 << BTS_IDX_ISP_S0),
	BTS_ISP_S1 = (1 << BTS_IDX_ISP_S1),
	BTS_ISP_S2 = (1 << BTS_IDX_ISP_S2),
	BTS_MFC = (1 << BTS_IDX_MFC),
	BTS_JPEG = (1 << BTS_IDX_JPEG),
	BTS_MSCL0 = (1 << BTS_IDX_MSCL0),
	BTS_MSCL1 = (1 << BTS_IDX_MSCL1),
	BTS_MFCMSCL_M0 = (1 << BTS_IDX_MFCMSCL_M0),
	BTS_JPG_S0 = (1 << BTS_IDX_JPG_S0),
	BTS_MSCL0_S1 = (1 << BTS_IDX_MSCL0_S1),
	BTS_MSCL1_S2 = (1 << BTS_IDX_MSCL1_S2),
	BTS_AUD = (1 << BTS_IDX_AUD),
	BTS_FSYS0 = (1 << BTS_IDX_FSYS0),
	BTS_FSYS1 = (1 << BTS_IDX_FSYS1),
	BTS_IMEM = (1 << BTS_IDX_IMEM),
	BTS_MODAPIF = (1 << BTS_IDX_MODAPIF),
	BTS_CPU = (1 << BTS_IDX_CPU),
	BTS_APL = (1 << BTS_IDX_APL),
	BTS_G3D = (1 << BTS_IDX_G3D),
};

enum exynos_bts_scenario {
	BS_DISABLE,
	BS_DEFAULT,
	BS_CAM_BNS,
	BS_DEBUG,
	BS_MAX,
};

enum exynos_bts_function {
	BF_SETQOS,
	BF_SETQOS_BW,
	BF_SETQOS_MO,
	BF_DISABLE,
	BF_NOP,
};

struct bts_table {
	enum exynos_bts_function fn;
	unsigned int priority[2];
	unsigned int window;
	unsigned int token;
	unsigned int mo;
	struct bts_info *next_bts;
	int prev_scen;
	int next_scen;
};

struct bts_info {
	enum bts_id id;
	const char *name;
	unsigned int pa_base;
	void __iomem *va_base;
	struct bts_table table[BS_MAX];
	const char *pd_name;
	bool on;
	struct list_head list;
	bool enable;
	struct clk_info *ct_ptr;
	enum exynos_bts_scenario cur_scen;
	enum exynos_bts_scenario top_scen;
};

struct bts_scenario {
	const char *name;
	unsigned int ip;
	enum exynos_bts_scenario id;
	struct bts_info *head;
};

struct clk_info {
	const char *clk_name;
	struct clk *clk;
	enum bts_index index;
};

static struct pm_qos_request exynos7_mif_bts_qos;
static struct pm_qos_request exynos7_int_bts_qos;
static struct srcu_notifier_head exynos_media_notifier;

static DEFINE_MUTEX(media_mutex);
static DEFINE_SPINLOCK(timeout_mutex);
static unsigned int decon_bw, cam_bw, total_bw;
static void __iomem *base_drex;
static void __iomem *base_bts_sysreg_mif_modapif;
static int cur_target_idx;
static unsigned int mif_freq;
static unsigned int num_active_disp_win;

static int exynos7_bts_param_table[MAX_MIF_IDX][8] = {
	/* DISP(0)     DISP(1)     DISP(2)     DISP(3)   DISP(0)+CAM DISP(1)+CAM DISP(2)+CAM DISP(3)+CAM */
	{0x02000200, 0x02000200, 0x02000200, 0x02000200, 0x01000100, 0x01000100, 0x01000100, 0x01000100},/*MIF_LV0 */
	{0x02000200, 0x02000200, 0x02000200, 0x02000200, 0x01000100, 0x01000100, 0x01000100, 0x01000100},/*MIF_LV1 */
	{0x01000100, 0x01000100, 0x01000100, 0x01000100, 0x01000100, 0x01000100, 0x01000100, 0x01000100},/*MIF_LV2 */
	{0x01000100, 0x01000100, 0x01000100, 0x01000100, 0x01000100, 0x01000100, 0x01000100, 0x01000100},/*MIF_LV3 */
	{0x01000100, 0x01000100, 0x01000100, 0x00000000, 0x01000100, 0x01000100, 0x01000100, 0x00000000},/*MIF_LV4 */
	{0x01000100, 0x01000100, 0x01000100, 0x00000000, 0x01000100, 0x00800080, 0x00000000, 0x00000000},/*MIF_LV5 */
	{0x01000100, 0x01000100, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},/*MIF_LV6 */
	{0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},/*MIF_LV7 */
};

static struct clk_info clk_table[] = {
	{"aclk_qe_g3d", NULL, BTS_IDX_G3D},
	{"pclk_qe_g3d", NULL, BTS_IDX_G3D},
};

static int exynos7_timeout_table[MAX_MIF_IDX] = {
	0x01000100, /* MIF_LV0 */
	0x01000100, /* MIF_LV1 */
	0x01000100, /* MIF_LV2 */
	0x01000100, /* MIF_LV3 */
	0x01000100, /* MIF_LV4 */
	0x01000100, /* MIF_LV5 */
	0x00200020, /* MIF_LV6 */
	0x00200020, /* MIF_LV7 */
};

static struct bts_info exynos7_bts[] = {
	[BTS_IDX_DISPLAY] = {
		.id = BTS_DISPLAY,
		.name = "disp",
		.pa_base = EXYNOS7580_PA_SYSREG_DISP,
		.pd_name = "pd-disp",
		.table[BS_DEFAULT].fn = BF_SETQOS,
		.table[BS_DEFAULT].priority[0] = 0xA,
		.table[BS_DISABLE].fn = BF_SETQOS,
		.table[BS_DISABLE].priority[0] = 0x4,
		.cur_scen = BS_DISABLE,
		.on = false,
		.enable = true,
	},
	[BTS_IDX_DISPLAY_M0] = {
		.id = BTS_DISPLAY_M0,
		.name = "disp_m0",
		.pa_base = EXYNOS7580_PA_SYSREG_DISP,
		.pd_name = "pd-disp",
		.table[BS_DEFAULT].fn = BF_SETQOS_MO,
		.table[BS_DEFAULT].mo = 0x1,
		.table[BS_DISABLE].fn = BF_SETQOS_MO,
		.table[BS_DISABLE].mo = 0x20,
		.cur_scen = BS_DISABLE,
		.on = false,
		.enable = true,
	},
	[BTS_IDX_DISPLAY_S0] = {
		.id = BTS_DISPLAY_S0,
		.name = "disp_s0",
		.pa_base = EXYNOS7580_PA_SYSREG_DISP,
		.pd_name = "pd-disp",
		.table[BS_DEFAULT].fn = BF_SETQOS_MO,
		.table[BS_DEFAULT].mo = 0x5,
		.table[BS_DISABLE].fn = BF_SETQOS_MO,
		.table[BS_DISABLE].mo = 0x10,
		.cur_scen = BS_DISABLE,
		.on = false,
		.enable = true,
	},
	[BTS_IDX_DISPLAY_S1] = {
		.id = BTS_DISPLAY_S1,
		.name = "disp_s1",
		.pa_base = EXYNOS7580_PA_SYSREG_DISP,
		.pd_name = "pd-disp",
		.table[BS_DEFAULT].fn = BF_SETQOS_MO,
		.table[BS_DEFAULT].mo = 0x5,
		.table[BS_DISABLE].fn = BF_SETQOS_MO,
		.table[BS_DISABLE].mo = 0x10,
		.cur_scen = BS_DISABLE,
		.on = false,
		.enable = true,
	},
	[BTS_IDX_DISPLAY_S2] = {
		.id = BTS_DISPLAY_S2,
		.name = "disp_s2",
		.pa_base = EXYNOS7580_PA_SYSREG_DISP,
		.pd_name = "pd-disp",
		.table[BS_DEFAULT].fn = BF_SETQOS_MO,
		.table[BS_DEFAULT].mo = 0x5,
		.table[BS_DISABLE].fn = BF_SETQOS_MO,
		.table[BS_DISABLE].mo = 0x10,
		.cur_scen = BS_DISABLE,
		.on = false,
		.enable = true,
	},
	[BTS_IDX_ISP0] = {
		.id = BTS_ISP0,
		.name = "isp0",
		.pa_base = EXYNOS7580_PA_SYSREG_ISP,
		.pd_name = "pd-isp",
		.table[BS_DEFAULT].fn = BF_SETQOS,
		.table[BS_DEFAULT].priority[0] = 0xC,
		.table[BS_DEFAULT].priority[1] = 0xB,
		.table[BS_CAM_BNS].fn = BF_SETQOS,
		.table[BS_CAM_BNS].priority[0] = 0x8,
		.table[BS_DISABLE].fn = BF_SETQOS,
		.table[BS_DISABLE].priority[0] = 0x4,
		.cur_scen = BS_DISABLE,
		.on = false,
		.enable = true,
	},
	[BTS_IDX_ISP1] = {
		.id = BTS_ISP1,
		.name = "isp1",
		.pa_base = EXYNOS7580_PA_SYSREG_ISP,
		.pd_name = "pd-isp",
		.table[BS_DEFAULT].fn = BF_SETQOS,
		.table[BS_DEFAULT].priority[0] = 0xC,
		.table[BS_DISABLE].fn = BF_SETQOS,
		.table[BS_DISABLE].priority[0] = 0x4,
		.cur_scen = BS_DISABLE,
		.on = false,
		.enable = true,
	},
	[BTS_IDX_ISP_M0] = {
		.id = BTS_ISP_M0,
		.name = "isp1_m0",
		.pa_base = EXYNOS7580_PA_SYSREG_ISP,
		.pd_name = "pd-isp",
		.table[BS_DEFAULT].fn = BF_NOP,
		.table[BS_DISABLE].fn = BF_SETQOS_MO,
		.table[BS_DISABLE].mo = 0x20,
		.cur_scen = BS_DISABLE,
		.on = false,
		.enable = true,
	},
	[BTS_IDX_ISP_S0] = {
		.id = BTS_ISP_S0,
		.name = "isp1_s0",
		.pa_base = EXYNOS7580_PA_SYSREG_ISP,
		.pd_name = "pd-isp",
		.table[BS_DEFAULT].fn = BF_NOP,
		.table[BS_DISABLE].fn = BF_SETQOS_MO,
		.table[BS_DISABLE].mo = 0x20,
		.cur_scen = BS_DISABLE,
		.on = false,
		.enable = true,
	},
	[BTS_IDX_ISP_S1] = {
		.id = BTS_ISP_S1,
		.name = "isp1_s1",
		.pa_base = EXYNOS7580_PA_SYSREG_ISP,
		.pd_name = "pd-isp",
		.table[BS_DEFAULT].fn = BF_NOP,
		.table[BS_DISABLE].fn = BF_SETQOS_MO,
		.table[BS_DISABLE].mo = 0x20,
		.cur_scen = BS_DISABLE,
		.on = false,
		.enable = true,
	},
	[BTS_IDX_ISP_S2] = {
		.id = BTS_ISP_S2,
		.name = "isp1_s2",
		.pa_base = EXYNOS7580_PA_SYSREG_ISP,
		.pd_name = "pd-isp",
		.table[BS_DEFAULT].fn = BF_NOP,
		.table[BS_DISABLE].fn = BF_SETQOS_MO,
		.table[BS_DISABLE].mo = 0x20,
		.cur_scen = BS_DISABLE,
		.on = false,
		.enable = true,
	},
	[BTS_IDX_MFC] = {
		.id = BTS_MFC,
		.name = "mfc",
		.pa_base = EXYNOS7580_PA_SYSREG_MFCMSCL,
		.pd_name = "spd-mfc",
		.table[BS_DEFAULT].fn = BF_SETQOS,
		.table[BS_DEFAULT].priority[0] = 0x8,
		.table[BS_DISABLE].fn = BF_SETQOS,
		.table[BS_DISABLE].priority[0] = 0x4,
		.cur_scen = BS_DISABLE,
		.on = false,
		.enable = true,
	},
	[BTS_IDX_JPEG] = {
		.id = BTS_JPEG,
		.name = "jpeg",
		.pa_base = EXYNOS7580_PA_SYSREG_MFCMSCL,
		.pd_name = "spd-jpeg",
		.table[BS_DEFAULT].fn = BF_NOP,
		.table[BS_DISABLE].fn = BF_SETQOS,
		.table[BS_DISABLE].priority[0] = 0x4,
		.cur_scen = BS_DISABLE,
		.on = false,
		.enable = true,
	},
	[BTS_IDX_MSCL0] = {
		.id = BTS_MSCL0,
		.name = "mscl0",
		.pa_base = EXYNOS7580_PA_SYSREG_MFCMSCL,
		.pd_name = "spd-mscl0",
		.table[BS_DEFAULT].fn = BF_NOP,
		.table[BS_DISABLE].fn = BF_SETQOS,
		.table[BS_DISABLE].priority[0] = 0x4,
		.cur_scen = BS_DISABLE,
		.on = false,
		.enable = true,
	},
	[BTS_IDX_MSCL1] = {
		.id = BTS_MSCL1,
		.name = "mscl1",
		.pa_base = EXYNOS7580_PA_SYSREG_MFCMSCL,
		.pd_name = "spd-mscl1",
		.table[BS_DEFAULT].fn = BF_NOP,
		.table[BS_DISABLE].fn = BF_SETQOS,
		.table[BS_DISABLE].priority[0] = 0x4,
		.cur_scen = BS_DISABLE,
		.on = false,
		.enable = true,
	},
	[BTS_IDX_MFCMSCL_M0] = {
		.id = BTS_MFCMSCL_M0,
		.name = "mfcmscl_m0",
		.pa_base = EXYNOS7580_PA_SYSREG_MFCMSCL,
		.pd_name = "spd-mfc",
		.table[BS_DEFAULT].fn = BF_NOP,
		.table[BS_DISABLE].fn = BF_SETQOS_MO,
		.table[BS_DISABLE].mo = 0x20,
		.cur_scen = BS_DISABLE,
		.on = false,
		.enable = true,
	},
	[BTS_IDX_JPG_S0] = {
		.id = BTS_JPG_S0,
		.name = "jpeg_s0",
		.pa_base = EXYNOS7580_PA_SYSREG_MFCMSCL,
		.pd_name = "spd-jpeg",
		.table[BS_DEFAULT].fn = BF_NOP,
		.table[BS_DISABLE].fn = BF_SETQOS_MO,
		.table[BS_DISABLE].mo = 0x10,
		.cur_scen = BS_DISABLE,
		.on = false,
		.enable = true,
	},
	[BTS_IDX_MSCL0_S1] = {
		.id = BTS_MSCL0_S1,
		.name = "mscl0_s1",
		.pa_base = EXYNOS7580_PA_SYSREG_MFCMSCL,
		.pd_name = "spd-mscl0",
		.table[BS_DEFAULT].fn = BF_NOP,
		.table[BS_DISABLE].fn = BF_SETQOS_MO,
		.table[BS_DISABLE].mo = 0x10,
		.cur_scen = BS_DISABLE,
		.on = false,
		.enable = true,
	},
	[BTS_IDX_MSCL1_S2] = {
		.id = BTS_MSCL1_S2,
		.name = "mscl1_s2",
		.pa_base = EXYNOS7580_PA_SYSREG_MFCMSCL,
		.pd_name = "spd-mscl1",
		.table[BS_DEFAULT].fn = BF_NOP,
		.table[BS_DISABLE].fn = BF_SETQOS_MO,
		.table[BS_DISABLE].mo = 0x10,
		.cur_scen = BS_DISABLE,
		.on = false,
		.enable = true,
	},
	[BTS_IDX_AUD] = {
		.id = BTS_AUD,
		.name = "aud",
		.pa_base = EXYNOS7580_PA_SYSREG_AUD,
		.pd_name = "pd-aud",
		.table[BS_DEFAULT].fn = BF_NOP,
		.table[BS_DISABLE].fn = BF_SETQOS,
		.table[BS_DISABLE].priority[0] = 0x4,
		.cur_scen = BS_DISABLE,
		.on = false,
		.enable = true,
	},
	[BTS_IDX_FSYS0] = {
		.id = BTS_FSYS0,
		.name = "fsys0",
		.pa_base = EXYNOS7580_PA_SYSREG_FSYS,
		.pd_name = "pd-fsys0",
		.table[BS_DEFAULT].fn = BF_NOP,
		.table[BS_DISABLE].fn = BF_SETQOS,
		.table[BS_DISABLE].priority[0] = 0x4,
		.cur_scen = BS_DISABLE,
		.on = false,
		.enable = true,
	},
	[BTS_IDX_FSYS1] = {
		.id = BTS_FSYS1,
		.name = "fsys1",
		.pa_base = EXYNOS7580_PA_SYSREG_FSYS,
		.pd_name = "pd-fsys1",
		.table[BS_DEFAULT].fn = BF_NOP,
		.table[BS_DISABLE].fn = BF_SETQOS,
		.table[BS_DISABLE].priority[0] = 0x4,
		.cur_scen = BS_DISABLE,
		.on = false,
		.enable = true,
	},
	[BTS_IDX_IMEM] = {
		.id = BTS_IMEM,
		.name = "imem",
		.pa_base = EXYNOS7580_PA_SYSREG_IMEM,
		.pd_name = "pd-imem",
		.table[BS_DEFAULT].fn = BF_NOP,
		.table[BS_DISABLE].fn = BF_SETQOS,
		.table[BS_DISABLE].priority[0] = 0x4,
		.cur_scen = BS_DISABLE,
		.on = false,
		.enable = true,
	},
	[BTS_IDX_MODAPIF] = {
		.id = BTS_MODAPIF,
		.name = "modapif",
		.pa_base = EXYNOS7580_PA_SYSREG_MIF,
		.pd_name = "pd-modapif",
		.table[BS_DEFAULT].fn = BF_SETQOS,
		.table[BS_DEFAULT].priority[0] = 0xD,
		.table[BS_DISABLE].fn = BF_SETQOS,
		.table[BS_DISABLE].priority[0] = 0x4,
		.cur_scen = BS_DISABLE,
		.on = false,
		.enable = true,
	},
	[BTS_IDX_CPU] = {
		.id = BTS_CPU,
		.name = "cpu",
		.pa_base = EXYNOS7580_PA_SYSREG_MIF,
		.pd_name = "pd-cpu",
		.table[BS_DEFAULT].fn = BF_NOP,
		.table[BS_DISABLE].fn = BF_SETQOS,
		.table[BS_DISABLE].priority[0] = 0x4,
		.cur_scen = BS_DISABLE,
		.on = false,
		.enable = true,
	},
	[BTS_IDX_APL] = {
		.id = BTS_APL,
		.name = "apl",
		.pa_base = EXYNOS7580_PA_SYSREG_MIF,
		.pd_name = "pd-apl",
		.table[BS_DEFAULT].fn = BF_NOP,
		.table[BS_DISABLE].fn = BF_SETQOS,
		.table[BS_DISABLE].priority[0] = 0x4,
		.cur_scen = BS_DISABLE,
		.on = false,
		.enable = true,
	},
	[BTS_IDX_G3D] = {
		.id = BTS_G3D,
		.name = "g3d",
		.pa_base = EXYNOS7580_PA_BTS_G3D,
		.pd_name = "pd-g3d",
		.table[BS_DEFAULT].fn = BF_NOP,
		.table[BS_DISABLE].fn = BF_DISABLE,
		.cur_scen = BS_DISABLE,
		.on = false,
		.enable = true,
	},
};

static struct bts_scenario bts_scen[] = {
	[BS_DISABLE] = {
		.name = "bts_disable",
		.id = BS_DISABLE,
	},
	[BS_DEFAULT] = {
		.name = "bts_default",
		.id = BS_DEFAULT,
	},
	[BS_DEBUG] = {
		.name = "bts_dubugging_ip",
		.id = BS_DEBUG,
	},
	[BS_MAX] = {
		.name = "undefined"
	}
};

static DEFINE_SPINLOCK(bts_lock);
static LIST_HEAD(bts_list);

static void is_bts_clk_enabled(struct bts_info *bts)
{
	struct clk_info *ptr;
	enum bts_index btstable_index;

	ptr = bts->ct_ptr;
	if (ptr) {
		btstable_index = ptr->index;
		do {
			if (!__clk_is_enabled(ptr->clk))
				pr_err("[BTS] CLK is not enabled : %s in %s\n",
						ptr->clk_name,
						bts->name);
		} while (++ptr < clk_table + ARRAY_SIZE(clk_table)
				&& ptr->index == btstable_index);
	}
}

static void bts_clk_on(struct bts_info *bts)
{
	struct clk_info *ptr;
	enum bts_index btstable_index;

	ptr = bts->ct_ptr;
	if (ptr) {
		btstable_index = ptr->index;
		do {
			clk_enable(ptr->clk);
		} while (++ptr < clk_table + ARRAY_SIZE(clk_table)
				&& ptr->index == btstable_index);
	}
}

static void bts_clk_off(struct bts_info *bts)
{
	struct clk_info *ptr;
	enum bts_index btstable_index;

	ptr = bts->ct_ptr;
	if (ptr) {
		btstable_index = ptr->index;
		do {
			clk_disable(ptr->clk);
		} while (++ptr < clk_table + ARRAY_SIZE(clk_table)
				&& ptr->index == btstable_index);
	}
}

static void bts_setqos_ip_sysreg(enum bts_id id, void __iomem *va_base,
					unsigned int *priority)
{
	switch (id) {
	case BTS_DISPLAY:
		bts_setqos_sysreg(BTS_SYSREG_DISP, va_base, priority);
		break;
	case BTS_ISP0:
		bts_setqos_sysreg(BTS_SYSREG_ISP0, va_base, priority);
		break;
	case BTS_ISP1:
		bts_setqos_sysreg(BTS_SYSREG_ISP1, va_base, priority);
		break;
	case BTS_MFC:
		bts_setqos_sysreg(BTS_SYSREG_MFC, va_base, priority);
		break;
	case BTS_JPEG:
		bts_setqos_sysreg(BTS_SYSREG_JPEG, va_base, priority);
		break;
	case BTS_MSCL0:
		bts_setqos_sysreg(BTS_SYSREG_MSCL0, va_base, priority);
		break;
	case BTS_MSCL1:
		bts_setqos_sysreg(BTS_SYSREG_MSCL1, va_base, priority);
		break;
	case BTS_AUD:
		bts_setqos_sysreg(BTS_SYSREG_AUD, va_base, priority);
		break;
	case BTS_FSYS0:
		bts_setqos_sysreg(BTS_SYSREG_FSYS0, va_base, priority);
		break;
	case BTS_FSYS1:
		bts_setqos_sysreg(BTS_SYSREG_FSYS1, va_base, priority);
		break;
	case BTS_IMEM:
		bts_setqos_sysreg(BTS_SYSREG_IMEM, va_base, priority);
		break;
	case BTS_MODAPIF:
		bts_setqos_sysreg(BTS_SYSREG_MIF_MODAPIF, va_base, priority);
		break;
	case BTS_CPU:
		bts_setqos_sysreg(BTS_SYSREG_MIF_CPU, va_base, priority);
		break;
	case BTS_APL:
		bts_setqos_sysreg(BTS_SYSREG_MIF_APL, va_base, priority);
		break;
	default:
		break;
	}
}

static void bts_setmo_ip_sysreg(enum bts_id id, void __iomem *va_base,
					unsigned int ar, unsigned int aw)
{
	switch (id) {
	case BTS_DISPLAY_M0:
		bts_setmo_sysreg(BTS_DISP_M0, va_base, ar, aw);
		break;
	case BTS_DISPLAY_S0:
		bts_setmo_sysreg(BTS_DISP_S0, va_base, ar, aw);
		break;
	case BTS_DISPLAY_S1:
		bts_setmo_sysreg(BTS_DISP_S1, va_base, ar, aw);
		break;
	case BTS_DISPLAY_S2:
		bts_setmo_sysreg(BTS_DISP_S2, va_base, ar, aw);
		break;
	case BTS_ISP_M0:
		bts_setmo_sysreg(BTS_ISP1_M0, va_base, ar, aw);
		break;
	case BTS_ISP_S0:
		bts_setmo_sysreg(BTS_ISP1_S0, va_base, ar, aw);
		break;
	case BTS_ISP_S1:
		bts_setmo_sysreg(BTS_ISP1_S1, va_base, ar, aw);
		break;
	case BTS_ISP_S2:
		bts_setmo_sysreg(BTS_ISP1_S2, va_base, ar, aw);
		break;
	case BTS_MFCMSCL_M0:
		bts_setmo_sysreg(BTS_MSCL_M0, va_base, ar, aw);
		break;
	case BTS_JPG_S0:
		bts_setmo_sysreg(BTS_JPEG_S0, va_base, ar, aw);
		break;
	case BTS_MSCL0_S1:
		bts_setmo_sysreg(BTS_MSCALER0_S1, va_base, ar, aw);
		break;
	case BTS_MSCL1_S2:
		bts_setmo_sysreg(BTS_MSCALER1_S2, va_base, ar, aw);
		break;
	default:
		break;
	}
}

static void bts_set_ip_table(enum exynos_bts_scenario scen,
		struct bts_info *bts)
{
	enum exynos_bts_function fn = bts->table[scen].fn;
	bool on;

	is_bts_clk_enabled(bts);
	BTS_DBG("[BTS] %s on:%d bts scen: [%s]->[%s]\n", bts->name, bts->on,
			bts_scen[bts->cur_scen].name, bts_scen[scen].name);

	switch (fn) {
	case BF_SETQOS:
		if (bts->id & BTS_SYSREG_DOMAIN) {
			bts_setqos_ip_sysreg(bts->id, bts->va_base, \
						bts->table[scen].priority);

			if (bts->id == BTS_ISP0) {
				on = (scen == BS_DEFAULT) ? true : false;

				bts_setotf_sysreg(BTS_SYSREG_FIMC_ISP_SEL, \
							bts->va_base, on);
				bts_setotf_sysreg(BTS_SYSREG_FIMC_SC_SEL, \
							bts->va_base, on);
				bts_setotf_sysreg(BTS_SYSREG_FIMC_FD_SEL, \
							bts->va_base, on);
			}
		} else if (bts->id == BTS_G3D)
			bts_setqos(bts->va_base, bts->table[scen].priority[0]);
		break;
	case BF_SETQOS_BW:
		if (bts->id == BTS_G3D)
			bts_setqos_bw(bts->va_base, bts->table[scen].priority[0], \
				bts->table[scen].window, bts->table[scen].token);
		break;
	case BF_SETQOS_MO:
		if (bts->id & BTS_SYSREG_DOMAIN)
			bts_setmo_ip_sysreg(bts->id, bts->va_base, \
						bts->table[scen].mo, \
						bts->table[scen].mo);
		else if (bts->id == BTS_G3D)
			bts_setqos_mo(bts->va_base, bts->table[scen].priority[0], \
					bts->table[scen].mo);
		break;
	case BF_DISABLE:
		if (bts->id == BTS_G3D)
			bts_disable(bts->va_base);
		break;
	case BF_NOP:
		break;
	}

	bts->cur_scen = scen;
}

static enum exynos_bts_scenario bts_get_scen(struct bts_info *bts)
{
	enum exynos_bts_scenario scen;

	scen = BS_DEFAULT;

	return scen;
}


static void bts_add_scen(enum exynos_bts_scenario scen, struct bts_info *bts)
{
	struct bts_info *first = bts;
	int next = 0;
	int prev = 0;

	if (!bts)
		return;

	BTS_DBG("[bts %s] scen %s off\n",
			bts->name, bts_scen[scen].name);

	do {
		if (bts->enable) {
			if (bts->table[scen].next_scen == 0) {
				if (scen >= bts->top_scen) {
					bts->table[scen].prev_scen = bts->top_scen;
					bts->table[bts->top_scen].next_scen = scen;
					bts->top_scen = scen;
					bts->table[scen].next_scen = -1;

					if (bts->on)
						bts_set_ip_table(bts->top_scen, bts);

				} else {
					for (prev = bts->top_scen; prev > scen; prev = bts->table[prev].prev_scen)
						next = prev;

					bts->table[scen].prev_scen = bts->table[next].prev_scen;
					bts->table[scen].next_scen = bts->table[prev].next_scen;
					bts->table[next].prev_scen = scen;
					bts->table[prev].next_scen = scen;
				}
			}
		}

		bts = bts->table[scen].next_bts;

	} while (bts && bts != first);
}

static void bts_del_scen(enum exynos_bts_scenario scen, struct bts_info *bts)
{
	struct bts_info *first = bts;
	int next = 0;
	int prev = 0;

	if (!bts)
		return;

	BTS_DBG("[bts %s] scen %s off\n",
			bts->name, bts_scen[scen].name);

	do {
		if (bts->enable) {
			if (bts->table[scen].next_scen != 0) {
				if (scen == bts->top_scen) {
					prev = bts->table[scen].prev_scen;
					bts->top_scen = prev;
					bts->table[prev].next_scen = -1;
					bts->table[scen].next_scen = 0;
					bts->table[scen].prev_scen = 0;

					if (bts->on)
						bts_set_ip_table(prev, bts);
				} else if (scen < bts->top_scen) {
					prev = bts->table[scen].prev_scen;
					next = bts->table[scen].next_scen;

					bts->table[next].prev_scen = bts->table[scen].prev_scen;
					bts->table[prev].next_scen = bts->table[scen].next_scen;

					bts->table[scen].prev_scen = 0;
					bts->table[scen].next_scen = 0;

				} else {
					BTS_DBG("%s scenario couldn't exist above top_scen\n", bts_scen[scen].name);
				}
			}

		}

		bts = bts->table[scen].next_bts;

	} while (bts && bts != first);
}

void bts_scen_update(enum bts_scen_type type, unsigned int val)
{
	enum exynos_bts_scenario scen = BS_DEFAULT;
	struct bts_info *bts = NULL;
	bool on;

	spin_lock(&bts_lock);

	switch (type) {
	case TYPE_CAM_BNS:
		on = val ? true : false;
		scen = BS_CAM_BNS;
		bts = &exynos7_bts[BTS_IDX_ISP0];
		BTS_DBG("[BTS] CAM_BNS: %s\n", bts_scen[scen].name);
		break;
	default:
		spin_unlock(&bts_lock);
		return;
	}

	if (on)
		bts_add_scen(scen, bts);
	else
		bts_del_scen(scen, bts);

	spin_unlock(&bts_lock);
}

void bts_initialize(const char *pd_name, bool on)
{
	struct bts_info *bts;
	enum exynos_bts_scenario scen = BS_DISABLE;

	spin_lock(&bts_lock);

	list_for_each_entry(bts, &bts_list, list)
		if (pd_name && bts->pd_name && !strncmp(bts->pd_name, pd_name, strlen(pd_name))) {
			BTS_DBG("[BTS] %s on/off:%d->%d\n", bts->name, bts->on, on);

			if (!bts->on && on)
				bts_clk_on(bts);
			else if (bts->on && !on)
				bts_clk_off(bts);

			if (!bts->enable) {
				bts->on = on;
				continue;
			}

			scen = bts_get_scen(bts);
			if (on) {
				bts_add_scen(scen, bts);
				if (!bts->on) {
					bts->on = true;
					bts_set_ip_table(bts->top_scen, bts);
				}
			} else {
				if (bts->on)
					bts->on = false;

				bts_del_scen(scen, bts);
			}
		}

	spin_unlock(&bts_lock);
}

static void scen_chaining(enum exynos_bts_scenario scen)
{
	struct bts_info *prev = NULL;
	struct bts_info *first = NULL;
	struct bts_info *bts;

	if (bts_scen[scen].ip) {
		list_for_each_entry(bts, &bts_list, list) {
			if (bts_scen[scen].ip & bts->id) {
				if (!first)
					first = bts;
				if (prev)
					prev->table[scen].next_bts = bts;

				prev = bts;
			}
		}

		if (prev)
			prev->table[scen].next_bts = first;

		bts_scen[scen].head = first;
	}
}

static int exynos7_qos_status_open_show(struct seq_file *buf, void *d)
{
	unsigned int i;
	unsigned int val_r, val_w;
	unsigned int r_ctrl, w_ctrl;
	unsigned int r_mo, w_mo;

	spin_lock(&bts_lock);

	for (i = 0; i < ARRAY_SIZE(exynos7_bts); i++) {
		seq_printf(buf, "bts[%d] %s : ", i, exynos7_bts[i].name);
		if (exynos7_bts[i].on) {
			if (exynos7_bts[i].ct_ptr)
				bts_clk_on(exynos7_bts + i);

			/* axi qoscontrol */
			switch (exynos7_bts[i].id) {
			case BTS_DISPLAY:
				val_r = __raw_readl(exynos7_bts[i].va_base + DISP_XIU_DISP_QOS_CON);
				seq_printf(buf, "on, priority: S0_R:0x%x S1_R:0x%x S2_R:0x%x, scen:%s\n",
						(val_r&0xF), ((val_r>>4)&0xF),
						((val_r>>8)&0xF),
						bts_scen[exynos7_bts[i].cur_scen].name);
				break;
			case BTS_DISPLAY_M0:
				r_mo = __raw_readl(exynos7_bts[i].va_base + DISP_XIU_DISP1_AR_AC_TARGET_CON);
				seq_printf(buf, "R_MO:0x%x\n", ((r_mo>>25)&0x3F));
				break;
			case BTS_DISPLAY_S2:
				r_mo = __raw_readl(exynos7_bts[i].va_base + DISP_XIU_DISP1_AR_AC_TARGET_CON);
				seq_printf(buf, "R_MO:0x%x\n", ((r_mo>>10)&0x1F));
				break;
			case BTS_DISPLAY_S1:
				r_mo = __raw_readl(exynos7_bts[i].va_base + DISP_XIU_DISP1_AR_AC_TARGET_CON);
				seq_printf(buf, "R_MO:0x%x\n", ((r_mo>>5)&0x1F));
				break;
			case BTS_DISPLAY_S0:
				r_mo = __raw_readl(exynos7_bts[i].va_base + DISP_XIU_DISP1_AR_AC_TARGET_CON);
				seq_printf(buf, "R_MO:0x%x\n", (r_mo&0x1F));
				break;
			case BTS_ISP0:
				val_r = __raw_readl(exynos7_bts[i].va_base + ISP_QOS_CON0);
				seq_printf(buf, "on, priority: CPU_ISP_R:0x%x CPU_ISP_W:0x%x FIMC_ISP_R:0x%x FIMC_ISP_W:0x%x FIMC_FD_R:0x%x FIMC_FD_W:0x%x FIMC_SCALER_W:0x%x, scen:%s\n",
						(val_r&0xF), ((val_r>>4)&0xF),
						((val_r>>8)&0xF), ((val_r>>12)&0xF),
						((val_r>>16)&0xF), ((val_r>>20)&0xF),
						((val_r>>28)&0xF),
						bts_scen[exynos7_bts[i].cur_scen].name);
				break;
			case BTS_ISP1:
				val_r = __raw_readl(exynos7_bts[i].va_base + ISP_QOS_CON1);
				seq_printf(buf, "on, priority: FIMC_BNS_W:0x%x FIMC_BNS_L_W:0x%x, scen:%s\n",
						((val_r>>4)&0xF), ((val_r>>12)&0xF),
						bts_scen[exynos7_bts[i].cur_scen].name);
				break;
			case BTS_ISP_M0:
				r_mo = __raw_readl(exynos7_bts[i].va_base + ISP_XIU_ISP1_AR_AC_TARGET_CON);
				w_mo = __raw_readl(exynos7_bts[i].va_base + ISP_XIU_ISP1_AW_AC_TARGET_CON);
				seq_printf(buf, "R_MO:0x%x W_MO:0x%x\n",
						((r_mo>>24)&0x3F), ((w_mo>>24)&0x3F));
				break;
			case BTS_ISP_S2:
				r_mo = __raw_readl(exynos7_bts[i].va_base + ISP_XIU_ISP1_AR_AC_TARGET_CON);
				w_mo = __raw_readl(exynos7_bts[i].va_base + ISP_XIU_ISP1_AW_AC_TARGET_CON);
				seq_printf(buf, "R_MO:0x%x W_MO:0x%x\n",
						((r_mo>>16)&0x3F), ((w_mo>>16)&0x3F));
				break;
			case BTS_ISP_S1:
				r_mo = __raw_readl(exynos7_bts[i].va_base + ISP_XIU_ISP1_AR_AC_TARGET_CON);
				w_mo = __raw_readl(exynos7_bts[i].va_base + ISP_XIU_ISP1_AW_AC_TARGET_CON);
				seq_printf(buf, "R_MO:0x%x W_MO:0x%x\n",
						((r_mo>>8)&0x3F), ((w_mo>>8)&0x3F));
				break;
			case BTS_ISP_S0:
				r_mo = __raw_readl(exynos7_bts[i].va_base + ISP_XIU_ISP1_AR_AC_TARGET_CON);
				w_mo = __raw_readl(exynos7_bts[i].va_base + ISP_XIU_ISP1_AW_AC_TARGET_CON);
				seq_printf(buf, "R_MO:0x%x W_MO:0x%x\n",
						(r_mo&0x3F), (w_mo&0x3F));
				break;
			case BTS_MFC:
				val_r = __raw_readl(exynos7_bts[i].va_base + MFCMSCL_QOS_CON);
				seq_printf(buf, "on, priority: MFC_R:0x%x MFC_W:0x%x, scen:%s\n",
						((val_r>>24)&0xF), ((val_r>>28)&0xF),
						bts_scen[exynos7_bts[i].cur_scen].name);
				break;
			case BTS_JPEG:
				val_r = __raw_readl(exynos7_bts[i].va_base + MFCMSCL_QOS_CON);
				seq_printf(buf, "on, priority: JPEG_R:0x%x JPEG_W:0x%x, scen:%s\n",
						((val_r>>16)&0xF), ((val_r>>20)&0xF),
						bts_scen[exynos7_bts[i].cur_scen].name);
			case BTS_MSCL1:
				val_r = __raw_readl(exynos7_bts[i].va_base + MFCMSCL_QOS_CON);
				seq_printf(buf, "on, priority: MSCL1_R:0x%x MSCL1_W:0x%x, scen:%s\n",
						((val_r>>8)&0xF), ((val_r>>12)&0xF),
						bts_scen[exynos7_bts[i].cur_scen].name);
			case BTS_MSCL0:
				val_r = __raw_readl(exynos7_bts[i].va_base + MFCMSCL_QOS_CON);
				seq_printf(buf, "on, priority: MSCL0_R:0x%x MSCL0_W:0x%x, scen:%s\n",
						(val_r&0xF), ((val_r>>4)&0xF),
						bts_scen[exynos7_bts[i].cur_scen].name);
				break;
			case BTS_MFCMSCL_M0:
				r_mo = __raw_readl(exynos7_bts[i].va_base + MFCMSCL_XIU_MSCL0DX_M0);
				seq_printf(buf, "R_MO:0x%x W_MO:0x%x\n",
						(r_mo&0x3F), ((r_mo>>8)&0x3F));
				break;
			case BTS_JPG_S0:
				r_mo = __raw_readl(exynos7_bts[i].va_base + MFCMSCL_XIU_MSCL0DX_S0);
				seq_printf(buf, "R_MO:0x%x W_MO:0x%x\n",
						(r_mo&0x1F), ((r_mo>>8)&0x1F));
				break;
			case BTS_MSCL0_S1:
				r_mo = __raw_readl(exynos7_bts[i].va_base + MFCMSCL_XIU_MSCL0DX_S1);
				seq_printf(buf, "R_MO:0x%x W_MO:0x%x\n",
						(r_mo&0x1F), ((r_mo>>8)&0x1F));
				break;
			case BTS_MSCL1_S2:
				r_mo = __raw_readl(exynos7_bts[i].va_base + MFCMSCL_XIU_MSCL0DX_S2);
				seq_printf(buf, "R_MO:0x%x W_MO:0x%x\n",
						(r_mo&0x1F), ((r_mo>>8)&0x1F));
				break;
			case BTS_AUD:
				val_r = __raw_readl(exynos7_bts[i].va_base + AUD_QOS_CON);
				seq_printf(buf, "on, priority : AUD_R:0x%x AUD_W:0x%x, scen:%s\n",
						(val_r&0xF), ((val_r>>4)&0xF),
						bts_scen[exynos7_bts[i].cur_scen].name);
				break;
			case BTS_FSYS0:
				val_r = __raw_readl(exynos7_bts[i].va_base + FSYS_QOS_CON0);
				seq_printf(buf, "on, priority: USBHS_R:0x%x USBHS_W:0x%x PDMA0_R:0x%x PDMA0_W:0x%x USBOTG_R:0x%x USBOTG_W:0x%x PDMA1_R:0x%x PDMA1_W:0x%x, scen:%s\n",
						(val_r&0xF), ((val_r>>4)&0xF),
						((val_r>>8)&0xF), ((val_r>>12)&0xF),
						((val_r>>16)&0xF), ((val_r>>20)&0xF),
						((val_r>>24)&0xF), ((val_r>>28)&0xF),
						bts_scen[exynos7_bts[i].cur_scen].name);
				break;
			case BTS_FSYS1:
				val_r = __raw_readl(exynos7_bts[i].va_base + FSYS_QOS_CON1);
				seq_printf(buf, "on, priority: MMC0_R:0x%x MMC0_W:0x%x MMC1_R:0x%x MMC1_W:0x%x MMC2_R:0x%x MMC2_W:0x%x, scen:%s\n",
						(val_r&0xF), ((val_r>>4)&0xF),
						((val_r>>8)&0xF), ((val_r>>12)&0xF),
						((val_r>>16)&0xF), ((val_r>>20)&0xF),
						bts_scen[exynos7_bts[i].cur_scen].name);
				break;
			case BTS_IMEM:
				val_r = __raw_readl(exynos7_bts[i].va_base + IMEM_QOS_CON);
				seq_printf(buf, "on, priority: SSS0_R:0x%x SSS0_W:0x%x SSS1_R:0x%x SSS1_W:0x%x RTIC_R:0x%x, scen:%s\n",
						(val_r&0xF), ((val_r>>4)&0xF),
						((val_r>>8)&0xF), ((val_r>>12)&0xF),
						((val_r>>16)&0xF),
						bts_scen[exynos7_bts[i].cur_scen].name);
				break;
			case BTS_MODAPIF:
				val_r = __raw_readl(exynos7_bts[i].va_base + MIF_MODAPIF_QOS_CON);
				seq_printf(buf, "on, priority: MODAPIF_R:0x%x MODAPIF_W:0x%x, scen:%s\n",
						((val_r>>4)&0xF), ((val_r>>8)&0xF),
						bts_scen[exynos7_bts[i].cur_scen].name);
				break;
			case BTS_CPU:
				val_r = __raw_readl(exynos7_bts[i].va_base + MIF_CPU_QOS_CON);
				seq_printf(buf, "on, priority: CPU_R:0x%x CPU_W:0x%x, scen:%s\n",
						(val_r&0xF), ((val_r>>4)&0xF),
						bts_scen[exynos7_bts[i].cur_scen].name);
				break;
			case BTS_APL:
				val_r = __raw_readl(exynos7_bts[i].va_base + MIF_APL_QOS_CON);
				seq_printf(buf, "on, priority: APL_R:0x%x APL_W:0x%x, scen:%s\n",
						(val_r&0xF), ((val_r>>4)&0xF),
						bts_scen[exynos7_bts[i].cur_scen].name);
				break;
			case BTS_G3D:
				r_ctrl = __raw_readl(exynos7_bts[i].va_base + READ_QOS_CONTROL);
				w_ctrl = __raw_readl(exynos7_bts[i].va_base + WRITE_QOS_CONTROL);
				if (r_ctrl && w_ctrl) {
					val_r = __raw_readl(exynos7_bts[i].va_base + READ_CHANNEL_PRIORITY);
					val_w = __raw_readl(exynos7_bts[i].va_base + WRITE_CHANNEL_PRIORITY);
					seq_printf(buf, "on, priority ch_r:0x%x,ch_w:0x%x, scen:%s\n",
							val_r, val_w,
							bts_scen[exynos7_bts[i].cur_scen].name);
				} else {
					seq_printf(buf, "control disable, scen:%s\n",
							bts_scen[exynos7_bts[i].cur_scen].name);
				}
				r_mo = __raw_readl(exynos7_bts[i].va_base + READ_MO);
				w_mo = __raw_readl(exynos7_bts[i].va_base + WRITE_MO);
				seq_printf(buf, "R_MO: 0x%x, W_MO: 0x%x\n",
						r_mo, w_mo);
				break;
			default:
				break;
			}

			if (exynos7_bts[i].ct_ptr)
				bts_clk_off(exynos7_bts + i);
		} else {
			seq_puts(buf, "off\n");
		}
	}

	spin_unlock(&bts_lock);

	return 0;
}

static int exynos7_qos_open(struct inode *inode, struct file *file)
{
	return single_open(file, exynos7_qos_status_open_show, inode->i_private);
}

static const struct file_operations debug_qos_status_fops = {
	.open		= exynos7_qos_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int exynos7_bw_status_open_show(struct seq_file *buf, void *d)
{
	mutex_lock(&media_mutex);

	seq_printf(buf, "bts bandwidth (total %u) : decon %u, cam %u, mif_freq %u\n",
			total_bw, decon_bw, cam_bw, mif_freq);

	mutex_unlock(&media_mutex);

	return 0;
}

static int exynos7_bw_open(struct inode *inode, struct file *file)
{
	return single_open(file, exynos7_bw_status_open_show, inode->i_private);
}

static const struct file_operations debug_bw_status_fops = {
	.open		= exynos7_bw_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int debug_enable_get(void *data, unsigned long long *val)
{
	struct bts_info *first = bts_scen[BS_DEBUG].head;
	struct bts_info *bts = bts_scen[BS_DEBUG].head;
	int cnt = 0;

	if (first) {
		do {
			pr_info("%s, ", bts->name);
			cnt++;
			bts = bts->table[BS_DEBUG].next_bts;
		} while (bts && bts != first);
	}
	if (first && first->top_scen == BS_DEBUG)
		pr_info("is on\n");
	else
		pr_info("is off\n");
	*val = cnt;

	return 0;
}

static int debug_enable_set(void *data, unsigned long long val)
{
	struct bts_info *first = bts_scen[BS_DEBUG].head;
	struct bts_info *bts = bts_scen[BS_DEBUG].head;

	if (first) {
		do {
			pr_info("%s, ", bts->name);

			bts = bts->table[BS_DEBUG].next_bts;
		} while (bts && bts != first);
	}

	spin_lock(&bts_lock);

	if (val) {
		bts_add_scen(BS_DEBUG, bts_scen[BS_DEBUG].head);
		pr_info("is on\n");
	} else {
		bts_del_scen(BS_DEBUG, bts_scen[BS_DEBUG].head);
		pr_info("is off\n");
	}

	spin_unlock(&bts_lock);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(debug_enable_fops, debug_enable_get, debug_enable_set, "%llx\n");

static void bts_status_print(void)
{
	pr_info("0 : disable debug ip\n");
	pr_info("1 : BF_SETQOS\n");
	pr_info("2 : BF_SETQOS_BW\n");
	pr_info("3 : BF_SETQOS_MO\n");
}

static int debug_ip_enable_get(void *data, unsigned long long *val)
{
	struct bts_info *bts = data;

	if (bts->table[BS_DEBUG].next_scen) {
		switch (bts->table[BS_DEBUG].fn) {
		case BF_SETQOS:
			*val = 1;
			break;
		case BF_SETQOS_BW:
			*val = 2;
			break;
		case BF_SETQOS_MO:
			*val = 3;
			break;
		default:
			*val = 4;
			break;
		}
	} else {
		*val = 0;
	}

	bts_status_print();

	return 0;
}

static int debug_ip_enable_set(void *data, unsigned long long val)
{
	struct bts_info *bts = data;

	spin_lock(&bts_lock);

	if (val) {
		bts_scen[BS_DEBUG].ip |= bts->id;

		scen_chaining(BS_DEBUG);

		switch (val) {
		case 1:
			bts->table[BS_DEBUG].fn = BF_SETQOS;
			break;
		case 2:
			bts->table[BS_DEBUG].fn = BF_SETQOS_BW;
			break;
		case 3:
			bts->table[BS_DEBUG].fn = BF_SETQOS_MO;
			break;
		default:
			break;
		}

		bts_add_scen(BS_DEBUG, bts);

		pr_info("%s on 0x%x\n", bts->name, bts_scen[BS_DEBUG].ip);
	} else {
		bts->table[BS_DEBUG].next_bts = NULL;
		bts_del_scen(BS_DEBUG, bts);

		bts_scen[BS_DEBUG].ip &= ~bts->id;
		scen_chaining(BS_DEBUG);

		pr_info("%s off 0x%x\n", bts->name, bts_scen[BS_DEBUG].ip);
	}

	spin_unlock(&bts_lock);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(debug_ip_enable_fops, debug_ip_enable_get, debug_ip_enable_set, "%llx\n");

static int debug_ip_mo_get(void *data, unsigned long long *val)
{
	struct bts_info *bts = data;

	spin_lock(&bts_lock);

	*val = bts->table[BS_DEBUG].mo;

	spin_unlock(&bts_lock);

	return 0;
}

static int debug_ip_mo_set(void *data, unsigned long long val)
{
	struct bts_info *bts = data;

	spin_lock(&bts_lock);

	bts->table[BS_DEBUG].mo = val;
	if (bts->top_scen == BS_DEBUG) {
		if (bts->on)
			bts_set_ip_table(BS_DEBUG, bts);
	}
	pr_info("Debug mo set %s : mo 0x%x\n", bts->name, bts->table[BS_DEBUG].mo);

	spin_unlock(&bts_lock);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(debug_ip_mo_fops, debug_ip_mo_get, debug_ip_mo_set, "%llx\n");

static int debug_ip_token_get(void *data, unsigned long long *val)
{
	struct bts_info *bts = data;

	spin_lock(&bts_lock);

	if (bts->id == BTS_G3D)
		*val = bts->table[BS_DEBUG].token;
	else
		*val = 0;

	spin_unlock(&bts_lock);

	return 0;
}

static int debug_ip_token_set(void *data, unsigned long long val)
{
	struct bts_info *bts = data;

	spin_lock(&bts_lock);

	if (bts->id == BTS_G3D) {
		bts->table[BS_DEBUG].token = val;
		if (bts->top_scen == BS_DEBUG) {
			if (bts->on && bts->table[BS_DEBUG].window)
				bts_set_ip_table(BS_DEBUG, bts);
		}
		pr_info("Debug bw set %s : priority 0x%x window 0x%x token 0x%x\n",
			bts->name, bts->table[BS_DEBUG].priority[0],
			bts->table[BS_DEBUG].window,
			bts->table[BS_DEBUG].token);
	}

	spin_unlock(&bts_lock);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(debug_ip_token_fops, debug_ip_token_get, debug_ip_token_set, "%llx\n");

static int debug_ip_window_get(void *data, unsigned long long *val)
{
	struct bts_info *bts = data;

	spin_lock(&bts_lock);

	if (bts->id == BTS_G3D)
		*val = bts->table[BS_DEBUG].window;
	else
		*val = 0;

	spin_unlock(&bts_lock);

	return 0;
}

static int debug_ip_window_set(void *data, unsigned long long val)
{
	struct bts_info *bts = data;

	spin_lock(&bts_lock);

	if (bts->id == BTS_G3D) {
		bts->table[BS_DEBUG].window = val;
		if (bts->top_scen == BS_DEBUG) {
			if (bts->on && bts->table[BS_DEBUG].token)
				bts_set_ip_table(BS_DEBUG, bts);
		}
		pr_info("Debug bw set %s : priority 0x%x window 0x%x token 0x%x\n",
			bts->name, bts->table[BS_DEBUG].priority[0],
			bts->table[BS_DEBUG].window,
			bts->table[BS_DEBUG].token);
	}

	spin_unlock(&bts_lock);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(debug_ip_window_fops, debug_ip_window_get, debug_ip_window_set, "%llx\n");

static int debug_ip_qos_get(void *data, unsigned long long *val)
{
	struct bts_info *bts = data;

	spin_lock(&bts_lock);

	*val = bts->table[BS_DEBUG].priority[0];

	spin_unlock(&bts_lock);

	return 0;
}

static int debug_ip_qos_set(void *data, unsigned long long val)
{
	struct bts_info *bts = data;

	spin_lock(&bts_lock);

	bts->table[BS_DEBUG].priority[0] = val;
	if (bts->top_scen == BS_DEBUG) {
		if (bts->on)
			bts_setqos(bts->va_base, bts->table[BS_DEBUG].priority[0]);
	}
	pr_info("Debug qos set %s : 0x%x\n", bts->name, bts->table[BS_DEBUG].priority[0]);

	spin_unlock(&bts_lock);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(debug_ip_qos_fops, debug_ip_qos_get, debug_ip_qos_set, "%llx\n");

void bts_debugfs(void)
{
	struct bts_info *bts;
	struct dentry *den;
	struct dentry *subden;

	den = debugfs_create_dir("bts_dbg", NULL);
	debugfs_create_file("qos_status", 0440,	den, NULL, &debug_qos_status_fops);
	debugfs_create_file("bw_status", 0440,	den, NULL, &debug_bw_status_fops);
	debugfs_create_file("enable", 0440,	den, NULL, &debug_enable_fops);

	den = debugfs_create_dir("bts", den);
	list_for_each_entry(bts, &bts_list, list) {
		subden = debugfs_create_dir(bts->name, den);
		debugfs_create_file("qos", 0644, subden, bts, &debug_ip_qos_fops);
		debugfs_create_file("token", 0644, subden, bts, &debug_ip_token_fops);
		debugfs_create_file("window", 0644, subden, bts, &debug_ip_window_fops);
		debugfs_create_file("mo", 0644, subden, bts, &debug_ip_mo_fops);
		debugfs_create_file("enable", 0644, subden, bts, &debug_ip_enable_fops);
	}
}

static void bts_drex_init(void __iomem *base)
{

	BTS_DBG("[BTS][%s] bts drex init\n", __func__);

	__raw_writel(0x00000000, base + QOS_TIMEOUT_0xF);
	__raw_writel(0x00200020, base + QOS_TIMEOUT_0xE);
	__raw_writel(0x00800080, base + QOS_TIMEOUT_0xD);
	__raw_writel(0x00800080, base + QOS_TIMEOUT_0xC);
	__raw_writel(0x00000100, base + QOS_TIMEOUT_0xB);
	__raw_writel(0x00000200, base + QOS_TIMEOUT_0xA);
	__raw_writel(0x0CFF0FFF, base + QOS_TIMEOUT_0x8);
	__raw_writel(0x00000000, base + BRB_CON);
	__raw_writel(0x88888888, base + BRB_THRESHOLD);
}

static int exynos_bts_notifier_event(struct notifier_block *this,
		unsigned long event,
		void *ptr)
{
	switch (event) {
	case PM_POST_SUSPEND:
		bts_drex_init(base_drex);
		bts_initialize("pd-modapif", true);

		return NOTIFY_OK;
	}

	return NOTIFY_DONE;
}

static struct notifier_block exynos_bts_notifier = {
	.notifier_call = exynos_bts_notifier_event,
};

int exynos7_update_bts_param(int target_idx, int work)
{
	if (!work)
		return 0;

	spin_lock(&timeout_mutex);

	cur_target_idx = target_idx;

	if (cur_target_idx >= 6)
		__raw_writel(0xdd0, base_bts_sysreg_mif_modapif + MIF_MODAPIF_QOS_CON);
	else
		__raw_writel(0xdd1, base_bts_sysreg_mif_modapif + MIF_MODAPIF_QOS_CON);

	__raw_writel(exynos7_timeout_table[target_idx], base_drex + QOS_TIMEOUT_0xD);

	if (decon_bw && !cam_bw) {
		__raw_writel(exynos7_bts_param_table[target_idx][num_active_disp_win], base_drex + QOS_TIMEOUT_0xA);
		__raw_writel(0x01000100, base_drex + QOS_TIMEOUT_0xC);
	} else if (decon_bw && cam_bw) {
		__raw_writel(exynos7_bts_param_table[target_idx][num_active_disp_win + 4], base_drex + QOS_TIMEOUT_0xC);
		if ((target_idx >= 0) && (target_idx < 2))
			__raw_writel(0x02000200, base_drex + QOS_TIMEOUT_0xA);
		else if ((target_idx >= 2) && (target_idx < 4))
			__raw_writel(0x01000100, base_drex + QOS_TIMEOUT_0xA);
	}

	spin_unlock(&timeout_mutex);

	return 0;
}

static int exynos7_bts_notify(unsigned long freq)
{
	BUG_ON(irqs_disabled());

	return srcu_notifier_call_chain(&exynos_media_notifier, freq, NULL);
}

int exynos7_bts_register_notifier(struct notifier_block *nb)
{
	return srcu_notifier_chain_register(&exynos_media_notifier, nb);
}

int exynos7_bts_unregister_notifier(struct notifier_block *nb)
{
	return srcu_notifier_chain_unregister(&exynos_media_notifier, nb);
}

void exynos7_init_bts_ioremap(void)
{
	base_drex = ioremap(EXYNOS7580_PA_DREX0, SZ_4K);
	base_bts_sysreg_mif_modapif = ioremap(EXYNOS7580_PA_SYSREG_MIF, SZ_4K);

	pm_qos_add_request(&exynos7_mif_bts_qos, PM_QOS_BUS_THROUGHPUT, 0);
	pm_qos_add_request(&exynos7_int_bts_qos, PM_QOS_DEVICE_THROUGHPUT, 0);
}

void exynos7_update_media_scenario(enum bts_media_type media_type,
		unsigned int bw, int bw_type)
{
	mutex_lock(&media_mutex);

	switch (media_type) {
	case TYPE_DECON_INT:
		decon_bw = bw;
		num_active_disp_win = decon_bw / FHD_BW;
		break;
	case TYPE_CAM:
		cam_bw = bw;
		break;
	default:
		pr_err("DEVFREQ(MIF) : unsupportd media_type - %u", media_type);
		break;
	}

	total_bw = decon_bw + cam_bw;

	/* MIF minimum frequency calculation as per BTS guide */
	if (cam_bw && decon_bw) {
		if (decon_bw <= (2 * FHD_BW))
			mif_freq = 667000;
		else
			mif_freq = 728000;
	} else {
		if (decon_bw <= (2 * FHD_BW))
			mif_freq = 416000;
		else
			mif_freq = 559000;
	}

	exynos7_bts_notify(mif_freq);

	BTS_DBG("[BTS BW] total: %u, decon %u, cam %u\n",
			total_bw, decon_bw, cam_bw);
	BTS_DBG("[BTS FREQ] mif_freq: %u\n", mif_freq);

	pm_qos_update_request(&exynos7_mif_bts_qos, mif_freq);

	mutex_unlock(&media_mutex);
}

static int __init exynos7_bts_init(void)
{
	int i;
	int ret;
	enum bts_index btstable_index = BTS_MAX - 1;

	BTS_DBG("[BTS][%s] bts init\n", __func__);

	for (i = 0; i < ARRAY_SIZE(clk_table); i++) {
		if (btstable_index != clk_table[i].index) {
			btstable_index = clk_table[i].index;
			exynos7_bts[btstable_index].ct_ptr = clk_table + i;
		}

		clk_table[i].clk = clk_get(NULL, clk_table[i].clk_name);
		if (IS_ERR(clk_table[i].clk)) {
			BTS_DBG("failed to get bts clk %s\n",
					clk_table[i].clk_name);
			exynos7_bts[btstable_index].ct_ptr = NULL;
		} else {
			ret = clk_prepare(clk_table[i].clk);
			if (ret) {
				pr_err("[BTS] failed to prepare bts clk %s\n",
						clk_table[i].clk_name);
				for (; i >= 0; i--)
					clk_put(clk_table[i].clk);
				return ret;
			}
		}
	}

	for (i = 0; i < ARRAY_SIZE(exynos7_bts); i++) {
		exynos7_bts[i].va_base = ioremap(exynos7_bts[i].pa_base, SZ_8K);

		list_add(&exynos7_bts[i].list, &bts_list);
	}

	for (i = BS_DEFAULT + 1; i < BS_MAX; i++)
		scen_chaining(i);

	exynos7_init_bts_ioremap();
	bts_drex_init(base_drex);

	bts_initialize("pd-modapif", true);

	register_pm_notifier(&exynos_bts_notifier);

	bts_debugfs();

	srcu_init_notifier_head(&exynos_media_notifier);

	return 0;
}
arch_initcall(exynos7_bts_init);
