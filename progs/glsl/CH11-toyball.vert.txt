//
// Fragment shader for procedurally generated toy ball
//
// Author: Bill Licea-Kane
//
// Copyright (c) 2002-2003 ATI Research 
//
// See ATI-License.txt for license information
//

varying vec4 ECposition;   // surface position in eye coordinates
varying vec4 ECballCenter; // ball center in eye coordinates
uniform vec4 BallCenter;   // ball center in modelling coordinates

void main()
{ 
//orig:    ECposition   = gl_ModelViewMatrix * gl_Vertex;

    ECposition = gl_TextureMatrix[0] * gl_Vertex;
    ECposition = gl_ModelViewMatrix * ECposition;

    ECballCenter = gl_ModelViewMatrix * BallCenter;
    gl_Position  = ftransform();
}
