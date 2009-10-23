#include "intel_context.h"
#include "intel_tex.h"
#include "intel_chipset.h"




int intel_compressed_num_bytes(GLuint mesaFormat)
{
   int bytes = 0;
   switch(mesaFormat) {
     
   case MESA_FORMAT_RGB_FXT1:
   case MESA_FORMAT_RGBA_FXT1:
   case MESA_FORMAT_RGB_DXT1:
   case MESA_FORMAT_RGBA_DXT1:
     bytes = 2;
     break;
     
   case MESA_FORMAT_RGBA_DXT3:
   case MESA_FORMAT_RGBA_DXT5:
     bytes = 4;
   default:
     break;
   }
   
   return bytes;
}
