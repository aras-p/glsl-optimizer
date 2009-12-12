/* */

#define GL_GLEXT_PROTOTYPES

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glm.h"
#include "readtex.h"
#include "shaderutil.h"


/* defines */
#define T(x) model->triangles[(x)]


/* glmDraw: Renders the model to the current OpenGL context using the
 * mode specified.
 *
 * model    - initialized GLMmodel structure
 * mode     - a bitwise OR of values describing what is to be rendered.
 *            GLM_NONE     -  render with only vertices
 *            GLM_FLAT     -  render with facet normals
 *            GLM_SMOOTH   -  render with vertex normals
 *            GLM_TEXTURE  -  render with texture coords
 *            GLM_COLOR    -  render with colors (color material)
 *            GLM_MATERIAL -  render with materials
 *            GLM_COLOR and GLM_MATERIAL should not both be specified.  
 *            GLM_FLAT and GLM_SMOOTH should not both be specified.  
 */
GLvoid
glmDraw(GLMmodel* model, GLuint mode)
{
  GLuint i;
  GLMgroup* group;

  assert(model);
  assert(model->vertices);

  /* do a bit of warning */
  if (mode & GLM_FLAT && !model->facetnorms) {
    printf("glmDraw() warning: flat render mode requested "
	   "with no facet normals defined.\n");
    mode &= ~GLM_FLAT;
  }
  if (mode & GLM_SMOOTH && !model->normals) {
    printf("glmDraw() warning: smooth render mode requested "
	   "with no normals defined.\n");
    mode &= ~GLM_SMOOTH;
  }
  if (mode & GLM_TEXTURE && !model->texcoords) {
    printf("glmDraw() warning: texture render mode requested "
	   "with no texture coordinates defined.\n");
    mode &= ~GLM_TEXTURE;
  }
  if (mode & GLM_FLAT && mode & GLM_SMOOTH) {
    printf("glmDraw() warning: flat render mode requested "
	   "and smooth render mode requested (using smooth).\n");
    mode &= ~GLM_FLAT;
  }
  if (mode & GLM_COLOR && !model->materials) {
    printf("glmDraw() warning: color render mode requested "
	   "with no materials defined.\n");
    mode &= ~GLM_COLOR;
  }
  if (mode & GLM_MATERIAL && !model->materials) {
    printf("glmDraw() warning: material render mode requested "
	   "with no materials defined.\n");
    mode &= ~GLM_MATERIAL;
  }
  if (mode & GLM_COLOR && mode & GLM_MATERIAL) {
    printf("glmDraw() warning: color and material render mode requested "
	   "using only material mode\n");
    mode &= ~GLM_COLOR;
  }
  if (mode & GLM_COLOR)
    glEnable(GL_COLOR_MATERIAL);
  if (mode & GLM_MATERIAL)
    glDisable(GL_COLOR_MATERIAL);

  glPushMatrix();
  glTranslatef(model->position[0], model->position[1], model->position[2]);

  glBegin(GL_TRIANGLES);
  group = model->groups;
  while (group) {
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

    for (i = 0; i < group->numtriangles; i++) {
      if (mode & GLM_FLAT)
	glNormal3fv(&model->facetnorms[3 * T(group->triangles[i]).findex]);
      
      if (mode & GLM_SMOOTH)
	glNormal3fv(&model->normals[3 * T(group->triangles[i]).nindices[0]]);
      if (mode & GLM_TEXTURE)
	glTexCoord2fv(&model->texcoords[2*T(group->triangles[i]).tindices[0]]);
      glVertex3fv(&model->vertices[3 * T(group->triangles[i]).vindices[0]]);
#if 0
      printf("%f %f %f\n", 
	     model->vertices[3 * T(group->triangles[i]).vindices[0] + X],
	     model->vertices[3 * T(group->triangles[i]).vindices[0] + Y],
	     model->vertices[3 * T(group->triangles[i]).vindices[0] + Z]);
#endif
      
      if (mode & GLM_SMOOTH)
	glNormal3fv(&model->normals[3 * T(group->triangles[i]).nindices[1]]);
      if (mode & GLM_TEXTURE)
	glTexCoord2fv(&model->texcoords[2*T(group->triangles[i]).tindices[1]]);
      glVertex3fv(&model->vertices[3 * T(group->triangles[i]).vindices[1]]);
#if 0
      printf("%f %f %f\n", 
	     model->vertices[3 * T(group->triangles[i]).vindices[1] + X],
	     model->vertices[3 * T(group->triangles[i]).vindices[1] + Y],
	     model->vertices[3 * T(group->triangles[i]).vindices[1] + Z]);
#endif
      
      if (mode & GLM_SMOOTH)
	glNormal3fv(&model->normals[3 * T(group->triangles[i]).nindices[2]]);
      if (mode & GLM_TEXTURE)
	glTexCoord2fv(&model->texcoords[2*T(group->triangles[i]).tindices[2]]);
      glVertex3fv(&model->vertices[3 * T(group->triangles[i]).vindices[2]]);
#if 0
      printf("%f %f %f\n", 
	     model->vertices[3 * T(group->triangles[i]).vindices[2] + X],
	     model->vertices[3 * T(group->triangles[i]).vindices[2] + Y],
	     model->vertices[3 * T(group->triangles[i]).vindices[2] + Z]);
#endif
      
    }
    
    group = group->next;
  }
  glEnd();

  glPopMatrix();
}


