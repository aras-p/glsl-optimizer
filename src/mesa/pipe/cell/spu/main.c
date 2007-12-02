/* main.c for cell SPU code */


#include <stdio.h>
#include <libmisc.h>
#include <spu_mfcio.h>

#include "tri.h"
#include "pipe/cell/common.h"


static struct init_info init;


int
main(unsigned long long speid,
     unsigned long long argp,
     unsigned long long envp)
{
   int tag = 0;

   mfc_get(&init, (unsigned int) argp, sizeof(struct init_info), tag, 0, 0);

   printf("Enter spu main(): init.foo=%d\n", init.foo);

   draw_triangle(0, 1, 2);

   return 0;
}
