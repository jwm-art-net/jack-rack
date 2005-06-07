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

#ifndef __JLH_GLOBALS_H__
#define __JLH_GLOBALS_H__

#include <glib.h>

#include "ac_config.h"


#ifdef HAVE_LADCCA
	#include <ladcca/ladcca.h>
	extern cca_client_t * global_cca_client;
#endif

#define JACK_RACK_LOGO_FILE "jack-rack-logo.png"
#define JACK_RACK_ICON_FILE "jack-rack-icon.png"
#define JACK_RACK_CHANNELS_ICON_FILE "gnome-mixer-small.png"
#define JACK_RACK_URL "http://jack-rack.sourceforge.net/"


#define set_string_property(property, value) \
  \
  if (property) \
    g_free (property); \
  \
  if (value) \
    (property) = g_strdup (value); \
  else \
    (property) = NULL;
    
//#ifdef HAVE_GNOME
#if 0
	#include <libgnome/gnome-i18n.h>
#else
	#ifdef ENABLE_NLS
		#include <libintl.h>
		#define _(x) gettext(x)
	#else
		#define _(x) x
	#endif

	#define N_(x) x
#endif

extern struct _ui *global_ui;
extern gboolean   connect_inputs;
extern gboolean   connect_outputs;
extern gboolean   time_runs;
extern GString    *client_name;
extern GString    *initial_filename;

#endif /* __JLH_GLOBALS_H__ */
