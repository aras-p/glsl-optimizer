/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

/**
 * @file
 * SVGA Shader Dump Facilities
 * 
 * @author Michal Krol <michal@vmware.com>
 */

#include "svga_shader.h"
#include "svga_shader_dump.h"
#include "svga_shader_op.h"
#include "util/u_debug.h"

#include "../svga_hw_reg.h"
#include "svga3d_shaderdefs.h"

struct dump_info
{
   SVGA3dShaderVersion version;
   boolean is_ps;
};

static void dump_op( struct sh_op op, const char *mnemonic )
{
   assert( op.predicated == 0 );
   assert( op.is_reg == 0 );

   if (op.coissue)
      debug_printf( "+" );
   debug_printf( "%s", mnemonic );
   switch (op.control) {
   case 0:
      break;
   case SVGA3DOPCONT_PROJECT:
      debug_printf( "p" );
      break;
   case SVGA3DOPCONT_BIAS:
      debug_printf( "b" );
      break;
   default:
      assert( 0 );
   }
}


static void dump_comp_op( struct sh_op op, const char *mnemonic )
{
   assert( op.is_reg == 0 );

   if (op.coissue)
      debug_printf( "+" );
   debug_printf( "%s", mnemonic );
   switch (op.control) {
   case SVGA3DOPCOMP_RESERVED0:
      break;
   case SVGA3DOPCOMP_GT:
      debug_printf("_gt");
      break;
   case SVGA3DOPCOMP_EQ:
      debug_printf("_eq");
      break;
   case SVGA3DOPCOMP_GE:
      debug_printf("_ge");
      break;
   case SVGA3DOPCOMP_LT:
      debug_printf("_lt");
      break;
   case SVGA3DOPCOMPC_NE:
      debug_printf("_ne");
      break;
   case SVGA3DOPCOMP_LE:
      debug_printf("_le");
      break;
   case SVGA3DOPCOMP_RESERVED1:
   default:
      assert( 0 );
   }
}


static void dump_reg( struct sh_reg reg, struct sh_srcreg *indreg, const struct dump_info *di )
{
   assert( sh_reg_type( reg ) == SVGA3DREG_CONST || reg.relative == 0 );
   assert( reg.is_reg == 1 );

   switch (sh_reg_type( reg )) {
   case SVGA3DREG_TEMP:
      debug_printf( "r%u", reg.number );
      break;

   case SVGA3DREG_INPUT:
      debug_printf( "v%u", reg.number );
      break;

   case SVGA3DREG_CONST:
      if (reg.relative) {
         if (sh_srcreg_type( *indreg ) == SVGA3DREG_LOOP)
            debug_printf( "c[aL+%u]", reg.number );
         else
            debug_printf( "c[a%u.x+%u]", indreg->number, reg.number );
      }
      else
         debug_printf( "c%u", reg.number );
      break;

   case SVGA3DREG_ADDR:    /* VS */
   /* SVGA3DREG_TEXTURE */ /* PS */
      if (di->is_ps)
         debug_printf( "t%u", reg.number );
      else
         debug_printf( "a%u", reg.number );
      break;

   case SVGA3DREG_RASTOUT:
      switch (reg.number) {
      case 0 /*POSITION*/:
         debug_printf( "oPos" );
         break;
      case 1 /*FOG*/:
         debug_printf( "oFog" );
         break;
      case 2 /*POINT_SIZE*/:
         debug_printf( "oPts" );
         break;
      default:
         assert( 0 );
         debug_printf( "???" );
      }
      break;

   case SVGA3DREG_ATTROUT:
      assert( reg.number < 2 );
      debug_printf( "oD%u", reg.number );
      break;

   case SVGA3DREG_TEXCRDOUT:
   /* SVGA3DREG_OUTPUT */
      debug_printf( "oT%u", reg.number );
      break;

   case SVGA3DREG_COLOROUT:
      debug_printf( "oC%u", reg.number );
      break;

   case SVGA3DREG_DEPTHOUT:
      debug_printf( "oD%u", reg.number );
      break;

   case SVGA3DREG_SAMPLER:
      debug_printf( "s%u", reg.number );
      break;

   case SVGA3DREG_CONSTBOOL:
      assert( !reg.relative );
      debug_printf( "b%u", reg.number );
      break;

   case SVGA3DREG_CONSTINT:
      assert( !reg.relative );
      debug_printf( "i%u", reg.number );
      break;

   case SVGA3DREG_LOOP:
      assert( reg.number == 0 );
      debug_printf( "aL" );
      break;

   case SVGA3DREG_MISCTYPE:
      switch (reg.number) {
      case SVGA3DMISCREG_POSITION:
         debug_printf( "vPos" );
         break;
      case SVGA3DMISCREG_FACE:
         debug_printf( "vFace" );
         break;
      default:
         assert(0);
         break;
      }
      break;

   case SVGA3DREG_LABEL:
      debug_printf( "l%u", reg.number );
      break;

   case SVGA3DREG_PREDICATE:
      debug_printf( "p%u", reg.number );
      break;


   default:
      assert( 0 );
      debug_printf( "???" );
   }
}

