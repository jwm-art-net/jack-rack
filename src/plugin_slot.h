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

#ifndef __JR_PLUGIN_SLOT_H__
#define __JR_PLUGIN_SLOT_H__

#include <gtk/gtk.h>
#include <ladspa.h>

#include "plugin.h"
#include "port_controls.h"
#include "plugin_settings.h"

typedef struct _plugin_slot plugin_slot_t;


/** a slot in the main gui's box of plugin controls.  this contains all
    the widgets and plugin data for the slot */
struct _plugin_slot
{
  plugin_t * plugin;
  settings_t * settings;
  gint enabled;
  
  /* widgets */
  GtkWidget * main_vbox;
  GtkWidget * top_controls;
  GtkWidget * lock_all;
  GtkWidget * control_table;
  GtkWidget * separator;
  GtkWidget * plugin_selector;
  GtkWidget * enable;
  GtkWidget * plugin_menu;
/*  GtkWidget * time; */
  
  port_controls_t * port_controls; /* port controls array */
  
  struct _jack_rack * jack_rack;
};


plugin_slot_t * plugin_slot_new     (struct _jack_rack * jack_rack, plugin_t * plugin, settings_t * saved_settings);
void            plugin_slot_destroy (plugin_slot_t * plugin_slot);

void plugin_slot_change_plugin (plugin_slot_t * plugin_slot, plugin_t * plugin);
void plugin_slot_show_controls (plugin_slot_t * plugin_slot, guint copy_to_show);

#endif /* __JR_PLUGIN_SLOT_H__ */
