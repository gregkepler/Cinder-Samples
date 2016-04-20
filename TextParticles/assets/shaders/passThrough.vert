#version 150

uniform mat4 ciProjectionMatrix;
uniform mat4 ciModelViewProjection;

//varying vec4 texCoord;
in vec2 ciTexCoord0;
in vec4 ciPosition;

out vec2 texCoord;

void main()
{
	texCoord	= ciTexCoord0;
	
//	texCoord		= gl_MultiTexCoord0;
//	gl_Position		= ciModelViewProjection * gl_Vertex;
	gl_Position = ciModelViewProjection * ciPosition;
}