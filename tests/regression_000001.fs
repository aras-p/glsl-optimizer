varying vec2 TexCoords;

uniform sampler2D baseTexture;

void main()
{
    vec2 what = vec2(1,2);
    vec2 tc = TexCoords + what;
    vec4 oc = texture2D(baseTexture, tc);
    gl_FragColor = oc;
} 