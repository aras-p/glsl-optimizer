GLSL optimizer
==============

> :warning: As of mid-2016, the project is unlikely to have any significant developments. At Unity we are moving to a different
shader compilation pipeline, with glsl-optimizer is not used. So from my side there won't be significant work done on it. :warning:


A C++ library that takes GLSL shaders, does some GPU-independent optimizations on them
and outputs GLSL or Metal source back. Optimizations are function inlining, dead code removal, copy propagation,
constant folding, constant propagation, arithmetic optimizations and so on.

Apparently quite a few mobile platforms are pretty bad at optimizing shaders; and
unfortunately they *also* lack offline shader compilers. So using a GLSL optimizer offline
before can make the shader run much faster on a platform like that. See performance numbers
in [this blog post](http://aras-p.info/blog/2010/09/29/glsl-optimizer/).

Even for drivers that have decent shader optimization, GLSL optimizer could be useful to just strip away
dead code, make shaders smaller and do uniform/input reflection offline.

Almost all actual code is [Mesa 3D's GLSL](http://cgit.freedesktop.org/mesa/mesa/log/)
compiler; all this library does is spits out optimized GLSL or Metal back, and adds GLES type precision
handling to the optimizer.

This GLSL optimizer is made for [Unity's](http://unity3d.com/) purposes and is built-in
starting with Unity 3.0.

GLSL Optimizer is licensed according to the terms of the MIT license.

See [change log here](Changelog.md).


Usage
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

Interface for the library is `src/glsl/glsl_optimizer.h`. General usage is:
 
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


Notes
-----

* GLSL versions 1.10 and 1.20 are supported. 1.10 is the default, use #version 120 to specify 
1.20. Higher GLSL versions might work, but aren't tested now.
* GLSL ES versions 1.00 and 3.00 are supported.


Dev Notes
---------

Pulling Mesa upstream:

    git fetch upstream
    git merge upstream/master
    sh removeDeletedByUs.sh
    # inspect files, git rm unneeded ones, fix conflicts etc.
    # git commit
    
Rebuilding flex/bison parsers:

* When .y/.l files are changed, the parsers are *not* rebuilt automatically,
* Run ./generateParsers.sh to do that. You'll need bison & flex (on Mac, do "Install Command Line Tools" from Xcode)
* I use bison 2.3 and flex 2.5.35 (in OS X 10.8/10.9)

