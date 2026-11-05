GLSL optimizer
==============

> :warning: As of mid-2016, this project is unlikely to have any significant developments.
> This project was developed for Unity needs around year 2010, but by year 2016 Unity's
> shader compilation pipeline has evolved into a different one, and glsl-optimizer was no
> longer used there. So from my side there won't be significant work done on it. :warning:

A C++ library that takes GLSL vertex/fragment/compute shaders, does GPU-independent optimizations on them
and outputs GLSL or Metal source back. Optimizations are function inlining, dead code removal, copy propagation,
constant folding, constant propagation, arithmetic optimizations and so on.

Apparently quite a few mobile platforms are pretty bad at optimizing shaders; and
unfortunately they *also* lack offline shader compilers. So using a GLSL optimizer offline
before can make the shader run much faster on a platform like that. See performance numbers
in [this blog post](https://aras-p.info/blog/2010/09/29/glsl-optimizer/).

Even for drivers that have decent shader optimization, GLSL optimizer could be useful to just strip away
dead code, make shaders smaller and do uniform/input reflection offline.

Almost all actual code is [Mesa 3D's GLSL](https://gitlab.freedesktop.org/mesa/mesa)
compiler; all this library does is spits out optimized GLSL or Metal back, and adds GLES type precision
handling to the optimizer.

This GLSL optimizer is made for [Unity's](https://unity3d.com/) purposes and is built-in
starting with Unity 3.0.

GLSL Optimizer is licensed according to the terms of the MIT license.

See [change log here](Changelog.md).


Build
-----

Build with CMake 3.21 or newer and a C/C++17 compiler on Windows, macOS, or Linux, using
the typical CMake command lines like:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

To generate IDE projects, add `-G "Visual Studio 17 2022" -A x64` on Windows, or `-G Xcode`
on macOS to the first command.

The `glsl_optimizer` target produces a static library. To use it in another
CMake project, use `add_subdirectory` and link against `glsl_optimizer`; the public
header include directory is propagated automatically. Tests default to off when
embedded. MSVC builds use the static runtime by default; callers can override this
with `CMAKE_MSVC_RUNTIME_LIBRARY`.

CMake options:

* `BUILD_TESTING=OFF` builds without the regression runner.
* `GLSLOPT_BUILD_TOOLS=ON` builds the optional `glslopt` command-line optimizer.
  Run it without arguments for usage.
* `GLSLOPT_TEST_OPENGL=ON` enables optional external OpenGL validation in the test
  runner on Windows/macOS. It is off by default and is not needed for regression tests.

> There's a Node addon contributed; that is built via a separate `node-gyp` build.
> Its JavaScript smoke test is `node tests/tests.js` after building the addon. I have no
> idea if or how it works.

Usage
-----

Interface for the library is `src/glsl/glsl_optimizer.h`. General usage is:

```c 
ctx = glslopt_initialize(targetVersion);
for (lots of shaders) {
    shader = glslopt_optimize (ctx, shaderType, shaderSource, options);
    if (glslopt_get_status (shader)) {
        newSource = glslopt_get_output (shader);
    } else {
        errorLog = glslopt_get_log (shader);
    }
    glslopt_shader_delete (shader);
}
glslopt_cleanup (ctx);
```


Language Support
----------------

* Desktop GLSL: versions from 1.10 through 4.50 (no `#version` defaults to 1.10);
  they can produce GLSL or Metal.
* GLSL ES: `#version 300 es`, `310 es`, and `320 es` are supported (no ES2.0 GLSL!),
  and these produce either GLSL ES or Metal.
* Supported shader stages are vertex, fragment, and compute.

#### Limitations

* Last time this library was used "in production" in a big product (Unity) was around
  2015, anything since then got much less "real" testing.
* Geometry and tessellation shaders are not supported.
* Subgroup operations (`GL_KHR_shader_subgroup`) are not supported.
* Variable workgroup sizes (`GL_ARB_compute_variable_group_size`) are not supported.

#### Metal notes

* Metal: texture buffers require Metal 2.1+. Integer image atomics require
  Metal 3.1+ (4.0+ for cube images) and GPU support for texture
  atomics.
* Metal: multisample storage images support reads and queries only.
* Metal: `memoryBarrier*()` and `groupMemoryBarrier()` translate to `atomic_thread_fence`
  and thus require Metal 3.2 or later.
* Metal: coherent images and SSBOs require Metal 3.2+.
* Metal: buffer atomics require 32-bit `int` or `uint` storage. Atomics on
  shared variables require `highp` storage. Metal vertex shaders can only read
  atomic counters.

#### Resource reflection

* For compute dispatch on Metal, use `glslopt_shader_get_compute_local_size` to
  get the `threadsPerThreadgroup` dimensions.
* Texture reflection includes sampled textures and storage images.
  - GLSL: reflected location is the explicit binding if present, otherwise the
    explicit uniform location or `-1`.
  - Metal: the texture slot. Images follow sampled textures and consume only
    texture slots. Metal image resource arrays must have a fixed size and one dimension;
    their elements occupy consecutive texture slots.
* Buffer reflection lists blocks separately from ordinary uniforms. The source
  binding is `-1` when absent, and block arrays use consecutive bindings.
  Metal reserves buffer slot 0 for ordinary uniforms. Use the `metalBufferIndex` when
  binding a block.
* Explicit `std140` and `std430` layouts are preserved. For GLSL output,
  `shared` and `packed` block sizes, offsets, and strides are unknown
  (reported as `-1`). Metal uses `std140` for `shared` and `packed` UBOs and `std430` for
  those SSBOs. If any SSBO member is coherent, Metal applies device coherence to the whole block.
* Atomic counters sharing a binding form one reflected buffer named
  `atomic_counters_BINDING`, with layout `kGlslLayoutAtomicCounter`.
  Counters are 32-bit unsigned values at their reflected byte offsets, with
  an innermost array stride of 4 bytes. Use the reflected outer strides for
  multidimensional arrays. Metal allocates counter slots after block buffers
  and their optional size table. Initialize the counters on the host and bind
  the buffer at `metalBufferIndex`, writable if the shader updates it.
  Counter operations use relaxed ordering and barriers remain explicit.

#### Runtime-sized SSBO arrays on Metal

When a shader queries length of a runtime-sized array, bind one table of
`uint32_t` byte sizes for that shader stage at the reflected
`metalSizeBufferIndex` (all blocks that need the table report the same bind slot).
A block that does not need it reports `-1`.

Entry `metalBufferIndex + i` gives the size of the bound byte range for block
array element `i`, measured from the bound offset and including any header
before the runtime-sized array. Include entries through the highest data-buffer
slot that needs a size. Unused entries can be zero. The shader computes the
array length as `max(bufferBytes - arrayOffset, 0) / arrayStride`.

For example, if a fragment shader needs the sizes of data buffers at slots 1
and 3 and reports a table slot of 4, bind itlike this.

```objc
uint32_t sizes[] = {0, bytesForBuffer1, 0, bytesForBuffer3};
[encoder setFragmentBytes:sizes length:sizeof(sizes) atIndex:4]; // or setVertexBytes or setBytes
```


Tests
-----

CMake's CTest runs the shader regression suite. Each shader input is compared against
expected optimized output, including statistics and reflection data. GLES3 cases
also check generated Metal output on every platform.

The test runner can also be invoked directly as `glsl_optimizer_tests /path/to/tests`.
Mismatches leave expected files untouched and write actual output under
`test-output/` in the working directory (the build directory when using CTest).

To regenerate expectations, run the executable with
`/path/to/tests --update-expected` and review the changed files.
Optional `--check-glsl` validates against the system OpenGL compiler (requires
`GLSLOPT_TEST_OPENGL=ON`); `--check-metal` invokes the iPhoneOS Metal compiler
through `xcrun` on macOS.

If you're making changes to the project and want pull requests accepted easier, I'd
appreciate if there would be no test suite regressions. If you are implementing a
feature, it would be cool to add tests to cover it as well!


Dev Notes
---------

- Current Mesa codebase is from [Mesa 20.3.5](https://archive.mesa3d.org/older-versions/20.x/mesa-20.3.5.tar.xz).
- Use release snapshots, not merges with whole Mesa history.
- Only minimal subset of that, as needed by the glsl optimizer. A bunch
  of files were reduced to only contain what we need; they are mostly marked
  with `glsl-optimizer change:` comments.
- In order to rebuild the generated GLSL/glcpp lexers, parsers and some tables,
  you need to run `./generateParsers.sh`.

