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

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include <gtk/gtk.h>

#include "plugin_slot.h"
#include "port_controls.h"
#include "globals.h"
#include "control_callbacks.h"
#include "midi_control.h"
#include "jack_rack.h"

#define TEXT_BOX_WIDTH        75
#define TEXT_BOX_CHARS        -1


/** create a control for float ports */
static GtkWidget *
create_float_control (plugin_desc_t * desc, unsigned long port_index)
{
  LADSPA_Data lower;
  LADSPA_Data upper;
  GtkWidget *widget;

  if (LADSPA_IS_HINT_SAMPLE_RATE (desc->port_range_hints[port_index].HintDescriptor))
    {
      lower =
        desc->port_range_hints[port_index].LowerBound *
        (LADSPA_Data) sample_rate;
      upper =
        desc->port_range_hints[port_index].UpperBound *
        (LADSPA_Data) sample_rate;
    }
  else
    {
      lower = desc->port_range_hints[port_index].LowerBound;
      upper = desc->port_range_hints[port_index].UpperBound;
    }
  
  if (!LADSPA_IS_HINT_BOUNDED_BELOW
      (desc->port_range_hints[port_index].HintDescriptor))
    {
      lower = (LADSPA_Data) - 100.0;
    }

  if (!LADSPA_IS_HINT_BOUNDED_ABOVE
      (desc->port_range_hints[port_index].HintDescriptor))
    {
      upper = (LADSPA_Data) 100.0;
    }

  if (LADSPA_IS_HINT_LOGARITHMIC (desc->port_range_hints[port_index].HintDescriptor))
    {
      if (lower < FLT_EPSILON)
        lower = FLT_EPSILON;
        
      lower = log (lower);
      upper = log (upper);
      
    }


/*  printf ("%s: lower: %f, upper: %f, %s\n", __FUNCTION__, lower, upper,
  ›       LADSPA_IS_HINT_SAMPLE_RATE (desc->port_range_hints[port_index].HintDescriptor)
          ? "sample rate adjusted" : "normal"); */

  if (!(lower < upper))
    {
      printf ("%s: lower !< upper!\n", __FUNCTION__);
    }
  widget = gtk_hscale_new_with_range ((gdouble) lower, (gdouble) upper, (upper - lower) / 10.0);
  gtk_scale_set_draw_value (GTK_SCALE (widget), FALSE);
  gtk_scale_set_digits (GTK_SCALE (widget), 8);
  gtk_range_set_increments (GTK_RANGE (widget), (upper - lower) / 1000.0,
                            (upper - lower) / 10.0);
  
  g_assert (widget != NULL);
  
  return widget;
}

/** create a control for int ports */
static GtkWidget *
create_int_control (plugin_desc_t * desc, unsigned long port_index)
{
  int lower = 0, upper = 0;
  GtkWidget *widget;

  if (LADSPA_IS_HINT_SAMPLE_RATE
      (desc->port_range_hints[port_index].HintDescriptor))
    {
      lower =
        desc->port_range_hints[port_index].LowerBound *
        (LADSPA_Data) sample_rate;
      upper =
        desc->port_range_hints[port_index].UpperBound *
        (LADSPA_Data) sample_rate;
    }
  else
    {
      lower = desc->port_range_hints[port_index].LowerBound;
      upper = desc->port_range_hints[port_index].UpperBound;
    }

  if (!LADSPA_IS_HINT_BOUNDED_BELOW
      (desc->port_range_hints[port_index].HintDescriptor))
    {
      lower = -100.0;
    }

  if (!LADSPA_IS_HINT_BOUNDED_ABOVE
      (desc->port_range_hints[port_index].HintDescriptor))
    {
      upper = 100.0;
    }

  if (!(lower < upper))
    {
/*    printf("%s: lower !< upper! (%f, %f, %s, %s, %s)\n", __FUNCTION__,
           desc->port_range_hints[port_index].LowerBound,
           desc->port_range_hints[port_index].UpperBound,
           LADSPA_IS_HINT_SAMPLE_RATE (desc->port_range_hints[port_index].HintDescriptor) ? "normal" : "sample rate",
           LADSPA_IS_HINT_BOUNDED_BELOW (desc->port_range_hints[port_index].HintDescriptor) ? "use lower" : "do not use lower",
           LADSPA_IS_HINT_BOUNDED_ABOVE (desc->port_range_hints[port_index].HintDescriptor) ? "use upper" : "do not use upper"
           );*/
      if (!LADSPA_IS_HINT_BOUNDED_ABOVE
          (desc->port_range_hints[port_index].HintDescriptor))
        {
          lower = upper - 100;
        }
      else
        {
          upper = lower + 100;
        }
    }
  widget =
    gtk_spin_button_new_with_range ((gdouble) lower, (gdouble) upper, 1.0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (widget), TRUE);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (widget), 0);

  return widget;
}

/** creates a string for the a control port label, possibly splitting
    from "foo (bar=baz,foobar=foobaz)" to "foo\nbar=baz\nfoobar=foobaz" */
