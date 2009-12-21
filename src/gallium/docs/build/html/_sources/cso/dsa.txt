Depth, Stencil, & Alpha
=======================

These three states control the depth, stencil, and alpha tests, used to
discard fragments that have passed through the fragment shader.

Traditionally, these three tests have been clumped together in hardware, so
they are all stored in one structure.

During actual execution, the order of operations done on fragments is always:

* Stencil
* Depth
* Alpha

Depth Members
-------------

enabled
    Whether the depth test is enabled.
writemask
    Whether the depth buffer receives depth writes.
func
    The depth test function. One of PIPE_FUNC.

Stencil Members
---------------

XXX document valuemask, writemask

enabled
    Whether the stencil test is enabled. For the second stencil, whether the
    two-sided stencil is enabled.
func
    The stencil test function. One of PIPE_FUNC.
ref_value
    Stencil test reference value; used for certain functions.
fail_op
    The operation to carry out if the stencil test fails. One of
    PIPE_STENCIL_OP.
zfail_op
    The operation to carry out if the stencil test passes but the depth test
    fails. One of PIPE_STENCIL_OP.
zpass_op
    The operation to carry out if the stencil test and depth test both pass.
    One of PIPE_STENCIL_OP.

Alpha Members
-------------

enabled
    Whether the alpha test is enabled.
func
    The alpha test function. One of PIPE_FUNC.
ref_value
    Alpha test reference value; used for certain functions.
