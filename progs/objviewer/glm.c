/*    
 *  GLM library.  Wavefront .obj file format reader/writer/manipulator.
 *
 *  Written by Nate Robins, 1997.
 *  email: ndr@pobox.com
 *  www: http://www.pobox.com/~ndr
 */

/* includes */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "glm.h"
#include "readtex.h"


typedef unsigned char boolean;
#define TRUE 1
#define FALSE 0


/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* defines */
#define T(x) model->triangles[(x)]


/* enums */
enum { X, Y, Z, W };			/* elements of a vertex */


/* typedefs */

/* _GLMnode: general purpose node
 */
typedef struct _GLMnode {
  uint           index;
  boolean        averaged;
  struct _GLMnode* next;
} GLMnode;

/* strdup is actually not a standard ANSI C or POSIX routine
   so implement a private one.  OpenVMS does not have a strdup; Linux's
   standard libc doesn't declare strdup by default (unless BSD or SVID
   interfaces are requested). */
static char *
stralloc(const char *string)
{
  char *copy;

  copy = malloc(strlen(string) + 1);
  if (copy == NULL)
    return NULL;
  strcpy(copy, string);
  return copy;
}

/* private functions */

/* _glmMax: returns the maximum of two floats */
static float
_glmMax(float a, float b) 
{
  if (a > b)
    return a;
  return b;
}

/* _glmAbs: returns the absolute value of a float */
static float
_glmAbs(float f)
{
  if (f < 0)
    return -f;
  return f;
}

/* _glmDot: compute the dot product of two vectors
 *
 * u - array of 3 floats (float u[3])
 * v - array of 3 floats (float v[3])
 */
static float
_glmDot(float* u, float* v)
{
  assert(u);
  assert(v);

  /* compute the dot product */
  return u[X] * v[X] + u[Y] * v[Y] + u[Z] * v[Z];
}

/* _glmCross: compute the cross product of two vectors
 *
 * u - array of 3 floats (float u[3])
 * v - array of 3 floats (float v[3])
 * n - array of 3 floats (float n[3]) to return the cross product in
 */
static void
_glmCross(float* u, float* v, float* n)
{
  assert(u);
  assert(v);
  assert(n);

  /* compute the cross product (u x v for right-handed [ccw]) */
  n[X] = u[Y] * v[Z] - u[Z] * v[Y];
  n[Y] = u[Z] * v[X] - u[X] * v[Z];
  n[Z] = u[X] * v[Y] - u[Y] * v[X];
}

/* _glmNormalize: normalize a vector
 *
 * n - array of 3 floats (float n[3]) to be normalized
 */
static void
_glmNormalize(float* n)
{
  float l;

  assert(n);

  /* normalize */
  l = (float)sqrt(n[X] * n[X] + n[Y] * n[Y] + n[Z] * n[Z]);
  n[0] /= l;
  n[1] /= l;
  n[2] /= l;
}

/* _glmEqual: compares two vectors and returns TRUE if they are
 * equal (within a certain threshold) or FALSE if not. An epsilon
 * that works fairly well is 0.000001.
 *
 * u - array of 3 floats (float u[3])
 * v - array of 3 floats (float v[3]) 
 */
static boolean
_glmEqual(float* u, float* v, float epsilon)
{
  if (_glmAbs(u[0] - v[0]) < epsilon &&
      _glmAbs(u[1] - v[1]) < epsilon &&
      _glmAbs(u[2] - v[2]) < epsilon) 
  {
    return TRUE;
  }
  return FALSE;
}

/* _glmWeldVectors: eliminate (weld) vectors that are within an
 * epsilon of each other.
 *
 * vectors    - array of float[3]'s to be welded
 * numvectors - number of float[3]'s in vectors
 * epsilon    - maximum difference between vectors 
 *
 */
static float*
_glmWeldVectors(float* vectors, uint* numvectors, float epsilon)
{
  float* copies;
  uint   copied;
  uint   i, j;

  copies = (float*)malloc(sizeof(float) * 3 * (*numvectors + 1));
  memcpy(copies, vectors, (sizeof(float) * 3 * (*numvectors + 1)));

  copied = 1;
  for (i = 1; i <= *numvectors; i++) {
    for (j = 1; j <= copied; j++) {
      if (_glmEqual(&vectors[3 * i], &copies[3 * j], epsilon)) {
	goto duplicate;
      }
    }

    /* must not be any duplicates -- add to the copies array */
    copies[3 * copied + 0] = vectors[3 * i + 0];
    copies[3 * copied + 1] = vectors[3 * i + 1];
    copies[3 * copied + 2] = vectors[3 * i + 2];
    j = copied;				/* pass this along for below */
    copied++;

  duplicate:
    /* set the first component of this vector to point at the correct
       index into the new copies array */
    vectors[3 * i + 0] = (float)j;
  }

  *numvectors = copied-1;
  return copies;
}

/* _glmFindGroup: Find a group in the model
 */
static GLMgroup*
_glmFindGroup(GLMmodel* model, char* name)
{
  GLMgroup* group;

  assert(model);

  group = model->groups;
  while(group) {
    if (!strcmp(name, group->name))
      break;
    group = group->next;
  }

  return group;
}

/* _glmAddGroup: Add a group to the model
 */
static GLMgroup*
_glmAddGroup(GLMmodel* model, char* name)
{
  GLMgroup* group;

  group = _glmFindGroup(model, name);
  if (!group) {
    group = (GLMgroup*)malloc(sizeof(GLMgroup));
    group->name = stralloc(name);
    group->material = 0;
    group->numtriangles = 0;
    group->triangles = NULL;
    group->next = model->groups;
    model->groups = group;
    model->numgroups++;
  }

  return group;
}

/* _glmFindGroup: Find a material in the model
 */
static uint
_glmFindMaterial(GLMmodel* model, char* name)
{
  uint i;

  for (i = 0; i < model->nummaterials; i++) {
    if (!strcmp(model->materials[i].name, name))
      goto found;
  }

  /* didn't find the name, so set it as the default material */
  printf("_glmFindMaterial():  can't find material \"%s\".\n", name);
  i = 0;

found:
  return i;
}


/* _glmDirName: return the directory given a path
 *
 * path - filesystem path
 *
 * The return value should be free'd.
 */
static char*
_glmDirName(char* path)
{
  char* dir;
  char* s;

  dir = stralloc(path);

  s = strrchr(dir, '/');
  if (s)
    s[1] = '\0';
  else
    dir[0] = '\0';

  return dir;
}


/* _glmReadMTL: read a wavefront material library file
 *
 * model - properly initialized GLMmodel structure
 * name  - name of the material library
 */
