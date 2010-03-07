#include "brw_winsys.h"
#include "brw_disasm.h"
#include "brw_structs_dump.h"
#include "brw_structs.h"
#include "intel_decode.h"


void brw_dump_data( unsigned pci_id,
		    enum brw_buffer_data_type data_type,
		    unsigned offset,
		    const void *data,
		    size_t size )
{
   if (BRW_DUMP & DUMP_ASM) {
      switch (data_type) {
      case BRW_DATA_GS_WM_PROG:
      case BRW_DATA_GS_SF_PROG:
      case BRW_DATA_GS_VS_PROG:
      case BRW_DATA_GS_GS_PROG:
      case BRW_DATA_GS_CLIP_PROG:
         brw_disasm( stderr, data, size / sizeof(struct brw_instruction) );
         break;
      default:
         break;
      }
   }

   if (BRW_DUMP & DUMP_STATE) {
      switch (data_type) {
      case BRW_DATA_GS_CC_VP:
         brw_dump_cc_viewport( data );
         break;
      case BRW_DATA_GS_CC_UNIT:
         brw_dump_cc_unit_state( data );
         break;
      case BRW_DATA_GS_SAMPLER_DEFAULT_COLOR:
         brw_dump_sampler_default_color( data );
         break;
      case BRW_DATA_GS_SAMPLER:
         brw_dump_sampler_state( data );
         break;
      case BRW_DATA_GS_WM_UNIT:
         brw_dump_wm_unit_state( data );
         break;
      case BRW_DATA_GS_SF_VP:
         brw_dump_sf_viewport( data );
         break;
      case BRW_DATA_GS_SF_UNIT:
         brw_dump_sf_unit_state( data );
         break;
      case BRW_DATA_GS_VS_UNIT:
         brw_dump_vs_unit_state( data );
         break;
      case BRW_DATA_GS_GS_UNIT:
         brw_dump_gs_unit_state( data );
         break;
      case BRW_DATA_GS_CLIP_VP:
         brw_dump_clipper_viewport( data );
         break;
      case BRW_DATA_GS_CLIP_UNIT:
         brw_dump_clip_unit_state( data );
         break;
      case BRW_DATA_SS_SURFACE:
         brw_dump_surface_state( data );
         break;
      case BRW_DATA_SS_SURF_BIND:
         break;
      case BRW_DATA_OTHER:
         break;
      case BRW_DATA_CONSTANT_BUFFER:
         break;
      default:
         break;
      }
   }

   if (BRW_DUMP & DUMP_BATCH) {
      switch (data_type) {
      case BRW_DATA_BATCH_BUFFER:
         intel_decode(data, size / 4, offset, pci_id);
         break;
      default:
         break;
      }
   }
}

