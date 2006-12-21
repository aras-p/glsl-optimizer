/*
 * Mesa 3-D graphics library
 * Version:  6.5
 * Copyright (C) 1995-2006  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Library for glut using mesa fbdev driver
 *
 * Written by Sean D'Epagnier (c) 2006
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <inttypes.h>

#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/kd.h>

#include <linux/keyboard.h>
#include <linux/fb.h>
#include <linux/vt.h>

#include <GL/glut.h>

#include "internal.h"

#define MOUSEDEV "/dev/gpmdata"

#ifdef HAVE_GPM
#include <gpm.h>
int GpmMouse;
#endif

int CurrentVT = 0;
int ConsoleFD = -1;

int KeyboardModifiers;

int MouseX, MouseY;
int NumMouseButtons;

double MouseSpeed = 0;

int KeyRepeatMode = GLUT_KEY_REPEAT_DEFAULT;

int MouseVisible = 0;
int LastMouseTime = 0;

static int OldKDMode = -1;
static int OldMode = KD_TEXT;
static struct vt_mode OldVTMode;
static struct termios OldTermios;

static int KeyboardLedState;

static int MouseFD;

static int kbdpipe[2];

static int LastStdinKeyTime, LastStdinSpecialKey = -1, LastStdinCode = -1;

#define MODIFIER(mod) \
    KeyboardModifiers = release ? KeyboardModifiers & ~mod   \
                                : KeyboardModifiers | mod;

/* signal handler attached to SIGIO on keyboard input, vt
   switching and modifiers is handled in the signal handler
   other keypresses read from a pipe that leaves the handler
   if a program locks up the glut loop, you can still switch
   vts and kill it without Alt-SysRq hack */
static void KeyboardHandler(int sig)
{
   unsigned char code;

   while(read(ConsoleFD, &code, 1) == 1) {
      int release, labelval;
      struct kbentry entry;
      static int lalt; /* only left alt does vt switch */

      release = code & 0x80;
      
      entry.kb_index = code & 0x7F;
      entry.kb_table = 0;
      
      if (ioctl(ConsoleFD, KDGKBENT, &entry) < 0) {
	 sprintf(exiterror, "ioctl(KDGKBENT) failed.\n");
	 exit(0);
      }

      labelval = entry.kb_value;
      
      switch(labelval) {
      case K_SHIFT:
      case K_SHIFTL:
	 MODIFIER(GLUT_ACTIVE_SHIFT);
	 continue;
      case K_CTRL:
	 MODIFIER(GLUT_ACTIVE_CTRL);
	 continue;
      case K_ALT:
	 lalt = !release;
      case K_ALTGR:
	 MODIFIER(GLUT_ACTIVE_ALT);
	 continue;
      }

      if(lalt && !release) {
	 /* VT switch, we must do it */
	 int vt = -1;
	 struct vt_stat st;
	 if(labelval >= K_F1 && labelval <= K_F12)
	    vt = labelval - K_F1 + 1;
	 
	 if(labelval == K_LEFT)
	    if(ioctl(ConsoleFD, VT_GETSTATE, &st) >= 0)
	       vt = st.v_active - 1;
      
	 if(labelval == K_RIGHT)
	    if(ioctl(ConsoleFD, VT_GETSTATE, &st) >= 0)
	       vt = st.v_active + 1;
	 
	 if(vt != -1) {
	    if(Swapping)
	       VTSwitch = vt;
	    else
	       if(ioctl(ConsoleFD, VT_ACTIVATE, vt) < 0)
		  sprintf(exiterror, "Error switching console\n");
	    continue;
	 }
      }
      write(kbdpipe[1], &code, 1);
   }
}

static void LedModifier(int led, int release)
{
   static int releaseflag = K_CAPS | K_NUM | K_HOLD;
   if(release)
      releaseflag |= led;
   else
      if(releaseflag & led) {
	 KeyboardLedState ^= led;
	 releaseflag &= ~led;
      }

   ioctl(ConsoleFD, KDSKBLED, KeyboardLedState);
   ioctl(ConsoleFD, KDSETLED, 0x80);
}