static void
_glmReadMTL(GLMmodel* model, char* name)
{
  FILE* file;
  char* dir;
  char* filename;
  char  buf[128], buf2[128];
  uint nummaterials, i;
  GLMmaterial *mat;

  dir = _glmDirName(model->pathname);
  filename = (char*)malloc(sizeof(char) * (strlen(dir) + strlen(name) + 1));
  strcpy(filename, dir);
  strcat(filename, name);
  free(dir);

  /* open the file */
  file = fopen(filename, "r");
  if (!file) {
    fprintf(stderr, "_glmReadMTL() failed: can't open material file \"%s\".\n",
	    filename);
    exit(1);
  }
  free(filename);

  /* count the number of materials in the file */
  nummaterials = 1;
  while(fscanf(file, "%s", buf) != EOF) {
    switch(buf[0]) {
    case '#':				/* comment */
      /* eat up rest of line */
      fgets(buf, sizeof(buf), file);
      break;
    case 'n':				/* newmtl */
      fgets(buf, sizeof(buf), file);
      nummaterials++;
      sscanf(buf, "%s %s", buf, buf);
      break;
    default:
      /* eat up rest of line */
      fgets(buf, sizeof(buf), file);
      break;
    }
  }

  rewind(file);

  /* allocate memory for the materials */
  model->materials = (GLMmaterial*)calloc(nummaterials, sizeof(GLMmaterial));
  model->nummaterials = nummaterials;

  /* set the default material */
  for (i = 0; i < nummaterials; i++) {
    model->materials[i].name = NULL;
    model->materials[i].shininess = 0;
    model->materials[i].diffuse[0] = 0.8;
    model->materials[i].diffuse[1] = 0.8;
    model->materials[i].diffuse[2] = 0.8;
    model->materials[i].diffuse[3] = 1.0;
    model->materials[i].ambient[0] = 0.2;
    model->materials[i].ambient[1] = 0.2;
    model->materials[i].ambient[2] = 0.2;
    model->materials[i].ambient[3] = 0.0;
    model->materials[i].specular[0] = 0.0;
    model->materials[i].specular[1] = 0.0;
    model->materials[i].specular[2] = 0.0;
    model->materials[i].specular[3] = 0.0;
  }
  model->materials[0].name = stralloc("default");

  /* now, read in the data */
  nummaterials = 0;

  mat = &model->materials[nummaterials];

  while(fscanf(file, "%s", buf) != EOF) {
    switch(buf[0]) {
    case '#':				/* comment */
      /* eat up rest of line */
      fgets(buf, sizeof(buf), file);
      break;
    case 'n':				/* newmtl */
      fgets(buf, sizeof(buf), file);
      sscanf(buf, "%s %s", buf, buf);
      nummaterials++;
      model->materials[nummaterials].name = stralloc(buf);
      break;
    case 'N':
      fscanf(file, "%f", &model->materials[nummaterials].shininess);
      /* wavefront shininess is from [0, 1000], so scale for OpenGL */
      model->materials[nummaterials].shininess /= 1000.0;
      model->materials[nummaterials].shininess *= 128.0;
      mat = &model->materials[nummaterials];
      break;
    case 'K':
      switch(buf[1]) {
      case 'd':
	fscanf(file, "%f %f %f",
	       &model->materials[nummaterials].diffuse[0],
	       &model->materials[nummaterials].diffuse[1],
	       &model->materials[nummaterials].diffuse[2]);
	break;
      case 's':
	fscanf(file, "%f %f %f",
	       &model->materials[nummaterials].specular[0],
	       &model->materials[nummaterials].specular[1],
	       &model->materials[nummaterials].specular[2]);
	break;
      case 'a':
	fscanf(file, "%f %f %f",
	       &model->materials[nummaterials].ambient[0],
	       &model->materials[nummaterials].ambient[1],
	       &model->materials[nummaterials].ambient[2]);
	break;
      default:
	/* eat up rest of line */
	fgets(buf, sizeof(buf), file);
	break;
      }
      break;
    case 'd':  /* alpha? */
	fscanf(file, "%f",
	       &model->materials[nummaterials].diffuse[3]);
        break;
    case 'm':  /* texture map */
       fscanf(file, "%s", buf2);
       /*printf("map %s\n", buf2);*/
       mat->map_kd = strdup(buf2);
       break;

    default:
      /* eat up rest of line */
      fgets(buf, sizeof(buf), file);
      break;
    }
  }
  fclose(file);
}


/* _glmWriteMTL: write a wavefront material library file
 *
 * model      - properly initialized GLMmodel structure
 * modelpath  - pathname of the model being written
 * mtllibname - name of the material library to be written
 */
static void
_glmWriteMTL(GLMmodel* model, char* modelpath, char* mtllibname)
{
  FILE* file;
  char* dir;
  char* filename;
  GLMmaterial* material;
  uint i;

  dir = _glmDirName(modelpath);
  filename = (char*)malloc(sizeof(char) * (strlen(dir) + strlen(mtllibname)));
  strcpy(filename, dir);
  strcat(filename, mtllibname);
  free(dir);

  /* open the file */
  file = fopen(filename, "w");
  if (!file) {
    fprintf(stderr, "_glmWriteMTL() failed: can't open file \"%s\".\n",
	    filename);
    exit(1);
  }
  free(filename);

  /* spit out a header */
  fprintf(file, "#  \n");
  fprintf(file, "#  Wavefront MTL generated by GLM library\n");
  fprintf(file, "#  \n");
  fprintf(file, "#  GLM library copyright (C) 1997 by Nate Robins\n");
  fprintf(file, "#  email: ndr@pobox.com\n");
  fprintf(file, "#  www:   http://www.pobox.com/~ndr\n");
  fprintf(file, "#  \n\n");

  for (i = 0; i < model->nummaterials; i++) {
    material = &model->materials[i];
    fprintf(file, "newmtl %s\n", material->name);
    fprintf(file, "Ka %f %f %f\n", 
	    material->ambient[0], material->ambient[1], material->ambient[2]);
    fprintf(file, "Kd %f %f %f\n", 
	    material->diffuse[0], material->diffuse[1], material->diffuse[2]);
    fprintf(file, "Ks %f %f %f\n", 
	    material->specular[0],material->specular[1],material->specular[2]);
    fprintf(file, "Ns %f\n", material->shininess);
    fprintf(file, "\n");
  }
  fclose(file);
}


/* _glmFirstPass: first pass at a Wavefront OBJ file that gets all the
 * statistics of the model (such as #vertices, #normals, etc)
 *
 * model - properly initialized GLMmodel structure
 * file  - (fopen'd) file descriptor 
 */
static void
_glmFirstPass(GLMmodel* model, FILE* file) 
{
  uint    numvertices;		/* number of vertices in model */
  uint    numnormals;			/* number of normals in model */
  uint    numtexcoords;		/* number of texcoords in model */
  uint    numtriangles;		/* number of triangles in model */
  GLMgroup* group;			/* current group */
  unsigned  v, n, t;
  char      buf[128];

  /* make a default group */
  group = _glmAddGroup(model, "default");

  numvertices = numnormals = numtexcoords = numtriangles = 0;
  while(fscanf(file, "%s", buf) != EOF) {
    switch(buf[0]) {
    case '#':				/* comment */
      /* eat up rest of line */
      fgets(buf, sizeof(buf), file);
      break;
    case 'v':				/* v, vn, vt */
      switch(buf[1]) {
      case '\0':			/* vertex */
	/* eat up rest of line */
	fgets(buf, sizeof(buf), file);
	numvertices++;
	break;
      case 'n':				/* normal */
	/* eat up rest of line */
	fgets(buf, sizeof(buf), file);
	numnormals++;
	break;
      case 't':				/* texcoord */
	/* eat up rest of line */
	fgets(buf, sizeof(buf), file);
	numtexcoords++;
	break;
      default:
	printf("_glmFirstPass(): Unknown token \"%s\".\n", buf);
	exit(1);
	break;
      }
      break;
    case 'm':
      fgets(buf, sizeof(buf), file);
      sscanf(buf, "%s %s", buf, buf);
      model->mtllibname = stralloc(buf);
      _glmReadMTL(model, buf);
      break;
    case 'u':
      /* eat up rest of line */
      fgets(buf, sizeof(buf), file);
      break;
    case 'g':				/* group */
      /* eat up rest of line */
      fgets(buf, sizeof(buf), file);
      sscanf(buf, "%s", buf);
      group = _glmAddGroup(model, buf);
      break;
    case 'f':				/* face */
      v = n = t = 0;
      fscanf(file, "%s", buf);
      /* can be one of %d, %d//%d, %d/%d, %d/%d/%d %d//%d */
      if (strstr(buf, "//")) {
	/* v//n */
	sscanf(buf, "%u//%u", &v, &n);
	fscanf(file, "%u//%u", &v, &n);
	fscanf(file, "%u//%u", &v, &n);
	numtriangles++;
	group->numtriangles++;
	while(fscanf(file, "%u//%u", &v, &n) > 0) {
	  numtriangles++;
	  group->numtriangles++;
	}
      } else if (sscanf(buf, "%u/%u/%u", &v, &t, &n) == 3) {
	/* v/t/n */
	fscanf(file, "%u/%u/%u", &v, &t, &n);
	fscanf(file, "%u/%u/%u", &v, &t, &n);
	numtriangles++;
	group->numtriangles++;
	while(fscanf(file, "%u/%u/%u", &v, &t, &n) > 0) {
	  numtriangles++;
	  group->numtriangles++;
	}
      } else if (sscanf(buf, "%u/%u", &v, &t) == 2) {
	/* v/t */
	fscanf(file, "%u/%u", &v, &t);
	fscanf(file, "%u/%u", &v, &t);
	numtriangles++;
	group->numtriangles++;
	while(fscanf(file, "%u/%u", &v, &t) > 0) {
	  numtriangles++;
	  group->numtriangles++;
	}
      } else {
	/* v */
	fscanf(file, "%u", &v);
	fscanf(file, "%u", &v);
	numtriangles++;
	group->numtriangles++;
	while(fscanf(file, "%u", &v) > 0) {
	  numtriangles++;
	  group->numtriangles++;
	}
      }
      break;

    default:
      /* eat up rest of line */
      fgets(buf, sizeof(buf), file);
      break;
    }
  }

#if 0
  /* announce the model statistics */
  printf(" Vertices: %d\n", numvertices);
  printf(" Normals: %d\n", numnormals);
  printf(" Texcoords: %d\n", numtexcoords);
  printf(" Triangles: %d\n", numtriangles);
  printf(" Groups: %d\n", model->numgroups);
#endif

  /* set the stats in the model structure */
  model->numvertices  = numvertices;
  model->numnormals   = numnormals;
  model->numtexcoords = numtexcoords;
  model->numtriangles = numtriangles;

  /* allocate memory for the triangles in each group */
  group = model->groups;
  while(group) {
    group->triangles = (uint*)malloc(sizeof(uint) * group->numtriangles);
    group->numtriangles = 0;
    group = group->next;
  }
}

