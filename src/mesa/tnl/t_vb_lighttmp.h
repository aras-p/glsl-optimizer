
/*
 * Mesa 3-D graphics library
 * Version:  4.1
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * Authors:
 *    Brian Paul
 *    Keith Whitwell <keith@tungstengraphics.com>
 */


#if (IDX & LIGHT_FLAGS)
#  define VSTRIDE (4 * sizeof(GLfloat))
#  define NSTRIDE nstride /*(3 * sizeof(GLfloat))*/
#  define CHECK_MATERIAL(x)  (flags[x] & VERT_BIT_MATERIAL)
#  define CHECK_END_VB(x)    (flags[x] & VERT_BIT_END_VB)
#  if (IDX & LIGHT_COLORMATERIAL)
#    define CMSTRIDE STRIDE_F(CMcolor, CMstride)
#    define CHECK_COLOR_MATERIAL(x) (flags[x] & VERT_BIT_COLOR0)
#    define CHECK_VALIDATE(x) (flags[x] & (VERT_BIT_COLOR0|VERT_BIT_MATERIAL))
#    define DO_ANOTHER_NORMAL(x) \
     ((flags[x] & (VERT_BIT_COLOR0|VERT_BIT_NORMAL|VERT_BIT_END_VB|VERT_BIT_MATERIAL)) == VERT_BIT_NORMAL)
#    define REUSE_LIGHT_RESULTS(x) \
     ((flags[x] & (VERT_BIT_COLOR0|VERT_BIT_NORMAL|VERT_BIT_END_VB|VERT_BIT_MATERIAL)) == 0)
#  else
#    define CMSTRIDE (void)0
#    define CHECK_COLOR_MATERIAL(x) 0
#    define CHECK_VALIDATE(x) (flags[x] & (VERT_BIT_MATERIAL))
#    define DO_ANOTHER_NORMAL(x) \
      ((flags[x] & (VERT_BIT_NORMAL|VERT_BIT_END_VB|VERT_BIT_MATERIAL)) == VERT_BIT_NORMAL)
#    define REUSE_LIGHT_RESULTS(x) \
      ((flags[x] & (VERT_BIT_NORMAL|VERT_BIT_END_VB|VERT_BIT_MATERIAL)) == 0)
#  endif
#else
#  define VSTRIDE vstride
#  define NSTRIDE nstride
#  define CHECK_MATERIAL(x)   0	           /* no materials on array paths */
#  define CHECK_END_VB(XX)     (XX >= nr)
#  if (IDX & LIGHT_COLORMATERIAL)
#     define CMSTRIDE STRIDE_F(CMcolor, CMstride)
#     define CHECK_COLOR_MATERIAL(x) (x < nr) /* always have colormaterial */
#     define CHECK_VALIDATE(x) (x < nr)
#     define DO_ANOTHER_NORMAL(x) 0        /* always stop to recalc colormat */
#  else
#     define CMSTRIDE (void)0
#     define CHECK_COLOR_MATERIAL(x) 0        /* no colormaterial */
#     define CHECK_VALIDATE(x) (0)
#     define DO_ANOTHER_NORMAL(XX) (XX < nr) /* keep going to end of vb */
#  endif
#  define REUSE_LIGHT_RESULTS(x) 0         /* always have a new normal */
#endif



#if (IDX & LIGHT_TWOSIDE)
#  define NR_SIDES 2
#else
#  define NR_SIDES 1
#endif


/* define TRACE if to trace lighting code */


/*
 * ctx is the current context
 * VB is the vertex buffer
 * stage is the lighting stage-private data
 * input is the vector of eye or object-space vertex coordinates
 */
