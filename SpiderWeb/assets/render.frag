#version 330 core

in vec3 oVel;
in vec4 oColor;

layout (location = 0) out vec4 color;


void main(void)
{
	vec3 colorSpectrum = clamp( normalize( abs( oVel ) ), vec3( 0.1 ), vec3( 1.0 ) );
//	color = vec4( 1.0 - colorSpectrum, oColor.a );
//	color = vec4( vec3(1.0, 1.0, 1.0), 0.5 );
	color = vec4( oColor.rgb, oColor.a * colorSpectrum.r );
}
