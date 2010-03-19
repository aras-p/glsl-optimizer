Context
=======

The context object represents the purest, most directly accessible, abilities
of the device's 3D rendering pipeline.

Methods
-------

CSO State
^^^^^^^^^

All CSO state is created, bound, and destroyed, with triplets of methods that
all follow a specific naming scheme. For example, ``create_blend_state``,
``bind_blend_state``, and ``destroy_blend_state``.

CSO objects handled by the context object:

* :ref:`Blend`: ``*_blend_state``
* :ref:`Sampler`: These are special; they can be bound to either vertex or
  fragment samplers, and they are bound in groups.
  ``bind_fragment_sampler_states``, ``bind_vertex_sampler_states``
* :ref:`Rasterizer`: ``*_rasterizer_state``
* :ref:`Depth, Stencil, & Alpha`: ``*_depth_stencil_alpha_state``
* :ref:`Shader`: These have two sets of methods. ``*_fs_state`` is for
  fragment shaders, and ``*_vs_state`` is for vertex shaders.
* :ref:`Vertex Elements`: ``*_vertex_elements_state``


Resource Binding State
^^^^^^^^^^^^^^^^^^^^^^

This state describes how resources in various flavours (textures,
buffers, surfaces) are bound to the driver.


* ``set_constant_buffer`` sets a constant buffer to be used for a given shader
  type. index is used to indicate which buffer to set (some apis may allow
  multiple ones to be set, and binding a specific one later, though drivers
  are mostly restricted to the first one right now).

* ``set_framebuffer_state``

* ``set_vertex_buffers``


Non-CSO State
^^^^^^^^^^^^^

These pieces of state are too small, variable, and/or trivial to have CSO
objects. They all follow simple, one-method binding calls, e.g.
``set_blend_color``.

* ``set_stencil_ref`` sets the stencil front and back reference values
  which are used as comparison values in stencil test.
* ``set_blend_color``
* ``set_clip_state``
* ``set_polygon_stipple``
* ``set_scissor_state`` sets the bounds for the scissor test, which culls
  pixels before blending to render targets. If the :ref:`Rasterizer` does
  not have the scissor test enabled, then the scissor bounds never need to
  be set since they will not be used.
* ``set_viewport_state``


Sampler Views
^^^^^^^^^^^^^

These are the means to bind textures to shader stages. To create one, specify
its format, swizzle and LOD range in sampler view template.

If texture format is different than template format, it is said the texture
is being cast to another format. Casting can be done only between compatible
formats, that is formats that have matching component order and sizes.

Swizzle fields specify they way in which fetched texel components are placed
in the result register. For example, swizzle_r specifies what is going to be
placed in destination register x (AKA r).

first_level and last_level fields of sampler view template specify the LOD
range the texture is going to be constrained to.

* ``set_fragment_sampler_views`` binds an array of sampler views to
  fragment shader stage. Every binding point acquires a reference
  to a respective sampler view and releases a reference to the previous
  sampler view.

* ``set_vertex_sampler_views`` binds an array of sampler views to vertex
  shader stage. Every binding point acquires a reference to a respective
  sampler view and releases a reference to the previous sampler view.

* ``create_sampler_view`` creates a new sampler view. texture is associated
  with the sampler view which results in sampler view holding a reference
  to the texture. Format specified in template must be compatible
  with texture format.

* ``sampler_view_destroy`` destroys a sampler view and releases its reference
  to associated texture.


Clearing
^^^^^^^^

``clear`` initializes some or all of the surfaces currently bound to
the framebuffer to particular RGBA, depth, or stencil values.

Clear is one of the most difficult concepts to nail down to a single
interface and it seems likely that we will want to add additional
clear paths, for instance clearing surfaces not bound to the
framebuffer, or read-modify-write clears such as depth-only or
stencil-only clears of packed depth-stencil buffers.  


Drawing
^^^^^^^

``draw_arrays`` draws a specified primitive.

This command is equivalent to calling ``draw_arrays_instanced``
with ``startInstance`` set to 0 and ``instanceCount`` set to 1.

``draw_elements`` draws a specified primitive using an optional
index buffer.

This command is equivalent to calling ``draw_elements_instanced``
with ``startInstance`` set to 0 and ``instanceCount`` set to 1.

``draw_range_elements``

XXX: this is (probably) a temporary entrypoint, as the range
information should be available from the vertex_buffer state.
Using this to quickly evaluate a specialized path in the draw
module.

``draw_arrays_instanced`` draws multiple instances of the same primitive.

