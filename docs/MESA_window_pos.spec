Name

    MESA_window_pos

Name Strings

    GL_MESA_window_pos

Contact

    Brian Paul, brianp 'at' mesa3d.org

Status

    Shipping (since Mesa version 1.2.8)

Version

    $Id: MESA_window_pos.spec,v 1.1 1999/07/20 00:30:41 brianp Exp $

Number

    XXX non assigned

Dependencies

    OpenGL 1.0 is required.
    The extension is written against the OpenGL 1.2 Specification

Overview

    In order to set the current raster position to a specific window
    coordinate with the RasterPos command, the modelview matrix, projection
    matrix and viewport must be set very carefully.  Furthermore, if the
    desired window coordinate is outside of the window's bounds one must
    rely a subtle side-effect of the Bitmap command in order to circumvent
    frustum clipping.

    This extension provides a set of functions to directly set the
    current raster position, bypassing the modelview matrix, the
    projection matrix and the viewport to window mapping.  Furthermore,
    clip testing is not performed.

    This greatly simplifies the process of setting the current raster
    position to a specific window coordinate prior to calling DrawPixels,
    CopyPixels or Bitmap.

New Procedures and Functions

    void WindowPos2dMESA(double x, double y)
    void WindowPos2fMESA(float x, float y)
    void WindowPos2iMESA(int x, int y)
    void WindowPos2sMESA(short x, short y)

New Tokens

    none

Additions to Chapter 2 of the OpenGL 1.2 Specification (OpenGL Operation)

  - (2.12, p. 41) Insert after third paragraph:

      Alternately, the current raster position may be set by one of the
      WindowPosMESA commands:

         void WindowPos{234}{sidf}MESA( T coords );
         void Window Pos{234}{sidf}vMESA( T coords );

      WindosPos4MESA takes four values indicating x, y, z, and w.
      WindowPos3MESA (or WindowPos2MESA) is analaguos, but sets only
      x, y, and z with w implicitly set to 1 (or only x and y with z
      implicititly set to 0 and w implicitly set to 1).

      WindowPosMESA operates like RasterPos except that the current modelview
      matrix, projection matrix and viewport parameters are ignored and the
      clip test operation always passes.  The current raster position values
      are directly set to the parameters passed to WindowPosMESA.  The current
      color, color index and texture coordinate update the current raster
      position's associated data.

      The current raster distance ??? XXX ???







Additions to the AGL/GLX/WGL Specifications

    None

GLX Protocol

    Not specified at this time.  However, a protocol message very similar
    to that of RasterPos is expected.

Errors

    INVALID_OPERATION is generated if WindowPosMESA is called betweeen
    Begin and End.


New State

    None.

New Implementation Dependent State

    None.

Revision History

  * Revision 1.0 - Initial specification
