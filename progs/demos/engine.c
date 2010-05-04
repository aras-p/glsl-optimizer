/**
 * Simple engine demo (crankshaft, pistons, connecting rods)
 *
 * Brian Paul
 * June 2006
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include "readtex.h"
#include "trackball.h"


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEG_TO_RAD(DEG)  ((DEG) * M_PI / 180.0)

#define TEXTURE_FILE "../images/reflect.rgb"

/* Target engine speed: */
const int RPM = 100.0;

static int Win = 0;


/**
 * Engine description.
 */
typedef struct
{
   const char *Name;
   int Pistons;
   int Cranks;
   float V_Angle;
   float PistonRadius;
   float PistonHeight;
   float WristPinRadius;
   float Throw;
   float CrankPlateThickness;
   float CrankPinRadius;
   float CrankJournalRadius;
   float CrankJournalLength;
   float ConnectingRodLength;
   float ConnectingRodThickness;
   /* display list IDs */
   GLuint CrankList;
   GLuint ConnRodList;
   GLuint PistonList;
   GLuint BlockList;
} Engine;


typedef struct
{
   float CurQuat[4];
   float Distance;
   /* When mouse is moving: */
   GLboolean Rotating, Translating;
   GLint StartX, StartY;
   float StartDistance;
} ViewInfo;


typedef enum
{
   LIT,
   WIREFRAME,
   TEXTURED
} RenderMode;


typedef struct
{
   RenderMode Mode;
   GLboolean Anim;
   GLboolean Wireframe;
   GLboolean Blend;
   GLboolean Antialias;
   GLboolean Texture;
   GLboolean UseLists;
   GLboolean DrawBox;
   GLboolean ShowInfo;
   GLboolean ShowBlock;
} RenderInfo;


static GLUquadric *Q;

static GLfloat Theta = 0.0;

static const GLfloat PistonColor[4] = { 1.0, 0.5, 0.5, 1.0 };
static const GLfloat ConnRodColor[4] = { 0.7, 1.0, 0.7, 1.0 };
static const GLfloat CrankshaftColor[4] = { 0.7, 0.7, 1.0, 1.0 };
static const GLfloat BlockColor[4] = {0.8, 0.8, 0.8, 0.75 };

static GLuint TextureObj;
static GLint WinWidth = 800, WinHeight = 500;

static ViewInfo View;
static RenderInfo Render;

#define NUM_ENGINES 3
static Engine Engines[NUM_ENGINES] =
{
   {
      "V-6",
      6,    /* Pistons */
      3,    /* Cranks */
      90.0, /* V_Angle */
      0.5,  /* PistonRadius */
      0.6,  /* PistonHeight */
      0.1,  /* WristPinRadius */
      0.5,  /* Throw */
      0.2,  /* CrankPlateThickness */
      0.25, /* CrankPinRadius */
      0.3,  /* CrankJournalRadius */
      0.4,  /* CrankJournalLength */
      1.5,  /* ConnectingRodLength */
      0.1,  /* ConnectingRodThickness */
      0,    /* CrankList */
      0,    /* ConnRodList */
      0,    /* PistonList */
      0     /* BlockList */
   },
   {
      "Inline-4",
      4,    /* Pistons */
      4,    /* Cranks */
      0.0,  /* V_Angle */
      0.5,  /* PistonRadius */
      0.6,  /* PistonHeight */
      0.1,  /* WristPinRadius */
      0.5,  /* Throw */
      0.2,  /* CrankPlateThickness */
      0.25, /* CrankPinRadius */
      0.3,  /* CrankJournalRadius */
      0.4,  /* CrankJournalLength */
      1.5,  /* ConnectingRodLength */
      0.1,  /* ConnectingRodThickness */
      0,    /* CrankList */
      0,    /* ConnRodList */
      0,    /* PistonList */
      0     /* BlockList */
   },
   {
      "Boxer-6",
      6,    /* Pistons */
      3,    /* Cranks */
      180.0,/* V_Angle */
      0.5,  /* PistonRadius */
      0.6,  /* PistonHeight */
      0.1,  /* WristPinRadius */
      0.5,  /* Throw */
      0.2,  /* CrankPlateThickness */
      0.25, /* CrankPinRadius */
      0.3,  /* CrankJournalRadius */
      0.4,  /* CrankJournalLength */
      1.5,  /* ConnectingRodLength */
      0.1,  /* ConnectingRodThickness */
      0,    /* CrankList */
      0,    /* ConnRodList */
      0,    /* PistonList */
      0     /* BlockList */
   }
};

static int CurEngine = 0;



