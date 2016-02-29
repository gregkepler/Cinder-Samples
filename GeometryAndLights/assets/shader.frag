#version 150

uniform vec3 uAmbient;
uniform vec3 uLightColor;
uniform vec3 uLightDirection;

in vec4		ciPosition;
in vec3		Color;
in vec3		Normal;
in vec2		TexCoord;
in vec3		ec_pos;

out vec4 	oColor;

void main( void )
{

	vec3 N = normalize(cross(dFdx(ec_pos), dFdy(ec_pos)));	// for flat shading
	float diffuse = max(0.0, dot(N, uLightDirection));
	vec3 scatteredLight = uAmbient + uLightColor * diffuse;
	vec3 rgb = min( scatteredLight, vec3(1.0));
	oColor = vec4(rgb, 1.0);
}
