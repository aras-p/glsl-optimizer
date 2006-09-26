/*
 * Vertex shader test.
 * Uses all conventional attributes and 15 generic attributes to print
 * their values, using printMESA() extension function, to the debugger
 * to compare them with the actual passed-in values.
 * Use different types for generic attributes to check matrix handling.
 *
 * Author: Michal Krol
 */

#version 110

#extension MESA_shader_debug: require

attribute vec4 Attribute1;
attribute vec4 Attribute2;
attribute vec4 Attribute3;
attribute float Attribute4;
attribute vec2 Attribute5;
attribute vec3 Attribute6;
attribute mat2 Attribute7;
attribute mat3 Attribute9;
attribute mat4 Attribute12;

void main ()
{
   //
   // Do some legal stuff.
   //
   gl_Position = gl_ModelViewMatrix * gl_Vertex;
   gl_FrontColor = vec4 (1.0);

   //
   // Conventional attributes - except for gl_Vertex.
   //
   printMESA (gl_Color);
   printMESA (gl_SecondaryColor);
   printMESA (gl_Normal);
   printMESA (gl_MultiTexCoord0);
   printMESA (gl_MultiTexCoord1);
   printMESA (gl_MultiTexCoord2);
   printMESA (gl_MultiTexCoord3);
   printMESA (gl_MultiTexCoord4);
   printMESA (gl_MultiTexCoord5);
   printMESA (gl_MultiTexCoord6);
   printMESA (gl_MultiTexCoord7);
   printMESA (gl_FogCoord);

   //
   // Generic attributes - attrib with index 0 is not used because it would
   // alias with gl_Vertex, which is not allowed.
   //
   printMESA (Attribute1);
   printMESA (Attribute2);
   printMESA (Attribute3);
   printMESA (Attribute4);
   printMESA (Attribute5);
   printMESA (Attribute6);
   printMESA (Attribute7);
   printMESA (Attribute9);
   printMESA (Attribute12);

   //
   // Vertex position goes last.
   //
   printMESA (gl_Vertex);
}