/* _glmSecondPass: second pass at a Wavefront OBJ file that gets all
 * the data.
 *
 * model - properly initialized GLMmodel structure
 * file  - (fopen'd) file descriptor 
 */
static void
_glmSecondPass(GLMmodel* model, FILE* file) 
{
  uint    numvertices;		/* number of vertices in model */
  uint    numnormals;			/* number of normals in model */
  uint    numtexcoords;		/* number of texcoords in model */
  uint    numtriangles;		/* number of triangles in model */
  float*  vertices;			/* array of vertices  */
  float*  normals;			/* array of normals */
  float*  texcoords;			/* array of texture coordinates */
  GLMgroup* group;			/* current group pointer */
  uint    material;			/* current material */
  uint    v, n, t;
  char      buf[128];

  /* set the pointer shortcuts */
  vertices     = model->vertices;
  normals      = model->normals;
  texcoords    = model->texcoords;
  group        = model->groups;

  /* on the second pass through the file, read all the data into the
     allocated arrays */
  numvertices = numnormals = numtexcoords = 1;
  numtriangles = 0;
  material = 0;
  while(fscanf(file, "%s", buf) != EOF) {
    switch(buf[0]) {
    case '#':				/* comment */
      /* eat up rest of line */
      fgets(buf, sizeof(buf), file);
      break;
    case 'v':				/* v, vn, vt */
      switch(buf[1]) {
      case '\0':			/* vertex */
	fscanf(file, "%f %f %f", 
	       &vertices[3 * numvertices + X], 
	       &vertices[3 * numvertices + Y], 
	       &vertices[3 * numvertices + Z]);
	numvertices++;
	break;
      case 'n':				/* normal */
	fscanf(file, "%f %f %f", 
	       &normals[3 * numnormals + X],
	       &normals[3 * numnormals + Y], 
	       &normals[3 * numnormals + Z]);
	numnormals++;
	break;
      case 't':				/* texcoord */
	fscanf(file, "%f %f", 
	       &texcoords[2 * numtexcoords + X],
	       &texcoords[2 * numtexcoords + Y]);
	numtexcoords++;
	break;
      }
      break;
    case 'u':
      fgets(buf, sizeof(buf), file);
      sscanf(buf, "%s %s", buf, buf);
      material = _glmFindMaterial(model, buf);
      if (!group->material)
         group->material = material;
      /*printf("material %s = %u\n", buf, material);*/
      break;
    case 'g':				/* group */
      /* eat up rest of line */
      fgets(buf, sizeof(buf), file);
      sscanf(buf, "%s", buf);
      group = _glmFindGroup(model, buf);
      group->material = material;
      /*printf("GROUP %s  material %u\n", buf, material);*/
      break;
    case 'f':				/* face */
      v = n = t = 0;
      fscanf(file, "%s", buf);
      /* can be one of %d, %d//%d, %d/%d, %d/%d/%d %d//%d */
      if (strstr(buf, "//")) {
	/* v//n */
	sscanf(buf, "%u//%u", &v, &n);
	T(numtriangles).vindices[0] = v;
	T(numtriangles).nindices[0] = n;
	fscanf(file, "%u//%u", &v, &n);
	T(numtriangles).vindices[1] = v;
	T(numtriangles).nindices[1] = n;
	fscanf(file, "%u//%u", &v, &n);
	T(numtriangles).vindices[2] = v;
	T(numtriangles).nindices[2] = n;
	group->triangles[group->numtriangles++] = numtriangles;
	numtriangles++;
	while(fscanf(file, "%u//%u", &v, &n) > 0) {
	  T(numtriangles).vindices[0] = T(numtriangles-1).vindices[0];
	  T(numtriangles).nindices[0] = T(numtriangles-1).nindices[0];
	  T(numtriangles).vindices[1] = T(numtriangles-1).vindices[2];
	  T(numtriangles).nindices[1] = T(numtriangles-1).nindices[2];
	  T(numtriangles).vindices[2] = v;
	  T(numtriangles).nindices[2] = n;
	  group->triangles[group->numtriangles++] = numtriangles;
	  numtriangles++;
	}
      } else if (sscanf(buf, "%u/%u/%u", &v, &t, &n) == 3) {
	/* v/t/n */
	T(numtriangles).vindices[0] = v;
	T(numtriangles).tindices[0] = t;
	T(numtriangles).nindices[0] = n;
	fscanf(file, "%u/%u/%u", &v, &t, &n);
	T(numtriangles).vindices[1] = v;
	T(numtriangles).tindices[1] = t;
	T(numtriangles).nindices[1] = n;
	fscanf(file, "%u/%u/%u", &v, &t, &n);
	T(numtriangles).vindices[2] = v;
	T(numtriangles).tindices[2] = t;
	T(numtriangles).nindices[2] = n;
	group->triangles[group->numtriangles++] = numtriangles;
	numtriangles++;
	while(fscanf(file, "%u/%u/%u", &v, &t, &n) > 0) {
	  T(numtriangles).vindices[0] = T(numtriangles-1).vindices[0];
	  T(numtriangles).tindices[0] = T(numtriangles-1).tindices[0];
	  T(numtriangles).nindices[0] = T(numtriangles-1).nindices[0];
	  T(numtriangles).vindices[1] = T(numtriangles-1).vindices[2];
	  T(numtriangles).tindices[1] = T(numtriangles-1).tindices[2];
	  T(numtriangles).nindices[1] = T(numtriangles-1).nindices[2];
	  T(numtriangles).vindices[2] = v;
	  T(numtriangles).tindices[2] = t;
	  T(numtriangles).nindices[2] = n;
	  group->triangles[group->numtriangles++] = numtriangles;
	  numtriangles++;
	}
      } else if (sscanf(buf, "%u/%u", &v, &t) == 2) {
	/* v/t */
	T(numtriangles).vindices[0] = v;
	T(numtriangles).tindices[0] = t;
	fscanf(file, "%u/%u", &v, &t);
	T(numtriangles).vindices[1] = v;
	T(numtriangles).tindices[1] = t;
	fscanf(file, "%u/%u", &v, &t);
	T(numtriangles).vindices[2] = v;
	T(numtriangles).tindices[2] = t;
	group->triangles[group->numtriangles++] = numtriangles;
	numtriangles++;
	while(fscanf(file, "%u/%u", &v, &t) > 0) {
	  T(numtriangles).vindices[0] = T(numtriangles-1).vindices[0];
	  T(numtriangles).tindices[0] = T(numtriangles-1).tindices[0];
	  T(numtriangles).vindices[1] = T(numtriangles-1).vindices[2];
	  T(numtriangles).tindices[1] = T(numtriangles-1).tindices[2];
	  T(numtriangles).vindices[2] = v;
	  T(numtriangles).tindices[2] = t;
	  group->triangles[group->numtriangles++] = numtriangles;
	  numtriangles++;
	}
      } else {
	/* v */
	sscanf(buf, "%u", &v);
	T(numtriangles).vindices[0] = v;
	fscanf(file, "%u", &v);
	T(numtriangles).vindices[1] = v;
	fscanf(file, "%u", &v);
	T(numtriangles).vindices[2] = v;
	group->triangles[group->numtriangles++] = numtriangles;
	numtriangles++;
	while(fscanf(file, "%u", &v) > 0) {
	  T(numtriangles).vindices[0] = T(numtriangles-1).vindices[0];
	  T(numtriangles).vindices[1] = T(numtriangles-1).vindices[2];
	  T(numtriangles).vindices[2] = v;
	  group->triangles[group->numtriangles++] = numtriangles;
	  numtriangles++;
	}
      }
      break;

    default:
      /* eat up rest of line */
      fgets(buf, sizeof(buf), file);
      break;
    }
  }

#if 0
  /* announce the memory requirements */
  printf(" Memory: %d bytes\n",
	 numvertices  * 3*sizeof(float) +
	 numnormals   * 3*sizeof(float) * (numnormals ? 1 : 0) +
	 numtexcoords * 3*sizeof(float) * (numtexcoords ? 1 : 0) +
	 numtriangles * sizeof(GLMtriangle));
#endif
}




