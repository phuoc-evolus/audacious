/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  Audacious development team
 *
 *  Based on BMP:
 *  Copyright (C) 2003-2004  BMP development team
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */
#ifndef VISUALIZATION_H
#define VISUALIZATION_H

#include <glib.h>

#include "plugin.h"

typedef struct _VisPluginData VisPluginData;

struct _VisPluginData {
    GList *vis_list;
    GList *enabled_list;
    gboolean playback_started;
};

GList *get_vis_list(void);
GList *get_vis_enabled_list(void);
void enable_vis_plugin(gint i, gboolean enable);
void vis_disable_plugin(VisPlugin * vp);
void vis_about(gint i);
void vis_configure(gint i);
void vis_playback_start(void);
void vis_playback_stop(void);
gboolean vis_enabled(gint i);
gchar *vis_stringify_enabled_list(void);
void vis_enable_from_stringified_list(gchar * list);
void vis_send_data(gint16 pcm_data[2][512], gint nch, gint length);

extern VisPluginData vp_data;

#endif
