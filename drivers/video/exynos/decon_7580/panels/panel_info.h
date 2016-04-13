#ifndef __PANEL_INFO_H__
#define __PANEL_INFO_H__

#if defined(CONFIG_PANEL_S6E3FA2_DYNAMIC)
#include "s6e3fa2_param.h"
#elif defined(CONFIG_PANEL_S6E3FA3_DYNAMIC)
#include "s6e3fa3_param.h"
#elif defined(CONFIG_PANEL_S6D7AA0_DYNAMIC)
#include "s6d7aa0_param.h"
#elif defined(CONFIG_PANEL_S6D7AA0X62_DYNAMIC)
#include "s6d7aa0x62_param.h"
#include "ea8061_param.h"
extern struct dsim_panel_ops ea8061_panel_ops;
#else
#error "ERROR !! Check LCD Panel Header File"
#endif

#ifdef CONFIG_PANEL_AID_DIMMING
#include "aid_dimming.h"
#endif

#define LCD_S6E3FA2 0
#define LCD_EA8064G 1

#ifdef CONFIG_PANEL_S6D7AA0_DYNAMIC
int sky82896_array_write(const struct SKY82896_rom_data * eprom_ptr, int eprom_size);
#endif

#endif