void
glmMakeVBOs(GLMmodel *model)
{
   uint bytes, vertexFloats, i;
   float *buffer;

   vertexFloats = 3;
   model->posOffset = 0;

   if (model->numnormals > 0) {
      assert(model->numnormals == model->numvertices);
      model->normOffset = vertexFloats * sizeof(GLfloat);
      vertexFloats += 3;
   }      

   if (model->numtexcoords > 0) {
      assert(model->numtexcoords == model->numvertices);
      model->texOffset = vertexFloats * sizeof(GLfloat);
      vertexFloats += 2;
   }

   model->vertexSize = vertexFloats;

   bytes = (model->numvertices + 1) * vertexFloats * sizeof(float);

   buffer = (float *) malloc(bytes);
   for (i = 0; i < model->numvertices; i++) {
      /* copy vertex pos */
      uint j = 0;
      buffer[i * vertexFloats + j++] = model->vertices[i * 3 + 0];
      buffer[i * vertexFloats + j++] = model->vertices[i * 3 + 1];
      buffer[i * vertexFloats + j++] = model->vertices[i * 3 + 2];
      if (model->numnormals > 0) {
         buffer[i * vertexFloats + j++] = model->normals[i * 3 + 0];
         buffer[i * vertexFloats + j++] = model->normals[i * 3 + 1];
         buffer[i * vertexFloats + j++] = model->normals[i * 3 + 2];
      }
      if (model->numtexcoords > 0) {
         buffer[i * vertexFloats + j++] = model->texcoords[i * 2 + 0];
         buffer[i * vertexFloats + j++] = model->texcoords[i * 2 + 1];
      }
   }

   glGenBuffersARB(1, &model->vbo);
   glBindBufferARB(GL_ARRAY_BUFFER_ARB, model->vbo);
   glBufferDataARB(GL_ARRAY_BUFFER_ARB, bytes, buffer, GL_STATIC_DRAW_ARB);
   glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

   free(buffer);
}


static void
_glmLoadTexture(GLMmaterial *mat)
{
  if (mat->map_kd) {
     GLint imgWidth, imgHeight;
     GLenum imgFormat;
     GLubyte *image = NULL;
     
     glGenTextures(1, &mat->texture_kd);

     image = LoadRGBImage( mat->map_kd, &imgWidth, &imgHeight, &imgFormat );
     if (!image) {
        /*fprintf(stderr, "Couldn't open texture %s\n", mat->map_kd);*/
        free(mat->map_kd);
        mat->map_kd = NULL;
        mat->texture_kd = 0;
        return;
     }
     if (0)
        printf("load texture %s %d x %d\n", mat->map_kd, imgWidth, imgHeight);

     glBindTexture(GL_TEXTURE_2D, mat->texture_kd);
     gluBuild2DMipmaps(GL_TEXTURE_2D, 3, imgWidth, imgHeight,
                       imgFormat, GL_UNSIGNED_BYTE, image);
     free(image);

     glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
     glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
     glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
     glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
  }
}

