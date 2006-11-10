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

/*
 * all setups needs this
 */

#include <glib.h>

#include "plugin_settings.h"

typedef struct _saved_plugin saved_plugin_t;

struct _saved_plugin
{
  settings_t     *settings;
  GSList *       midi_controls;
};



#ifdef HAVE_XML

#include <jack/jack.h>

#include "ui.h"

typedef struct _saved_rack   saved_rack_t;

struct _saved_rack
{
  unsigned long  channels;
  jack_nframes_t sample_rate;
  GSList *       plugins;
};


#endif /* HAVE_XML */

#endif /* __JR_FILE_H__ */
