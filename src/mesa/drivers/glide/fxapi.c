/* -*- mode: C; tab-width:8;  -*-

             fxapi.c - 3Dfx VooDoo/Mesa interface
*/

/*
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
 *
 ********************************************************************
 *
 * Function names:
 *  fxMesa....     (The driver API)
 *  fxDD....       (Mesa device driver functions)
 *  fxTM....       (Texture manager functions)
 *  fxSetup....    (Voodoo units setup functions)
 *  fx....         (Internal driver functions)
 *
 * Data type names:
 *  fxMesa....     (Public driver data types)
 *  tfx....        (Private driver data types)
 *
 ********************************************************************
 *
 * V0.30 - David Bucciarelli (davibu@tin.it) Humanware s.r.l.
 *         - introduced a new MESA_GLX_FX related behavior
 *         - the Glide fog table was built in a wrong way (using
 *           gu* Glide function). Added the code for building the
 *           table following the OpenGL specs. Thanks to Steve Baker
 *           for highlighting the problem.
 *         - fixed few problems in my and Keith's fxDDClear code
 *         - merged my code with the Keith's one
 *         - used the new BlendFunc Mesa device driver function 
 *         - used the new AlphaFunc Mesa device driver function 
 *         - used the new Enable Mesa device driver function 
 *         - fixed a bug related to fog in the Mesa core. Fog
 *           were applied two times: at vertex level and at fragment
 *           level (thanks to Steve Baker for reporting the problem)
 *         - glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE) now works
 *           (thanks to Jiri Pop for reporting the problem)
 *         - the driver works fine with OpenGL Unreal
 *         - fixed a bug in the Mesa core clipping code (related
 *           to the q texture coordinate)
 *         - introduced the support for the q texture coordinate
 *
 *         Keith Whitwell (keithw@cableinet.co.uk)
 *         - optimized the driver and written all the new code
 *           required by the new Mesa-3.1 device driver API
 *           and by the new Mesa-3.1 core changes
 *         - written the cva support and many other stuff
 *
 *         Brian Paul (brian_paul@avid.com) Avid Technology
 *         - fixed display list share bug for MESA_GLX_FX = window/fullscreen
 *         - fixed glClear/gl...Mask related problem
 *
 *         Bert Schoenwaelder (bert@prinz-atm.CS.Uni-Magdeburg.De)
 *         - the driver is now able to sleep when waiting for the completation
 *           of multiple swapbuffer operations instead of wasting
 *           CPU time (NOTE: you must uncomment the lines in the
 *           fxMesaSwapBuffers function in order to enable this option)
 *
 *         Eero Pajarre (epajarre@koti.tpo.fi)
 *         - enabled the macro FLOAT_COLOR_TO_UBYTE_COLOR under
 *           windows
 *         - written an asm x86 optimized float->integer conversions
 *           for windows
 *
 *         Theodore Jump (tjump@cais.com)
 *         - fixed a small problem in the __wglMonitor function of the
 *           wgl emulator
 *         - written the in-window-rendering hack support for windows
 *           and Vooodoo1/2 cards
 *
 * V0.29 - David Bucciarelli (tech.hmw@plus.it) Humanware s.r.l.
 *         - included in Mesa-3.0
 *         - now glGetString(GL_RENDERER) returns more information
 *           about the hardware configuration: "Mesa Glide <version>
 *           <Voodoo_Graphics|Voodoo_Rush|UNKNOWN> <num> CARD/<num> FB/
 *           <num> TM/<num> TMU/<NOSLI|SLI>"
 *           where: <num> CARD is the card used for the current context,
 *           <num> FB is the number of MB for the framebuffer,
 *           <num> TM is the number of MB for the texture memory,
 *           <num> TMU is the number of TMU. You can try to run
 *           Mesa/demos/glinfo in order to have an example of the
 *           output
 *         - fixed a problem of the WGL emulator with the
 *           OpenGL Optimizer 1.1 (thanks to Erwin Coumans for
 *           the bug report)
 *         - fixed some bug in the fxwgl.c code (thanks to  
 *           Peter Pettersson for a patch and a bug report)
 *
 *         Theodore Jump (tjump@cais.com)
 *         - written the SST_DUALHEAD support in the WGL emulator
 *
 *         Daryll Strauss (daryll@harlot.rb.ca.us)
 *         - fixed the Voodoo Rush support for the in window rendering
 *
 * V0.28 - David Bucciarelli (tech.hmw@plus.it) Humanware s.r.l.
 *         - the only supported multitexture functions are GL_MODULATE
 *           for texture set 0 and GL_MODULATE for texture set 1. In
 *           all other cases, the driver falls back to pure software
 *           rendering
 *         - written the support for the new  GL_EXT_multitexture
 *         - written the DD_MAX_TEXTURE_COORD_SETS support in the
 *           fxDDGetParameteri() function
 *         - the driver falls back to pure software rendering when
 *           texture mapping function is GL_BLEND
 *
 * V0.27 - David Bucciarelli (tech.hmw@plus.it) Humanware s.r.l.
 *         - inluded in the Mesa-3.0beta5
 *         - written a smal extension (GL_FXMESA_global_texture_lod_bias) in
 *           order to expose the LOD bias related Glide function
 *         - fixed a bug fxDDWriteMonoRGBAPixels()
 *         - the driver is now able to fallback to software rendering in
 *           any case not directly supported by the hardware
 *         - written the support for enabling/disabling dithering
 *         - the in-window-rendering hack now works with any X11 screen
 *           depth
 *         - fixed a problem related to color/depth/alpha buffer clears
 *         - fixed a problem when clearing buffer for a context with the
 *           alpha buffer
 *         - fixed a problem in the fxTMReloadSubMipMapLevel() function,
 *           I have forget a "break;" (thanks to Joe Waters for the bug report)
 *         - fixed a problem in the color format for the in window
 *           rendering hack
 *         - written the fxDDReadRGBAPixels function
 *         - written the fxDDDepthTestPixelsGeneric function
 *         - written the fxDDDepthTestSpanGeneric function
 *         - written the fxDDWriteMonoRGBAPixels function
 *         - written the fxDDWriteRGBAPixels function
 *         - removed the multitexture emulation code for Voodoo board
 *         with only one TMU
 *
 *         Chris Prince <cprince@cs.washington.edu>
 *         - fixed a new bug in the wglUseFontBitmaps code
 *
 *         Ralf Knoesel (rknoesel@Stormfront.com)
 *         - fixed a bug in the wglUseFontBitmaps code
 *
 *         Rune Hasvold (runeh@ifi.uio.no)
 *         - fixed a problem related to the aux usage in the fxBestResolution
 *           function
 *         - fixed the order of pixel formats in the WGL emulator
 *
 *         Fredrik Hubinette (hubbe@hubbe.net)
 *         - the driver shutdown the Glide for most common signals
 *
 * V0.26 - David Bucciarelli (tech.hmw@plus.it) Humanware s.r.l.
 *         - included in the Mesa-3.0beta4
 *         - fixed a problem related to a my optimization for the Rune's
 *           pixel-span optimization
 *         - fixed a problem related to fxDDSetNearFar() and ctx->ProjectionMatrixType
 *           (thanks to Ben "Ctrl-Alt-Delete" and the Raul Alonso's ssystem)
 *         - fixed a small bug in the Rune's pixel-span optimization
 *         - fixed a problem with GL_CCW (thanks to Curt Olson for and example
 *           of the problem)
 *         - grVertex setup code is now ready for the internal thread support
 *         - fixed a no used optimization with clipped vertices in
 *           grVertex setup code
 *         - fixed a problem in the GL_LIGHT_MODEL_TWO_SIDE support (thanks
 *           to Patrick H. Madden for a complete example of the bug)
 *
 *         Rune Hasvold (runeh@ifi.uio.no)
 *         - highly optimized the driver functions for writing pixel
 *           span (2-3 times faster !)
 *
 *         Axel W. Volley (volley@acm.org) Krauss-Maffei Wehrtechnik
 *         - written the fxDDReadDepthSpanFloat() and fxDDReadDepthSpanInt()
 *           functions
 *
 * V0.25 - David Bucciarelli (tech.hmw@plus.it) Humanware s.r.l.
 *         - fixed a problem with Voodoo boards with only one TMU
 *         - fixed a bug in the fxMesaCreateContext()
 *         - now the GL_FRONT_AND_BACK works fine also with
 *           the alpha buffer and/or antialiasing
 *         - written the support for GL_FRONT_AND_BACK drawing but
 *           it doesn't works with the alpha buffer and/or antialiasing
 *         - fixed some bug in the Mesa core for glCopyTexSubImage
 *           and glCopyTexImage functions (thanks to Mike Connell
 *           for an example of the problem)
 *         - make some small optimizations in the Mesa core in order
 *           to save same driver call and state change for not very
 *           well written applications
 *         - introduced the NEW_DRVSTATE and make other optimizations
 *           for minimizing state changes
 *         - made a lot of optimizations in order to minimize state
 *           changes
 *         - it isn't more possible to create a context with the
 *           depth buffer and the stancil buffer (it isn't yet supported)
 *         - now the partial support for the Multitexture extension
 *           works with Quake2 for windows
 *         - vertex snap is not longer used for the Voodoo2 (FX_V2
 *           must be defined)
 *         - done a lot of cleanup in the fxsetup.c file
 *         - now the partial support for the Multitexture extension
 *           works with GLQuake for windows
 *
 *         Dieter Nuetzel (nuetzel@kogs.informatik.uni-hamburg.de) University of Hamburg
 *         - fixed a problem in the asm code for Linux of the fxvsetup.c file
 *           highlighted by the binutils-2.8.1.0.29. 'fildw' asm instruction
 *           changed in 'fild'
 *
 *         Kevin Hester (kevinh@glassworks.net)
 *         - written the wglUseFontBitmaps() function in the WGL emulator
 *
 * V0.24 - David Bucciarelli (tech.hmw@plus.it) Humanware s.r.l.
 *         - now the drive always uses per fragment fog
 *         - written a small optimization in the points drawing function
 *         - written the support for trilinear filtering with 2 TMUs
 *         - written the first partial support for the Multitexture extension.
 *           This task is quite hard because the color combine units work after
 *           the two texture combine units and not before as required by the 
 *           Multitexture extension
 *         - written a workaround in fxBestResolution() in order to solve a
 *           problem with bzflag (it asks for 1x1 window !)
 *         - changed the fxBestResolution() behavior. It now returns the larger
 *           screen resolution supported by the hardware (instead of 640x480)
 *           when it is unable to find an appropriate resolution that is large
 *           enough for the requested size 
 *         - the driver is now able to use also the texture memory attached to
 *           second TMU
 *         - the texture memory manager is now able to work with two TMUs and
 *           store texture maps in the memory attached to TMU0, TMU1 or to split
 *           the mimpmap levels across TMUs in order to support trilinear filtering
 *         - I have bought a Voodoo2 board !
 *         - the amount of frambuffer ram is now doubled when an SLI configuration
 *           is detected
 *         - solved a problem related to the fxDDTexParam() and fxTexInvalidate()
 *           functions (thanks to Rune Hasvold for highlighting the problem)
 *         - done some cleanup in the fxvsetup.c file, written
 *           the FXVSETUP_FUNC macro
 *         - done a lot of cleanup in data types and field names
 *
 *         Rune Hasvold (runeh@ifi.uio.no)
 *         - written the support for a right management of the auxiliary buffer.
 *           You can now use an 800x600 screen without the depth and alpha
 *           buffer
 *         - written the support for a new pixel format (without the depth
 *           and alpha buffer) in the WGL emulator
 *         - fixed a bug in the window version of the GLUT (it was ever asking
 *           for depth buffer)
 *
 * V0.23 - David Bucciarelli (tech.hmw@plus.it) Humanware s.r.l.
 *         - included in the Mesa-3.0beta2 release
 *         - written the support for the OpenGL 1.2 GL_TEXTURE_BASE_LEVEL
 *           and GL_TEXTURE_MAX_LEVEL
 *         - rewritten several functions for a more clean support of texture
 *           mapping and in order to solve some bug
 *         - the accumulation buffer works (it is  bit slow beacuase it requires
 *           to read/write from/to the Voodoo frame buffer but it works)
 *         - fixed a bug in the fxDDReadRGBASpan driver function (the R and
 *           B channels were read in the wrong order). Thanks to Jason Heym
 *           for highlighting the problem
 *         - written the support for multiple contexts on multiple boards.
 *           you can now use the Mesa/Voodoo with multiple Voodoo Graphics
 *           boards (for example with multiple screens or an HMD)
 *         - the fxBestResolution() now check all available resolutions
 *           and it is able to check the amount of framebuffer memory
 *           before return a resolution
 *         - modified the GLX/X11 driver in order to support all the
 *           resolution available
 *         - changed all function names. They should be now a bit more
 *           readable
 *         - written the Glide grVertex setup code for two TMU or
 *           for Multitexture support with emulationa dn one TMU
 *         - written the support for the new Mesa driver
 *           function GetParametri
 *         - small optimization/clean up in the texbind() function
 *         - fixed a FPU precision problem for glOrtho and texture
 *           mapping (thanks to Antti Juhani Huovilainen for an example
 *           of the problem)
 *         - written some small SGI OpenGL emulation code for the wgl,
 *           the OpenGL Optimizer and Cosmo3D work fine under windows !
 *         - moved the point/line/triangle/quad support in the fxmesa7.c
 *         - fixed a bug in the clear_color_depth() (thanks to Henk Kok
 *           for an example of the problem)
 *         - written a small workaround for Linux GLQuake, it asks
 *           for the alpha buffer and the depth buffer at the some time
 *           (but it never uses the alpha buffer)
 *         - checked the antialiasing points, lines and polygons support.
 *           It works fine
 *         - written the support for standard OpenGL antialiasing using
 *           blending. Lines support works fine (tested with BZflag)
 *           while I have still to check the polygons and points support
 *         - written the support for the alpha buffer. The driver is now
 *           able to use the Voodoo auxiliary buffer as an alpha buffer
 *           instead of a depth buffer. Also all the OpenGL blending
 *           modes are now supported. But you can't request a context
 *           with an alpha buffer AND a depth buffer at the some time
 *           (this is an hardware limitation)
 *         - written the support for switching between the fullscreen
 *           rendering and the in-window-rendering hack on the fly
 *
 *         Rune Hasvold (runeh@ifi.uio.no)
 *         - fixed a bug in the texparam() function
 *
 *         Brian Paul (brianp@elastic.avid.com) Avid Technology
 *         - sources accomodated for the new Mesa 3.0beta1
 *
 * V0.22 - David Bucciarelli (tech.hmw@plus.it) Humanware s.r.l.
 *         - included with some v0.23 bug fix in the final release
 *           of the Mesa-2.6
 *         - written the support for the MESA_WGL_FX env. var. but
 *           not tested because I have only Voodoo Graphics boards
 *         - fixed a bug in the backface culling code
 *           (thanks to David Farrell for an example of the problem)
 *         - fixed the "Quake2 elevator" bug
 *         - GL_POLYGONS with 3/4 vertices are now drawn as
 *           GL_TRIANLGES/GL_QUADS (a small optimization for GLQuake)
 *         - fixed a bug in fxmesa6.h for GL_LINE_LOOP
 *         - fixed a NearFarStack bug in the Mesa when applications
 *           directly call glLoadMatrix to load a projection matrix 
 *         - done some cleanup in the fxmesa2.c file
 *         - the driver no longer translates the texture maps
 *           when the Mesa internal format and the Voodoo
 *           format are the some (usefull for 1 byte texture maps
 *           where the driver can directly use the Mesa texture
 *           map). Also the amount of used memory is halfed
 *         - fixed a bug for GL_DECAL and GL_RGBA
 *         - fixed a bug in the clear_color_depth()
 *         - tested the v0.22 with the Mesa-2.6beta2. Impressive
 *           performances improvement thanks to the new Josh's
 *           asm code (+10fps in the isosurf demo, +5fps in GLQuake
 *           TIMEREFRESH)
 *         - written a optimized version of the RenderVB Mesa driver
 *           function. The Voodoo driver is now able to upload polygons
 *           in the most common cases at a very good speed. Good
 *           performance improvement for large set of small polygons
 *         - optimized the asm code for setting up the color information
 *           in the Glide grVertex structure
 *         - fixed a bug in the fxmesa2.c asm code (the ClipMask[]
 *           wasn't working)
 *
 *         Josh Vanderhoof (joshv@planet.net)
 *         - removed the flush() function because it isn't required
 *         - limited the maximum number of swapbuffers in the Voodoo
 *           commands FIFO (controlled by the env. var. MESA_FX_SWAP_PENDING)
 *
 *         Holger Kleemiss (holger.kleemiss@metronet.de) STN Atlas Elektronik GmbH
 *         - applied some patch for the Voodoo Rush
 *
 * V0.21 - David Bucciarelli (tech.hmw@plus.it) Humanware s.r.l.
 *         - the driver is now able to take advantage of the ClipMask[],
 *           ClipOrMask and ClipAndMask information also under Windows
 *         - introduced a new function in the Mesa driver interface
 *           ClearColorAndDepth(). Now the glClear() function is
 *           2 times faster (!) when you have to clear the color buffer
 *           and the depth buffer at some time
 *         - written the first version of the fxRenderVB() driver
 *           function
 *         - optimized the glTexImage() path
 *         - removed the fxMesaTextureUsePalette() support
 *         - fixed a bug in the points support (thanks to David Farrell
 *           for an example of the problem)
 *         - written the optimized path for glSubTexImage(),
 *           introduced a new function in the Mesa driver interface
 *           TexSubImage(...)
 *         - fixed a bug for glColorMask and glDepthMask
 *         - the wbuffer is not more used. The Voodoo driver uses
 *           a standard 16bit zbuffer in all cases. It is more consistent
 *           and now GLQuake and GLQuake2test work also with a GL_ZTRICK 0
 *         - the driver is now able to take advantage of the ClipMask[],
 *           ClipOrMask and ClipAndMask information (under Linux);
 *         - rewritten the setup_fx_units() function, now the texture
 *           mapping support is compliant to the OpenGL specs (GL_BLEND
 *           support is still missing). The LinuxGLQuake console correctly
 *           fade in/out and transparent water of GLQuake2test works fine
 *         - written the support for the env. var. FX_GLIDE_SWAPINTERVAL
 *         - found a bug in the Mesa core. There is a roundup problem for
 *           color values out of the [0.0,1.0] range
 *
 *         Wonko <matt@khm.de>
 *         - fixed a Voodoo Rush related problem in the fxwgl.c
 *
 *         Daryll Strauss <daryll@harlot.rb.ca.us>
 *         - written the scissor test support
 *
 * V0.20 - David Bucciarelli (tech.hmw@plus.it) Humanware s.r.l.
 *         - written the closetmmanger() function in order to free all the memory
 *           allocated by the Texture Memory Manager (it will be useful
 *           when the support for multiple contexts/boards will be ready)
 *         - now the Voodoo driver runs without printing any information,
 *           define the env. var. MESA_FX_INFO if you want to read some
 *           information about the hardware and some statistic
 *         - written a small workaround for the "GLQuake multiplayer white box bug"
 *           in the setup_fx_units() funxtions. I'm already rewriting
 *           this function because it is the source of nearly all the current
 *           Voodoo driver problems
 *         - fixed the GLQuake texture misalignment problem (the texture
 *           coordinates must be scaled between 0.0 and 256.0 and not
 *           between 0.0 and 255.0)
 *         - written the support for the GL_EXT_shared_texture_palette
 *         - some small change for supporting the automatic building of the
 *           OpenGL32.dll under the Windows platform
 *         - the redefinition of a mipmap level is now a LOT faster. This path
 *           is used by GLQuake for dynamic lighting with some call to glTexSubImage2D()
 *         - the texture memory is now managed a set of 2MB blocks so
 *           texture maps can't be allocated on a 2MB boundary. The new Pure3D
 *           needs this kind of support (and probably any other Voodoo Graphics
 *           board with more than 2MB of texture memory)
 *
 *         Brian Paul (brianp@elastic.avid.com) Avid Technology
 *         - added write_monocolor_span(), fixed bug in write_color_span()
 *         - added test for stenciling in choosepoint/line/triangle functions
 *
 *         Joe Waters (falc@attila.aegistech.com) Aegis
 *         - written the support for the env. var. SST_SCREENREFRESH
 *
 * V0.19 - David Bucciarelli (tech.hmw@plus.it) Humanware s.r.l.
 *         - written the 3Dfx Global Palette extension for GLQuake
 *         - written the support for the GL_EXT_paletted_texture (it works only with GL_RGBA
 *           palettes and the alpha value is ignored ... this is a limitation of the
 *           the current Glide version and Voodoo hardware)
 *         - fixed the amount of memory allocated for 8bit textures
 *         - merged the under construction v0.19 driver with the Mesa 2.5
 *         - finally written the support for deleting textures
 *         - introduced a new powerful texture memory manager: the texture memory
 *           is used as a cache of the set of all defined texture maps. You can
 *           now define several MB of texture maps also with a 2MB of texture memory
 *           (the texture memory manager will do automatically all the swap out/swap in
 *           work). The new texture memory manager has also
 *           solved a lot of other bugs/no specs compliance/problems
 *           related to the texture memory usage. The texture
 *           manager code is inside the new fxmesa3.c file
 *         - broken the fxmesa.c file in two files (fxmesa1.c and fxmesa2.c)
 *           and done some code cleanup
 *         - now is possible to redefine texture mipmap levels already defined
 *         - fixed a problem with the amount of texture memory allocated for textures
 *           with not all mipmap levels defined
 *         - fixed a small problem with single buffer rendering
 *
 *         Brian Paul (brianp@elastic.avid.com) Avid Technology
 *         - read/write_color_span() now use front/back buffer correctly
 *         - create GLvisual with 5,6,5 bits per pixel, not 8,8,8
 *         - removed a few ^M characters from fxmesa2.c file
 *
 * V0.18 - David Bucciarelli (tech.hmw@plus.it) Humanware s.r.l.
 *         - the Mesa-2.4beta3 is finally using the driver quads support (the
 *           previous Mesa versions have never taken any advantage from the quads support !)
 *         - tested with the Glide 2.4 for Win
 *         - ported all asm code to Linux
 *         - ported the v0.18 to Linux (without asm code)
 *         - back to Linux !!!
 *         - optimized the SETUP macro (no more vertex snap for points and lines)
 *         - optimized the SETUP macro (added one argument)
 *         - the Mesa/Voodoo is now 20/30% for points, lines and small triangles !
 *         - performance improvement setting VBSIZE to 72 
 *         - the GrVertex texture code is now written in asm
 *         - the GrVertex zbuffer code is now written in asm
 *         - the GrVertex wbuffer code is now written in asm
 *         - the GrVertex gouraud code is now written in asm
 *         - the GrVertex snap code is now written in asm
 *         - changed the 8bit compressed texture maps in 8bit palette texture maps
 *           support (it has the some advantage of compressed texture maps without the
 *           problem of a fixed NCC table for all mipmap levels)
 *         - written the support for 8bit compressed texture maps (but texture maps with
 *           more than one mipmap level aren't working fine)
 *         - finnaly everthing is working fine in MesaQuake !
 *         - fixed a bug in the computation of texture mapping coordinates (I have found
 *           the bug thanks to MesaQuake !)
 *         - written the GL_REPLACE support (mainly for MesaQuake)
 *         - written the support for textures with not all mipmap levels defined
 *         - rewritten all the Texture memory stuff
 *         - written the MesaQuake support (define MESAQUAKE)
 *         - working with a ZBuffer if glOrtho or not int the default glDepthRange,
 *           otherwise working with the WBuffer
 *         written the glDepthRange support
 *
 *         Diego Picciani (d.picciani@novacomp.it) Nova Computer s.r.l.
 *         - written the fxCloseHardware() and the fxQuaryHardware() (mainly
 *           for the VoodooWGL emulator)
 *
 *         Brian Paul (brianp@elastic.avid.com) Avid Technology
 *         - implemented read/write_color_span() so glRead/DrawPixels() works
 *         - now needs Glide 2.3 or later.  Removed GLIDE_FULL_SCREEN and call to grSstOpen()
 *
 * V0.17 - David Bucciarelli (tech.hmw@plus.it) Humanware s.r.l.
 *         - optimized the bitmap support (66% faster)
 *         - tested with the Mesa 2.3beta2
 *
 *         Diego Picciani (d.picciani@novacomp.it) Nova Computer s.r.l.
 *         - solved a problem with the drawbitmap() and the Voodoo Rush
 *           (GR_ORIGIN_LOWER_LEFT did not work with the Stingray)
 *
 *         Brian Paul (brianp@elastic.avid.com) Avid Technology
 *         - linux stuff
 *         - general code clean-up
 *         - added attribList parameter to fxMesaCreateContext()
 *         - single buffering works now
 *         - VB colors are now GLubytes, removed ColorShift stuff
 *
 *         Paul Metzger
 *         - linux stuff
 *
 * V0.16 - David Bucciarelli (tech.hmw@plus.it) Humanware s.r.l.
 *         - written the quadfunc support (no performance improvement)
 *         - written the support for the new Mesa 2.3beta1 driver interface (Wow ! It is faaaster)
 *         - rewritten the glBitmap support for the Glide 2.3 (~35% slower !)
 *         - written the glBitmap support for the most common case (fonts)
 *
 *         Jack Palevich
 *         - Glide 2.3 porting
 *
 *         Diego Picciani (d.picciani@novacomp.it) Nova Computer s.r.l.
 *         - extended the fxMesaCreateContext() and fxMesaCreateBestContext()
 *           functions in order to support also the Voodoo Rush
 *         - tested with the Hercules Stingray 128/3D (The rendering in a window works !)
 *
 * V0.15 - David Bucciarelli (tech.hmw@plus.it) Humanware s.r.l.
 *         - written the GL_LUMINANCE_ALPHA support
 *         - written the GL_ALPHA support
 *         - written the GL_LUMINANCE support
 *         - now SETUP correctly set color for mono color sequences
 *         - written the 9x1,10x1,...,1x9,1x10,... texture map ratio support
 *         - written the no square texture map support
 *         - the fog table is no more rebuilt inside setup_fx_units() each time
 *
 *         Henri Fousse (arnaud@pobox.oleane.com) Thomson Training & Simulation
 *         - written (not yet finished: no texture mapping) support for glOrtho
 *         - some change to setup functions
 *         - the fog support is now fully compatible with the standard OpenGL
 *         - rewritten several parts of the driver in order to take
 *           advantage of meshes (40% faster !!!)
 *
 * V0.14 - David Bucciarelli (tech.hmw@plus.it) Humanware s.r.l.
 *         - now glAlphaFunc() works
 *         - now glDepthMask() works
 *         - solved a mipmap problem when using more than one texture
 *         - moved ti, texid and wscale inside the fxMesaContext (now we can
 *           easy support more ctx and more boards)
 *         - the management of the fxMesaContext was completly broken !
 *         - solved several problems about Alpha and texture Alpha
 *         - 4 (RGBA) texture channels supported
 *         - setting the default color to white
 *
 *         Henri Fousse (arnaud@pobox.oleane.com) Thomson Training & Simulation
 *         - small change to fxMesaCreateContext() and fxMesaMakeCurrent()
 *         - written the fog support
 *         - setting the default clear color to black
 *         - written cleangraphics() for the onexit() function
 *         - written fxMesaCreateBestContext()
 *
 * V0.13 - David Bucciarelli (tech.hmw@plus.it) Humanware s.r.l.
 *         - now glBlendFunc() works for all glBlendFunc without DST_ALPHA
 *           (because the alpha buffer is not yet implemented) 
 *         - now fxMesaCreateContext() accept resolution and refresh rate
 *         - fixed a bug for texture mapping: the w (alias z) must be set
 *           also without depth buffer
 *         - fixed a bug for texture image with width!=256
 *         - written texparam()
 *         - written all point, line and triangle functions for all possible supported
 *           contexts and the driver is slight faster with points, lines and small triangles
 *         - fixed a small bug in fx/fxmesa.h (glOrtho)
 *
 * V0.12 - David Bucciarelli (tech.hmw@plus.it) Humanware s.r.l.
 *         - glDepthFunc supported
 *         - introduced a trick to discover the far plane distance
 *           (see fxMesaSetFar and fx/fxmesa.h)
 *         - now the wbuffer works with homogeneous coordinate (and it
 *           doesn't work with a glOrtho projection :)
 *         - solved several problems with homogeneous coordinate and texture mapping
 *         - fixed a bug in all line functions
 *         - fixed a clear framebuffer bug
 *         - solved a display list/teximg problem (but use
 *           glBindTexture: it is several times faster)
 *
 * V0.11 - David Bucciarelli (tech.hmw@plus.it) Humanware s.r.l.
 *         - introduced texture mapping support (not yet finished !)
 *         - tested with Mesa2.2b6
 *         - the driver is faster 
 *         - written glFlush/glFinish
 *         - the driver print a lot of info about the Glide lib
 *
 * v0.1  - David Bucciarelli (tech.hmw@plus.it) Humanware s.r.l.
 *         - Initial revision
 *
 */