static void HandleKeyPress(unsigned char key, int up)
{
   if(up) {
      if(KeyboardUpFunc)
         KeyboardUpFunc(key, MouseX, MouseY);
   } else
      if(KeyboardFunc)
         KeyboardFunc(key, MouseX, MouseY);
      else
         if(key == 27)
            exit(0);  /* no handler, to provide a way to exit */
}

static void HandleSpecialPress(int key, int up)
{
   if(up) {
      if(SpecialUpFunc)
         SpecialUpFunc(key, MouseX, MouseY);
   } else
      if(SpecialFunc)
         SpecialFunc(key, MouseX, MouseY);
}

static void ReleaseStdinKey(void)
{
   if(LastStdinSpecialKey != -1) {
      HandleSpecialPress(LastStdinSpecialKey, 1);
      LastStdinSpecialKey = -1;
   }
   if(LastStdinCode != -1) {
      HandleKeyPress(LastStdinCode, 1);
      LastStdinCode = -1;
   }
}

#define READKEY read(kbdpipe[0], &code, 1)
static int ReadKey(void)
{
   int release, labelval, labelvalnoshift;
   unsigned char code;
   int specialkey = 0;
   struct kbentry entry; 

   if(READKEY != 1) {
      /* if we are reading from stdin, we detect key releases when the key
         does not repeat after a given timeout */
      if(ConsoleFD == 0 && LastStdinKeyTime + 100 < glutGet(GLUT_ELAPSED_TIME))
         ReleaseStdinKey();
      return 0;
   }

   if(code == 0)
      return 0;

   /* stdin input escape code based */
   if(ConsoleFD == 0) {
      KeyboardModifiers = 0;
   altset:
      if(code == 27 && READKEY == 1) {
         if(code != 91) {
	    KeyboardModifiers |= GLUT_ACTIVE_ALT;
	    goto altset;
	 }
         READKEY;
         switch(code) {
         case 68:
            specialkey = GLUT_KEY_LEFT; break;
         case 65:
            specialkey = GLUT_KEY_UP; break;
         case 67:
            specialkey = GLUT_KEY_RIGHT; break;
         case 66:
            specialkey = GLUT_KEY_DOWN; break;
         case 52:
            specialkey = GLUT_KEY_END; READKEY; break;
         case 53:
            specialkey = GLUT_KEY_PAGE_UP; READKEY; break;
         case 54:
            specialkey = GLUT_KEY_PAGE_DOWN; READKEY; break;
         case 49:
            READKEY;
            if(code == 126)
               specialkey = GLUT_KEY_HOME;
            else {
               specialkey = GLUT_KEY_F1 + code - 50;
               READKEY;
            }
            break;
         case 50:
            READKEY;
            if(code == 126)
               specialkey = GLUT_KEY_INSERT;
            else {
               if(code > '1')
                  code--;
               if(code > '6')
                  code--;
               if(code > '3') {
                  KeyboardModifiers |= GLUT_ACTIVE_SHIFT;
                  code -= 12;
               }
               specialkey = GLUT_KEY_F1 + code - 40;
               READKEY;
            }
            break; 
         case 51:
            READKEY;
            if(code == 126) {
               code = '\b';
               goto stdkey;
            }
            KeyboardModifiers |= GLUT_ACTIVE_SHIFT;
            specialkey = GLUT_KEY_F1 + code - 45;
            READKEY;
            break;
         case 91:
            READKEY;
            specialkey = GLUT_KEY_F1 + code - 65;
            break;
         default:
            return 0;
	 }
      }

      if(specialkey) {
         LastStdinKeyTime = glutGet(GLUT_ELAPSED_TIME);

         if(LastStdinSpecialKey != specialkey) {
            ReleaseStdinKey();
            HandleSpecialPress(specialkey, 0);
            LastStdinSpecialKey = specialkey;
            LastStdinKeyTime += 200; /* initial repeat */
         } else
         if(KeyRepeatMode != GLUT_KEY_REPEAT_OFF)
            HandleSpecialPress(specialkey, 0);
      } else {
	 if(code >= 1 && code <= 26 && code != '\r') {
	    KeyboardModifiers |= GLUT_ACTIVE_CTRL;
	    code += 'a' - 1;
	 }
	 if((code >= 43 && code <= 34) || (code == 60)
	    || (code >= 62 && code <= 90) || (code == 94)
	    || (code == 95)  || (code >= 123 && code <= 126))
	    KeyboardModifiers |= GLUT_ACTIVE_SHIFT;

      stdkey:
         LastStdinKeyTime = glutGet(GLUT_ELAPSED_TIME);
         if(LastStdinCode != code) {
            ReleaseStdinKey();
            HandleKeyPress(code, 0);
            LastStdinCode = code;
            LastStdinKeyTime += 200; /* initial repeat */
         } else
         if(KeyRepeatMode != GLUT_KEY_REPEAT_OFF)
            HandleSpecialPress(code, 0);
      }
      return 1;
   }

   /* linux kbd reading */
   release = code & 0x80;
   code &= 0x7F;

   if(KeyRepeatMode == GLUT_KEY_REPEAT_OFF) {
      static char keystates[128];
      if(release)
	 keystates[code] = 0;
      else {
	 if(keystates[code])
	    return 1;
	 keystates[code] = 1;
      }
   }
	    
   entry.kb_index = code;
   entry.kb_table = 0;

   if (ioctl(ConsoleFD, KDGKBENT, &entry) < 0) {
      sprintf(exiterror, "ioctl(KDGKBENT) failed.\n");
      exit(0);
   }

   labelvalnoshift = entry.kb_value;

   if(KeyboardModifiers & GLUT_ACTIVE_SHIFT)
      entry.kb_table |= K_SHIFTTAB;
	
   if (ioctl(ConsoleFD, KDGKBENT, &entry) < 0) {
      sprintf(exiterror, "ioctl(KDGKBENT) failed.\n");
      exit(0);
   }

   labelval = entry.kb_value;

   switch(labelvalnoshift) {
   case K_CAPS:
      LedModifier(LED_CAP, release);
      return 0;
   case K_NUM:
      LedModifier(LED_NUM, release);
      return 0;
   case K_HOLD: /* scroll lock suspends glut */
      LedModifier(LED_SCR, release);
      while(KeyboardLedState & LED_SCR) {
	 usleep(10000);
	 ReadKey();
      }
      return 0;
   }

   /* we could queue keypresses here */
   if(KeyboardLedState & LED_SCR)
      return 0;

   if(labelvalnoshift >= K_F1 && labelvalnoshift <= K_F12)
      specialkey = GLUT_KEY_F1 + labelvalnoshift - K_F1;
   else
      switch(labelvalnoshift) {
      case K_LEFT:
	 specialkey = GLUT_KEY_LEFT; break;
      case K_UP:
	 specialkey = GLUT_KEY_UP; break;
      case K_RIGHT:
	 specialkey = GLUT_KEY_RIGHT; break;
      case K_DOWN:
	 specialkey = GLUT_KEY_DOWN; break;
      case K_PGUP:
	 specialkey = GLUT_KEY_PAGE_UP; break;
      case K_PGDN:
	 specialkey = GLUT_KEY_PAGE_DOWN; break;
      case K_FIND:
	 specialkey = GLUT_KEY_HOME; break;
      case K_SELECT:
	 specialkey = GLUT_KEY_END; break;
      case K_INSERT:
	 specialkey = GLUT_KEY_INSERT; break; 
      case K_REMOVE:
	 labelval = '\b';
	 break;
      case K_ENTER:
	 labelval = '\r'; break;
      }

   /* likely a keypad input, but depends on keyboard mapping, ignore */
   if(labelval == 512)
      return 1;

   /* dispatch callback */
   if(specialkey)
      HandleSpecialPress(specialkey, release);
   else {
      char c = labelval;

      if(KeyboardLedState & LED_CAP) {
	 if(c >= 'A' && c <= 'Z')
	    c += 'a' - 'A';
	 else
	    if(c >= 'a' && c <= 'z')
	       c += 'A' - 'a';
      }
      HandleKeyPress(c, release);
   }
   return 1;
}

