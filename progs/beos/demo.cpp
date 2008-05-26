
// Simple BeOS GLView demo
// Written by Brian Paul
// Changes by Philippe Houdoin
// This file is in the public domain.



#include <stdio.h>
#include <Application.h>
#include <Window.h>
#include <GLView.h>

class MyGL : public BGLView
{
public:
	MyGL(BRect rect, char *name, ulong options);

	virtual void AttachedToWindow();
	virtual void Pulse();
	virtual void FrameResized(float w, float h);

private:
	void Render();
	void Reshape(float w, float h);
	float mAngle;
};


class MyWindow : public BWindow
{
public:
	MyWindow(BRect frame);
	virtual bool QuitRequested();
};


MyWindow::MyWindow(BRect frame)
   : BWindow(frame, "demo", B_TITLED_WINDOW, B_NOT_ZOOMABLE)
{
   // Make OpenGL view and put it in the window
   BRect r = Bounds();
   r.InsetBy(5, 5);
   
   MyGL *gl = new MyGL(r, "GL", BGL_RGB | BGL_DOUBLE);
   AddChild(gl);
   SetPulseRate(1000000 / 30);
}

bool MyWindow::QuitRequested()
{
   be_app->PostMessage(B_QUIT_REQUESTED);
   return true;
}



MyGL::MyGL(BRect rect, char *name, ulong options)
   : BGLView(rect, name, B_FOLLOW_ALL_SIDES, B_PULSE_NEEDED, options)
{
	mAngle = 0.0;
}


void MyGL::AttachedToWindow()
{
	BGLView::AttachedToWindow();

	LockGL();
	glClearColor(0.7, 0.7, 0, 0);
	Reshape(Bounds().Width(), Bounds().Height());
	UnlockGL();
}


void MyGL::FrameResized(float w, float h)
{
	BGLView::FrameResized(w, h);

	LockGL();
	Reshape(w, h);
	UnlockGL();

	Render();
}


void MyGL::Pulse()
{
	mAngle += 1.0;
	Render();
}


void MyGL::Render()
{
    LockGL();

    glClear(GL_COLOR_BUFFER_BIT);
    
    glPushMatrix();

    glRotated(mAngle, 0, 0, 1);
    glColor3f(0, 0, 1);

    glBegin(GL_POLYGON);
    glVertex2f(-1, -1);
    glVertex2f( 1, -1);
    glVertex2f( 1,  1);
    glVertex2f(-1,  1);
    glEnd();

	glPopMatrix();

    SwapBuffers();

    UnlockGL();
}


void MyGL::Reshape(float w, float h)
{
	glViewport(0, 0, (int) (w + 1), (int) (h + 1));
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-1, 1, -1, 1, 10, 30);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, -18);
}


int main(int argc, char *argv[])
{
   BApplication *app = new BApplication("application/demo");

   // make top-level window
   MyWindow *win = new MyWindow(BRect(100, 100, 500, 500));
   win->Show();

    app->Run();

   delete app;

   return 0;
}
