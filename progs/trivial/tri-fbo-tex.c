/* Framebuffer object test */


#include <GL/glew.h>
#include <GL/glut.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* For debug */


static int Win = 0;
static int Width = 512, Height = 512;

static GLenum TexTarget = GL_TEXTURE_2D;
static int TexWidth = 512, TexHeight = 512;

static GLuint MyFB;
static GLuint TexObj;
static GLboolean Anim = GL_FALSE;
static GLfloat Rot = 0.0;
static GLuint TextureLevel = 0;  /* which texture level to render to */
static GLenum TexIntFormat = GL_RGB; /* either GL_RGB or GL_RGBA */


static void
CheckError(int line)
{
   GLenum err = glGetError();
   if (err) {
      printf("GL Error 0x%x at line %d\n", (int) err, line);
   }
}


static void
Idle(void)
{
   Rot = glutGet(GLUT_ELAPSED_TIME) * 0.1;
   glutPostRedisplay();
}


static void
RenderTexture(void)
{
   GLenum status;

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-1.0, 1.0, -1.0, 1.0, 5.0, 25.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -15.0);

   if (1) {
      /* draw to texture image */
      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, MyFB);

      status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
      if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
	 printf("Framebuffer incomplete!!!\n");
      }

      glViewport(0, 0, TexWidth, TexHeight);

      glClearColor(0.5, 0.5, 1.0, 0.0);
      glClear(GL_COLOR_BUFFER_BIT);
      
      CheckError(__LINE__);

      glBegin(GL_POLYGON);
      glColor3f(1, 0, 0);
      glVertex2f(-1, -1);
      glColor3f(0, 1, 0);
      glVertex2f(1, -1);
      glColor3f(0, 0, 1);
      glVertex2f(0, 1);
      glEnd();

      /* Bind normal framebuffer */
      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
   }
   else {
   }

   CheckError(__LINE__);
}



static void
Display(void)
{
   float ar = (float) Width / (float) Height;

   RenderTexture();
   
   /* draw textured quad in the window */
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-ar, ar, -1.0, 1.0, 5.0, 25.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -7.0);

   glViewport(0, 0, Width, Height);

   glClearColor(0.25, 0.25, 0.25, 0);
   glClear(GL_COLOR_BUFFER_BIT);

   glPushMatrix();
   glRotatef(Rot, 0, 1, 0);
   glEnable(TexTarget);
   glBindTexture(TexTarget, TexObj);

   {
      glBegin(GL_POLYGON);
      glColor3f(0.25, 0.25, 0.25);
      glTexCoord2f(0, 0);
      glVertex2f(-1, -1);
      glTexCoord2f(1, 0);
      glVertex2f(1, -1);
      glColor3f(1.0, 1.0, 1.0);
      glTexCoord2f(1, 1);
      glVertex2f(1, 1);
      glTexCoord2f(0, 1);
      glVertex2f(-1, 1);
      glEnd();
   }

   glPopMatrix();
   glDisable(TexTarget);

   glutSwapBuffers();
   CheckError(__LINE__);
}


static void
Reshape(int width, int height)
{
   glViewport(0, 0, width, height);
   Width = width;
   Height = height;
}


static void
CleanUp(void)
{
   glDeleteFramebuffersEXT(1, &MyFB);

   glDeleteTextures(1, &TexObj);

   glutDestroyWindow(Win);

   exit(0);
}


static void
Key(unsigned char key, int x, int y)
{
   (void) x;
   (void) y;
   switch (key) {
      case 'a':
         Anim = !Anim;
         if (Anim)
            glutIdleFunc(Idle);
         else
            glutIdleFunc(NULL);
         break;
      case 's':
         Rot += 2.0;
         break;
      case 27:
         CleanUp();
         break;
   }
   glutPostRedisplay();
}


static void
Init(int argc, char *argv[])
{
   if (!glutExtensionSupported("GL_EXT_framebuffer_object")) {
      printf("GL_EXT_framebuffer_object not found!\n");
      exit(0);
   }

   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));


   /* Make texture object/image */
   glGenTextures(1, &TexObj);
   glBindTexture(TexTarget, TexObj);
   glTexParameteri(TexTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(TexTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(TexTarget, GL_TEXTURE_BASE_LEVEL, TextureLevel);
   glTexParameteri(TexTarget, GL_TEXTURE_MAX_LEVEL, TextureLevel);

   glTexImage2D(TexTarget, 0, TexIntFormat, TexWidth, TexHeight, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, NULL);


   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);




   /* gen framebuffer id, delete it, do some assertions, just for testing */
   glGenFramebuffersEXT(1, &MyFB);
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, MyFB);
   assert(glIsFramebufferEXT(MyFB));


   CheckError(__LINE__);

   /* Render color to texture */
   glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                             TexTarget, TexObj, TextureLevel);



   CheckError(__LINE__);

   /* bind regular framebuffer */
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);


}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(Width, Height);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
   Win = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Display);
   if (Anim)
      glutIdleFunc(Idle);
   Init(argc, argv);
   glutMainLoop();
   return 0;
}
