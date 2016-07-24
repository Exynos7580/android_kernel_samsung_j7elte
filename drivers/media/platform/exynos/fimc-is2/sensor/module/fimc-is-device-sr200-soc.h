/*
 * Samsung Exynos5 SoC series Sensor driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_DEVICE_SR200_H
#define FIMC_IS_DEVICE_SR200_H


/*#define CONFIG_LOAD_FILE*/
#if defined(CONFIG_LOAD_FILE)
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define LOAD_FILE_PATH "/data/media/0/fimc-is-device-sr200-soc-reg.h"
#endif
#define SENSOR_SR200_INSTANCE	1
#define SENSOR_SR200_NAME	SENSOR_NAME_SR200
#define SENSOR_SR200_DRIVING
#define SENSOR_REGISTER_REGSET(y)		\
	{					\
		.reg	= (y),			\
		.size	= ARRAY_SIZE((y)),	\
	}

#define CHECK_ERR_COND(condition, ret)	\
	do { if (unlikely(condition)) return ret; } while (0)
#define CHECK_ERR_COND_MSG(condition, ret, fmt, ...) \
		if (unlikely(condition)) { \
			err("%s: error, " fmt, __func__, ##__VA_ARGS__); \
			return ret; \
		}
#define CHECK_ERR(x)	CHECK_ERR_COND(((x) < 0), (x))
#define CHECK_ERR_MSG(x, fmt, ...) \
	CHECK_ERR_COND_MSG(((x) < 0), (x), fmt, ##__VA_ARGS__)

#define TAG_NAME	"[SR200] "
#define cam_err(fmt, ...)	\
	printk(KERN_ERR TAG_NAME fmt, ##__VA_ARGS__)
#define cam_warn(fmt, ...)	\
	printk(KERN_WARNING TAG_NAME fmt, ##__VA_ARGS__)
#define cam_info(fmt, ...)	\
	printk(KERN_INFO TAG_NAME fmt, ##__VA_ARGS__)
#if 1 // defined(CONFIG_CAM_DEBUG)
#define cam_dbg(fmt, ...)	\
	printk(KERN_DEBUG TAG_NAME fmt, ##__VA_ARGS__)
#endif

struct sr200_preview {
	const struct sr200_framesize *frmsize;
	u32 update_frmsize:1;
	u32 fast_ae:1;
};

struct sr200_capture {
	const struct sr200_framesize *frmsize;
	u32 pre_req;	/* for fast capture */
	u32 ae_manual_mode:1;
	u32 lowlux_night:1;
	u32 ready:1;	/* for fast capture */
};

struct sensor_reg {
	u8 addr;
	u8 data;
};

struct sensor_regset {
	struct sensor_reg	*reg;
	u32			size;
};

struct sensor_regset_table {
	struct sensor_regset init;
	struct sensor_regset init_vt;
	struct sensor_regset effect_none;
	struct sensor_regset effect_negative;
	struct sensor_regset effect_gray;
	struct sensor_regset effect_sepia;
	struct sensor_regset fps_auto;
	struct sensor_regset fps_7;
	struct sensor_regset fps_15;
	struct sensor_regset fps_25;
	struct sensor_regset wb_auto;
	struct sensor_regset brightness_default;
	struct sensor_regset preview;
	struct sensor_regset stop_stream;
	struct sensor_regset start_stream;
	struct sensor_regset resol_352_288;
	struct sensor_regset resol_640_480;
	struct sensor_regset resol_800_600;
	struct sensor_regset resol_1280_960;
	struct sensor_regset resol_1600_1200;
	struct sensor_regset capture_mode;
};

struct sr200_framesize {
	s32 index;
	u32 width;
	u32 height;
};

struct sr200_fps {
	u32 index;
	u32 fps;
};

struct sr200_exif {
	u32 exp_time_den;
	u16 iso;
	u16 flash;
};

enum sr200_fps_index {
	I_FPS_0,
	I_FPS_7,
	I_FPS_10,
	I_FPS_12,
	I_FPS_15,
	I_FPS_20,
	I_FPS_25,
	I_FPS_MAX,
};

/* Preview Size List: refer to the belows.
	PREVIEW_SZ_QCIF = 0 :	176x144
	PREVIEW_SZ_320x240 :	320x240
	PREVIEW_SZ_CIF :		352x288
	PREVIEW_SZ_528x432 :	528x432
	PREVIEW_SZ_640x360 :	640x360, 16:9
	PREVIEW_SZ_VGA :		640x480
	PREVIEW_SZ_D1 :			720x480
	PREVIEW_SZ_880x720 :	880x720
	PREVIEW_SZ_SVGA :		800x600
	PREVIEW_SZ_1024x576 :	1024x576, 16:9
	PREVIEW_SZ_1024x616 :	1024x616
	PREVIEW_SZ_XGA :		1024x768
	PREVIEW_SZ_PVGA :		1280x720
	PREVIEW_SZ_SXGA :		1280x1024
*/
enum sr200_preview_frame_size {
	PREVIEW_SZ_CIF=0,		/* 352x288 */
	PREVIEW_SZ_VGA,			/* 640x480 : Video mode */
	PREVIEW_SZ_SVGA,		/* 800x600 : Capture mode */
	PREVIEW_SZ_MAX,
};

/* Capture Size List: Capture size is defined as below.
 *
 *	CAPTURE_SZ_VGA:	640x480
 *	CAPTURE_SZ_WVGA:	800x480
 *	CAPTURE_SZ_SVGA:	800x600
 *	CAPTURE_SZ_WSVGA:	1024x600
 *	CAPTURE_SZ_1MP:	1280x960
 *	CAPTURE_SZ_W1MP:	1600x960
 *	CAPTURE_SZ_2MP:	UXGA - 1600x1200
 *	CAPTURE_SZ_3MP:	QXGA  - 2048x1536
 *	CAPTURE_SZ_W4MP:	WQXGA - 2560x1536
 *	CAPTURE_SZ_5MP:	2560x1920
 */
enum sr200_capture_frame_size {
	CAPTURE_SZ_VGA = 0,	/* 640x480 */
	CAPTURE_SZ_1MP,		/* 1280x960 */
	CAPTURE_SZ_2MP,		/* UXGA - 1600x1200 */
	CAPTURE_SZ_MAX,
};

enum sr200_oprmode {
	SR200_OPRMODE_VIDEO = 0,
	SR200_OPRMODE_IMAGE = 1,
	SR200_OPRMODE_MAX   = 2,
};

enum sr200_hw_power {
	SR200_HW_POWER_OFF,
	SR200_HW_POWER_ON,
};

enum runmode {
	RUNMODE_NOTREADY,
	RUNMODE_INIT,
	/*RUNMODE_IDLE,*/
	RUNMODE_RUNNING,	/* previewing */
	RUNMODE_RUNNING_STOP,
	RUNMODE_CAPTURING,
	RUNMODE_CAPTURING_STOP,
	RUNMODE_RECORDING,	/* camcorder mode */
	RUNMODE_RECORDING_STOP,
};

/* SR200 default format (codes, sizes, preset values) */
static struct v4l2_mbus_framefmt default_fmt[SR200_OPRMODE_MAX] = {
	[SR200_OPRMODE_VIDEO] = {
		.width		= 640,
		.height		= 480,
		.code		= V4L2_MBUS_FMT_YUYV8_2X8,
		.field		= V4L2_FIELD_NONE,
		.colorspace	= V4L2_COLORSPACE_JPEG,
	},
	[SR200_OPRMODE_IMAGE] = {
		.width		= 1280,
		.height		= 960,
		.code		= V4L2_MBUS_FMT_JPEG_1X8,
		.field		= V4L2_FIELD_NONE,
		.colorspace	= V4L2_COLORSPACE_JPEG,
	},
};
#define SIZE_DEFAULT_FFMT	ARRAY_SIZE(default_fmt)

struct sr200_resolution {
	u8			value;
	enum sr200_oprmode	type;
	u16			width;
	u16			height;
};

/* make look-up table */
static const struct sr200_resolution sr200_resolutions[] = {
	/* monitor size */
	{PREVIEW_SZ_CIF		, SR200_OPRMODE_VIDEO, 352, 288},
	{PREVIEW_SZ_VGA		, SR200_OPRMODE_VIDEO, 640, 480},
	{PREVIEW_SZ_SVGA	, SR200_OPRMODE_VIDEO, 800, 600},

	/* capture(JPEG or Bayer RAW or YUV Raw) size */
	{CAPTURE_SZ_VGA		, SR200_OPRMODE_IMAGE, 640, 480},
	{CAPTURE_SZ_1MP		, SR200_OPRMODE_IMAGE, 1280, 960},
	{CAPTURE_SZ_2MP		, SR200_OPRMODE_IMAGE, 1600, 1200},
};

struct sr200_state {
	struct fimc_is_image	image;

	u16			vis_duration;
	u16			frame_length_line;
	u32			line_length_pck;
	u32			system_clock;

	u32			mode;
	u32			contrast;
	u32			effect;
	u32			ev;
	u32			flash_mode;
	u32			focus_mode;
	u32			iso;
	u32			metering;
	u32			saturation;
	u32			scene_mode;
	u32			sharpness;
	u32			white_balance;
	u32			fps;
	u32			aeawb_lockunlock;
	u32			zoom_ratio;

	u32			initialized:1;
	u32			recording:1;
	s32			vt_mode;
	s32			req_fps;
	u32			res_type;
	u32			resolution;
	bool			power_on;

	enum v4l2_pix_format_mode	format_mode;
	enum v4l2_sensor_mode		sensor_mode;
	enum runmode				runmode;
	enum sr200_oprmode			oprmode;

	struct sr200_exif				exif;
	struct v4l2_pix_format			req_fmt;
	struct sr200_preview			preview;
	struct sr200_capture			capture;
	struct v4l2_mbus_framefmt		ffmt[2]; /* for media deivce fmt */
	enum v4l2_mbus_pixelcode		code; /* for media deivce code */
};

int sensor_sr200_probe(struct i2c_client *client,
	const struct i2c_device_id *id);

#endif
