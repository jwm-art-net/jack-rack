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

#include "ac_config.h"
#include "plugin.h"
#include "port_controls.h"
#include "wet_dry_controls.h"
#include "plugin_settings.h"
#include "midi_control.h"

typedef struct _plugin_slot plugin_slot_t;

struct _jack_rack;
struct _saved_plugin;

/** a slot in the main gui's box of plugin controls.  this contains all
    the widgets and plugin data for the slot */
struct _plugin_slot
{
  struct _jack_rack  *jack_rack;
  wet_dry_controls_t *wet_dry_controls;
  plugin_t           *plugin;
  settings_t         *settings;
  /* port controls *array* */
  port_controls_t    *port_controls; 

#ifdef HAVE_ALSA
  GSList              *midi_controls;
#endif
  

  /* widgets */
  GtkWidget *main_vbox;
  GtkWidget *top_controls;
  GtkWidget *lock_all;
  GtkWidget *control_table;
  GtkWidget *separator;
  GtkWidget *plugin_selector;
  GtkWidget *enable;
  GtkWidget *wet_dry;
  GtkWidget *plugin_menu;
  
};


plugin_slot_t * plugin_slot_new     (struct _jack_rack *jack_rack,
                                     plugin_t *plugin,
                                     struct _saved_plugin * saved_plugin);
void            plugin_slot_destroy (plugin_slot_t *plugin_slot);

void plugin_slot_send_ablise          (plugin_slot_t *plugin_slot, gboolean enabled);
void plugin_slot_send_ablise_wet_dry  (plugin_slot_t *plugin_slot, gboolean enabled);
void plugin_slot_send_change_plugin   (plugin_slot_t *plugin_slot, plugin_desc_t *desc);
void plugin_slot_change_plugin        (plugin_slot_t *plugin_slot, plugin_t *plugin);
void plugin_slot_remove_midi_controls (plugin_slot_t *plugin_slot);

void plugin_slot_show_controls  (plugin_slot_t *plugin_slot, guint copy_to_show);
void plugin_slot_show_wet_dry_controls (plugin_slot_t *plugin_slot);

void plugin_slot_set_wet_dry_controls (plugin_slot_t *plugin_slot, gboolean block_callbacks);
void plugin_slot_set_port_controls (plugin_slot_t *plugin_slot, port_controls_t *port_controls,
                                    gboolean block_callbacks);
void plugin_slot_set_wet_dry_locked (plugin_slot_t *plugin_slot, gboolean locked);
void plugin_slot_set_lock_all (plugin_slot_t *plugin_slot, gboolean lock_all, guint lock_copy);

#ifdef HAVE_ALSA
void plugin_slot_add_midi_control    (plugin_slot_t *plugin_slot, midi_control_t *midi_ctrl);
#endif


#endif /* __JR_PLUGIN_SLOT_H__ */