void
glmLoadTextures(GLMmodel *model)
{
   uint i;

   for (i = 0; i < model->nummaterials; i++) {
      GLMmaterial *mat = &model->materials[i];
      _glmLoadTexture(mat);
   }
}


void
glmDrawVBO(GLMmodel *model)
{
   GLMgroup* group;

   assert(model->vbo);

   glBindBufferARB(GL_ARRAY_BUFFER_ARB, model->vbo);

   glVertexPointer(3, GL_FLOAT, model->vertexSize * sizeof(float),
                   (void *) model->posOffset);
   glEnableClientState(GL_VERTEX_ARRAY);

   if (model->numnormals > 0) {
      glNormalPointer(GL_FLOAT, model->vertexSize * sizeof(float),
                      (void *) model->normOffset);
      glEnableClientState(GL_NORMAL_ARRAY);
   }

   if (model->numtexcoords > 0) {
      glTexCoordPointer(2, GL_FLOAT, model->vertexSize * sizeof(float),
                        (void *) model->texOffset);
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   }

   glPushMatrix();
   glTranslatef(model->position[0], model->position[1], model->position[2]);
   glScalef(model->scale, model->scale, model->scale);

   for (group = model->groups; group; group = group->next) {
      if (group->numtriangles > 0) {

         glmShaderMaterial(&model->materials[group->material]);

         glDrawRangeElements(GL_TRIANGLES,
                             group->minIndex, group->maxIndex,
                             3 * group->numtriangles,
                             GL_UNSIGNED_INT, group->triIndexes);
      }
   }

   glPopMatrix();

   glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
   glDisableClientState(GL_VERTEX_ARRAY);
   glDisableClientState(GL_NORMAL_ARRAY);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}



/* glmList: Generates and returns a display list for the model using
 * the mode specified.
 *
 * model    - initialized GLMmodel structure
 * mode     - a bitwise OR of values describing what is to be rendered.
 *            GLM_NONE     -  render with only vertices
 *            GLM_FLAT     -  render with facet normals
 *            GLM_SMOOTH   -  render with vertex normals
 *            GLM_TEXTURE  -  render with texture coords
 *            GLM_COLOR    -  render with colors (color material)
 *            GLM_MATERIAL -  render with materials
 *            GLM_COLOR and GLM_MATERIAL should not both be specified.  
 *            GLM_FLAT and GLM_SMOOTH should not both be specified.  
 */
GLuint
glmList(GLMmodel* model, GLuint mode)
{
  GLuint list;

  list = glGenLists(1);
  glNewList(list, GL_COMPILE);
  glmDraw(model, mode);
  glEndList();

  return list;
}



static const char *VertexShader =
   "varying vec3 normal; \n"
   "void main() { \n"
   "   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex; \n"
   "   normal = gl_NormalMatrix * gl_Normal; \n"
   "   gl_TexCoord[0] = gl_MultiTexCoord0; \n"
   "} \n";

/**
 * Two %s substitutions:
 *   diffuse texture? true/false
 *   specular texture? true/false
 */
