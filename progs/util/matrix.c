/*
 * matrix.c
 *
 * Some useful matrix functions.
 *
 * Brian Paul
 * 10 Feb 2004
 */



#include <stdio.h>
#include <stdlib.h>
#include <math.h>


/**
 * Pretty-print the given matrix.
 */
void
PrintMatrix(const float p[16])
{
   printf("[ %6.3f %6.3f %6.3f %6.3f ]\n", p[0], p[4], p[8], p[12]);
   printf("[ %6.3f %6.3f %6.3f %6.3f ]\n", p[1], p[5], p[9], p[13]);
   printf("[ %6.3f %6.3f %6.3f %6.3f ]\n", p[2], p[6], p[10], p[14]);
   printf("[ %6.3f %6.3f %6.3f %6.3f ]\n", p[3], p[7], p[11], p[15]);
}


/**
 * Build a glFrustum matrix.
 */
void
Frustum(float left, float right, float bottom, float top, float nearZ, float farZ, float *m)
{
   float x = (2.0F*nearZ) / (right-left);
   float y = (2.0F*nearZ) / (top-bottom);
   float a = (right+left) / (right-left);
   float b = (top+bottom) / (top-bottom);
   float c = -(farZ+nearZ) / ( farZ-nearZ);
   float d = -(2.0F*farZ*nearZ) / (farZ-nearZ);

#define M(row,col)  m[col*4+row]
   M(0,0) = x;     M(0,1) = 0.0F;  M(0,2) = a;      M(0,3) = 0.0F;
   M(1,0) = 0.0F;  M(1,1) = y;     M(1,2) = b;      M(1,3) = 0.0F;
   M(2,0) = 0.0F;  M(2,1) = 0.0F;  M(2,2) = c;      M(2,3) = d;
   M(3,0) = 0.0F;  M(3,1) = 0.0F;  M(3,2) = -1.0F;  M(3,3) = 0.0F;
#undef M
}


/**
 * Build a glOrtho marix.
 */
void
Ortho(float left, float right, float bottom, float top, float nearZ, float farZ, float *m)
{
#define M(row,col)  m[col*4+row]
   M(0,0) = 2.0F / (right-left);
   M(0,1) = 0.0F;
   M(0,2) = 0.0F;
   M(0,3) = -(right+left) / (right-left);

   M(1,0) = 0.0F;
   M(1,1) = 2.0F / (top-bottom);
   M(1,2) = 0.0F;
   M(1,3) = -(top+bottom) / (top-bottom);

   M(2,0) = 0.0F;
   M(2,1) = 0.0F;
   M(2,2) = -2.0F / (farZ-nearZ);
   M(2,3) = -(farZ+nearZ) / (farZ-nearZ);

   M(3,0) = 0.0F;
   M(3,1) = 0.0F;
   M(3,2) = 0.0F;
   M(3,3) = 1.0F;
#undef M
}


/**
 * Decompose a projection matrix to determine original glFrustum or
 * glOrtho parameters.
 */
void
DecomposeProjection( const float *m,
                     int *isPerspective,
                     float *leftOut, float *rightOut,
                     float *botOut, float *topOut,
                     float *nearOut, float *farOut)
{
   if (m[15] == 0.0) {
      /* perspective */
      float p[16];
      const float x = m[0];  /* 2N / (R-L) */
      const float y = m[5];  /* 2N / (T-B) */
      const float a = m[8];  /* (R+L) / (R-L) */
      const float b = m[9];  /* (T+B) / (T-B) */
      const float c = m[10]; /* -(F+N) / (F-N) */
      const float d = m[14]; /* -2FN / (F-N) */

      /* These equations found with simple algebra, knowing the arithmetic
       * use to set up a typical perspective projection matrix in OpenGL.
       */
      const float nearZ = -d / (1.0 - c);
      const float farZ = (c - 1.0) * nearZ / (c + 1.0);
      const float left = nearZ * (a - 1.0) / x;
      const float right = 2.0 * nearZ / x + left;
      const float bottom = nearZ * (b - 1.0) / y;
      const float top = 2.0 * nearZ / y + bottom;

      *isPerspective = 1;
      *leftOut = left;
      *rightOut = right;
      *botOut = bottom;
      *topOut = top;
      *nearOut = nearZ;
      *farOut = farZ;
   }
   else {
      /* orthographic */
      const float x = m[0];  /*  2 / (R-L) */
      const float y = m[5];  /*  2 / (T-B) */
      const float z = m[10]; /* -2 / (F-N) */
      const float a = m[12]; /* -(R+L) / (R-L) */
      const float b = m[13]; /* -(T+B) / (T-B) */
      const float c = m[14]; /* -(F+N) / (F-N) */
      /* again, simple algebra */
      const float right  = -(a - 1.0) / x;
      const float left   = right - 2.0 / x;
      const float top    = -(b - 1.0) / y;
      const float bottom = top - 2.0 / y;
      const float farZ   = (c - 1.0) / z;
      const float nearZ  = farZ + 2.0 / z;

      *isPerspective = 0;
      *leftOut = left;
      *rightOut = right;
      *botOut = bottom;
      *topOut = top;
      *nearOut = nearZ;
      *farOut = farZ;
   }
}


#if 0
/* test harness */
int
main(int argc, char *argv[])
{
   float m[16], p[16];
   float l, r, b, t, n, f;
   int persp;
   int i;

#if 0
   l = -.9;
   r = 1.2;
   b = -0.5;
   t = 1.4;
   n = 30;
   f = 84;
   printf("  Frustum(%f, %f, %f, %f, %f, %f\n",l+1, r+1.2, b+.5, t+.3, n, f);
   Frustum(l+1, r+1.2, b+.5, t+.3, n, f, p);
   DecomposeProjection(p, &persp, &l, &r, &b, &t, &n, &f);
   printf("glFrustum(%f, %f, %f, %f, %f, %f)\n",
          l, r, b, t, n, f);
   PrintMatrix(p);
#else
   printf("Ortho(-1, 1, -1, 1, 10, 84)\n");
   Ortho(-1, 1, -1, 1, 10, 84, m);
   PrintMatrix(m);
   DecomposeProjection(m, &persp, &l, &r, &b, &t, &n, &f);
   printf("Ortho(%f, %f, %f, %f, %f, %f) %d\n", l, r, b, t, n, f, persp);
#endif

   return 0;
}
#endif
