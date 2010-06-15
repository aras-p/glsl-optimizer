#ifndef EGLSTRING_INCLUDED
#define EGLSTRING_INCLUDED

#include <string.h>

#ifdef _EGL_OS_WINDOWS
#define _eglstrcasecmp _stricmp
#define _eglsnprintf _snprintf
#else
#define _eglstrcasecmp strcasecmp
#define _eglsnprintf snprintf
#endif

extern char *
_eglstrdup(const char *s);


#endif /* EGLSTRING_INCLUDED */