static const char *TexFragmentShader =
   "uniform vec4 ambient, diffuse, specular; \n"
   "uniform vec4 ambientLight, diffuseLight, specularLight; \n"
   "uniform float shininess; \n"
   "uniform sampler2D diffTex; \n"
   "uniform samplerCube specTex; \n"
   "varying vec3 normal; \n"
   "\n"
   "void main() \n"
   "{ \n"
   "   vec4 diffTerm, specTerm; \n"
   "   float dotProd = max(dot(gl_LightSource[0].position.xyz, \n"
   "                           normalize(normal)), 0.0);\n"
   "   float dotProd2 = max(dot(-gl_LightSource[0].position.xyz, \n"
   "                           normalize(normal)), 0.0);\n"
   "   dotProd += dotProd2; \n"
   " \n"
   "   diffTerm = diffuse * diffuseLight * dotProd; \n"
   "   if (%s) \n"
   "      diffTerm *= texture2D(diffTex, gl_TexCoord[0].st); \n"
   " \n"
   "   specTerm = specular * specularLight * pow(dotProd, shininess); \n"
   "   if (%s) \n"
   "      specTerm *= textureCube(specTex, normal); \n"
   " \n"
   "   gl_FragColor = ambient * ambientLight + diffTerm + specTerm; \n"
   "} \n";


void
glmShaderMaterial(GLMmaterial *mat)
{
   static const float ambientLight[4] = { 0.1, 0.1, 0.1, 0.0 };
   static const float diffuseLight[4] = { 0.75, 0.75, 0.75, 1.0 };
   static const float specularLight[4] = { 1.0, 1.0, 1.0, 0.0 };

   if (!mat->prog) {
      /* make shader now */
      char newShader[10000];
      GLuint vs, fs;
      const char *diffuseTex = mat->texture_kd ? "true" : "false";
      const char *specularTex = mat->texture_ks ? "true" : "false";
      GLint uAmbientLight, uDiffuseLight, uSpecularLight;

      /* replace %d with 0 or 1 */
      sprintf(newShader, TexFragmentShader, diffuseTex, specularTex);
      if (0)
         printf("===== new shader =====\n%s\n============\n", newShader);

      vs = CompileShaderText(GL_VERTEX_SHADER, VertexShader);
      fs = CompileShaderText(GL_FRAGMENT_SHADER, newShader);
      mat->prog = LinkShaders(vs, fs);
      assert(mat->prog);

      glUseProgram(mat->prog);

      mat->uAmbient = glGetUniformLocation(mat->prog, "ambient");
      mat->uDiffuse = glGetUniformLocation(mat->prog, "diffuse");
      mat->uSpecular = glGetUniformLocation(mat->prog, "specular");
      mat->uShininess = glGetUniformLocation(mat->prog, "shininess");
      mat->uDiffTex = glGetUniformLocation(mat->prog, "diffTex");
      mat->uSpecTex = glGetUniformLocation(mat->prog, "specTex");

      uAmbientLight = glGetUniformLocation(mat->prog, "ambientLight");
      uDiffuseLight = glGetUniformLocation(mat->prog, "diffuseLight");
      uSpecularLight = glGetUniformLocation(mat->prog, "specularLight");

      glUniform4fv(mat->uAmbient, 1, mat->ambient);
      glUniform4fv(mat->uDiffuse, 1, mat->diffuse);
      glUniform4fv(mat->uSpecular, 1, mat->specular);
      glUniform1f(mat->uShininess, mat->shininess);
      glUniform1i(mat->uDiffTex, 0);
      glUniform1i(mat->uSpecTex, 1);

      glUniform4fv(uAmbientLight, 1, ambientLight);
      glUniform4fv(uDiffuseLight, 1, diffuseLight);
      glUniform4fv(uSpecularLight, 1, specularLight);
   }

   glActiveTexture(GL_TEXTURE1);
   if (mat->texture_ks)
      glBindTexture(GL_TEXTURE_CUBE_MAP, mat->texture_ks);
   else
      glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

   glActiveTexture(GL_TEXTURE0);
   if (mat->texture_kd)
      glBindTexture(GL_TEXTURE_2D, mat->texture_kd);
   else
      glBindTexture(GL_TEXTURE_2D, 0);

   if (mat->diffuse[3] < 1.0) {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   }
   else {
      glDisable(GL_BLEND);
   }

   glUseProgram(mat->prog);
}


void
glmSpecularTexture(GLMmodel *model, uint cubeTex)
{
   uint i;

   for (i = 0; i < model->nummaterials; i++) {
      model->materials[i].texture_ks = cubeTex;
   }
}