static void
InitViewInfo(ViewInfo *view)
{
   view->Rotating = GL_FALSE;
   view->Translating = GL_FALSE;
   view->StartX = view->StartY = 0;
   view->Distance = 12.0;
   view->StartDistance = 0.0;
   view->CurQuat[0] = -0.194143;
   view->CurQuat[1] = 0.507848;
   view->CurQuat[2] = 0.115245;
   view->CurQuat[3] = 0.831335;
}


static void
InitRenderInfo(RenderInfo *render)
{
   render->Mode = LIT;
   render->Anim = GL_TRUE;
   render->Wireframe = GL_FALSE;
   render->Blend = GL_FALSE;
   render->Antialias = GL_FALSE;
   render->Texture = GL_FALSE;
   render->DrawBox = GL_FALSE;
   render->ShowInfo = GL_TRUE;
   render->ShowBlock = GL_FALSE;
   render->UseLists = GL_FALSE;
}


/**
 * Set GL for given rendering mode.
 */
static void
SetRenderState(RenderMode mode)
{
   static const GLfloat gray2[4] = { 0.2, 0.2, 0.2, 1.0 };
   static const GLfloat gray4[4] = { 0.4, 0.4, 0.4, 1.0 };

   /* defaults */
   glDisable(GL_LIGHTING);
   glDisable(GL_TEXTURE_2D);
   glDisable(GL_BLEND);
   glDisable(GL_LINE_SMOOTH);
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
   glDisable(GL_TEXTURE_GEN_S);
   glDisable(GL_TEXTURE_GEN_T);
   glLightModelfv(GL_LIGHT_MODEL_AMBIENT, gray2);

   switch (mode) {
   case LIT:
      glEnable(GL_LIGHTING);
      break;
   case WIREFRAME:
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glEnable(GL_LINE_SMOOTH);
      glEnable(GL_BLEND);
      glLineWidth(1.5);
      break;
   case TEXTURED:
      glEnable(GL_LIGHTING);
      glEnable(GL_TEXTURE_2D);
      glEnable(GL_TEXTURE_GEN_S);
      glEnable(GL_TEXTURE_GEN_T);
      glLightModelfv(GL_LIGHT_MODEL_AMBIENT, gray4);
      break;
   default:
      ;
   }
}


/**
 * Animate the engine parts.
 */
static void
Idle(void)
{
   /* convert degrees per millisecond to RPM: */
   const float m = 360.0 / 1000.0 / 60.0;
   GLint t = glutGet(GLUT_ELAPSED_TIME);
   Theta = ((int) (t * RPM * m)) % 360;
   glutPostRedisplay();
}


/**
 * Compute piston's position along its stroke.
 */
static float
PistonStrokePosition(float throwDist, float crankAngle, float connRodLength)
{
   float x = throwDist * cos(DEG_TO_RAD(crankAngle));
   float y = throwDist * sin(DEG_TO_RAD(crankAngle));
   float pos = y + sqrt(connRodLength * connRodLength - x * x);
   return pos;
}


/**
 * Compute position of nth piston along the crankshaft.
 */
static float
PistonShaftPosition(const Engine *eng, int piston)
{
   const int i = piston / (eng->Pistons / eng->Cranks);
   float z;
   assert(piston < eng->Pistons);
   z = 1.5 * eng->CrankJournalLength + eng->CrankPlateThickness
      + i * (2.0 * (eng->CrankJournalLength + eng->CrankPlateThickness));
   if (eng->Pistons > eng->Cranks) {
      if (piston & 1)
         z += eng->ConnectingRodThickness;
      else
         z -= eng->ConnectingRodThickness;
   }
   return z;
}


/**
 * Compute distance between two adjacent pistons
 */
static float
PistonSpacing(const Engine *eng)
{
   const int pistonsPerCrank = eng->Pistons / eng->Cranks;
   const float z0 = PistonShaftPosition(eng, 0);
   const float z1 = PistonShaftPosition(eng, pistonsPerCrank);
   return z1 - z0;
}


/**
 * (x0, y0) = position of big end on crankshaft
 * (x1, y1) = position of small end on piston
 */
static void
ComputeConnectingRodPosition(float throwDist, float crankAngle,
                             float connRodLength,
                             float *x0, float *y0, float *x1, float *y1)
{
   *x0 = throwDist * cos(DEG_TO_RAD(crankAngle));
   *y0 = throwDist * sin(DEG_TO_RAD(crankAngle));
   *x1 = 0.0;
   *y1 = PistonStrokePosition(throwDist, crankAngle, connRodLength);
}


/**
 * Compute total length of the crankshaft.
 */
static float
CrankshaftLength(const Engine *eng)
{
   float len = (eng->Cranks * 2 + 1) * eng->CrankJournalLength
             + 2 * eng->Cranks * eng->CrankPlateThickness;
   return len;
}


/**
 * Draw a piston.
 * Axis of piston = Z axis.  Wrist pin is centered on (0, 0, 0).
 */
