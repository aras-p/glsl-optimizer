.. _screen:

Screen
======

A screen is an object representing the context-independent part of a device.

Flags and enumerations
----------------------

XXX some of these don't belong in this section.


.. _pipe_cap:

PIPE_CAP_*
^^^^^^^^^^

Capability queries return information about the features and limits of the
driver/GPU.  For floating-point values, use :ref:`get_paramf`, and for boolean
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
* ``TIMER_QUERY``: Whether timer queries are available.
* ``INSTANCED_DRAWING``: indicates support for instanced drawing.
* ``TEXTURE_SHADOW_MAP``: indicates whether the fragment shader hardware
  can do the depth texture / Z comparison operation in TEX instructions
  for shadow testing.
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
* ``MAX_PREDICATE_REGISTERS``: indicates the number of predicate registers
  available.  Predicate register may be set as a side-effect of ALU
  instructions to indicate less than, greater than or equal to zero.
  Later instructions can use a predicate register to control writing to
  each channel of destination registers.  NOTE: predicate registers have
  not been fully implemented in Gallium at this time.  See the
  GL_NV_fragment_program extension for more info (look for "condition codes").
* ``MAX_COMBINED_SAMPLERS``: The total number of samplers accessible from
  the vertex and fragment shader, inclusive.
* ``MAX_CONST_BUFFERS``: Maximum number of constant buffers that can be bound
  to any shader stage using ``set_constant_buffer``. If 0 or 1, the pipe will
  only permit binding one constant buffer per shader, and the shaders will
  not permit two-dimensional access to constants.

If a value greater than 0 is returned, the driver can have multiple
constant buffers bound to shader stages. The CONST register file can
be accessed with two-dimensional indices, like in the example below.

DCL CONST[0][0..7]       # declare first 8 vectors of constbuf 0
DCL CONST[3][0]          # declare first vector of constbuf 3
MOV OUT[0], CONST[0][3]  # copy vector 3 of constbuf 0

For backwards compatibility, one-dimensional access to CONST register
file is still supported. In that case, the constbuf index is assumed
to be 0.

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

Fragment shader limits:

* ``PIPE_CAP_MAX_FS_INSTRUCTIONS``: The maximum number of instructions.
* ``PIPE_CAP_MAX_FS_ALU_INSTRUCTIONS``: The maximum number of arithmetic instructions.
* ``PIPE_CAP_MAX_FS_TEX_INSTRUCTIONS``: The maximum number of texture instructions.
* ``PIPE_CAP_MAX_FS_TEX_INDIRECTIONS``: The maximum number of texture indirections.
* ``PIPE_CAP_MAX_FS_CONTROL_FLOW_DEPTH``: The maximum nested control flow depth.
* ``PIPE_CAP_MAX_FS_INPUTS``: The maximum number of input registers.
* ``PIPE_CAP_MAX_FS_CONSTS``: The maximum number of constants.
* ``PIPE_CAP_MAX_FS_TEMPS``: The maximum number of temporary registers.
* ``PIPE_CAP_MAX_FS_ADDRS``: The maximum number of address registers.
* ``PIPE_CAP_MAX_FS_PREDS``: The maximum number of predicate registers.

Vertex shader limits:

* ``PIPE_CAP_MAX_VS_*``: Identical to ``PIPE_CAP_MAX_FS_*``.


.. _pipe_bind:

PIPE_BIND_*
^^^^^^^^^^^

These flags indicate how a resource will be used and are specified at resource
creation time. Resources may be used in different roles
during their lifecycle. Bind flags are cumulative and may be combined to create
a resource which can be used for multiple things.
Depending on the pipe driver's memory management and these bind flags,
resources might be created and handled quite differently.

* ``PIPE_BIND_RENDER_TARGET``: A color buffer or pixel buffer which will be
  rendered to.  Any surface/resource attached to pipe_framebuffer_state::cbufs
  must have this flag set.
* ``PIPE_BIND_DEPTH_STENCIL``: A depth (Z) buffer and/or stencil buffer. Any
  depth/stencil surface/resource attached to pipe_framebuffer_state::zsbuf must
  have this flag set.
* ``PIPE_BIND_DISPLAY_TARGET``: A surface that can be presented to screen. Arguments to
  pipe_screen::flush_front_buffer must have this flag set.
* ``PIPE_BIND_SAMPLER_VIEW``: A texture that may be sampled from in a fragment
  or vertex shader.
* ``PIPE_BIND_VERTEX_BUFFER``: A vertex buffer.
* ``PIPE_BIND_INDEX_BUFFER``: An vertex index/element buffer.
* ``PIPE_BIND_CONSTANT_BUFFER``: A buffer of shader constants.
* ``PIPE_BIND_TRANSFER_WRITE``: A transfer object which will be written to.
* ``PIPE_BIND_TRANSFER_READ``: A transfer object which will be read from.
* ``PIPE_BIND_CUSTOM``:
* ``PIPE_BIND_SCANOUT``: A front color buffer or scanout buffer.
* ``PIPE_BIND_SHARED``: A sharable buffer that can be given to another
  process.

.. _pipe_usage:

PIPE_USAGE_*
^^^^^^^^^^^^

The PIPE_USAGE enums are hints about the expected usage pattern of a resource.

* ``PIPE_USAGE_DEFAULT``: Expect many uploads to the resource, intermixed with draws.
* ``PIPE_USAGE_DYNAMIC``: Expect many uploads to the resource, intermixed with draws.
* ``PIPE_USAGE_STATIC``: Same as immutable (?)
* ``PIPE_USAGE_IMMUTABLE``: Resource will not be changed after first upload.
* ``PIPE_USAGE_STREAM``: Upload will be followed by draw, followed by upload, ...



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

XXX to-do

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

Determine if a resource in the given format can be used in a specific manner.

**format** the resource format

**target** one of the PIPE_TEXTURE_x flags

**sample_count** the number of samples. 0 and 1 mean no multisampling,
the maximum allowed legal value is 32.

**bindings** is a bitmask of :ref:`PIPE_BIND` flags.

**geom_flags** is a bitmask of PIPE_TEXTURE_GEOM_x flags.

Returns TRUE if all usages can be satisfied.

.. _resource_create:

resource_create
^^^^^^^^^^^^^^^

Create a new resource from a template.
The following fields of the pipe_resource must be specified in the template:

**target** one of the pipe_texture_target enums.
Note that PIPE_BUFFER and PIPE_TEXTURE_X are not really fundamentally different.
Modern APIs allow using buffers as shader resources.

**format** one of the pipe_format enums.

**width0** the width of the base mip level of the texture or size of the buffer.

**height0** the height of the base mip level of the texture
(1 for 1D or 1D array textures).

**depth0** the depth of the base mip level of the texture
(1 for everything else).

**array_size** the array size for 1D and 2D array textures.
For cube maps this must be 6, for other textures 1.

**last_level** the last mip map level present.

**nr_samples** the nr of msaa samples. 0 (or 1) specifies a resource
which isn't multisampled.

**usage** one of the PIPE_USAGE flags.

**bind** bitmask of the PIPE_BIND flags.

**flags** bitmask of PIPE_RESOURCE_FLAG flags.



resource_destroy
^^^^^^^^^^^^^^^^

Destroy a resource. A resource is destroyed if it has no more references.

