
#include "util/u_debug.h"
#include "pipe/p_inlines.h"
#include "pipe/internal/p_winsys_screen.h"
#include "pipe/p_compiler.h"

#include "draw/draw_vbuf.h"

#include "nv04_context.h"
#include "nv04_state.h"

#define VERTEX_SIZE 40
#define VERTEX_BUFFER_SIZE (4096*VERTEX_SIZE) // 4096 vertices of 40 bytes each

/**
 * Primitive renderer for nv04.
 */
struct nv04_vbuf_render {
	struct vbuf_render base;

	struct nv04_context *nv04;   

	/** Vertex buffer */
	unsigned char* buffer;

	/** Vertex size in bytes */
	unsigned vertex_size;

	/** Current primitive */
	unsigned prim;
};


/**
 * Basically a cast wrapper.
 */
static INLINE struct nv04_vbuf_render *
nv04_vbuf_render( struct vbuf_render *render )
{
	assert(render);
	return (struct nv04_vbuf_render *)render;
}


static const struct vertex_info *
nv04_vbuf_render_get_vertex_info( struct vbuf_render *render )
{
	struct nv04_vbuf_render *nv04_render = nv04_vbuf_render(render);
	struct nv04_context *nv04 = nv04_render->nv04;
	return &nv04->vertex_info;
}


static boolean
nv04_vbuf_render_allocate_vertices( struct vbuf_render *render,
		ushort vertex_size,
		ushort nr_vertices )
{
	struct nv04_vbuf_render *nv04_render = nv04_vbuf_render(render);

	nv04_render->buffer = (unsigned char*) MALLOC(VERTEX_BUFFER_SIZE);
	assert(!nv04_render->buffer);

	return nv04_render->buffer ? TRUE : FALSE;
}

static void *
nv04_vbuf_render_map_vertices( struct vbuf_render *render )
{
	struct nv04_vbuf_render *nv04_render = nv04_vbuf_render(render);
	return nv04_render->buffer;
}

static void
nv04_vbuf_render_unmap_vertices( struct vbuf_render *render,
		ushort min_index,
		ushort max_index )
{
}

static boolean 
nv04_vbuf_render_set_primitive( struct vbuf_render *render, 
		unsigned prim )
{
	struct nv04_vbuf_render *nv04_render = nv04_vbuf_render(render);

	if (prim <= PIPE_PRIM_LINE_STRIP)
		return FALSE;

	nv04_render->prim = prim;
	return TRUE;
}

static INLINE void nv04_2triangles(struct nv04_context* nv04, unsigned char* buffer, ushort v0, ushort v1, ushort v2, ushort v3, ushort v4, ushort v5)
{
	BEGIN_RING(fahrenheit,NV04_DX5_TEXTURED_TRIANGLE_TLVERTEX_SX(0xA),49);
	OUT_RINGp(buffer + VERTEX_SIZE * v0,8);
	OUT_RINGp(buffer + VERTEX_SIZE * v1,8);
	OUT_RINGp(buffer + VERTEX_SIZE * v2,8);
	OUT_RINGp(buffer + VERTEX_SIZE * v3,8);
	OUT_RINGp(buffer + VERTEX_SIZE * v4,8);
	OUT_RINGp(buffer + VERTEX_SIZE * v5,8);
	OUT_RING(0xFEDCBA);
}

static INLINE void nv04_1triangle(struct nv04_context* nv04, unsigned char* buffer, ushort v0, ushort v1, ushort v2)
{
	BEGIN_RING(fahrenheit,NV04_DX5_TEXTURED_TRIANGLE_TLVERTEX_SX(0xD),25);
	OUT_RINGp(buffer + VERTEX_SIZE * v0,8);
	OUT_RINGp(buffer + VERTEX_SIZE * v1,8);
	OUT_RINGp(buffer + VERTEX_SIZE * v2,8);
	OUT_RING(0xFED);
}

