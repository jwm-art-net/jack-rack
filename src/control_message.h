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

#ifndef __JLH_CONTROL_MESSAGE_H__
#define __JLH_CONTROL_MESSAGE_H__

#include <glib.h>

/** These are for messages between the gui and the process() callback */

typedef enum { CTRLMSG_ADD,
               CTRLMSG_REMOVE,
               CTRLMSG_ABLE,
               CTRLMSG_MOVE,
               CTRLMSG_CHANGE,
               CTRLMSG_CLEAR,
               CTRLMSG_QUIT,
               CTRLMSG_ABLE_WET_DRY
             } ctrlmsg_type_t;

typedef struct ctrlmsg {
  ctrlmsg_type_t type;
  long number;
  long second_number;
  gpointer pointer;
  gpointer second_pointer;
} ctrlmsg_t;

#endif /* __JLH_CONTROL_MESSAGE_H__ */
