XXX - Not complete yet!!!

Name

     MESA_trace

Name Strings

     GL_MESA_TRACE

Contact
    
    Bernd Kreimeier, Loki Entertainment, bk 'at' lokigames.com
    Brian Paul, VA Linux Systems, Inc., brianp 'at' valinux.com

Status

    XXX - Not complete yet!!!

Version

    $Id: MESA_trace.spec,v 1.1 2000/11/03 15:10:04 brianp Exp $

Number

    ???

Dependencies

    OpenGL 1.2 is required.
    The extension is written against the OpenGL 1.2 Specification

Overview

    Provides the application with means to enable and disable logging
    of GL calls including parameters as readable text. The verbosity
    of the generated log can be controlled. The resulting logs are
    valid (but possibly incomplete) C code and can be compiled and 
    linked for standalone test programs. The set of calls and the 
    amount of static data that is logged can be controlled at runtime. 
    The application can add comments and enable or disable tracing of GL 
    operations at any time. The data flow from the application to GL
    and back is unaffected except for timing.

    Application-side implementation of these features raises namespace
    and linkage issues. In the driver dispatch table a simple
    "chain of responsibility" pattern (aka "composable piepline")
    can be added.

IP Status

    The extension spec is in the public domain.  The current implementation
    in Mesa is covered by Mesa's XFree86-style copyright by the authors above.
    This extension is partially inspired by the Quake2 QGL wrapper.

Issues

    none yet

New Procedures and Functions
 
    void NewTraceMESA( bitfield mask, const ubyte *traceName )

    void EndTraceMESA( void )

    void EnableTraceMESA( bitfield mask )

    void DisableTraceMESA( bitfield mask )

    void TraceAssertAttribMESA( bitfield attribMask )

    void TraceCommentMESA( const ubyte *comment )

    void TraceTextureMESA( uint name, const ubyte *comment )

    void TraceListMESA( uint name, const ubyte *comment )

    void TracePointerMESA( void *pointer, const ubyte *comment )

    void TracePointerRangeMESA( const void *first, const void *last,
                                const ubyte *comment ) 

New Tokens
 
    Accepted by the <mask> parameter of EnableTrace and DisableTrace:

       TRACE_ALL_BITS_MESA           0x0001
       TRACE_OPERATIONS_BIT_MESA     0x0002
       TRACE_PRIMITIVES_BIT_MESA     0x0004
       TRACE_ARRAYS_BIT_MESA         0x0008
       TRACE_TEXTURES_BIT_MESA       0x0010
       TRACE_PIXELS_BIT_MESA         0x0020

    Accepted by the <pname> parameter of GetIntegerv, GetBooleanv,
    GetFloatv, and GetDoublev:

       TRACE_MASK_MESA               0x8755

    Accepted by the <pname> parameter to GetString:

       TRACE_NAME_MESA               0x8756

Additions to Chapter 2 of the OpenGL 1.2.1 Specification (OpenGL Operation)

    None.

Additions to Chapter 3 of the OpenGL 1.2.1 Specification (OpenGL Operation)

    None.

Additions to Chapter 4 of the OpenGL 1.2.1 Specification (OpenGL Operation)

    None.