static INLINE void nv04_1quad(struct nv04_context* nv04, unsigned char* buffer, ushort v0, ushort v1, ushort v2, ushort v3)
{
	BEGIN_RING(fahrenheit,NV04_DX5_TEXTURED_TRIANGLE_TLVERTEX_SX(0xC),33);
	OUT_RINGp(buffer + VERTEX_SIZE * v0,8);
	OUT_RINGp(buffer + VERTEX_SIZE * v1,8);
	OUT_RINGp(buffer + VERTEX_SIZE * v2,8);
	OUT_RINGp(buffer + VERTEX_SIZE * v3,8);
	OUT_RING(0xFECEDC);
}

static void nv04_vbuf_render_triangles_elts(struct nv04_vbuf_render * render, const ushort * indices, uint nr_indices)
{
	unsigned char* buffer = render->buffer;
	struct nv04_context* nv04 = render->nv04;
	int i;

	for( i=0; i< nr_indices-5; i+=6)
		nv04_2triangles(nv04,
				buffer,
				indices[i+0],
				indices[i+1],
				indices[i+2],
				indices[i+3],
				indices[i+4],
				indices[i+5]
			       );
	if (i != nr_indices)
	{
		nv04_1triangle(nv04,
				buffer,
				indices[i+0],
				indices[i+1],
				indices[i+2]
			       );
		i+=3;
	}
	if (i != nr_indices)
		NOUVEAU_ERR("Houston, we have lost some vertices\n");
}

static void nv04_vbuf_render_tri_strip_elts(struct nv04_vbuf_render* render, const ushort* indices, uint nr_indices)
{
	const uint32_t striptbl[]={0x321210,0x543432,0x765654,0x987876,0xBA9A98,0xDCBCBA,0xFEDEDC};
	unsigned char* buffer = render->buffer;
	struct nv04_context* nv04 = render->nv04;
	int i,j;

	for(i = 0; i<nr_indices; i+=14) 
	{
		int numvert = MIN2(16, nr_indices - i);
		int numtri = numvert - 2;
		if (numvert<3)
			break;

		BEGIN_RING( fahrenheit, NV04_DX5_TEXTURED_TRIANGLE_TLVERTEX_SX(0x0), numvert*8 );
		for(j = 0; j<numvert; j++)
			OUT_RINGp( buffer + VERTEX_SIZE * indices [i+j], 8 );

		BEGIN_RING_NI( fahrenheit, NV04_DX5_TEXTURED_TRIANGLE_TLVERTEX_DRAWPRIMITIVE(0), (numtri+1)/2 );
		for(j = 0; j<numtri/2; j++ )
			OUT_RING(striptbl[j]);
		if (numtri%2)
			OUT_RING(striptbl[numtri/2]&0xFFF);
	}
}

static void nv04_vbuf_render_tri_fan_elts(struct nv04_vbuf_render* render, const ushort* indices, uint nr_indices)
{
	const uint32_t fantbl[]={0x320210,0x540430,0x760650,0x980870,0xBA0A90,0xDC0CB0,0xFE0ED0};
	unsigned char* buffer = render->buffer;
	struct nv04_context* nv04 = render->nv04;
	int i,j;

	BEGIN_RING(fahrenheit, NV04_DX5_TEXTURED_TRIANGLE_TLVERTEX_SX(0x0), 8);
	OUT_RINGp(buffer + VERTEX_SIZE * indices[0], 8);

	for(i = 1; i<nr_indices; i+=14)
	{
		int numvert=MIN2(15, nr_indices - i);
		int numtri=numvert-2;
		if (numvert < 3)
			break;

		BEGIN_RING(fahrenheit, NV04_DX5_TEXTURED_TRIANGLE_TLVERTEX_SX(0x1), numvert*8);

		for(j=0;j<numvert;j++)
			OUT_RINGp( buffer + VERTEX_SIZE * indices[ i+j ], 8 );

		BEGIN_RING_NI(fahrenheit, NV04_DX5_TEXTURED_TRIANGLE_TLVERTEX_DRAWPRIMITIVE(0), (numtri+1)/2);
		for(j = 0; j<numtri/2; j++)
			OUT_RING(fantbl[j]);
		if (numtri%2)
			OUT_RING(fantbl[numtri/2]&0xFFF);
	}
}

