#ifndef vl_csc_h
#define vl_csc_h

#include <pipe/p_compiler.h>

struct vl_procamp
{
   float brightness;
   float contrast;
   float saturation;
   float hue;
};

enum VL_CSC_COLOR_STANDARD
{
   VL_CSC_COLOR_STANDARD_IDENTITY,
   VL_CSC_COLOR_STANDARD_BT_601,
   VL_CSC_COLOR_STANDARD_BT_709
};

void vl_csc_get_matrix(enum VL_CSC_COLOR_STANDARD cs,
                       struct vl_procamp *procamp,
                       bool full_range,
                       float *matrix);

#endif /* vl_csc_h */
