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

#ifndef __JR_CONTROL_MESSAGE_H__
#define __JR_CONTROL_MESSAGE_H__

#include "ac_config.h"

#include <glib.h>
#include <ladspa.h>

/** These are for messages between the gui and the process() callback */
typedef enum _ctrlmsg_type
{
  CTRLMSG_ADD,
  CTRLMSG_REMOVE,
  CTRLMSG_ABLE,
  CTRLMSG_ABLE_WET_DRY,
  CTRLMSG_MOVE,
  CTRLMSG_CHANGE,
  CTRLMSG_QUIT,

#ifdef HAVE_ALSA  
  CTRLMSG_MIDI_ADD,
  CTRLMSG_MIDI_REMOVE,
  CTRLMSG_MIDI_CTRL,
  CTRLMSG_MIDI_ABLE
#endif

} ctrlmsg_type_t;

typedef struct _ctrlmsg ctrlmsg_t;

struct _plugin;
struct _plugin_desc;
struct _plugin_slot;
struct _midi_control;

struct _ctrlmsg
{
  ctrlmsg_type_t type;
  union
  {
    struct
    {
      struct _plugin      * plugin;
    } add;
    
    struct
    {
      struct _plugin_slot * plugin_slot;
      struct _plugin      * plugin;
    } remove;
    
    struct
    {
      struct _plugin_slot * plugin_slot;
      struct _plugin      * plugin;
      gboolean              enable;
    } ablise;
    
    struct
    {
      struct _plugin_slot * plugin_slot;
      struct _plugin      * plugin;
      gboolean              up;
    } move;
    
    struct
    {
      struct _plugin_slot * plugin_slot;
      struct _plugin      * old_plugin;
      struct _plugin      * new_plugin;
    } change;
    
/*    struct
    {
      struct _plugin      * chain;
    } clear;*/


    
    struct
    {
      struct _midi_control  *midi_control;
      LADSPA_Data           value;
    } midi;
    
  } data;
 
};

#endif /* __JR_CONTROL_MESSAGE_H__ */