static void dump_cdata( struct sh_cdata cdata )
{
   debug_printf( "%f, %f, %f, %f", cdata.xyzw[0], cdata.xyzw[1], cdata.xyzw[2], cdata.xyzw[3] );
}

static void dump_idata( struct sh_idata idata )
{
   debug_printf( "%d, %d, %d, %d", idata.xyzw[0], idata.xyzw[1], idata.xyzw[2], idata.xyzw[3] );
}

static void dump_bdata( boolean bdata )
{
   debug_printf( bdata ? "TRUE" : "FALSE" );
}

static void dump_sampleinfo( struct ps_sampleinfo sampleinfo )
{
   switch (sampleinfo.texture_type) {
   case SVGA3DSAMP_2D:
      debug_printf( "_2d" );
      break;
   case SVGA3DSAMP_CUBE:
      debug_printf( "_cube" );
      break;
   case SVGA3DSAMP_VOLUME:
      debug_printf( "_volume" );
      break;
   default:
      assert( 0 );
   }
}


static void dump_usageinfo( struct vs_semantic semantic )
{
   switch (semantic.usage) {
   case SVGA3D_DECLUSAGE_POSITION:
      debug_printf("_position" );
      break;
   case SVGA3D_DECLUSAGE_BLENDWEIGHT:
      debug_printf("_blendweight" );
      break;
   case SVGA3D_DECLUSAGE_BLENDINDICES:
      debug_printf("_blendindices" );
      break;
   case SVGA3D_DECLUSAGE_NORMAL:
      debug_printf("_normal" );
      break;
   case SVGA3D_DECLUSAGE_PSIZE:
      debug_printf("_psize" );
      break;
   case SVGA3D_DECLUSAGE_TEXCOORD:
      debug_printf("_texcoord");
      break;
   case SVGA3D_DECLUSAGE_TANGENT:
      debug_printf("_tangent" );
      break;
   case SVGA3D_DECLUSAGE_BINORMAL:
      debug_printf("_binormal" );
      break;
   case SVGA3D_DECLUSAGE_TESSFACTOR:
      debug_printf("_tessfactor" );
      break;
   case SVGA3D_DECLUSAGE_POSITIONT:
      debug_printf("_positiont" );
      break;
   case SVGA3D_DECLUSAGE_COLOR:
      debug_printf("_color" );
      break;
   case SVGA3D_DECLUSAGE_FOG:
      debug_printf("_fog" );
      break;
   case SVGA3D_DECLUSAGE_DEPTH:
      debug_printf("_depth" );
      break;
   case SVGA3D_DECLUSAGE_SAMPLE:
      debug_printf("_sample");
      break;
   default:
      assert( 0 );
      return;
   }

   if (semantic.usage_index != 0) {
      debug_printf("%d", semantic.usage_index );
   }
}

