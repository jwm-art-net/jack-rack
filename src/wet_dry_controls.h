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

#ifndef __JR_WET_DRY_CONTROLS_H__
#define __JR_WET_DRY_CONTROLS_H__

#include <gtk/gtk.h>

#include "lock_free_fifo.h"

struct _plugin_slot;

typedef struct _wet_dry_controls wet_dry_controls_t;

struct _wet_dry_controls
{
  struct _plugin_slot *    plugin_slot;
  lff_t *                  fifos;
  
  GtkWidget *              control_box;
  GtkWidget **             controls;
  GtkWidget **             labels;
  GtkWidget *              lock;
};

wet_dry_controls_t * wet_dry_controls_new     (struct _plugin_slot * plugin_slot);
void                 wet_dry_controls_destroy (wet_dry_controls_t * wet_dry);

void wet_dry_controls_block_callback (wet_dry_controls_t * wet_dry, unsigned long channel);
void wet_dry_controls_unblock_callback (wet_dry_controls_t * wet_dry, unsigned long channel);

#endif /* __JR_WET_DRY_CONTROLS_H__ */
