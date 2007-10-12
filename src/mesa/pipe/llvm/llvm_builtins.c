

inline float4 compute_clip(float4 vec, float4 scale, float4 trans)
{
   return vec*scale + trans;
}


inline float
dot4(const float4 a, const float4 b)
{
   float4 c = a*b;
   return c.x + c.y + c.z + c.w;
}

inline unsigned
compute_clipmask(float4 clip, const float4 (*plane), unsigned nr)
{
   unsigned mask = 0;
   unsigned i;

   for (i = 0; i < nr; i++) {
      if (dot4(clip, plane[i]) < 0)
         mask |= (1<<i);
   }

   return mask;
}

inline void collect_results(float4 *results, struct vertex_header *vOut,
                            float4 *planes, int nr_planes,
                            float4 scale, float4 trans)
{
   /* store results */
   unsigned slot;
   float x, y, z, w;

   /* Handle attr[0] (position) specially:
    */
   float4 res0 = results[0];
   x = vOut->clip[0] = clip.x;
   y = vOut->clip[1] = clip.y;
   z = vOut->clip[2] = clip.z;
   w = vOut->clip[3] = clip.w;

   vOut[i]->clipmask = compute_clipmask(res0, planes, nr_planes);
   vOut[i]->edgeflag = 1;

   /* divide by w */
   w = 1.0f / w;
   x *= w;
   y *= w;
   z *= w;
   res0.x = x; res0.y = y; res0.z = z; res0.w = 1;

   /* Viewport mapping */
   res = res * scale + trans;
   vOut->data[0][0] = res.x;
   vOut->data[0][1] = res.y;
   vOut->data[0][2] = res.z;
   vOut->data[0][3] = w;

   /* Remaining attributes are packed into sequential post-transform
    * vertex attrib slots.
    * Skip 0 since we just did it above.
    * Subtract two because of the VERTEX_HEADER, CLIP_POS attribs.
    */
   for (slot = 1; slot < draw->vertex_info.num_attribs - 2; slot++) {
      float4 vec = results[slot];
      vOut->data[slot][0] = vec.x;
      vOut->data[slot][1] = vec.y;
      vOut->data[slot][2] = vec.z;
      vOut->data[slot][3] = vec.w;

      printf("output %d: %f %f %f %f\n", slot,
             vOut->data[slot][0],
             vOut->data[slot][1],
             vOut->data[slot][2],
             vOut->data[slot][3]);
   }
}

void run_vertex_shader(float ainputs[VS_QUEUE_LENGTH][PIPE_MAX_SHADER_INPUTS][4],
                       struct vertex_header *dests[VS_QUEUE_LENGTH],
                       float *aconsts[4]
                       int count)
{
   float4  inputs[VS_QUEUE_LENGTH][PIPE_MAX_SHADER_INPUTS];
   float4 *consts;
}