static const char *
create_port_label (const char *port_name)
{
  static GString *string = NULL;
  gchar *start;
  gchar *str;
  gchar *end;

  if (!string)
    {
      string = g_string_new ("");
    }

  str = g_strdup (port_name);

  start = strchr (str, '(');
  if (start)
    {
      end = strchr (start, '=');
      if (end)
        {
          *(start - 1) = '\0';
          g_string_printf (string, "%s", str);

          start++;
          do
            {
              end = strchr (start, ',');
              if (!end)
                {
                  end = strchr (start, ')');
                  if (end)
                    {
                      *end = '\0';
                    }

                  g_string_append_printf (string, "\n%s", start);
                  g_free (str);
                  return string->str;
                }

              *end = '\0';
              g_string_append_printf (string, "\n%s", start);

              end++;
              while (*end == ' ')
                end++;
              start = end;
            }
          while (1);
        }
    }

  g_string_printf (string, "%s", port_name);

  g_free (str);

  return string->str;
}

static void
plugin_slot_create_control_table_row (plugin_slot_t * plugin_slot, port_controls_t * port_controls)
{
  guint i, copies;
  plugin_desc_t * desc;
  const LADSPA_Descriptor * descriptor;
  GtkWidget * label;
  GtkWidget * control_box;
  
  desc = plugin_slot->plugin->desc;
  descriptor = plugin_slot->plugin->descriptor;
  copies = plugin_slot->plugin->copies;
  

  if (copies > 1)
    {
      /* lock control */
      port_controls->lock = gtk_toggle_button_new_with_label (_("Lock"));
      g_signal_connect (G_OBJECT (port_controls->lock), "toggled",
                        G_CALLBACK (control_lock_cb), port_controls);
      gtk_table_attach (GTK_TABLE (plugin_slot->control_table),
                        port_controls->lock,
                        2, 3,
                        port_controls->control_index, port_controls->control_index + 1,
                        GTK_FILL, GTK_FILL,
                        0, 0);
    }
    
  /* label */
  label =
    gtk_label_new (create_port_label (descriptor->PortNames[port_controls->port_index]));
  gtk_label_set_justify (GTK_LABEL (label),
                         GTK_JUSTIFY_CENTER);
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (plugin_slot->control_table),
                    label,
                    0, 1, port_controls->control_index, port_controls->control_index + 1,
                    GTK_FILL, GTK_FILL,
                    3, 0);
  
  
  /* control box */
  control_box = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (control_box);
  gtk_table_attach (GTK_TABLE (plugin_slot->control_table),
                    control_box,
                    1, 2,
                    port_controls->control_index, port_controls->control_index + 1,
                    GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL,
                    0, 0);

  for (i = 0; i < copies; i++)
    {
      /* control fifo */
      port_controls->controls[i].control_fifo =
        plugin_slot->plugin->holders[i].ui_control_fifos + port_controls->control_index;

      /* create the control(s) */
      switch (port_controls->type)
        {
        case JR_CTRL_FLOAT:
          port_controls->controls[i].control =
            create_float_control (desc, port_controls->port_index);
          g_signal_connect (G_OBJECT (port_controls->controls[i].control), "value-changed",
                            G_CALLBACK (control_float_cb), port_controls);

          port_controls->controls[i].text = gtk_entry_new ();
          gtk_entry_set_max_length (GTK_ENTRY (port_controls->controls[i].text), TEXT_BOX_CHARS);
          gtk_widget_set_size_request (port_controls->controls[i].
                                       text, TEXT_BOX_WIDTH, -1);
          gtk_widget_show (port_controls->controls[i].text);
          g_object_set_data (G_OBJECT (port_controls->controls[i].text),
                             "jack-rack-plugin-copy", GINT_TO_POINTER (i));
          g_signal_connect (G_OBJECT
                            (port_controls->controls[i].text),
                            "activate",
                            G_CALLBACK (control_float_text_cb),
                            port_controls);
          break;

        case JR_CTRL_INT:
          port_controls->controls[i].control =
            create_int_control (desc, port_controls->port_index);
          g_signal_connect (G_OBJECT
                            (port_controls->controls[i].control),
                            "value-changed",
                            G_CALLBACK (control_int_cb),
                            port_controls);
          port_controls->controls[i].text = NULL;
          break;

        case JR_CTRL_BOOL:
          port_controls->controls[i].control =
            gtk_toggle_button_new_with_label (_("On"));
          g_signal_connect (G_OBJECT
                            (port_controls->controls[i].control),
                            "toggled", G_CALLBACK (control_bool_cb),
                            port_controls);
          port_controls->controls[i].text = NULL;
          break;
        }
      
      gtk_widget_show (port_controls->controls[i].control);
      g_object_set_data (G_OBJECT (port_controls->controls[i].control),
                         "jack-rack-plugin-copy", GUINT_TO_POINTER (i));
      
      g_signal_connect (G_OBJECT (port_controls->controls[i].control), "button-press-event",
                        G_CALLBACK (control_button_press_cb), port_controls);
      
      gtk_box_pack_start (GTK_BOX (control_box), port_controls->controls[i].control,
                          TRUE, TRUE, 0);
      if (port_controls->type == JR_CTRL_FLOAT)
        gtk_box_pack_start (GTK_BOX (control_box), port_controls->controls[i].text,
                            FALSE, TRUE, 0);

    }

}