static void TAG(light_rgba_spec)( GLcontext *ctx,
				  struct vertex_buffer *VB,
				  struct gl_pipeline_stage *stage,
				  GLvector4f *input )
{
   struct light_stage_data *store = LIGHT_STAGE_DATA(stage);
   GLfloat (*base)[3] = ctx->Light._BaseColor;
   GLchan sumA[2];
   GLuint j;

   const GLuint vstride = input->stride;
   const GLfloat *vertex = (GLfloat *)input->data;
   const GLuint nstride = VB->NormalPtr->stride;
   const GLfloat *normal = (GLfloat *)VB->NormalPtr->data;

   GLfloat *CMcolor;
   GLuint CMstride;

   GLchan (*Fcolor)[4] = (GLchan (*)[4]) store->LitColor[0].Ptr;
   GLchan (*Bcolor)[4] = (GLchan (*)[4]) store->LitColor[1].Ptr;
   GLchan (*Fspec)[4] = (GLchan (*)[4]) store->LitSecondary[0].Ptr;
   GLchan (*Bspec)[4] = (GLchan (*)[4]) store->LitSecondary[1].Ptr;

   const GLuint nr = VB->Count;
   const GLuint *flags = VB->Flag;
   struct gl_material (*new_material)[2] = VB->Material;
   const GLuint *new_material_mask = VB->MaterialMask;

   (void) flags;
   (void) nstride;
   (void) vstride;

#ifdef TRACE
   fprintf(stderr, "%s\n", __FUNCTION__ );
#endif

   if (IDX & LIGHT_COLORMATERIAL) {
      if (VB->ColorPtr[0]->Type != GL_FLOAT || 
	  VB->ColorPtr[0]->Size != 4)
	 import_color_material( ctx, stage );

      CMcolor = (GLfloat *) VB->ColorPtr[0]->Ptr;
      CMstride = VB->ColorPtr[0]->StrideB;
   }

   VB->ColorPtr[0] = &store->LitColor[0];
   VB->SecondaryColorPtr[0] = &store->LitSecondary[0];
   UNCLAMPED_FLOAT_TO_CHAN(sumA[0], ctx->Light.Material[0].Diffuse[3]);

   if (IDX & LIGHT_TWOSIDE) {
      VB->ColorPtr[1] = &store->LitColor[1];
      VB->SecondaryColorPtr[1] = &store->LitSecondary[1];
      UNCLAMPED_FLOAT_TO_CHAN(sumA[1], ctx->Light.Material[1].Diffuse[3]);
   }

   /* Side-effects done, can we finish now?
    */
   if (stage->changed_inputs == 0)
      return;

   for ( j=0 ;
	 j<nr ;
	 j++,STRIDE_F(vertex,VSTRIDE),STRIDE_F(normal,NSTRIDE),CMSTRIDE)
   {
      GLfloat sum[2][3], spec[2][3];
      struct gl_light *light;

      if ( CHECK_COLOR_MATERIAL(j) )
	 _mesa_update_color_material( ctx, CMcolor );

      if ( CHECK_MATERIAL(j) )
	 _mesa_update_material( ctx, new_material[j], new_material_mask[j] );

      if ( CHECK_VALIDATE(j) ) {
	 TNL_CONTEXT(ctx)->Driver.NotifyMaterialChange( ctx );
	 UNCLAMPED_FLOAT_TO_CHAN(sumA[0], ctx->Light.Material[0].Diffuse[3]);
	 if (IDX & LIGHT_TWOSIDE) 
	    UNCLAMPED_FLOAT_TO_CHAN(sumA[1], ctx->Light.Material[1].Diffuse[3]);
      }

      COPY_3V(sum[0], base[0]);
      ZERO_3V(spec[0]);

      if (IDX & LIGHT_TWOSIDE) {
	 COPY_3V(sum[1], base[1]);
	 ZERO_3V(spec[1]);
      }

      /* Add contribution from each enabled light source */
      foreach (light, &ctx->Light.EnabledList) {
	 GLfloat n_dot_h;
	 GLfloat correction;
	 GLint side;
	 GLfloat contrib[3];
	 GLfloat attenuation;
	 GLfloat VP[3];  /* unit vector from vertex to light */
	 GLfloat n_dot_VP;       /* n dot VP */
	 GLfloat *h;

	 /* compute VP and attenuation */
	 if (!(light->_Flags & LIGHT_POSITIONAL)) {
	    /* directional light */
	    COPY_3V(VP, light->_VP_inf_norm);
	    attenuation = light->_VP_inf_spot_attenuation;
	 }
	 else {
	    GLfloat d;     /* distance from vertex to light */

	    SUB_3V(VP, light->_Position, vertex);

	    d = (GLfloat) LEN_3FV( VP );

	    if (d > 1e-6) {
	       GLfloat invd = 1.0F / d;
	       SELF_SCALE_SCALAR_3V(VP, invd);
	    }

	    attenuation = 1.0F / (light->ConstantAttenuation + d *
				  (light->LinearAttenuation + d *
				   light->QuadraticAttenuation));

	    /* spotlight attenuation */
	    if (light->_Flags & LIGHT_SPOT) {
	       GLfloat PV_dot_dir = - DOT3(VP, light->_NormDirection);

	       if (PV_dot_dir<light->_CosCutoff) {
		  continue; /* this light makes no contribution */
	       }
	       else {
		  GLdouble x = PV_dot_dir * (EXP_TABLE_SIZE-1);
		  GLint k = (GLint) x;
		  GLfloat spot = (GLfloat) (light->_SpotExpTable[k][0]
				    + (x-k)*light->_SpotExpTable[k][1]);
		  attenuation *= spot;
	       }
	    }
	 }

	 if (attenuation < 1e-3)
	    continue;		/* this light makes no contribution */

	 /* Compute dot product or normal and vector from V to light pos */
	 n_dot_VP = DOT3( normal, VP );

	 /* Which side gets the diffuse & specular terms? */
	 if (n_dot_VP < 0.0F) {
	    ACC_SCALE_SCALAR_3V(sum[0], attenuation, light->_MatAmbient[0]);
	    if (!(IDX & LIGHT_TWOSIDE)) {
	       continue;
	    }
	    side = 1;
	    correction = -1;
	    n_dot_VP = -n_dot_VP;
	 }
         else {
	    if (IDX & LIGHT_TWOSIDE) {
	       ACC_SCALE_SCALAR_3V( sum[1], attenuation, light->_MatAmbient[1]);
	    }
	    side = 0;
	    correction = 1;
	 }

	 /* diffuse term */
	 COPY_3V(contrib, light->_MatAmbient[side]);
	 ACC_SCALE_SCALAR_3V(contrib, n_dot_VP, light->_MatDiffuse[side]);
	 ACC_SCALE_SCALAR_3V(sum[side], attenuation, contrib );

	 /* specular term - cannibalize VP... */
	 if (ctx->Light.Model.LocalViewer) {
	    GLfloat v[3];
	    COPY_3V(v, vertex);
	    NORMALIZE_3FV(v);
	    SUB_3V(VP, VP, v);                /* h = VP + VPe */
	    h = VP;
	    NORMALIZE_3FV(h);
	 }
	 else if (light->_Flags & LIGHT_POSITIONAL) {
	    h = VP;
	    ACC_3V(h, ctx->_EyeZDir);
	    NORMALIZE_3FV(h);
	 }
         else {
	    h = light->_h_inf_norm;
	 }

	 n_dot_h = correction * DOT3(normal, h);

	 if (n_dot_h > 0.0F) {
	    GLfloat spec_coef;
	    struct gl_shine_tab *tab = ctx->_ShineTable[side];
	    GET_SHINE_TAB_ENTRY( tab, n_dot_h, spec_coef );

	    if (spec_coef > 1.0e-10) {
	       spec_coef *= attenuation;
	       ACC_SCALE_SCALAR_3V( spec[side], spec_coef,
				    light->_MatSpecular[side]);
	    }
	 }
      } /*loop over lights*/

      UNCLAMPED_FLOAT_TO_RGB_CHAN( Fcolor[j], sum[0] );
      UNCLAMPED_FLOAT_TO_RGB_CHAN( Fspec[j], spec[0] );
      Fcolor[j][3] = sumA[0];

      if (IDX & LIGHT_TWOSIDE) {
	 UNCLAMPED_FLOAT_TO_RGB_CHAN( Bcolor[j], sum[1] );
	 UNCLAMPED_FLOAT_TO_RGB_CHAN( Bspec[j], spec[1] );
	 Bcolor[j][3] = sumA[1];
      }
   }
}