/* public functions */

/* glmUnitize: "unitize" a model by translating it to the origin and
 * scaling it to fit in a unit cube around the origin.  Returns the
 * scalefactor used.
 *
 * model - properly initialized GLMmodel structure 
 */
float
glmUnitize(GLMmodel* model)
{
  uint  i;
  float maxx, minx, maxy, miny, maxz, minz;
  float cx, cy, cz, w, h, d;
  float scale;

  assert(model);
  assert(model->vertices);

  /* get the max/mins */
  maxx = minx = model->vertices[3 + X];
  maxy = miny = model->vertices[3 + Y];
  maxz = minz = model->vertices[3 + Z];
  for (i = 1; i <= model->numvertices; i++) {
    if (maxx < model->vertices[3 * i + X])
      maxx = model->vertices[3 * i + X];
    if (minx > model->vertices[3 * i + X])
      minx = model->vertices[3 * i + X];

    if (maxy < model->vertices[3 * i + Y])
      maxy = model->vertices[3 * i + Y];
    if (miny > model->vertices[3 * i + Y])
      miny = model->vertices[3 * i + Y];

    if (maxz < model->vertices[3 * i + Z])
      maxz = model->vertices[3 * i + Z];
    if (minz > model->vertices[3 * i + Z])
      minz = model->vertices[3 * i + Z];
  }

  /* calculate model width, height, and depth */
  w = _glmAbs(maxx) + _glmAbs(minx);
  h = _glmAbs(maxy) + _glmAbs(miny);
  d = _glmAbs(maxz) + _glmAbs(minz);

  /* calculate center of the model */
  cx = (maxx + minx) / 2.0;
  cy = (maxy + miny) / 2.0;
  cz = (maxz + minz) / 2.0;

  /* calculate unitizing scale factor */
  scale = 2.0 / _glmMax(_glmMax(w, h), d);

  /* translate around center then scale */
  for (i = 1; i <= model->numvertices; i++) {
    model->vertices[3 * i + X] -= cx;
    model->vertices[3 * i + Y] -= cy;
    model->vertices[3 * i + Z] -= cz;
    model->vertices[3 * i + X] *= scale;
    model->vertices[3 * i + Y] *= scale;
    model->vertices[3 * i + Z] *= scale;
  }

  return scale;
}

/* glmDimensions: Calculates the dimensions (width, height, depth) of
 * a model.
 *
 * model      - initialized GLMmodel structure
 * dimensions - array of 3 floats (float dimensions[3])
 */
void
glmDimensions(GLMmodel* model, float* dimensions)
{
  uint i;
  float maxx, minx, maxy, miny, maxz, minz;

  assert(model);
  assert(model->vertices);
  assert(dimensions);

  /* get the max/mins */
  maxx = minx = model->vertices[3 + X];
  maxy = miny = model->vertices[3 + Y];
  maxz = minz = model->vertices[3 + Z];
  for (i = 1; i <= model->numvertices; i++) {
    if (maxx < model->vertices[3 * i + X])
      maxx = model->vertices[3 * i + X];
    if (minx > model->vertices[3 * i + X])
      minx = model->vertices[3 * i + X];

    if (maxy < model->vertices[3 * i + Y])
      maxy = model->vertices[3 * i + Y];
    if (miny > model->vertices[3 * i + Y])
      miny = model->vertices[3 * i + Y];

    if (maxz < model->vertices[3 * i + Z])
      maxz = model->vertices[3 * i + Z];
    if (minz > model->vertices[3 * i + Z])
      minz = model->vertices[3 * i + Z];
  }

  /* calculate model width, height, and depth */
  dimensions[X] = _glmAbs(maxx) + _glmAbs(minx);
  dimensions[Y] = _glmAbs(maxy) + _glmAbs(miny);
  dimensions[Z] = _glmAbs(maxz) + _glmAbs(minz);
}

/* glmScale: Scales a model by a given amount.
 * 
 * model - properly initialized GLMmodel structure
 * scale - scalefactor (0.5 = half as large, 2.0 = twice as large)
 */
void
glmScale(GLMmodel* model, float scale)
{
  uint i;

  for (i = 1; i <= model->numvertices; i++) {
    model->vertices[3 * i + X] *= scale;
    model->vertices[3 * i + Y] *= scale;
    model->vertices[3 * i + Z] *= scale;
  }
}

/* glmReverseWinding: Reverse the polygon winding for all polygons in
 * this model.  Default winding is counter-clockwise.  Also changes
 * the direction of the normals.
 * 
 * model - properly initialized GLMmodel structure 
 */
void
glmReverseWinding(GLMmodel* model)
{
  uint i, swap;

  assert(model);

  for (i = 0; i < model->numtriangles; i++) {
    swap = T(i).vindices[0];
    T(i).vindices[0] = T(i).vindices[2];
    T(i).vindices[2] = swap;

    if (model->numnormals) {
      swap = T(i).nindices[0];
      T(i).nindices[0] = T(i).nindices[2];
      T(i).nindices[2] = swap;
    }

    if (model->numtexcoords) {
      swap = T(i).tindices[0];
      T(i).tindices[0] = T(i).tindices[2];
      T(i).tindices[2] = swap;
    }
  }

  /* reverse facet normals */
  for (i = 1; i <= model->numfacetnorms; i++) {
    model->facetnorms[3 * i + X] = -model->facetnorms[3 * i + X];
    model->facetnorms[3 * i + Y] = -model->facetnorms[3 * i + Y];
    model->facetnorms[3 * i + Z] = -model->facetnorms[3 * i + Z];
  }

  /* reverse vertex normals */
  for (i = 1; i <= model->numnormals; i++) {
    model->normals[3 * i + X] = -model->normals[3 * i + X];
    model->normals[3 * i + Y] = -model->normals[3 * i + Y];
    model->normals[3 * i + Z] = -model->normals[3 * i + Z];
  }
}

