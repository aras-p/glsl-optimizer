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
* ``INDEP_BLEND_ENABLE``: Whether per-rendertarget blend enabling and channel
  masks are supported. If 0, then the first rendertarget's blend mask is
  replicated across all MRTs.
* ``INDEP_BLEND_FUNC``: Whether per-rendertarget blend functions are
  available. If 0, then the first rendertarget's blend functions affect all
  MRTs.
* ``PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT``: Whether the TGSI property
  FS_COORD_ORIGIN with value UPPER_LEFT is supported.
* ``PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT``: Whether the TGSI property
  FS_COORD_ORIGIN with value LOWER_LEFT is supported.
* ``PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER``: Whether the TGSI
  property FS_COORD_PIXEL_CENTER with value HALF_INTEGER is supported.
* ``PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER``: Whether the TGSI
  property FS_COORD_PIXEL_CENTER with value INTEGER is supported.

The floating-point capabilities:

* ``MAX_LINE_WIDTH``: The maximum width of a regular line.
* ``MAX_LINE_WIDTH_AA``: The maximum width of a smoothed line.
* ``MAX_POINT_WIDTH``: The maximum width and height of a point.
* ``MAX_POINT_WIDTH_AA``: The maximum width and height of a smoothed point.
* ``MAX_TEXTURE_ANISOTROPY``: The maximum level of anisotropy that can be
  applied to anisotropically filtered textures.
* ``MAX_TEXTURE_LOD_BIAS``: The maximum :term:`LOD` bias that may be applied
  to filtered textures.
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

.. _pipe_bind:

PIPE_BIND
^^^^^^^^^

These flags control resource creation. Resources may be used in different roles
during their lifecycle. Bind flags are cumulative and may be combined to create
a resource which can be used as multiple things.
Depending on the pipe driver's memory management, depending on these bind flags
resources might be created and handled quite differently.

* ``RENDER_TARGET``: A color buffer or pixel buffer which will be rendered to.
* ``DISPLAY_TARGET``: A sharable buffer that can be given to another process.
* ``DEPTH_STENCIL``: A depth (Z) buffer or stencil buffer.  Gallium does
  not explicitly provide for stencil-only buffers, so any stencil buffer
  validated here is implicitly also a depth buffer.
* ``SAMPLER_VIEW``: A texture that may be sampled from in a fragment or vertex
  shader.
* ``VERTEX_BUFFER``: A vertex buffer.
* ``INDEX_BUFFER``: An element buffer.
* ``CONSTANT_BUFFER``: A buffer of shader constants.
* ``BLIT_SOURCE``: A blit source, as given to surface_copy.
* ``BLIT_DESTINATION``: A blit destination, as given to surface_copy and surface_fill.
* ``TRANSFER_WRITE``: A transfer object which will be written to.
* ``TRANSFER_READ``: A transfer object which will be read from.
* ``CUSTOM``:
* ``SCANOUT``: A front color buffer or scanout buffer.
* ``SHARED``:

.. _pipe_usage:

PIPE_USAGE
^^^^^^^^^^

The PIPE_USAGE enums are hints about the expected lifecycle of a resource.
* ``DEFAULT``: Expect many uploads to the resource, intermixed with draws.
* ``DYNAMIC``: Expect many uploads to the resource, intermixed with draws.
* ``STATIC``: Same as immutable (?)
* ``IMMUTABLE``: Resource will not be changed after first upload.
* ``STREAM``: Upload will be followed by draw, followed by upload, ...



PIPE_TEXTURE_GEOM
^^^^^^^^^^^^^^^^^

These flags are used when querying whether a particular pipe_format is
supported by the driver (with the `is_format_supported` function).
Some formats may only be supported for certain kinds of textures.
For example, a compressed format might only be used for POT textures.

* ``PIPE_TEXTURE_GEOM_NON_SQUARE``: The texture may not be square
* ``PIPE_TEXTURE_GEOM_NON_POWER_OF_TWO``: The texture dimensions may not be
  powers of two.


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

context_create
^^^^^^^^^^^^^^

Create a pipe_context.

**priv** is private data of the caller, which may be put to various
unspecified uses, typically to do with implementing swapbuffers
and/or front-buffer rendering.

is_format_supported
^^^^^^^^^^^^^^^^^^^

See if a format can be used in a specific manner.

**tex_usage** is a bitmask of :ref:`PIPE_BIND` flags.

Returns TRUE if all usages can be satisfied.


.. _resource_create:

resource_create
^^^^^^^^^^^^^^

Given a template of texture setup, create a resource.
The way a resource may be used is specifed by bind flags, :ref:`pipe_bind`.
and hints are used to indicate to the driver what access pattern might be
likely, :ref:`pipe_usage`.

resource_destroy
^^^^^^^^^^^^^^^

Destroy a resource. A resource is destroyed if it has no more references.

