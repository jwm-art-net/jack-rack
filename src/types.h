#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

typedef enum
{
	E_NOTICE,
	E_WARNING,
	E_ERROR,
	E_FATAL,
	E_BUG
} error_severity_t;

typedef	void (*status_callback)(void* data, const char*, ...);
typedef	void (*error_callback)(void* data, const error_severity_t, const char*, ...);

#endif
