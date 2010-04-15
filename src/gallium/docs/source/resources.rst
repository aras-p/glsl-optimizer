Resources
=========

Resources represent objects that hold data: textures and buffers.

They are mostly modelled after the resources in Direct3D 10/11, but with a
different transfer/update mechanism, and more features for OpenGL support.

Resource targets
----------------

Resource targets determine the type of a resource.

Note that drivers may not actually have the restrictions listed regarding
coordinate normalization and wrap modes, and in fact efficient OpenCL
support will probably require drivers that don't have any of them, which
will probably be advertised with an appropriate cap.

TODO: document all targets. Note that both 3D and cube have restrictions
that depend on the hardware generation.

TODO: can buffers have a non-R8 format?

PIPE_TEXTURE_RECT
^^^^^^^^^^^^^^^^^
2D surface with OpenGL GL_TEXTURE_RECTANGLE semantics.

depth must be 1
- last_level must be 0
- Must use unnormalized coordinates
- Must use a clamp wrap mode

PIPE_TEXTURE_2D
^^^^^^^^^^^^^^^
2D surface accessed with normalized coordinates.

- If PIPE_CAP_NPOT_TEXTURES is not supported,
      width and height must be powers of two
- Mipmaps can be used
- Must use normalized coordinates
- No special restrictions on wrap modes
