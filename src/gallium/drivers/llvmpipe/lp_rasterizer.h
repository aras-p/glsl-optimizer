
/* Initially create and program a single rasterizer directly.  Later
 * will want multiple of these, one or two per core.  At that stage
 * will probably pass command buffers into the rasterizers rather than
 * individual function calls like this.
 */
struct lp_rasterizer;

struct lp_rast_state {
   /* State:
    */
   struct lp_jit_context jc;
   
   /* Shader itself:
    */
};

/* Coefficients necessary to run the shader at a given location:
 */
struct lp_rast_shader_inputs {

   /* Current rasterizer state:
    */
   const struct lp_rast_state *state;

   /* Attribute interpolation:
    */
   float oneoverarea;
   float x1;
   float y1;

   struct tgsi_interp_coef position_coef;
   struct tgsi_interp_coef *coef;
};


/* Rasterization information for a triangle known to be in this bin,
 * plus inputs to run the shader:
 */
struct lp_rast_triangle {
   /* one-pixel sized trivial accept offsets for each plane */
   float ei1;                   
   float ei2;
   float ei3;

   /* one-pixel sized trivial reject offsets for each plane */
   float eo1;                   
   float eo2;
   float eo3;

   /* y deltas for vertex pairs */
   float dy12;
   float dy23;
   float dy31;

   /* x deltas for vertex pairs */
   float dx12;
   float dx23;
   float dx31;

   /* State to run the shader: */
   struct lp_rast_shader_inputs inputs;
};



struct lp_rasterizer *lp_rast_create( void );

void lp_rast_bind_surfaces( struct lp_rasterizer *,
			    struct pipe_surface *color,
			    struct pipe_surface *zstencil,
			    const float *clear_color,
			    double clear_depth,
			    unsigned clear_stencil);

/* Begining of each tile:
 */
void lp_rast_start_tile( struct lp_rasterizer *,
			 unsigned x,
			 unsigned y );

void lp_rast_clear_color( struct lp_rasterizer * );

void lp_rast_clear_zstencil( struct lp_rasterizer * );

void lp_rast_load_color( struct lp_rasterizer * );

void lp_rast_load_zstencil( struct lp_rasterizer * );


/* Within a tile:
 */
void lp_rast_set_state( struct lp_rasterizer *,
		       const struct lp_rast_state * );

void lp_rast_triangle( struct lp_rasterizer *,
		       const struct lp_rast_triangle * );

void lp_rast_shade_tile( struct lp_rasterizer *,
			 const struct lp_rast_shader_inputs * );

/* End of tile:
 */
void lp_rast_store_color( struct lp_rasterizer * );

void lp_rast_store_zstencil( struct lp_rasterizer * );


/* Shutdown:
 */
void lp_rast_destroy( struct lp_rasterizer * );

