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

* ``PIPE_CAP_NPOT_TEXTURES``: Whether :term:`NPOT` textures may have repeat modes,
  normalized coordinates, and mipmaps.
* ``PIPE_CAP_TWO_SIDED_STENCIL``: Whether the stencil test can also affect back-facing
  polygons.
* ``PIPE_CAP_DUAL_SOURCE_BLEND``: Whether dual-source blend factors are supported. See
  :ref:`Blend` for more information.
* ``PIPE_CAP_ANISOTROPIC_FILTER``: Whether textures can be filtered anisotropically.
* ``PIPE_CAP_POINT_SPRITE``: Whether point sprites are available.
* ``PIPE_CAP_MAX_RENDER_TARGETS``: The maximum number of render targets that may be
  bound.
* ``PIPE_CAP_OCCLUSION_QUERY``: Whether occlusion queries are available.
* ``PIPE_CAP_TIMER_QUERY``: Whether timer queries are available.
* ``PIPE_CAP_TEXTURE_SHADOW_MAP``: indicates whether the fragment shader hardware
  can do the depth texture / Z comparison operation in TEX instructions
  for shadow testing.
* ``PIPE_CAP_TEXTURE_SWIZZLE``: Whether swizzling through sampler views is
  supported.
* ``PIPE_CAP_MAX_TEXTURE_2D_LEVELS``: The maximum number of mipmap levels available
  for a 2D texture.
* ``PIPE_CAP_MAX_TEXTURE_3D_LEVELS``: The maximum number of mipmap levels available
  for a 3D texture.
* ``PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS``: The maximum number of mipmap levels available
  for a cubemap.
* ``PIPE_CAP_TEXTURE_MIRROR_CLAMP``: Whether mirrored texture coordinates with clamp
  are supported.
* ``PIPE_CAP_BLEND_EQUATION_SEPARATE``: Whether alpha blend equations may be different
  from color blend equations, in :ref:`Blend` state.
* ``PIPE_CAP_SM3``: Whether the vertex shader and fragment shader support equivalent
  opcodes to the Shader Model 3 specification. XXX oh god this is horrible
* ``PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS``: The maximum number of stream buffers.
* ``PIPE_CAP_PRIMITIVE_RESTART``: Whether primitive restart is supported.
* ``PIPE_CAP_MAX_COMBINED_SAMPLERS``: The total number of samplers accessible from
  the vertex and fragment shader, inclusive.
* ``PIPE_CAP_INDEP_BLEND_ENABLE``: Whether per-rendertarget blend enabling and channel
  masks are supported. If 0, then the first rendertarget's blend mask is
  replicated across all MRTs.
* ``PIPE_CAP_INDEP_BLEND_FUNC``: Whether per-rendertarget blend functions are
  available. If 0, then the first rendertarget's blend functions affect all
  MRTs.
* ``PIPE_CAP_DEPTHSTENCIL_CLEAR_SEPARATE``: Whether clearing only depth or only
  stencil in a combined depth-stencil buffer is supported.
* ``PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS``: The maximum number of texture array
  layers supported. If 0, the array textures are not supported at all and
  the ARRAY texture targets are invalid.
* ``PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT``: Whether the TGSI property
  FS_COORD_ORIGIN with value UPPER_LEFT is supported.
* ``PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT``: Whether the TGSI property
  FS_COORD_ORIGIN with value LOWER_LEFT is supported.
* ``PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER``: Whether the TGSI
  property FS_COORD_PIXEL_CENTER with value HALF_INTEGER is supported.
* ``PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER``: Whether the TGSI
  property FS_COORD_PIXEL_CENTER with value INTEGER is supported.


.. _pipe_capf:

PIPE_CAPF_*
^^^^^^^^^^^^^^^^

The floating-point capabilities are:

