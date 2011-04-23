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

New Tokens

    Accepted as <target> in eglCreateImageKHR

        EGL_WAYLAND_BUFFER_WL                   0x31D5

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

    Import a wl_buffer by calling eglCreateImageKHR with
    wl_buffer as EGLClientBuffer, EGL_WAYLAND_BUFFER_WL as the target,
    NULL context and an empty attribute_list.

Issues

Revision History

    Version 1, March 1, 2011
        Initial draft (Benjamin Franzke)
