#include <assert.h>
#include <X11/Xlib.h>
#include <X11/extensions/XvMClib.h>
#include <util/u_memory.h>
#include "xvmc_private.h"

Status XvMCCreateBlocks(Display *dpy, XvMCContext *context, unsigned int num_blocks, XvMCBlockArray *blocks)
{
   assert(dpy);

   if (!context)
      return XvMCBadContext;
   if (num_blocks == 0)
      return BadValue;

   assert(blocks);

   blocks->context_id = context->context_id;
   blocks->num_blocks = num_blocks;
   blocks->blocks = MALLOC(BLOCK_SIZE_BYTES * num_blocks);
   blocks->privData = NULL;

   return Success;
}

Status XvMCDestroyBlocks(Display *dpy, XvMCBlockArray *blocks)
{
   assert(dpy);
   assert(blocks);
   FREE(blocks->blocks);

   return Success;
}

Status XvMCCreateMacroBlocks(Display *dpy, XvMCContext *context, unsigned int num_blocks, XvMCMacroBlockArray *blocks)
{
   assert(dpy);

   if (!context)
      return XvMCBadContext;
   if (num_blocks == 0)
      return BadValue;

   assert(blocks);

   blocks->context_id = context->context_id;
   blocks->num_blocks = num_blocks;
   blocks->macro_blocks = MALLOC(sizeof(XvMCMacroBlock) * num_blocks);
   blocks->privData = NULL;

   return Success;
}

Status XvMCDestroyMacroBlocks(Display *dpy, XvMCMacroBlockArray *blocks)
{
   assert(dpy);
   assert(blocks);
   FREE(blocks->macro_blocks);

   return Success;
}
