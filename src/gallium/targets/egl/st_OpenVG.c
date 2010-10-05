#include "vg_api.h"
#include "egl.h"

PUBLIC struct st_api *
st_api_create_OpenVG(void)
{
   return (struct st_api *) vg_api_get();
}