void glutIgnoreKeyRepeat(int ignore)
{
   KeyRepeatMode = ignore ? GLUT_KEY_REPEAT_OFF : GLUT_KEY_REPEAT_ON;
}

void glutSetKeyRepeat(int repeatMode)
{
   KeyRepeatMode = repeatMode;
}

void glutForceJoystickFunc(void)
{
}

static void HandleMousePress(int button, int pressed)
{
   if(TryMenu(button, pressed))
      return;
 
   if(MouseFunc)
      MouseFunc(button, pressed ? GLUT_DOWN : GLUT_UP, MouseX, MouseY);
}

static int ReadMouse(void)
{
   int l, r, m;
   static int ll, lm, lr;
   signed char dx, dy;

#ifdef HAVE_GPM
   if(GpmMouse) {
      Gpm_Event event;
      struct pollfd pfd;
      pfd.fd = gpm_fd;
      pfd.events = POLLIN;
      if(poll(&pfd, 1, 1) != 1)
	 return 0;
	
      if(Gpm_GetEvent(&event) != 1)
	 return 0;
	
      l = event.buttons & GPM_B_LEFT;
      m = event.buttons & GPM_B_MIDDLE;
      r = event.buttons & GPM_B_RIGHT;

      /* gpm is weird in that it gives a button number when the button
	 is released, with type set to GPM_UP, this is only a problem
	 if it is the last button released */
    
      if(event.type & GPM_UP)
	 if(event.buttons == GPM_B_LEFT || event.buttons == GPM_B_MIDDLE ||
	    event.buttons == GPM_B_RIGHT || event.buttons == GPM_B_FOURTH)
	    l = m = r = 0;

      dx = event.dx;
      dy = event.dy;
   } else
#endif
   {
      char data[4];

      if(MouseFD == -1)
         return 0;

      if(read(MouseFD, data, 4) != 4)
         return 0;
	
      l = ((data[0] & 0x20) >> 3);
      m = ((data[3] & 0x10) >> 3);
      r = ((data[0] & 0x10) >> 4);

      dx = (((data[0] & 0x03) << 6) | (data[1] & 0x3F));
      dy = (((data[0] & 0x0C) << 4) | (data[2] & 0x3F));
   }

   MouseX += dx * MouseSpeed;
   if(MouseX < 0)
      MouseX = 0;
   else
      if(MouseX >= VarInfo.xres)
	 MouseX = VarInfo.xres - 1;

   MouseY += dy * MouseSpeed;
   if(MouseY < 0)
      MouseY = 0;
   else
      if(MouseY >= VarInfo.yres)
	 MouseY = VarInfo.yres - 1;

   if(l != ll)
      HandleMousePress(GLUT_LEFT_BUTTON, l);
   if(m != lm)
      HandleMousePress(GLUT_MIDDLE_BUTTON, m);
   if(r != lr)
      HandleMousePress(GLUT_RIGHT_BUTTON, r);

   ll = l, lm = m, lr = r;

   if(dx || dy || !MouseVisible) {
      if(l || m || r) {
	 if(MotionFunc)
	    MotionFunc(MouseX, MouseY);
      } else
	 if(PassiveMotionFunc)
	    PassiveMotionFunc(MouseX, MouseY);

      EraseCursor();

      MouseVisible = 1;

      if(ActiveMenu)
	 Redisplay = 1;
      else
	 SwapCursor();
   }

   LastMouseTime = glutGet(GLUT_ELAPSED_TIME);

   return 1;
}

