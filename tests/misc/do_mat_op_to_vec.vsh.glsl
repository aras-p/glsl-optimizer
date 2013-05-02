uniform mat4 sys_WorldMatrix;

void main()
{
	gl_Position = sys_WorldMatrix * vec4( 1.0, 0.0, 0.0, 0.0 );
}
