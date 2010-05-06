

static void FUNC( struct draw_pt_front_end *frontend, 
                  pt_elt_func get_elt,
                  const void *elts,
                  int elt_bias,
                  unsigned count )
{
   struct vcache_frontend *vcache = (struct vcache_frontend *)frontend;
   struct draw_context *draw = vcache->draw;

   boolean flatfirst = (draw->rasterizer->flatshade && 
                        draw->rasterizer->flatshade_first);
   unsigned i;
   ushort flags;

   if (0) debug_printf("%s %d\n", __FUNCTION__, count);


   switch (vcache->input_prim) {
   case PIPE_PRIM_POINTS:
      for (i = 0; i < count; i ++) {
	 POINT( vcache,
                get_elt(elts, i + 0) + elt_bias );
      }
      break;

   case PIPE_PRIM_LINES:
      for (i = 0; i+1 < count; i += 2) {
         LINE( vcache, 
               DRAW_PIPE_RESET_STIPPLE,
               get_elt(elts, i + 0) + elt_bias,
               get_elt(elts, i + 1) + elt_bias);
      }
      break;

   case PIPE_PRIM_LINE_LOOP:  
      if (count >= 2) {
         flags = DRAW_PIPE_RESET_STIPPLE;

         for (i = 1; i < count; i++, flags = 0) {
            LINE( vcache, 
                  flags,
                  get_elt(elts, i - 1) + elt_bias,
                  get_elt(elts, i ) + elt_bias);
         }

	 LINE( vcache, 
               flags,
               get_elt(elts, i - 1) + elt_bias,
               get_elt(elts, 0 ) + elt_bias);
      }
      break;

   case PIPE_PRIM_LINE_STRIP:
      flags = DRAW_PIPE_RESET_STIPPLE;
      for (i = 1; i < count; i++, flags = 0) {
         LINE( vcache, 
               flags,
               get_elt(elts, i - 1) + elt_bias,
               get_elt(elts, i ) + elt_bias);
      }
      break;

   case PIPE_PRIM_TRIANGLES:
      for (i = 0; i+2 < count; i += 3) {
         TRIANGLE( vcache,
                   DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL, 
                   get_elt(elts, i + 0) + elt_bias,
                   get_elt(elts, i + 1) + elt_bias,
                   get_elt(elts, i + 2 ) + elt_bias);
      }
      break;

   case PIPE_PRIM_TRIANGLE_STRIP:
      if (flatfirst) {
         for (i = 0; i+2 < count; i++) {
            TRIANGLE( vcache,
                      DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL, 
                      get_elt(elts, i + 0) + elt_bias,
                      get_elt(elts, i + 1 + (i&1)) + elt_bias,
                      get_elt(elts, i + 2 - (i&1)) + elt_bias);
         }
      }
      else {
         for (i = 0; i+2 < count; i++) {
            TRIANGLE( vcache,
                      DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL, 
                      get_elt(elts, i + 0 + (i&1)) + elt_bias,
                      get_elt(elts, i + 1 - (i&1)) + elt_bias,
                      get_elt(elts, i + 2 ) + elt_bias);
         }
      }
      break;

   case PIPE_PRIM_TRIANGLE_FAN:
      if (count >= 3) {
         if (flatfirst) {
            for (i = 0; i+2 < count; i++) {
               TRIANGLE( vcache,
                         DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL, 
                         get_elt(elts, i + 1) + elt_bias,
                         get_elt(elts, i + 2) + elt_bias,
                         get_elt(elts, 0 ) + elt_bias);
            }
         }
         else {
            for (i = 0; i+2 < count; i++) {
               TRIANGLE( vcache,
                         DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL, 
                         get_elt(elts, 0) + elt_bias,
                         get_elt(elts, i + 1) + elt_bias,
                         get_elt(elts, i + 2 ) + elt_bias);
            }
         }
      }
      break;


   case PIPE_PRIM_QUADS:
      for (i = 0; i+3 < count; i += 4) {
         QUAD( vcache,
               get_elt(elts, i + 0) + elt_bias,
               get_elt(elts, i + 1) + elt_bias,
               get_elt(elts, i + 2) + elt_bias,
               get_elt(elts, i + 3) + elt_bias );
      }
      break;

   case PIPE_PRIM_QUAD_STRIP:
      for (i = 0; i+3 < count; i += 2) {
         QUAD( vcache,
               get_elt(elts, i + 2) + elt_bias,
               get_elt(elts, i + 0) + elt_bias,
               get_elt(elts, i + 1) + elt_bias,
               get_elt(elts, i + 3) + elt_bias );
      }
      break;

   case PIPE_PRIM_POLYGON:
      {
         /* These bitflags look a little odd because we submit the
          * vertices as (1,2,0) to satisfy flatshade requirements.  
          */
         ushort edge_next, edge_finish;

         if (flatfirst) {
            flags = (DRAW_PIPE_RESET_STIPPLE |
                     DRAW_PIPE_EDGE_FLAG_1 |
                     DRAW_PIPE_EDGE_FLAG_2);
            edge_next = DRAW_PIPE_EDGE_FLAG_2;
            edge_finish = DRAW_PIPE_EDGE_FLAG_0;
         }
         else {
            flags = (DRAW_PIPE_RESET_STIPPLE |
                     DRAW_PIPE_EDGE_FLAG_2 |
                     DRAW_PIPE_EDGE_FLAG_0);
            edge_next = DRAW_PIPE_EDGE_FLAG_0;
            edge_finish = DRAW_PIPE_EDGE_FLAG_1;
         }

	 for (i = 0; i+2 < count; i++, flags = edge_next) {

            if (i + 3 == count)
               flags |= edge_finish;

            if (flatfirst) {
               TRIANGLE( vcache,
                         flags,
                         get_elt(elts, 0) + elt_bias,
                         get_elt(elts, i + 1) + elt_bias,
                         get_elt(elts, i + 2) + elt_bias );
            }
            else {
               TRIANGLE( vcache,
                         flags,
                         get_elt(elts, i + 1) + elt_bias,
                         get_elt(elts, i + 2) + elt_bias,
                         get_elt(elts, 0) + elt_bias);
            }
	 }
      }
      break;

   default:
      assert(0);
      break;
   }
   
   vcache_flush( vcache );
}


#undef TRIANGLE
#undef QUAD
#undef POINT
#undef LINE
#undef FUNC
