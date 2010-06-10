
static void FUNC( struct draw_geometry_shader *shader,
                  unsigned pipe_prim,
                  unsigned count )
{
   struct draw_context *draw = shader->draw;

   boolean flatfirst = (draw->rasterizer->flatshade &&
                        draw->rasterizer->flatshade_first);
   unsigned i;

   if (0) debug_printf("%s %d\n", __FUNCTION__, count);


   switch (pipe_prim) {
   case PIPE_PRIM_POINTS:
      for (i = 0; i < count; i++) {
	 POINT( shader, i + 0 );
      }
      break;

   case PIPE_PRIM_LINES:
      for (i = 0; i+1 < count; i += 2) {
         LINE( shader , i + 0 , i + 1 );
      }
      break;

   case PIPE_PRIM_LINE_LOOP:
      if (count >= 2) {

         for (i = 1; i < count; i++) {
            LINE( shader, i - 1, i );
         }

	 LINE( shader, i - 1, 0 );
      }
      break;

   case PIPE_PRIM_LINE_STRIP:
      for (i = 1; i < count; i++) {
         LINE( shader, i - 1, i );
      }
      break;

   case PIPE_PRIM_TRIANGLES:
      for (i = 0; i+2 < count; i += 3) {
         TRIANGLE( shader, i + 0, i + 1, i + 2 );
      }
      break;

   case PIPE_PRIM_TRIANGLE_STRIP:
      if (flatfirst) {
         for (i = 0; i+2 < count; i++) {
            TRIANGLE( shader,
                      i + 0,
                      i + 1 + (i&1),
                      i + 2 - (i&1) );
         }
      }
      else {
         for (i = 0; i+2 < count; i++) {
            TRIANGLE( shader,
                      i + 0 + (i&1),
                      i + 1 - (i&1),
                      i + 2 );
         }
      }
      break;

   case PIPE_PRIM_TRIANGLE_FAN:
      if (count >= 3) {
         if (flatfirst) {
            for (i = 0; i+2 < count; i++) {
               TRIANGLE( shader,
                         i + 1,
                         i + 2,
                         0 );
            }
         }
         else {
            for (i = 0; i+2 < count; i++) {
               TRIANGLE( shader,
                         0,
                         i + 1,
                         i + 2 );
            }
         }
      }
      break;

   case PIPE_PRIM_POLYGON:
      {
         /* These bitflags look a little odd because we submit the
          * vertices as (1,2,0) to satisfy flatshade requirements.
          */
         ushort edge_next, edge_finish;

         if (flatfirst) {
            edge_next = DRAW_PIPE_EDGE_FLAG_2;
            edge_finish = DRAW_PIPE_EDGE_FLAG_0;
         }
         else {
            edge_next = DRAW_PIPE_EDGE_FLAG_0;
            edge_finish = DRAW_PIPE_EDGE_FLAG_1;
         }

	 for (i = 0; i+2 < count; i++) {

            if (flatfirst) {
               TRIANGLE( shader, 0, i + 1, i + 2 );
            }
            else {
               TRIANGLE( shader, i + 1, i + 2, 0 );
            }
	 }
      }
      break;

   default:
      assert(0);
      break;
   }
}


#undef TRIANGLE
#undef POINT
#undef LINE
#undef FUNC
