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
 
#include "ac_config.h"
#include "wet_dry_controls.h"
#include "plugin_slot.h"
#include "control_callbacks.h"
#include "plugin_slot_callbacks.h"
#include "jack_rack.h"

static GtkWidget *
wet_dry_create_control (wet_dry_controls_t * wet_dry, unsigned long channel)
{
  GtkWidget * control;
  
  control = gtk_hscale_new_with_range (0.0, 1.0, 0.001);
  gtk_range_set_value (GTK_RANGE (control), 1.0);
  gtk_scale_set_draw_value (GTK_SCALE (control), FALSE);
  gtk_widget_show (control);
  g_signal_connect  (G_OBJECT (control), "value-changed",
                     G_CALLBACK (slot_wet_dry_control_cb), wet_dry);
  g_object_set_data (G_OBJECT (control),
                     "jack-rack-wet-dry-channel", GUINT_TO_POINTER (channel));
                     
#ifdef HAVE_ALSA
  g_signal_connect  (G_OBJECT (control), "button-press-event",
                     G_CALLBACK (wet_dry_button_press_cb), wet_dry);
#endif

  return control;
}
 
void
wet_dry_create_controls (wet_dry_controls_t * wet_dry)
{
  GtkWidget * sep;
  GtkWidget * box;
  
  /* main vbox */
  wet_dry->control_box = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (wet_dry->control_box);
  
  /* seperator */
  box = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (box), 6);
  gtk_widget_show (box);
  gtk_box_pack_start (GTK_BOX (wet_dry->control_box), box, FALSE, TRUE, 0);
  
  sep = gtk_hseparator_new ();
  gtk_widget_show (sep);
  gtk_box_pack_start (GTK_BOX (box), sep, FALSE, TRUE, 0);
  
  wet_dry->controls = g_malloc (sizeof (GtkWidget *) * wet_dry->plugin_slot->jack_rack->channels);
  if (wet_dry->plugin_slot->jack_rack->channels > 1)
    {
      GtkWidget * control_box;
      GtkWidget * control_and_lock_box;
      char * label;
      unsigned long channel;
      
      /* control and lock box */
      control_and_lock_box = gtk_hbox_new (FALSE, 0);
      gtk_widget_show (control_and_lock_box);
      gtk_box_pack_start (GTK_BOX (wet_dry->control_box), control_and_lock_box, TRUE, TRUE, 0);
  
      /* control box */
      box = gtk_vbox_new (FALSE, 0);
      gtk_widget_show (box);
      gtk_box_pack_start (GTK_BOX (control_and_lock_box), box, TRUE, TRUE, 0);

      /* lock */
      wet_dry->lock = gtk_toggle_button_new_with_label (_("Lock"));
      gtk_widget_show (wet_dry->lock);
      gtk_box_pack_start (GTK_BOX (control_and_lock_box), wet_dry->lock, FALSE, TRUE, 0);
      g_signal_connect (G_OBJECT (wet_dry->lock), "toggled",
                        G_CALLBACK (slot_wet_dry_lock_cb), wet_dry);
  
      wet_dry->labels =   g_malloc (sizeof (GtkWidget *) * wet_dry->plugin_slot->jack_rack->channels);
   
      for (channel = 0; channel < wet_dry->plugin_slot->jack_rack->channels; channel++)
        {
          /* the control's box */
          control_box = gtk_hbox_new (FALSE, 0);
          gtk_widget_show (control_box);
          gtk_box_pack_start (GTK_BOX (box), control_box, TRUE, TRUE, 0);
      
          /* the label */
          label = g_strdup_printf ("%s %ld", _("Channel"), channel + 1);
          wet_dry->labels[channel] = gtk_label_new (label);
          g_free (label);
          gtk_widget_show (wet_dry->labels[channel]);
          gtk_box_pack_start (GTK_BOX (control_box), wet_dry->labels[channel], FALSE, TRUE, 0);
      
          /* the control itself */
          wet_dry->controls[channel] = wet_dry_create_control (wet_dry, channel);
          gtk_box_pack_start (GTK_BOX (control_box), wet_dry->controls[channel], TRUE, TRUE, 0);
        }
    }
  else
    {
      wet_dry->controls[0] = wet_dry_create_control (wet_dry, 0);
      gtk_box_pack_start (GTK_BOX (wet_dry->control_box), wet_dry->controls[0], TRUE, TRUE, 0);
    }
  
}

wet_dry_controls_t *
wet_dry_controls_new (plugin_slot_t * plugin_slot)
{
  wet_dry_controls_t * wet_dry;
  
  wet_dry = g_malloc (sizeof (wet_dry_controls_t));
  
  wet_dry->plugin_slot = plugin_slot;
  
  /* control fifos */
  wet_dry->fifos = plugin_slot->plugin->wet_dry_fifos;
  
  wet_dry_create_controls (wet_dry);
  
  return wet_dry;
}

void
wet_dry_controls_destroy (wet_dry_controls_t * wet_dry)
{
  g_free (wet_dry->controls);
  if (wet_dry->plugin_slot->jack_rack->channels > 1)
    g_free (wet_dry->labels);
  g_free (wet_dry);
}

void
wet_dry_controls_block_callback (wet_dry_controls_t * wet_dry, unsigned long channel)
{
  gtk_widget_block_signal (wet_dry->controls[channel], "value-changed",
                           G_CALLBACK (slot_wet_dry_control_cb));
}

void
wet_dry_controls_unblock_callback (wet_dry_controls_t * wet_dry, unsigned long channel)
{
  gtk_widget_unblock_signal (wet_dry->controls[channel], "value-changed",
                           G_CALLBACK (slot_wet_dry_control_cb));
}


/* EOF */



