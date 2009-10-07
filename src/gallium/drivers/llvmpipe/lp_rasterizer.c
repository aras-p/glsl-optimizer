
struct lp_rasterizer {

   /* We can choose whatever layout for the internal tile storage we
    * prefer:
    */
   struct {
      unsigned color[TILESIZE][TILESIZE];
      unsigned depth[TILESIZE][TILESIZE];
      char stencil[TILESIZE][TILESIZE];
   } tile;

      
   unsigned x;
   unsigned y;

   
   struct {
      struct pipe_surface *color;
      struct pipe_surface *zstencil;
      unsigned clear_color;
      unsigned clear_depth;
      char clear_stencil;
   } state;
};

struct lp_rasterizer *lp_rast_create( void )
{
   return CALLOC_STRUCT(lp_rasterizer);
}

void lp_rast_bind_surfaces( struct lp_rasterizer *,
			    struct pipe_surface *color,
			    struct pipe_surface *zstencil,
			    const float *clear_color,
			    double clear_depth,
			    unsigned clear_stencil)
{
   pipe_surface_reference(&rast->state.color, color);
   pipe_surface_reference(&rast->state.depth, depth);
   rast->state.clear_color = util_pack_8888(clear_color);
   rast->state.clear_depth = clear_depth * 0xffffffff;
   rast->state.clear_stencil = clear_stencil;
}

/* Begining of each tile:
 */
void lp_rast_start_tile( struct lp_rasterizer *,
			 unsigned x,
			 unsigned y )
{
   rast->x = x;
   rast->y = y;
}

void lp_rast_clear_color( struct lp_rasterizer *rast )
{
   const unsigned clear_color = rast->state.clear_color;
   unsigned i, j;
   
   for (i = 0; i < TILESIZE; i++)
      for (j = 0; j < TILESIZE; j++)
	 rast->tile[i][j] = clear_color;
}

void lp_rast_clear_depth( struct lp_rasterizer *rast )
{
   const unsigned clear_depth = rast->state.clear_depth;
   unsigned i, j;
   
   for (i = 0; i < TILESIZE; i++)
      for (j = 0; j < TILESIZE; j++)
	 rast->tile[i][j] = clear_depth;
}

void lp_rast_clear_stencil( struct lp_rasterizer *rast )
{
   const unsigned clear_stencil = rast->state.clear_stencil;

   memset(rast->tile.stencil, clear_stencil, sizeof rast->tile.stencil );
}

void lp_rast_load_color( struct lp_rasterizer *rast )
{
   /* call u_tile func to load colors from surface */
}

void lp_rast_load_zstencil( struct lp_rasterizer *rast )
{
   /* call u_tile func to load depth (and stencil?) from surface */
}

/* Within a tile:
 */
void lp_rast_set_state( struct lp_rasterizer *rast,
		       const struct lp_rast_state *state )
{
   rast->shader_state = state;
}

void lp_rast_triangle( struct lp_rasterizer *rast,
		       const struct lp_rast_triangle *inputs )
{
   /* Set up the silly quad coef pointers
    */
   for (i = 0; i < 4; i++) {
      rast->quads[i].posCoef = inputs->posCoef;
      rast->quads[i].coef = inputs->coef;
   }

   /* Scan the tile in 4x4 chunks (?) and figure out which bits to
    * rasterize:
    */

}

void lp_rast_shade_tile( struct lp_rasterizer *rast,
			 const struct lp_rast_shader_inputs *inputs )
{
   /* Set up the silly quad coef pointers
    */
   for (i = 0; i < 4; i++) {
      rast->quads[i].posCoef = inputs->posCoef;
      rast->quads[i].coef = inputs->coef;
   }

   /* Use the existing preference for 8x2 (four quads) shading:
    */
   for (i = 0; i < TILESIZE; i += 8) {
      for (j = 0; j < TILESIZE; j += 2) {
	 rast->shader_state.shade( inputs->jc,
				   rast->x + i,
				   rast->y + j,
				   rast->quads, 4 );
      }
   }
}

/* End of tile:
 */
void lp_rast_store_color( struct lp_rasterizer *rast )
{
   /* call u_tile func to store colors to surface */
}

void lp_rast_store_zstencil( struct lp_rasterizer *rast )
{
   /* call u_tile func to store depth/stencil to surface */
}

/* Shutdown:
 */
void lp_rast_destroy( struct lp_rasterizer *rast )
{
   FREE(rast);
}