/* glmFacetNormals: Generates facet normals for a model (by taking the
 * cross product of the two vectors derived from the sides of each
 * triangle).  Assumes a counter-clockwise winding.
 *
 * model - initialized GLMmodel structure
 */
void
glmFacetNormals(GLMmodel* model)
{
  uint  i;
  float u[3];
  float v[3];
  
  assert(model);
  assert(model->vertices);

  /* clobber any old facetnormals */
  if (model->facetnorms)
    free(model->facetnorms);

  /* allocate memory for the new facet normals */
  model->numfacetnorms = model->numtriangles;
  model->facetnorms = (float*)malloc(sizeof(float) *
				       3 * (model->numfacetnorms + 1));

  for (i = 0; i < model->numtriangles; i++) {
    model->triangles[i].findex = i+1;

    u[X] = model->vertices[3 * T(i).vindices[1] + X] -
           model->vertices[3 * T(i).vindices[0] + X];
    u[Y] = model->vertices[3 * T(i).vindices[1] + Y] -
           model->vertices[3 * T(i).vindices[0] + Y];
    u[Z] = model->vertices[3 * T(i).vindices[1] + Z] -
           model->vertices[3 * T(i).vindices[0] + Z];

    v[X] = model->vertices[3 * T(i).vindices[2] + X] -
           model->vertices[3 * T(i).vindices[0] + X];
    v[Y] = model->vertices[3 * T(i).vindices[2] + Y] -
           model->vertices[3 * T(i).vindices[0] + Y];
    v[Z] = model->vertices[3 * T(i).vindices[2] + Z] -
           model->vertices[3 * T(i).vindices[0] + Z];

    _glmCross(u, v, &model->facetnorms[3 * (i+1)]);
    _glmNormalize(&model->facetnorms[3 * (i+1)]);
  }
}

/* glmVertexNormals: Generates smooth vertex normals for a model.
 * First builds a list of all the triangles each vertex is in.  Then
 * loops through each vertex in the list averaging all the facet
 * normals of the triangles each vertex is in.  Finally, sets the
 * normal index in the triangle for the vertex to the generated smooth
 * normal.  If the dot product of a facet normal and the facet normal
 * associated with the first triangle in the list of triangles the
 * current vertex is in is greater than the cosine of the angle
 * parameter to the function, that facet normal is not added into the
 * average normal calculation and the corresponding vertex is given
 * the facet normal.  This tends to preserve hard edges.  The angle to
 * use depends on the model, but 90 degrees is usually a good start.
 *
 * model - initialized GLMmodel structure
 * angle - maximum angle (in degrees) to smooth across
 */
void
glmVertexNormals(GLMmodel* model, float angle)
{
  GLMnode*  node;
  GLMnode*  tail;
  GLMnode** members;
  float*  normals;
  uint    numnormals;
  float   average[3];
  float   dot, cos_angle;
  uint    i, avg;

  assert(model);
  assert(model->facetnorms);

  /* calculate the cosine of the angle (in degrees) */
  cos_angle = cos(angle * M_PI / 180.0);

  /* nuke any previous normals */
  if (model->normals)
    free(model->normals);

  /* allocate space for new normals */
  model->numnormals = model->numtriangles * 3; /* 3 normals per triangle */
  model->normals = (float*)malloc(sizeof(float)* 3* (model->numnormals+1));

  /* allocate a structure that will hold a linked list of triangle
     indices for each vertex */
  members = (GLMnode**)malloc(sizeof(GLMnode*) * (model->numvertices + 1));
  for (i = 1; i <= model->numvertices; i++)
    members[i] = NULL;
  
  /* for every triangle, create a node for each vertex in it */
  for (i = 0; i < model->numtriangles; i++) {
    node = (GLMnode*)malloc(sizeof(GLMnode));
    node->index = i;
    node->next  = members[T(i).vindices[0]];
    members[T(i).vindices[0]] = node;

    node = (GLMnode*)malloc(sizeof(GLMnode));
    node->index = i;
    node->next  = members[T(i).vindices[1]];
    members[T(i).vindices[1]] = node;

    node = (GLMnode*)malloc(sizeof(GLMnode));
    node->index = i;
    node->next  = members[T(i).vindices[2]];
    members[T(i).vindices[2]] = node;
  }

  /* calculate the average normal for each vertex */
  numnormals = 1;
  for (i = 1; i <= model->numvertices; i++) {
    /* calculate an average normal for this vertex by averaging the
       facet normal of every triangle this vertex is in */
    node = members[i];
    if (!node)
      fprintf(stderr, "glmVertexNormals(): vertex w/o a triangle\n");
    average[0] = 0.0; average[1] = 0.0; average[2] = 0.0;
    avg = 0;
    while (node) {
      /* only average if the dot product of the angle between the two
         facet normals is greater than the cosine of the threshold
         angle -- or, said another way, the angle between the two
         facet normals is less than (or equal to) the threshold angle */
      dot = _glmDot(&model->facetnorms[3 * T(node->index).findex],
 		    &model->facetnorms[3 * T(members[i]->index).findex]);
      if (dot > cos_angle) {
	node->averaged = TRUE;
	average[0] += model->facetnorms[3 * T(node->index).findex + 0];
	average[1] += model->facetnorms[3 * T(node->index).findex + 1];
	average[2] += model->facetnorms[3 * T(node->index).findex + 2];
	avg = 1;			/* we averaged at least one normal! */
      } else {
	node->averaged = FALSE;
      }
      node = node->next;
    }

    if (avg) {
      /* normalize the averaged normal */
      _glmNormalize(average);

      /* add the normal to the vertex normals list */
      model->normals[3 * numnormals + 0] = average[0];
      model->normals[3 * numnormals + 1] = average[1];
      model->normals[3 * numnormals + 2] = average[2];
      avg = numnormals;
      numnormals++;
    }

    /* set the normal of this vertex in each triangle it is in */
    node = members[i];
    while (node) {
      if (node->averaged) {
	/* if this node was averaged, use the average normal */
	if (T(node->index).vindices[0] == i)
	  T(node->index).nindices[0] = avg;
	else if (T(node->index).vindices[1] == i)
	  T(node->index).nindices[1] = avg;
	else if (T(node->index).vindices[2] == i)
	  T(node->index).nindices[2] = avg;
      } else {
	/* if this node wasn't averaged, use the facet normal */
	model->normals[3 * numnormals + 0] = 
	  model->facetnorms[3 * T(node->index).findex + 0];
	model->normals[3 * numnormals + 1] = 
	  model->facetnorms[3 * T(node->index).findex + 1];
	model->normals[3 * numnormals + 2] = 
	  model->facetnorms[3 * T(node->index).findex + 2];
	if (T(node->index).vindices[0] == i)
	  T(node->index).nindices[0] = numnormals;
	else if (T(node->index).vindices[1] == i)
	  T(node->index).nindices[1] = numnormals;
	else if (T(node->index).vindices[2] == i)
	  T(node->index).nindices[2] = numnormals;
	numnormals++;
      }
      node = node->next;
    }
  }
  
  model->numnormals = numnormals - 1;

  /* free the member information */
  for (i = 1; i <= model->numvertices; i++) {
    node = members[i];
    while (node) {
      tail = node;
      node = node->next;
      free(tail);
    }
  }
  free(members);

  /* pack the normals array (we previously allocated the maximum
     number of normals that could possibly be created (numtriangles *
     3), so get rid of some of them (usually alot unless none of the
     facet normals were averaged)) */
  normals = model->normals;
  model->normals = (float*)malloc(sizeof(float)* 3* (model->numnormals+1));
  for (i = 1; i <= model->numnormals; i++) {
    model->normals[3 * i + 0] = normals[3 * i + 0];
    model->normals[3 * i + 1] = normals[3 * i + 1];
    model->normals[3 * i + 2] = normals[3 * i + 2];
  }
  free(normals);

  printf("glmVertexNormals(): %d normals generated\n", model->numnormals);
}


