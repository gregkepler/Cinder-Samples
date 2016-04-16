#version 150

uniform sampler2D uTex0;

in vec2 texCoord;
in vec4 vertColor;

out vec4 fragColor;

void main(void)
{
	float d = length( texCoord );
	float w = fwidth( d );
	float c = smoothstep( 1.0 + w, 1.0 - w, d );


	fragColor = texture( uTex0, texCoord );
//	fragColor *= vertColor;
	
//	fragColor.a = fragColor.a * c;
	// fragColor = vec4(1 ,0, 0, 1);
}