static void dump_dstreg( struct sh_dstreg dstreg, const struct dump_info *di )
{
   union {
      struct sh_reg reg;
      struct sh_dstreg dstreg;
   } u;

   assert( (dstreg.modifier & (SVGA3DDSTMOD_SATURATE | SVGA3DDSTMOD_PARTIALPRECISION)) == dstreg.modifier );

   if (dstreg.modifier & SVGA3DDSTMOD_SATURATE)
      debug_printf( "_sat" );
   if (dstreg.modifier & SVGA3DDSTMOD_PARTIALPRECISION)
      debug_printf( "_pp" );
   switch (dstreg.shift_scale) {
   case 0:
      break;
   case 1:
      debug_printf( "_x2" );
      break;
   case 2:
      debug_printf( "_x4" );
      break;
   case 3:
      debug_printf( "_x8" );
      break;
   case 13:
      debug_printf( "_d8" );
      break;
   case 14:
      debug_printf( "_d4" );
      break;
   case 15:
      debug_printf( "_d2" );
      break;
   default:
      assert( 0 );
   }
   debug_printf( " " );

   u.dstreg = dstreg;
   dump_reg( u.reg, NULL, di );
   if (dstreg.write_mask != SVGA3DWRITEMASK_ALL) {
      debug_printf( "." );
      if (dstreg.write_mask & SVGA3DWRITEMASK_0)
         debug_printf( "x" );
      if (dstreg.write_mask & SVGA3DWRITEMASK_1)
         debug_printf( "y" );
      if (dstreg.write_mask & SVGA3DWRITEMASK_2)
         debug_printf( "z" );
      if (dstreg.write_mask & SVGA3DWRITEMASK_3)
         debug_printf( "w" );
   }
}

static void dump_srcreg( struct sh_srcreg srcreg, struct sh_srcreg *indreg, const struct dump_info *di )
{
   union {
      struct sh_reg reg;
      struct sh_srcreg srcreg;
   } u;

   switch (srcreg.modifier) {
   case SVGA3DSRCMOD_NEG:
   case SVGA3DSRCMOD_BIASNEG:
   case SVGA3DSRCMOD_SIGNNEG:
   case SVGA3DSRCMOD_X2NEG:
      debug_printf( "-" );
      break;
   case SVGA3DSRCMOD_ABS:
      debug_printf( "|" );
      break;
   case SVGA3DSRCMOD_ABSNEG:
      debug_printf( "-|" );
      break;
   case SVGA3DSRCMOD_COMP:
      debug_printf( "1-" );
      break;
   case SVGA3DSRCMOD_NOT:
      debug_printf( "!" );
   }

   u.srcreg = srcreg;
   dump_reg( u.reg, indreg, di );
   switch (srcreg.modifier) {
   case SVGA3DSRCMOD_NONE:
   case SVGA3DSRCMOD_NEG:
   case SVGA3DSRCMOD_COMP:
   case SVGA3DSRCMOD_NOT:
      break;
   case SVGA3DSRCMOD_ABS:
   case SVGA3DSRCMOD_ABSNEG:
      debug_printf( "|" );
      break;
   case SVGA3DSRCMOD_BIAS:
   case SVGA3DSRCMOD_BIASNEG:
      debug_printf( "_bias" );
      break;
   case SVGA3DSRCMOD_SIGN:
   case SVGA3DSRCMOD_SIGNNEG:
      debug_printf( "_bx2" );
      break;
   case SVGA3DSRCMOD_X2:
   case SVGA3DSRCMOD_X2NEG:
      debug_printf( "_x2" );
      break;
   case SVGA3DSRCMOD_DZ:
      debug_printf( "_dz" );
      break;
   case SVGA3DSRCMOD_DW:
      debug_printf( "_dw" );
      break;
   default:
      assert( 0 );
   }
   if (srcreg.swizzle_x != 0 || srcreg.swizzle_y != 1 || srcreg.swizzle_z != 2 || srcreg.swizzle_w != 3) {
      debug_printf( "." );
      if (srcreg.swizzle_x == srcreg.swizzle_y && srcreg.swizzle_y == srcreg.swizzle_z && srcreg.swizzle_z == srcreg.swizzle_w) {
         debug_printf( "%c", "xyzw"[srcreg.swizzle_x] );
      }
      else {
         debug_printf( "%c", "xyzw"[srcreg.swizzle_x] );
         debug_printf( "%c", "xyzw"[srcreg.swizzle_y] );
         debug_printf( "%c", "xyzw"[srcreg.swizzle_z] );
         debug_printf( "%c", "xyzw"[srcreg.swizzle_w] );
      }
   }
}

