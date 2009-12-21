.. _rasterizer:

Rasterizer
==========

The rasterizer is the main chunk of state controlling how vertices are
interpolated into fragments.

Members
-------

XXX undocumented light_twoside, front_winding, cull_mode, fill_cw, fill_ccw, offset_cw, offset_ccw
XXX moar undocumented poly_smooth, line_stipple_factor, line_last_pixel, offset_units, offset_scale
XXX sprite_coord_mode

flatshade
    If set, the provoking vertex of each polygon is used to determine the
    color of the entire polygon. If not set, the color fragments will be
    interpolated from each vertex's color.
scissor
    Whether the scissor test is enabled.
poly_stipple_enable
    Whether polygon stippling is enabled.
point_smooth
    Whether points should be smoothed. Point smoothing turns rectangular
    points into circles or ovals.
point_sprite
    Whether point sprites are enabled.
point_size_per_vertex
    Whether vertices have a point size element.
multisample
    Whether :ref:`MSAA` is enabled.
line_smooth
    Whether lines should be smoothed. Line smoothing is simply anti-aliasing.
line_stipple_enable
    Whether line stippling is enabled.
line_stipple_pattern
    16-bit bitfield of on/off flags, used to pattern the line stipple.
bypass_vs_clip_and_viewport
    Whether the entire TCL pipeline should be bypassed. This implies that
    vertices are pre-transformed for the viewport, and will not be run
    through the vertex shader. Note that implementations may still clip away
    vertices that are not in the viewport.
flatshade_first
    Whether the first vertex should be the provoking vertex, for most
    primitives. If not set, the last vertex is the provoking vertex.
gl_rasterization_rules
    Whether the rasterizer should use (0.5, 0.5) pixel centers. When not set,
    the rasterizer will use (0, 0) for pixel centers.
line_width
    The width of lines.
point_size
    The size of points, if not specified per-vertex.
point_size_min
    The minimum size of points.
point_size_max
    The maximum size of points.

Notes
-----

flatshade
^^^^^^^^^

The actual interpolated shading algorithm is obviously
implementation-dependent, but will usually be Gourard for most hardware.

bypass_vs_clip_and_viewport
^^^^^^^^^^^^^^^^^^^^^^^^^^^

When set, this implies that vertices are pre-transformed for the viewport, and
will not be run through the vertex shader. Note that implementations may still
clip away vertices that are not visible.

flatshade_first
^^^^^^^^^^^^^^^

There are several important exceptions to the specification of this rule.

* ``PIPE_PRIMITIVE_POLYGON``: The provoking vertex is always the first
  vertex. If the caller wishes to change the provoking vertex, they merely
  need to rotate the vertices themselves.
* ``PIPE_PRIMITIVE_QUAD``, ``PIPE_PRIMITIVE_QUAD_STRIP``: This option has no
  effect; the provoking vertex is always the last vertex.
* ``PIPE_PRIMITIVE_TRIANGLE_FAN``: When set, the provoking vertex is the
  second vertex, not the first. This permits each segment of the fan to have
  a different color.
