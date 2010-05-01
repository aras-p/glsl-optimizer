/*    
 *  GLM library.  Wavefront .obj file format reader/writer/manipulator.
 *
 *  Written by Nate Robins, 1997.
 *  email: ndr@pobox.com
 *  www: http://www.pobox.com/~ndr
 */

#ifndef GLM_H
#define GLM_H


typedef unsigned int uint;


#ifndef M_PI
#define M_PI 3.14159265
#endif


/* defines */
#define GLM_NONE     (0)		/* render with only vertices */
#define GLM_FLAT     (1 << 0)		/* render with facet normals */
#define GLM_SMOOTH   (1 << 1)		/* render with vertex normals */
#define GLM_TEXTURE  (1 << 2)		/* render with texture coords */
#define GLM_COLOR    (1 << 3)		/* render with colors */
#define GLM_MATERIAL (1 << 4)		/* render with materials */


/* structs */

/* GLMmaterial: Structure that defines a material in a model. 
 */
typedef struct _GLMmaterial
{
  char* name;				/* name of material */
  float diffuse[4];			/* diffuse component */
  float ambient[4];			/* ambient component */
  float specular[4];			/* specular component */
  float emmissive[4];			/* emmissive component */
  float shininess;			/* specular exponent */
  char *map_kd;                         /* diffuse texture map file */
  uint texture_kd;                      /* diffuse texture map */
  uint texture_ks;                      /* specular texture map */
  int uDiffuse, uAmbient, uSpecular, uShininess, uDiffTex, uSpecTex;
  uint prog;
} GLMmaterial;

/* GLMtriangle: Structure that defines a triangle in a model.
 */
typedef struct {
  uint vindices[3];			/* array of triangle vertex indices */
  uint nindices[3];			/* array of triangle normal indices */
  uint tindices[3];			/* array of triangle texcoord indices*/
  uint findex;			/* index of triangle facet normal */
} GLMtriangle;

/* GLMgroup: Structure that defines a group in a model.
 */
typedef struct _GLMgroup {
  char*             name;		/* name of this group */
  uint            numtriangles;	/* number of triangles in this group */
  uint*           triangles;		/* array of triangle indices */
  uint            material;           /* index to material for group */
  uint *          triIndexes;
  uint            minIndex, maxIndex;
  struct _GLMgroup* next;		/* pointer to next group in model */
} GLMgroup;

/* GLMmodel: Structure that defines a model.
 */
typedef struct {
  char*    pathname;			/* path to this model */
  char*    mtllibname;			/* name of the material library */

  uint   numvertices;			/* number of vertices in model */
  float* vertices;			/* array of vertices  */

  uint   numnormals;			/* number of normals in model */
  float* normals;			/* array of normals */

  uint   numtexcoords;		/* number of texcoords in model */
  float* texcoords;			/* array of texture coordinates */

  uint   numfacetnorms;		/* number of facetnorms in model */
  float* facetnorms;			/* array of facetnorms */

  uint       numtriangles;		/* number of triangles in model */
  GLMtriangle* triangles;		/* array of triangles */

  uint       nummaterials;		/* number of materials in model */
  GLMmaterial* materials;		/* array of materials */

  uint       numgroups;		/* number of groups in model */
  GLMgroup*    groups;			/* linked list of groups */

  float position[3];			/* position of the model */
  float scale;

  uint vbo;  /* OpenGL VBO for vertex data */
  uint vertexSize;  /* number of floats per vertex */
  uint posOffset;   /* offset of position within vertex, in bytes */
  uint normOffset;   /* offset of normal within vertex, in bytes */
  uint texOffset;   /* offset of texcoord within vertex, in bytes */
} GLMmodel;


/* public functions */

/* glmUnitize: "unitize" a model by translating it to the origin and
 * scaling it to fit in a unit cube around the origin.  Returns the
 * scalefactor used.
 *
 * model - properly initialized GLMmodel structure 
 */
float
glmUnitize(GLMmodel* model);

/* glmDimensions: Calculates the dimensions (width, height, depth) of
 * a model.
 *
 * model      - initialized GLMmodel structure
 * dimensions - array of 3 floats (float dimensions[3])
 */