static void
DrawPiston(const Engine *eng)
{
   const int slices = 30, stacks = 4, loops = 4;
   const float innerRadius = 0.9 * eng->PistonRadius;
   const float innerHeight = eng->PistonHeight - 0.15;
   const float wristPinLength = 1.8 * eng->PistonRadius;

   assert(Q);

   glPushMatrix();
   glTranslatef(0, 0, -1.1 * eng->WristPinRadius);

   gluQuadricOrientation(Q, GLU_INSIDE);

   /* bottom rim */
   gluDisk(Q, innerRadius, eng->PistonRadius, slices, 1/*loops*/);

   /* inner cylinder */
   gluCylinder(Q, innerRadius, innerRadius, innerHeight, slices, stacks);

   /* inside top */
   glPushMatrix();
   glTranslatef(0, 0, innerHeight);
   gluDisk(Q, 0, innerRadius, slices, loops);
   glPopMatrix();

   gluQuadricOrientation(Q, GLU_OUTSIDE);

   /* outer cylinder */
   gluCylinder(Q, eng->PistonRadius, eng->PistonRadius, eng->PistonHeight,
               slices, stacks);

   /* top */
   glTranslatef(0, 0, eng->PistonHeight);
   gluDisk(Q, 0, eng->PistonRadius, slices, loops);

   glPopMatrix();

   /* wrist pin */
   glPushMatrix();
   glTranslatef(0, 0.5 * wristPinLength, 0.0);
   glRotatef(90, 1, 0, 0);
   gluCylinder(Q, eng->WristPinRadius, eng->WristPinRadius, wristPinLength,
               slices, stacks);
   glPopMatrix();
}


/**
 * Draw piston at particular position.
 */
static void
DrawPositionedPiston(const Engine *eng, float crankAngle)
{
   const float pos = PistonStrokePosition(eng->Throw, crankAngle,
                                          eng->ConnectingRodLength);
   glPushMatrix();
      glRotatef(-90, 1, 0, 0);
      glTranslatef(0, 0, pos);
      if (eng->PistonList)
         glCallList(eng->PistonList);
      else
         DrawPiston(eng);
   glPopMatrix();
}


/**
 * Draw connector plate.  Used for crankshaft and connecting rods.
 */
static void
DrawConnector(float length, float thickness,
              float bigEndRadius, float smallEndRadius)
{
   const float bigRadius = 1.2 * bigEndRadius;
   const float smallRadius = 1.2 * smallEndRadius;
   const float z0 = -0.5 * thickness, z1 = -z0;
   GLfloat points[36][2], normals[36][2];
   int i;

   /* compute vertex locations, normals */
   for (i = 0; i < 36; i++) {
      const int angle = i * 10;
      float x = cos(DEG_TO_RAD(angle));
      float y = sin(DEG_TO_RAD(angle));
      normals[i][0] = x;
      normals[i][1] = y;
      if (angle >= 0 && angle <= 180) {
         x *= smallRadius;
         y = y * smallRadius + length;
      }
      else {
         x *= bigRadius;
         y *= bigRadius;
      }
      points[i][0] = x;
      points[i][1] = y;
   }

   /* front face */
   glNormal3f(0, 0, 1);
   glBegin(GL_POLYGON);
   for (i = 0; i < 36; i++) {
      glVertex3f(points[i][0], points[i][1], z1);
   }
   glEnd();

   /* back face */
   glNormal3f(0, 0, -1);
   glBegin(GL_POLYGON);
   for (i = 0; i < 36; i++) {
      glVertex3f(points[35-i][0], points[35-i][1], z0);
   }
   glEnd();

   /* edge */
   glBegin(GL_QUAD_STRIP);
   for (i = 0; i <= 36; i++) {
      const int j = i % 36;
      glNormal3f(normals[j][0], normals[j][1], 0);
      glVertex3f(points[j][0], points[j][1], z1);
      glVertex3f(points[j][0], points[j][1], z0);
   }
   glEnd();
}


/**
 * Draw a crankshaft.  Shaft lies along +Z axis, starting at zero.
 */
