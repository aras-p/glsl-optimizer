

static void FUNC( ARGS,
                  unsigned count )
{
   LOCAL_VARS;

   switch (prim) {
   case PIPE_PRIM_POINTS:
      for (i = 0; i < count; i ++) {
	 POINT( (i + 0) );
      }
      break;

   case PIPE_PRIM_LINES:
      for (i = 0; i+1 < count; i += 2) {
         LINE( DRAW_PIPE_RESET_STIPPLE,
               (i + 0),
               (i + 1));
      }
      break;

   case PIPE_PRIM_LINE_LOOP:
      if (count >= 2) {
         flags = DRAW_PIPE_RESET_STIPPLE;

         for (i = 1; i < count; i++, flags = 0) {
            LINE( flags,
                  (i - 1),
                  (i ));
         }

	 LINE( flags,
               (i - 1),
               (0 ));
      }
      break;

   case PIPE_PRIM_LINE_STRIP:
      flags = DRAW_PIPE_RESET_STIPPLE;
      for (i = 1; i < count; i++, flags = 0) {
         LINE( flags,
               (i - 1),
               (i ));
      }
      break;

   case PIPE_PRIM_TRIANGLES:
      for (i = 0; i+2 < count; i += 3) {
         if (flatfirst) {
            /* put provoking vertex in last pos for clipper */
            TRIANGLE( DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL,
                      (i + 1),
                      (i + 2),
                      (i + 0 ));
         }
         else {
            TRIANGLE( DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL,
                      (i + 0),
                      (i + 1),
                      (i + 2 ));
         }
      }
      break;

   case PIPE_PRIM_TRIANGLE_STRIP:
      if (flatfirst) {
         for (i = 0; i+2 < count; i++) {
            TRIANGLE( DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL,
                      (i + 1 + (i&1)),
                      (i + 2 - (i&1)),
                      (i + 0) );
         }
      }
      else {
         for (i = 0; i+2 < count; i++) {
            TRIANGLE( DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL,
                      (i + 0 + (i&1)),
                      (i + 1 - (i&1)),
                      (i + 2 ));
         }
      }
      break;

   case PIPE_PRIM_TRIANGLE_FAN:
      if (count >= 3) {
         if (flatfirst) {
            for (i = 0; i+2 < count; i++) {
               TRIANGLE( DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL,
                         (i + 2),
                         0,
                         (i + 1) );
            }
         }
         else {
            for (i = 0; i+2 < count; i++) {
               TRIANGLE( DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL,
                         (0),
                         (i + 1),
                         (i + 2 ));
            }
         }
      }
      break;


   case PIPE_PRIM_QUADS:
      if (flatfirst) {
         for (i = 0; i+3 < count; i += 4) {
            QUAD( (i + 1),
                  (i + 2),
                  (i + 3),
                  (i + 0) );
         }
      }
      else {
         for (i = 0; i+3 < count; i += 4) {
            QUAD( (i + 0),
                  (i + 1),
                  (i + 2),
                  (i + 3));
         }
      }
      break;

   case PIPE_PRIM_QUAD_STRIP:
      if (flatfirst) {
         for (i = 0; i+3 < count; i += 2) {
            QUAD( (i + 1),
                  (i + 3),
                  (i + 2),
                  (i + 0) );
         }
      }
      else {
         for (i = 0; i+3 < count; i += 2) {
            QUAD( (i + 2),
                  (i + 0),
                  (i + 1),
                  (i + 3));
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

	 for (i = 0; i+2 < count; i++, flags = edge_middle) {

            if (i + 3 == count)
               flags |= edge_last;

	    TRIANGLE( flags,
                      (i + 1),
                      (i + 2),
                      (0));
	 }
      }
      break;

   default:
      assert(0);
      break;
   }

   FLUSH;
}


#undef TRIANGLE
#undef QUAD
#undef POINT
#undef LINE
#undef FUNC
