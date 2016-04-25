#version 150 core

uniform float uMouseForce;
uniform vec3  uMousePos;

uniform sampler2D	uPerlinTex;
uniform float		uTime = 1.0;
uniform float		uStep = 1.0;
uniform float		uDampingSpeed = 0.004;
uniform vec3		uNoiseOffset = vec3( 1.0, 1.0, 0.0 );
uniform vec3		uEndColor = vec3( 1.0, 1.0, 1.0 );

in vec3   iPosition;
in vec3   iPPosition;
in float  iDamping;
in vec4   iColor;
in vec2	  iTexCoord;
in float  iInvMass;

out vec3  position;
out vec3  pposition;
out float damping;
out vec4  color;
out vec2  texcoord;
out float invmass;

const float dt2 = 1.0 / (60.0 * 60.0);

void main()
{
	position =  iPosition;
	pposition =  iPPosition;
	damping =   iDamping;
	color =     iColor;
	texcoord =  iTexCoord;
	invmass =	iInvMass;
	
	vec3 vel	   = (position - pposition);
	vec3 direction = normalize( vel );
	vec3 perlin	   = ( texture( uPerlinTex, texcoord * uNoiseOffset.xy ).rgb * vec3( 2.0 ) ) - vec3(1.0);	// [-1.0,1.0]
	vec3 acc	   = direction + ((perlin * invmass) * uStep);
	
	// UPDATE damping
	damping -= uDampingSpeed;
	damping = max( damping, 0 );	// min is 0
	
	// UPDATE velocity
	vel += acc;
	vel *= damping;
	
	// UPDATE position and previous position
	pposition = position;
	position += vel;
	
	// UPDATE color
	float a = clamp( damping * 2.0, 0.0, 1.0 );			// alpha
	float colorVal = clamp((a * 2.0 ), 0.0, 1.0 );		// color value
	vec3 newColor = (vec3(1) - uEndColor ) * vec3( colorVal );
	color.rgb *= ( uEndColor + newColor );
	color.a = a;
}