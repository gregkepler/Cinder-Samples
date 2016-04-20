#version 150
uniform sampler2D displacementMap;
uniform sampler2D velocityMap;
uniform mat4 ciModelViewProjection;

in vec2 ciTexCoord0;

out vec4 vColor;
out vec2 vTexCoord;

void main()
{
	vTexCoord		= ciTexCoord0;
	
    //using the displacement map to move vertices
	vec4 pos		= texture( displacementMap, ciTexCoord0 );
	//	pos.y -= 500.0; // we can set position offsets in the displacement map
    vColor			= texture( velocityMap, ciTexCoord0 );
	gl_Position		= ciModelViewProjection * vec4( pos.xyz, 1.0 ) ;
}