#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#if defined(FX)
#include "fxdrv.h"

fxMesaContext fxMesaCurrentCtx=NULL;

/*
 * Status of 3Dfx hardware initialization
 */

static int glbGlideInitialized=0;
static int glb3DfxPresent=0;
static int glbTotNumCtx=0;

GrHwConfiguration glbHWConfig;
int glbCurrentBoard=0;


#if defined(__WIN32__)
static int cleangraphics(void)
{
  glbTotNumCtx=1;
  fxMesaDestroyContext(fxMesaCurrentCtx);

  return 0;
}
#elif defined(__linux__)
static void cleangraphics(void)
{
  glbTotNumCtx=1;
  fxMesaDestroyContext(fxMesaCurrentCtx);
}

static void cleangraphics_handler(int s)
{
  fprintf(stderr,"fxmesa: Received a not handled signal %d\n",s);

  cleangraphics();
/*    abort(); */
  exit(1);
}
#endif


/*
 * Select the Voodoo board to use when creating
 * a new context.
 */
GLboolean GLAPIENTRY fxMesaSelectCurrentBoard(int n)
{
  fxQueryHardware();

  if((n<0) || (n>=glbHWConfig.num_sst))
    return GL_FALSE;

  glbCurrentBoard=n;

  return GL_TRUE;
}


