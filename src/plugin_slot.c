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

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>

#include <gtk/gtk.h>
#include <ladspa.h>

#include "plugin_slot.h"
#include "callbacks.h"
#include "plugin_mgr.h"
#include "jack_rack.h"
#include "process.h"
#include "ui.h"
#include "globals.h"

#define TEXT_BOX_WIDTH        75
#define CONTROL_FIFO_SIZE     256
#define TEXT_BOX_CHARS        -1

void
plugin_slot_show_controls (plugin_slot_t * plugin_slot, guint copy_to_show)
{
  unsigned long control;
  guint copy;
  port_controls_t * port_controls;
  gboolean lock_all;
  
  
  if (plugin_slot->plugin->desc->control_port_count == 0
      || plugin_slot->plugin->copies < 2)
    return;

  if (copy_to_show >= plugin_slot->plugin->copies)
    copy_to_show = 0;
  
  lock_all = settings_get_lock_all (plugin_slot->settings);
  
  for (control = 0; control < plugin_slot->plugin->desc->control_port_count; control++)
    {
      port_controls = plugin_slot->port_controls + control;

      for (copy = 0; copy < plugin_slot->plugin->copies; copy++)
        {
          if ((copy != copy_to_show) && (lock_all || port_controls->locked))
            {
              gtk_widget_hide (port_controls->controls[copy].control);
              if (port_controls->type == JR_CTRL_FLOAT)
                {
                  gtk_widget_hide (port_controls->controls[copy].text);
                }
            }
          else
            {
              gtk_widget_show (port_controls->controls[copy].control);
              if (port_controls->type == JR_CTRL_FLOAT)
                {
                  gtk_widget_show (port_controls->controls[copy].text);
                }
            }
        }
      
      if (lock_all)
        gtk_widget_hide (port_controls->lock);
      else
        gtk_widget_show (port_controls->lock);
    }
}

static void
plugin_slot_set_controls (plugin_slot_t * plugin_slot, settings_t * settings)
{
  unsigned long control;
  LADSPA_Data value;
  guint copies, copy;
  plugin_t * plugin;
  port_controls_t * port_controls;
  plugin_desc_t * desc;
  
  plugin = plugin_slot->plugin;
  desc = plugin->desc;
  copies = plugin->copies;
  
  if (desc->control_port_count == 0)
    return;
  
  if (copies > 1)
    {
      gboolean lock_all;
      gboolean lock;
      
      lock_all = settings_get_lock_all (settings);
      
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(plugin_slot->lock_all), lock_all);
      
      for (control = 0; control < desc->control_port_count; control++)
        {
          lock = settings_get_lock (settings, control);
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(plugin_slot->port_controls[control].lock), lock);
          plugin_slot->port_controls[control].locked = lock_all ? TRUE : lock;
        }
    }
  
  for (control = 0; control < desc->control_port_count; control++)
    {
      port_controls = plugin_slot->port_controls + control;
      for (copy = 0; copy < copies; copy++)
        {
          value = settings_get_control_value (settings, copy, control);
          switch (port_controls->type)
            {
            case JR_CTRL_FLOAT:
              {
                gchar *str;
                gtk_range_set_value (GTK_RANGE (port_controls->controls[copy].control),
                                     value);
              
                str = g_strdup_printf ("%f", value);
                gtk_entry_set_text (GTK_ENTRY (port_controls->controls[copy].text), str);
                g_free (str);
                break;
              }

            case JR_CTRL_INT:
              gtk_spin_button_set_value (GTK_SPIN_BUTTON (port_controls->controls[copy].control),
                                         value);
              break;

            case JR_CTRL_BOOL:
              gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (port_controls->controls[copy].control),
                                            value > 0.0 ? TRUE : FALSE);
              break;
            }
        }
    }
}

static void
plugin_slot_create_lock_all_button (plugin_slot_t * plugin_slot)
{
  plugin_slot->lock_all = gtk_toggle_button_new_with_label (_("Lock All"));
  g_signal_connect (G_OBJECT (plugin_slot->lock_all), "toggled",
                    G_CALLBACK (slot_lock_all_cb), plugin_slot);
  gtk_widget_show (plugin_slot->lock_all);
  gtk_box_pack_end (GTK_BOX (plugin_slot->top_controls),
                    plugin_slot->lock_all, FALSE, FALSE, 0);
}