static void TAG(light_rgba)( GLcontext *ctx,
			     struct vertex_buffer *VB,
			     struct gl_pipeline_stage *stage,
			     GLvector4f *input )
{
   struct light_stage_data *store = LIGHT_STAGE_DATA(stage);
   GLuint j;

   GLfloat (*base)[3] = ctx->Light._BaseColor;
   GLchan sumA[2];

   const GLuint vstride = input->stride;
   const GLfloat *vertex = (GLfloat *) input->data;
   const GLuint nstride = VB->NormalPtr->stride;
   const GLfloat *normal = (GLfloat *)VB->NormalPtr->data;

   GLfloat *CMcolor;
   GLuint CMstride;

   GLchan (*Fcolor)[4] = (GLchan (*)[4]) store->LitColor[0].Ptr;
   GLchan (*Bcolor)[4] = (GLchan (*)[4]) store->LitColor[1].Ptr;
   GLchan (*color[2])[4];
   const GLuint *flags = VB->Flag;

   struct gl_material (*new_material)[2] = VB->Material;
   const GLuint *new_material_mask = VB->MaterialMask;
   const GLuint nr = VB->Count;

#ifdef TRACE
   fprintf(stderr, "%s\n", __FUNCTION__ );
#endif

   (void) flags;
   (void) nstride;
   (void) vstride;

   color[0] = Fcolor;
   color[1] = Bcolor;

   if (IDX & LIGHT_COLORMATERIAL) {
      if (VB->ColorPtr[0]->Type != GL_FLOAT || 
	  VB->ColorPtr[0]->Size != 4)
	 import_color_material( ctx, stage );

      CMcolor = (GLfloat *)VB->ColorPtr[0]->Ptr;
      CMstride = VB->ColorPtr[0]->StrideB;
   }

   VB->ColorPtr[0] = &store->LitColor[0];
   UNCLAMPED_FLOAT_TO_CHAN(sumA[0], ctx->Light.Material[0].Diffuse[3]);

   if (IDX & LIGHT_TWOSIDE) {
      VB->ColorPtr[1] = &store->LitColor[1];
      UNCLAMPED_FLOAT_TO_CHAN(sumA[1], ctx->Light.Material[1].Diffuse[3]);
   }

   if (stage->changed_inputs == 0)
      return;

   for ( j=0 ;
	 j<nr ;
	 j++,STRIDE_F(vertex,VSTRIDE), STRIDE_F(normal,NSTRIDE),CMSTRIDE)
   {
      GLfloat sum[2][3];
      struct gl_light *light;

      if ( CHECK_COLOR_MATERIAL(j) )
	 _mesa_update_color_material( ctx, CMcolor );

      if ( CHECK_MATERIAL(j) )
	 _mesa_update_material( ctx, new_material[j], new_material_mask[j] );

      if ( CHECK_VALIDATE(j) ) {
	 TNL_CONTEXT(ctx)->Driver.NotifyMaterialChange( ctx );
	 UNCLAMPED_FLOAT_TO_CHAN(sumA[0], ctx->Light.Material[0].Diffuse[3]);
	 if (IDX & LIGHT_TWOSIDE)
	    UNCLAMPED_FLOAT_TO_CHAN(sumA[1], ctx->Light.Material[1].Diffuse[3]);
      }

      COPY_3V(sum[0], base[0]);

      if ( IDX & LIGHT_TWOSIDE )
	 COPY_3V(sum[1], base[1]);

      /* Add contribution from each enabled light source */
      foreach (light, &ctx->Light.EnabledList) {

	 GLfloat n_dot_h;
	 GLfloat correction;
	 GLint side;
	 GLfloat contrib[3];
	 GLfloat attenuation = 1.0;
	 GLfloat VP[3];          /* unit vector from vertex to light */
	 GLfloat n_dot_VP;       /* n dot VP */
	 GLfloat *h;

	 /* compute VP and attenuation */
	 if (!(light->_Flags & LIGHT_POSITIONAL)) {
	    /* directional light */
	    COPY_3V(VP, light->_VP_inf_norm);
	    attenuation = light->_VP_inf_spot_attenuation;
	 }
	 else {
	    GLfloat d;     /* distance from vertex to light */


	    SUB_3V(VP, light->_Position, vertex);

	    d = (GLfloat) LEN_3FV( VP );

	    if ( d > 1e-6) {
	       GLfloat invd = 1.0F / d;
	       SELF_SCALE_SCALAR_3V(VP, invd);
	    }

            attenuation = 1.0F / (light->ConstantAttenuation + d *
                                  (light->LinearAttenuation + d *
                                   light->QuadraticAttenuation));

	    /* spotlight attenuation */
	    if (light->_Flags & LIGHT_SPOT) {
	       GLfloat PV_dot_dir = - DOT3(VP, light->_NormDirection);

	       if (PV_dot_dir<light->_CosCutoff) {
		  continue; /* this light makes no contribution */
	       }
	       else {
		  GLdouble x = PV_dot_dir * (EXP_TABLE_SIZE-1);
		  GLint k = (GLint) x;
		  GLfloat spot = (GLfloat) (light->_SpotExpTable[k][0]
				  + (x-k)*light->_SpotExpTable[k][1]);
		  attenuation *= spot;
	       }
	    }
	 }

	 if (attenuation < 1e-3)
	    continue;		/* this light makes no contribution */

	 /* Compute dot product or normal and vector from V to light pos */
	 n_dot_VP = DOT3( normal, VP );

	 /* which side are we lighting? */
	 if (n_dot_VP < 0.0F) {
	    ACC_SCALE_SCALAR_3V(sum[0], attenuation, light->_MatAmbient[0]);

	    if (!(IDX & LIGHT_TWOSIDE))
	       continue;

	    side = 1;
	    correction = -1;
	    n_dot_VP = -n_dot_VP;
	 }
         else {
	    if (IDX & LIGHT_TWOSIDE) {
	       ACC_SCALE_SCALAR_3V( sum[1], attenuation, light->_MatAmbient[1]);
	    }
	    side = 0;
	    correction = 1;
	 }

	 COPY_3V(contrib, light->_MatAmbient[side]);

	 /* diffuse term */
	 ACC_SCALE_SCALAR_3V(contrib, n_dot_VP, light->_MatDiffuse[side]);

	 /* specular term - cannibalize VP... */
	 {
	    if (ctx->Light.Model.LocalViewer) {
	       GLfloat v[3];
	       COPY_3V(v, vertex);
	       NORMALIZE_3FV(v);
	       SUB_3V(VP, VP, v);                /* h = VP + VPe */
	       h = VP;
	       NORMALIZE_3FV(h);
	    }
	    else if (light->_Flags & LIGHT_POSITIONAL) {
	       h = VP;
	       ACC_3V(h, ctx->_EyeZDir);
	       NORMALIZE_3FV(h);
	    }
            else {
	       h = light->_h_inf_norm;
	    }

	    n_dot_h = correction * DOT3(normal, h);

	    if (n_dot_h > 0.0F)
	    {
	       GLfloat spec_coef;
	       struct gl_shine_tab *tab = ctx->_ShineTable[side];

	       GET_SHINE_TAB_ENTRY( tab, n_dot_h, spec_coef );

	       ACC_SCALE_SCALAR_3V( contrib, spec_coef,
				    light->_MatSpecular[side]);
	    }
	 }

	 ACC_SCALE_SCALAR_3V( sum[side], attenuation, contrib );
      }

      UNCLAMPED_FLOAT_TO_RGB_CHAN( Fcolor[j], sum[0] );
      Fcolor[j][3] = sumA[0];

      if (IDX & LIGHT_TWOSIDE) {
	 UNCLAMPED_FLOAT_TO_RGB_CHAN( Bcolor[j], sum[1] );
	 Bcolor[j][3] = sumA[1];
      }
   }
}