void
glmDimensions(GLMmodel* model, float* dimensions);

/* glmScale: Scales a model by a given amount.
 * 
 * model - properly initialized GLMmodel structure
 * scale - scalefactor (0.5 = half as large, 2.0 = twice as large)
 */
void
glmScale(GLMmodel* model, float scale);

/* glmReverseWinding: Reverse the polygon winding for all polygons in
 * this model.  Default winding is counter-clockwise.  Also changes
 * the direction of the normals.
 * 
 * model - properly initialized GLMmodel structure 
 */
void
glmReverseWinding(GLMmodel* model);

/* glmFacetNormals: Generates facet normals for a model (by taking the
 * cross product of the two vectors derived from the sides of each
 * triangle).  Assumes a counter-clockwise winding.
 *
 * model - initialized GLMmodel structure
 */
void
glmFacetNormals(GLMmodel* model);

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
glmVertexNormals(GLMmodel* model, float angle);

/* glmLinearTexture: Generates texture coordinates according to a
 * linear projection of the texture map.  It generates these by
 * linearly mapping the vertices onto a square.
 *
 * model - pointer to initialized GLMmodel structure
 */
void
glmLinearTexture(GLMmodel* model);

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
glmSpheremapTexture(GLMmodel* model);

/* glmDelete: Deletes a GLMmodel structure.
 *
 * model - initialized GLMmodel structure
 */
void
glmDelete(GLMmodel* model);

/* glmReadOBJ: Reads a model description from a Wavefront .OBJ file.
 * Returns a pointer to the created object which should be free'd with
 * glmDelete().
 *
 * filename - name of the file containing the Wavefront .OBJ format data.  
 */
GLMmodel* 
glmReadOBJ(char* filename);

/* glmWriteOBJ: Writes a model description in Wavefront .OBJ format to
 * a file.
 *
 * model    - initialized GLMmodel structure
 * filename - name of the file to write the Wavefront .OBJ format data to
 * mode     - a bitwise or of values describing what is written to the file
 *            GLM_NONE    -  write only vertices
 *            GLM_FLAT    -  write facet normals
 *            GLM_SMOOTH  -  write vertex normals
 *            GLM_TEXTURE -  write texture coords
 *            GLM_FLAT and GLM_SMOOTH should not both be specified.
 */
void
glmWriteOBJ(GLMmodel* model, char* filename, uint mode);

/* glmDraw: Renders the model to the current OpenGL context using the
 * mode specified.
 *
 * model    - initialized GLMmodel structure
 * mode     - a bitwise OR of values describing what is to be rendered.
 *            GLM_NONE    -  render with only vertices
 *            GLM_FLAT    -  render with facet normals
 *            GLM_SMOOTH  -  render with vertex normals
 *            GLM_TEXTURE -  render with texture coords
 *            GLM_FLAT and GLM_SMOOTH should not both be specified.
 */
void
glmDraw(GLMmodel* model, uint mode);

/* glmList: Generates and returns a display list for the model using
 * the mode specified.
 *
 * model    - initialized GLMmodel structure
 * mode     - a bitwise OR of values describing what is to be rendered.
 *            GLM_NONE    -  render with only vertices
 *            GLM_FLAT    -  render with facet normals
 *            GLM_SMOOTH  -  render with vertex normals
 *            GLM_TEXTURE -  render with texture coords
 *            GLM_FLAT and GLM_SMOOTH should not both be specified.  
 */
uint
glmList(GLMmodel* model, uint mode);

/* glmWeld: eliminate (weld) vectors that are within an epsilon of
 * each other.
 *
 * model      - initialized GLMmodel structure
 * epsilon    - maximum difference between vertices
 *              ( 0.00001 is a good start for a unitized model)
 *
 */
void
glmWeld(GLMmodel* model, float epsilon);

void
glmReIndex(GLMmodel *model);

void
glmMakeVBOs(GLMmodel *model);

void
glmDrawVBO(GLMmodel *model);

void
glmPrint(const GLMmodel *model);

void
glmShaderMaterial(GLMmaterial *mat);

void
glmLoadTextures(GLMmodel *model);

void
glmSpecularTexture(GLMmodel *model, uint cubeTex);

#endif /* GLM_H */
