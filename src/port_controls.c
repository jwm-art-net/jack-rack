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

#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>

#include "plugin_slot.h"
#include "port_controls.h"
#include "callbacks.h"

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

/*  printf ("%s: lower: %f, upper: %f, %s\n", __FUNCTION__, lower, upper,
          LADSPA_IS_HINT_SAMPLE_RATE (desc->port_range_hints[port_index].HintDescriptor)
          ? "sample rate adjusted" : "normal");*/

  if (!(lower < upper))
    {
      printf ("%s: lower !< upper!\n", __FUNCTION__);
    }
  widget =
    gtk_hscale_new_with_range ((gdouble) lower, (gdouble) upper,
                               (upper - lower) / 10.0);
  gtk_scale_set_draw_value (GTK_SCALE (widget), FALSE);
  gtk_scale_set_digits (GTK_SCALE (widget), 8);
  gtk_range_set_increments (GTK_RANGE (widget), (upper - lower) / 1000.0,
                            (upper - lower) / 10.0);
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
  gint i;
  plugin_desc_t * desc;
  const LADSPA_Descriptor * descriptor;
  GtkWidget * label;
  unsigned long copies;
  
  desc = plugin_slot->plugin->desc;
  descriptor = plugin_slot->plugin->descriptor;
  copies = plugin_slot->plugin->copies;

  if (copies > 1)
    {
      /* lock control */
      port_controls->locked = TRUE;
      port_controls->lock = gtk_toggle_button_new_with_label ("Lock");
      gtk_widget_show (port_controls->lock);
      g_signal_connect (G_OBJECT (port_controls->lock), "toggled",
                        G_CALLBACK (control_lock_cb), port_controls);
      gtk_table_attach (GTK_TABLE (plugin_slot->control_table),
                        port_controls->lock,
                        copies * 2,  copies * 2 + 1,
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

  for (i = 0; i < copies; i++)
    {
      /* control fifo */
      port_controls->controls[i].control_fifo =
        plugin_slot->plugin->holders[i].control_fifos + port_controls->control_index;


      /* create the controls */
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
            gtk_toggle_button_new_with_label ("On");
          g_signal_connect (G_OBJECT
                            (port_controls->controls[i].control),
                            "toggled", G_CALLBACK (control_bool_cb),
                            port_controls);
          port_controls->controls[i].text = NULL;
          break;
        }

      g_object_set_data (G_OBJECT
                         (port_controls->controls[i].control),
                         "jack-rack-plugin-copy", GINT_TO_POINTER (i));

      gtk_widget_show (port_controls->controls[i].control);

      /* pack the controls */
      switch (port_controls->type)
        {
        case JR_CTRL_FLOAT:
          gtk_table_attach (GTK_TABLE (plugin_slot->control_table),
                            port_controls->controls[i].control,
                            /* left-most column */ (i * 3) + 1,
                            /* right-most column */ (i * 3) + 2,
                            /* upper-most row */ port_controls->control_index,
                            /* lower-most row */ port_controls->control_index + 1,
                            /* x pack options */ GTK_EXPAND | GTK_FILL,
                            /* y pack options */ 0,
                            /* x padding */ 0,
                            /* y padding */ 0);
          gtk_table_attach (GTK_TABLE (plugin_slot->control_table),
                            port_controls->controls[i].text,
                            /* left-most column */ (i * 3) + 2,
                            /* right-most column */ (i * 3) + 3,
                            /* upper-most row */ port_controls->control_index,
                            /* lower-most row */ port_controls->control_index + 1,
                            /* x pack options */ GTK_FILL,
                            /* y pack options */ 0,
                            /* x padding */ 0,
                            /* y padding */ 0);
          break;

        case JR_CTRL_INT:
        case JR_CTRL_BOOL:
          gtk_table_attach (GTK_TABLE (plugin_slot->control_table),
                            port_controls->controls[i].control,
                            /* left-most column */ (i * 3) + 1,
                            /* right-most column */ (i * 3) + 3,
                            /* upper-most row */ port_controls->control_index,
                            /* lower-most row */ port_controls->control_index + 1,
                            /* x pack options */ GTK_EXPAND | GTK_FILL,
                            /* y pack options */ 0,
                            /* x padding */ 0,
                            /* y padding */ 0);
          break;
        }
    }

}

port_controls_t *
port_controls_new	(struct _plugin_slot * plugin_slot)
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
      port_controls->plugin_slot = plugin_slot;
      port_controls->control_index = i;
      port_controls->port_index = desc->control_port_indicies[i];

      /* get the port control type from the hints */
      if (LADSPA_IS_HINT_TOGGLED (desc->port_range_hints[desc->control_port_indicies[i]].HintDescriptor))
        port_controls->type = JR_CTRL_BOOL;
      else if (LADSPA_IS_HINT_INTEGER (desc->port_range_hints[desc->control_port_indicies[i]].HintDescriptor))
        port_controls->type = JR_CTRL_INT;
      else
        port_controls->type = JR_CTRL_FLOAT;
      
      
      plugin_slot_create_control_table_row (plugin_slot, port_controls);
    }
  
  plugin_slot->port_controls = port_controls_array;
  
  return port_controls_array;
}