static void
plugin_slot_create_control_table (plugin_slot_t * plugin_slot)
{
  if (plugin_slot->plugin->desc->control_port_count > 0)
    {
      guint copies = plugin_slot->plugin->copies;
      plugin_slot->control_table =
        gtk_table_new (plugin_slot->plugin->desc->control_port_count,
                       (copies > 1) ? 3 : 2,
                       FALSE);

      gtk_widget_show (plugin_slot->control_table);
      gtk_box_pack_start (GTK_BOX (plugin_slot->main_vbox),
                          plugin_slot->control_table, FALSE, FALSE, 0);

      /* fill the control table and create the port controls */
      plugin_slot->port_controls = port_controls_new (plugin_slot);
    }
  else
    {
      plugin_slot->port_controls = NULL;
      plugin_slot->control_table = NULL;
    }
}

static void
plugin_slot_init_gui (plugin_slot_t * plugin_slot)
{
  gchar *str;
  GtkWidget * slot_up; /* button */
  GtkWidget * slot_up_image; /* button */
  GtkWidget * slot_remove; /* button */
  GtkWidget * slot_remove_image; /* button */
  GtkWidget * slot_down; /* button */
  GtkWidget * slot_down_image; /* button */

  /* main vbox */
  plugin_slot->main_vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (plugin_slot->main_vbox);

  /* top control box */
  plugin_slot->top_controls = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (plugin_slot->top_controls);
  gtk_box_pack_start (GTK_BOX (plugin_slot->main_vbox),
                      plugin_slot->top_controls, FALSE, FALSE, 0);

  /* plugin selector menu thingy */
  plugin_slot->plugin_selector = gtk_button_new_with_label (plugin_slot->plugin->desc->name);
  gtk_widget_show (plugin_slot->plugin_selector);
  plugin_slot->plugin_menu = plugin_mgr_get_menu (plugin_slot->jack_rack->ui->plugin_mgr,
                                     G_CALLBACK (slot_change_cb), plugin_slot);
  g_signal_connect_swapped (G_OBJECT (plugin_slot->plugin_selector), "event",
                            G_CALLBACK (plugin_button_cb),
                            G_OBJECT (plugin_slot->plugin_menu));
  gtk_box_pack_start (GTK_BOX (plugin_slot->top_controls),
                      plugin_slot->plugin_selector, FALSE, FALSE, 0);

  /* slot up */
  slot_up = gtk_button_new ();
  gtk_widget_show (slot_up);
  str = g_strconcat (PKGDATADIR, "/jr-up-arrow.png", NULL);
  slot_up_image = gtk_image_new_from_file (str);
  g_free (str);
  g_object_set_data (G_OBJECT (slot_up), "jack-rack-up",
                     GINT_TO_POINTER (1));
  gtk_widget_show (slot_up_image);
  gtk_container_add (GTK_CONTAINER (slot_up),
                     slot_up_image);
  g_signal_connect (G_OBJECT (slot_up), "clicked",
                    G_CALLBACK (slot_move_cb), plugin_slot);
  gtk_box_pack_start (GTK_BOX (plugin_slot->top_controls),
                      slot_up, FALSE, FALSE, 0);

  /* slot remove */
  slot_remove = gtk_button_new ();
  gtk_widget_show (slot_remove);
  str = g_strconcat (PKGDATADIR, "/jr-cross.png", NULL);
  slot_remove_image = gtk_image_new_from_file (str);
  g_free (str);
  gtk_widget_show (slot_remove_image);
  gtk_container_add (GTK_CONTAINER (slot_remove),
                     slot_remove_image);
  g_signal_connect (G_OBJECT (slot_remove), "clicked",
                    G_CALLBACK (slot_remove_cb), plugin_slot);
  gtk_box_pack_start (GTK_BOX (plugin_slot->top_controls),
                      slot_remove, FALSE, FALSE, 0);

  /* slot down */
  slot_down = gtk_button_new ();
  gtk_widget_show (slot_down);
  str = g_strconcat (PKGDATADIR, "/jr-down-arrow.png", NULL);
  slot_down_image = gtk_image_new_from_file (str);
  g_free (str);
  g_object_set_data (G_OBJECT (slot_down), "jack-rack-up",
                     GINT_TO_POINTER (0));
  gtk_widget_show (slot_down_image);
  gtk_container_add (GTK_CONTAINER (slot_down),
                     slot_down_image);
  g_signal_connect (G_OBJECT (slot_down), "clicked",
                    G_CALLBACK (slot_move_cb), plugin_slot);
  gtk_box_pack_start (GTK_BOX (plugin_slot->top_controls),
                      slot_down, FALSE, FALSE, 0);

  /* enable button */
  plugin_slot->enable = gtk_toggle_button_new_with_label (_("Enable"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin_slot->enable),
                                FALSE);
  g_signal_connect (G_OBJECT (plugin_slot->enable), "button-press-event",
                    G_CALLBACK (slot_ablise_cb), plugin_slot);
  gtk_widget_show (plugin_slot->enable);
  gtk_box_pack_start (GTK_BOX (plugin_slot->top_controls),
                      plugin_slot->enable, FALSE, FALSE, 0);



  /* sort out the port controls */
  if (plugin_slot->plugin->copies > 1
      && plugin_slot->plugin->desc->control_port_count > 0)
    plugin_slot_create_lock_all_button (plugin_slot);
  else
    plugin_slot->lock_all = NULL;

  /* control table */
  plugin_slot_create_control_table (plugin_slot);

   
  /* final seperator bar */
  plugin_slot->separator = gtk_hseparator_new ();
  gtk_widget_show (plugin_slot->separator);
  gtk_box_pack_start (GTK_BOX (plugin_slot->main_vbox),
                      plugin_slot->separator, FALSE, FALSE, 4);
}

