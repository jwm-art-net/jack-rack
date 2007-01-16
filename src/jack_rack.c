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

#include "ac_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>

#include <gtk/gtk.h>
#include <ladspa.h>

#include "jack_rack.h"
#include "lock_free_fifo.h"
#include "control_message.h"
#include "globals.h"
#include "ui.h"
#include "file.h"

jack_rack_t *
jack_rack_new (ui_t * ui, unsigned long channels)
{
  jack_rack_t *rack;

  rack = g_malloc (sizeof (jack_rack_t));
  rack->slots          = NULL;
  rack->saved_plugins  = NULL;
  rack->ui             = ui;
  rack->channels       = channels;

  return rack;
}


void
jack_rack_destroy (jack_rack_t * jack_rack)
{
  g_free (jack_rack);
}


plugin_t *
jack_rack_instantiate_plugin (jack_rack_t * jack_rack, plugin_desc_t * desc)
{
  plugin_t * plugin;
  
  /* check whether or not the plugin is RT capable and confirm with the user if it isn't */
  if (!LADSPA_IS_HARD_RT_CAPABLE(desc->properties)) {
    GtkWidget * dialog;
    gint response;

    dialog = gtk_message_dialog_new (GTK_WINDOW(jack_rack->ui->main_window),
               GTK_DIALOG_DESTROY_WITH_PARENT|GTK_DIALOG_MODAL,
               GTK_MESSAGE_WARNING,
               GTK_BUTTONS_YES_NO,
               _("Plugin not RT capable\n\nThe plugin '%s' does not describe itself as being capable of real-time operation.  If you use it, you may experience drop-outs or unexpected disconnection from JACK.\n\nAre you sure you want to add this plugin?"),
               desc->name);
    response = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    
    if (response != GTK_RESPONSE_YES) {
      return NULL;
    }
  }

  /* create the plugin */
  plugin = plugin_new (desc, jack_rack);

  if (!plugin) {
    GtkWidget * dialog;
    
    dialog = gtk_message_dialog_new (GTK_WINDOW(jack_rack->ui->main_window),
               GTK_DIALOG_DESTROY_WITH_PARENT,
               GTK_MESSAGE_ERROR,
               GTK_BUTTONS_CLOSE,
               _("Error loading file plugin '%s' from file '%s'"),
               desc->name, desc->object_file);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    return NULL;
  }
  
  return plugin;
}


void
jack_rack_send_add_plugin (jack_rack_t * jack_rack, plugin_desc_t * desc)
{
  plugin_t * plugin;
  ctrlmsg_t ctrlmsg;
  
  plugin = jack_rack_instantiate_plugin (jack_rack, desc);
  
  if (!plugin)
    return;
  
  /* send the chain link off to the process() callback */
  ctrlmsg.type = CTRLMSG_ADD;
  ctrlmsg.data.add.plugin = plugin;
  lff_write (jack_rack->ui->ui_to_process, &ctrlmsg);
}


void
jack_rack_add_saved_plugin (jack_rack_t * jack_rack, saved_plugin_t * saved_plugin)
{
  jack_rack->saved_plugins = g_slist_append (jack_rack->saved_plugins, saved_plugin);
  
  jack_rack_send_add_plugin (jack_rack, saved_plugin->settings->desc);
}


void
jack_rack_add_plugin (jack_rack_t * jack_rack, plugin_t * plugin)
{
  plugin_slot_t *plugin_slot;
  saved_plugin_t * saved_plugin = NULL;
  GSList * list;
  
  /* see if there's any saved settings that match the plugin id */
  for (list = jack_rack->saved_plugins; list; list = g_slist_next (list))
    {
      saved_plugin = list->data;
      
      if (saved_plugin->settings->desc->id == plugin->desc->id)
        {
          jack_rack->saved_plugins = g_slist_remove (jack_rack->saved_plugins, saved_plugin);
          break;
        }
      saved_plugin = NULL;
    }

  /* create the plugin_slot */
  plugin_slot = plugin_slot_new (jack_rack, plugin, saved_plugin);

  jack_rack->slots = g_list_append (jack_rack->slots, plugin_slot);
  
  plugin_slot_send_ablise (plugin_slot, settings_get_enabled (plugin_slot->settings));
  plugin_slot_send_ablise_wet_dry (plugin_slot, settings_get_wet_dry_enabled (plugin_slot->settings));

  gtk_box_pack_start (GTK_BOX (jack_rack->ui->plugin_box),
                      plugin_slot->main_vbox, FALSE, FALSE, 0);
}


