#include "state_tracker/st_api.h"
#include "vg_api.h"

PUBLIC struct st_api *
st_api_create_OpenVG(void)
{
   return (struct st_api *) vg_api_get();
}
