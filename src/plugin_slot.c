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
#include <math.h>

#include <gtk/gtk.h>
#include <ladspa.h>

#include "plugin_slot.h"
#include "plugin_slot_callbacks.h"
#include "ui_callbacks.h"
#include "plugin_mgr.h"
#include "jack_rack.h"
#include "process.h"
#include "ui.h"
#include "globals.h"
#include "control_message.h"
#include "file.h"

#define TEXT_BOX_WIDTH        75
#define CONTROL_FIFO_SIZE     256
#define TEXT_BOX_CHARS        -1

void
plugin_slot_show_wet_dry_controls (plugin_slot_t * plugin_slot)
{
  if (settings_get_wet_dry_enabled (plugin_slot->settings))
    {
      gtk_widget_show (plugin_slot->wet_dry_controls->control_box);
      
      if (plugin_slot->jack_rack->channels > 1)
        {
          unsigned long channel;
          gboolean locked;
      
          locked = settings_get_wet_dry_locked (plugin_slot->settings);
      
          if (locked)
            gtk_widget_hide (plugin_slot->wet_dry_controls->labels[0]);
          else
            gtk_widget_show (plugin_slot->wet_dry_controls->labels[0]);
      
          for (channel = 1; channel < plugin_slot->jack_rack->channels; channel++)
            if (locked)
              {
                gtk_widget_hide (plugin_slot->wet_dry_controls->controls[channel]);
                gtk_widget_hide (plugin_slot->wet_dry_controls->labels[channel]);
              }
            else
              {
                gtk_widget_show (plugin_slot->wet_dry_controls->controls[channel]);
                gtk_widget_show (plugin_slot->wet_dry_controls->labels[channel]);
              }
        }
    }
  else
    gtk_widget_hide (plugin_slot->wet_dry_controls->control_box);
}

void
plugin_slot_show_controls (plugin_slot_t * plugin_slot, guint copy_to_show)
{
  port_controls_t * port_controls;
  unsigned long control;
  guint copy;
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


void
plugin_slot_set_port_controls (plugin_slot_t *plugin_slot,
                               port_controls_t *port_controls,
                               gboolean block_callbacks)
{
  LADSPA_Data value;
  guint copy;
  for (copy = 0; copy < plugin_slot->plugin->copies; copy++)
    {
      value = settings_get_control_value (plugin_slot->settings, copy, port_controls->control_index);
      switch (port_controls->type)
        {
        case JR_CTRL_FLOAT:
          {
            gdouble logval;
            gchar *str;
            
            logval = (port_controls->logarithmic ? log (value) : value);
            if (block_callbacks)
              port_controls_block_float_callback (port_controls, copy);
            gtk_range_set_value (GTK_RANGE (port_controls->controls[copy].control),
                                 logval);
            if (block_callbacks)
              port_controls_unblock_float_callback (port_controls, copy);
              
            str = g_strdup_printf ("%f", value);
            gtk_entry_set_text (GTK_ENTRY (port_controls->controls[copy].text), str);
            g_free (str);
            break;
          }

        case JR_CTRL_INT:
          if (block_callbacks)
              port_controls_block_int_callback (port_controls, copy);
          gtk_spin_button_set_value (GTK_SPIN_BUTTON (port_controls->controls[copy].control),
                                     value);
          if (block_callbacks)
              port_controls_unblock_int_callback (port_controls, copy);
          break;

        case JR_CTRL_BOOL:
          if (block_callbacks)
              port_controls_block_bool_callback (port_controls, copy);
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (port_controls->controls[copy].control),
                                        value > 0.0 ? TRUE : FALSE);
          if (block_callbacks)
              port_controls_unblock_bool_callback (port_controls, copy);
          break;
        }
    }
}

void
plugin_slot_set_wet_dry_controls (plugin_slot_t *plugin_slot, gboolean block_callbacks)
{
  unsigned long channel;

  for (channel = 0; channel < plugin_slot->jack_rack->channels; channel++)
    {
      if (block_callbacks)
        wet_dry_controls_block_callback (plugin_slot->wet_dry_controls, channel);
      gtk_range_set_value (GTK_RANGE (plugin_slot->wet_dry_controls->controls[channel]),
                           settings_get_wet_dry_value (plugin_slot->settings, channel));
      if (block_callbacks)
        wet_dry_controls_unblock_callback (plugin_slot->wet_dry_controls, channel);
    }

  if (plugin_slot->jack_rack->channels > 1)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin_slot->wet_dry_controls->lock), 
                                  settings_get_wet_dry_locked (plugin_slot->settings));
}