static void nv04_vbuf_render_quads_elts(struct nv04_vbuf_render* render, const ushort* indices, uint nr_indices)
{
	unsigned char* buffer = render->buffer;
	struct nv04_context* nv04 = render->nv04;
	int i;

	for(i = 0; i < nr_indices; i += 4)
		nv04_1quad(nv04,
				buffer,
				indices[i+0],
				indices[i+1],
				indices[i+2],
				indices[i+3]
			       );
}


static void 
nv04_vbuf_render_draw( struct vbuf_render *render,
		const ushort *indices,
		uint nr_indices)
{
	struct nv04_vbuf_render *nv04_render = nv04_vbuf_render(render);

	// emit the indices
	switch( nv04_render->prim )
	{
		case PIPE_PRIM_TRIANGLES:
			nv04_vbuf_render_triangles_elts(nv04_render, indices, nr_indices);
			break;
		case PIPE_PRIM_QUAD_STRIP:
		case PIPE_PRIM_TRIANGLE_STRIP:
			nv04_vbuf_render_tri_strip_elts(nv04_render, indices, nr_indices);
			break;
		case PIPE_PRIM_TRIANGLE_FAN:
		case PIPE_PRIM_POLYGON:
			nv04_vbuf_render_tri_fan_elts(nv04_render, indices, nr_indices);
			break;
		case PIPE_PRIM_QUADS:
			nv04_vbuf_render_quads_elts(nv04_render, indices, nr_indices);
			break;
		default:
			NOUVEAU_ERR("You have to implement primitive %d, young padawan\n", nv04_render->prim);
			break;
	}
}


static void
nv04_vbuf_render_release_vertices( struct vbuf_render *render )
{
	struct nv04_vbuf_render *nv04_render = nv04_vbuf_render(render);

	free(nv04_render->buffer);
	nv04_render->buffer = NULL;
}


static void
nv04_vbuf_render_destroy( struct vbuf_render *render )
{
	struct nv04_vbuf_render *nv04_render = nv04_vbuf_render(render);
	FREE(nv04_render);
}


/**
 * Create a new primitive render.
 */
static struct vbuf_render *
nv04_vbuf_render_create( struct nv04_context *nv04 )
{
	struct nv04_vbuf_render *nv04_render = CALLOC_STRUCT(nv04_vbuf_render);

	nv04_render->nv04 = nv04;

	nv04_render->base.max_vertex_buffer_bytes = VERTEX_BUFFER_SIZE;
	nv04_render->base.max_indices = 65536; 
	nv04_render->base.get_vertex_info = nv04_vbuf_render_get_vertex_info;
	nv04_render->base.allocate_vertices = nv04_vbuf_render_allocate_vertices;
	nv04_render->base.map_vertices = nv04_vbuf_render_map_vertices;
	nv04_render->base.unmap_vertices = nv04_vbuf_render_unmap_vertices;
	nv04_render->base.set_primitive = nv04_vbuf_render_set_primitive;
	nv04_render->base.draw = nv04_vbuf_render_draw;
	nv04_render->base.release_vertices = nv04_vbuf_render_release_vertices;
	nv04_render->base.destroy = nv04_vbuf_render_destroy;

	return &nv04_render->base;
}


/**
 * Create a new primitive vbuf/render stage.
 */
struct draw_stage *nv04_draw_vbuf_stage( struct nv04_context *nv04 )
{
	struct vbuf_render *render;
	struct draw_stage *stage;

	render = nv04_vbuf_render_create(nv04);
	if(!render)
		return NULL;

	stage = draw_vbuf_stage( nv04->draw, render );
	if(!stage) {
		render->destroy(render);
		return NULL;
	}

	return stage;
}
