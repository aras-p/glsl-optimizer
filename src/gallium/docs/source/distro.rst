Distribution
============

Along with the interface definitions, the following drivers, state trackers,
and auxiliary modules are shipped in the standard Gallium distribution.

Drivers
-------

Cell
^^^^

Failover
^^^^^^^^

Deprecated.

Intel i915
^^^^^^^^^^

Intel i965
^^^^^^^^^^

Highly experimental.

Identity
^^^^^^^^

Wrapper driver.

LLVM Softpipe
^^^^^^^^^^^^^

nVidia nv04
^^^^^^^^^^^

Deprecated.

nVidia nv10
^^^^^^^^^^^

Deprecated.

nVidia nv20
^^^^^^^^^^^

Deprecated.

nVidia nv30
^^^^^^^^^^^

nVidia nv40
^^^^^^^^^^^

nVidia nv50
^^^^^^^^^^^

VMWare SVGA
^^^^^^^^^^^

ATI r300
^^^^^^^^

Testing-quality.

Softpipe
^^^^^^^^

Reference software rasterizer.

Trace
^^^^^

Wrapper driver.

State Trackers
--------------

Direct Rendering Infrastructure
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

EGL
^^^

GLX
^^^

MesaGL
^^^^^^

Python
^^^^^^

OpenVG
^^^^^^

WGL
^^^

Xorg XFree86 DDX
^^^^^^^^^^^^^^^^

Auxiliary
---------

CSO Cache
^^^^^^^^^

The CSO cache is used to accelerate preparation of state by saving
driver-specific state structures for later use.

.. _draw:

Draw
^^^^

Draw is a software :term:`TCL` pipeline for hardware that lacks vertex shaders
or other essential parts of pre-rasterization vertex preparation.

Gallivm
^^^^^^^

Indices
^^^^^^^

Indices provides tools for translating or generating element indices for
use with element-based rendering.

Pipe Buffer Managers
^^^^^^^^^^^^^^^^^^^^

Each of these managers provides various services to drivers that are not
fully utilizing a memory manager.

Remote Debugger
^^^^^^^^^^^^^^^

Runtime Assembly Emission
^^^^^^^^^^^^^^^^^^^^^^^^^

TGSI
^^^^

The TGSI auxiliary module provides basic utilities for manipulating TGSI
streams.

Translate
^^^^^^^^^

Util
^^^^

