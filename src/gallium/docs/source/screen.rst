Screen
======

A screen is an object representing the context-independent part of a device.

Useful Flags
------------

.. _pipe_cap:

PIPE_CAP
^^^^^^^^

Pipe capabilities help expose hardware functionality not explicitly required
by Gallium. For floating-point values, use :ref:`get_paramf`, and for boolean
or integer values, use :ref:`get_param`.

The integer capabilities:

* ``MAX_TEXTURE_IMAGE_UNITS``: The maximum number of samplers available.
* ``NPOT_TEXTURES``: Whether :term:`NPOT` textures may have repeat modes,
  normalized coordinates, and mipmaps.
* ``TWO_SIDED_STENCIL``: Whether the stencil test can also affect back-facing
  polygons.
* ``GLSL``: Deprecated.
* ``DUAL_SOURCE_BLEND``: Whether dual-source blend factors are supported. See
  :ref:`Blend` for more information.
* ``ANISOTROPIC_FILTER``: Whether textures can be filtered anisotropically.
* ``POINT_SPRITE``: Whether point sprites are available.
* ``MAX_RENDER_TARGETS``: The maximum number of render targets that may be
  bound.
* ``OCCLUSION_QUERY``: Whether occlusion queries are available.
* ``TEXTURE_SHADOW_MAP``: XXX
* ``MAX_TEXTURE_2D_LEVELS``: The maximum number of mipmap levels available
  for a 2D texture.
* ``MAX_TEXTURE_3D_LEVELS``: The maximum number of mipmap levels available
  for a 3D texture.
* ``MAX_TEXTURE_CUBE_LEVELS``: The maximum number of mipmap levels available
  for a cubemap.
* ``TEXTURE_MIRROR_CLAMP``: Whether mirrored texture coordinates with clamp
  are supported.
* ``TEXTURE_MIRROR_REPEAT``: Whether mirrored repeating texture coordinates
  are supported.
* ``MAX_VERTEX_TEXTURE_UNITS``: The maximum number of samplers addressable
  inside the vertex shader. If this is 0, then the vertex shader cannot
  sample textures.
* ``TGSI_CONT_SUPPORTED``: Whether the TGSI CONT opcode is supported.
* ``BLEND_EQUATION_SEPARATE``: Whether alpha blend equations may be different
  from color blend equations, in :ref:`Blend` state.
* ``SM3``: Whether the vertex shader and fragment shader support equivalent
  opcodes to the Shader Model 3 specification. XXX oh god this is horrible
* ``MAX_PREDICATE_REGISTERS``: XXX
* ``MAX_COMBINED_SAMPLERS``: The total number of samplers accessible from
  the vertex and fragment shader, inclusive.
* ``MAX_CONST_BUFFERS``: Maximum number of constant buffers that can be bound
  to any shader stage using ``set_constant_buffer``. If 0 or 1, the pipe will
  only permit binding one constant buffer per shader, and the shaders will
  not permit two-dimensional access to constants.
* ``MAX_CONST_BUFFER_SIZE``: Maximum byte size of a single constant buffer.

The floating-point capabilities:

* ``MAX_LINE_WIDTH``: The maximum width of a regular line.
* ``MAX_LINE_WIDTH_AA``: The maximum width of a smoothed line.
* ``MAX_POINT_WIDTH``: The maximum width and height of a point.
* ``MAX_POINT_WIDTH_AA``: The maximum width and height of a smoothed point.
* ``GUARD_BAND_LEFT``, ``GUARD_BAND_TOP``, ``GUARD_BAND_RIGHT``,
  ``GUARD_BAND_BOTTOM``: XXX

XXX Is there a better home for this? vvv

If 0 is returned, the driver is not aware of multiple constant buffers,
supports binding of only one constant buffer, and does not support
two-dimensional CONST register file access in TGSI shaders.

If a value greater than 0 is returned, the driver can have multiple
constant buffers bound to shader stages. The CONST register file can
be accessed with two-dimensional indices, like in the example below.

DCL CONST[0][0..7]       # declare first 8 vectors of constbuf 0
DCL CONST[3][0]          # declare first vector of constbuf 3
MOV OUT[0], CONST[0][3]  # copy vector 3 of constbuf 0

