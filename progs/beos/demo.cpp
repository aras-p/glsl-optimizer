// $Id: demo.cpp,v 1.1 1999/08/19 00:55:40 jtg Exp $

// Simple BeOS GLView demo
// Written by Brian Paul
// This file is in the public domain.



#include <stdio.h>
#include <Application.h>
#include <Window.h>
#include <GLView.h>


class MyWindow : public BWindow
{
public:
   MyWindow(BRect frame);
   virtual bool QuitRequested();
};


MyWindow::MyWindow(BRect frame)
   : BWindow(frame, "demo", B_TITLED_WINDOW, B_NOT_ZOOMABLE)
{
   // no-op
}

bool MyWindow::QuitRequested()
{
   be_app->PostMessage(B_QUIT_REQUESTED);
   return true;
}


class MyGL : public BGLView
{
public:
   MyGL(BRect rect, char *name, ulong options);

//	virtual void AttachedToWindow();
   virtual void Draw(BRect updateRect);
   virtual void Pulse();
   virtual void FrameResized(float w, float h);
private:
   float mAngle;
};


MyGL::MyGL(BRect rect, char *name, ulong options)
   : BGLView(rect, name, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP_BOTTOM, 0, options)
{
   mAngle = 0.0;
}


#if 0
void MyGL::AttachedToWindow()
{
   BGLView::AttachedToWindow();
   LockGL();
   glClearColor(.7, .7, 0, 0);
   UnlockGL();
}
#endif


void MyGL::FrameResized(float w, float h)
{
    BGLView::FrameResized(w, h);

    printf("FrameResized\n");
    LockGL();
    BGLView::FrameResized(w,h);
    glViewport(0, 0, (int) (w + 1), (int) (h + 1));
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1, 1, -1, 1, 10, 30);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, 0, -18);
    UnlockGL();
}



void MyGL::Draw(BRect r)
{
    printf("MyGL::Draw\n");
    BGLView::Draw(r);
    LockGL();
    glClear(GL_COLOR_BUFFER_BIT);
    glPushMatrix();
    glRotatef(mAngle, 0, 0, 1);
    glColor3f(0, 0, 1);
    glBegin(GL_POLYGON);
    glVertex2f(-1, -1);
    glVertex2f( 1, -1);
    glVertex2f( 1,  1);
    glVertex2f(-1,  1);
    glEnd();
    SwapBuffers();
    UnlockGL();
}


void MyGL::Pulse()
{
   printf("pulse\n");
   BGLView::Pulse();
   mAngle += 1.0;

   LockGL();
   glClear(GL_COLOR_BUFFER_BIT);
   glPushMatrix();
   glRotatef(mAngle, 0, 0, 1);
   glColor3f(0, 0, 1);
   glBegin(GL_POLYGON);
   glVertex2f(-1, -1);
   glVertex2f( 1, -1);
   glVertex2f( 1,  1);
   glVertex2f(-1,  1);
   glEnd();
   SwapBuffers();
   UnlockGL();
}



int main(int argc, char *argv[])
{
   BApplication *app = new BApplication("application/demo");

   // make top-level window
   int x = 500, y = 500;
   int w = 400, h = 400;
   MyWindow *win = new MyWindow(BRect(x, y, x + w, y + h));
   //	win->Lock();
   //	win->Unlock();
   win->Show();

   // Make OpenGL view and put it in the window
   MyGL *gl = new MyGL(BRect(5, 5, w-10, h-10), "GL", BGL_RGB | BGL_DOUBLE);
   //	MyGL *gl = new MyGL(BRect(5, 5, w-10, h-10), "GL", BGL_RGB );
   win->AddChild(gl);

   printf("calling app->Run\n");	
   app->Run();

   delete app;

   return 0;
}
