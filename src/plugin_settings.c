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

#include "plugin_settings.h"

#define PLUGIN_COPIES 50

static void
settings_collection_create_settings (settings_collection_t * collection, plugin_mgr_t * plugin_mgr)
{
  GSList * list;
  settings_t * settings;
  
  for (list = plugin_mgr->plugins; list; list = g_slist_next (list))
    {
      settings = settings_new ((plugin_desc_t *) list->data, collection->sample_rate);
      collection->settings = g_slist_append (collection->settings, settings);
    }
}

settings_collection_t *
settings_collection_new (plugin_mgr_t * plugin_mgr, jack_nframes_t sample_rate)
{
  settings_collection_t * collection;
  
  collection = g_malloc (sizeof (settings_collection_t));
  
  collection->sample_rate = sample_rate;
  collection->settings = NULL;
  
  settings_collection_create_settings (collection, plugin_mgr);
  
  return collection;
}

void settings_collection_destroy (settings_collection_t * collection);

void
settings_collection_change_sample_rate (settings_collection_t * collection, jack_nframes_t sample_rate)
{
  GSList * list;
  
  if (sample_rate == collection->sample_rate)
    return;
  
  for (list = collection->settings; list; list = g_slist_next (list))
    settings_change_sample_rate ((settings_t *) list->data,
                                 (LADSPA_Data) collection->sample_rate,
                                 (LADSPA_Data) sample_rate);
  
  collection->sample_rate = sample_rate;
}

static gint
settings_collection_find_settings (gconstpointer a, gconstpointer b)
{
  const settings_t * settings = (const settings_t *) a;
  const unsigned long * plugin_id = (const unsigned long *) b;
  
  return settings->desc->id == *plugin_id ? 0 : 1;
}


settings_t *
settings_collection_get_settings (settings_collection_t * collection, unsigned long plugin_id)
{
  GSList * settings;
  settings = g_slist_find_custom (collection->settings, &plugin_id,
                                   settings_collection_find_settings);
  return settings->data;
}


static void
settings_set_to_default (settings_t * settings, jack_nframes_t sample_rate)
{
  unsigned long copy, control;
  LADSPA_Data value;
  
  for (control = 0; control < settings->desc->control_port_count; control++)
    {
      value = plugin_desc_get_default_control_value (settings->desc, control, sample_rate);

      for (copy = 0; copy < PLUGIN_COPIES; copy++)
        settings->control_values[copy][control] = value;
          
      settings->locks[control] = TRUE;
    }
}

settings_t *
settings_new     (plugin_desc_t * desc, jack_nframes_t sample_rate)
{
  settings_t * settings;
  
  settings = g_malloc (sizeof (settings_t));
  
  settings->desc = desc;
  settings->lock_all = TRUE;
  settings->locks = NULL;
  settings->control_values = NULL;
  
  if (desc->control_port_count > 0)
    {
      unsigned long copy;
      
      settings->locks = g_malloc (sizeof (gboolean) * desc->control_port_count);

      settings->control_values = g_malloc (sizeof (LADSPA_Data *) * PLUGIN_COPIES);
      for (copy = 0; copy < PLUGIN_COPIES; copy++)
        settings->control_values[copy] = g_malloc (sizeof (LADSPA_Data) * desc->control_port_count);
      
      settings_set_to_default (settings, sample_rate);
    }
  
  return settings;
}

void
settings_destroy (settings_t * settings)
{
  if (settings->desc->control_port_count)
    {
      unsigned long i;
      
      for (i = 0; i < PLUGIN_COPIES; i++)
        g_free (settings->control_values[i]);

      g_free (settings->control_values);
      g_free (settings->locks);
    }
  
  g_free (settings);
}

void
settings_set_control_value (settings_t * settings, unsigned long control_index, unsigned long copy, LADSPA_Data value)
{
  settings->control_values[copy][control_index] = value;
}

void
settings_set_lock          (settings_t * settings, unsigned long control_index, gboolean locked)
{
  settings->locks[control_index] = locked;
}

void
settings_set_lock_all (settings_t * settings, gboolean lock_all)
{
  settings->lock_all = lock_all;
}

LADSPA_Data
settings_get_control_value (const settings_t * settings, unsigned long control_index, unsigned long copy)
{
  return settings->control_values[copy][control_index];
}

gboolean
settings_get_lock          (const settings_t * settings, unsigned long control_index)
{
  return settings->locks[control_index]; 
}

gboolean
settings_get_lock_all      (const settings_t * settings)
{
  return settings->lock_all;
}

void
settings_change_sample_rate (settings_t * settings, LADSPA_Data old_sample_rate, LADSPA_Data new_sample_rate)
{
  if (settings->desc->control_port_count)
    {
      unsigned long copy, control;

      for (control = 0; control < settings->desc->control_port_count; control++)
        for (copy = 0; copy < PLUGIN_COPIES; copy++)
          if (LADSPA_IS_HINT_SAMPLE_RATE (settings->desc->port_range_hints[control].HintDescriptor))
            settings->control_values[copy][control] =
              (settings->control_values[copy][control] / old_sample_rate) * new_sample_rate;
    }
}

/* EOF */


