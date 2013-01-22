#ifdef GL_ES
precision highp float;
#endif

uniform float time;
uniform vec2 resolution;

float u( float x ) 
{
if (x > 0.0) return 1.0; 
else return 0.0;
}

float atanSurrogate(float y, float x)
{
float z = y/x;
float ab = abs(z);
float res;

if (ab<0.9626995623)
{
    return z/(1.0 + 0.28*z*z);
}
else if(ab > 1.094294782)
{
    res = - z/(z*z + 0.28);

    if (z>0.0) return res + 3.14159/2.0;
    else return res - 3.14159/2.0;
}
else
{
    res = 0.5*z;
    if (z>0.0) return res + 0.283;
    else return res - 0.283;
}
}

void main(void)
{
vec2 p = (2.0*gl_FragCoord.xy-resolution)/resolution.y;

float a = atanSurrogate(p.x, p.y);

float r = length(p)*.75;

float w = cos(3.1415927*time-r*2.0);
float h = 0.5+0.5*cos(12.0*a-w*7.0+r*8.0);
float d = 0.25+0.75*pow(h,1.0*r)*(0.7+0.3*w);

float col = u( d-r ) * sqrt(1.0-r/d)*r*2.5;
col *= 1.25+0.25*cos((12.0*a-w*7.0+r*8.0)/2.0);
col *= 1.0 - 0.35*(0.5+0.5*sin(r*30.0))*(0.5+0.5*cos(12.0*a-w*7.0+r*8.0));
gl_FragColor = vec4(
    col,
    col-h*0.5+r*.2 + 0.35*h*(1.0-r),
    col-h*r + 0.1*h*(1.0-r),
    1.0);
}