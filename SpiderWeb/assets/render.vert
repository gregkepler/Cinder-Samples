#version 330 core

layout (location = 0) in vec3 position;	// POSITION_INDEX
layout (location = 1) in vec3 velocity;	// VELOCITY_INDEX
layout (location = 4) in vec4 color;	// COLOR_INDEX

uniform mat4 ciModelViewProjection;

out vec4 oColor;
out vec3 oVel;

void main(void)
{
	gl_Position = ciModelViewProjection * vec4(position, 1.0);
	oVel =  velocity;
	oColor = color;
}