/* As below, but with just a single light.
 */
static void TAG(light_fast_rgba_single)( GLcontext *ctx,
					 struct vertex_buffer *VB,
					 struct gl_pipeline_stage *stage,
					 GLvector4f *input )

{
   struct light_stage_data *store = LIGHT_STAGE_DATA(stage);
   const GLuint nstride = VB->NormalPtr->stride;
   const GLfloat *normal = (GLfloat *)VB->NormalPtr->data;
   GLfloat *CMcolor;
   GLuint CMstride;
   GLchan (*Fcolor)[4] = (GLchan (*)[4]) store->LitColor[0].Ptr;
   GLchan (*Bcolor)[4] = (GLchan (*)[4]) store->LitColor[1].Ptr;
   const struct gl_light *light = ctx->Light.EnabledList.next;
   const GLuint *flags = VB->Flag;
   GLchan basechan[2][4];
   GLuint j = 0;
   struct gl_material (*new_material)[2] = VB->Material;
   const GLuint *new_material_mask = VB->MaterialMask;
   GLfloat base[2][3];
   const GLuint nr = VB->Count;

#ifdef TRACE
   fprintf(stderr, "%s\n", __FUNCTION__ );
#endif

   (void) input;		/* doesn't refer to Eye or Obj */
   (void) flags;
   (void) nr;
   (void) nstride;

   if (IDX & LIGHT_COLORMATERIAL) {
      if (VB->ColorPtr[0]->Type != GL_FLOAT || 
	  VB->ColorPtr[0]->Size != 4)
	 import_color_material( ctx, stage );

      CMcolor = (GLfloat *)VB->ColorPtr[0]->Ptr;
      CMstride = VB->ColorPtr[0]->StrideB;
   }

   VB->ColorPtr[0] = &store->LitColor[0];
   if (IDX & LIGHT_TWOSIDE)
      VB->ColorPtr[1] = &store->LitColor[1];

   if (stage->changed_inputs == 0)
      return;

   do {
      
      if ( CHECK_COLOR_MATERIAL(j) ) {
	 _mesa_update_color_material( ctx, CMcolor );
      }

      if ( CHECK_MATERIAL(j) )
	 _mesa_update_material( ctx, new_material[j], new_material_mask[j] );

      if ( CHECK_VALIDATE(j) )
	 TNL_CONTEXT(ctx)->Driver.NotifyMaterialChange( ctx );


      /* No attenuation, so incoporate _MatAmbient into base color.
       */
      COPY_3V(base[0], light->_MatAmbient[0]);
      ACC_3V(base[0], ctx->Light._BaseColor[0] );
      UNCLAMPED_FLOAT_TO_RGB_CHAN( basechan[0], base[0] );
      UNCLAMPED_FLOAT_TO_CHAN(basechan[0][3], 
			      ctx->Light.Material[0].Diffuse[3]);

      if (IDX & LIGHT_TWOSIDE) {
	 COPY_3V(base[1], light->_MatAmbient[1]);
	 ACC_3V(base[1], ctx->Light._BaseColor[1]);
	 UNCLAMPED_FLOAT_TO_RGB_CHAN( basechan[1], base[1]);
	 UNCLAMPED_FLOAT_TO_CHAN(basechan[1][3], 
				 ctx->Light.Material[1].Diffuse[3]);
      }

      do {
	 GLfloat n_dot_VP = DOT3(normal, light->_VP_inf_norm);

	 if (n_dot_VP < 0.0F) {
	    if (IDX & LIGHT_TWOSIDE) {
	       GLfloat n_dot_h = -DOT3(normal, light->_h_inf_norm);
	       GLfloat sum[3];
	       COPY_3V(sum, base[1]);
	       ACC_SCALE_SCALAR_3V(sum, -n_dot_VP, light->_MatDiffuse[1]);
	       if (n_dot_h > 0.0F) {
		  GLfloat spec;
		  GET_SHINE_TAB_ENTRY( ctx->_ShineTable[1], n_dot_h, spec );
		  ACC_SCALE_SCALAR_3V(sum, spec, light->_MatSpecular[1]);
	       }
	       UNCLAMPED_FLOAT_TO_RGB_CHAN(Bcolor[j], sum );
	       Bcolor[j][3] = basechan[1][3];
	    }
	    COPY_CHAN4(Fcolor[j], basechan[0]);
	 }
         else {
	    GLfloat n_dot_h = DOT3(normal, light->_h_inf_norm);
	    GLfloat sum[3];
	    COPY_3V(sum, base[0]);
	    ACC_SCALE_SCALAR_3V(sum, n_dot_VP, light->_MatDiffuse[0]);
	    if (n_dot_h > 0.0F) {
	       GLfloat spec;
	       GET_SHINE_TAB_ENTRY( ctx->_ShineTable[0], n_dot_h, spec );
	       ACC_SCALE_SCALAR_3V(sum, spec, light->_MatSpecular[0]);

	    }
	    UNCLAMPED_FLOAT_TO_RGB_CHAN(Fcolor[j], sum );
	    Fcolor[j][3] = basechan[0][3];
	    if (IDX & LIGHT_TWOSIDE) COPY_CHAN4(Bcolor[j], basechan[1]);
	 }

	 j++;
	 CMSTRIDE;
	 STRIDE_F(normal, NSTRIDE);
      } while (DO_ANOTHER_NORMAL(j));


      for ( ; REUSE_LIGHT_RESULTS(j) ; j++, CMSTRIDE, STRIDE_F(normal,NSTRIDE))
      {
	 COPY_CHAN4(Fcolor[j], Fcolor[j-1]);
	 if (IDX & LIGHT_TWOSIDE)
	    COPY_CHAN4(Bcolor[j], Bcolor[j-1]);
      }

   } while (!CHECK_END_VB(j));
}


