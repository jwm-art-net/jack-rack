#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "types.h"
#include "globals.h"


/**
 * Delete the char at the specified position from a string.
 *
 * @param pos The position, starting from zero.
 * @return A pointer to s
 *
 * TODO: realloc result to strlen(s)-1. 
 */
static char*
str_delete_char ( char* s, const unsigned int pos )
{
	    int j;

	    for (j = pos; s[j] != '\0'; ++j)
		s[j] = s[j + 1];

	    return s;
}


/**
 * Prepare a string for the JACK server.
 * The passed string is modified accordingly.
 *
 * @return a pointer to client_name.
 */
char*
sanitize_client_name ( char* client_name )
{
    unsigned short i;

    for (i = 0; client_name[i] != '\0'; ++i)
    {
	if (client_name[i] == ' ')
	    client_name[i] = '_';

	else if (!isalnum (client_name[i]))
	    str_delete_char ( client_name, i );

 	else if (isupper (client_name[i]))
	    client_name[i] = tolower (client_name[i]);
    }

    return (client_name);
}


/**
 * Provides simple console status logging.
 *
 * @todo: add "last message repeated n times'' functionality.
 */
void
default_status_handler (void* data, const char* format, ...)
{
	gchar* msg;
	va_list args;
	va_start(args, format);
	msg = g_strdup_vprintf (format, args);

	printf("Status update: %s\n", msg);

	g_free (msg);
}


/**
 * Provides simple console error logging.
 *
 * @todo: add "last message repeated n times'' functionality.
 */
void
default_error_handler (void* data, const error_severity_t severity,
                       const char* format, ...)
{
	gchar* msg;
	va_list args;
	va_start (args, format);
	msg = g_strdup_vprintf (format, args);

	switch (severity)
	{
		case E_NOTICE:
			printf (_("Notice: %s\n"), msg);
			break;

		case E_WARNING:
			printf (_("Warning: %s\n"), msg);
			break;

		case E_ERROR:
			printf (_("Error: %s\n"), msg);
			break;

		case E_FATAL:
			printf (_("FATAL ERROR: %s\n"), msg);
			g_free (msg);
			abort();
			break;
			
		case E_BUG:
			printf (_("BUG: %s\n"), msg);
			break;
	}

	g_free (msg);
}