static void
DrawCrankshaft(const Engine *eng)
{
   const int slices = 20, stacks = 2;
   const int n = eng->Cranks * 4 + 1;
   const float phiStep = 360 / eng->Cranks;
   float phi = -90.0;
   int i;
   float z = 0.0;

   for (i = 0; i < n; i++) {
      glPushMatrix();
      glTranslatef(0, 0, z);
      if (i & 1) {
         /* draw a crank plate */
         glRotatef(phi, 0, 0, 1);
         glTranslatef(0, 0, 0.5 * eng->CrankPlateThickness);
         DrawConnector(eng->Throw, eng->CrankPlateThickness,
                       eng->CrankJournalRadius, eng->CrankPinRadius);
         z += 0.2;
         if (i % 4 == 3)
            phi += phiStep;
      }
      else if (i % 4 == 0) {
         /* draw crank journal segment */
         gluCylinder(Q, eng->CrankJournalRadius, eng->CrankJournalRadius,
                     eng->CrankJournalLength, slices, stacks);
         z += eng->CrankJournalLength;
      }
      else if (i % 4 == 2) {
         /* draw crank pin segment */
         glRotatef(phi, 0, 0, 1);
         glTranslatef(0, eng->Throw, 0);
         gluCylinder(Q, eng->CrankPinRadius, eng->CrankPinRadius,
                     eng->CrankJournalLength, slices, stacks);
         z += eng->CrankJournalLength;
      }
      glPopMatrix();
   }
}


/**
 * Draw crankshaft at a particular rotation.
 * \param crankAngle  current crankshaft rotation, in radians
 */
static void
DrawPositionedCrankshaft(const Engine *eng, float crankAngle)
{
   glPushMatrix();
      glRotatef(crankAngle, 0, 0, 1);
      if (eng->CrankList)
         glCallList(eng->CrankList);
      else
         DrawCrankshaft(eng);
   glPopMatrix();
}


/**
 * Draw a connecting rod at particular position.
 * \param eng  description of connecting rod to draw
 * \param crankAngle  current crankshaft rotation, in radians
 */
static void
DrawPositionedConnectingRod(const Engine *eng, float crankAngle)
{
   float x0, y0, x1, y1;
   float d, phi;

   ComputeConnectingRodPosition(eng->Throw, crankAngle,
                                eng->ConnectingRodLength,
                                &x0, &y0, &x1, &y1);
   d = sqrt(eng->ConnectingRodLength * eng->ConnectingRodLength - x0 * x0);
   phi = atan(x0 / d) * 180.0 / M_PI;

   glPushMatrix();
      glTranslatef(x0, y0, 0);
      glRotatef(phi, 0, 0, 1);
      if (eng->ConnRodList)
         glCallList(eng->ConnRodList);
      else
         DrawConnector(eng->ConnectingRodLength, eng->ConnectingRodThickness,
                       eng->CrankPinRadius, eng->WristPinRadius);
   glPopMatrix();
}


/**
 * Draw a square with a hole in middle.
 */
static void
SquareWithHole(float squareSize, float holeRadius)
{
   int i;
   glBegin(GL_QUAD_STRIP);
   glNormal3f(0, 0, 1);
   for (i = 0; i <= 360; i += 5) {
      const float x1 = holeRadius * cos(DEG_TO_RAD(i));
      const float y1 = holeRadius * sin(DEG_TO_RAD(i));
      float x2 = 0.0F, y2 = 0.0F;
      if (i > 315 || i <= 45) {
         x2 = squareSize;
         y2 = squareSize * tan(DEG_TO_RAD(i));
      }
      else if (i > 45 && i <= 135) {
         x2 = -squareSize * tan(DEG_TO_RAD(i - 90));
         y2 = squareSize;
      }
      else if (i > 135 && i <= 225) {
         x2 = -squareSize;
         y2 = -squareSize * tan(DEG_TO_RAD(i-180));
      }
      else if (i > 225 && i <= 315) {
         x2 = squareSize * tan(DEG_TO_RAD(i - 270));
         y2 = -squareSize;
      }
      glVertex2f(x1, y1); /* inner circle */
      glVertex2f(x2, y2); /* outer square */
   }
   glEnd();
}


/**
 * Draw block with hole through middle.
 * Hole is centered on Z axis.
 * Bottom of block is at z=0, top of block is at z = blockHeight.
 * index is in [0, count - 1] to determine which block faces are drawn.
 */
static void
DrawBlockWithHole(float blockSize, float blockHeight, float holeRadius,
                  int index, int count)
{
   const int slices = 30, stacks = 4;
   const float x = blockSize;
   const float y = blockSize;
   const float z0 = 0;
   const float z1 = blockHeight;

   assert(index < count);
   assert(Q);
   gluQuadricOrientation(Q, GLU_INSIDE);

   glBegin(GL_QUADS);
   /* +X face */
   glNormal3f(1, 0, 0);
   glVertex3f( x, -y, z0);
   glVertex3f( x, y, z0);
   glVertex3f( x, y, z1);
   glVertex3f( x, -y, z1);
   /* -X face */
   glNormal3f(-1, 0, 0);
   glVertex3f(-x, -y, z1);
   glVertex3f(-x, y, z1);
   glVertex3f(-x, y, z0);
   glVertex3f(-x, -y, z0);
   if (index == 0) {
      /* +Y face */
      glNormal3f(0, 1, 0);
      glVertex3f(-x, y, z1);
      glVertex3f( x, y, z1);
      glVertex3f( x, y, z0);
      glVertex3f(-x, y, z0);
   }
   if (index == count - 1) {
      /* -Y face */
      glNormal3f(0, -1, 0);
      glVertex3f(-x, -y, z0);
      glVertex3f( x, -y, z0);
      glVertex3f( x, -y, z1);
      glVertex3f(-x, -y, z1);
   }
   glEnd();

   /* cylinder / hole */
   gluCylinder(Q, holeRadius, holeRadius, blockHeight, slices, stacks);

   /* face at z0 */
   glPushMatrix();
   glRotatef(180, 1, 0, 0);
   SquareWithHole(blockSize, holeRadius);
   glPopMatrix();

   /* face at z1 */
   glTranslatef(0, 0, z1);
   SquareWithHole(blockSize, holeRadius);

   gluQuadricOrientation(Q, GLU_OUTSIDE);
}


