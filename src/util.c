#include "util.h"
#include "types.h"

static char*
str_delete_char ( char* s, const unsigned int pos )
{
	    int j;

	    for (j = pos; s[j] != '\0'; ++j)
		s[j] = s[j + 1];

	    return s;
}


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


void
default_status_handler (const char* format, ...)
{
	gchar* msg;
	va_list args;
	va_start(args, format);
	msg = g_strdup_vprintf (format, args);

	g_printf("Status update: %s\n", msg);

	g_free (msg);
}


void
default_error_handler (const error_severity_t severity, const char* format, ...)
{
	gchar* msg;
	va_list args;
	va_start (args, format);
	msg = g_strdup_vprintf (format, args);

	switch (severity)
	{
		case E_NOTICE:
			printf ("Notice: %s\n", msg);
			break;

		case E_WARNING:
			printf ("Warning: %s\n", msg);
			break;

		case E_ERROR:
			printf ("Error: %s\n", msg);
			break;

		case E_FATAL:
			printf ("FATAL ERROR: %s\n", msg);
			g_free (msg);
			abort();
			break;
			
		case E_BUG:
			printf ("BUG: ", msg);
			break;
	}

	g_free (msg);
}
