#version 120
/* PASS */

uniform mat2x3 a;
uniform mat3x2 b;
uniform mat3x3 c;
uniform mat3x3 d;

void main()
{
    mat3x3 x;

    /* Multiplying a 2 column, 3 row matrix with a 3 column, 2 row matrix
     * results in a 3 column, 3 row matrix.
     */
    x = (a * b) + c / d;

    gl_Position = gl_Vertex;
}
