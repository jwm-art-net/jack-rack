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

#ifndef __JR_FILE_H__
#define __JR_FILE_H__

#include "ac_config.h"

#include <stdio.h>

#include "jack_rack.h"
#include "plugin_desc.h"

typedef struct _saved_plugin saved_plugin_t;

struct _saved_plugin
{
  gboolean        enabled;
  settings_t *    settings;  
  plugin_desc_t * desc;
};


saved_plugin_t * saved_plugin_new ();
void             saved_plugin_destroy (saved_plugin_t * saved_plugin);


void jack_rack_open_file (jack_rack_t * jack_rack, const char * filename);
void jack_rack_save_file (jack_rack_t * jack_rack, const char * filename);


#endif /* __JR_FILE_H__ */