static void
plugin_slot_set_controls (plugin_slot_t * plugin_slot, settings_t * settings)
{
  plugin_t * plugin;
  plugin_desc_t * desc;
  unsigned long control;
  guint copies;
  
  plugin = plugin_slot->plugin;
  desc   = plugin->desc;
  copies = plugin->copies;

  /* wet/dry controls */
  plugin_slot_set_wet_dry_controls (plugin_slot, FALSE);  
  
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
    plugin_slot_set_port_controls (plugin_slot, plugin_slot->port_controls + control, FALSE);
    
  
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


  /* wet/dry button */
  plugin_slot->wet_dry = gtk_toggle_button_new_with_label (_("Wet/Dry"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin_slot->wet_dry), FALSE);
  g_signal_connect (G_OBJECT (plugin_slot->wet_dry), "button-press-event",
                      G_CALLBACK (slot_wet_dry_cb), plugin_slot);
  gtk_widget_show (plugin_slot->wet_dry);
  gtk_box_pack_start (GTK_BOX (plugin_slot->top_controls),
                      plugin_slot->wet_dry, FALSE, FALSE, 0);
  

  /* sort out the port controls */
  if (plugin_slot->plugin->copies > 1
      && plugin_slot->plugin->desc->control_port_count > 0)
    plugin_slot_create_lock_all_button (plugin_slot);
  else
    plugin_slot->lock_all = NULL;

  /* control table */
  plugin_slot_create_control_table (plugin_slot);

  /* wet/dry controls */
  plugin_slot->wet_dry_controls = wet_dry_controls_new (plugin_slot);
  gtk_box_pack_start (GTK_BOX (plugin_slot->main_vbox),
                      plugin_slot->wet_dry_controls->control_box,
                      TRUE, FALSE, 0);
   
  /* final seperator bar */
  plugin_slot->separator = gtk_hseparator_new ();
  gtk_widget_show (plugin_slot->separator);
  gtk_box_pack_start (GTK_BOX (plugin_slot->main_vbox),
                      plugin_slot->separator, FALSE, FALSE, 4);
}

plugin_slot_t *
plugin_slot_new     (jack_rack_t * jack_rack, plugin_t * plugin, saved_plugin_t * saved_plugin)
{
  plugin_slot_t * plugin_slot;
  
  plugin_slot = g_malloc (sizeof (plugin_slot_t));

  plugin_slot->jack_rack     = jack_rack;
  plugin_slot->plugin        = plugin;
#ifdef HAVE_ALSA
  plugin_slot->midi_controls = NULL;
#endif

  /* create plugin settings */
  plugin_slot->settings = saved_plugin
                            ? saved_plugin->settings
                            : settings_new (plugin->desc, jack_rack->channels, sample_rate);

  /* create the gui */
  plugin_slot_init_gui (plugin_slot);
  
  /* set the controls */
  plugin_slot_set_controls (plugin_slot, plugin_slot->settings);
  
  plugin_slot_show_controls (plugin_slot, 0);
  plugin_slot_show_wet_dry_controls (plugin_slot);
  
  /* add the midi controls */
  if (saved_plugin)
  {
#ifdef HAVE_ALSA
    GSList *list;
    midi_control_t *midi_ctrl;
    midi_control_t *new_midi_ctrl;
    
    for (list = saved_plugin->midi_controls; list; list = g_slist_next (list))
      {
        midi_ctrl = list->data;
        
        if (midi_ctrl->ladspa_control)
          new_midi_ctrl =
            ladspa_midi_control_new (plugin_slot,
                                     midi_ctrl->control.ladspa.copy,
                                     midi_ctrl->control.ladspa.control);
        else
          new_midi_ctrl =
            wet_dry_midi_control_new (plugin_slot, midi_ctrl->control.wet_dry.channel);
        
        midi_control_set_midi_channel (new_midi_ctrl, midi_ctrl->midi_channel);
        midi_control_set_midi_param   (new_midi_ctrl, midi_ctrl->midi_param);
        
        plugin_slot_add_midi_control (plugin_slot, new_midi_ctrl);
        
        g_free (midi_ctrl);
      }
    
    g_slist_free (saved_plugin->midi_controls);
#endif /* HAVE_ALSA */

    g_free (saved_plugin);
  }
  
  return plugin_slot;
}

void
plugin_slot_destroy (plugin_slot_t * plugin_slot)
{
  gtk_widget_destroy (plugin_slot->plugin_menu);
  gtk_widget_destroy (plugin_slot->main_vbox);
  g_free (plugin_slot->port_controls);
  wet_dry_controls_destroy (plugin_slot->wet_dry_controls);
  g_free (plugin_slot);
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
  settings_destroy (plugin_slot->settings);
  plugin_slot->settings = settings_new (plugin->desc, plugin_slot->jack_rack->channels, sample_rate);
  
  /* create the new port controls */
  if (plugin->desc->control_port_count > 0)
    {
      plugin_slot_create_control_table (plugin_slot);
      plugin_slot_set_controls (plugin_slot, plugin_slot->settings);
    }
  
  plugin_slot_show_wet_dry_controls (plugin_slot);

  /* move the separator */
  gtk_box_reorder_child (GTK_BOX (plugin_slot->main_vbox),
                         plugin_slot->separator, plugin->desc->control_port_count + 1);
                             
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin_slot->enable),
                                FALSE);
}

