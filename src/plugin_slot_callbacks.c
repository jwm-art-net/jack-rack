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
 
#define _GNU_SOURCE

#include "ac_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <ladspa.h>

#ifdef HAVE_GNOME
#include <libgnomeui/libgnomeui.h>
#endif

#include "jack_rack.h"
#include "lock_free_fifo.h"
#include "plugin_slot_callbacks.h"
#include "control_message.h"
#include "globals.h"
#include "file.h"
#include "ui.h"
#include "wet_dry_controls.h"
#include "midi_control.h"

void
slot_change_cb (GtkMenuItem * menuitem, gpointer user_data)
{
  plugin_slot_t * plugin_slot;
  plugin_desc_t * desc;
  
  plugin_slot = user_data;
  
  desc = (plugin_desc_t *) g_object_get_data (G_OBJECT(menuitem), "jack-rack-plugin-desc");

  plugin_slot_send_change_plugin (plugin_slot, desc);
}

void
slot_move_cb (GtkButton * button, gpointer user_data)
{
  plugin_slot_t * plugin_slot;
  gboolean up;

  plugin_slot = user_data;
  up = GPOINTER_TO_INT (g_object_get_data (G_OBJECT(button), "jack-rack-up"));
  
  jack_rack_send_move_plugin_slot (plugin_slot->jack_rack, plugin_slot, up);
}

void
slot_remove_cb (GtkButton * button, gpointer user_data)
{
  plugin_slot_t * plugin_slot;
  
  plugin_slot = (plugin_slot_t *) user_data;
  
  jack_rack_send_remove_plugin_slot (plugin_slot->jack_rack, plugin_slot);
}


gboolean
slot_ablise_cb (GtkWidget * button, GdkEventButton *event, gpointer user_data)
{
  if (event->type == GDK_BUTTON_PRESS) {
    plugin_slot_t * plugin_slot; 
    gboolean ablise;
  
    plugin_slot = (plugin_slot_t *) user_data;
    ablise = settings_get_enabled (plugin_slot->settings) ? FALSE : TRUE;
  
    plugin_slot_send_ablise (plugin_slot, ablise);
  
    return TRUE;
  }
  
  return FALSE;
}

gboolean
slot_wet_dry_cb (GtkWidget * button, GdkEventButton *event, gpointer user_data)
{
  if (event->type == GDK_BUTTON_PRESS)
    {
      plugin_slot_t * plugin_slot; 
      gboolean ablise;
    
      plugin_slot = (plugin_slot_t *) user_data;
      ablise = settings_get_wet_dry_enabled (plugin_slot->settings) ? FALSE : TRUE;
  
      plugin_slot_send_ablise_wet_dry (plugin_slot, ablise);
  
      return TRUE;
  }
  
  return FALSE;
}
    	                              

void
slot_wet_dry_control_cb (GtkRange * range, gpointer user_data)
{
  LADSPA_Data value;
  wet_dry_controls_t * wet_dry;
  unsigned long channel;
  
  wet_dry = (wet_dry_controls_t *) user_data;
  channel = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(range), "jack-rack-wet-dry-channel"));
  value = gtk_range_get_value (range);

  lff_write (wet_dry->fifos + channel, &value);
  
  settings_set_wet_dry_value (wet_dry->plugin_slot->settings, channel, value);
  
  /* set peers */
  if (settings_get_wet_dry_locked (wet_dry->plugin_slot->settings))
    {
      unsigned long c;
      for (c = 0; c < wet_dry->plugin_slot->jack_rack->channels ; c++)
        if (c != channel)
          gtk_range_set_value (GTK_RANGE(wet_dry->controls[c]),
                               gtk_range_get_value (range));
    }
}

void
slot_wet_dry_lock_cb    (GtkToggleButton * button, gpointer user_data)
{
  wet_dry_controls_t * wet_dry;
  
  wet_dry = (wet_dry_controls_t *) user_data;
  
  plugin_slot_set_wet_dry_locked (wet_dry->plugin_slot,
                                  gtk_toggle_button_get_active (button));
}

void
slot_lock_all_cb (GtkToggleButton * button, gpointer user_data)
{
  gboolean lock_all;
  plugin_slot_t * plugin_slot;
  guint lock_copy;
  
  lock_all = gtk_toggle_button_get_active (button);
  plugin_slot = (plugin_slot_t *) user_data;
  lock_copy = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(button), "jack-rack-lock-copy"));

  plugin_slot_set_lock_all (plugin_slot, lock_all, lock_copy);
  
  g_object_set_data (G_OBJECT(button), "jack-rack-lock-copy", GINT_TO_POINTER (0));
}

/* EOF */


