#version 150
uniform sampler2D	uNoiseTex;
uniform float		uTime;
uniform float		uAlpha;

in vec4		vColor;
in vec2		vTexCoord;

out vec4 fragColor;

void main(void)
{
	float noiseVal		= texture( uNoiseTex, vTexCoord + vec2( uTime * 0.1, uTime * 0.1 ) ).r;
    float alpha        	= min( vColor.a, 1.0 ) * uAlpha * noiseVal;
	fragColor = vec4( alpha );
}