#include "pipe/p_config.h"

#include "os_stream.h"
#include "util/u_memory.h"
#include "util/u_string.h"

int
os_default_stream_vprintf (struct os_stream* stream, const char *format, va_list ap)
{
   char buf[1024];
   int retval;
   va_list ap2;
   va_copy(ap2, ap);
   retval = util_vsnprintf(buf, sizeof(buf), format, ap2);
   va_end(ap2);
   if(retval <= 0)
   {}
   else if(retval < sizeof(buf))
      stream->write(stream, buf, retval);
   else
   {
      char* str = MALLOC(retval + 1);
      if(!str)
         return -1;
      retval = util_vsnprintf(str, retval + 1, format, ap);
      if(retval > 0)
         stream->write(stream, str, retval);
      FREE(str);
   }

   return retval;
}
