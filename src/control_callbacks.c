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

#include "jack_rack.h"
#include "lock_free_fifo.h"
#include "control_callbacks.h"
#include "control_message.h"
#include "globals.h"
#include "file.h"
#include "ui.h"
#include "wet_dry_controls.h"

/* lock a specific control with a click+ctrl */
gboolean
control_button_press_cb (GtkWidget * widget, GdkEventButton * event, gpointer user_data)
{
  port_controls_t * port_controls;
  guint copy;
  

  port_controls = (port_controls_t *) user_data;
  copy = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(widget), "jack-rack-plugin-copy"));
  
  if (event->state & GDK_CONTROL_MASK)
    {
      switch (event->button)
        {
        /* lock the control row */
        case 1:
          if (port_controls->locked)
            return FALSE;
            
          port_controls->lock_copy = copy;
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (port_controls->lock), TRUE);
          return TRUE;
          
        /* lock all */
        case 3:
          if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (port_controls->plugin_slot->lock_all)))
            return FALSE;
        
          g_object_set_data (G_OBJECT(port_controls->plugin_slot->lock_all),
                             "jack-rack-lock-copy", GINT_TO_POINTER (copy));
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (port_controls->plugin_slot->lock_all), TRUE);
          return TRUE;
        
        default:
          break;
        }
    }
    
#ifdef HAVE_ALSA
  else if (event->button == 3)
    {
      /* right click; display midi menu */
      ui_t * ui;
      
      ui = port_controls->plugin_slot->jack_rack->ui;
      
      g_object_set_data (G_OBJECT (ui->midi_menu_item), "jack-rack-port-control", GINT_TO_POINTER (TRUE));
      g_object_set_data (G_OBJECT (ui->midi_menu_item), "jack-rack-port-controls", port_controls);
      g_object_set_data (G_OBJECT (ui->midi_menu_item), "jack-rack-plugin-copy", GUINT_TO_POINTER (copy));
      gtk_menu_popup (GTK_MENU (ui->midi_menu), NULL, NULL, NULL, NULL, event->button, event->time);
        
      return TRUE;
    }
#endif /* HAVE_ALSA */
  
  return FALSE;
}

#ifdef HAVE_ALSA
gboolean
wet_dry_button_press_cb (GtkWidget * widget, GdkEventButton * event, gpointer user_data)
{
  if (event->button == 3)
    {
      /* right click; display midi menu */
      ui_t * ui;
      wet_dry_controls_t * wet_dry;
      
      wet_dry = (wet_dry_controls_t *) user_data;
      ui = wet_dry->plugin_slot->jack_rack->ui;
      
      g_object_set_data (G_OBJECT (ui->midi_menu_item), "jack-rack-port-control", GINT_TO_POINTER (FALSE));
      g_object_set_data (G_OBJECT (ui->midi_menu_item), "jack-rack-wet-dry", wet_dry);
      g_object_set_data (G_OBJECT (ui->midi_menu_item), "jack-rack-wet-dry-channel",
                         g_object_get_data (G_OBJECT(widget), "jack-rack-wet-dry-channel"));
      gtk_menu_popup (GTK_MENU (ui->midi_menu), NULL, NULL, NULL, NULL, event->button, event->time);
        
      return TRUE;
    }
  
  return FALSE;
}

void
control_add_midi_cb (GtkMenuItem * menuitem, gpointer user_data)
{
  gboolean port_control;
  plugin_slot_t * plugin_slot;
  midi_control_t * midi_ctrl;
  
  port_control = GPOINTER_TO_INT (g_object_get_data (G_OBJECT(menuitem), "jack-rack-port-control"));
  
  if (port_control)
    {
      port_controls_t * port_controls;
      guint copy;
      
      port_controls = g_object_get_data (G_OBJECT(menuitem), "jack-rack-port-controls");
      copy = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(menuitem), "jack-rack-plugin-copy"));
      plugin_slot = port_controls->plugin_slot;
      
      midi_ctrl = ladspa_midi_control_new (plugin_slot, copy, port_controls->control_index);
      midi_control_set_midi_channel (midi_ctrl, 1);
      midi_control_set_midi_param (midi_ctrl,
        jack_rack_get_next_midi_param (plugin_slot->jack_rack, 1));
      
      plugin_slot_add_midi_control (plugin_slot, midi_ctrl);
    }
  else
    {
      wet_dry_controls_t * wet_dry;
      unsigned long channel;
      
      wet_dry = g_object_get_data (G_OBJECT(menuitem), "jack-rack-wet-dry");
      channel = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(menuitem), "jack-rack-wet-dry-channel"));
      plugin_slot = wet_dry->plugin_slot;
      
      midi_ctrl = wet_dry_midi_control_new (plugin_slot, channel);
      midi_control_set_midi_channel (midi_ctrl, 1);
      midi_control_set_midi_param (midi_ctrl,
        jack_rack_get_next_midi_param (plugin_slot->jack_rack, 1));
      
      plugin_slot_add_midi_control (plugin_slot, midi_ctrl);
    }
}
#endif /* HAVE_ALSA */