fxMesaContext GLAPIENTRY fxMesaGetCurrentContext(void)
{
  return fxMesaCurrentCtx;
}


void GLAPIENTRY fxMesaSetNearFar(GLfloat n, GLfloat f)
{
  if(fxMesaCurrentCtx)
    fxDDSetNearFar(fxMesaCurrentCtx->glCtx,n,f);
}


/*
 * The extension GL_FXMESA_global_texture_lod_bias
 */
void GLAPIENTRY glGlobalTextureLODBiasFXMESA(GLfloat biasVal)
{
  grTexLodBiasValue(GR_TMU0,biasVal);

  if(fxMesaCurrentCtx->haveTwoTMUs)
    grTexLodBiasValue(GR_TMU1,biasVal);
}


/*
 * The 3Dfx Global Palette extension for GLQuake.
 * More a trick than a real extesion, use the shared global
 * palette extension. 
 */
void GLAPIENTRY gl3DfxSetPaletteEXT(GLuint *pal)
{
  fxMesaContext fxMesa =fxMesaCurrentCtx;
  
  if (MESA_VERBOSE&VERBOSE_DRIVER) {
    int i;

    fprintf(stderr,"fxmesa: gl3DfxSetPaletteEXT()\n");

    for(i=0;i<256;i++)
      fprintf(stderr,"%x\n",pal[i]);
  }
  
  if(fxMesa) {
    fxMesa->haveGlobalPaletteTexture=1;
    
    FX_grTexDownloadTable(GR_TMU0,GR_TEXTABLE_PALETTE,(GuTexPalette *)pal);
    if (fxMesa->haveTwoTMUs)
    	 FX_grTexDownloadTable(GR_TMU1,GR_TEXTABLE_PALETTE,(GuTexPalette *)pal);
  }
}


