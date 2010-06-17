
static void FUNC( struct draw_geometry_shader *shader,
                  const struct draw_prim_info *input_prims,
                  const struct draw_vertex_info *input_verts,
                  struct draw_prim_info *output_prims,
                  struct draw_vertex_info *output_verts)
{
   struct draw_context *draw = shader->draw;

   boolean flatfirst = (draw->rasterizer->flatshade &&
                        draw->rasterizer->flatshade_first);
   unsigned i, j;
   unsigned count = input_prims->count;
   LOCAL_VARS

   if (0) debug_printf("%s %d\n", __FUNCTION__, count);

   debug_assert(input_prims->primitive_count == 1);

   switch (input_prims->prim) {
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

   case PIPE_PRIM_LINES_ADJACENCY:
      for (i = 0; i+3 < count; i += 4) {
         LINE_ADJ( shader , i + 0 , i + 1, i + 2, i + 3 );
      }
      break;
   case PIPE_PRIM_LINE_STRIP_ADJACENCY:
      for (i = 1; i + 2 < count; i++) {
         LINE_ADJ( shader, i - 1, i, i + 1, i + 2 );
      }
      break;

   case PIPE_PRIM_TRIANGLES_ADJACENCY:
      for (i = 0; i+5 < count; i += 5) {
         TRI_ADJ( shader, i + 0, i + 1, i + 2,
                  i + 3, i + 4, i + 5);
      }
      break;
   case PIPE_PRIM_TRIANGLE_STRIP_ADJACENCY:
      for (i = 0, j = 0; i+5 < count; i += 2, ++j) {
         TRI_ADJ( shader,
                  i + 0,
                  i + 1 + 2*(j&1),
                  i + 2 + 2*(j&1),
                  i + 3 - 2*(j&1),
                  i + 4 - 2*(j&1),
                  i + 5);
      }
      break;

   default:
      debug_assert(!"Unsupported primitive in geometry shader");
      break;
   }
}


#undef TRIANGLE
#undef TRI_ADJ
#undef POINT
#undef LINE
#undef LINE_ADJ
#undef FUNC
#undef LOCAL_VARS