Additions to Chapter 5 of the OpenGL 1.2.1 Specification (Special Functions)

    Add a new section:

    5.7 Tracing

    The tracing facility is used to record the execution of a GL program
    to a human-readable log.  The log appears as a sequence of GL commands
    using C syntax.  The primary intention of tracing is to aid in program
    debugging.

    A trace is started with the command

      void NewTraceMESA( bitfield mask, const GLubyte * traceName )

    <mask> may be any value accepted by PushAttrib and specifies a set of
    attribute groups.  The state values included in those attribute groups
    is written to the trace as a sequence of GL commands.

    <traceName> specifies a name or label for the trace.  It is expected
    that <traceName> will be interpreted as a filename in most implementations.

    A trace is ended by calling the command

      void EndTraceMESA( void )

    It is illegal to call NewTrace or EndTrace between Begin and End. 

    The commands

      void EnableTraceMESA( bitfield mask )
      void DisableTraceMESA( bitfield mask )

    enable or disable tracing of different classes of GL commands.
    <mask> may be the union of any of TRACE_OPERATIONS_BIT_MESA,
    TRACE_PRIMITIVES_BIT_MESA, TRACE_ARRAYS_BIT_MESA, TRACE_TEXTURES_BIT_MESA,
    and TRACE_PIXELS_BIT_MESA.  The special token TRACE_ALL_BITS_MESA
    indicates all classes of commands are to be logged.

    TRACE_OPERATIONS_BIT_MESA controls logging of all commands outside of
    Begin/End, including Begin/End.
  
    TRACE_PRIMITIVES_BIT_MESA controls logging of all commands inside of
    Begin/End, including Begin/End.

    TRACE_ARRAYS_BIT_MESA controls logging of VertexPointer, NormalPointer,
    ColorPointer, IndexPointer, TexCoordPointer and EdgeFlagPointer commands.

    TRACE_TEXTURES_BIT_MESA controls logging of texture data dereferenced by
    TexImage1D, TexImage2D, TexImage3D, TexSubImage1D, TexSubImage2D, and
    TexSubImage3D commands.

    TRACE_PIXELS_BIT_MESA controls logging of image data dereferenced by
    Bitmap and DrawPixels commands.

    The command

      void TraceCommentMESA( const ubyte* comment )

    immediately adds the <comment> string to the trace output, surrounded
    by C-style comment delimiters.

    The commands

      void TraceTextureMESA( uint name, const ubyte* comment )
      void TraceListMESA( uint name, const ubyte* comment )

    associates <comment> with the texture object or display list specified
    by <name>.  Logged commands which reference the named texture object or
    display list will be annotated with <comment>.  If IsTexture(name) or
    IsList(name) fail (respectively) the command is quietly ignored.

    The commands

      void TracePointerMESA( void* pointer, const ubyte* comment )

      void TracePointerRangeMESA( const void* first, const void* last,
                              const ubyte* comment ) 

    associate <comment> with the address specified by <pointer> or with
    a range of addresses specified by <first> through <last>.
    Any logged commands which reference <pointer> or an address between
    <first> and <last> will be annotated with <comment>.

    The command

      void TraceAssertAttribMESA( bitfield attribMask )

    will add GL state queries and assertion statements to the log to
    confirm that the current state at the time TraceAssertAttrib is
    executed matches the current state when the trace log is executed
    in the future.

    <attribMask> is any value accepted by PushAttrib and specifies
    the groups of state variables which are to be asserted.

    The commands NewTrace, EndTrace, EnableTrace, DisableTrace,
    TraceAssertAttrib, TraceComment, TraceTexture, TraceList, TracePointer,
    TracePointerRange, GetTraceName and GetTraceMask are not compiled
    into display lists.


    Examples:

    The command NewTrace(DEPTH_BUFFER_BIT, "log") will query the state
    variables DEPTH_TEST, DEPTH_FUNC, DEPTH_WRITEMASK, and DEPTH_CLEAR_VALUE
    to get the values <test>, <func>, <mask>, and <clear> respectively.
    Statements equivalent to the following will then be logged:

       glEnable(GL_DEPTH_TEST);   (if <test> is true)
       glDisable(GL_DEPTH_TEST);  (if <test> is false)
       glDepthFunc(<func>); 
       glDepthMask(<mask>);
       glClearDepth(<clear>);
   

    The command TraceAssertAttrib(DEPTH_BUFFER_BIT) will query the state
    variables DEPTH_TEST, DEPTH_FUNC, DEPTH_WRITEMASK, and DEPTH_CLEAR_VALUE
    to get the values <test>, <func>, <mask>, and <clear> respectively.
    Statements equivalent to the following will then be logged:

    {
      GLboolean b;
      GLint i;
      GLfloat f;
      b = glIsEnabled(GL_DEPTH_TEST);
      assert(b == <test>);
      glGetIntegerv(GL_DEPTH_FUNC, &i);
      assert(i == <func>);
      glGetIntegerv(GL_DEPTH_MASK, &i);
      assert(i == <mask>);
      glGetFloatv(GL_DEPTH_CLEAR_VALUE, &f);
      assert(f == <clear>);
    }


Additions to Chapter 6 of the OpenGL 1.2.1 Specification 
    (State and State Requests)

    Querying TRACE_MASK_MESA with GetIntegerv, GetFloatv, GetBooleanv or
    GetDoublev returns the current command class trace mask.

    Querying TRACE_NAME_MESA with GetString returns the current trace name.

Additions to Appendix A of the OpenGL 1.2.1 Specification (Invariance)

    The MESA_tracelog extension does not affect data flow from application 
    to OpenGL, as well as data flow from OpenGL to application, except for 
    timing, possible print I/O, and sequence of GetError queries. With
    the possible exception of performance, OpenGL rendering should not be
    affect by the logging operation.

Additions to the AGL/GLX/WGL Specifications

    None.
    ? Hooking into glXSwapBuffers() ?

GLX Protocol

    None. The logging operation is carried out client-side, by exporting
    entry points to the wrapper functions that execute the logging operation.

Errors

    INVALID_OPERATION is generated if any trace command except TraceComment
    is called between Begin and End.

New State

    The current trace name and current command class mask are stored
    per-context.

New Implementation Dependent State

    None.

Revision History

    * Revision 0.1 - Initial draft from template (bk000415)
    * Revision 0.2 - Draft (bk000906)
    * Revision 0.3 - Draft (bk000913)
    * Revision 0.4 - Added GetTraceName/Mask, fixed typos (bp000914)
    * Revision 0.5 - Assigned final GLenum values
