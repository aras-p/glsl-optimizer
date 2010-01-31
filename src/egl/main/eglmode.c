#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "egldisplay.h"
#include "egldriver.h"
#include "eglmode.h"
#include "eglcurrent.h"
#include "eglscreen.h"
#include "eglstring.h"


#define MIN2(A, B)  (((A) < (B)) ? (A) : (B))


/**
 * Given an EGLModeMESA handle, return the corresponding _EGLMode object
 * or null if non-existant.
 */
_EGLMode *
_eglLookupMode(EGLModeMESA mode, _EGLDisplay *disp)
{
   EGLint scrnum;

   /* loop over all screens on the display */
   for (scrnum = 0; scrnum < disp->NumScreens; scrnum++) {
      const _EGLScreen *scrn = disp->Screens[scrnum];
      EGLint i;
      /* search list of modes for handle */
      for (i = 0; i < scrn->NumModes; i++) {
         if (scrn->Modes[i].Handle == mode) {
            return scrn->Modes + i;
         }
      }
   }

   return NULL;
}


/**
 * Add a new mode with the given attributes (width, height, depth, refreshRate)
 * to the given screen.
 * Assign a new mode ID/handle to the mode as well.
 * \return pointer to the new _EGLMode
 */
_EGLMode *
_eglAddNewMode(_EGLScreen *screen, EGLint width, EGLint height,
               EGLint refreshRate, const char *name)
{
   EGLint n;
   _EGLMode *newModes;

   assert(screen);
   assert(width > 0);
   assert(height > 0);
   assert(refreshRate > 0);

   n = screen->NumModes;
   newModes = (_EGLMode *) realloc(screen->Modes, (n+1) * sizeof(_EGLMode));
   if (newModes) {
      screen->Modes = newModes;
      screen->Modes[n].Handle = n + 1;
      screen->Modes[n].Width = width;
      screen->Modes[n].Height = height;
      screen->Modes[n].RefreshRate = refreshRate;
      screen->Modes[n].Optimal = EGL_FALSE;
      screen->Modes[n].Interlaced = EGL_FALSE;
      screen->Modes[n].Name = _eglstrdup(name);
      screen->NumModes++;
      return screen->Modes + n;
   }
   else {
      return NULL;
   }
}



/**
 * Parse the attrib_list to fill in the fields of the given _eglMode
 * Return EGL_FALSE if any errors, EGL_TRUE otherwise.
 */
static EGLBoolean
_eglParseModeAttribs(_EGLMode *mode, const EGLint *attrib_list)
{
   EGLint i;

   /* init all attribs to EGL_DONT_CARE */
   mode->Handle = EGL_DONT_CARE;
   mode->Width = EGL_DONT_CARE;
   mode->Height = EGL_DONT_CARE;
   mode->RefreshRate = EGL_DONT_CARE;
   mode->Optimal = EGL_DONT_CARE;
   mode->Interlaced = EGL_DONT_CARE;
   mode->Name = NULL;

   for (i = 0; attrib_list && attrib_list[i] != EGL_NONE; i++) {
      switch (attrib_list[i]) {
      case EGL_MODE_ID_MESA:
         mode->Handle = attrib_list[++i];
         if (mode->Handle <= 0) {
            _eglError(EGL_BAD_PARAMETER, "eglChooseModeMESA(handle)");
            return EGL_FALSE;
         }
         break;
      case EGL_WIDTH:
         mode->Width = attrib_list[++i];
         if (mode->Width <= 0) {
            _eglError(EGL_BAD_PARAMETER, "eglChooseModeMESA(width)");
            return EGL_FALSE;
         }
         break;
      case EGL_HEIGHT:
         mode->Height = attrib_list[++i];
         if (mode->Height <= 0) {
            _eglError(EGL_BAD_PARAMETER, "eglChooseModeMESA(height)");
            return EGL_FALSE;
         }
         break;
      case EGL_REFRESH_RATE_MESA:
         mode->RefreshRate = attrib_list[++i];
         if (mode->RefreshRate <= 0) {
            _eglError(EGL_BAD_PARAMETER, "eglChooseModeMESA(refresh rate)");
            return EGL_FALSE;
         }
         break;
      case EGL_INTERLACED_MESA:
         mode->Interlaced = attrib_list[++i];
         if (mode->Interlaced != EGL_TRUE && mode->Interlaced != EGL_FALSE) {
            _eglError(EGL_BAD_PARAMETER, "eglChooseModeMESA(interlaced)");
            return EGL_FALSE;
         }
         break;
      case EGL_OPTIMAL_MESA:
         mode->Optimal = attrib_list[++i];
         if (mode->Optimal != EGL_TRUE && mode->Optimal != EGL_FALSE) {
            _eglError(EGL_BAD_PARAMETER, "eglChooseModeMESA(optimal)");
            return EGL_FALSE;
         }
         break;
      default:
         _eglError(EGL_BAD_ATTRIBUTE, "eglChooseModeMESA");
         return EGL_FALSE;
      }
   }
   return EGL_TRUE;
}


/**
 * Determine if the candidate mode's attributes are at least as good
 * as the minimal mode's.
 * \return EGL_TRUE if qualifies, EGL_FALSE otherwise
 */
static EGLBoolean
_eglModeQualifies(const _EGLMode *c, const _EGLMode *min)
{
   if (min->Handle != EGL_DONT_CARE && c->Handle != min->Handle)
      return EGL_FALSE;
   if (min->Width != EGL_DONT_CARE && c->Width < min->Width)
      return EGL_FALSE;
   if (min->Height != EGL_DONT_CARE && c->Height < min->Height)
      return EGL_FALSE;
   if (min->RefreshRate != EGL_DONT_CARE && c->RefreshRate < min->RefreshRate)
      return EGL_FALSE;
   if (min->Optimal != EGL_DONT_CARE && c->Optimal != min->Optimal)
      return EGL_FALSE;
   if (min->Interlaced != EGL_DONT_CARE && c->Interlaced != min->Interlaced)
      return EGL_FALSE;

   return EGL_TRUE;
}


