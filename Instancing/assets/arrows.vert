#version 150

uniform mat4 ciModelView;
uniform mat4 ciProjectionMatrix;
uniform mat3 ciNormalMatrix;
uniform mat4 ciModelViewProjection;

uniform vec2 uOffset = vec2( 0.0, 0.0 );

in vec4 ciPosition;
in vec4 ciColor;
in vec2 ciTexCoord0;

in mat4 vInstanceTransform;
in vec4 vInstanceData;
in vec4 vTexCoord;

out vec4 vertColor;
out vec2 texCoord;

void main(void)
{
	mat4 viewMatrix = ciModelView * vInstanceTransform;
	vec4 vertPosition = viewMatrix * ciPosition;
	vertColor = vInstanceData * ciColor;
	texCoord = ciTexCoord0 * vTexCoord.zw + vTexCoord.xy;

    gl_Position = ciProjectionMatrix * vertPosition + vec4( uOffset, 0.0, 0.0 );
}