static GrScreenResolution_t fxBestResolution(int width, int height, int aux)
{
  static int resolutions[][5]={ 
    { 512, 384, GR_RESOLUTION_512x384, 2, 2 },
    { 640, 400, GR_RESOLUTION_640x400, 2, 2 },
    { 640, 480, GR_RESOLUTION_640x480, 2, 2 },
    { 800, 600, GR_RESOLUTION_800x600, 4, 2 },
    { 960, 720, GR_RESOLUTION_960x720, 6, 4 }
#ifdef GR_RESOLUTION_1024x768
    ,{ 1024, 768, GR_RESOLUTION_1024x768, 8, 4 }
#endif
#ifdef GR_RESOLUTION_1280x1024
    ,{ 1024, 768, GR_RESOLUTION_1280x1024, 8, 8 }
#endif
#ifdef GR_RESOLUTION_1600x1200
    ,{ 1024, 768, GR_RESOLUTION_1600x1200, 16, 8 }
#endif
  };
  int NUM_RESOLUTIONS = sizeof(resolutions) / (sizeof(int)*5);
  int i,fbmem;
  GrScreenResolution_t lastvalidres=resolutions[1][2];

  fxQueryHardware();

  if(glbHWConfig.SSTs[glbCurrentBoard].type==GR_SSTTYPE_VOODOO) {
    fbmem=glbHWConfig.SSTs[glbCurrentBoard].sstBoard.VoodooConfig.fbRam;

    if(glbHWConfig.SSTs[glbCurrentBoard].sstBoard.VoodooConfig.sliDetect)
      fbmem*=2;
  } else if(glbHWConfig.SSTs[glbCurrentBoard].type==GR_SSTTYPE_SST96)
    fbmem=glbHWConfig.SSTs[glbCurrentBoard].sstBoard.SST96Config.fbRam;
  else
    fbmem=2;

  /* A work around for BZFlag */

  if((width==1) && (height==1)) {
    width=640;
    height=480;
  }

  for(i=0;i<NUM_RESOLUTIONS;i++)
    if(resolutions[i][4-aux]<=fbmem) {
      if((width<=resolutions[i][0]) && (height<=resolutions[i][1]))
        return resolutions[i][2];

      lastvalidres=resolutions[i][2];
    }

  return lastvalidres;
}