void
svga_shader_dump(
   const unsigned *assem,
   unsigned dwords,
   unsigned do_binary )
{
   const unsigned *start = assem;
   boolean finished = FALSE;
   struct dump_info di;
   unsigned i;

   if (do_binary) {
      for (i = 0; i < dwords; i++) 
         debug_printf("  0x%08x,\n", assem[i]);
      
      debug_printf("\n\n");
   }

   di.version.value = *assem++;
   di.is_ps = (di.version.type == SVGA3D_PS_TYPE);

   debug_printf(
      "%s_%u_%u\n",
      di.is_ps ? "ps" : "vs",
      di.version.major,
      di.version.minor );

   while (!finished) {
      struct sh_op op = *(struct sh_op *) assem;

      if (assem - start >= dwords) {
         debug_printf("... ran off end of buffer\n");
         assert(0);
         return;
      }

      switch (op.opcode) {
      case SVGA3DOP_DCL:
         {
            struct sh_dcl dcl = *(struct sh_dcl *) assem;

            debug_printf( "dcl" );
            if (sh_dstreg_type( dcl.reg ) == SVGA3DREG_SAMPLER)
               dump_sampleinfo( dcl.u.ps.sampleinfo );
            else if (di.is_ps) {
               if (di.version.major == 3 && 
                   sh_dstreg_type( dcl.reg ) != SVGA3DREG_MISCTYPE)
                  dump_usageinfo( dcl.u.vs.semantic );
            }
            else
               dump_usageinfo( dcl.u.vs.semantic );
            dump_dstreg( dcl.reg, &di );
            debug_printf( "\n" );
            assem += sizeof( struct sh_dcl ) / sizeof( unsigned );
         }
         break;

      case SVGA3DOP_DEFB:
         {
            struct sh_defb defb = *(struct sh_defb *) assem;

            debug_printf( "defb " );
            dump_reg( defb.reg, NULL, &di );
            debug_printf( ", " );
            dump_bdata( defb.data );
            debug_printf( "\n" );
            assem += sizeof( struct sh_defb ) / sizeof( unsigned );
         }
         break;

      case SVGA3DOP_DEFI:
         {
            struct sh_defi defi = *(struct sh_defi *) assem;

            debug_printf( "defi " );
            dump_reg( defi.reg, NULL, &di );
            debug_printf( ", " );
            dump_idata( defi.idata );
            debug_printf( "\n" );
            assem += sizeof( struct sh_defi ) / sizeof( unsigned );
         }
         break;

      case SVGA3DOP_TEXCOORD:
         assert( di.is_ps );
         dump_op( op, "texcoord" );
         if (0) {
            struct sh_dstop dstop = *(struct sh_dstop *) assem;
            dump_dstreg( dstop.dst, &di );
            assem += sizeof( struct sh_dstop ) / sizeof( unsigned );
         }
         else {
            struct sh_unaryop unaryop = *(struct sh_unaryop *) assem;
            dump_dstreg( unaryop.dst, &di );
            debug_printf( ", " );
            dump_srcreg( unaryop.src, NULL, &di );
            assem += sizeof( struct sh_unaryop ) / sizeof( unsigned );
         }
         debug_printf( "\n" );
         break;

      case SVGA3DOP_TEX:
         assert( di.is_ps );
         if (0) {
            dump_op( op, "tex" );
            if (0) {
               struct sh_dstop dstop = *(struct sh_dstop *) assem;

               dump_dstreg( dstop.dst, &di );
               assem += sizeof( struct sh_dstop ) / sizeof( unsigned );
            }
            else {
               struct sh_unaryop unaryop = *(struct sh_unaryop *) assem;

               dump_dstreg( unaryop.dst, &di );
               debug_printf( ", " );
               dump_srcreg( unaryop.src, NULL, &di );
               assem += sizeof( struct sh_unaryop ) / sizeof( unsigned );
            }
         }
         else {
            struct sh_binaryop binaryop = *(struct sh_binaryop *) assem;

            dump_op( op, "texld" );
            dump_dstreg( binaryop.dst, &di );
            debug_printf( ", " );
            dump_srcreg( binaryop.src0, NULL, &di );
            debug_printf( ", " );
            dump_srcreg( binaryop.src1, NULL, &di );
            assem += sizeof( struct sh_binaryop ) / sizeof( unsigned );
         }
         debug_printf( "\n" );
         break;

      case SVGA3DOP_DEF:
         {
            struct sh_def def = *(struct sh_def *) assem;

            debug_printf( "def " );
            dump_reg( def.reg, NULL, &di );
            debug_printf( ", " );
            dump_cdata( def.cdata );
            debug_printf( "\n" );
            assem += sizeof( struct sh_def ) / sizeof( unsigned );
         }
         break;

      case SVGA3DOP_PHASE:
         debug_printf( "phase\n" );
         assem += sizeof( struct sh_op ) / sizeof( unsigned );
         break;

      case SVGA3DOP_COMMENT:
         {
            struct sh_comment comment = *(struct sh_comment *)assem;

            /* Ignore comment contents. */
            assem += sizeof(struct sh_comment) / sizeof(unsigned) + comment.size;
         }
         break;

      case SVGA3DOP_RET:
         debug_printf( "ret\n" );
         assem += sizeof( struct sh_op ) / sizeof( unsigned );
         break;

      case SVGA3DOP_END:
         debug_printf( "end\n" );
         finished = TRUE;
         break;

      default:
         {
            const struct sh_opcode_info *info = svga_opcode_info( op.opcode );
            uint i;
            uint num_src = info->num_src + op.predicated;
            boolean not_first_arg = FALSE;

            assert( info->num_dst <= 1 );

            if (op.opcode == SVGA3DOP_SINCOS && di.version.major < 3)
               num_src += 2;

            dump_comp_op( op, info->mnemonic );
            assem += sizeof( struct sh_op ) / sizeof( unsigned );

            if (info->num_dst > 0) {
               struct sh_dstreg dstreg = *(struct sh_dstreg *) assem;

               dump_dstreg( dstreg, &di );
               assem += sizeof( struct sh_dstreg ) / sizeof( unsigned );
               not_first_arg = TRUE;
            }

            for (i = 0; i < num_src; i++) {
               struct sh_srcreg srcreg;
               struct sh_srcreg indreg;

               srcreg = *(struct sh_srcreg *) assem;
               assem += sizeof( struct sh_srcreg ) / sizeof( unsigned );
               if (srcreg.relative && !di.is_ps && di.version.major >= 2) {
                  indreg = *(struct sh_srcreg *) assem;
                  assem += sizeof( struct sh_srcreg ) / sizeof( unsigned );
               }

               if (not_first_arg)
                  debug_printf( ", " );
               else
                  debug_printf( " " );
               dump_srcreg( srcreg, &indreg, &di );
               not_first_arg = TRUE;
            }

            debug_printf( "\n" );
         }
      }
   }
}