* ``PIPE_CAPF_MAX_LINE_WIDTH``: The maximum width of a regular line.
* ``PIPE_CAPF_MAX_LINE_WIDTH_AA``: The maximum width of a smoothed line.
* ``PIPE_CAPF_MAX_POINT_WIDTH``: The maximum width and height of a point.
* ``PIPE_CAPF_MAX_POINT_WIDTH_AA``: The maximum width and height of a smoothed point.
* ``PIPE_CAPF_MAX_TEXTURE_ANISOTROPY``: The maximum level of anisotropy that can be
  applied to anisotropically filtered textures.
* ``PIPE_CAPF_MAX_TEXTURE_LOD_BIAS``: The maximum :term:`LOD` bias that may be applied
  to filtered textures.
* ``PIPE_CAPF_GUARD_BAND_LEFT``,
  ``PIPE_CAPF_GUARD_BAND_TOP``,
  ``PIPE_CAPF_GUARD_BAND_RIGHT``,
  ``PIPE_CAPF_GUARD_BAND_BOTTOM``: TODO


.. _pipe_shader_cap:

PIPE_SHADER_CAP_*
^^^^^^^^^^^^^^^^^

These are per-shader-stage capabitity queries. Different shader stages may
support different features.

* ``PIPE_SHADER_CAP_MAX_INSTRUCTIONS``: The maximum number of instructions.
* ``PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS``: The maximum number of arithmetic instructions.
* ``PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS``: The maximum number of texture instructions.
* ``PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS``: The maximum number of texture indirections.
* ``PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH``: The maximum nested control flow depth.
* ``PIPE_SHADER_CAP_MAX_INPUTS``: The maximum number of input registers.
* ``PIPE_SHADER_CAP_MAX_CONSTS``: The maximum number of constants.
* ``PIPE_SHADER_CAP_MAX_CONST_BUFFERS``: Maximum number of constant buffers that can be bound
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
  
* ``PIPE_SHADER_CAP_MAX_TEMPS``: The maximum number of temporary registers.
* ``PIPE_SHADER_CAP_MAX_ADDRS``: The maximum number of address registers.
* ``PIPE_SHADER_CAP_MAX_PREDS``: The maximum number of predicate registers.
* ``PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED``: Whether the continue opcode is supported.
* ``PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR``: Whether indirect addressing
  of the input file is supported.
* ``PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR``: Whether indirect addressing
  of the output file is supported.
* ``PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR``: Whether indirect addressing
  of the temporary file is supported.
* ``PIPE_SHADER_CAP_INDIRECT_CONST_ADDR``: Whether indirect addressing
  of the constant file is supported.
* ``PIPE_SHADER_CAP_SUBROUTINES``: Whether subroutines are supported, i.e.
  BGNSUB, ENDSUB, CAL, and RET, including RET in the main block.
* ``PIPE_SHADER_CAP_INTEGERS``: Whether integer opcodes are supported.
  If unsupported, only float opcodes are supported.
* ``PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS``: THe maximum number of texture
  samplers.


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
* ``PIPE_BIND_BLENDABLE``: Used in conjunction with PIPE_BIND_RENDER_TARGET to
  query whether a device supports blending for a given format.
  If this flag is set, surface creation may fail if blending is not supported
  for the specified format. If it is not set, a driver may choose to ignore
  blending on surfaces with formats that would require emulation.
* ``PIPE_BIND_DISPLAY_TARGET``: A surface that can be presented to screen. Arguments to
  pipe_screen::flush_front_buffer must have this flag set.
* ``PIPE_BIND_SAMPLER_VIEW``: A texture that may be sampled from in a fragment
  or vertex shader.
* ``PIPE_BIND_VERTEX_BUFFER``: A vertex buffer.
* ``PIPE_BIND_INDEX_BUFFER``: An vertex index/element buffer.
* ``PIPE_BIND_CONSTANT_BUFFER``: A buffer of shader constants.
* ``PIPE_BIND_TRANSFER_WRITE``: A transfer object which will be written to.
* ``PIPE_BIND_TRANSFER_READ``: A transfer object which will be read from.
* ``PIPE_BIND_STREAM_OUTPUT``: A stream output buffer.
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

