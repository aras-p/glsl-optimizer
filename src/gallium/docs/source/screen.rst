Screen
======

A screen is an object representing the context-independent part of a device.

Useful Flags
------------

PIPE_TEXTURE_USAGE
^^^^^^^^^^^^^^^^^^

These flags determine the possible roles a texture may be used for during its
lifetime. Texture usage flags are cumulative and may be combined to create a
texture that can be used as multiple things.

* ``RENDER_TARGET``: A colorbuffer or pixelbuffer.
* ``DISPLAY_TARGET``: A sharable buffer that can be given to another process.
* ``PRIMARY``: A frontbuffer or scanout buffer.
* ``DEPTH_STENCIL``: A depthbuffer, stencilbuffer, or Z buffer. Gallium does
  not explicitly provide for stencil-only buffers, so any stencilbuffer
  validated here is implicitly also a depthbuffer.
* ``SAMPLER``: A texture that may be sampled from in a fragment or vertex
  shader.
* ``DYNAMIC``: XXX undefined

Methods
-------

XXX moar; got bored

get_name
^^^^^^^^

Returns an identifying name for the screen.

get_vendor
^^^^^^^^^^

Returns the screen vendor.

get_param
^^^^^^^^^

Get an integer/boolean screen parameter.

get_paramf
^^^^^^^^^^

Get a floating-point screen parameter.

is_format_supported
^^^^^^^^^^^^^^^^^^^

See if a format can be used in a specific manner.

**usage** is a bitmask of ``PIPE_TEXTURE_USAGE`` flags.

Returns TRUE if all usages can be satisfied.

texture_create
^^^^^^^^^^^^^^

Given a template of texture setup, create a BO-backed texture.