void ReceiveInput(void)
{
   if(ConsoleFD != -1)
      while(ReadKey());
    
   while(ReadMouse());

   /* implement a 2 second timeout on the mouse */
   if(MouseVisible && glutGet(GLUT_ELAPSED_TIME) - LastMouseTime > 2000) {
      EraseCursor();
      MouseVisible = 0;
      SwapCursor();
   }
}

static void VTSwitchHandler(int sig)
{
   struct vt_stat st;
   switch(sig) {
   case SIGUSR1:
      ioctl(ConsoleFD, VT_RELDISP, 1);
      Active = 0;
#ifdef MULTIHEAD
      VisiblePoll = 1;
      TestVisible();
#else
      VisibleSwitch = 1;
      Visible = 0;
#endif
      break;
   case SIGUSR2:
      ioctl(ConsoleFD, VT_GETSTATE, &st);
      if(st.v_active)
	 ioctl(ConsoleFD, VT_RELDISP, VT_ACKACQ);

      RestoreColorMap();

      Active = 1;
      Visible = 1;
      VisibleSwitch = 1;

      Redisplay = 1;
      break;
   }
}

void InitializeVT(int usestdin)
{
   struct termios tio;
   struct vt_mode vt;
   char console[128];

   signal(SIGIO, SIG_IGN);

   Active = 1;

   if(usestdin) {
      ConsoleFD = 0;
      goto setattribs;
   }

   /* detect the current vt if it was not specified */
   if(CurrentVT == 0) {
      int fd = open("/dev/tty", O_RDWR | O_NDELAY, 0);
      struct vt_stat st;
      if(fd == -1) {
	 sprintf(exiterror, "Failed to open /dev/tty\n");
	 exit(0);
      }

      if(ioctl(fd, VT_GETSTATE, &st) == -1) {
	 fprintf(stderr, "Could not detect current vt, specify with -vt\n");
	 fprintf(stderr, "Defaulting to stdin input\n");
	 ConsoleFD = 0;
	 close(fd);
         goto setattribs;
      }

      CurrentVT =  st.v_active;
      close(fd);
   }
    
   /* if we close with the modifier set in glutIconifyWindow, we won't
      get the signal when they are released, so set to zero here */
   KeyboardModifiers = 0;

   /* open the console tty */
   sprintf(console, "/dev/tty%d", CurrentVT);
   ConsoleFD = open(console, O_RDWR | O_NDELAY, 0);
   if (ConsoleFD < 0) {
      sprintf(exiterror, "error couldn't open %s,"
	      " defaulting to stdin \n", console);
      ConsoleFD = 0;
      goto setattribs;
   }

   signal(SIGUSR1, VTSwitchHandler);
   signal(SIGUSR2, VTSwitchHandler);

   if (ioctl(ConsoleFD, VT_GETMODE, &OldVTMode) < 0) {
      sprintf(exiterror,"Failed to grab %s, defaulting to stdin\n", console);
      close(ConsoleFD);
      ConsoleFD = 0;
      goto setattribs;
   }

   vt = OldVTMode;

   vt.mode = VT_PROCESS;
   vt.waitv = 0;
   vt.relsig = SIGUSR1;
   vt.acqsig = SIGUSR2;
   if (ioctl(ConsoleFD, VT_SETMODE, &vt) < 0) {
      sprintf(exiterror, "error: ioctl(VT_SETMODE) failed: %s\n",
	      strerror(errno));
      close(ConsoleFD);
      ConsoleFD = 0;
      exit(1);
   }

   if (ioctl(ConsoleFD, KDGKBMODE, &OldKDMode) < 0) {
      sprintf(exiterror, "Warning: ioctl KDGKBMODE failed!\n");
      OldKDMode = K_XLATE;
   }

   /* use SIGIO so VT switching can work if the program is locked */
   signal(SIGIO, KeyboardHandler);

   pipe(kbdpipe);

   if(fcntl(kbdpipe[0], F_SETFL, O_NONBLOCK | O_ASYNC) < 0) {
      sprintf(exiterror, "Failed to set keyboard to non-blocking\n");
      exit(0);
   }

   fcntl(ConsoleFD, F_SETOWN, getpid());

   if(ioctl(ConsoleFD, KDGETMODE, &OldMode) < 0)
      sprintf(exiterror, "Warning: Failed to get terminal mode\n");

#ifdef HAVE_GPM
   if(!GpmMouse)
#endif
      if(ioctl(ConsoleFD, KDSETMODE, KD_GRAPHICS) < 0)
	 sprintf(exiterror,"Warning: Failed to set terminal to graphics\n");

   if(ioctl(ConsoleFD, KDSKBMODE, K_MEDIUMRAW) < 0) {
      sprintf(exiterror, "ioctl KDSKBMODE failed!\n");
      exit(0);
   }

   if(ioctl(ConsoleFD, KDGKBLED, &KeyboardLedState) < 0) {
      sprintf(exiterror, "ioctl KDGKBLED failed!\n");
      exit(0);
   }

 setattribs:
   /* enable async input input */
   if(fcntl(ConsoleFD, F_SETFL, O_ASYNC) < 0) {
      sprintf(exiterror, "Failed to set O_ASYNC mode on fd %d\n", ConsoleFD);
      exit(0);
   }

   /* save old terminos settings */
   if (tcgetattr(ConsoleFD, &OldTermios) < 0) {
      sprintf(exiterror, "tcgetattr failed\n");
      exit(0);
   }

   tio = OldTermios;

   /* terminos settings for straight-through mode */  
   tio.c_lflag &= ~(ICANON | ECHO  | ISIG);
   tio.c_iflag &= ~(ISTRIP | IGNCR | ICRNL | INLCR | IXOFF | IXON);
   tio.c_iflag |= IGNBRK;

   tio.c_cc[VMIN]  = 0;
   tio.c_cc[VTIME] = 0;

   if (tcsetattr(ConsoleFD, TCSANOW, &tio) < 0) {
      sprintf(exiterror, "tcsetattr failed\n");
      exit(0);
   }
}

