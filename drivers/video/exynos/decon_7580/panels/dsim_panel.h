/* linux/drivers/video/exynos_decon/panel/dsim_panel.h
 *
 * Header file for Samsung MIPI-DSI LCD Panel driver.
 *
 * Copyright (c) 2013 Samsung Electronics
 * Minwoo Kim <minwoo7945.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __DSIM_PANEL__
#define __DSIM_PANEL__


extern unsigned int lcdtype;
#if defined(CONFIG_PANEL_EA8064G_DYNAMIC)
extern struct mipi_dsim_lcd_driver ea8064g_mipi_lcd_driver;
extern struct dsim_panel_ops ea8064g_panel_ops;
#elif defined(CONFIG_PANEL_LTM184HL01)
extern struct mipi_dsim_lcd_driver ltm184hl01_mipi_lcd_driver;
extern struct dsim_panel_ops ltm184hl01_panel_ops;
#elif defined(CONFIG_PANEL_S6E3AA2_A3XE)
extern struct mipi_dsim_lcd_driver s6e3aa2_mipi_lcd_driver;
extern struct dsim_panel_ops s6e3aa2_panel_ops;
#elif defined(CONFIG_PANEL_S6E3FA3_A5XE)
extern struct mipi_dsim_lcd_driver s6e3fa3_mipi_lcd_driver;
extern struct dsim_panel_ops s6e3fa3_panel_ops;
#elif defined(CONFIG_PANEL_S6E3FA3_A7XE)
extern struct mipi_dsim_lcd_driver s6e3fa3_mipi_lcd_driver;
extern struct dsim_panel_ops s6e3fa3_panel_ops;
#elif defined(CONFIG_PANEL_EA8061_DYNAMIC)
extern struct mipi_dsim_lcd_driver ea8061_mipi_lcd_driver;
extern struct dsim_panel_ops ea8061_panel_ops;
#endif

int dsim_panel_ops_init(struct dsim_device *dsim);

/* dsim_panel_get_priv_ops() this function comes from XXXX_mipi_lcd.c */
struct dsim_panel_ops *dsim_panel_get_priv_ops(struct dsim_device *dsim);


#endif
