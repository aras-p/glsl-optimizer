
static void copy_pv_rgba4_spec5( GLcontext *ctx, GLuint edst, GLuint esrc )
{
   i810ContextPtr imesa = I810_CONTEXT( ctx );
   GLubyte *i810verts = (GLubyte *)imesa->verts;
   GLuint shift = imesa->vertex_stride_shift;
   i810Vertex *dst = (i810Vertex *)(i810verts + (edst << shift));
   i810Vertex *src = (i810Vertex *)(i810verts + (esrc << shift));
   dst->ui[4] = src->ui[4];
   dst->ui[5] = src->ui[5];
}

static void copy_pv_rgba4( GLcontext *ctx, GLuint edst, GLuint esrc )
{
   i810ContextPtr imesa = I810_CONTEXT( ctx );
   GLubyte *i810verts = (GLubyte *)imesa->verts;
   GLuint shift = imesa->vertex_stride_shift;
   i810Vertex *dst = (i810Vertex *)(i810verts + (edst << shift));
   i810Vertex *src = (i810Vertex *)(i810verts + (esrc << shift));
   dst->ui[4] = src->ui[4];
}

static void copy_pv_rgba3( GLcontext *ctx, GLuint edst, GLuint esrc )
{
   i810ContextPtr imesa = I810_CONTEXT( ctx );
   GLubyte *i810verts = (GLubyte *)imesa->verts;
   GLuint shift = imesa->vertex_stride_shift;
   i810Vertex *dst = (i810Vertex *)(i810verts + (edst << shift));
   i810Vertex *src = (i810Vertex *)(i810verts + (esrc << shift));
   dst->ui[3] = src->ui[3];
}