void RestoreVT(void)
{
   if(ConsoleFD < 0)
      return;

   if (tcsetattr(ConsoleFD, TCSANOW, &OldTermios) < 0)
      sprintf(exiterror, "tcsetattr failed\n");

   /* setting the mode to text from graphics restores the colormap */
   if(
#ifdef HAVE_GPM
   !GpmMouse ||
#endif
   ConsoleFD == 0)
      if(ioctl(ConsoleFD, KDSETMODE, KD_GRAPHICS) < 0)
	 goto skipioctl; /* no need to fail twice */
   
   if(ioctl(ConsoleFD, KDSETMODE, OldMode) < 0)
      fprintf(stderr, "ioctl KDSETMODE failed!\n");

 skipioctl:

   if(ConsoleFD == 0)
       return;

   /* restore keyboard state */
   if (ioctl(ConsoleFD, VT_SETMODE, &OldVTMode) < 0)
      fprintf(stderr, "Failed to set vtmode\n");

   if (ioctl(ConsoleFD, KDSKBMODE, OldKDMode) < 0)
      fprintf(stderr, "ioctl KDSKBMODE failed!\n");
  
   close(ConsoleFD);

   close(kbdpipe[0]);
   close(kbdpipe[1]);
}

void InitializeMouse(void)
{
#ifdef HAVE_GPM
   if(!GpmMouse)
#endif
   {
      const char *mousedev = getenv("MOUSE");
      if(!mousedev)
	 mousedev = MOUSEDEV;
      if((MouseFD = open(mousedev, O_RDONLY | O_NONBLOCK)) >= 0) {
         if(!MouseSpeed)
            MouseSpeed = 1;
         NumMouseButtons = 3;
         return;
      }
   }
#ifdef HAVE_GPM
   {
      Gpm_Connect conn;  
      int c;
      conn.eventMask  = ~0;   /* Want to know about all the events */
      conn.defaultMask = 0;   /* don't handle anything by default  */
      conn.minMod     = 0;    /* want everything                   */
      conn.maxMod     = ~0;   /* all modifiers included            */
      if(Gpm_Open(&conn, 0) != -1) {
	 if(!MouseSpeed)
	    MouseSpeed = 8;
	 NumMouseButtons = 3;
	 return;
      }
      fprintf(stderr, "Cannot open gpmctl.\n");
   }
#endif
   fprintf(stderr,"Cannot open %s.\n"
	   "Continuing without Mouse\n", MOUSEDEV);
}

void CloseMouse(void)
{
#ifdef HAVE_GPM
   if(GpmMouse) {
      if(NumMouseButtons)
	 Gpm_Close();
   } else
#endif
      if(MouseFD >= 0)
	 close(MouseFD);
}
