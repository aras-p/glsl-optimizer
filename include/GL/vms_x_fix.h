/***************************************************************************
 *                                                                         *
 * Repair definitions of Xlib when compileing with /name=(as_is) on VMS    *
 * You'll need the PORTING_LIBRARY (get it from Compaq) installed          *
 *                                                                         *
 * Author : Jouk Jansen (joukj@hrem.stm.tudelft.nl)                        *
 *                                                                         *
 * Last revision : 22 August 2000                                          *
 *                                                                         *
 ***************************************************************************/

#ifndef VMS_X_FIX
#define VMS_X_FIX

#ifdef __cplusplus
#define VMS_BEGIN_C_PLUS_PLUS extern "C" {
#define VMS_END_C_PLUS_PLUS }
#else
#define VMS_BEGIN_C_PLUS_PLUS
#define VMS_END_C_PLUS_PLUS
#endif

#include <motif_redefines.h>

#define XQueryFont XQUERYFONT
#define XSetPlaneMask XSETPLANEMASK
#define XChangeKeyboardControl XCHANGEKEYBOARDCONTROL
#define XDestroySubwindows XDESTROYSUBWINDOWS
#define XFreeDeviceList XFREEDEVICELIST
#define XFreeDeviceState XFREEDEVICESTATE
#define XGetExtensionVersion XGETEXTENSIONVERSION
#define XGetRGBColormaps XGETRGBCOLORMAPS
#define XIconifyWindow XICONIFYWINDOW
#define XInstallColormap XINSTALLCOLORMAP
#define XListInputDevices XLISTINPUTDEVICES
#define XLookupKeysym XLOOKUPKEYSYM
#define XOpenDevice XOPENDEVICE
#define XQueryDeviceState XQUERYDEVICESTATE
#define XSelectExtensionEvent XSELECTEXTENSIONEVENT
#define XWarpPointer XWARPPOINTER
#define XmuLookupStandardColormap XMULOOKUPSTANDARDCOLORMAP

#endif
