
static void FUNC(struct draw_pt_front_end *frontend,
                 pt_elt_func get_elt,
                 const void *elts,
                 unsigned count)
{
   struct varray_frontend *varray = (struct varray_frontend *)frontend;
   struct draw_context *draw = varray->draw;
   unsigned start = (unsigned)elts;

   boolean flatfirst = (draw->rasterizer->flatshade &&
                        draw->rasterizer->flatshade_first);
   unsigned i, flags;

#if 0
   debug_printf("%s (%d) %d/%d\n", __FUNCTION__, draw->prim, start, count);
#endif
#if 0
   debug_printf("INPUT PRIM = %d (start = %d, count = %d)\n", varray->input_prim,
                start, count);
#endif

   for (i = 0; i < count; ++i) {
      varray->fetch_elts[i] = start + i;
   }
   varray->fetch_count = count;

   switch (varray->input_prim) {
   case PIPE_PRIM_POINTS:
      for (i = 0; i < count; i ++) {
         POINT(varray, i + 0);
      }
      break;

   case PIPE_PRIM_LINES:
      for (i = 0; i+1 < count; i += 2) {
         LINE(varray, DRAW_PIPE_RESET_STIPPLE,
              i + 0, i + 1);
      }
      break;

   case PIPE_PRIM_LINE_LOOP:
      if (count >= 2) {
         flags = DRAW_PIPE_RESET_STIPPLE;

         for (i = 1; i < count; i++, flags = 0) {
            LINE(varray, flags, i - 1, i);
         }
         LINE(varray, flags, i - 1, 0);
      }
      break;

   case PIPE_PRIM_LINE_STRIP:
      flags = DRAW_PIPE_RESET_STIPPLE;
      for (i = 1; i < count; i++, flags = 0) {
         LINE(varray, flags, i - 1, i);
      }
      break;

   case PIPE_PRIM_TRIANGLES:
      for (i = 0; i+2 < count; i += 3) {
         TRIANGLE(varray, DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL,
                  i + 0, i + 1, i + 2);
      }
      break;

   case PIPE_PRIM_TRIANGLE_STRIP:
      if (flatfirst) {
         for (i = 0; i+2 < count; i++) {
            TRIANGLE(varray, DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL,
                     i + 0, i + 1 + (i&1), i + 2 - (i&1));
         }
      }
      else {
         for (i = 0; i+2 < count; i++) {
            TRIANGLE(varray, DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL,
                     i + 0 + (i&1), i + 1 - (i&1), i + 2);
         }
      }
      break;

   case PIPE_PRIM_TRIANGLE_FAN:
      if (count >= 3) {
         if (flatfirst) {
            flags = DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL;
            for (i = 0; i+2 < count; i++) {
               TRIANGLE(varray, flags, i + 1, i + 2, 0);
            }
         }
         else {
            flags = DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL;
            for (i = 0; i+2 < count; i++) {
               TRIANGLE(varray, flags, 0, i + 1, i + 2);
            }
         }
      }
      break;

   case PIPE_PRIM_QUADS:
      for (i = 0; i+3 < count; i += 4) {
         QUAD(varray, i + 0, i + 1, i + 2, i + 3);
      }
      break;

   case PIPE_PRIM_QUAD_STRIP:
      for (i = 0; i+3 < count; i += 2) {
         QUAD(varray, i + 2, i + 0, i + 1, i + 3);
      }
      break;

   case PIPE_PRIM_POLYGON:
   {
         /* These bitflags look a little odd because we submit the
          * vertices as (1,2,0) to satisfy flatshade requirements.
          */
         const unsigned edge_first  = DRAW_PIPE_EDGE_FLAG_2;
         const unsigned edge_middle = DRAW_PIPE_EDGE_FLAG_0;
         const unsigned edge_last   = DRAW_PIPE_EDGE_FLAG_1;

         flags = DRAW_PIPE_RESET_STIPPLE | edge_first | edge_middle;

	 for (i = 0; i+2 < count; i++, flags = edge_middle) {

            if (i + 3 == count)
               flags |= edge_last;

	    TRIANGLE(varray, flags, i + 1, i + 2, 0);
	 }
      }
      break;

   default:
      assert(0);
      break;
   }

   varray_flush(varray);
}

#undef TRIANGLE
#undef QUAD
#undef POINT
#undef LINE
#undef FUNC