fxMesaContext GLAPIENTRY fxMesaCreateBestContext(GLuint win,GLint width, GLint height,
					       const GLint attribList[])
{
  GrScreenRefresh_t refresh;
  int i;
  int res,aux;
  refresh=GR_REFRESH_75Hz;

  if(getenv("SST_SCREENREFRESH")) {
    if(!strcmp(getenv("SST_SCREENREFRESH"),"60"))
      refresh=GR_REFRESH_60Hz;
    if(!strcmp(getenv("SST_SCREENREFRESH"),"70"))
      refresh=GR_REFRESH_70Hz;
    if(!strcmp(getenv("SST_SCREENREFRESH"),"72"))
      refresh=GR_REFRESH_72Hz;
    if(!strcmp(getenv("SST_SCREENREFRESH"),"75"))
      refresh=GR_REFRESH_75Hz;
    if(!strcmp(getenv("SST_SCREENREFRESH"),"80"))
      refresh=GR_REFRESH_80Hz;
    if(!strcmp(getenv("SST_SCREENREFRESH"),"85"))
      refresh=GR_REFRESH_85Hz;
    if(!strcmp(getenv("SST_SCREENREFRESH"),"90"))
      refresh=GR_REFRESH_90Hz;
    if(!strcmp(getenv("SST_SCREENREFRESH"),"100"))
      refresh=GR_REFRESH_100Hz;
    if(!strcmp(getenv("SST_SCREENREFRESH"),"120"))
      refresh=GR_REFRESH_120Hz;
  }

  aux=0;
  for(i=0;attribList[i]!=FXMESA_NONE;i++)
    if((attribList[i]==FXMESA_ALPHA_SIZE) ||
       (attribList[i]==FXMESA_DEPTH_SIZE)) {
      if(attribList[++i]>0) {
        aux=1;
        break;
      }
    }

  res=fxBestResolution(width,height,aux);

  return fxMesaCreateContext(win,res,refresh,attribList);
}


