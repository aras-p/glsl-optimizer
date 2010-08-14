Debugging
=========

Debugging utilities in gallium.

Debug Variables
^^^^^^^^^^^^^^^

All drivers respond to a couple of debug enviromental variables. Below is
a collection of them. Set them as you would any normal enviromental variable
for the platform/operating system you are running. For linux this can be
done by typing "export var=value" into a console and then running the
program from that console.

Common
""""""

GALLIUM_PRINT_OPTIONS <bool> (false)

This options controls if the debug variables should be printed to stderr.
This is probably the most usefull variable since it allows you to find
which variables a driver responds to.

GALLIUM_RBUG <bool> (false)

Controls if the :ref:`rbug` should be used.

GALLIUM_TRACE <string> ("")

If not set tracing is not used, if set it will write the output to the file
specifed by the variable. So setting it to "trace.xml" will write the output
to the file "trace.xml".

GALLIUM_DUMP_CPU <bool> (false)

Dump information about the current cpu that the driver is running on.

TGSI_PRINT_SANITY <bool> (false)

Gallium has a inbuilt shader sanity checker, this option controls if results
from it should be printed. This include warnings such as unused variables.

DRAW_USE_LLVM <bool> (false)

Should the :ref:`draw` module use llvm for vertex and geometry shaders.

ST_DEBUG <flags> (0x0)

Debug :ref:`flags` for the GL state tracker.


Driver specific
"""""""""""""""

I915_DEBUG <flags> (0x0)

Debug :ref:`flags` for the i915 driver.

I915_NO_HW <bool> (false)

Stop the i915 driver from submitting commands to the hardware.

I915_DUMP_CMD <bool> (false)

Dump all commands going to the hardware.

LP_DEBUG <flags> (0x0)

Debug :ref:`flags` for the llvmpipe driver.

LP_NUM_THREADS <int> (num cpus)

Number of threads that the llvmpipe driver should use.


.. _flags:

Flags
"""""

The variables of type <flags> all take a string with comma seperated
flags to enable different debugging for different parts of the drivers
or state tracker. If set to "help" the driver will print a list of flags
to which the variable can be set to. Order does not matter.


.. _rbug:

Remote Debugger
^^^^^^^^^^^^^^^

Or rbug for short allows for runtime inspections of :ref:`Context`,
:ref:`Screen`, Resources and Shaders; pauseing and stepping of draw calls;
and runtime disable and replacement of shaders. Is used with rbug-gui which
is hosted outside of the main mesa repositor. Rbug is can be used over a
network connection so the debbuger does not need to be on the same machine.