/* glmLinearTexture: Generates texture coordinates according to a
 * linear projection of the texture map.  It generates these by
 * linearly mapping the vertices onto a square.
 *
 * model - pointer to initialized GLMmodel structure
 */
void
glmLinearTexture(GLMmodel* model)
{
  GLMgroup *group;
  float dimensions[3];
  float x, y, scalefactor;
  uint i;
  
  assert(model);

  if (model->texcoords)
    free(model->texcoords);
  model->numtexcoords = model->numvertices;
  model->texcoords=(float*)malloc(sizeof(float)*2*(model->numtexcoords+1));
  
  glmDimensions(model, dimensions);
  scalefactor = 2.0 / 
    _glmAbs(_glmMax(_glmMax(dimensions[0], dimensions[1]), dimensions[2]));

  /* do the calculations */
  for(i = 1; i <= model->numvertices; i++) {
    x = model->vertices[3 * i + 0] * scalefactor;
    y = model->vertices[3 * i + 2] * scalefactor;
    model->texcoords[2 * i + 0] = (x + 1.0) / 2.0;
    model->texcoords[2 * i + 1] = (y + 1.0) / 2.0;
  }
  
  /* go through and put texture coordinate indices in all the triangles */
  group = model->groups;
  while(group) {
    for(i = 0; i < group->numtriangles; i++) {
      T(group->triangles[i]).tindices[0] = T(group->triangles[i]).vindices[0];
      T(group->triangles[i]).tindices[1] = T(group->triangles[i]).vindices[1];
      T(group->triangles[i]).tindices[2] = T(group->triangles[i]).vindices[2];
    }    
    group = group->next;
  }

#if 0
  printf("glmLinearTexture(): generated %d linear texture coordinates\n",
	  model->numtexcoords);
#endif
}

/* glmSpheremapTexture: Generates texture coordinates according to a
 * spherical projection of the texture map.  Sometimes referred to as
 * spheremap, or reflection map texture coordinates.  It generates
 * these by using the normal to calculate where that vertex would map
 * onto a sphere.  Since it is impossible to map something flat
 * perfectly onto something spherical, there is distortion at the
 * poles.  This particular implementation causes the poles along the X
 * axis to be distorted.
 *
 * model - pointer to initialized GLMmodel structure
 */
void
glmSpheremapTexture(GLMmodel* model)
{
  GLMgroup* group;
  float theta, phi, rho, x, y, z, r;
  uint i;
  
  assert(model);
  assert(model->normals);

  if (model->texcoords)
    free(model->texcoords);
  model->numtexcoords = model->numnormals;
  model->texcoords=(float*)malloc(sizeof(float)*2*(model->numtexcoords+1));
     
  /* do the calculations */
  for (i = 1; i <= model->numnormals; i++) {
    z = model->normals[3 * i + 0];	/* re-arrange for pole distortion */
    y = model->normals[3 * i + 1];
    x = model->normals[3 * i + 2];
    r = sqrt((x * x) + (y * y));
    rho = sqrt((r * r) + (z * z));
      
    if(r == 0.0) {
	theta = 0.0;
	phi = 0.0;
    } else {
      if(z == 0.0)
	phi = M_PI / 2.0;
      else
	phi = acos(z / rho);
      
#if WE_DONT_NEED_THIS_CODE
      if(x == 0.0)
	theta = M_PI / 2.0;	/* asin(y / r); */
      else
	theta = acos(x / r);
#endif
      
      if(y == 0.0)
	theta = M_PI / 2.0;	/* acos(x / r); */
      else
	theta = asin(y / r) + (M_PI / 2.0);
    }
    
    model->texcoords[2 * i + 0] = theta / M_PI;
    model->texcoords[2 * i + 1] = phi / M_PI;
  }
  
  /* go through and put texcoord indices in all the triangles */
  group = model->groups;
  while(group) {
    for (i = 0; i < group->numtriangles; i++) {
      T(group->triangles[i]).tindices[0] = T(group->triangles[i]).nindices[0];
      T(group->triangles[i]).tindices[1] = T(group->triangles[i]).nindices[1];
      T(group->triangles[i]).tindices[2] = T(group->triangles[i]).nindices[2];
    }
    group = group->next;
  }

#if 0  
  printf("glmSpheremapTexture(): generated %d spheremap texture coordinates\n",
	 model->numtexcoords);
#endif
}

/* glmDelete: Deletes a GLMmodel structure.
 *
 * model - initialized GLMmodel structure
 */
void
glmDelete(GLMmodel* model)
{
  GLMgroup* group;
  uint i;

  assert(model);

  if (model->pathname)   free(model->pathname);
  if (model->mtllibname) free(model->mtllibname);
  if (model->vertices)   free(model->vertices);
  if (model->normals)    free(model->normals);
  if (model->texcoords)  free(model->texcoords);
  if (model->facetnorms) free(model->facetnorms);
  if (model->triangles)  free(model->triangles);
  if (model->materials) {
    for (i = 0; i < model->nummaterials; i++)
      free(model->materials[i].name);
  }
  free(model->materials);
  while(model->groups) {
    group = model->groups;
    model->groups = model->groups->next;
    free(group->name);
    free(group->triangles);
    free(group);
  }

  free(model);
}

static GLMmaterial *
glmDefaultMaterial(void)
{
   GLMmaterial *m = (GLMmaterial *) calloc(1, sizeof(GLMmaterial));
   
   m->diffuse[0] = 0.75;
   m->diffuse[1] = 0.75;
   m->diffuse[2] = 0.75;
   m->diffuse[3] = 1.0;

   m->specular[0] = 1.0;
   m->specular[1] = 1.0;
   m->specular[2] = 1.0;
   m->specular[3] = 1.0;

   m->shininess = 5;

   return m;
}


/* glmReadOBJ: Reads a model description from a Wavefront .OBJ file.
 * Returns a pointer to the created object which should be free'd with
 * glmDelete().
 *
 * filename - name of the file containing the Wavefront .OBJ format data.  
 */
GLMmodel* 
glmReadOBJ(char* filename)
{
  GLMmodel* model;
  FILE*     file;

  /* open the file */
  file = fopen(filename, "r");
  if (!file) {
    fprintf(stderr, "glmReadOBJ() failed: can't open data file \"%s\".\n",
	    filename);
    exit(1);
  }

#if 0
  /* announce the model name */
  printf("Model: %s\n", filename);
#endif

  /* allocate a new model */
  model = (GLMmodel*)malloc(sizeof(GLMmodel));
  model->pathname      = stralloc(filename);
  model->mtllibname    = NULL;
  model->numvertices   = 0;
  model->vertices      = NULL;
  model->numnormals    = 0;
  model->normals       = NULL;
  model->numtexcoords  = 0;
  model->texcoords     = NULL;
  model->numfacetnorms = 0;
  model->facetnorms    = NULL;
  model->numtriangles  = 0;
  model->triangles     = NULL;
  model->nummaterials  = 0;
  model->materials     = NULL;
  model->numgroups     = 0;
  model->groups        = NULL;
  model->position[0]   = 0.0;
  model->position[1]   = 0.0;
  model->position[2]   = 0.0;
  model->scale         = 1.0;

  /* make a first pass through the file to get a count of the number
     of vertices, normals, texcoords & triangles */
  _glmFirstPass(model, file);

  /* allocate memory */
  model->vertices = (float*)malloc(sizeof(float) *
				     3 * (model->numvertices + 1));
  model->triangles = (GLMtriangle*)malloc(sizeof(GLMtriangle) *
					  model->numtriangles);
  if (model->numnormals) {
    model->normals = (float*)malloc(sizeof(float) *
				      3 * (model->numnormals + 1));
  }
  if (model->numtexcoords) {
    model->texcoords = (float*)malloc(sizeof(float) *
					2 * (model->numtexcoords + 1));
  }

  /* rewind to beginning of file and read in the data this pass */
  rewind(file);

  _glmSecondPass(model, file);

  /* close the file */
  fclose(file);

  if (!model->materials) {
     model->materials = glmDefaultMaterial();
     model->nummaterials = 1;
  }

  return model;
}

