/*
 *   jack-ladspa-host
 *    
 *   Copyright (C) Robert Ham 2002 (node@users.sourceforge.net)
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
#include <ladspa.h>
#include <dlfcn.h>

#include <glib.h>

#include "plugin.h"
#include "jack_rack.h"
#include "process.h"

#define CONTROL_FIFO_SIZE   128


/** connect up the ladspa instance's input buffers to the previous
    plugin's audio memory.  make sure to check that plugin->prev
    exists. */
void
plugin_connect_input_ports (plugin_t * plugin)
{
  gint copy;
  unsigned long channel;
  unsigned long rack_channel;

  if (!plugin || !plugin->prev)
    return;

  rack_channel = 0;
  for (copy = 0; copy < plugin->copies; copy++)
    {
      for (channel = 0; channel < plugin->desc->channels; channel++)
        {
          plugin->descriptor->
            connect_port (plugin->holders[copy].instance,
                          plugin->desc->audio_input_port_indicies[channel],
                          plugin->prev->audio_memory[rack_channel]);
          rack_channel++;
        }
    }
                    
}

/** connect up a plugin's output ports to its own audio_memory output memory */
void
plugin_connect_output_ports (plugin_t * plugin)
{
  gint copy;
  unsigned long channel;
  unsigned long rack_channel = 0;

  if (!plugin)
    return;


  for (copy = 0; copy < plugin->copies; copy++)
    {
      for (channel = 0; channel < plugin->desc->channels; channel++)
        {
          plugin->descriptor->
            connect_port (plugin->holders[copy].instance,
                          plugin->desc->audio_output_port_indicies[channel],
                          plugin->audio_memory[rack_channel]);
          rack_channel++;
        }
    }
}

void
process_add_plugin (process_info_t * procinfo, plugin_t * plugin)
{

  /* sort out list pointers */
  plugin->next = NULL;
  plugin->prev = procinfo->chain_end;

  if (procinfo->chain_end)
    procinfo->chain_end->next = plugin;
  else
    procinfo->chain = plugin;

  procinfo->chain_end = plugin;

}

/** find a plugin given its index in the chain */
static plugin_t *
process_get_plugin (process_info_t * procinfo, gint plugin_index)
{
  gint i = 0;
  plugin_t * plugin = procinfo->chain;

  while (i < plugin_index && plugin)
    {
      plugin = plugin->next;
      i++;
    }

  return plugin;
}

/** remove a plugin from the chain */
plugin_t *
process_remove_plugin (process_info_t * procinfo, gint plugin_index)
{
  plugin_t * plugin;

  /* find the plugin in the chain */
  plugin = process_get_plugin (procinfo, plugin_index);

  /* sort out chain pointers */
  if (plugin->prev)
    plugin->prev->next = plugin->next;
  else
    procinfo->chain = plugin->next;

  if (plugin->next)
    plugin->next->prev = plugin->prev;
  else
    procinfo->chain_end = plugin->prev;

  return plugin;
}

/** enable/disable a plugin */
void
process_ablise_plugin (process_info_t * procinfo, gint plugin_index, gint enable)
{
  plugin_t * plugin;

  plugin = process_get_plugin (procinfo, plugin_index);

  plugin->enabled = enable;
}

/** move a plugin up or down one place in the chain */
void
process_move_plugin (process_info_t * procinfo, gint plugin_index, gint up)
{

  /* specified plugin */
  plugin_t *plugin;
  /* other plugins in the chain */
  plugin_t *pp = NULL, *p, *n, *nn = NULL;

  plugin = process_get_plugin (procinfo, plugin_index);

  /* note that we should never recieve an illogical move request
     ie, there will always be at least 1 plugin before for an up
     request or 1 plugin after for a down request */

  /* these are pointers to the plugins surrounding the specified one:
     { pp, p, plugin, n, nn } which makes things much clearer than
     tptr, tptr2 etc */
  p = plugin->prev;
  if (p) pp = p->prev;
  n = plugin->next;
  if (n) nn = n->next;

  if (up)
    {
      if (!p)
	return;

      if (pp)
        pp->next = plugin;
      else
        procinfo->chain = plugin;

      p->next = n;
      p->prev = plugin;

      plugin->prev = pp;
      plugin->next = p;

      if (n)
	n->prev = p;
      else
	procinfo->chain_end = p;

    }
  else
    {
      if (!n)
	return;

      if (p)
	p->next = n;
      else
	procinfo->chain = n;

      n->prev = p;
      n->next = plugin;

      plugin->prev = n;
      plugin->next = nn;

      if (nn)
	nn->prev = plugin;
      else
	procinfo->chain_end = plugin;
    }
}

/** exchange an existing plugin for a newly created one */
plugin_t *
process_change_plugin (process_info_t * procinfo,
	               gint plugin_index, plugin_t * new_plugin)
{

  plugin_t *plugin;

  plugin = process_get_plugin (procinfo, plugin_index);

  new_plugin->next = plugin->next;
  new_plugin->prev = plugin->prev;

  if (plugin->prev)
    plugin->prev->next = new_plugin;
  else
    procinfo->chain = new_plugin;

  if (plugin->next)
    plugin->next->prev = new_plugin;
  else
    procinfo->chain_end = new_plugin;

  return plugin;
}