/**
 * Draw the engine block.
 */
static void
DrawEngineBlock(const Engine *eng)
{
   const float blockHeight = eng->Throw + 1.5 * eng->PistonHeight;
   const float cylRadius = 1.01 * eng->PistonRadius;
   const float blockSize = 0.5 * PistonSpacing(eng);
   const int pistonsPerCrank = eng->Pistons / eng->Cranks;
   int i;

   for (i = 0; i < eng->Pistons; i++) {
      const float z = PistonShaftPosition(eng, i);
      const int crank = i / pistonsPerCrank;
      int k;

      glPushMatrix();
         glTranslatef(0, 0, z);

         /* additional rotation for kth piston per crank */
         k = i % pistonsPerCrank;
         glRotatef(k * -eng->V_Angle, 0, 0, 1);

         /* the block */
         glRotatef(-90, 1, 0, 0);
         glTranslatef(0, 0, eng->Throw * 2);
         DrawBlockWithHole(blockSize, blockHeight, cylRadius,
                           crank, eng->Cranks);
      glPopMatrix();
   }
}


/**
 * Generate display lists for engine parts.
 */
static void
GenerateDisplayLists(Engine *eng)
{
   eng->CrankList = glGenLists(1);
   glNewList(eng->CrankList, GL_COMPILE);
   DrawCrankshaft(eng);
   glEndList();

   eng->ConnRodList = glGenLists(1);
   glNewList(eng->ConnRodList, GL_COMPILE);
   DrawConnector(eng->ConnectingRodLength, eng->ConnectingRodThickness,
                 eng->CrankPinRadius, eng->WristPinRadius);
   glEndList();

   eng->PistonList = glGenLists(1);
   glNewList(eng->PistonList, GL_COMPILE);
   DrawPiston(eng);
   glEndList();

   eng->BlockList = glGenLists(1);
   glNewList(eng->BlockList, GL_COMPILE);
   DrawEngineBlock(eng);
   glEndList();
}


/**
 * Free engine display lists (render with immediate mode).
 */
static void
FreeDisplayLists(Engine *eng)
{
   glDeleteLists(eng->CrankList, 1);
   eng->CrankList = 0;
   glDeleteLists(eng->ConnRodList, 1);
   eng->ConnRodList = 0;
   glDeleteLists(eng->PistonList, 1);
   eng->PistonList = 0;
   glDeleteLists(eng->BlockList, 1);
   eng->BlockList = 0;
}


/**
 * Draw complete engine.
 * \param eng  description of engine to draw
 * \param crankAngle  current crankshaft angle, in radians
 */
static void
DrawEngine(const Engine *eng, float crankAngle)
{
   const float crankDelta = 360.0 / eng->Cranks;
   const float crankLen = CrankshaftLength(eng);
   const int pistonsPerCrank = eng->Pistons / eng->Cranks;
   int i;

   glPushMatrix();
   glRotatef(eng->V_Angle * 0.5, 0, 0, 1);
   glTranslatef(0, 0, -0.5 * crankLen);

   /* crankshaft */
   glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, CrankshaftColor);
   glColor4fv(CrankshaftColor);
   DrawPositionedCrankshaft(eng, crankAngle);

   for (i = 0; i < eng->Pistons; i++) {
      const float z = PistonShaftPosition(eng, i);
      const int crank = i / pistonsPerCrank;
      float rot = crankAngle + crank * crankDelta;
      int k;

      glPushMatrix();
         glTranslatef(0, 0, z);

         /* additional rotation for kth piston per crank */
         k = i % pistonsPerCrank;
         glRotatef(k * -eng->V_Angle, 0, 0, 1);
         rot += k * eng->V_Angle;

         /* piston */
         glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, PistonColor);
         glColor4fv(PistonColor);
         DrawPositionedPiston(eng, rot);

         /* connecting rod */
         glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, ConnRodColor);
         glColor4fv(ConnRodColor);
         DrawPositionedConnectingRod(eng, rot);
      glPopMatrix();
   }

   if (Render.ShowBlock) {
      const GLboolean blend = glIsEnabled(GL_BLEND);

      glDepthMask(GL_FALSE);
      if (!blend) {
         glEnable(GL_BLEND);
      }
      glEnable(GL_CULL_FACE);

      glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, BlockColor);
      glColor4fv(BlockColor);
      if (eng->CrankList)
         glCallList(eng->BlockList);
      else
         DrawEngineBlock(eng);

      glDisable(GL_CULL_FACE);
      glDepthMask(GL_TRUE);
      if (!blend) {
         glDisable(GL_BLEND);
      }
   }

   glPopMatrix();
}


