//
// Vertex shader for procedural bumps
//
// Authors: Randi Rost, John Kessenich
//
// Copyright (c) 2002-2006 3Dlabs Inc. Ltd. 
//
// See 3Dlabs-License.txt for license information
//

varying vec3 LightDir;
varying vec3 EyeDir;

uniform vec3 LightPosition;

attribute vec3 Tangent;

void main() 
{
    EyeDir         = vec3(gl_ModelViewMatrix * gl_Vertex);
    gl_Position    = ftransform();
    gl_TexCoord[0] = gl_MultiTexCoord0;

    vec3 n = normalize(gl_NormalMatrix * gl_Normal);
    vec3 t = normalize(gl_NormalMatrix * Tangent);
    vec3 b = cross(n, t);

    vec3 v;
    v.x = dot(LightPosition, t);
    v.y = dot(LightPosition, b);
    v.z = dot(LightPosition, n);
    LightDir = normalize(v);

    v.x = dot(EyeDir, t);
    v.y = dot(EyeDir, b);
    v.z = dot(EyeDir, n);
    EyeDir = normalize(v);
}
