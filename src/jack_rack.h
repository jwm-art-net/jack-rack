/*
 *   JACK Rack
 *    
 *   Copyright (C) Robert Ham 2002, 2003 (node@users.sourceforge.net)
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

#ifndef __JR_JACK_RACK_H__
#define __JR_JACK_RACK_H__

#include "ac_config.h"

#include <gtk/gtk.h>
#include <ladspa.h>

#include "plugin.h"
#include "plugin_mgr.h"
#include "plugin_slot.h"

typedef struct _jack_rack jack_rack_t;

struct _jack_rack
{
  struct _ui *      ui;
  
  unsigned long     channels;
  GList *           slots;
  
  GSList *          saved_settings;
};

jack_rack_t * jack_rack_new     (struct _ui * ui, unsigned long channels);
void          jack_rack_destroy (jack_rack_t * jack_rack);

void jack_rack_add_plugin (jack_rack_t * jack_rack, plugin_desc_t * plugin);
void jack_rack_add_plugin_slot (jack_rack_t * jack_rack, plugin_t * plugin);
void jack_rack_remove_plugin_slot (jack_rack_t * jack_rack, plugin_slot_t * plugin_slot);
void jack_rack_move_plugin_slot (jack_rack_t * jack_rack, plugin_slot_t * plugin_slot, gint up);
void jack_rack_change_plugin_slot (jack_rack_t * jack_rack, plugin_slot_t * plugin_slot, plugin_t * plugin);
void jack_rack_clear_plugins (jack_rack_t * jack_rack, plugin_t * plugin);


plugin_slot_t * jack_rack_get_plugin_slot (jack_rack_t * jack_rack, unsigned long plugin_index);

plugin_t * jack_rack_instantiate_plugin (jack_rack_t * jack_rack, plugin_desc_t * desc);

#endif /* __JR_JACK_RACK_H__ */
