uniform vec4 any;

float deg2rad(float deg)
{
	return deg * 3.141592 / 180.0;
}

void main()
{
	vec4 res;
	res.x = deg2rad(any.x);
	res.y = mix(0.0, any.x, any.y);
	res.z = mix(any.z, 0.0, any.y);
	res.w = 0.0;
	gl_FragData[0] = res;
}