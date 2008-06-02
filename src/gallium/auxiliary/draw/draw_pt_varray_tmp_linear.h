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
   unsigned start = (unsigned)elts;

   unsigned i, j;
   unsigned first, incr;

   varray->fetch_start = start;

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
      for (j = 0; j < count;) {
         unsigned remaining = count - j;
         unsigned nr = trim( MIN2(varray->fetch_max, remaining), first, incr );
         varray_flush_linear(varray, start + j, nr);
         j += nr;
         if (nr != remaining) 
            j -= (first - incr);
      }
      break;

   case PIPE_PRIM_LINE_LOOP:
      if (count >= 2) {
         unsigned fetch_max = MIN2(FETCH_MAX, varray->fetch_max);
         for (j = 0; j + first <= count; j += i) {
            unsigned end = MIN2(fetch_max, count - j);
            end -= (end % incr);
            for (i = 1; i < end; i++) {
               LINE(varray, i - 1, i);
            }
            LINE(varray, i - 1, 0);
            i = end;
            fetch_init(varray, end);
            varray_flush(varray);
         }
      }
      break;


   case PIPE_PRIM_POLYGON:
   case PIPE_PRIM_TRIANGLE_FAN: {
      unsigned fetch_max = MIN2(FETCH_MAX, varray->fetch_max);
      for (j = 0; j + first <= count; j += i) {
         unsigned end = MIN2(fetch_max, count - j);
         end -= (end % incr);
         for (i = 2; i < end; i++) {
            TRIANGLE(varray, 0, i - 1, i);
         }
         i = end;
         fetch_init(varray, end);
         varray_flush(varray);
      }
      break;
   }

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
