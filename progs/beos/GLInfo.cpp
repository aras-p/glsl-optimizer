// Small app to display GL infos

#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Window.h>
#include <OutlineListView.h>
#include <ScrollView.h>
#include <GLView.h>

#include <String.h>

#include <GL/gl.h>


class GLInfoWindow : public BWindow 
{
public:
   GLInfoWindow(BRect frame);
   virtual bool   QuitRequested() { be_app->PostMessage(B_QUIT_REQUESTED); return true; }
   
private:
	BGLView 			*gl;
	BOutlineListView	*list;
	BScrollView			*scroller;
};


class GLInfoApp : public BApplication
{
public:
   GLInfoApp();
private:
   GLInfoWindow      *window;
};


GLInfoApp::GLInfoApp()
   : BApplication("application/x-vnd.OBOS-GLInfo")
{
   window = new GLInfoWindow(BRect(50, 50, 350, 350));
}

GLInfoWindow::GLInfoWindow(BRect frame)
   : BWindow(frame, "OpenGL Info", B_TITLED_WINDOW, 0)
{
	BRect	r = Bounds();
	char *s;
	BString l;

	// Add a outline list view
	r.right -= B_V_SCROLL_BAR_WIDTH;
	list 		= new BOutlineListView(r, "GLInfoList", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL_SIDES);
	scroller 	= new BScrollView("GLInfoListScroller", list, B_FOLLOW_ALL_SIDES,
						B_WILL_DRAW | B_FRAME_EVENTS, false, true);
						
	gl = new BGLView(r, "opengl", B_FOLLOW_ALL_SIDES, 0, BGL_RGB | BGL_DOUBLE);
	gl->Hide();
	AddChild(gl);
	AddChild(scroller);
	
	Show();
	
	LockLooper();
	
	// gl->LockGL();
	
	s = (char *) glGetString(GL_RENDERER);
	if (!s)
		goto error;
		
	list->AddItem(new BStringItem(s));

	s = (char *) glGetString(GL_VENDOR);
	if (s) {
		l = ""; l << "Vendor Name: " << s;
		list->AddItem(new BStringItem(l.String(), 1));
	}

	s = (char *) glGetString(GL_VERSION);
	if (s) {
		l = ""; l << "Version: " << s;
		list->AddItem(new BStringItem(l.String(), 1));
	}
	
	s = (char *) glGetString(GL_RENDERER);
	if (s) {
		l = ""; l << "Renderer Name: " << s;
		list->AddItem(new BStringItem(l.String(), 1));
	}

	s = (char *) glGetString(GL_EXTENSIONS);
	if (s) {
		list->AddItem(new BStringItem("OpenGL Extensions", 1));
		while (*s) {
			char extname[255];
			int n = strcspn(s, " ");
			strncpy(extname, s, n);
			extname[n] = 0;
			list->AddItem(new BStringItem(extname, 2));
			if (! s[n])
				break;
			s += (n + 1);	// next !
		}
	}

	// gl->UnlockGL();

error:
	UnlockLooper();
}



int main(int argc, char *argv[])
{
   GLInfoApp *app = new GLInfoApp;
   app->Run();
   delete app;
   return 0;
}
