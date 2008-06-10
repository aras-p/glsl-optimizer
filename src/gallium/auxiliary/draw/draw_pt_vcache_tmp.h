

static void FUNC( struct draw_pt_front_end *frontend, 
                  pt_elt_func get_elt,
                  const void *elts,
                  unsigned count )
{
   struct vcache_frontend *vcache = (struct vcache_frontend *)frontend;
   struct draw_context *draw = vcache->draw;
   unsigned i;
   ushort flags;

   if (0) debug_printf("%s %d\n", __FUNCTION__, count);

   /* Note that no adjustment is made here for flatshade provoking
    * vertex.  This was previously the case, but was incorrect as the
    * same logic would be applied twice both here & in the pipeline
    * code or driver.  The rule is now that we just preserve the
    * original order in this code, and leave identification of the PV
    * to the pipeline and driver.
    */
   switch (vcache->input_prim) {
   case PIPE_PRIM_POINTS:
      for (i = 0; i < count; i ++) {
	 POINT( vcache,
                get_elt(elts, i + 0) );
      }
      break;

   case PIPE_PRIM_LINES:
      for (i = 0; i+1 < count; i += 2) {
         LINE( vcache, 
               DRAW_PIPE_RESET_STIPPLE,
               get_elt(elts, i + 0),
               get_elt(elts, i + 1));
      }
      break;

   case PIPE_PRIM_LINE_LOOP:  
      if (count >= 2) {
         flags = DRAW_PIPE_RESET_STIPPLE;

         for (i = 1; i < count; i++, flags = 0) {
            LINE( vcache, 
                  flags,
                  get_elt(elts, i - 1),
                  get_elt(elts, i ));
         }

	 LINE( vcache, 
               flags,
               get_elt(elts, i - 1),
               get_elt(elts, 0 ));
      }
      break;

   case PIPE_PRIM_LINE_STRIP:
      flags = DRAW_PIPE_RESET_STIPPLE;
      for (i = 1; i < count; i++, flags = 0) {
         LINE( vcache, 
               flags,
               get_elt(elts, i - 1),
               get_elt(elts, i ));
      }
      break;

   case PIPE_PRIM_TRIANGLES:
      for (i = 0; i+2 < count; i += 3) {
         TRIANGLE( vcache,
                   DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL, 
                   get_elt(elts, i + 0),
                   get_elt(elts, i + 1),
                   get_elt(elts, i + 2 ));
      }
      break;

   case PIPE_PRIM_TRIANGLE_STRIP:
      for (i = 0; i+2 < count; i++) {
         TRIANGLE( vcache,
                   DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL, 
                   get_elt(elts, i + 0 + (i&1)),
                   get_elt(elts, i + 1 - (i&1)),
                   get_elt(elts, i + 2 ));
      }
      break;

   case PIPE_PRIM_TRIANGLE_FAN:
      if (count >= 3) {
         for (i = 0; i+2 < count; i++) {
            TRIANGLE( vcache,
                      DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL, 
                      get_elt(elts, i + 1),
                      get_elt(elts, i + 2),
                      get_elt(elts, 0 ));
         }
      }
      break;


   case PIPE_PRIM_QUADS:
      for (i = 0; i+3 < count; i += 4) {
         QUAD( vcache,
               get_elt(elts, i + 0),
               get_elt(elts, i + 1),
               get_elt(elts, i + 2),
               get_elt(elts, i + 3));
      }
      break;

   case PIPE_PRIM_QUAD_STRIP:
      for (i = 0; i+3 < count; i += 2) {
         QUAD( vcache,
               get_elt(elts, i + 2),
               get_elt(elts, i + 0),
               get_elt(elts, i + 1),
               get_elt(elts, i + 3));
      }
      break;

   case PIPE_PRIM_POLYGON:
      {
         boolean flatfirst = (draw->rasterizer->flatshade && 
                              draw->rasterizer->flatshade_first);

         /* These bitflags look a little odd because we submit the
          * vertices as (1,2,0) to satisfy flatshade requirements.  
          *
          * Polygon is defined has having vertex 0 be the provoking
          * flatshade vertex and all known API's match this usage.
          * However, the PV's for the triangles we emit from this
          * decomposition vary according to the API, and hence we have
          * to choose between two ways of emitting the triangles.
          */
         if (flatfirst) {
            const ushort edge_first  = DRAW_PIPE_EDGE_FLAG_0;
            const ushort edge_middle = DRAW_PIPE_EDGE_FLAG_1;
            const ushort edge_last   = DRAW_PIPE_EDGE_FLAG_2;
            
            flags = DRAW_PIPE_RESET_STIPPLE | edge_first | edge_middle;
            
            for (i = 0; i+2 < count; i++, flags = edge_middle) {
               
               if (i + 3 == count)
                  flags |= edge_last;
               
               /* PV is first vertex */
               TRIANGLE( vcache,
                         flags,
                         get_elt(elts, 0),
                         get_elt(elts, i + 1),
                         get_elt(elts, i + 2));
            }
         }
         else {
            const ushort edge_first  = DRAW_PIPE_EDGE_FLAG_2;
            const ushort edge_middle = DRAW_PIPE_EDGE_FLAG_0;
            const ushort edge_last   = DRAW_PIPE_EDGE_FLAG_1;
            
            flags = DRAW_PIPE_RESET_STIPPLE | edge_first | edge_middle;
            
            for (i = 0; i+2 < count; i++, flags = edge_middle) {
               
               if (i + 3 == count)
                  flags |= edge_last;
               
               /* PV is third vertex */
               TRIANGLE( vcache,
                         flags,
                         get_elt(elts, i + 1),
                         get_elt(elts, i + 2),
                         get_elt(elts, 0));
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
