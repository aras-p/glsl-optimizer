#include <stdlib.h>
#include <stdio.h>
#include <float.h>

#include "util/u_math.h"
#include "util/u_half.h"

int
main(int argc, char **argv)
{
   unsigned i;
   unsigned roundtrip_fails = 0;
   for(i = 0; i < 1 << 16; ++i)
   {
      uint16_t h = (uint16_t) i;
      union fi f;
      uint16_t rh;
      f.ui = util_half_to_floatui(h);
      rh = util_floatui_to_half(f.ui);
      if(h != rh)
      {
	 printf("Roundtrip failed: %x -> %x = %f -> %x\n", h, f.ui, f.f, rh);
	 ++roundtrip_fails;
      }
   }

   if(roundtrip_fails)
      printf("Failure! %u/65536 half floats failed a conversion to float and back.\n", roundtrip_fails);
   else
      printf("Success!\n");
   return 0;
}