/**
 * Return value of given mode attribute, or -1 if bad attrib.
 */
static EGLint
getModeAttrib(const _EGLMode *m, EGLint attrib)
{
   switch (attrib) {
   case EGL_MODE_ID_MESA:
      return m->Handle;
   case EGL_WIDTH:
      return m->Width;
   case EGL_HEIGHT:
      return m->Height;
   case EGL_REFRESH_RATE_MESA:
      return m->RefreshRate;
   case EGL_OPTIMAL_MESA:
      return m->Optimal;
   case EGL_INTERLACED_MESA:
      return m->Interlaced;
   default:
      return -1;
   }
}


#define SMALLER 1
#define LARGER  2

struct sort_info {
   EGLint Attrib;
   EGLint Order; /* SMALLER or LARGER */
};

/* the order of these entries is the priority */
static struct sort_info SortInfo[] = {
   { EGL_OPTIMAL_MESA, LARGER },
   { EGL_INTERLACED_MESA, SMALLER },
   { EGL_WIDTH, LARGER },
   { EGL_HEIGHT, LARGER },
   { EGL_REFRESH_RATE_MESA, LARGER },
   { EGL_MODE_ID_MESA, SMALLER },
   { 0, 0 }
};


/**
 * Compare modes 'a' and 'b' and return -1 if a belongs before b, or 1 if a
 * belongs after b, or 0 if they're equal.
 * Used by qsort().
 */
static int
_eglCompareModes(const void *a, const void *b)
{
   const _EGLMode *aMode = *((const _EGLMode **) a);
   const _EGLMode *bMode = *((const _EGLMode **) b);
   EGLint i;

   for (i = 0; SortInfo[i].Attrib; i++) {
      const EGLint aVal = getModeAttrib(aMode, SortInfo[i].Attrib);
      const EGLint bVal = getModeAttrib(bMode, SortInfo[i].Attrib);
      if (aVal == bVal) {
         /* a tie */
         continue;
      }
      else if (SortInfo[i].Order == SMALLER) {
         return (aVal < bVal) ? -1 : 1;
      }
      else if (SortInfo[i].Order == LARGER) {
         return (aVal > bVal) ? -1 : 1;
      }
   }

   /* all attributes identical */
   return 0;
}


/**
 * Search for EGLModes which match the given attribute list.
 * Called via eglChooseModeMESA API function.
 */
EGLBoolean
_eglChooseModeMESA(_EGLDriver *drv, _EGLDisplay *dpy, _EGLScreen *scrn,
                   const EGLint *attrib_list, EGLModeMESA *modes,
                   EGLint modes_size, EGLint *num_modes)
{
   _EGLMode **modeList, min;
   EGLint i, count;

   if (!_eglParseModeAttribs(&min, attrib_list)) {
      /* error code will have been recorded */
      return EGL_FALSE;
   }

   /* allocate array of mode pointers */
   modeList = (_EGLMode **) malloc(modes_size * sizeof(_EGLMode *));
   if (!modeList) {
      _eglError(EGL_BAD_MODE_MESA, "eglChooseModeMESA(out of memory)");
      return EGL_FALSE;
   }

   /* make array of pointers to qualifying modes */
   for (i = count = 0; i < scrn->NumModes && count < modes_size; i++) {
      if (_eglModeQualifies(scrn->Modes + i, &min)) {
         modeList[count++] = scrn->Modes + i;
      }
   }

   /* sort array of pointers */
   qsort(modeList, count, sizeof(_EGLMode *), _eglCompareModes);

   /* copy mode handles to output array */
   for (i = 0; i < count; i++) {
      modes[i] = modeList[i]->Handle;
   }

   free(modeList);

   *num_modes = count;

   return EGL_TRUE;
}



/**
 * Return all possible modes for the given screen.  No sorting of results.
 * Called via eglGetModesMESA() API function.
 */
EGLBoolean
_eglGetModesMESA(_EGLDriver *drv, _EGLDisplay *dpy, _EGLScreen *scrn,
                 EGLModeMESA *modes, EGLint modes_size, EGLint *num_modes)
{
   if (modes) {
      EGLint i;
      *num_modes = MIN2(scrn->NumModes, modes_size);
      for (i = 0; i < *num_modes; i++) {
         modes[i] = scrn->Modes[i].Handle;
      }
   }
   else {
      /* just return total number of supported modes */
      *num_modes = scrn->NumModes;
   }

   return EGL_TRUE;
}


/**
 * Query an attribute of a mode.
 */
EGLBoolean
_eglGetModeAttribMESA(_EGLDriver *drv, _EGLDisplay *dpy,
                      _EGLMode *m, EGLint attribute, EGLint *value)
{
   EGLint v;

   v = getModeAttrib(m, attribute);
   if (v < 0) {
      _eglError(EGL_BAD_ATTRIBUTE, "eglGetModeAttribMESA");
      return EGL_FALSE;
   }
   *value = v;
   return EGL_TRUE;
}


/**
 * Return human-readable string for given mode.
 * This is the default function called by eglQueryModeStringMESA().
 */
const char *
_eglQueryModeStringMESA(_EGLDriver *drv, _EGLDisplay *dpy, _EGLMode *m)
{
   return m->Name;
}
