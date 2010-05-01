if 0 { 
 Copyright (c) 2008, 2009 Apple Inc.
 
 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation files
 (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge,
 publish, distribute, sublicense, and/or sell copies of the Software,
 and to permit persons to whom the Software is furnished to do so,
 subject to the following conditions:
 
 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT.  IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT
 HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 DEALINGS IN THE SOFTWARE.
 
 Except as contained in this notice, the name(s) of the above
 copyright holders shall not be used in advertising or otherwise to
 promote the sale, use or other dealings in this Software without
 prior written authorization.
}

package require Tcl 8.5

proc main {argc argv} {
    if {2 != $argc} {
	puts stderr "syntax is: [info script] serialized-array-file export.list"
	return 1
    }

    set fd [open [lindex $argv 0] r]
    array set api [read $fd]
    close $fd

    #Start with 1.0
    set glxlist [list \
                     glXChooseVisual glXCreateContext glXDestroyContext \
                     glXMakeCurrent glXCopyContext glXSwapBuffers \
                     glXCreateGLXPixmap glXDestroyGLXPixmap \
                     glXQueryExtension glXQueryVersion \
                     glXIsDirect glXGetConfig \
                     glXGetCurrentContext glXGetCurrentDrawable \
                     glXWaitGL glXWaitX glXUseXFont]

    #GLX 1.1 and later
    lappend glxlist glXQueryExtensionsString glXQueryServerString \
                     glXGetClientString

    #GLX 1.2 and later
    lappend glxlist glXGetCurrentDisplay

    #GLX 1.3 and later
    lappend glxlist glXChooseFBConfig glXGetFBConfigAttrib \
        glXGetFBConfigs glXGetVisualFromFBConfig \
        glXCreateWindow glXDestroyWindow \
        glXCreatePixmap glXDestroyPixmap \
        glXCreatePbuffer glXDestroyPbuffer \
        glXQueryDrawable glXCreateNewContext \
        glXMakeContextCurrent glXGetCurrentReadDrawable \
        glXQueryContext glXSelectEvent glXGetSelectedEvent

    #GLX 1.4 and later
    lappend glxlist glXGetProcAddress

    #Extensions
    lappend glxlist glXGetProcAddressARB

    #Old extensions we don't support and never really have, but need for
    #symbol compatibility.  See also: glx_empty.c
    lappend glxlist glXSwapIntervalSGI glXSwapIntervalMESA \
	glXGetSwapIntervalMESA glXBeginFrameTrackingMESA \
	glXEndFrameTrackingMESA glXGetFrameUsageMESA \
	glXQueryFrameTrackingMESA glXGetVideoSyncSGI \
	glXWaitVideoSyncSGI glXJoinSwapGroupSGIX \
	glXBindSwapBarrierSGIX glXQueryMaxSwapBarriersSGIX \
	glXGetSyncValuesOML glXSwapBuffersMscOML \
	glXWaitForMscOML glXWaitForSbcOML \
	glXAllocateMemoryMESA glXFreeMemoryMESA \
	glXGetMemoryOffsetMESA glXReleaseBuffersMESA \
	glXCreateGLXPixmapMESA glXCopySubBufferMESA \
	glXQueryGLXPbufferSGIX glXCreateGLXPbufferSGIX \
	glXDestroyGLXPbufferSGIX glXSelectEventSGIX \
	glXGetSelectedEventSGIX 
    
    #These are for GLX_SGIX_fbconfig, which isn't implemented, because
    #we have the GLX 1.3 GLXFBConfig functions which are in the standard spec.
    #It should be possible to support these to some extent.
    #The old libGL somewhat supported the GLXFBConfigSGIX code, but lacked
    #pbuffer, and pixmap support.
    #We mainly just need these stubs for linking with apps, because for 
    #some reason the OpenGL site suggests using the latest glxext.h, 
    #and glxext.h defines all GLX extensions, which doesn't seem right for
    #compile-time capability detection.
    #See also: http://www.mesa3d.org/brianp/sig97/exten.htm#Compile
    #which conflicts with: the ABI registry from what I saw on opengl.org. 
    #By disabling some of the #defines in glxext.h we break some software,
    #and by enabling them without the symbols we break others (in Mesa).
    #I think a lot of OpenGL-based programs have issues one way or another.
    #It seems that even Mesa developers are confused on this issue, because
    #Mesa-7.3/progs/xdemos/glxgears_fbconfig.c has comments about breakage 
    #in some comments.
    lappend glxlist glXGetFBConfigAttribSGIX \
	glXChooseFBConfigSGIX \
	glXGetVisualFromFBConfigSGIX \
	glXCreateGLXPixmapWithConfigSGIX \
	glXCreateContextWithConfigSGIX \
	glXGetFBConfigFromVisualSGIX
    

    set fd [open [lindex $argv 1] w]
    
    foreach f [lsort -dictionary [array names api]] {
	puts $fd _gl$f
    }

    foreach f [lsort -dictionary $glxlist] {
	puts $fd _$f
    }
    
    close $fd

    return 0
}

exit [main $::argc $::argv]