static void
gtk_widget_set_text_colour (GtkWidget * widget, const char * colour_str)
{
  GdkColor colour;
  GdkColormap * colourmap;
                                                                                                               
  colourmap = gtk_widget_get_colormap (widget);
                                                                                                               
  gdk_color_parse (colour_str, &colour);
  gdk_colormap_alloc_color (colourmap, &colour, FALSE, TRUE);
  gtk_widget_modify_text (widget, GTK_STATE_NORMAL, &colour);
}


void control_float_cb (GtkRange * range, gpointer user_data) {
  port_controls_t * port_controls;
  LADSPA_Data value;
  guint copy;
  gchar *str;

  
  port_controls = (port_controls_t *) user_data;
  value = gtk_range_get_value (range);
  if (port_controls->logarithmic)
      value = exp (value);
  copy = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(range), "jack-rack-plugin-copy"));
    
  port_control_send_value (port_controls, copy, value);
  
  /* print the value in the text box */
  str = g_strdup_printf ("%f", value);
  gtk_entry_set_text (GTK_ENTRY(port_controls->controls[copy].text), str);
  g_free (str);
  
  /* possibly set our peers */
  if (port_controls->plugin_slot->plugin->copies > 1
      && port_controls->locked)
    {
      guint i;
      for (i = 0; i < port_controls->plugin_slot->plugin->copies; i++)
        {
          if (i != copy)
            gtk_range_set_value (GTK_RANGE(port_controls->controls[i].control),
                                 gtk_range_get_value (range));
        }
    }
  
  gtk_widget_set_text_colour (port_controls->controls[copy].text, "Black");
}

void control_float_text_cb (GtkEntry * entry, gpointer user_data) {
  port_controls_t * port_controls;
  LADSPA_Data value;
  const char * str;
  char * endptr;
  GtkAdjustment * adjustment;
  guint copy;

  port_controls = (port_controls_t *) user_data;
  copy = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(entry), "jack-rack-plugin-copy"));
  adjustment = gtk_range_get_adjustment (GTK_RANGE (port_controls->controls[copy].control));

  str = gtk_entry_get_text (entry);
  value = strtof (str, &endptr);
  
  /* check for errors */
  if (value == HUGE_VALF ||
      value == -HUGE_VALF ||
      endptr == str ||
      value < (port_controls->logarithmic ? exp (adjustment->lower) : adjustment->lower) ||
      value > (port_controls->logarithmic ? exp (adjustment->upper) : adjustment->upper))
    {
      /* set the entry's text colour */
      gtk_widget_set_text_colour (GTK_WIDGET(entry), "DarkRed");
      return;
    }
  
  
  /* set the value in range */
  gtk_range_set_value (GTK_RANGE(port_controls->controls[copy].control),
                       port_controls->logarithmic ? log (value) : value);
    
  /* no need to set peers as the set_value above will do it */
  gtk_widget_set_text_colour (GTK_WIDGET(entry), "Black");
}


void control_bool_cb (GtkToggleButton * button, gpointer user_data) {
  port_controls_t * port_controls;
  gboolean on;
  LADSPA_Data data;
  int copy;
  
  port_controls = (port_controls_t *) user_data;
  
  on = gtk_toggle_button_get_active (button);
  data = on ? 1.0 : -1.0;

  /* get which channel we're using from the g object data stuff */
  copy = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(button), "jack-rack-plugin-copy"));
  
  port_control_send_value (port_controls, copy, data);
  
  /* possibly set other controls */
  if (port_controls->plugin_slot->plugin->copies > 1 && port_controls->locked)
    {
      guint i;
      for (i = 0; i < port_controls->plugin_slot->plugin->copies; i++)
        if (i != copy)
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(port_controls->controls[i].control), on);
    }
}


void control_int_cb (GtkSpinButton * spinbutton, gpointer user_data) {
  port_controls_t * port_controls;
  int value;
  LADSPA_Data data;
  int copy;
  
  port_controls = (port_controls_t *) user_data;
  
  value = gtk_spin_button_get_value_as_int (spinbutton);
  data = value;
  
  /* get which channel we're using from the g object data stuff */
  copy = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(spinbutton), "jack-rack-plugin-copy"));
  
  port_control_send_value (port_controls, copy, data);
      
  /* possibly set other controls */
  if (port_controls->plugin_slot->plugin->copies > 1 && port_controls->locked)
    {
      guint i;
      for (i = 0; i < port_controls->plugin_slot->plugin->copies; i++)
        if (i != copy)
          gtk_spin_button_set_value (GTK_SPIN_BUTTON (port_controls->controls[copy].control), value);
    }
}

void control_lock_cb (GtkToggleButton * button, gpointer user_data) {
  port_controls_t * port_controls;
  gboolean locked;
                                                                                                               
  port_controls = (port_controls_t *) user_data;
  locked = gtk_toggle_button_get_active (button);

  port_control_set_locked (port_controls, locked);
                                                                                                               
  port_controls->lock_copy = 0;
}


/* EOF */

