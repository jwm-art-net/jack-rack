/*
 *   JACK Rack
 *    
 *   Copyright (C) Robert Ham 2003 (node@users.sourceforge.net)
 *    
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __JR_PLUGIN_SETTINGS_H__
#define __JR_PLUGIN_SETTINGS_H__

#include <glib.h>
#include <jack/jack.h>
#include <ladspa.h>

#include "plugin_mgr.h"
#include "plugin_desc.h"

typedef struct _settings settings_t;

struct _settings
{
  jack_nframes_t   sample_rate;
  plugin_desc_t *  desc;
  guint            copies;
  LADSPA_Data **   control_values;
  gboolean *       locks;
  gboolean         lock_all;
  gboolean         enabled;
};

settings_t * settings_new     (plugin_desc_t * desc, guint copies, jack_nframes_t sample_rate);
settings_t * settings_dup     (settings_t * settings);
void         settings_destroy (settings_t * settings);

void settings_set_control_value (settings_t * settings, guint copy, unsigned long control_index, LADSPA_Data value);
void settings_set_lock          (settings_t * settings, unsigned long control_index, gboolean locked);
void settings_set_lock_all      (settings_t * settings, gboolean lock_all);
void settings_set_enabled       (settings_t * settings, gboolean enabled);

LADSPA_Data settings_get_control_value (settings_t * settings, guint copy, unsigned long control_index);
gboolean    settings_get_lock          (const settings_t * settings, unsigned long control_index);
gboolean    settings_get_lock_all      (const settings_t * settings);
gboolean    settings_get_enabled       (const settings_t * settings);
guint       settings_get_copies        (const settings_t * settings);

void settings_set_sample_rate (settings_t * settings, jack_nframes_t sample_rate);

#endif /* __JR_PLUGIN_SETTINGS_H__ */