void
jack_rack_send_remove_plugin_slot (jack_rack_t *jack_rack, plugin_slot_t *plugin_slot)
{
  ctrlmsg_t ctrlmsg;

#ifdef HAVE_ALSA
  plugin_slot_remove_midi_controls (plugin_slot);
#endif

  ctrlmsg.type = CTRLMSG_REMOVE;
  ctrlmsg.data.remove.plugin = plugin_slot->plugin;
  ctrlmsg.data.remove.plugin_slot = plugin_slot;
                                                                                                               
  lff_write (jack_rack->ui->ui_to_process, &ctrlmsg);
}


void
jack_rack_remove_plugin_slot (jack_rack_t * jack_rack, plugin_slot_t * plugin_slot)
{
  jack_rack->slots = g_list_remove (jack_rack->slots, plugin_slot);

  plugin_slot_destroy (plugin_slot);
}


void
jack_rack_send_move_plugin_slot (jack_rack_t * jack_rack, plugin_slot_t * plugin_slot, gint up)
{
  ctrlmsg_t ctrlmsg;
  GList * slot_list_data;

  /* this is because we use indices */
  if (ui_get_state (jack_rack->ui)  == STATE_RACK_CHANGE)
    return;
                                                                                                               
  slot_list_data = g_list_find (global_ui->jack_rack->slots, plugin_slot);
  /* check the logic of what we're doing */
  if ((up && !g_list_previous (slot_list_data)) ||
      (!up && !g_list_next (slot_list_data)))
    return;
                                                                                                               
  ui_set_state (global_ui, STATE_RACK_CHANGE);
                                                                                                               
  ctrlmsg.type = CTRLMSG_MOVE;
  ctrlmsg.data.move.plugin = plugin_slot->plugin;
  ctrlmsg.data.move.up = up;
  ctrlmsg.data.move.plugin_slot = plugin_slot;
                                                                                                               
  lff_write (jack_rack->ui->ui_to_process, &ctrlmsg);
}


void
jack_rack_move_plugin_slot (jack_rack_t * jack_rack, plugin_slot_t * plugin_slot, gint up)
{
  gint index;
  
  index = g_list_index (jack_rack->slots, plugin_slot);
  jack_rack->slots = g_list_remove (jack_rack->slots, plugin_slot);
  
  if (up)
    index--;
  else
    index++;
  
  jack_rack->slots = g_list_insert (jack_rack->slots, plugin_slot, index);

  gtk_box_reorder_child (GTK_BOX (jack_rack->ui->plugin_box),
                         plugin_slot->main_vbox, index);
}


void
jack_rack_change_plugin_slot (jack_rack_t * jack_rack, plugin_slot_t * plugin_slot, plugin_t * plugin)
{
  plugin_slot_change_plugin (plugin_slot, plugin);
}


void
jack_rack_send_clear_plugins (jack_rack_t * jack_rack)
{
  GList * slot;
  
  for (slot = jack_rack->slots; slot; slot = g_list_next (slot))
    jack_rack_send_remove_plugin_slot (jack_rack, (plugin_slot_t *) slot->data);
}


plugin_slot_t *
jack_rack_get_plugin_slot (jack_rack_t * jack_rack, unsigned long plugin_index)
{
  return (plugin_slot_t *) g_list_nth_data (jack_rack->slots, plugin_index);
}


#ifdef HAVE_ALSA
unsigned int
jack_rack_get_next_midi_param (jack_rack_t * jack_rack, unsigned char channel)
{
  plugin_slot_t *plugin_slot;
  midi_control_t *midi_ctrl;
  GList *slot;
  GList *next_slot;
  GSList *control;
  unsigned int param = 1;
                                                                                                               
  for (slot = jack_rack->slots; slot; slot = next_slot)
    {
      next_slot = g_list_next (slot);
      plugin_slot = slot->data;
                                                                                                               
      for (control = plugin_slot->midi_controls; control; control = g_slist_next (control))
        {
          midi_ctrl = control->data;
                                                                                                               
          if (midi_control_get_midi_channel (midi_ctrl) == channel &&
              midi_control_get_midi_param (midi_ctrl) == param)
            {
              param++;
              next_slot = jack_rack->slots;
              break;
            }
       }
    }
                                                                                                               
  g_assert (param < 129);
                                                                                                               
  return param;
}
#endif /* HAVE_ALSA */


/* EOF */



