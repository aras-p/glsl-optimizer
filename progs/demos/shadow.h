#ifndef SHADOW_H
#define SHADOW_H


extern void
shadowmatrix(GLfloat shadowMat[4][4],
             GLfloat groundplane[4],
             GLfloat lightpos[4]);


extern void
findplane(GLfloat plane[4],
          GLfloat v0[3], GLfloat v1[3], GLfloat v2[3]);


#endif