static void
DrawBox(void)
{
   const float xmin = -3.0, xmax = 3.0;
   const float ymin = -1.0, ymax = 3.0;
   const float zmin = -4.0, zmax = 4.0;
   const float step = 0.5;
   const float d = 0.01;
   float x, y, z;
   GLboolean lit = glIsEnabled(GL_LIGHTING);
   GLboolean tex = glIsEnabled(GL_TEXTURE_2D);

   glDisable(GL_LIGHTING);
   glDisable(GL_TEXTURE_2D);
   glLineWidth(1.0);

   glColor3f(1, 1, 1);

   /* Z min */
   glBegin(GL_LINES);
   for (x = xmin; x <= xmax; x += step) {
      glVertex3f(x, ymin, zmin);
      glVertex3f(x, ymax, zmin);
   }
   glEnd();
   glBegin(GL_LINES);
   for (y = ymin; y <= ymax; y += step) {
      glVertex3f(xmin, y, zmin);
      glVertex3f(xmax, y, zmin);
   }
   glEnd();

   /* Y min */
   glBegin(GL_LINES);
   for (x = xmin; x <= xmax; x += step) {
      glVertex3f(x, ymin, zmin);
      glVertex3f(x, ymin, zmax);
   }
   glEnd();
   glBegin(GL_LINES);
   for (z = zmin; z <= zmax; z += step) {
      glVertex3f(xmin, ymin, z);
      glVertex3f(xmax, ymin, z);
   }
   glEnd();

   /* X min */
   glBegin(GL_LINES);
   for (y = ymin; y <= ymax; y += step) {
      glVertex3f(xmin, y, zmin);
      glVertex3f(xmin, y, zmax);
   }
   glEnd();
   glBegin(GL_LINES);
   for (z = zmin; z <= zmax; z += step) {
      glVertex3f(xmin, ymin, z);
      glVertex3f(xmin, ymax, z);
   }
   glEnd();

   glColor3f(0.4, 0.4, 0.6);
   glBegin(GL_QUADS);
   /* xmin */
   glVertex3f(xmin-d, ymin, zmin);
   glVertex3f(xmin-d, ymax, zmin);
   glVertex3f(xmin-d, ymax, zmax);
   glVertex3f(xmin-d, ymin, zmax);
   /* ymin */
   glVertex3f(xmin, ymin-d, zmin);
   glVertex3f(xmax, ymin-d, zmin);
   glVertex3f(xmax, ymin-d, zmax);
   glVertex3f(xmin, ymin-d, zmax);
   /* zmin */
   glVertex3f(xmin, ymin, zmin-d);
   glVertex3f(xmax, ymin, zmin-d);
   glVertex3f(xmax, ymax, zmin-d);
   glVertex3f(xmin, ymax, zmin-d);
   glEnd();

   if (lit)
      glEnable(GL_LIGHTING);
   if (tex)
      glEnable(GL_TEXTURE_2D);
}


static void
PrintString(const char *s)
{
   while (*s) {
      glutBitmapCharacter(GLUT_BITMAP_8_BY_13, (int) *s);
      s++;
   }
}


static int
ComputeFPS(void)
{
   static double t0 = -1.0;
   static int frames = 0;
   double t = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
   static int fps = 0;

   frames++;

   if (t0 < 0.0) {
      t0 = t;
      fps = 0;
   }
   else if (t - t0 >= 1.0) {
      fps = (int) (frames / (t - t0) + 0.5);
      t0 = t;
      frames = 0;
   }

   return fps;
}