#if 0
void fxsignals()
{
   signal(SIGINT,SIG_IGN);
   signal(SIGHUP,SIG_IGN);
   signal(SIGPIPE,SIG_IGN);
   signal(SIGFPE,SIG_IGN);
   signal(SIGBUS,SIG_IGN);
   signal(SIGILL,SIG_IGN);
   signal(SIGSEGV,SIG_IGN);
   signal(SIGTERM,SIG_IGN);
}
#endif

/*
 * Create a new FX/Mesa context and return a handle to it.
 */
fxMesaContext GLAPIENTRY fxMesaCreateContext(GLuint win,GrScreenResolution_t res,
					   GrScreenRefresh_t ref,
					   const GLint attribList[])
{
   fxMesaContext fxMesa = NULL;
   int i,type;
   int aux;
   GLboolean doubleBuffer=GL_FALSE;
   GLboolean alphaBuffer=GL_FALSE;
   GLboolean verbose=GL_FALSE;
   GLint depthSize=0;
   GLint stencilSize=0;
   GLint accumSize=0;
   GLcontext *shareCtx = NULL;
   GLcontext *ctx = 0;
   FX_GrContext_t glideContext = 0;
   char *errorstr;

   if (MESA_VERBOSE&VERBOSE_DRIVER) {
      fprintf(stderr,"fxmesa: fxMesaCreateContext() Start\n");
   }

   if(getenv("MESA_FX_INFO"))
      verbose=GL_TRUE;

   aux=0;
   i=0;
   while(attribList[i]!=FXMESA_NONE) {
      switch (attribList[i]) {
      case FXMESA_DOUBLEBUFFER:
	 doubleBuffer=GL_TRUE;
	 break;
      case FXMESA_ALPHA_SIZE:
	 i++;
	 alphaBuffer=attribList[i]>0;
	 if(alphaBuffer)
	    aux=1;
	 break;
      case FXMESA_DEPTH_SIZE:
	 i++;
	 depthSize=attribList[i];
	 if(depthSize)
	    aux=1;
	 break;
      case FXMESA_STENCIL_SIZE:
	 i++;
	 stencilSize=attribList[i];
	 break;
      case FXMESA_ACCUM_SIZE:
	 i++;
	 accumSize=attribList[i];
	 break;
	 /* XXX ugly hack here for sharing display lists */
#define FXMESA_SHARE_CONTEXT 990099  /* keep in sync with xmesa1.c! */
      case FXMESA_SHARE_CONTEXT:
	 i++;
	 {
	    const void *vPtr = &attribList[i];
	    GLcontext **ctx = (GLcontext **) vPtr;
	    shareCtx = *ctx;
	 }
	 break;
      default:
	 if (MESA_VERBOSE&VERBOSE_DRIVER) {
	    fprintf(stderr,"fxmesa: fxMesaCreateContext() End (defualt)\n");
	 }
	 return NULL;
      }
      i++;
   }

   /* A workaround for Linux GLQuake */
   if(depthSize && alphaBuffer)
      alphaBuffer=0;

   if(verbose)
      fprintf(stderr,"Mesa fx Voodoo Device Driver v0.30\nWritten by David Bucciarelli (davibu@tin.it.it)\n");

   if((type=fxQueryHardware()) < 0) {
      fprintf(stderr,"fx Driver: ERROR no Voodoo1/2 Graphics or Voodoo Rush !\n");
      return NULL;
   }

   if(type==GR_SSTTYPE_VOODOO)
      win=0;

   grSstSelect(glbCurrentBoard);

   fxMesa=(fxMesaContext)calloc(1,sizeof(struct tfxMesaContext));
   if(!fxMesa) {
      errorstr = "malloc";
      goto errorhandler;
   }

   if(glbHWConfig.SSTs[glbCurrentBoard].type==GR_SSTTYPE_VOODOO)
      fxMesa->haveTwoTMUs=(glbHWConfig.SSTs[glbCurrentBoard].sstBoard.VoodooConfig.nTexelfx > 1);
   else if(glbHWConfig.SSTs[glbCurrentBoard].type==GR_SSTTYPE_SST96)
      fxMesa->haveTwoTMUs=(glbHWConfig.SSTs[glbCurrentBoard].sstBoard.SST96Config.nTexelfx > 1);
   else
      fxMesa->haveTwoTMUs=GL_FALSE;

   fxMesa->haveDoubleBuffer=doubleBuffer;
   fxMesa->haveAlphaBuffer=alphaBuffer;
   fxMesa->haveGlobalPaletteTexture=GL_FALSE;
   fxMesa->haveZBuffer=depthSize ? 1 : 0;
   fxMesa->verbose=verbose;
   fxMesa->board=glbCurrentBoard;


   fxMesa->glideContext = FX_grSstWinOpen((FxU32)win,res,ref,
#if  FXMESA_USE_ARGB
					  GR_COLORFORMAT_ARGB,
#else
					  GR_COLORFORMAT_ABGR,
#endif
					  GR_ORIGIN_LOWER_LEFT,
					  2,aux);
   if (!fxMesa->glideContext){
      errorstr = "grSstWinOpen"; 
      goto errorhandler;
   }


   fxMesa->width=FX_grSstScreenWidth();
   fxMesa->height=FX_grSstScreenHeight();

   if(verbose)
      fprintf(stderr,"Glide screen size: %dx%d\n",
              (int)FX_grSstScreenWidth(),(int)FX_grSstScreenHeight());

   fxMesa->glVis=gl_create_visual(GL_TRUE,     /* RGB mode */
				  alphaBuffer,
				  doubleBuffer,
				  GL_FALSE,    /* stereo */
				  depthSize,   /* depth_size */
				  stencilSize, /* stencil_size */
				  accumSize,   /* accum_size */
				  0,           /* index bits */
				  5,6,5,0);    /* RGBA bits */
   if (!fxMesa->glVis) {
      errorstr = "gl_create_visual";
      goto errorhandler;
   }

   ctx = fxMesa->glCtx=gl_create_context(fxMesa->glVis,
					 shareCtx,  /* share list context */
					 (void *) fxMesa, GL_TRUE);
   if (!ctx) {
      errorstr = "gl_create_context";
      goto errorhandler;
   }


   if (!fxDDInitFxMesaContext( fxMesa )) {
      errorstr = "fxDDInitFxMesaContext failed"; 
      goto errorhandler;
   }


   fxMesa->glBuffer=gl_create_framebuffer(fxMesa->glVis);
   if (!fxMesa->glBuffer) {
      errorstr = "gl_create_framebuffer";
      goto errorhandler;
   }
  
   glbTotNumCtx++;

   /* install signal handlers */
#if defined(__linux__)
   if (fxMesa->glCtx->CatchSignals) {
      signal(SIGINT,cleangraphics_handler);
      signal(SIGHUP,cleangraphics_handler);
      signal(SIGPIPE,cleangraphics_handler);
      signal(SIGFPE,cleangraphics_handler);
      signal(SIGBUS,cleangraphics_handler);
      signal(SIGILL,cleangraphics_handler);
      signal(SIGSEGV,cleangraphics_handler);
      signal(SIGTERM,cleangraphics_handler);
   }
#endif

   if (MESA_VERBOSE&VERBOSE_DRIVER) {
      fprintf(stderr,"fxmesa: fxMesaCreateContext() End\n");
   }

   return fxMesa;

 errorhandler:
   if (fxMesa) {
      if (fxMesa->glideContext)
	 FX_grSstWinClose(fxMesa->glideContext);
      fxMesa->glideContext = 0;
      
      if (fxMesa->state)  
	 free(fxMesa->state);
      if (fxMesa->fogTable)
	 free(fxMesa->fogTable);
      if (fxMesa->glBuffer)
	 gl_destroy_framebuffer(fxMesa->glBuffer);
      if (fxMesa->glVis)
	 gl_destroy_visual(fxMesa->glVis);
      if (fxMesa->glCtx)
	 gl_destroy_context(fxMesa->glCtx);
      free(fxMesa);
   }
   
   if (MESA_VERBOSE&VERBOSE_DRIVER) {
      fprintf(stderr,"fxmesa: fxMesaCreateContext() End (%s)\n",errorstr);
   }
   return NULL;
}


