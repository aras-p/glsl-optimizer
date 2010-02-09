.. _rasterizer:

Rasterizer
==========

The rasterizer state controls the rendering of points, lines and triangles.
Attributes include polygon culling state, line width, line stipple,
multisample state, scissoring and flat/smooth shading.

Members
-------

bypass_vs_clip_and_viewport
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Whether the entire TCL pipeline should be bypassed. This implies that
vertices are pre-transformed for the viewport, and will not be run
through the vertex shader.

.. note::
   
   Implementations may still clip away vertices that are not in the viewport
   when this is set.

flatshade
^^^^^^^^^

If set, the provoking vertex of each polygon is used to determine the color
of the entire polygon.  If not set, fragment colors will be interpolated
between the vertex colors.

The actual interpolated shading algorithm is obviously
implementation-dependent, but will usually be Gourard for most hardware.

.. note::

    This is separate from the fragment shader input attributes
    CONSTANT, LINEAR and PERSPECTIVE. The flatshade state is needed at
    clipping time to determine how to set the color of new vertices.

    :ref:`Draw` can implement flat shading by copying the provoking vertex
    color to all the other vertices in the primitive.

flatshade_first
^^^^^^^^^^^^^^^

Whether the first vertex should be the provoking vertex, for most primitives.
If not set, the last vertex is the provoking vertex.

There are several important exceptions to the specification of this rule.

* ``PIPE_PRIMITIVE_POLYGON``: The provoking vertex is always the first
  vertex. If the caller wishes to change the provoking vertex, they merely
  need to rotate the vertices themselves.
* ``PIPE_PRIMITIVE_QUAD``, ``PIPE_PRIMITIVE_QUAD_STRIP``: This option has no
  effect; the provoking vertex is always the last vertex.
* ``PIPE_PRIMITIVE_TRIANGLE_FAN``: When set, the provoking vertex is the
  second vertex, not the first. This permits each segment of the fan to have
  a different color.

Other Members
^^^^^^^^^^^^^

light_twoside
    If set, there are per-vertex back-facing colors. :ref:`Draw`
    uses this state along with the front/back information to set the
    final vertex colors prior to rasterization.

front_winding
    Indicates the window order of front-facing polygons, either
    PIPE_WINDING_CW or PIPE_WINDING_CCW

cull_mode
    Indicates which polygons to cull, either PIPE_WINDING_NONE (cull no
    polygons), PIPE_WINDING_CW (cull clockwise-winding polygons),
    PIPE_WINDING_CCW (cull counter clockwise-winding polygons), or
    PIPE_WINDING_BOTH (cull all polygons).

fill_cw
    Indicates how to fill clockwise polygons, either PIPE_POLYGON_MODE_FILL,
    PIPE_POLYGON_MODE_LINE or PIPE_POLYGON_MODE_POINT.
fill_ccw
    Indicates how to fill counter clockwise polygons, either
    PIPE_POLYGON_MODE_FILL, PIPE_POLYGON_MODE_LINE or PIPE_POLYGON_MODE_POINT.

poly_stipple_enable
    Whether polygon stippling is enabled.
poly_smooth
    Controls OpenGL-style polygon smoothing/antialiasing
offset_cw
    If set, clockwise polygons will have polygon offset factors applied
offset_ccw
    If set, counter clockwise polygons will have polygon offset factors applied
offset_units
    Specifies the polygon offset bias
offset_scale
    Specifies the polygon offset scale

line_width
    The width of lines.
line_smooth
    Whether lines should be smoothed. Line smoothing is simply anti-aliasing.
line_stipple_enable
    Whether line stippling is enabled.
line_stipple_pattern
    16-bit bitfield of on/off flags, used to pattern the line stipple.
line_stipple_factor
    When drawing a stippled line, each bit in the stipple pattern is
    repeated N times, where N = line_stipple_factor + 1.
line_last_pixel
    Controls whether the last pixel in a line is drawn or not.  OpenGL
    omits the last pixel to avoid double-drawing pixels at the ends of lines
    when drawing connected lines.

point_smooth
    Whether points should be smoothed. Point smoothing turns rectangular
    points into circles or ovals.
point_size_per_vertex
    Whether vertices have a point size element.
point_size
    The size of points, if not specified per-vertex.
sprite_coord_enable
    Specifies if a coord has its texture coordinates replaced or not. This
    is a packed bitfield containing the enable for all coords - if all are 0
    point sprites are effectively disabled, though points may still be
    rendered slightly different according to point_quad_rasterization.
    If any coord is non-zero, point_smooth should be disabled, and
    point_quad_rasterization enabled.
    If enabled, the four vertices of the resulting quad will be assigned
    texture coordinates, according to sprite_coord_mode.
sprite_coord_mode
    Specifies how the value for each shader output should be computed when
    drawing sprites, for each coord which has sprite_coord_enable set.
    For PIPE_SPRITE_COORD_LOWER_LEFT, the lower left vertex will have
    coordinate (0,0,0,1).
    For PIPE_SPRITE_COORD_UPPER_LEFT, the upper-left vertex will have
    coordinate (0,0,0,1).
    This state is needed by :ref:`Draw` because that's where each
    point vertex is converted into four quad vertices.  There's no other
    place to emit the new vertex texture coordinates which are required for
    sprite rendering.
    Note that when geometry shaders are available, this state could be
    removed.  A special geometry shader defined by the state tracker could
    convert the incoming points into quads with the proper texture coords.
point_quad_rasterization
    This determines if points should be rasterized as quads or points.
    d3d always uses quad rasterization for points, regardless if point sprites
    are enabled or not, but OGL has different rules. If point_quad_rasterization
    is set, point_smooth should be disabled, and points will be rendered as
    squares even if multisample is enabled.
    sprite_coord_enable should be zero if point_quad_rasterization is not
    enabled.

scissor
    Whether the scissor test is enabled.

multisample
    Whether :term:`MSAA` is enabled.

gl_rasterization_rules
    Whether the rasterizer should use (0.5, 0.5) pixel centers. When not set,
    the rasterizer will use (0, 0) for pixel centers.

