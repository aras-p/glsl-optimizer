
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
   unsigned i, j;
   ushort flags;
   unsigned first, incr;

   varray->fetch_start = start;

   draw_pt_split_prim(varray->input_prim, &first, &incr);

#if 0
   debug_printf("%s (%d) %d/%d\n", __FUNCTION__,
                varray->input_prim,
                start, count);
#endif

   switch (varray->input_prim) {
   case PIPE_PRIM_POINTS:
      for (j = 0; j + first <= count; j += i) {
         unsigned end = MIN2(FETCH_MAX, count - j);
         end -= (end % incr);
         for (i = 0; i < end; i++) {
            POINT(varray, i + 0);
         }
         i = end;
         fetch_init(varray, end);
         varray_flush(varray);
      }
      break;

   case PIPE_PRIM_LINES:
      for (j = 0; j + first <= count; j += i) {
         unsigned end = MIN2(FETCH_MAX, count - j);
         end -= (end % incr);
         for (i = 0; i+1 < end; i += 2) {
            LINE(varray, DRAW_PIPE_RESET_STIPPLE,
                 i + 0, i + 1);
         }
         i = end;
         fetch_init(varray, end);
         varray_flush(varray);
      }
      break;

   case PIPE_PRIM_LINE_LOOP:
      if (count >= 2) {
         flags = DRAW_PIPE_RESET_STIPPLE;

         for (j = 0; j + first <= count; j += i) {
            unsigned end = MIN2(FETCH_MAX, count - j);
            end -= (end % incr);
            for (i = 1; i < end; i++, flags = 0) {
               LINE(varray, flags, i - 1, i);
            }
            LINE(varray, flags, i - 1, 0);
            i = end;
            fetch_init(varray, end);
            varray_flush(varray);
         }
      }
      break;

   case PIPE_PRIM_LINE_STRIP:
      flags = DRAW_PIPE_RESET_STIPPLE;
      for (j = 0; j + first <= count; j += i) {
         unsigned end = MIN2(FETCH_MAX, count - j);
         end -= (end % incr);
         for (i = 1; i < end; i++, flags = 0) {
            LINE(varray, flags, i - 1, i);
         }
         i = end;
         fetch_init(varray, end);
         varray_flush(varray);
      }
      break;

   case PIPE_PRIM_TRIANGLES:
      for (j = 0; j + first <= count; j += i) {
         unsigned end = MIN2(FETCH_MAX, count - j);
         end -= (end % incr);
         for (i = 0; i+2 < end; i += 3) {
            TRIANGLE(varray, DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL,
                     i + 0, i + 1, i + 2);
         }
         i = end;
         fetch_init(varray, end);
         varray_flush(varray);
      }
      break;

   case PIPE_PRIM_TRIANGLE_STRIP:
      if (flatfirst) {
         for (j = 0; j + first <= count; j += i) {
            unsigned end = MIN2(FETCH_MAX, count - j);
            end -= (end % incr);
            for (i = 0; i+2 < end; i++) {
               TRIANGLE(varray, DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL,
                        i + 0, i + 1 + (i&1), i + 2 - (i&1));
            }
            i = end;
            fetch_init(varray, end);
            varray_flush(varray);
            if (j + first + i <= count) {
               varray->fetch_start -= 2;
               i -= 2;
            }
         }
      }
      else {
         for (j = 0; j + first <= count; j += i) {
            unsigned end = MIN2(FETCH_MAX, count - j);
            end -= (end  % incr);
            for (i = 0; i + 2 < end; i++) {
               TRIANGLE(varray, DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL,
                        i + 0 + (i&1), i + 1 - (i&1), i + 2);
            }
            i = end;
            fetch_init(varray, end);
            varray_flush(varray);
            if (j + first + i <= count) {
               varray->fetch_start -= 2;
               i -= 2;
            }
         }
      }
      break;

   case PIPE_PRIM_TRIANGLE_FAN:
      if (count >= 3) {
         if (flatfirst) {
            flags = DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL;
            for (j = 0; j + first <= count; j += i) {
               unsigned end = MIN2(FETCH_MAX, count - j);
               end -= (end % incr);
               for (i = 0; i+2 < end; i++) {
                  TRIANGLE(varray, flags, i + 1, i + 2, 0);
               }
               i = end;
               fetch_init(varray, end);
               varray_flush(varray);
            }
         }
         else {
            flags = DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL;
            for (j = 0; j + first <= count; j += i) {
               unsigned end = MIN2(FETCH_MAX, count - j);
               end -= (end % incr);
               for (i = 0; i+2 < end; i++) {
                  TRIANGLE(varray, flags, 0, i + 1, i + 2);
               }
               i = end;
               fetch_init(varray, end);
               varray_flush(varray);
            }
         }
      }
      break;

   case PIPE_PRIM_QUADS:
      for (j = 0; j + first <= count; j += i) {
         unsigned end = MIN2(FETCH_MAX, count - j);
         end -= (end % incr);
         for (i = 0; i+3 < end; i += 4) {
            QUAD(varray, i + 0, i + 1, i + 2, i + 3);
         }
         i = end;
         fetch_init(varray, end);
         varray_flush(varray);
      }
      break;

   case PIPE_PRIM_QUAD_STRIP:
      for (j = 0; j + first <= count; j += i) {
         unsigned end = MIN2(FETCH_MAX, count - j);
         end -= (end % incr);
         for (i = 0; i+3 < end; i += 2) {
            QUAD(varray, i + 2, i + 0, i + 1, i + 3);
         }
         i = end;
         fetch_init(varray, end);
         varray_flush(varray);
         if (j + first + i <= count) {
            varray->fetch_start -= 2;
            i -= 2;
         }
      }
      break;

   case PIPE_PRIM_POLYGON:
   {
      /* These bitflags look a little odd because we submit the
       * vertices as (1,2,0) to satisfy flatshade requirements.
       */
      const ushort edge_first  = DRAW_PIPE_EDGE_FLAG_2;
      const ushort edge_middle = DRAW_PIPE_EDGE_FLAG_0;
      const ushort edge_last   = DRAW_PIPE_EDGE_FLAG_1;

      flags = DRAW_PIPE_RESET_STIPPLE | edge_first | edge_middle;
      for (j = 0; j + first <= count; j += i) {
         unsigned end = MIN2(FETCH_MAX, count - j);
         end -= (end % incr);
         for (i = 0; i+2 < end; i++, flags = edge_middle) {

            if (i + 3 == count)
               flags |= edge_last;

            TRIANGLE(varray, flags, i + 1, i + 2, 0);
         }
         i = end;
         fetch_init(varray, end);
         varray_flush(varray);
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
