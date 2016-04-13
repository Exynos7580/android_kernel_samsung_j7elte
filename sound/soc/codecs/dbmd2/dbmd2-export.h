/*
 * dbmd2-export.h  --  DBMD2 exported interface
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DBMD2_EXPORT_H
#define _DBMD2_EXPORT_H

#include <sound/soc.h>

int dbmd2_remote_add_codec_controls(struct snd_soc_codec *codec);

typedef void (*event_cb)(int);
void dbmd2_remote_register_event_callback(event_cb func);

#endif
