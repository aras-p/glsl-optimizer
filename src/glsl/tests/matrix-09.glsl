/* FAIL - matrix-to-matrix constructors are not available in GLSL 1.10 */

uniform mat3 a;

void main()
{
    mat2 m;

    m = mat2(a);
    gl_Position = gl_Vertex;
}