static void
Draw(void)
{
   int fps;
   GLfloat rot[4][4];

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glPushMatrix();

      glTranslatef(0.0, 0.0, -View.Distance);
      build_rotmatrix(rot, View.CurQuat);
      glMultMatrixf(&rot[0][0]);

      glPushMatrix();
         glTranslatef(0, -0.75, 0);
         if (Render.DrawBox)
            DrawBox();
         DrawEngine(Engines + CurEngine, Theta);
      glPopMatrix();

   glPopMatrix();

   fps = ComputeFPS();
   if (Render.ShowInfo) {
      GLboolean lit = glIsEnabled(GL_LIGHTING);
      GLboolean tex = glIsEnabled(GL_TEXTURE_2D);
      char s[100];
      sprintf(s, "%s  %d FPS  %s", Engines[CurEngine].Name, fps,
              Render.UseLists ? "Display Lists" : "Immediate mode");
      glDisable(GL_LIGHTING);
      glDisable(GL_TEXTURE_2D);
      glColor3f(1, 1 , 1);
      glWindowPos2iARB(10, 10);
      PrintString(s);
      if (lit)
	 glEnable(GL_LIGHTING);
      if (tex)
	 glEnable(GL_TEXTURE_2D);
   }

   /* also print out a periodic fps to stdout.  useful for trying to
    * figure out the performance impact of rendering the string above
    * with glBitmap.
    */
   {
      static GLint T0 = 0;
      static GLint Frames = 0;
      GLint t = glutGet(GLUT_ELAPSED_TIME);
      
      Frames++;
         
      if (t - T0 >= 5000) {
         GLfloat seconds = (t - T0) / 1000.0;
         GLfloat fps = Frames / seconds;
         printf("%d frames in %6.3f seconds = %6.3f FPS\n", Frames, seconds, fps);
         fflush(stdout);
         T0 = t;
         Frames = 0;
      }
   }


   glutSwapBuffers();
}


/**
 * Handle window resize.
 */
static void
Reshape(int width, int height)
{
   float ar = (float) width / height;
   float s = 0.5;
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-ar * s, ar * s, -s, s, 2.0, 50.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   WinWidth = width;
   WinHeight = height;
}


/**
 * Handle mouse button.
 */
static void
Mouse(int button, int state, int x, int y)
{
   if (button == GLUT_LEFT_BUTTON) {
      if (state == GLUT_DOWN) {
         View.StartX = x;
         View.StartY = y;
         View.Rotating = GL_TRUE;
      }
      else if (state == GLUT_UP) {
         View.Rotating = GL_FALSE;
      }
   }
   else if (button == GLUT_MIDDLE_BUTTON) {
      if (state == GLUT_DOWN) {
         View.StartX = x;
         View.StartY = y;
         View.StartDistance = View.Distance;
         View.Translating = GL_TRUE;
      }
      else if (state == GLUT_UP) {
         View.Translating = GL_FALSE;
      }
   }
}


/**
 * Handle mouse motion
 */
static void
Motion(int x, int y)
{
   int i;
   if (View.Rotating) {
      float x0 = (2.0 * View.StartX - WinWidth) / WinWidth;
      float y0 = (WinHeight - 2.0 * View.StartY) / WinHeight;
      float x1 = (2.0 * x - WinWidth) / WinWidth;
      float y1 = (WinHeight - 2.0 * y) / WinHeight;
      float q[4];

      trackball(q, x0, y0, x1, y1);
      View.StartX = x;
      View.StartY = y;
      for (i = 0; i < 1; i++)
         add_quats(q, View.CurQuat, View.CurQuat);

      glutPostRedisplay();
   }
   else if (View.Translating) {
      float dz = 0.01 * (y - View.StartY);
      View.Distance = View.StartDistance + dz;
      glutPostRedisplay();
   }
}


/**
 ** Menu Callbacks
 **/

static void
OptAnimation(void)
{
   Render.Anim = !Render.Anim;
   if (Render.Anim)
      glutIdleFunc(Idle);
   else
      glutIdleFunc(NULL);
}

static void
OptChangeEngine(void)
{
   CurEngine = (CurEngine + 1) % NUM_ENGINES;
}

static void
OptRenderMode(void)
{
   Render.Mode++;
   if (Render.Mode > TEXTURED)
      Render.Mode = 0;
   SetRenderState(Render.Mode);
}

static void
OptDisplayLists(void)
{
   int i;
   Render.UseLists = !Render.UseLists;
   if (Render.UseLists) {
      for (i = 0; i < NUM_ENGINES; i++) {
         GenerateDisplayLists(Engines + i);
      }
   }
   else {
      for (i = 0; i < NUM_ENGINES; i++) {
         FreeDisplayLists(Engines + i);
      }
   }
}

static void
OptShowBlock(void)
{
   Render.ShowBlock = !Render.ShowBlock;
}

static void
OptShowInfo(void)
{
   Render.ShowInfo = !Render.ShowInfo;
}

static void
OptShowBox(void)
{
   Render.DrawBox = !Render.DrawBox;
}

static void
OptRotate(void)
{
   Theta += 5.0;
}

