Name

    WL_bind_wayland_display

Name Strings

    EGL_WL_bind_wayland_display

Contact

    Kristian Høgsberg <krh@bitplanet.net>
    Benjamin Franzke <benjaminfranzke@googlemail.com>

Status

    Proposal

Version

    Version 1, March 1, 2011

Number

    EGL Extension #not assigned

Dependencies

    Requires EGL 1.4 or later.  This extension is written against the
    wording of the EGL 1.4 specification.

    EGL_KHR_base_image is required.

Overview

    This extension provides entry points for binding and unbinding the
    wl_display of a Wayland compositor to an EGLDisplay.  Binding a
    wl_display means that the EGL implementation should provide one or
    more interfaces in the Wayland protocol to allow clients to create
    wl_buffer objects.  On the server side, this extension also
    provides a new target for eglCreateImageKHR, to create an EGLImage
    from a wl_buffer

    Adding an implementation specific wayland interface, allows the
    EGL implementation to define specific wayland requests and events,
    needed for buffer sharing in an EGL wayland platform.

IP Status

    Open-source; freely implementable.

New Procedures and Functions

    EGLBoolean eglBindWaylandDisplayWL(EGLDisplay dpy,
                                       struct wl_display *display);

    EGLBoolean eglUnbindWaylandDisplayWL(EGLDisplay dpy,
                                         struct wl_display *display);

    EGLBoolean eglQueryWaylandBufferWL(EGLDisplay dpy,
                                       struct wl_buffer *buffer,
                                       EGLint attribute, EGLint *value);

New Tokens

    Accepted as <target> in eglCreateImageKHR

        EGL_WAYLAND_BUFFER_WL                   0x31D5

    Accepted in the <attrib_list> parameter of eglCreateImageKHR:

        EGL_WAYLAND_PLANE_WL                    0x31D6


    Accepted as a eglQueryWaylandBufferWL attribute:

        EGL_WAYLAND_BUFFER_COMPONENTS_WL	0x31D7

    Possible values for EGL_WAYLAND_BUFFER_COMPONENTS_WL:    

        EGL_WAYLAND_BUFFER_RGB_WL               0x31D8
        EGL_WAYLAND_BUFFER_RGBA_WL              0x31D9
        EGL_WAYLAND_BUFFER_Y_U_V_WL             0x31Da
        EGL_WAYLAND_BUFFER_Y_UV_WL              0x31Db
        EGL_WAYLAND_BUFFER_Y_XUXV_WL            0x31Dc


Additions to the EGL 1.4 Specification:

    To bind a server side wl_display to an EGLDisplay, call

        EGLBoolean eglBindWaylandDisplayWL(EGLDisplay dpy,
                                           struct wl_display *display);

    To unbind a server side wl_display from an EGLDisplay, call
    
        EGLBoolean eglUnbindWaylandDisplayWL(EGLDisplay dpy,
                                             struct wl_display *display);

    eglBindWaylandDisplayWL returns EGL_FALSE when there is already a
    wl_display bound to EGLDisplay otherwise EGL_TRUE.

    eglUnbindWaylandDisplayWL returns EGL_FALSE when there is no
    wl_display bound to the EGLDisplay currently otherwise EGL_TRUE.

    A wl_buffer can have several planes, typically in case of planar
    YUV formats.  Depending on the exact YUV format in use, the
    compositor will have to create one or more EGLImages for the
    various planes.  The eglQueryWaylandBufferWL function should be
    used to first query the wl_buffer components using
    EGL_WAYLAND_BUFFER_COMPONENTS_WL as the attribute.  If the
    wl_buffer object is not an EGL wl_buffer (wl_shm and other wayland
    extensions can create wl_buffer objects), this query will return
    EGL_FALSE.  In that case the wl_buffer can not be used with EGL
    and the compositor should have another way to get the buffer
    contents.

    If eglQueryWaylandBufferWL succeeds, the returned value will be
    one of EGL_WAYLAND_BUFFER_RGB_WL, EGL_WAYLAND_BUFFER_RGBA_WL,
    EGL_WAYLAND_BUFFER_Y_U_V_WL, EGL_WAYLAND_BUFFER_Y_UV_WL,
    EGL_WAYLAND_BUFFER_Y_XUXV_WL.  The value returned describes how
    many EGLImages must be used, which components will be sampled from
    each EGLImage and how they map to rgba components in the shader.
    The naming conventions separates planes by _ and within each
    plane, the order or R, G, B, A, Y, U, and V indicates how those
    components map to the rgba value returned by the sampler.  X
    indicates that the corresponding component in the rgba value isn't
    used.

    RGB and RGBA buffer types:

        EGL_WAYLAND_BUFFER_RGB_WL
                One plane, samples RGB from the texture to rgb in the
                shader.  Alpha channel is not valid.

        EGL_WAYLAND_BUFFER_RGBA_WL              0x31D9
                One plane, samples RGBA from the texture to rgba in the
                shader.

    YUV buffer types:

        EGL_WAYLAND_BUFFER_Y_U_V_WL             0x31Da
                Three planes, samples Y from the first plane to r in
                the shader, U from the second plane to r, and V from
                the third plane to r.

        EGL_WAYLAND_BUFFER_Y_UV_WL              0x31Db
                Two planes, samples Y from the first plane to r in
                the shader, U and V from the second plane to rg.

        EGL_WAYLAND_BUFFER_Y_XUXV_WL            0x31Dc
                Two planes, samples Y from the first plane to r in
                the shader, U and V from the second plane to g and a.

    After querying the wl_buffer layout, create EGLImages for the
    planes by calling eglCreateImageKHR with wl_buffer as
    EGLClientBuffer, EGL_WAYLAND_BUFFER_WL as the target, NULL
    context.  If no attributes are given, an EGLImage will be created
    for the first plane.  For multi-planar buffers, specify the plane
    to create the EGLImage for by using the EGL_WAYLAND_PLANE_WL
    attribute.  The value of the attribute is the index of the plane,
    as defined by the buffer format.  Writing to an EGLImage created
    from a wl_buffer in any way (such as glTexImage2D, binding the
    EGLImage as a renderbuffer etc) will result in undefined behavior.

Issues

Revision History

    Version 1, March 1, 2011
        Initial draft (Benjamin Franzke)
    Version 2, July 5, 2012
        Add EGL_WAYLAND_PLANE_WL attribute to allow creating an EGLImage
        for different planes of planar buffer. (Kristian Høgsberg)
    Version 3, July 10, 2012
        Add eglQueryWaylandBufferWL and the various buffer
        formats. (Kristian Høgsberg)
