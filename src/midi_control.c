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

#include "ac_config.h"

#ifdef HAVE_ALSA

#include <math.h>
#include <ctype.h>
#include <string.h>

#include "midi_control.h"
#include "plugin_slot.h"

static midi_control_t *
midi_control_new (plugin_slot_t * plugin_slot)
{
  midi_control_t * midi_ctrl;
  
  midi_ctrl = g_malloc0 (sizeof (midi_control_t));
  
  midi_ctrl->plugin_slot = plugin_slot;
  
  pthread_mutex_init (&midi_ctrl->locked_lock, NULL);
  pthread_mutex_init (&midi_ctrl->midi_channel_lock, NULL);
  pthread_mutex_init (&midi_ctrl->midi_param_lock, NULL);
  
  return midi_ctrl;
}

midi_control_t *
ladspa_midi_control_new (plugin_slot_t * plugin_slot, guint copy, unsigned long control)
{
  midi_control_t *midi_ctrl;
  port_controls_t * port_controls;
  
  midi_ctrl = midi_control_new (plugin_slot);
  port_controls = plugin_slot->port_controls + control;
  
  midi_ctrl->fifo = plugin_slot->plugin->holders[copy].midi_control_fifos + control;
  midi_ctrl->fifos = plugin_slot->plugin->holders[copy].midi_control_fifos;
  midi_ctrl->ctrl_type         = LADSPA_CONTROL;
  midi_ctrl->control.ladspa.control = control;
  midi_ctrl->control.ladspa.copy    = copy;
  
  
  if (port_controls->type == JR_CTRL_BOOL)
    {
      midi_ctrl->min = 0.0;
      midi_ctrl->max = 1.0;
    }
  else if (port_controls->type == JR_CTRL_POINTS)
    {
      GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (port_controls->controls[copy].control));
      GtkTreeIter iter;
      gboolean is_first = TRUE, valid;

      gtk_tree_model_get_iter_first (model, &iter);
      do {
        gfloat value;
        gtk_tree_model_get (model, &iter, 1, &value, -1);
        if (is_first || value < midi_ctrl->min)
          midi_ctrl->min = value;
        if (is_first || value > midi_ctrl->max)
          midi_ctrl->max = value;
        is_first = FALSE;
        valid = gtk_tree_model_iter_next (model, &iter);
      } while (valid);
    }
  else
    {
      GtkAdjustment *adjustment;
      
      if (port_controls->type == JR_CTRL_INT)
        adjustment =
          gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (port_controls->controls[copy].control));
      else
        adjustment = gtk_range_get_adjustment (GTK_RANGE (port_controls->controls[copy].control));
      
      midi_ctrl->min = adjustment->lower;
      midi_ctrl->max = adjustment->upper;
      
      if (port_controls->type == JR_CTRL_FLOAT &&
          port_controls->logarithmic)
        {
          midi_ctrl->min = exp (midi_ctrl->min);
          midi_ctrl->max = exp (midi_ctrl->max);
        }
    }
  midi_ctrl->real_min = midi_ctrl->min;
  midi_ctrl->real_max = midi_ctrl->max;
  
  
  return midi_ctrl;
}

midi_control_t *
wet_dry_midi_control_new (struct _plugin_slot * plugin_slot, unsigned long channel)
{
  midi_control_t *midi_ctrl;
  
  midi_ctrl = midi_control_new (plugin_slot);
  
  midi_ctrl->fifo = plugin_slot->plugin->wet_dry_midi_fifos + channel;
  midi_ctrl->fifos = plugin_slot->plugin->wet_dry_midi_fifos;
  midi_ctrl->ctrl_type          = WET_DRY_CONTROL;
  midi_ctrl->control.wet_dry.channel = channel;
  
  midi_ctrl->min = 0.0;
  midi_ctrl->max = 1.0;

  midi_ctrl->real_min = midi_ctrl->min;
  midi_ctrl->real_max = midi_ctrl->max;
  
  return midi_ctrl;
}

midi_control_t *
toggle_midi_control_new (struct _plugin_slot * plugin_slot)
{
  midi_control_t *midi_ctrl;
  
  midi_ctrl = midi_control_new (plugin_slot);
  
  midi_ctrl->ctrl_type          = PLUGIN_ENABLE_CONTROL;
   
  midi_ctrl->min = 0.0;
  midi_ctrl->max = 1.0;

  midi_ctrl->real_min = midi_ctrl->min;
  midi_ctrl->real_max = midi_ctrl->max;
  
  return midi_ctrl;
}

void
midi_control_destroy (midi_control_t * midi_ctrl)
{
  pthread_mutex_destroy (&midi_ctrl->locked_lock);
  pthread_mutex_destroy (&midi_ctrl->midi_channel_lock);
  pthread_mutex_destroy (&midi_ctrl->midi_param_lock);
  g_free (midi_ctrl);
}

void
midi_control_set_midi_channel (midi_control_t * midi_ctrl, unsigned char channel)
{
  pthread_mutex_lock (&midi_ctrl->midi_channel_lock);
  midi_ctrl->midi_channel = channel;
  pthread_mutex_unlock (&midi_ctrl->midi_channel_lock);
}