/* Light infinite lights
 */
static void TAG(light_fast_rgba)( GLcontext *ctx,
				  struct vertex_buffer *VB,
				  struct gl_pipeline_stage *stage,
				  GLvector4f *input )
{
   struct light_stage_data *store = LIGHT_STAGE_DATA(stage);
   GLchan sumA[2];
   const GLuint nstride = VB->NormalPtr->stride;
   const GLfloat *normal = (GLfloat *)VB->NormalPtr->data;
   GLfloat *CMcolor;
   GLuint CMstride;
   GLchan (*Fcolor)[4] = (GLchan (*)[4]) store->LitColor[0].Ptr;
   GLchan (*Bcolor)[4] = (GLchan (*)[4]) store->LitColor[1].Ptr;
   const GLuint *flags = VB->Flag;
   GLuint j = 0;
   struct gl_material (*new_material)[2] = VB->Material;
   GLuint *new_material_mask = VB->MaterialMask;
   const GLuint nr = VB->Count;
   const struct gl_light *light;

#ifdef TRACE
   fprintf(stderr, "%s\n", __FUNCTION__ );
#endif

   (void) flags;
   (void) input;
   (void) nr;
   (void) nstride;

   UNCLAMPED_FLOAT_TO_CHAN(sumA[0], ctx->Light.Material[0].Diffuse[3]);
   UNCLAMPED_FLOAT_TO_CHAN(sumA[1], ctx->Light.Material[1].Diffuse[3]);

   if (IDX & LIGHT_COLORMATERIAL) {
      if (VB->ColorPtr[0]->Type != GL_FLOAT || 
	  VB->ColorPtr[0]->Size != 4)
	 import_color_material( ctx, stage );

      CMcolor = (GLfloat *)VB->ColorPtr[0]->Ptr;
      CMstride = VB->ColorPtr[0]->StrideB;
   }

   VB->ColorPtr[0] = &store->LitColor[0];
   if (IDX & LIGHT_TWOSIDE)
      VB->ColorPtr[1] = &store->LitColor[1];

   if (stage->changed_inputs == 0)
      return;

   do {
      do {
	 GLfloat sum[2][3];

	 if ( CHECK_COLOR_MATERIAL(j) )
	    _mesa_update_color_material( ctx, CMcolor );

	 if ( CHECK_MATERIAL(j) )
	    _mesa_update_material( ctx, new_material[j], new_material_mask[j] );

	 if ( CHECK_VALIDATE(j) ) {
	    TNL_CONTEXT(ctx)->Driver.NotifyMaterialChange( ctx );
	    UNCLAMPED_FLOAT_TO_CHAN(sumA[0], ctx->Light.Material[0].Diffuse[3]);
	    if (IDX & LIGHT_TWOSIDE)
	       UNCLAMPED_FLOAT_TO_CHAN(sumA[1], 
				       ctx->Light.Material[1].Diffuse[3]);
	 }


	 COPY_3V(sum[0], ctx->Light._BaseColor[0]);
	 if (IDX & LIGHT_TWOSIDE)
	    COPY_3V(sum[1], ctx->Light._BaseColor[1]);

	 foreach (light, &ctx->Light.EnabledList) {
	    GLfloat n_dot_h, n_dot_VP, spec;

	    ACC_3V(sum[0], light->_MatAmbient[0]);
	    if (IDX & LIGHT_TWOSIDE)
	       ACC_3V(sum[1], light->_MatAmbient[1]);

	    n_dot_VP = DOT3(normal, light->_VP_inf_norm);

	    if (n_dot_VP > 0.0F) {
	       ACC_SCALE_SCALAR_3V(sum[0], n_dot_VP, light->_MatDiffuse[0]);
	       n_dot_h = DOT3(normal, light->_h_inf_norm);
	       if (n_dot_h > 0.0F) {
		  struct gl_shine_tab *tab = ctx->_ShineTable[0];
		  GET_SHINE_TAB_ENTRY( tab, n_dot_h, spec );
		  ACC_SCALE_SCALAR_3V( sum[0], spec,
				       light->_MatSpecular[0]);
	       }
	    }
	    else if (IDX & LIGHT_TWOSIDE) {
	       ACC_SCALE_SCALAR_3V(sum[1], -n_dot_VP, light->_MatDiffuse[1]);
	       n_dot_h = -DOT3(normal, light->_h_inf_norm);
	       if (n_dot_h > 0.0F) {
		  struct gl_shine_tab *tab = ctx->_ShineTable[1];
		  GET_SHINE_TAB_ENTRY( tab, n_dot_h, spec );
		  ACC_SCALE_SCALAR_3V( sum[1], spec,
				       light->_MatSpecular[1]);
	       }
	    }
	 }

	 UNCLAMPED_FLOAT_TO_RGB_CHAN( Fcolor[j], sum[0] );
	 Fcolor[j][3] = sumA[0];

	 if (IDX & LIGHT_TWOSIDE) {
	    UNCLAMPED_FLOAT_TO_RGB_CHAN( Bcolor[j], sum[1] );
	    Bcolor[j][3] = sumA[1];
	 }

	 j++;
	 CMSTRIDE;
	 STRIDE_F(normal, NSTRIDE);
      } while (DO_ANOTHER_NORMAL(j));

      /* Reuse the shading results while there is no change to
       * normal or material values.
       */
      for ( ; REUSE_LIGHT_RESULTS(j) ; j++, CMSTRIDE, STRIDE_F(normal, NSTRIDE))
      {
	 COPY_CHAN4(Fcolor[j], Fcolor[j-1]);
	 if (IDX & LIGHT_TWOSIDE)
	    COPY_CHAN4(Bcolor[j], Bcolor[j-1]);
      }

   } while (!CHECK_END_VB(j));
}





