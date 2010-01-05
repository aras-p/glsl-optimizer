static unsigned trim( unsigned count, unsigned first, unsigned incr )
{
   return count - (count - first) % incr; 
}

static void FUNC(struct draw_pt_front_end *frontend,
                 pt_elt_func get_elt,
                 const void *elts,
                 unsigned count)
{
   struct varray_frontend *varray = (struct varray_frontend *)frontend;
   unsigned start = (unsigned) ((char *) elts - (char *) NULL);

   unsigned j;
   unsigned first, incr;

   draw_pt_split_prim(varray->input_prim, &first, &incr);
   
   /* Sanitize primitive length:
    */
   count = trim(count, first, incr); 
   if (count < first)
      return;

#if 0
   debug_printf("%s (%d) %d/%d\n", __FUNCTION__,
                varray->input_prim,
                start, count);
#endif

   switch (varray->input_prim) {
   case PIPE_PRIM_POINTS:
   case PIPE_PRIM_LINES:
   case PIPE_PRIM_TRIANGLES:
   case PIPE_PRIM_LINE_STRIP:
   case PIPE_PRIM_TRIANGLE_STRIP:
   case PIPE_PRIM_QUADS:
   case PIPE_PRIM_QUAD_STRIP:
   case PIPE_PRIM_LINES_ADJACENCY:
   case PIPE_PRIM_LINE_STRIP_ADJACENCY:
   case PIPE_PRIM_TRIANGLES_ADJACENCY:
   case PIPE_PRIM_TRIANGLE_STRIP_ADJACENCY:
      for (j = 0; j < count;) {
         unsigned remaining = count - j;
         unsigned nr = trim( MIN2(varray->driver_fetch_max, remaining), first, incr );
         varray_flush_linear(varray, start + j, nr);
         j += nr;
         if (nr != remaining) 
            j -= (first - incr);
      }
      break;

   case PIPE_PRIM_LINE_LOOP:
      /* Always have to decompose as we've stated that this will be
       * emitted as a line-strip.
       */
      for (j = 0; j < count;) {
         unsigned remaining = count - j;
         unsigned nr = trim( MIN2(varray->fetch_max-1, remaining), first, incr );
         varray_line_loop_segment(varray, start, j, nr, nr == remaining);
         j += nr;
         if (nr != remaining) 
            j -= (first - incr);
      }
      break;


   case PIPE_PRIM_POLYGON:
   case PIPE_PRIM_TRIANGLE_FAN: 
      if (count < varray->driver_fetch_max) {
         varray_flush_linear(varray, start, count);
      }
      else {
         for ( j = 0; j < count;) {
            unsigned remaining = count - j;
            unsigned nr = trim( MIN2(varray->fetch_max-1, remaining), first, incr );
            varray_fan_segment(varray, start, j, nr);
            j += nr;
            if (nr != remaining) 
               j -= (first - incr);
         }
      }
      break;

   default:
      assert(0);
      break;
   }
}

#undef TRIANGLE
#undef QUAD
#undef POINT
#undef LINE
#undef FUNC
