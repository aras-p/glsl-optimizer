import qbs

Project {
    references: [
        "src/glsl_optimizer.qbs"
    ]
    CppApplication {
        type: "application" // To suppress bundle generation on Mac
        name: "glsl-optimizer-test"
        consoleApplication: true
        cpp.debugInformation: true
        Depends { name: "glsl-optimizer" }
        property stringList StaticLibs: {
            if (qbs.targetOS.contains("windows"))
                return ["opengl32", "gdi32", "user32"];
            else if (qbs.targetOS.contains("linux"))
                if (qbs.toolchain.contains("clang"))
                    return ["GLEW", "GLU", "GL", "glut", "pthread"];
                else
                    return ["GLEW", "GLU", "GL", "glut"];
            else
                return [];
        }
        property stringList Frameworks: {
            if (qbs.targetOS.contains("darwin"))
                return ["OpenGL", "GLUT"];
        }
        files: "tests/glsl_optimizer_tests.cpp"
        cpp.staticLibraries: base.concat(StaticLibs)
        Properties {
            condition: qbs.targetOS.contains("darwin")
            cpp.frameworks: outer.concat(Frameworks)
        }
    }
}