For backwards compatibility, one-dimensional access to CONST register
file is still supported. In that case, the constbuf index is assumed
to be 0.

.. _pipe_buffer_usage:

PIPE_BUFFER_USAGE
^^^^^^^^^^^^^^^^^

These flags control buffer creation. Buffers may only have one role, so
care should be taken to not allocate a buffer with the wrong usage.

* ``PIXEL``: This is the flag to use for all textures.
* ``VERTEX``: A vertex buffer.
* ``INDEX``: An element buffer.
* ``CONSTANT``: A buffer of shader constants.

Buffers are inevitably abstracting the pipe's underlying memory management,
so many of their usage flags can be used to direct the way the buffer is
handled.

* ``CPU_READ``, ``CPU_WRITE``: Whether the user will map and, in the case of
  the latter, write to, the buffer. The convenience flag ``CPU_READ_WRITE`` is
  available to signify a read/write buffer.
* ``GPU_READ``, ``GPU_WRITE``: Whether the driver will internally need to
  read from or write to the buffer. The latter will only happen if the buffer
  is made into a render target.
* ``DISCARD``: When set on a map, the contents of the map will be discarded
  beforehand. Cannot be used with ``CPU_READ``.
* ``DONTBLOCK``: When set on a map, the map will fail if the buffer cannot be
  mapped immediately.
* ``UNSYNCHRONIZED``: When set on a map, any outstanding operations on the
  buffer will be ignored. The interaction of any writes to the map and any
  operations pending with the buffer are undefined. Cannot be used with
  ``CPU_READ``.
* ``FLUSH_EXPLICIT``: When set on a map, written ranges of the map require
  explicit flushes using :ref:`buffer_flush_mapped_range`. Requires
  ``CPU_WRITE``.

.. _pipe_texture_usage:

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
* ``DYNAMIC``: A texture that will be mapped frequently.

Methods
-------

XXX moar; got bored

get_name
^^^^^^^^

Returns an identifying name for the screen.

get_vendor
^^^^^^^^^^

Returns the screen vendor.

.. _get_param:

get_param
^^^^^^^^^

Get an integer/boolean screen parameter.

**param** is one of the :ref:`PIPE_CAP` names.

.. _get_paramf:

get_paramf
^^^^^^^^^^

Get a floating-point screen parameter.

**param** is one of the :ref:`PIPE_CAP` names.

is_format_supported
^^^^^^^^^^^^^^^^^^^

See if a format can be used in a specific manner.

**usage** is a bitmask of :ref:`PIPE_TEXTURE_USAGE` flags.

Returns TRUE if all usages can be satisfied.

.. note::

   ``PIPE_TEXTURE_USAGE_DYNAMIC`` is not a valid usage.

.. _texture_create:

texture_create
^^^^^^^^^^^^^^

Given a template of texture setup, create a buffer and texture.

texture_blanket
^^^^^^^^^^^^^^^

Like :ref:`texture_create`, but use a supplied buffer instead of creating a
new one.

texture_destroy
^^^^^^^^^^^^^^^

Destroy a texture. The buffer backing the texture is destroyed if it has no
more references.

buffer_map
^^^^^^^^^^

Map a buffer into memory.

**usage** is a bitmask of :ref:`PIPE_TEXTURE_USAGE` flags.

Returns a pointer to the map, or NULL if the mapping failed.

buffer_map_range
^^^^^^^^^^^^^^^^

Map a range of a buffer into memory.

The returned map is always relative to the beginning of the buffer, not the
beginning of the mapped range.

.. _buffer_flush_mapped_range:

buffer_flush_mapped_range
^^^^^^^^^^^^^^^^^^^^^^^^^

Flush a range of mapped memory into a buffer.

The buffer must have been mapped with ``PIPE_BUFFER_USAGE_FLUSH_EXPLICIT``.

**usage** is a bitmask of :ref:`PIPE_TEXTURE_USAGE` flags.

buffer_unmap
^^^^^^^^^^^^

Unmap a buffer from memory.

Any pointers into the map should be considered invalid and discarded.