void
midi_control_set_midi_param   (midi_control_t * midi_ctrl, unsigned int param)
{
  pthread_mutex_lock (&midi_ctrl->midi_param_lock);
  midi_ctrl->midi_param = param;
  pthread_mutex_unlock (&midi_ctrl->midi_param_lock);
}

void
midi_control_set_locked       (midi_control_t * midi_ctrl, gboolean locked)
{
  pthread_mutex_lock (&midi_ctrl->locked_lock);
  midi_ctrl->locked = locked;
  pthread_mutex_unlock (&midi_ctrl->locked_lock);
}

unsigned char
midi_control_get_midi_channel    (midi_control_t * midi_ctrl)
{
  unsigned char channel;
  
  pthread_mutex_lock (&midi_ctrl->midi_channel_lock);
  channel = midi_ctrl->midi_channel;
  pthread_mutex_unlock (&midi_ctrl->midi_channel_lock);
  
  return channel;
}

unsigned int
midi_control_get_midi_param      (midi_control_t * midi_ctrl)
{
  unsigned int param;
  
  pthread_mutex_lock (&midi_ctrl->midi_param_lock);
  param = midi_ctrl->midi_param;
  pthread_mutex_unlock (&midi_ctrl->midi_param_lock);
  
  return param;
}

gboolean
midi_control_get_locked      (midi_control_t * midi_ctrl)
{
  gboolean locked;
  
  pthread_mutex_lock (&midi_ctrl->locked_lock);
  locked = midi_ctrl->locked;
  pthread_mutex_unlock (&midi_ctrl->locked_lock);
  
  return locked;
}

unsigned long
midi_control_get_wet_dry_channel (midi_control_t * midi_ctrl)
{
  g_assert (midi_ctrl->ctrl_type == WET_DRY_CONTROL);
  
  return midi_ctrl->control.wet_dry.channel;
}

unsigned long
midi_control_get_ladspa_control  (midi_control_t * midi_ctrl)
{
  g_assert (midi_ctrl->ctrl_type == LADSPA_CONTROL);
  
  return midi_ctrl->control.ladspa.control;
}

guint
midi_control_get_ladspa_copy     (midi_control_t * midi_ctrl)
{
  g_assert (midi_ctrl->ctrl_type == LADSPA_CONTROL);
  
  return midi_ctrl->control.ladspa.copy;
}

const char *
midi_control_get_control_name    (midi_control_t * midi_ctrl)
{
  static GString *str = NULL;
  
  if (!str)
    str = g_string_new ("");
  
  switch(midi_ctrl->ctrl_type)
    {
    case LADSPA_CONTROL:
      {
        char *control;
        char *ptr;
        control = g_strdup (midi_ctrl->plugin_slot->plugin->desc->
                            port_names[midi_ctrl->plugin_slot->plugin->desc->
                            control_port_indicies[midi_ctrl->control.ladspa.control]]);

        ptr = strchr (control, '(');
        if (ptr)
          {
            *ptr = '\0';

            while (isspace(*(--ptr)))
              *ptr = '\0';
          }

        g_string_assign (str, control);
        g_free (control);

        break;
      }
    case WET_DRY_CONTROL:
      {
        g_string_assign (str, _("Wet/dry"));
        break;
      }
    case PLUGIN_ENABLE_CONTROL:
      {
        g_string_assign (str, _("Enable"));
        break;
      }
    }
  
  return str->str;
}

static LADSPA_Data
midi_control_clip_value	(midi_control_t *midi_ctrl, LADSPA_Data value)
{
  if (value < midi_ctrl->real_min)
  	value = midi_ctrl->real_min;
  else if (value > midi_ctrl->real_max)
  	value = midi_ctrl->real_max;
  return value;
}

LADSPA_Data
midi_control_set_min_value	(midi_control_t *midi_ctrl, LADSPA_Data value)
{
  value = midi_control_clip_value (midi_ctrl, value);
  pthread_mutex_lock (&midi_ctrl->midi_param_lock);
  midi_ctrl->min = value;
  pthread_mutex_unlock (&midi_ctrl->midi_param_lock);
  return value;
}

LADSPA_Data
midi_control_set_max_value	(midi_control_t *midi_ctrl, LADSPA_Data value)
{
  value = midi_control_clip_value (midi_ctrl, value);
  pthread_mutex_lock (&midi_ctrl->midi_param_lock);
  midi_ctrl->max = value;
  pthread_mutex_unlock (&midi_ctrl->midi_param_lock);
  return value;
}

LADSPA_Data
midi_control_get_min_value       (midi_control_t * midi_ctrl)
{
  LADSPA_Data value;
  
  pthread_mutex_lock (&midi_ctrl->midi_param_lock);
  value = midi_ctrl->min;
  pthread_mutex_unlock (&midi_ctrl->midi_param_lock);
  
  return value;
}

LADSPA_Data
midi_control_get_max_value       (midi_control_t * midi_ctrl)
{
  LADSPA_Data value;
  
  pthread_mutex_lock (&midi_ctrl->midi_param_lock);
  value = midi_ctrl->max;
  pthread_mutex_unlock (&midi_ctrl->midi_param_lock);
  
  return value;
}

#endif /* HAVE_ALSA */


