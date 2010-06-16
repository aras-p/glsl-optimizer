
static void FUNC( struct pt_so_emit *so,
                  const struct draw_prim_info *input_prims,
                  const struct draw_vertex_info *input_verts,
                  unsigned start,
                  unsigned count)
{
   struct draw_context *draw = so->draw;

   boolean flatfirst = (draw->rasterizer->flatshade &&
                        draw->rasterizer->flatshade_first);
   unsigned i;
   LOCAL_VARS

   if (0) debug_printf("%s %d\n", __FUNCTION__, count);

   debug_assert(input_prims->primitive_count == 1);

   switch (input_prims->prim) {
   case PIPE_PRIM_POINTS:
      for (i = 0; i < count; i++) {
	 POINT( so, start + i + 0 );
      }
      break;

   case PIPE_PRIM_LINES:
      for (i = 0; i+1 < count; i += 2) {
         LINE( so , start + i + 0 , start + i + 1 );
      }
      break;

   case PIPE_PRIM_LINE_LOOP:
      if (count >= 2) {

         for (i = 1; i < count; i++) {
            LINE( so, start + i - 1, start + i );
         }

	 LINE( so, start + i - 1, start );
      }
      break;

   case PIPE_PRIM_LINE_STRIP:
      for (i = 1; i < count; i++) {
         LINE( so, start + i - 1, start + i );
      }
      break;

   case PIPE_PRIM_TRIANGLES:
      for (i = 0; i+2 < count; i += 3) {
         TRIANGLE( so, start + i + 0, start + i + 1, start + i + 2 );
      }
      break;

   case PIPE_PRIM_TRIANGLE_STRIP:
      if (flatfirst) {
         for (i = 0; i+2 < count; i++) {
            TRIANGLE( so,
                      start + i + 0,
                      start + i + 1 + (i&1),
                      start + i + 2 - (i&1) );
         }
      }
      else {
         for (i = 0; i+2 < count; i++) {
            TRIANGLE( so,
                      start + i + 0 + (i&1),
                      start + i + 1 - (i&1),
                      start + i + 2 );
         }
      }
      break;

   case PIPE_PRIM_TRIANGLE_FAN:
      if (count >= 3) {
         if (flatfirst) {
            for (i = 0; i+2 < count; i++) {
               TRIANGLE( so,
                         start + i + 1,
                         start + i + 2,
                         start );
            }
         }
         else {
            for (i = 0; i+2 < count; i++) {
               TRIANGLE( so,
                         start,
                         start + i + 1,
                         start + i + 2 );
            }
         }
      }
      break;

   case PIPE_PRIM_POLYGON:
      {
         /* These bitflags look a little odd because we submit the
          * vertices as (1,2,0) to satisfy flatshade requirements.
          */

	 for (i = 0; i+2 < count; i++) {

            if (flatfirst) {
               TRIANGLE( so, start + 0, start + i + 1, start + i + 2 );
            }
            else {
               TRIANGLE( so, start + i + 1, start + i + 2, start + 0 );
            }
	 }
      }
      break;

   default:
      debug_assert(!"Unsupported primitive in stream output");
      break;
   }
}


#undef TRIANGLE
#undef POINT
#undef LINE
#undef FUNC
