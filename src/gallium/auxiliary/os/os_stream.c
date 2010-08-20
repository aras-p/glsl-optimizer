#include "pipe/p_config.h"

#include "os_stream.h"
#include "util/u_memory.h"
#include "util/u_string.h"

int
os_default_stream_vprintf (struct os_stream* stream, const char *format, va_list ap)
{
   char buf[1024];
   int retval;

   retval = util_vsnprintf(buf, sizeof(buf), format, ap);
   if(retval <= 0)
   {}
   else if(retval < sizeof(buf))
      stream->write(stream, buf, retval);
   else
   {
      int alloc = sizeof(buf);
      char* str = NULL;
      for(;;)
      {
         alloc += alloc;
         if(str)
            FREE(str);
         str = MALLOC(alloc);
         if(!str)
            return -1;

         retval = util_vsnprintf(str, alloc, format, ap);
      } while(retval >= alloc);

      if(retval > 0)
         stream->write(stream, str, retval);
      FREE(str);
   }

   return retval;
}
