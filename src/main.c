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

#include "ac_config.h"

#include <gtk/gtk.h>
#define _GNU_SOURCE
#include <getopt.h>
#undef _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <glib.h>

#ifdef HAVE_LADCCA
#include <ladcca/ladcca.h>
#endif

#ifdef HAVE_GNOME
#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>
#endif

#ifdef HAVE_XML
#include <libxml/tree.h>
#define XML_COMPRESSION_LEVEL 5
#endif

#include "ui.h"
#include "ac_config.h"
#include "callbacks.h"
#include "globals.h"

ui_t *        global_ui        = NULL;
unsigned long global_channels  = 2;

gboolean connect_inputs = FALSE;
gboolean connect_outputs = FALSE;

#ifdef HAVE_LADCCA
cca_client_t * global_cca_client;
#endif

#define CLIENT_NAME_BASE      "JACK Rack"
#define JACK_CLIENT_NAME_BASE "jack_rack"

void print_help () {
  printf("%s version %s\n", PACKAGE_NAME, PACKAGE_VERSION);
  printf("Copyright (C) 2002 Robert Ham (node@users.sourceforge.net)\n");
  printf("\n");
  printf("This program comes with ABSOLUTELY NO WARRANTY.  You are licensed to use it\n");
  printf("under the terms of the GNU General Public License, version 2 or later.  See\n");
  printf("the COPYING file that came with this software for details.\n");
  printf("\n");
  printf(_("Compiled with:\n"));
  printf(  "  JACK %s\n", JACK_VERSION);
#ifdef HAVE_ALSA
  printf(  "  ALSA %s\n", ALSA_VERSION);
#endif
#ifdef HAVE_LADCCA
  printf(  "  LADCCA %s\n", LADCCA_VERSION);
#endif
#ifdef HAVE_XML
  printf(  "  libxml2 %s\n", XML_VERSION);
#endif
#ifdef HAVE_GNOME
  printf(  "  GNOME %s\n", GNOME_VERSION);
#endif
  printf("\n");
  printf(" -h, --help                  %s\n", _("Display this help info"));
  printf("\n");
  printf(" -p, --pid-name              %s\n", _("Use the pid in the JACK client name (default)"));
  printf(" -s, --string-name <string>  %s\n", _("Use <string> in the JACK client name"));
  printf(" -n, --name                  %s\n", _("Use just jack_rack as the client name"));
  printf("\n");
  printf(" -i, --input                 %s\n", _("Connected inputs to the first two physical capture ports"));
  printf(" -o, --output                %s\n", _("Connected outputs to the first two physical playback ports"));
  printf(" -c, --channels <int>        %s\n", _("How many input and output channels the rack should use (default: 2)"));
  printf("\n");
#ifdef HAVE_JACK_SET_SERVER_DIR
  printf(" -D, --tmpdir <dir>          %s\n", _("Tell JACK to use <dir> for its temporary files"));
  printf("\n");
#endif
}

int main (int argc, char ** argv) {
  char * client_name = NULL;
  unsigned long channels = 2;
  int opt;

#ifdef HAVE_JACK_SET_SERVER_DIR
  const char * options = "hps:ionc:D:";
#else
  const char * options = "hps:ionc:";
#endif /* HAVE_JACK_SET_SERVER_DIR */

  struct option long_options[] = {
    { "help", 0, NULL, 'h' },
    { "pid-name", 0, NULL, 'p' },
    { "string-name", 1, NULL, 's' },
    { "name", 1, NULL, 'n' },
    { "channels", 1, NULL, 'c' },
#ifdef HAVE_JACK_SET_SERVER_DIR
    { "tmpdir", 1, NULL, 'D' },
#endif
    { "input", 0, NULL, 'i' },
    { "output", 0, NULL, 'o' },
    { 0, 0, 0, 0 }
  };
  
  /* fuck the gnome popt bollocks */
  for (opt = 1; opt < argc; opt++)
    if (strcmp (argv[opt], "-h") == 0 ||
        strcmp (argv[opt], "--help") == 0)
      {
        print_help ();
        exit (0);
      }

#ifdef HAVE_LADCCA
  char * ladcca_client_name = NULL;
  cca_args_t * cca_args;
  cca_event_t * cca_event;
#endif  


#ifdef HAVE_LADCCA
  cca_args = cca_extract_args (&argc, &argv);
#endif  

  gtk_set_locale ();
  gtk_init (&argc, &argv);

#ifdef HAVE_GNOME
  gnome_program_init (PACKAGE, VERSION, LIBGNOMEUI_MODULE,
                      argc, argv, NULL);
#endif  
  
  while ((opt = getopt_long (argc, argv, options, long_options, NULL)) != -1) {
    switch (opt) {

      case 'h':
        print_help ();
        exit (0);
        break;

      case 's':
        if (!client_name) client_name = g_strdup_printf ("%s_%s", JACK_CLIENT_NAME_BASE, optarg);
#ifdef HAVE_LADCCA
        if (!ladcca_client_name) ladcca_client_name = g_strdup_printf ("%s (%s)", CLIENT_NAME_BASE, optarg);
#endif
        break;

      case 'p':
        if (!client_name) client_name = g_strdup_printf ("%s_%d", JACK_CLIENT_NAME_BASE, getpid());
#ifdef HAVE_LADCCA
        if (!ladcca_client_name) ladcca_client_name = g_strdup_printf ("%s (%d)", CLIENT_NAME_BASE, getpid());
#endif
        break;
      
      case 'n':
        if (!client_name) client_name = g_strdup (JACK_CLIENT_NAME_BASE);
#ifdef HAVE_LADCCA
        if (!ladcca_client_name) ladcca_client_name = g_strdup_printf (CLIENT_NAME_BASE);
#endif

#ifdef HAVE_JACK_SET_SERVER_DIR
      case 'D':
        jack_set_server_dir (optarg);
        break;
#endif

      case 'i':
        connect_inputs = TRUE;
        break;

      case 'o':
        connect_outputs = TRUE;
        break;
      
      case 'c':
        channels = atof (optarg);
        if (channels < 1)
          {
            fprintf (stderr, _("there must be at least one channel\n"));
            exit (1);
          }
        break;

      case ':':
      case '?':
        print_help ();
        exit (1);
        break;
    }
  }
  

#ifdef HAVE_LADCCA
  if (!ladcca_client_name)
    ladcca_client_name = g_strdup_printf ("%s (%d)", CLIENT_NAME_BASE, getpid());

  global_cca_client = cca_init (cca_args, "JACK Rack" , CCA_Use_Jack|CCA_Config_File, CCA_PROTOCOL (1,0));

  if (cca_enabled (global_cca_client))
    {
      cca_event = cca_event_new_with_type (CCA_Client_Name);
      cca_event_set_string (cca_event, ladcca_client_name);
      cca_send_event (global_cca_client, cca_event);
    }
#endif

#ifdef HAVE_XML
  xmlSetCompressMode (XML_COMPRESSION_LEVEL);
#endif

  if (!client_name)
    client_name = g_strdup_printf ("%s_%d", JACK_CLIENT_NAME_BASE, getpid());

  global_ui = ui_new (client_name, channels);
  g_free (client_name);
  
  
  jack_on_shutdown (global_ui->procinfo->jack_client,
                    jack_shutdown_cb,
                    &global_ui);
  
  jack_activate (global_ui->procinfo->jack_client);
  
  gtk_main ();

/*  jack_deactivate (global_ui->procinfo->jack_client); */
  
  ui_destroy (global_ui);
  
  return 0;
}