This command is equivalent to calling ``draw_elements_instanced``
with ``indexBuffer`` set to NULL and ``indexSize`` set to 0.

``draw_elements_instanced`` draws multiple instances of the same primitive
using an optional index buffer.

For instanceID in the range between ``startInstance``
and ``startInstance``+``instanceCount``-1, inclusive, draw a primitive
specified by ``mode`` and sequential numbers in the range between ``start``
and ``start``+``count``-1, inclusive.

If ``indexBuffer`` is not NULL, it specifies an index buffer with index
byte size of ``indexSize``. The sequential numbers are used to lookup
the index buffer and the resulting indices in turn are used to fetch
vertex attributes.

If ``indexBuffer`` is NULL, the sequential numbers are used directly
as indices to fetch vertex attributes.

If a given vertex element has ``instance_divisor`` set to 0, it is said
it contains per-vertex data and effective vertex attribute address needs
to be recalculated for every index.

  attribAddr = ``stride`` * index + ``src_offset``

If a given vertex element has ``instance_divisor`` set to non-zero,
it is said it contains per-instance data and effective vertex attribute
address needs to recalculated for every ``instance_divisor``-th instance.

  attribAddr = ``stride`` * instanceID / ``instance_divisor`` + ``src_offset``

In the above formulas, ``src_offset`` is taken from the given vertex element
and ``stride`` is taken from a vertex buffer associated with the given
vertex element.

The calculated attribAddr is used as an offset into the vertex buffer to
fetch the attribute data.

The value of ``instanceID`` can be read in a vertex shader through a system
value register declared with INSTANCEID semantic name.


Queries
^^^^^^^

Queries gather some statistic from the 3D pipeline over one or more
draws.  Queries may be nested, though no state tracker currently
exercises this.  

Queries can be created with ``create_query`` and deleted with
``destroy_query``. To start a query, use ``begin_query``, and when finished,
use ``end_query`` to end the query.

``get_query_result`` is used to retrieve the results of a query.  If
the ``wait`` parameter is TRUE, then the ``get_query_result`` call
will block until the results of the query are ready (and TRUE will be
returned).  Otherwise, if the ``wait`` parameter is FALSE, the call
will not block and the return value will be TRUE if the query has
completed or FALSE otherwise.

A common type of query is the occlusion query which counts the number of
fragments/pixels which are written to the framebuffer (and not culled by
Z/stencil/alpha testing or shader KILL instructions).


Conditional Rendering
^^^^^^^^^^^^^^^^^^^^^

A drawing command can be skipped depending on the outcome of a query
(typically an occlusion query).  The ``render_condition`` function specifies
the query which should be checked prior to rendering anything.

If ``render_condition`` is called with ``query`` = NULL, conditional
rendering is disabled and drawing takes place normally.

If ``render_condition`` is called with a non-null ``query`` subsequent
drawing commands will be predicated on the outcome of the query.  If
the query result is zero subsequent drawing commands will be skipped.

If ``mode`` is PIPE_RENDER_COND_WAIT the driver will wait for the
query to complete before deciding whether to render.

If ``mode`` is PIPE_RENDER_COND_NO_WAIT and the query has not yet
completed, the drawing command will be executed normally.  If the query
has completed, drawing will be predicated on the outcome of the query.

If ``mode`` is PIPE_RENDER_COND_BY_REGION_WAIT or
PIPE_RENDER_COND_BY_REGION_NO_WAIT rendering will be predicated as above
for the non-REGION modes but in the case that an occulusion query returns
a non-zero result, regions which were occluded may be ommitted by subsequent
drawing commands.  This can result in better performance with some GPUs.
Normally, if the occlusion query returned a non-zero result subsequent
drawing happens normally so fragments may be generated, shaded and
processed even where they're known to be obscured.


Flushing
^^^^^^^^

``flush``


Resource Busy Queries
^^^^^^^^^^^^^^^^^^^^^

``is_texture_referenced``

``is_buffer_referenced``



Blitting
^^^^^^^^

These methods emulate classic blitter controls. They are not guaranteed to be
available; if they are set to NULL, then they are not present.

These methods operate directly on ``pipe_surface`` objects, and stand
apart from any 3D state in the context.  Blitting functionality may be
moved to a separate abstraction at some point in the future.

``surface_fill`` performs a fill operation on a section of a surface.

``surface_copy`` blits a region of a surface to a region of another surface,
provided that both surfaces are the same format. The source and destination
may be the same surface, and overlapping blits are permitted.

The interfaces to these calls are likely to change to make it easier
for a driver to batch multiple blits with the same source and
destination.