/* glmWriteOBJ: Writes a model description in Wavefront .OBJ format to
 * a file.
 *
 * model    - initialized GLMmodel structure
 * filename - name of the file to write the Wavefront .OBJ format data to
 * mode     - a bitwise or of values describing what is written to the file
 *            GLM_NONE     -  render with only vertices
 *            GLM_FLAT     -  render with facet normals
 *            GLM_SMOOTH   -  render with vertex normals
 *            GLM_TEXTURE  -  render with texture coords
 *            GLM_COLOR    -  render with colors (color material)
 *            GLM_MATERIAL -  render with materials
 *            GLM_COLOR and GLM_MATERIAL should not both be specified.  
 *            GLM_FLAT and GLM_SMOOTH should not both be specified.  
 */
void
glmWriteOBJ(GLMmodel* model, char* filename, uint mode)
{
  uint    i;
  FILE*     file;
  GLMgroup* group;

  assert(model);

  /* do a bit of warning */
  if (mode & GLM_FLAT && !model->facetnorms) {
    printf("glmWriteOBJ() warning: flat normal output requested "
	   "with no facet normals defined.\n");
    mode &= ~GLM_FLAT;
  }
  if (mode & GLM_SMOOTH && !model->normals) {
    printf("glmWriteOBJ() warning: smooth normal output requested "
	   "with no normals defined.\n");
    mode &= ~GLM_SMOOTH;
  }
  if (mode & GLM_TEXTURE && !model->texcoords) {
    printf("glmWriteOBJ() warning: texture coordinate output requested "
	   "with no texture coordinates defined.\n");
    mode &= ~GLM_TEXTURE;
  }
  if (mode & GLM_FLAT && mode & GLM_SMOOTH) {
    printf("glmWriteOBJ() warning: flat normal output requested "
	   "and smooth normal output requested (using smooth).\n");
    mode &= ~GLM_FLAT;
  }

  /* open the file */
  file = fopen(filename, "w");
  if (!file) {
    fprintf(stderr, "glmWriteOBJ() failed: can't open file \"%s\" to write.\n",
	    filename);
    exit(1);
  }

  /* spit out a header */
  fprintf(file, "#  \n");
  fprintf(file, "#  Wavefront OBJ generated by GLM library\n");
  fprintf(file, "#  \n");
  fprintf(file, "#  GLM library copyright (C) 1997 by Nate Robins\n");
  fprintf(file, "#  email: ndr@pobox.com\n");
  fprintf(file, "#  www:   http://www.pobox.com/~ndr\n");
  fprintf(file, "#  \n");

  if (mode & GLM_MATERIAL && model->mtllibname) {
    fprintf(file, "\nmtllib %s\n\n", model->mtllibname);
    _glmWriteMTL(model, filename, model->mtllibname);
  }

  /* spit out the vertices */
  fprintf(file, "\n");
  fprintf(file, "# %d vertices\n", model->numvertices);
  for (i = 1; i <= model->numvertices; i++) {
    fprintf(file, "v %f %f %f\n", 
	    model->vertices[3 * i + 0],
	    model->vertices[3 * i + 1],
	    model->vertices[3 * i + 2]);
  }

  /* spit out the smooth/flat normals */
  if (mode & GLM_SMOOTH) {
    fprintf(file, "\n");
    fprintf(file, "# %d normals\n", model->numnormals);
    for (i = 1; i <= model->numnormals; i++) {
      fprintf(file, "vn %f %f %f\n", 
	      model->normals[3 * i + 0],
	      model->normals[3 * i + 1],
	      model->normals[3 * i + 2]);
    }
  } else if (mode & GLM_FLAT) {
    fprintf(file, "\n");
    fprintf(file, "# %d normals\n", model->numfacetnorms);
    for (i = 1; i <= model->numnormals; i++) {
      fprintf(file, "vn %f %f %f\n", 
	      model->facetnorms[3 * i + 0],
	      model->facetnorms[3 * i + 1],
	      model->facetnorms[3 * i + 2]);
    }
  }

  /* spit out the texture coordinates */
  if (mode & GLM_TEXTURE) {
    fprintf(file, "\n");
    fprintf(file, "# %d texcoords\n", model->numtexcoords);
    for (i = 1; i <= model->numtexcoords; i++) {
      fprintf(file, "vt %f %f\n", 
	      model->texcoords[2 * i + 0],
	      model->texcoords[2 * i + 1]);
    }
  }

  fprintf(file, "\n");
  fprintf(file, "# %d groups\n", model->numgroups);
  fprintf(file, "# %d faces (triangles)\n", model->numtriangles);
  fprintf(file, "\n");

  group = model->groups;
  while(group) {
    fprintf(file, "g %s\n", group->name);
    if (mode & GLM_MATERIAL)
      fprintf(file, "usemtl %s\n", model->materials[group->material].name);
    for (i = 0; i < group->numtriangles; i++) {
      if (mode & GLM_SMOOTH && mode & GLM_TEXTURE) {
	fprintf(file, "f %d/%d/%d %d/%d/%d %d/%d/%d\n",
		T(group->triangles[i]).vindices[0], 
		T(group->triangles[i]).nindices[0], 
		T(group->triangles[i]).tindices[0],
		T(group->triangles[i]).vindices[1],
		T(group->triangles[i]).nindices[1],
		T(group->triangles[i]).tindices[1],
		T(group->triangles[i]).vindices[2],
		T(group->triangles[i]).nindices[2],
		T(group->triangles[i]).tindices[2]);
      } else if (mode & GLM_FLAT && mode & GLM_TEXTURE) {
	fprintf(file, "f %d/%d %d/%d %d/%d\n",
		T(group->triangles[i]).vindices[0],
		T(group->triangles[i]).findex,
		T(group->triangles[i]).vindices[1],
		T(group->triangles[i]).findex,
		T(group->triangles[i]).vindices[2],
		T(group->triangles[i]).findex);
      } else if (mode & GLM_TEXTURE) {
	fprintf(file, "f %d/%d %d/%d %d/%d\n",
		T(group->triangles[i]).vindices[0],
		T(group->triangles[i]).tindices[0],
		T(group->triangles[i]).vindices[1],
		T(group->triangles[i]).tindices[1],
		T(group->triangles[i]).vindices[2],
		T(group->triangles[i]).tindices[2]);
      } else if (mode & GLM_SMOOTH) {
	fprintf(file, "f %d//%d %d//%d %d//%d\n",
		T(group->triangles[i]).vindices[0],
		T(group->triangles[i]).nindices[0],
		T(group->triangles[i]).vindices[1],
		T(group->triangles[i]).nindices[1],
		T(group->triangles[i]).vindices[2], 
		T(group->triangles[i]).nindices[2]);
      } else if (mode & GLM_FLAT) {
	fprintf(file, "f %d//%d %d//%d %d//%d\n",
		T(group->triangles[i]).vindices[0], 
		T(group->triangles[i]).findex,
		T(group->triangles[i]).vindices[1],
		T(group->triangles[i]).findex,
		T(group->triangles[i]).vindices[2],
		T(group->triangles[i]).findex);
      } else {
	fprintf(file, "f %d %d %d\n",
		T(group->triangles[i]).vindices[0],
		T(group->triangles[i]).vindices[1],
		T(group->triangles[i]).vindices[2]);
      }
    }
    fprintf(file, "\n");
    group = group->next;
  }

  fclose(file);
}

/* glmWeld: eliminate (weld) vectors that are within an epsilon of
 * each other.
 *
 * model      - initialized GLMmodel structure
 * epsilon    - maximum difference between vertices
 *              ( 0.00001 is a good start for a unitized model)
 *
 */
