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

Non-CSO State
^^^^^^^^^^^^^

These pieces of state are too small, variable, and/or trivial to have CSO
objects. They all follow simple, one-method binding calls, e.g.
``set_edgeflags``.

* ``set_edgeflags``
* ``set_blend_color``
* ``set_clip_state``
* ``set_constant_buffer``
* ``set_framebuffer_state``
* ``set_polygon_stipple``
* ``set_scissor_state``
* ``set_viewport_state``
* ``set_fragment_sampler_textures``
* ``set_vertex_sampler_textures``
* ``set_vertex_buffers``
* ``set_vertex_elements``

Queries
^^^^^^^

Queries can be created with ``create_query`` and deleted with
``destroy_query``. To enable a query, use ``begin_query``, and when finished,
use ``end_query`` to stop the query. Finally, ``get_query_result`` is used
to retrieve the results.

VBO Drawing
^^^^^^^^^^^

``draw_arrays``

``draw_elements``

``draw_range_elements``

``flush``

Surface Drawing
^^^^^^^^^^^^^^^

These methods emulate classic blitter controls. They are not guaranteed to be
available; if they are set to NULL, then they are not present.

``surface_fill`` performs a fill operation on a section of a surface.

``surface_copy`` blits a region of a surface to a region of another surface,
provided that both surfaces are the same format. The source and destination
may be the same surface, and overlapping blits are permitted.

``clear`` initializes entire buffers to an RGBA, depth, or stencil value,
depending on the formats of the buffers. Use ``set_framebuffer_state`` to
specify the buffers to clear.
