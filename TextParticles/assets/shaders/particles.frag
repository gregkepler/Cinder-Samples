#version 150
uniform float		uTime;
uniform float		uStep;

uniform sampler2D	positions;
uniform sampler2D	velocities;
uniform sampler2D	uPerlinTex;
uniform sampler2D	uNoiseTex;

//varying vec4		texCoord;
in vec2 texCoord;

void main(void)
{
	vec4 p0			= texture( positions, texCoord );
	float invmass	= p0.a;
	
	vec4 v0			= texture( velocities, texCoord );
	float texAlpha	= v0.a;
	
	vec3 newV		= texture( uPerlinTex, texCoord * vec2( 5.0, 1.0 ) ).rgb * 2.0 - 1.0;
	
	float noiseVal	= texture( uNoiseTex, texCoord ).r;
	
//	if( texAlpha < 0.01 )
//		discard;

	vec3 force = p0.xyz - vec3( 0.0 );			// Calculate direction of force
    float d = length( force );					// Distance between objects
	d = normalize(d);
	force = normalize(force);					// Normalize vector (distance doesn't matter here, we just want this vector for direction
	
	float strength = invmass / (d * d);			// Calculate gravitational force magnitude
	force *= ( 1.0 * strength );
	
	float animDelay	= max( uTime * 4.0 -  texCoord.s - noiseVal, 0.0 );
	
	vec3 v1 = v0.xyz + newV * uStep;//( v0.xyz + newV * invmass ) * animDelay;
//	v1 *= ( 1.0 + uStep * 0.3 );
	vec3 p1 = p0.xyz + v1;
//	vec3 p1 = p0.xyz;
	
    //Render to positions texture
	// TODO: Figure out how to write to FBO
//    gl_FragData[0] = vec4( p1, invmass );
    //Render to velocities texture
//	gl_FragData[1] = vec4( v1, texAlpha ); //alpha component used for coloring
}