/*
 * Function to set the new window size in the context (mainly for the Voodoo Rush)
 */
void GLAPIENTRY fxMesaUpdateScreenSize(fxMesaContext fxMesa)
{
  fxMesa->width=FX_grSstScreenWidth();
  fxMesa->height=FX_grSstScreenHeight();
}


/*
 * Destroy the given FX/Mesa context.
 */
void GLAPIENTRY fxMesaDestroyContext(fxMesaContext fxMesa)
{
  if (MESA_VERBOSE&VERBOSE_DRIVER) {
    fprintf(stderr,"fxmesa: fxMesaDestroyContext()\n");
  }

  if(fxMesa) {
    gl_destroy_visual(fxMesa->glVis);
    gl_destroy_context(fxMesa->glCtx);
    gl_destroy_framebuffer(fxMesa->glBuffer);

    glbTotNumCtx--;

    fxCloseHardware();
    FX_grSstWinClose(fxMesa->glideContext);

    if(fxMesa->verbose) {
      fprintf(stderr,"Misc Stats:\n");
      fprintf(stderr,"  # swap buffer: %u\n",fxMesa->stats.swapBuffer);

      if(!fxMesa->stats.swapBuffer)
        fxMesa->stats.swapBuffer=1;

      fprintf(stderr,"Textures Stats:\n");
      fprintf(stderr,"  Free texture memory on TMU0: %d:\n",fxMesa->freeTexMem[FX_TMU0]);
      if(fxMesa->haveTwoTMUs)
        fprintf(stderr,"  Free texture memory on TMU1: %d:\n",fxMesa->freeTexMem[FX_TMU1]);
      fprintf(stderr,"  # request to TMM to upload a texture objects: %u\n",
              fxMesa->stats.reqTexUpload);
      fprintf(stderr,"  # request to TMM to upload a texture objects per swapbuffer: %.2f\n",
              fxMesa->stats.reqTexUpload/(float)fxMesa->stats.swapBuffer);
      fprintf(stderr,"  # texture objects uploaded: %u\n",
              fxMesa->stats.texUpload);
      fprintf(stderr,"  # texture objects uploaded per swapbuffer: %.2f\n",
              fxMesa->stats.texUpload/(float)fxMesa->stats.swapBuffer);
      fprintf(stderr,"  # MBs uploaded to texture memory: %.2f\n",
              fxMesa->stats.memTexUpload/(float)(1<<20));
      fprintf(stderr,"  # MBs uploaded to texture memory per swapbuffer: %.2f\n",
              (fxMesa->stats.memTexUpload/(float)fxMesa->stats.swapBuffer)/(float)(1<<20));
    }
    if (fxMesa->state)  
       free(fxMesa->state);
    if (fxMesa->fogTable)
       free(fxMesa->fogTable);
    fxTMClose(fxMesa);
    
    free(fxMesa);
  }

  if(fxMesa==fxMesaCurrentCtx)
    fxMesaCurrentCtx=NULL;
}


/*
 * Make the specified FX/Mesa context the current one.
 */