plugin_slot_t *
plugin_slot_new     (jack_rack_t * jack_rack, plugin_t * plugin, settings_t * saved_settings)
{
  plugin_slot_t * plugin_slot;
  
  plugin_slot = g_malloc (sizeof (plugin_slot_t));

  plugin_slot->jack_rack = jack_rack;
  plugin_slot->plugin = plugin;
  plugin_slot->enabled = FALSE;

  /* create plugin settings */
  plugin_slot->settings = saved_settings ? settings_dup (saved_settings)
                                         : settings_new (plugin->desc, plugin->copies, sample_rate);

  /* create the gui */
  plugin_slot_init_gui (plugin_slot);
  
  /* set the controls */
  plugin_slot_set_controls (plugin_slot, plugin_slot->settings);
  
  plugin_slot_show_controls (plugin_slot, 0);
  
  return plugin_slot;
}

void
plugin_slot_destroy (plugin_slot_t * plugin_slot)
{
  gtk_widget_destroy (plugin_slot->plugin_menu);
  gtk_widget_destroy (plugin_slot->main_vbox);
  g_free (plugin_slot->port_controls);
}

void
plugin_slot_change_plugin (plugin_slot_t * plugin_slot, plugin_t * plugin)
{
  /* sort out the lock all button */
  if (plugin->copies > 1 &&
      plugin->desc->control_port_count > 0 && !plugin_slot->lock_all)
    {
      plugin_slot_create_lock_all_button (plugin_slot);
    }
    
  if ((plugin->copies == 1 && plugin_slot->lock_all) ||
      (plugin->copies > 1 && plugin_slot->lock_all &&
       plugin->desc->control_port_count == 0))
    {
      gtk_widget_destroy (plugin_slot->lock_all);
      plugin_slot->lock_all = NULL;
    }



  /* kill all the control stuff */
  if (plugin_slot->control_table)
    {
      gtk_widget_destroy (plugin_slot->control_table);
      g_free (plugin_slot->port_controls);
      plugin_slot->control_table = NULL;
    }

  gtk_button_set_label (GTK_BUTTON (plugin_slot->plugin_selector), plugin->desc->name);

  plugin_slot->plugin = plugin;
  plugin_slot->enabled = FALSE;
  settings_destroy (plugin_slot->settings);
  plugin_slot->settings = settings_new (plugin->desc, plugin->copies, sample_rate);
  
  /* create the new port controls */
  if (plugin->desc->control_port_count > 0)
    {
      plugin_slot_create_control_table (plugin_slot);
      plugin_slot_set_controls (plugin_slot, plugin_slot->settings);
    }

  /* move the separator */
  gtk_box_reorder_child (GTK_BOX (plugin_slot->main_vbox),
                         plugin_slot->separator, plugin->desc->control_port_count + 1);
                             
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin_slot->enable),
                                FALSE);
}

/* EOF */