/******************************************
 ************* non RT stuff ***************
 ******************************************/


static int
plugin_open_plugin (plugin_desc_t * desc,
                    void ** dl_handle_ptr,
                    const LADSPA_Descriptor ** descriptor_ptr)
{
  void * dl_handle;
  const char * dlerr;
  LADSPA_Descriptor_Function get_descriptor;
    
  /* open the object file */
  dl_handle = dlopen (desc->object_file, RTLD_NOW|RTLD_GLOBAL);
  if (!dl_handle)
    {
      fprintf (stderr, "%s: error opening shared object file '%s': %s\n",
               __FUNCTION__, desc->object_file, dlerror());
      return 1;
    }
  
  
  /* get the get_descriptor function */
  dlerror (); /* clear the error report */
  
  get_descriptor = (LADSPA_Descriptor_Function)
    dlsym (dl_handle, "ladspa_descriptor");
  
  dlerr = dlerror();
  if (dlerr)
    {
      fprintf (stderr, "%s: error finding descriptor symbol in object file '%s': %s\n",
               __FUNCTION__, desc->object_file, dlerr);
      dlclose (dl_handle);
      return 1;
    }
  
  *descriptor_ptr = get_descriptor (desc->index);
  *dl_handle_ptr = dl_handle;
  
  return 0;
}

static int
plugin_instantiate (const LADSPA_Descriptor * descriptor,
                    unsigned long plugin_index,
                    gint copies,
                    LADSPA_Handle * instances)
{
  gint i;
  
  for (i = 0; i < copies; i++)
    {
      instances[i] = descriptor->instantiate (descriptor, sample_rate);
      
      if (!instances[i])
        {
          unsigned long d;
 
          for (d = 0; d < i; d++)
            descriptor->cleanup (instances[d]);
          
          return 1;
        }
    }
  
  return 0;
}

static LADSPA_Data unused_control_port_output;

static void
plugin_init_holder (ladspa_holder_t * holder,
                    plugin_t * plugin,
                    LADSPA_Handle instance)
{
  unsigned long i;
  plugin_desc_t * desc;
  
  desc = plugin->desc;
  
  holder->instance = instance;
  
  if (desc->control_port_count > 0)
    {
      holder->control_fifos  = g_malloc (sizeof (lff_t) * desc->control_port_count);
      holder->control_memory = g_malloc (sizeof (LADSPA_Data) * desc->control_port_count);
    }
  else
    {
      holder->control_fifos  = NULL;
      holder->control_memory = NULL;
    }
  
  for (i = 0; i < desc->control_port_count; i++)
    {
      lff_init (holder->control_fifos + i, CONTROL_FIFO_SIZE, sizeof (LADSPA_Data));
      
      holder->control_memory[i] =
        plugin_desc_get_default_control_value (desc, desc->control_port_indicies[i], sample_rate);        
      
      plugin->descriptor->
        connect_port (instance, desc->control_port_indicies[i], holder->control_memory + i);
    }
  
  for (i = 0; i < desc->port_count; i++)
    {
      if (!LADSPA_IS_PORT_CONTROL (desc->port_descriptors[i]))
        continue;
      
      if (LADSPA_IS_PORT_OUTPUT (desc->port_descriptors[i]))
        plugin->descriptor-> connect_port (instance, i, &unused_control_port_output);
    }
}


plugin_t *
plugin_new (plugin_desc_t * desc, unsigned long rack_channels)
{
  void * dl_handle;
  const LADSPA_Descriptor * descriptor;
  LADSPA_Handle * instances;
  gint copies;
  unsigned long i;
  int err;
  plugin_t * plugin;
  
  /* open the plugin */
  err = plugin_open_plugin (desc, &dl_handle, &descriptor);
  if (err)
    return NULL;

  
  /* create the instances */
  copies = plugin_desc_get_copies (desc, rack_channels);
  instances = g_malloc (sizeof (LADSPA_Handle) * copies);

  err = plugin_instantiate (descriptor, desc->index, copies, instances);
  if (err)
    {
      g_free (instances);
      dlclose (dl_handle);
      return NULL;
    }
  

  plugin = g_malloc (sizeof (plugin_t));
  
  plugin->descriptor = descriptor;
  plugin->dl_handle = dl_handle;
  plugin->desc = desc;
  plugin->copies = copies;
  plugin->enabled = FALSE;
  plugin->next = NULL;
  plugin->prev = NULL;
  
  plugin->audio_memory = g_malloc (sizeof (LADSPA_Data *) * rack_channels);
  for (i = 0; i < rack_channels; i++)
    {
      plugin->audio_memory[i] = g_malloc (sizeof (LADSPA_Data) * buffer_size);
    }
  
  plugin->holders = g_malloc (sizeof (ladspa_holder_t) * copies);
  for (i = 0; i < copies; i++)
    {
      plugin_init_holder (plugin->holders + i, plugin, instances[i]);
    }
  
  
  return plugin;
}


void      	plugin_destroy (plugin_t * plugin)
{
}


/* EOF */