static void
OptExit(void)
{
   glutDestroyWindow(Win);
   exit(0);
}


/**
 * Define menu entries (w/ keyboard shortcuts)
 */

typedef struct
{
   const char *Text;
   const char Key;
   void (*Function)(void);
} MenuInfo;

static const MenuInfo MenuItems[] = {
   { "Animation", 'a', OptAnimation },
   { "Change Engine", 'e', OptChangeEngine },
   { "Rendering Style", 'm', OptRenderMode },
   { "Display Lists", 'd', OptDisplayLists },
   { "Show Block", 'b', OptShowBlock },
   { "Show Info", 'i', OptShowInfo },
   { "Show Box", 'x', OptShowBox },
   { "Exit", 27, OptExit },
   { NULL, 'r', OptRotate },
   { NULL, 0, NULL }
};


/**
 * Handle menu selection.
 */
static void
MenuHandler(int entry)
{
   MenuItems[entry].Function();
   glutPostRedisplay();
}


/**
 * Make pop-up menu.
 */
static void
MakeMenu(void)
{
   int i;
   glutCreateMenu(MenuHandler);
   for (i = 0; MenuItems[i].Text; i++) {
      glutAddMenuEntry(MenuItems[i].Text, i);
   }
   glutAttachMenu(GLUT_RIGHT_BUTTON);
}


/**
 * Handle keyboard event.
 */
static void
Key(unsigned char key, int x, int y)
{
   int i;
   (void) x; (void) y;
   for (i = 0; MenuItems[i].Key; i++) {
      if (MenuItems[i].Key == key) {
         MenuItems[i].Function();
         glutPostRedisplay();
         break;
      }
   }
}


static
void LoadTexture(void)
{
   GLboolean convolve = GL_FALSE;

   glGenTextures(1, &TextureObj);
   glBindTexture(GL_TEXTURE_2D, TextureObj);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
   glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

   if (convolve) {
#define FILTER_SIZE 7
      /* use convolution to blur the texture to simulate a dull finish
       * on the object.
       */
      GLubyte *img;
      GLenum format;
      GLint w, h;
      GLfloat filter[FILTER_SIZE][FILTER_SIZE][4];

      for (h = 0; h < FILTER_SIZE; h++) {
         for (w = 0; w < FILTER_SIZE; w++) {
            const GLfloat k = 1.0 / (FILTER_SIZE * FILTER_SIZE);
            filter[h][w][0] = k;
            filter[h][w][1] = k;
            filter[h][w][2] = k;
            filter[h][w][3] = k;
         }
      }

      glEnable(GL_CONVOLUTION_2D);
      glConvolutionParameteri(GL_CONVOLUTION_2D,
                              GL_CONVOLUTION_BORDER_MODE, GL_CONSTANT_BORDER);
      glConvolutionFilter2D(GL_CONVOLUTION_2D, GL_RGBA,
                            FILTER_SIZE, FILTER_SIZE,
                            GL_RGBA, GL_FLOAT, filter);

      img = LoadRGBImage(TEXTURE_FILE, &w, &h, &format);
      if (!img) {
         printf("Error: couldn't load texture image file %s\n", TEXTURE_FILE);
         exit(1);
      }

      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0,
                   format, GL_UNSIGNED_BYTE, img);
      free(img);
   }
   else {
      if (!LoadRGBMipmaps(TEXTURE_FILE, GL_RGB)) {
         printf("Error: couldn't load texture image file %s\n", TEXTURE_FILE);
         exit(1);
      }
   }
}


static void
Init(void)
{
   const GLfloat lightColor[4] = { 0.7, 0.7, 0.7, 1.0 };
   const GLfloat specular[4] = { 0.8, 0.8, 0.8, 1.0 };
   const GLfloat backColor[4] = { 1, 1, 0, 0 };

   Q = gluNewQuadric();
   gluQuadricNormals(Q, GLU_SMOOTH);

   LoadTexture();

   glClearColor(0.3, 0.3, 0.3, 0.0);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor);
   glMaterialf(GL_FRONT, GL_SHININESS, 40);
   glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
   glEnable(GL_NORMALIZE);

   glMaterialfv(GL_BACK, GL_DIFFUSE, backColor);
#if 0
   glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);
#endif
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   InitViewInfo(&View);
   InitRenderInfo(&Render);
}


int
main(int argc, char *argv[])
{
   glutInitWindowSize(WinWidth, WinHeight);
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   Win = glutCreateWindow("OpenGL Engine Demo");
   glewInit();
   glutReshapeFunc(Reshape);
   glutMouseFunc(Mouse);
   glutMotionFunc(Motion);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Draw);
   MakeMenu();
   Init();
   if (Render.Anim)
      glutIdleFunc(Idle);
   glutMainLoop();
   return 0;
}
