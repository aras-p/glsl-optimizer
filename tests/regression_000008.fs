varying vec2 uv;
uniform vec2 uvOffset;
uniform sampler2D tex;
uniform vec4 colorFilter;


void main(void)
{
	vec4 color = texture2D(tex, uvOffset + uv);
	gl_FragColor = color * colorFilter;
}