void GLAPIENTRY fxMesaMakeCurrent(fxMesaContext fxMesa)
{
  if (MESA_VERBOSE&VERBOSE_DRIVER) {
    fprintf(stderr,"fxmesa: fxMesaMakeCurrent(...) Start\n");
  }

  if(!fxMesa) {
    gl_make_current(NULL,NULL);
    fxMesaCurrentCtx=NULL;

    if (MESA_VERBOSE&VERBOSE_DRIVER) {
      fprintf(stderr,"fxmesa: fxMesaMakeCurrent(NULL) End\n");
    }

    return;
  }

  /* if this context is already the current one, we can return early */
  if (fxMesaCurrentCtx == fxMesa
      && fxMesaCurrentCtx->glCtx == gl_get_current_context()) {
     if (MESA_VERBOSE&VERBOSE_DRIVER) {
        fprintf(stderr,"fxmesa: fxMesaMakeCurrent(fxMesaCurrentCtx==fxMesa) End\n");
    }

    return;
  }

  if(fxMesaCurrentCtx)
    grGlideGetState((GrState*)fxMesaCurrentCtx->state);

  fxMesaCurrentCtx=fxMesa;

  grSstSelect(fxMesa->board);
  grGlideSetState((GrState*)fxMesa->state);

  gl_make_current(fxMesa->glCtx,fxMesa->glBuffer);

  fxSetupDDPointers(fxMesa->glCtx);

  /* The first time we call MakeCurrent we set the initial viewport size */
  if(fxMesa->glCtx->Viewport.Width==0)
    gl_Viewport(fxMesa->glCtx,0,0,fxMesa->width,fxMesa->height);

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
    fprintf(stderr,"fxmesa: fxMesaMakeCurrent(...) End\n");
  }
}


/*
 * Swap front/back buffers for current context if double buffered.
 */
void GLAPIENTRY fxMesaSwapBuffers(void)
{
  if (MESA_VERBOSE&VERBOSE_DRIVER) {
    fprintf(stderr,"fxmesa: ------------------------------- fxMesaSwapBuffers() -------------------------------\n");
  }

  if(fxMesaCurrentCtx) {
   FLUSH_VB( fxMesaCurrentCtx->glCtx, "swap buffers" );

    if(fxMesaCurrentCtx->haveDoubleBuffer) {

      grBufferSwap(fxMesaCurrentCtx->swapInterval);

      /*
       * Don't allow swap buffer commands to build up!
       */
      while(FX_grGetInteger(FX_PENDING_BUFFERSWAPS)>fxMesaCurrentCtx->maxPendingSwapBuffers)
        /* The driver is able to sleep when waiting for the completation
           of multiple swapbuffer operations instead of wasting
           CPU time (NOTE: you must uncomment the following line in the
           in order to enable this option) */
        /* usleep(10000); */
        ;

      fxMesaCurrentCtx->stats.swapBuffer++;
    }
  }
}


/*
 * Query 3Dfx hardware presence/kind
 */
int GLAPIENTRY fxQueryHardware(void)
{
  if (MESA_VERBOSE&VERBOSE_DRIVER) {
    fprintf(stderr,"fxmesa: fxQueryHardware() Start\n");
  }

  if(!glbGlideInitialized) {
    grGlideInit();
    if(FX_grSstQueryHardware(&glbHWConfig)) {
      grSstSelect(glbCurrentBoard);
      glb3DfxPresent=1;

      if(getenv("MESA_FX_INFO")) {
        char buf[80];
                        
        FX_grGlideGetVersion(buf);
        fprintf(stderr,"Using Glide V%s\n",0);
        fprintf(stderr,"Number of boards: %d\n",glbHWConfig.num_sst);

        if(glbHWConfig.SSTs[glbCurrentBoard].type==GR_SSTTYPE_VOODOO) {
          fprintf(stderr,"Framebuffer RAM: %d\n",
                  glbHWConfig.SSTs[glbCurrentBoard].sstBoard.VoodooConfig.sliDetect ?
                  (glbHWConfig.SSTs[glbCurrentBoard].sstBoard.VoodooConfig.fbRam*2) :
                  glbHWConfig.SSTs[glbCurrentBoard].sstBoard.VoodooConfig.fbRam);
          fprintf(stderr,"Number of TMUs: %d\n",
                  glbHWConfig.SSTs[glbCurrentBoard].sstBoard.VoodooConfig.nTexelfx);
          fprintf(stderr,"SLI detected: %d\n",
                  glbHWConfig.SSTs[glbCurrentBoard].sstBoard.VoodooConfig.sliDetect);
        } else if(glbHWConfig.SSTs[glbCurrentBoard].type==GR_SSTTYPE_SST96) {
          fprintf(stderr,"Framebuffer RAM: %d\n",
                  glbHWConfig.SSTs[glbCurrentBoard].sstBoard.SST96Config.fbRam);
          fprintf(stderr,"Number of TMUs: %d\n",
                  glbHWConfig.SSTs[glbCurrentBoard].sstBoard.SST96Config.nTexelfx);
        }

      }
    } else
      glb3DfxPresent=0;

    glbGlideInitialized=1;

#if defined(__WIN32__)
    onexit((_onexit_t)cleangraphics);
#elif defined(__linux__)
    atexit(cleangraphics);
#endif
  }

  if(!glb3DfxPresent) {
    if (MESA_VERBOSE&VERBOSE_DRIVER) {
      fprintf(stderr,"fxmesa: fxQueryHardware() End (-1)\n");
    }
    return(-1);
  }

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
    fprintf(stderr,"fxmesa: fxQueryHardware() End (voodooo)\n");
  }
  return(glbHWConfig.SSTs[glbCurrentBoard].type);
}


/*
 * Shutdown Glide library
 */
void GLAPIENTRY fxCloseHardware(void)
{
  if(glbGlideInitialized) {
    if(getenv("MESA_FX_INFO")) {
      GrSstPerfStats_t          st;

      FX_grSstPerfStats(&st);
      fprintf(stderr,"Pixels Stats:\n");
      fprintf(stderr,"  # pixels processed (minus buffer clears): %u\n",(unsigned)st.pixelsIn);
      fprintf(stderr,"  # pixels not drawn due to chroma key test failure: %u\n",(unsigned)st.chromaFail);
      fprintf(stderr,"  # pixels not drawn due to depth test failure: %u\n",(unsigned)st.zFuncFail);
      fprintf(stderr,"  # pixels not drawn due to alpha test failure: %u\n",(unsigned)st.aFuncFail);
      fprintf(stderr,"  # pixels drawn (including buffer clears and LFB writes): %u\n",(unsigned)st.pixelsOut);
    }

    if(glbTotNumCtx==0) {
      grGlideShutdown();
      glbGlideInitialized=0;
    }
  }
}


#else


/*
 * Need this to provide at least one external definition.
 */

int gl_fx_dummy_function_api(void)
{
  return 0;
}

#endif  /* FX */
