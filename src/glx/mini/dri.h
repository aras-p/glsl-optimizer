/**
 * \file miniglxP.h
 * \brief Define replacements for some X data types and define the DRI-related
 * data structures.
 *
 * \note Cut down version of glxclient.h.
 *
 */

#ifndef _dri_h_
#define _dri_h_

#include "driver.h"

typedef struct __DRIscreenRec   __DRIscreen;   /**< \copydoc __DRIscreenRec */
typedef struct __DRIcontextRec  __DRIcontext;  /**< \copydoc __DRIcontextRec */
typedef struct __DRIdrawableRec __DRIdrawable; /**< \copydoc __DRIdrawableRec */

/**
 * \brief Screen dependent methods.
 *
 * This structure is initialized during the MiniGLXDisplayRec::createScreen
 * call.
 */
struct __DRIscreenRec {
    /**
     * \brief Method to destroy the private DRI screen data.
     */
    void (*destroyScreen)(__DRIscreen *screen);

    /**
     * \brief Method to create the private DRI context data and initialize the
     * context dependent methods.
     */
    void *(*createContext)(__DRIscreen *screen, const __GLcontextModes *glVisual,
			   void *sharedPrivate);
    /**
     * \brief Method to create the private DRI drawable data and initialize the
     * drawable dependent methods.
     */
    void *(*createDrawable)(__DRIscreen *screen,
			    int width, int height, int index,
			    const __GLcontextModes *glVisual);

    /*
     * XXX in the future, implement this:
    void *(*createPBuffer)(Display *dpy, int scrn, GLXPbuffer pbuffer,
			   GLXFBConfig config, __DRIdrawable *pdraw);
     */

    /**
     * \brief Opaque pointer to private per screen direct rendering data.  
     *
     * \c NULL if direct rendering is not supported on this screen.  Never
     * dereferenced in libGL.
     */
};

/**
 * \brief Context dependent methods. 
 * 
 * This structure is initialized during the __DRIscreenRec::createContext call.
 */
struct __DRIcontextRec {
    /**
     * \brief Method to destroy the private DRI context data.
     */
    void (*destroyContext)(__DRIcontext *context);

    /**
     * \brief Method to bind a DRI drawable to a DRI graphics context.
     * 
     * \todo XXX in the future, also pass a 'read' GLXDrawable for
     * glXMakeCurrentReadSGI() and GLX 1.3's glXMakeContextCurrent().
     */
    GLboolean (*bindContext)(__DRIscreen *screen, __DRIdrawable *drawable, __DRIcontext *context);

    /**
     * \brief Method to unbind a DRI drawable to a DRI graphics context.
     */
    GLboolean (*unbindContext)(__DRIdrawable *drawable, __DRIcontext *context);
    /**
     * \brief Opaque pointer to private per context direct rendering data.
     * 
     * NULL if direct rendering is not supported on the display or
     * screen used to create this context.  Never dereferenced in libGL.
     */
};

/**
 * \brief Drawable dependent methods.
 *
 * This structure is initialized during the __DRIscreenRec::createDrawable call.
 *
 * __DRIscreenRec::createDrawable is not called by libGL at this time.  It's
 * currently used via the dri_util.c utility code instead.
 */
struct __DRIdrawableRec {
    /**
     * \brief Method to destroy the private DRI drawable data.
     */
    void (*destroyDrawable)(__DRIdrawable *drawable);


    /**
     * \brief Method to swap the front and back buffers.
     */
    void (*swapBuffers)(__DRIdrawable *drawable);

    /**
     * \brief Opaque pointer to private per drawable direct rendering data.
     * 
     * \c NULL if direct rendering is not supported on the display or
     * screen used to create this drawable.  Never dereferenced in libGL.
     */
};

typedef void *(driCreateScreenFunc)(struct DRIDriverRec *driver,
                  struct DRIDriverContextRec *driverContext);

/** This must be implemented in each driver */
extern driCreateScreenFunc __driCreateScreen;

#endif /* _dri_h_ */