void
glmWeld(GLMmodel* model, float epsilon)
{
  float* vectors;
  float* copies;
  uint   numvectors;
  uint   i;

  /* vertices */
  numvectors = model->numvertices;
  vectors    = model->vertices;
  copies = _glmWeldVectors(vectors, &numvectors, epsilon);

  printf("glmWeld(): %d redundant vertices.\n", 
	 model->numvertices - numvectors - 1);

  for (i = 0; i < model->numtriangles; i++) {
    T(i).vindices[0] = (uint)vectors[3 * T(i).vindices[0] + 0];
    T(i).vindices[1] = (uint)vectors[3 * T(i).vindices[1] + 0];
    T(i).vindices[2] = (uint)vectors[3 * T(i).vindices[2] + 0];
  }

  /* free space for old vertices */
  free(vectors);

  /* allocate space for the new vertices */
  model->numvertices = numvectors;
  model->vertices = (float*)malloc(sizeof(float) * 
				     3 * (model->numvertices + 1));

  /* copy the optimized vertices into the actual vertex list */
  for (i = 1; i <= model->numvertices; i++) {
    model->vertices[3 * i + 0] = copies[3 * i + 0];
    model->vertices[3 * i + 1] = copies[3 * i + 1];
    model->vertices[3 * i + 2] = copies[3 * i + 2];
  }

  free(copies);
}


void
glmReIndex(GLMmodel *model)
{
  uint i, j, n;
  GLMgroup* group;
  float *newNormals = NULL;
  float *newTexcoords = NULL;
  const uint numv = model->numvertices;

  if (model->numnormals > 0)
     newNormals = (float *) malloc((numv + 1) * 3 * sizeof(float));

  if (model->numtexcoords > 0)
     newTexcoords = (float *) malloc((numv + 1) * 2 * sizeof(float));

  for (group = model->groups; group; group = group->next) {

    n = group->numtriangles;

    group->triIndexes = (uint *) malloc(n * 3 * sizeof(uint));

    group->minIndex = 10000000;
    group->maxIndex = 0;

    for (i = 0; i < n; i++) {

       for (j = 0; j < 3; j++) {
          uint vindex = T(group->triangles[i]).vindices[j];
          uint nindex = T(group->triangles[i]).nindices[j];
          uint tindex = T(group->triangles[i]).tindices[j];

          float *nrm = &model->normals[nindex * 3];
          float *tex = &model->texcoords[tindex * 2];

          if (newNormals) {
             assert(vindex * 3 + 2 < (numv + 1) * 3);
             newNormals[vindex * 3 + 0] = nrm[0];
             newNormals[vindex * 3 + 1] = nrm[1];
             newNormals[vindex * 3 + 2] = nrm[2];
          }
          if (newTexcoords) {
             newTexcoords[vindex * 2 + 0] = tex[0];
             newTexcoords[vindex * 2 + 1] = tex[1];
          }

          T(group->triangles[i]).nindices[j] = vindex;
          T(group->triangles[i]).tindices[j] = vindex;

          group->triIndexes[i * 3 + j] = vindex;

          if (vindex > group->maxIndex)
             group->maxIndex = vindex;
          if (vindex < group->minIndex)
             group->minIndex = vindex;
       }
    }
  }

  if (newNormals) {
     free(model->normals);
     model->normals = newNormals;
     model->numnormals = model->numvertices;
  }

  if (newTexcoords) {
     free(model->texcoords);
     model->texcoords = newTexcoords;
     model->numtexcoords = model->numvertices;
  }
}



void
glmPrint(const GLMmodel *model)
{
  uint i, j, grp, n;
  GLMgroup* group;
  uint totalTris = 0;

  grp = 0;

  printf("%u vertices\n", model->numvertices);
  printf("%u normals\n", model->numnormals);
  printf("%u texcoords\n", model->numtexcoords);

  for (group = model->groups; group; group = group->next, grp++) {
     printf("Group %u:\n", grp);
     printf("  Min index %u, max index %u\n", group->minIndex, group->maxIndex);

#if 0
    if (mode & GLM_MATERIAL) {
      glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, 
		   model->materials[group->material].ambient);
      glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, 
		   model->materials[group->material].diffuse);
      glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, 
		   model->materials[group->material].specular);
       glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 
  		  model->materials[group->material].shininess);
    }

    if (mode & GLM_COLOR) {
      glColor3fv(model->materials[group->material].diffuse);
    }
#endif
    totalTris += group->numtriangles;

    printf("  %u triangles\n", group->numtriangles);
    n = group->numtriangles;
    if (n > 10)
      n = 10;

    for (i = 0; i < n; i++) {

       printf("    %u: vert ", i);
       for (j = 0; j < 3; j++) {
          printf("%u ", T(group->triangles[i]).vindices[j]);
       }

       printf("    normal ");
       for (j = 0; j < 3; j++) {
          printf("%u ", T(group->triangles[i]).nindices[j]);
       }

       printf("    tex ");
       for (j = 0; j < 3; j++) {
          printf("%u ", T(group->triangles[i]).tindices[j]);
       }

       printf("\n");
    }
  }
  printf("Total tris: %u\n", totalTris);
}



#if 0
  /* normals */
  if (model->numnormals) {
  numvectors = model->numnormals;
  vectors    = model->normals;
  copies = _glmOptimizeVectors(vectors, &numvectors);

  printf("glmOptimize(): %d redundant normals.\n", 
	 model->numnormals - numvectors);

  for (i = 0; i < model->numtriangles; i++) {
    T(i).nindices[0] = (uint)vectors[3 * T(i).nindices[0] + 0];
    T(i).nindices[1] = (uint)vectors[3 * T(i).nindices[1] + 0];
    T(i).nindices[2] = (uint)vectors[3 * T(i).nindices[2] + 0];
  }

  /* free space for old normals */
  free(vectors);

  /* allocate space for the new normals */
  model->numnormals = numvectors;
  model->normals = (float*)malloc(sizeof(float) * 
				    3 * (model->numnormals + 1));

  /* copy the optimized vertices into the actual vertex list */
  for (i = 1; i <= model->numnormals; i++) {
    model->normals[3 * i + 0] = copies[3 * i + 0];
    model->normals[3 * i + 1] = copies[3 * i + 1];
    model->normals[3 * i + 2] = copies[3 * i + 2];
  }

  free(copies);
  }

  /* texcoords */
  if (model->numtexcoords) {
  numvectors = model->numtexcoords;
  vectors    = model->texcoords;
  copies = _glmOptimizeVectors(vectors, &numvectors);

  printf("glmOptimize(): %d redundant texcoords.\n", 
	 model->numtexcoords - numvectors);

  for (i = 0; i < model->numtriangles; i++) {
    for (j = 0; j < 3; j++) {
      T(i).tindices[j] = (uint)vectors[3 * T(i).tindices[j] + 0];
    }
  }

  /* free space for old texcoords */
  free(vectors);

  /* allocate space for the new texcoords */
  model->numtexcoords = numvectors;
  model->texcoords = (float*)malloc(sizeof(float) * 
				      2 * (model->numtexcoords + 1));

  /* copy the optimized vertices into the actual vertex list */
  for (i = 1; i <= model->numtexcoords; i++) {
    model->texcoords[2 * i + 0] = copies[2 * i + 0];
    model->texcoords[2 * i + 1] = copies[2 * i + 1];
  }

  free(copies);
  }
#endif

#if 0
  /* look for unused vertices */
  /* look for unused normals */
  /* look for unused texcoords */
  for (i = 1; i <= model->numvertices; i++) {
    for (j = 0; j < model->numtriangles; i++) {
      if (T(j).vindices[0] == i || 
	  T(j).vindices[1] == i || 
	  T(j).vindices[1] == i)
	break;
    }
  }
#endif
