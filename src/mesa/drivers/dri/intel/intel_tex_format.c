#include "intel_context.h"
#include "intel_tex.h"
#include "main/enums.h"
#include "main/formats.h"

int intel_compressed_num_bytes(GLuint mesaFormat)
{
   GLuint bw, bh;
   GLuint block_size;

   block_size = _mesa_get_format_bytes(mesaFormat);
   _mesa_get_format_block_size(mesaFormat, &bw, &bh);

   return block_size / bw;
}
