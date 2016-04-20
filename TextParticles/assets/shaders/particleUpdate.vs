#version 150 core

uniform float uMouseForce;
uniform vec3  uMousePos;

in vec3   iPosition;
in vec3   iPPosition;
in vec3   iHome;
in float  iDamping;
in vec4   iColor;

out vec3  position;
out vec3  pposition;
out vec3  home;
out float damping;
out vec4  color;

const float dt2 = 1.0 / (60.0 * 60.0);

void main()
{
	position =  iPosition;
	pposition =  iPPosition;
	damping =   iDamping;
	home =      iHome;
	color =     iColor;

	vec3 vel = (position - pposition) * damping;
	pposition = position;
	vec3 acc = (home - position) * 32.0f;
	position += vel + acc * dt2;
	
	
	
	
	
//	vec4 p0			= texture( positions, texCoord );
//	float invmass	= p0.a;
	
//	vec4 v0			= texture( velocities, texCoord );
//	float texAlpha	= v0.a;
	
//	vec3 newV		= texture( uPerlinTex, texCoord * vec2( 5.0, 1.0 ) ).rgb * 2.0 - 1.0;
	
//	float noiseVal	= texture( uNoiseTex, texCoord ).r;
	
/*
	vec3 force = p0.xyz - vec3( 0.0 );			// Calculate direction of force
    float d = length( force );					// Distance between objects
	d = normalize(d);
	force = normalize(force);					// Normalize vector (distance doesn't matter here, we just want this vector for direction
	
	float strength = invmass / (d * d);			// Calculate gravitational force magnitude
	force *= ( 1.0 * strength );
	
	float animDelay	= max( uTime * 4.0 -  texCoord.s - noiseVal, 0.0 );
	
	vec3 v1 = v0.xyz + newV * uStep;//( v0.xyz + newV * invmass ) * animDelay;
	vec3 p1 = p0.xyz + v1;*/
}