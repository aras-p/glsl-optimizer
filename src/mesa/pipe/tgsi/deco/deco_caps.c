#include "tgsi_platform.h"
#include "tgsi_deco.h"

void
tgsi_deco_caps_init(
   struct tgsi_deco_caps *caps )
{
   memset( caps, 0, sizeof( *caps ) );
}

