GLSL optimizer
==============

> :warning: As of mid-2016, this project is unlikely to have any significant developments.
> This project was developed for Unity needs around year 2010, but by year 2016 Unity's
> shader compilation pipeline has evolved into a different one, and glsl-optimizer was no
> longer used there. So from my side there won't be significant work done on it. :warning:

A C++ library that takes GLSL vertex/fragment/compute shaders, does GPU-independent optimizations on them
and outputs GLSL or Metal source back. Optimizations are function inlining, dead code removal, copy propagation,
constant folding, constant propagation, arithmetic optimizations, loop unrolling and so on (Mesa's NIR optimizer).

Apparently quite a few mobile platforms are pretty bad at optimizing shaders; and
unfortunately they *also* lack offline shader compilers. So using a GLSL optimizer offline
before can make the shader run much faster on a platform like that. See performance numbers
in [this blog post](https://aras-p.info/blog/2010/09/29/glsl-optimizer/).

Even for drivers that have decent shader optimization, GLSL optimizer could be useful to just strip away
dead code, make shaders smaller and do uniform/input reflection offline.

Almost all actual code is [Mesa 3D's GLSL](https://gitlab.freedesktop.org/mesa/mesa)
compiler and NIR optimizer; all this library does is spits out optimized GLSL or Metal back, and adds GLES type precision
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
  Use `-m` for Metal output. Run it without arguments for usage.
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
* GLSL ES: the OpenGL ES and Metal targets accept `#version 300 es`, `310 es`,
  and `320 es`. The desktop OpenGL target accepts ES 3.00 input only.
* Supported shader stages are vertex, fragment, and compute.

#### Limitations

* Last time this library was used "in production" in a big product (Unity) was around
  2015, anything since then got much less "real" testing.
* Geometry and tessellation shaders are not supported.
* Double precision types and shader subroutines are not supported.
* Variable workgroup sizes (`GL_ARB_compute_variable_group_size`) are not supported.
* Each shader stage is optimized independently. Optimizer retains vertex outputs
  even if fragment shader never reads them, and it does not match or validate
  varyings across stages.

#### Metal notes

* Basic vertex and fragment shaders compile as Metal 1.2, but the version can
  become higher depending on the features it uses.
* Floating point `mediump` and `lowp` mostly translate to Metal `half`.
* Shadow sampling uses a fixed sampler with clamp-to-edge addressing,
  linear filtering, and a less-equal comparison. Shadow gathers use the bound sampler.
* No support for arrays of samplers, `gl_ClipDistance`,
  or sampled textures such as multisample, rectangle, buffer, 1D, or integer
  3D textures.
* Texture buffers require Metal 2.1+. Integer image atomics require
  Metal 3.1+ (4.0+ for cube images) and GPU support for texture
  atomics.
* Multisample storage images support reads and queries only.
* Subgroup scans other than add/mul and clustered reductions are emulated with
  shuffles and assume all lanes are active.
* `memoryBarrier*()` and `groupMemoryBarrier()` translate to `atomic_thread_fence`
  and thus require Metal 3.2 or later. A `memoryBarrierShared()` immediately
  followed by `barrier()` in the optimizer's instruction stream can use a single
  threadgroup barrier on earlier Metal versions.
* Coherent images and SSBOs require Metal 3.2+.
* Buffer atomics require 32-bit `int` or `uint` storage. Atomics on
  shared variables require `highp` storage. Metal vertex shaders can only read
  atomic counters.

#### Resource reflection

* Reflection returns up to 1024 global uniforms, 128 inputs, and 128 textures,
  including storage images.
* Metal packs global uniforms into one struct at `buffer(0)` using Metal's
  natural alignment and padding; reflected uniform location is a byte
  offset within that struct.
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
and 3 and reports a table slot of 4, bind it like this.

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

The Mesa code (`src/compiler`, `src/util`, `src/mesa`, `src/gallium`, `src/c11`,
`include`) is a small subset of the
[Mesa 26.2.6](https://archive.mesa3d.org/mesa-26.2.6.tar.xz) release: the GLSL
frontend, `glsl_to_nir`, NIR with the passes we run, and the util code they need.

#### Upgrading Mesa

- Copy the new versions of the files that exist here from the new release, then
  bring back the `glsl-optimizer change` edits. Files that fail to build usually
  need a header that we deliberately dropped (see below), not the header itself.
- Regenerate our minimal files with the scripts in `upgrade-tools/` (they take
  the path of an upstream Mesa tree): `gen_minimal_formats.py` (pipe_format
  enum + descriptions) and `extract_nir_helpers.py` (helpers copied out of NIR
  passes we do not build).
- Run `upgrade-tools/gen_sources.sh` to regenerate the checked-in lexers,
  parsers and NIR/GLSL tables (needs bison, flex, python3 with Mako).
- Check the test corpus output.

#### What we have removed on purpose

- GL context/driver state (vbo, state tracker, glapi, glthread, driver function
  tables, gallium state): an offline compiler has no GL context. `mtypes.h`,
  `context.h` and `pipe/p_screen.h` are minimal versions with just the members
  the compiler reads.
- Khronos GL/GLES headers: `util/glminimal.h` has the types and enums we use.
- Texture formats/codecs: only GLSL image formats are needed.
- Disk cache, BLAKE3 source hashes, fossilize, compression, NIR serialization,
  perf tracing, CPU detection, xxHash (hash tables use FNV-1a), most of
  logging/OS layers (`log.c`, `os_misc.c` are stderr/getenv-only).
- Driver-oriented compiler parts: fp64 emulation library, NIR IO/explicit-IO/
  int64/double lowering, divergence analysis, per-app workarounds, and all
  unused passes. A coverage run over the test corpus was used to find code that
  was linked but never executed.
- GLSL IR optimizations and `lower_precision`: NIR does all optimization.
- `nir_opt_loop`: it rotates loops into shapes that cannot be printed as `for`
  loops, with no measurable benefit for our use.

#### Our code (`src/glsl`)

- `glsl_optimizer.cpp`: public API, GL context setup, validation of what the
  printers can express, reflection and stats (all from the final NIR).
- `nir_optimize.cpp`: the NIR pipeline (inlining, optimization loop, out of SSA),
  and the GLSL ES precision analysis that the pipeline and printers share.
- `nir_printer.cpp`: printing core shared by GLSL and Metal: typing of NIR values,
  folding single-use values into expressions, and print-time recovery of shapes NIR
  lowered (matrix multiplies, `normalize`/`length`/`clamp`/..., `for`/`while`/`do` loops).
- `nir_print_glsl.cpp`, `nir_print_metal.cpp`: the dialects. `buffer_layout.h` has the
  std140/std430 and Metal slot rules shared by reflection and the Metal printer.

#### Behavior changes

- No GLSL IR optimization before `glsl_to_nir`: it erased stores into locals of
  different precision, i.e. the source's precision conversions.
- `glslopt_precision` NIR intrinsic: marks such precision conversions so they
  survive NIR optimization and get printed as properly qualified temporaries.
- Frontend changes (marked in the Mesa sources): a compile entry point that can skip
  the preprocessor and hands out the parse state (version, extensions) for printing;
  `texture()`/`textureProj()` bias overloads for external textures in fragment shaders;
  block-level locations allowed on arrays of interface blocks.

#### Generated code size

`nir_opt_algebraic.c` now stores its automaton tables compressed (the dense
`bcsel` table alone was 2.57M entries), only the two algebraic passes we use are
generated, and `upgrade-tools/compact_c_tables.py` strips whitespace from generated
tables. Original file was 51MB source, now about 4MB source.