void
plugin_slot_send_ablise        (plugin_slot_t * plugin_slot, gboolean enable)
{
  ctrlmsg_t ctrlmsg;
  
  ctrlmsg.type = CTRLMSG_ABLE;
  ctrlmsg.data.ablise.plugin = plugin_slot->plugin;
  ctrlmsg.data.ablise.enable = enable;
  ctrlmsg.data.ablise.plugin_slot = plugin_slot;
  
  lff_write (plugin_slot->jack_rack->ui->ui_to_process, &ctrlmsg);
}

void
plugin_slot_send_ablise_wet_dry (plugin_slot_t * plugin_slot, gboolean enable)
{
  ctrlmsg_t ctrlmsg;
  
  ctrlmsg.type = CTRLMSG_ABLE_WET_DRY;
  ctrlmsg.data.ablise.plugin = plugin_slot->plugin;
  ctrlmsg.data.ablise.enable = enable;
  ctrlmsg.data.ablise.plugin_slot = plugin_slot;
  
  lff_write (plugin_slot->jack_rack->ui->ui_to_process, &ctrlmsg);
}

void
plugin_slot_send_change_plugin   (plugin_slot_t *plugin_slot, plugin_desc_t *desc)
{
  plugin_t *plugin;
  ctrlmsg_t ctrlmsg;
                                                                                                               
  plugin = jack_rack_instantiate_plugin (global_ui->jack_rack, desc);
                                                                                                               
  if (!plugin)
    return;
                                                                                                               
  ctrlmsg.type = CTRLMSG_CHANGE;
  ctrlmsg.data.change.old_plugin = plugin_slot->plugin;
  ctrlmsg.data.change.new_plugin = plugin;
  ctrlmsg.data.change.plugin_slot = plugin_slot;
                                                                                                               
  lff_write (plugin_slot->jack_rack->ui->ui_to_process, &ctrlmsg);
}

void
plugin_slot_set_wet_dry_locked (plugin_slot_t *plugin_slot, gboolean locked)
{
  GSList * list;
#ifdef HAVE_ALSA
  midi_control_t * midi_ctrl;
#endif
  
  settings_set_wet_dry_locked (plugin_slot->settings, locked);

#ifdef HAVE_ALSA  
  for (list = plugin_slot->midi_controls; list; list = g_slist_next (list))
    {
      midi_ctrl = (midi_control_t *) list->data;
      
      if (!midi_ctrl->ladspa_control)
        midi_control_set_locked (midi_ctrl, locked);
    }
#endif
  
  plugin_slot_show_wet_dry_controls (plugin_slot);
}

void
plugin_slot_set_lock_all (plugin_slot_t *plugin_slot, gboolean lock_all, guint lock_copy)
{
  GSList * list;
  unsigned long i;
#ifdef HAVE_ALSA
  midi_control_t * midi_ctrl;
#endif
  
  settings_set_lock_all (plugin_slot->settings, lock_all);
  
  for (i = 0; i < plugin_slot->plugin->desc->control_port_count; i++)
    if (lock_all)
      plugin_slot->port_controls[i].locked = TRUE;
    else
      plugin_slot->port_controls[i].locked =
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(plugin_slot->port_controls[i].lock));

#ifdef HAVE_ALSA  
  for (list = plugin_slot->midi_controls; list; list = g_slist_next (list))
    {
      midi_ctrl = (midi_control_t *) list->data;
      
      if (midi_ctrl->ladspa_control)
        midi_control_set_locked (midi_ctrl,
          lock_all ? TRUE : plugin_slot->port_controls[midi_ctrl->control.ladspa.control].locked);
    }
#endif /* HAVE_ALSA */
  
  plugin_slot_show_controls (plugin_slot, lock_copy);
}

#ifdef HAVE_ALSA
void
plugin_slot_add_midi_control (plugin_slot_t *plugin_slot, midi_control_t *midi_ctrl)
{
  ctrlmsg_t ctrlmsg;
                                                                                                               
  ctrlmsg.type = CTRLMSG_MIDI_ADD;
  ctrlmsg.data.midi.midi_control = midi_ctrl;
                                                                                                               
  lff_write (plugin_slot->jack_rack->ui->ui_to_midi, &ctrlmsg);
}
                                                                                                               
void
plugin_slot_remove_midi_controls (plugin_slot_t *plugin_slot)
{
  GSList * list;
  midi_control_t *midi_ctrl;
  ctrlmsg_t ctrlmsg;

  for (list = plugin_slot->midi_controls; list; list = g_slist_next (list))
    {
      midi_ctrl = list->data;
                                                                                                               
      ctrlmsg.type = CTRLMSG_MIDI_REMOVE;
      ctrlmsg.data.midi.midi_control = midi_ctrl;
                                                                                                               
      lff_write (plugin_slot->jack_rack->ui->ui_to_midi, &ctrlmsg);
    }
                                                                                                               
  g_slist_free (plugin_slot->midi_controls);
  plugin_slot->midi_controls = NULL;
}
                                                                                                               
#endif /* HAVE_ALSA */

/* EOF */


