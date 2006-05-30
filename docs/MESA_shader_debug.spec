Name

    MESA_shader_debug

Name Strings

    GL_MESA_shader_debug

Contact

    Brian Paul (brian.paul 'at' tungstengraphics.com)
    Michal Krol (mjkrol 'at' gmail.com)

Status

    XXX - Not complete yet!!!

Version

    Last Modified Date: May 29, 2006
    Author Revision: 0.1
    $Date: 2006/05/30 09:35:36 $ $Revision: 1.1 $

Number

    TBD

Dependencies

    OpenGL 1.5 is required.
    The extension is written against the OpenGL 1.5 specification.
    ARB_shading_language_100 is required.
    ARB_shader_objects is required.
    The extension is written against the OpenGL Shading Language
    1.10 Specification.

Overview

    TBD

IP Status

    None

Issues

    None

New Procedures and Functions

    TBD

New Types

    None

New Tokens

    TBD

Additions to Chapter 2 of the OpenGL 1.5 Specification
(OpenGL Operation)

    None

Additions to Chapter 3 of the OpenGL 1.5 Specification (Rasterization)

    None

Additions to Chapter 4 of the OpenGL 1.5 Specification (Per-Fragment
Operations and the Frame Buffer)

    None

Additions to Chapter 5 of the OpenGL 1.5 Specification
(Special Functions)

    None

Additions to Chapter 6 of the OpenGL 1.5 Specification (State and State
Requests)

    None

Additions to Appendix A of the OpenGL 1.5 Specification (Invariance)

    None

Additions to Chapter 1 of the OpenGL Shading Language 1.10 Specification
(Introduction)

    None

Additions to Chapter 2 of the OpenGL Shading Language 1.10 Specification
(Overview of OpenGL Shading)

    None

Additions to Chapter 3 of the OpenGL Shading Language 1.10 Specification
(Basics)

    None

Additions to Chapter 4 of the OpenGL Shading Language 1.10 Specification
(Variables and Types)

    None

Additions to Chapter 5 of the OpenGL Shading Language 1.10 Specification
(Operators and Expressions)

    None

Additions to Chapter 6 of the OpenGL Shading Language 1.10 Specification
(Statements and Structure)

    None

Additions to Chapter 7 of the OpenGL Shading Language 1.10 Specification
(Built-in Variables)

    None

Additions to Chapter 8 of the OpenGL Shading Language 1.10 Specification
(Built-in Functions)

    Add a new section 8.10 "Debug Functions":

    Debug functions are available to both fragment and vertex shaders.
    They are used to track the execution of a shader by logging
    passed-in arguments to the shader's info log. That values can be
    retrieved and validated by the application after shader execution
    is complete.

    void printMESA(const float value);
    void printMESA(const int value);
    void printMESA(const bool value);
    void printMESA(const vec2 value);
    void printMESA(const vec3 value);
    void printMESA(const vec4 value);
    void printMESA(const ivec2 value);
    void printMESA(const ivec3 value);
    void printMESA(const ivec4 value);
    void printMESA(const bvec2 value);
    void printMESA(const bvec3 value);
    void printMESA(const bvec4 value);
    void printMESA(const mat2 value);
    void printMESA(const mat3 value);
    void printMESA(const mat4 value);
    void printMESA(const sampler1D value);
    void printMESA(const sampler2D value);
    void printMESA(const sampler3D value);
    void printMESA(const samplerCube value);
    void printMESA(const sampler1DShadow value);
    void printMESA(const sampler2DShadow value);

Additions to Chapter 9 of the OpenGL Shading Language 1.10 Specification
(Shading Language Grammar)

    None

Additions to Chapter 10 of the OpenGL Shading Language 1.10
Specification (Issues)

    None

Additions to the AGL/EGL/GLX/WGL Specifications

    None

GLX Protocol

    None

Errors

    TBD

New State

    TBD

New Implementation Dependent State

    TBD

Sample Code

    TBD

Revision History

    29 May 2006
        Initial draft. (Michal Krol)
