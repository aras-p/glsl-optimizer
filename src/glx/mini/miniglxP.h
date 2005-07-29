/**
 * \file miniglxP.h
 * \brief Define replacements for some X data types and define the DRI-related
 * data structures.
 *
 * \note Cut down version of glxclient.h.
 *
 */

#ifndef _mini_GLX_client_h_
#define _mini_GLX_client_h_

#include <signal.h>
#include <linux/fb.h>

#include <GL/miniglx.h>
#include "glheader.h"
#include "mtypes.h"

#include "driver.h"
#include "GL/internal/dri_interface.h"

/**
 * \brief Supported pixel formats.
 */
enum PixelFormat {
   PF_B8G8R8,    /**< \brief 24-bit BGR */
   PF_B8G8R8A8,  /**< \brief 32-bit BGRA */
   PF_B5G6R5,    /**< \brief 16-bit BGR */
   PF_B5G5R5,    /**< \brief 15-bit BGR */
   PF_CI8        /**< \brief 8-bit color index */
};

/**
 * \brief X Visual type.
 *
 * \sa ::Visual, \ref datatypes.
 */
struct MiniGLXVisualRec {
   /** \brief GLX visual information */
   const __GLcontextModes *mode;

   /** \brief pointer back to corresponding ::XVisualInfo */
   XVisualInfo *visInfo;               

   /** \brief display handle */
   Display *dpy;

   /** \brief pixel format */
   enum PixelFormat pixelFormat;       
};



/**
 * \brief X Window type.
 *
 * \sa ::Window, \ref datatypes.
 */
struct MiniGLXWindowRec {
   Visual *visual;
                                   /** \brief position (always 0,0) */
   int x, y;
                                   /** \brief size */
   unsigned int w, h;
   void *frontStart;               /**< \brief start of front color buffer */
   void *backStart;                /**< \brief start of back color buffer */
   size_t size;                    /**< \brief color buffer size, in bytes */
   GLuint bytesPerPixel;
   GLuint rowStride;               /**< \brief in bytes */
   GLubyte *frontBottom;           /**< \brief pointer to last row */
   GLubyte *backBottom;            /**< \brief pointer to last row */
   GLubyte *curBottom;             /**<  = frontBottom or backBottom */
   __DRIdrawable driDrawable;
   GLuint ismapped;
};


/**
 * \brief GLXContext type.
 *
 * \sa ::GLXContext, \ref datatypes.
 */
struct MiniGLXContextRec {
   Window drawBuffer;       /**< \brief drawing buffer */
   Window curBuffer;        /**< \brief current buffer */
   VisualID vid;            /**< \brief visual ID */
   __DRIcontext driContext; /**< \brief context dependent methods */
};

#define MINIGLX_BUF_SIZE 512
#define MINIGLX_MAX_SERVER_FDS 10
#define MINIGLX_MAX_CLIENT_FDS 1
#define MINIGLX_EVENT_QUEUE_SZ 16
#define MINIGLX_EVENT_QUEUE_MASK (MINIGLX_EVENT_QUEUE_SZ-1)

/**
 * A connection to/from the server
 *
 * All information is to/from the server is buffered and then dispatched by 
 * __miniglx_Select() to avoid blocking the server.
 */
struct MiniGLXConnection {
   int fd;				/**< \brief file descriptor */
   char readbuf[MINIGLX_BUF_SIZE];	/**< \brief read buffer */
   char writebuf[MINIGLX_BUF_SIZE];	/**< \brief write buffer */
   int readbuf_count;			/**< \brief count of bytes waiting to be read */
   int writebuf_count;			/**< \brief count of bytes waiting to be written */
};


/**
 * \brief X Display type
 *
 * \sa ::Display, \ref datatypes.
 */
struct MiniGLXDisplayRec {
   /** \brief fixed framebuffer screen info */
   struct fb_fix_screeninfo FixedInfo; 
   /** \brief original and current variable framebuffer screen info */
   struct fb_var_screeninfo OrigVarInfo, VarInfo;
   struct sigaction OrigSigUsr1;
   struct sigaction OrigSigUsr2;
   int OriginalVT;
   int ConsoleFD;        /**< \brief console TTY device file descriptor */
   int FrameBufferFD;    /**< \brief framebuffer device file descriptor */
   int NumWindows;       /**< \brief number of open windows */
   Window TheWindow;     /**< \brief open window - only allow one window for now */
   int rotateMode;


   volatile int vtSignalFlag;
   volatile int haveVT;	/**< \brief whether the VT is hold */
   int hwActive;	/**< \brief whether the hardware is active -- mimics
			  the variations of MiniGLXDisplayRec::haveVT */


   int IsClient;	/**< \brief whether it's a client or the server */
   int clientID;
   int nrFds;		/**< \brief number of connections (usually just one for the clients) */
   struct MiniGLXConnection *fd;	/**< \brief connections */
   int drmFd;           /**< \brief handle to drm device */
   int authorized;      /**< \brief has server authorized this process? */

   struct {
      int nr, head, tail;
      XEvent queue[MINIGLX_EVENT_QUEUE_SZ];
   } eventqueue;
   
   /**
    * \name Visuals 
    *
    * Visuals (configs) in this screen.
    */
   /*@{*/
   const __GLcontextModes *driver_modes; /**< \brief Modes filtered by driver. */
   /*@}*/
    
   /**
   * \name From __GLXdisplayPrivate
   */
   /*@{*/
   PFNCREATENEWSCREENFUNC createNewScreen; /**< \brief \e __driCreateScreen hook */
   __DRIscreen driScreen;         /**< \brief Screen dependent methods */
   void *dlHandle;                /**<
				   * \brief handle to the client dynamic
				   * library 
				   */
   /*@}*/

   /**
    * \brief Mini GLX specific driver hooks
    */
   struct DRIDriverRec *driver;
   struct DRIDriverContextRec driverContext;

   /**
    * \name Configuration details
    *
    * They are read from a configuration file by __read_config_file().
    */
   /*@{*/
   const char *fbdevDevice;
   const char *clientDriverName;
   /*@}*/
};

/** Character messages. */
enum msgs {
   _CanIHaveFocus,
   _IDontWantFocus,
   _YouveGotFocus,
   _YouveLostFocus,
   _RepaintPlease,
   _Authorize,
};
extern int send_msg( Display *dpy, int i, const void *msg, size_t sz );
extern int send_char_msg( Display *dpy, int i, char msg );
extern int blocking_read( Display *dpy, int connection, char *msg, size_t msg_size );
extern int handle_fd_events( Display *dpy, int nonblock );

extern int __miniglx_open_connections( Display *dpy );
extern void __miniglx_close_connections( Display *dpy );

#endif /* !_mini_GLX_client_h_ */
