/*
 *   jack-ladspa-host
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

#include <gtk/gtk.h>
#define _GNU_SOURCE
#include <getopt.h>
#undef _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <glib.h>
#include <signal.h>
#include <locale.h>

#include <libxml/tree.h>
#define XML_COMPRESSION_LEVEL 5

#include "ui.h"
#include "globals.h"

ui_t *        global_ui        = NULL;

gboolean connect_inputs = FALSE;
gboolean connect_outputs = FALSE;
gboolean time_runs = TRUE;
GString  *client_name = NULL;
GString  *session_uuid = NULL;
GString  *initial_filename = NULL;

#ifdef HAVE_LASH
lash_client_t * global_lash_client;
#endif

#define CLIENT_NAME_BASE      "JACK Rack"

void print_help (void) {
  printf("%s version %s\n", PACKAGE_NAME, PACKAGE_VERSION);
  printf("%s\n", COPYRIGHT_MESSAGE);
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
#ifdef HAVE_LASH
  printf(  "  LASH %s\n", LASH_VERSION);
#endif
  printf(  "  libxml2 %s\n", XML_VERSION);
  printf("\n");
  printf(_("Usage: jack-rack [OPTION]..."));
  printf(" [file]\n");
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
  printf(" -U, --jack-session-uuid     %s\n", _("For use by JACK session manager"));
/*  printf(" -t, --no-time               %s\n", _("Do not display plugins' execution time")); */
  printf("\n");
}

int main (int argc, char ** argv) {
  unsigned long channels = 2;
  int opt;

  const char * options = "hps:ionc:tD:";

  struct option long_options[] = {
    { "help", 0, NULL, 'h' },
    { "pid-name", 0, NULL, 'p' },
    { "string-name", 1, NULL, 's' },
    { "name", 1, NULL, 'n' },
    { "channels", 1, NULL, 'c' },
    { "input", 1, NULL, 'i' },
    { "output", 1, NULL, 'o' },
    { "no-time", 0, NULL, 't' },
    { "jack-session-uuid", 1, NULL, 'U' },
    { 0, 0, 0, 0 }
  };

#ifdef HAVE_LASH
  lash_args_t * lash_args;
  lash_event_t * lash_event;
#endif  

  gtk_set_locale ();
#ifdef ENABLE_NLS
  bindtextdomain (PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
#endif
  
  /* not using gnome popt */
  for (opt = 1; opt < argc; opt++)
    {
      if (strcmp (argv[opt], "-h") == 0 ||
          strcmp (argv[opt], "--help") == 0)
        {
          print_help ();
          exit (0);
        }
    }


#ifdef HAVE_LASH
  lash_args = lash_extract_args (&argc, &argv);
#endif  

  gtk_init (&argc, &argv);


  /* set the client name */
  client_name = g_string_new ("");
  session_uuid = g_string_new ("");
  g_string_printf (client_name, "%s", CLIENT_NAME_BASE);
  
  while ((opt = getopt_long (argc, argv, options, long_options, NULL)) != -1) {
    switch (opt) {

      case 'h':
        print_help ();
        exit (0);
        break;

      case 's':
        g_string_printf (client_name, "%s (%s)", CLIENT_NAME_BASE, optarg);
        break;

      case 'p':
        g_string_printf (client_name, "%s (%d)", CLIENT_NAME_BASE, getpid());
        break;
      
      case 'n':
        g_string_printf (client_name, CLIENT_NAME_BASE);
        break;

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
            fprintf (stderr, _("There must be at least one channel\n"));
            exit (EXIT_FAILURE);
          }
        break;
      
      case 't':
        time_runs = FALSE;
        break;

      case 'U':
        g_string_printf (session_uuid, "%s", optarg);
	break;

      case ':':
      case '?':
        print_help ();
        exit (EXIT_FAILURE);
        break;
    }
  }

  if (optind < argc)
  {
    if (argc - optind > 1)
    {
      fprintf (stderr, _("Please specify only one file to open"));
      exit(EXIT_FAILURE);
    }

    initial_filename = g_string_new(argv[optind]);
  }

#ifdef HAVE_LASH
  {
    int flags = LASH_Config_File;
    global_lash_client = lash_init (lash_args, PACKAGE_NAME, flags, LASH_PROTOCOL (2,0));
  }

  if (global_lash_client)
    {
      lash_event = lash_event_new_with_type (LASH_Client_Name);
      lash_event_set_string (lash_event, client_name->str);
      lash_send_event (global_lash_client, lash_event);
    }
#endif /* HAVE_LASH */

  xmlSetCompressMode (XML_COMPRESSION_LEVEL);

  global_ui = ui_new (channels);
  if (!global_ui)
    return 1;
  
  /* ignore the sighup (the jack client thread needs to deal with it) */
  signal (SIGHUP, SIG_IGN);
  
/*  jack_activate (global_ui->procinfo->jack_client); */
  
  gtk_main ();

/*  jack_deactivate (global_ui->procinfo->jack_client); */
  
  ui_destroy (global_ui);
  
  return 0;
}