port_controls_t *
port_controls_new	(plugin_slot_t * plugin_slot)
{
  plugin_desc_t *desc;
  unsigned long i;
  port_controls_t *port_controls_array;
  port_controls_t *port_controls;
  
  desc = plugin_slot->plugin->desc;

  /* make the controls array */
  port_controls_array = g_malloc (sizeof (port_controls_t) * desc->control_port_count);

  /* contruct the control rows */
  for (i = 0; i < desc->control_port_count; i++)
    {
      port_controls = port_controls_array + i;
      port_controls->lock = NULL;
      port_controls->locked = TRUE;
      port_controls->lock_copy = 0;
      port_controls->plugin_slot = plugin_slot;
      port_controls->control_index = i;
      port_controls->port_index = desc->control_port_indicies[i];

      /* get the port control type from the hints */
      if (LADSPA_IS_HINT_TOGGLED (desc->port_range_hints[port_controls->port_index].HintDescriptor))
        port_controls->type = JR_CTRL_BOOL;
      else if (LADSPA_IS_HINT_INTEGER (desc->port_range_hints[port_controls->port_index].HintDescriptor))
        port_controls->type = JR_CTRL_INT;
      else
        port_controls->type = JR_CTRL_FLOAT;
      
      if (LADSPA_IS_HINT_LOGARITHMIC (desc->port_range_hints[port_controls->port_index].HintDescriptor))
        port_controls->logarithmic = TRUE;
      else
        port_controls->logarithmic = FALSE;
      
      port_controls->controls = g_malloc (sizeof (controls_t) * plugin_slot->plugin->copies);
      
      plugin_slot_create_control_table_row (plugin_slot, port_controls);
    }
  
  return port_controls_array;
}

void
port_control_set_locked (port_controls_t *port_controls, gboolean locked)
{
  GSList *list;
  midi_control_t *midi_ctrl;
  plugin_slot_t *plugin_slot;
  
  plugin_slot = port_controls->plugin_slot;
  
  port_controls->locked = locked;
  settings_set_lock (plugin_slot->settings, port_controls->control_index, locked);
  
  for (list = plugin_slot->midi_controls; list; list = g_slist_next (list))
    {
      midi_ctrl = (midi_control_t *) list->data;
      
      if (midi_ctrl->ladspa_control &&
          midi_ctrl->control.ladspa.control == port_controls->control_index)
        midi_control_set_locked (midi_ctrl, locked);
    }
  
  plugin_slot_show_controls (port_controls->plugin_slot, port_controls->lock_copy);
}

void
gtk_widget_block_signal(GtkWidget *widget, const char *signal, GCallback callback)
{
  g_signal_handlers_block_matched (
    widget,
    G_SIGNAL_MATCH_FUNC,
    g_signal_lookup (signal, G_TYPE_FROM_INSTANCE (widget)),
    0,
    NULL,
    callback,
    NULL);
}

void
port_controls_block_float_callback (port_controls_t *port_controls, guint copy)
{
  gtk_widget_block_signal(port_controls->controls[copy].control, "value-changed",
            G_CALLBACK (control_float_cb));
}

void
port_controls_block_int_callback (port_controls_t * port_controls, guint copy)
{
  gtk_widget_block_signal(port_controls->controls[copy].control, "value-changed",
            G_CALLBACK (control_int_cb));
}

void
port_controls_block_bool_callback (port_controls_t * port_controls, guint copy)
{
  gtk_widget_block_signal(port_controls->controls[copy].control, "toggled",
            G_CALLBACK (control_bool_cb));
}

void
gtk_widget_unblock_signal (GtkWidget *widget, const char *signal, GCallback callback)
{
  g_signal_handlers_unblock_matched (
    widget,
    G_SIGNAL_MATCH_FUNC,
    g_signal_lookup (signal, G_TYPE_FROM_INSTANCE (widget)),
    0,
    NULL,
    callback,
    NULL);
}

void
port_controls_unblock_float_callback (port_controls_t *port_controls, guint copy)
{
  gtk_widget_unblock_signal (port_controls->controls[copy].control, "value-changed",
            G_CALLBACK (control_float_cb));
}

void
port_controls_unblock_int_callback (port_controls_t * port_controls, guint copy)
{
  gtk_widget_unblock_signal (port_controls->controls[copy].control, "value-changed",
            G_CALLBACK (control_int_cb));
}

void
port_controls_unblock_bool_callback (port_controls_t * port_controls, guint copy)
{
  gtk_widget_unblock_signal (port_controls->controls[copy].control, "toggled",
            G_CALLBACK (control_bool_cb));
}


                              

/* EOF */



