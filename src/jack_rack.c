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

#ifdef HAVE_LADCCA
#include <ladcca/ladcca.h>
#endif

#include "jack_rack.h"
#include "lock_free_fifo.h"
#include "callbacks.h"
#include "control_message.h"
#include "callbacks.h"
#include "globals.h"
#include "ui.h"

jack_rack_t *
jack_rack_new (ui_t * ui, unsigned long channels)
{
  jack_rack_t *rack;

  rack = g_malloc (sizeof (jack_rack_t));
  rack->slots = NULL;
  rack->saved_settings = NULL;
  rack->ui = ui;
  rack->channels = channels;

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
               "Plugin not RT capable\n\nThe plugin '%s' does not describe itself as being capable of real-time operation.  You may experience drop outs or jack may even kick us out if you use it.\n\nAre you sure you want to add this plugin?",
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
               "Error loading file plugin '%s' from file '%s'",
               desc->name, desc->object_file);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    return NULL;
  }
  
  return plugin;
}

void
jack_rack_add_plugin (jack_rack_t * jack_rack, plugin_desc_t * desc)
{
  plugin_t * plugin;
  ctrlmsg_t ctrlmsg;
  
  plugin = jack_rack_instantiate_plugin (jack_rack, desc);
  
  if (!plugin)
    return;
  
  /* send the chain link off to the process() callback */
  ctrlmsg.type = CTRLMSG_ADD;
  ctrlmsg.pointer = plugin;
  ctrlmsg.second_pointer = desc;
  lff_write (jack_rack->ui->ui_to_process, &ctrlmsg);
}


void
jack_rack_add_plugin_slot (jack_rack_t * jack_rack, plugin_t * plugin)
{
  plugin_slot_t *plugin_slot;
  settings_t * settings = NULL;
  GSList * list;
  
  /* see if there's any saved settings that match the plugin id */
  for (list = jack_rack->saved_settings; list; list = g_slist_next (list))
    {
      settings = (settings_t *) list->data;
      if (settings->desc->id == plugin->desc->id)
        {
          jack_rack->saved_settings = g_slist_remove (jack_rack->saved_settings, settings);
          break;
        }
      settings = NULL;
    }

  /* create the plugin_slot */
  plugin_slot = plugin_slot_new (jack_rack, plugin, settings);
  
  if (settings)
    settings_destroy (settings);

  jack_rack->slots = g_list_append (jack_rack->slots, plugin_slot);
  
  plugin_slot_ablise (plugin_slot, settings_get_enabled (plugin_slot->settings));
  plugin_slot_ablise_wet_dry (plugin_slot, settings_get_wet_dry_enabled (plugin_slot->settings));

  gtk_box_pack_start (GTK_BOX (jack_rack->ui->plugin_box),
                      plugin_slot->main_vbox, FALSE, FALSE, 0);
}

void
jack_rack_remove_plugin_slot (jack_rack_t * jack_rack, plugin_slot_t * plugin_slot)
{
  jack_rack->slots = g_list_remove (jack_rack->slots, plugin_slot);

  plugin_slot_destroy (plugin_slot);
}

void
jack_rack_move_plugin_slot (jack_rack_t * jack_rack, plugin_slot_t * plugin_slot, gint up)
{
/*  plugin_slot_t *tmpslot;
  if (up)
    {
      if (!plugin_slot->prev)
        return;

      plugin_slot->index--;
      plugin_slot->prev->index++;

      if (plugin_slot->next)
        {
          plugin_slot->next->prev = plugin_slot->prev;
        }
      else
        {
          jack_rack->slots_end = plugin_slot->prev;
        }
      tmpslot = plugin_slot->prev->prev;
      if (tmpslot)
        {
          tmpslot->next = plugin_slot;
        }
      else
        {
          jack_rack->slots = plugin_slot;
        }
      plugin_slot->prev->next = plugin_slot->next;
      plugin_slot->prev->prev = plugin_slot;
      plugin_slot->next = plugin_slot->prev;
      plugin_slot->prev = tmpslot;

    }
  else
    {
      if (!plugin_slot->next)
        return;

      plugin_slot->index++;
      plugin_slot->next->index--;

      if (plugin_slot->prev)
        {
          plugin_slot->prev->next = plugin_slot->next;
        }
      else
        {
          jack_rack->slots = plugin_slot->next;
        }
      tmpslot = plugin_slot->next->next;
      if (tmpslot)
        {
          tmpslot->prev = plugin_slot;
        }
      else
        {
          jack_rack->slots_end = plugin_slot;
        }
      plugin_slot->next->prev = plugin_slot->prev;
      plugin_slot->next->next = plugin_slot;
      plugin_slot->prev = plugin_slot->next;
      plugin_slot->next = tmpslot;
    }*/
  
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
jack_rack_clear_plugins (jack_rack_t * jack_rack, plugin_t * plugin)
{
  plugin_slot_t * slot;
  plugin_t * next_plugin;
  GList * slot_list;
  
  
  for (; plugin; plugin = next_plugin)
    {
      next_plugin = plugin->next;
      
      for (slot_list = jack_rack->slots; slot_list; slot_list = g_list_next (slot_list))
        {
          slot = (plugin_slot_t *) slot_list->data;
  
          if (slot->plugin == plugin)
            {
              jack_rack_remove_plugin_slot (jack_rack, slot);
              break;
            }
        }
      
      plugin_destroy (plugin, jack_rack->ui->procinfo->jack_client);
    }
}

plugin_slot_t *
jack_rack_get_plugin_slot (jack_rack_t * jack_rack, unsigned long plugin_index)
{
  return (plugin_slot_t *) g_list_nth_data (jack_rack->slots, plugin_index);
}



/* EOF */



