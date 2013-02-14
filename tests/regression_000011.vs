varying lowp vec3 outputA ;
uniform vec3 inputA;
uniform vec3 inputB;
uniform vec3 inputC;

void main()
{
vec3 tmp1 =  ( ( inputA.xyz / vec3( 127.5 ) ) - vec3( 1.0 ) ) ;
vec3 tmp2 =  vec3( 0, 0, 0 )  - (2.0 * inputB).xyz;

vec3 tmp3 =  (2.0 * tmp1) ;
tmp3 = normalize(tmp3.xyz);
vec3 tmp4 = normalize( tmp2.xyz );
vec3 tmp5 = normalize( inputC + tmp4 );
outputA = vec3(dot( tmp3, tmp5 ), 0.0, 0.0);
}