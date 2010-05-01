.. _vertexelements:

Vertex Elements
===============

This state controls format etc. of the input attributes contained
in the pipe_vertex_buffer(s). There's one pipe_vertex_element array member
for each input attribute.

Members
-------

src_offset
    The byte offset of the attribute in the buffer given by
    vertex_buffer_index for the first vertex.
instance_divisor
    The instance data rate divisor, used for instancing.
    0 means this is per-vertex data, n means per-instance data used for
    n consecutive instances (n > 0).
vertex_buffer_index
    The vertex buffer this attribute lives in. Several attributes may
    live in the same vertex buffer.
src_format
    The format of the attribute data. One of the PIPE_FORMAT tokens.
