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

OS
^^

The OS module contains the abstractions for basic operating system services:

* memory allocation
* simple message logging
* obtaining run-time configuration option
* threading primitives

This is the bare minimum required to port Gallium to a new platform.

The OS module already provides the implementations of these abstractions for
the most common platforms.  When targeting an embedded platform no
implementation will be provided -- these must be provided separately.

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