/*
 * Use current lighting/material settings to compute the color indexes
 * for an array of vertices.
 * Input:  n - number of vertices to light
 *         side - 0=use front material, 1=use back material
 *         vertex - array of [n] vertex position in eye coordinates
 *         normal - array of [n] surface normal vector
 * Output:  indexResult - resulting array of [n] color indexes
 */
static void TAG(light_ci)( GLcontext *ctx,
			   struct vertex_buffer *VB,
			   struct gl_pipeline_stage *stage,
			   GLvector4f *input )
{
   struct light_stage_data *store = LIGHT_STAGE_DATA(stage);
   GLuint j;
   const GLuint vstride = input->stride;
   const GLfloat *vertex = (GLfloat *) input->data;
   const GLuint nstride = VB->NormalPtr->stride;
   const GLfloat *normal = (GLfloat *)VB->NormalPtr->data;
   GLfloat *CMcolor;
   GLuint CMstride;
   const GLuint *flags = VB->Flag;
   GLuint *indexResult[2];
   struct gl_material (*new_material)[2] = VB->Material;
   GLuint *new_material_mask = VB->MaterialMask;
   const GLuint nr = VB->Count;

#ifdef TRACE
   fprintf(stderr, "%s\n", __FUNCTION__ );
#endif

   (void) flags;
   (void) nstride;
   (void) vstride;

   VB->IndexPtr[0] = &store->LitIndex[0];
   if (IDX & LIGHT_TWOSIDE)
      VB->IndexPtr[1] = &store->LitIndex[1];

   if (stage->changed_inputs == 0)
      return;

   indexResult[0] = VB->IndexPtr[0]->data;
   if (IDX & LIGHT_TWOSIDE)
      indexResult[1] = VB->IndexPtr[1]->data;

   if (IDX & LIGHT_COLORMATERIAL) {
      if (VB->ColorPtr[0]->Type != GL_FLOAT || 
	  VB->ColorPtr[0]->Size != 4)
	 import_color_material( ctx, stage );

      CMcolor = (GLfloat *)VB->ColorPtr[0]->Ptr;
      CMstride = VB->ColorPtr[0]->StrideB;
   }

   /* loop over vertices */
   for ( j=0 ;
	 j<nr ;
	 j++,STRIDE_F(vertex,VSTRIDE),STRIDE_F(normal, NSTRIDE), CMSTRIDE)
   {
      GLfloat diffuse[2], specular[2];
      GLuint side = 0;
      struct gl_light *light;

      if ( CHECK_COLOR_MATERIAL(j) )
	 _mesa_update_color_material( ctx, CMcolor );

      if ( CHECK_MATERIAL(j) )
	 _mesa_update_material( ctx, new_material[j], new_material_mask[j] );

      if ( CHECK_VALIDATE(j) )
	 TNL_CONTEXT(ctx)->Driver.NotifyMaterialChange( ctx );

      diffuse[0] = specular[0] = 0.0F;

      if ( IDX & LIGHT_TWOSIDE ) {
	 diffuse[1] = specular[1] = 0.0F;
      }

      /* Accumulate diffuse and specular from each light source */
      foreach (light, &ctx->Light.EnabledList) {

	 GLfloat attenuation = 1.0F;
	 GLfloat VP[3];  /* unit vector from vertex to light */
	 GLfloat n_dot_VP;  /* dot product of l and n */
	 GLfloat *h, n_dot_h, correction = 1.0;

	 /* compute l and attenuation */
	 if (!(light->_Flags & LIGHT_POSITIONAL)) {
	    /* directional light */
	    COPY_3V(VP, light->_VP_inf_norm);
	 }
	 else {
	    GLfloat d;     /* distance from vertex to light */

	    SUB_3V(VP, light->_Position, vertex);

	    d = (GLfloat) LEN_3FV( VP );
	    if ( d > 1e-6) {
	       GLfloat invd = 1.0F / d;
	       SELF_SCALE_SCALAR_3V(VP, invd);
	    }

	    attenuation = 1.0F / (light->ConstantAttenuation + d *
				  (light->LinearAttenuation + d *
				   light->QuadraticAttenuation));

	    /* spotlight attenuation */
	    if (light->_Flags & LIGHT_SPOT) {
	       GLfloat PV_dot_dir = - DOT3(VP, light->_NormDirection);
	       if (PV_dot_dir < light->_CosCutoff) {
		  continue; /* this light makes no contribution */
	       }
	       else {
		  GLdouble x = PV_dot_dir * (EXP_TABLE_SIZE-1);
		  GLint k = (GLint) x;
		  GLfloat spot = (GLfloat) (light->_SpotExpTable[k][0]
				  + (x-k)*light->_SpotExpTable[k][1]);
		  attenuation *= spot;
	       }
	    }
	 }

	 if (attenuation < 1e-3)
	    continue;		/* this light makes no contribution */

	 n_dot_VP = DOT3( normal, VP );

	 /* which side are we lighting? */
	 if (n_dot_VP < 0.0F) {
	    if (!(IDX & LIGHT_TWOSIDE))
	       continue;
	    side = 1;
	    correction = -1;
	    n_dot_VP = -n_dot_VP;
	 }

	 /* accumulate diffuse term */
	 diffuse[side] += n_dot_VP * light->_dli * attenuation;

	 /* specular term */
	 if (ctx->Light.Model.LocalViewer) {
	    GLfloat v[3];
	    COPY_3V(v, vertex);
	    NORMALIZE_3FV(v);
	    SUB_3V(VP, VP, v);                /* h = VP + VPe */
	    h = VP;
	    NORMALIZE_3FV(h);
	 }
	 else if (light->_Flags & LIGHT_POSITIONAL) {
	    h = VP;
            /* Strangely, disabling this addition fixes a conformance
             * problem.  If this code is enabled, l_sed.c fails.
             */
	    /*ACC_3V(h, ctx->_EyeZDir);*/
	    NORMALIZE_3FV(h);
	 }
         else {
	    h = light->_h_inf_norm;
	 }

	 n_dot_h = correction * DOT3(normal, h);
	 if (n_dot_h > 0.0F) {
	    GLfloat spec_coef;
	    struct gl_shine_tab *tab = ctx->_ShineTable[side];
	    GET_SHINE_TAB_ENTRY( tab, n_dot_h, spec_coef);
	    specular[side] += spec_coef * light->_sli * attenuation;
	 }
      } /*loop over lights*/

      /* Now compute final color index */
      for (side = 0 ; side < NR_SIDES ; side++) {
	 struct gl_material *mat = &ctx->Light.Material[side];
	 GLfloat index;

	 if (specular[side] > 1.0F) {
	    index = mat->SpecularIndex;
	 }
	 else {
	    GLfloat d_a = mat->DiffuseIndex - mat->AmbientIndex;
	    GLfloat s_a = mat->SpecularIndex - mat->AmbientIndex;

	    index = mat->AmbientIndex
	       + diffuse[side] * (1.0F-specular[side]) * d_a
	       + specular[side] * s_a;

	    if (index > mat->SpecularIndex) {
	       index = mat->SpecularIndex;
	    }
	 }
	 indexResult[side][j] = (GLuint) (GLint) index;
      }
   } /*for vertex*/
}



static void TAG(init_light_tab)( void )
{
   _tnl_light_tab[IDX] = TAG(light_rgba);
   _tnl_light_fast_tab[IDX] = TAG(light_fast_rgba);
   _tnl_light_fast_single_tab[IDX] = TAG(light_fast_rgba_single);
   _tnl_light_spec_tab[IDX] = TAG(light_rgba_spec);
   _tnl_light_ci_tab[IDX] = TAG(light_ci);
}


#undef TAG
#undef IDX
#undef NR_SIDES
#undef NSTRIDE
#undef VSTRIDE
#undef CHECK_MATERIAL
#undef CHECK_END_VB
#undef DO_ANOTHER_NORMAL
#undef REUSE_LIGHT_RESULTS
#undef CMSTRIDE
#undef CHECK_COLOR_MATERIAL
#undef CHECK_VALIDATE
