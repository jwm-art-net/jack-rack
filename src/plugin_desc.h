/*
 *   jack-rack
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

#ifndef __JR_PLUGIN_DESC_H__
#define __JR_PLUGIN_DESC_H__

#include <ladspa.h>
#include <glib.h>
#include <jack/jack.h>

typedef struct _plugin_desc plugin_desc_t;

struct _plugin_desc
{
  char *                   object_file;
  unsigned long            index;
  unsigned long            id;
  char *                   name;
  LADSPA_Properties        properties;
  gboolean                 rt;
  
  unsigned long            channels;

  unsigned long            port_count;
  LADSPA_PortDescriptor *  port_descriptors;
  LADSPA_PortRangeHint *   port_range_hints;
  
  unsigned long *          audio_input_port_indicies;
  unsigned long *          audio_output_port_indicies;

  unsigned long            control_port_count;
  unsigned long *          control_port_indicies;
};

plugin_desc_t * plugin_desc_new ();
plugin_desc_t * plugin_desc_new_with_descriptor (const char * object_file,
                                                 unsigned long index,
                                                 const LADSPA_Descriptor * descriptor);
void            plugin_desc_destroy ();

void plugin_desc_set_object_file (plugin_desc_t * pd, const char * object_file);
void plugin_desc_set_index       (plugin_desc_t * pd, unsigned long index);
void plugin_desc_set_id          (plugin_desc_t * pd, unsigned long id);
void plugin_desc_set_name        (plugin_desc_t * pd, const char * name);
void plugin_desc_set_properties  (plugin_desc_t * pd, LADSPA_Properties properties);
void plugin_desc_set_ports       (plugin_desc_t * pd,
                                  unsigned long port_count,
                                  const LADSPA_PortDescriptor * port_descriptors,
                                  const LADSPA_PortRangeHint * port_range_hints);

struct _plugin * plugin_desc_instantiate (plugin_desc_t * pd);

LADSPA_Data plugin_desc_get_default_control_value (plugin_desc_t * pd, unsigned long port_index, jack_nframes_t sample_rate);
LADSPA_Data plugin_desc_change_control_value (plugin_desc_t *, unsigned long, LADSPA_Data, jack_nframes_t, jack_nframes_t);

gint plugin_desc_get_copies (plugin_desc_t * pd, unsigned long rack_channels);

#endif /* __JR_PLUGIN_DESC_H__ */
