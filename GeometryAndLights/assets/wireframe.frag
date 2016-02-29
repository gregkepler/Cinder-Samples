#version 150

uniform vec3 uAmbient;
uniform vec3 uLightColor;
uniform vec3 uLightDirection;
uniform float uBrightness;
uniform float uShininess = 20.0;
uniform float uStrength = 0.001; // extra factor to adjust shininess

uniform vec3 uPointLight1;
uniform vec3 uPointLight1Color		= vec3( 1.0, 0.0, 0.0 );
uniform float ConstantAttenuation	= 1.0; // attenuation coefficients
uniform float LinearAttenuation		= 1.0; // attenuation coefficients
uniform float QuadraticAttenuation	= 0.1; // attenuation coefficients
uniform vec3 uEyeDirection;

in VertexData {
	noperspective vec3 distance;
	vec3 normal;
	vec2 texcoord;
	vec3 mvPos;
	vec3 pos;
	vec3 light1pos;
} vVertexIn;


out vec4 oFragColor;

vec3 pointLight( vec3 pos, vec3 normal, vec3 lightPos, vec3 lightColor )
{
	// point light
	vec3 lightDirection = lightPos - pos;
	float lightDist = length( lightDirection );
	lightDirection = lightDirection / lightDist;
	float attenuation = 1.0 / ( ConstantAttenuation +
								LinearAttenuation * lightDist +
								QuadraticAttenuation * lightDist * lightDist );
	
	// the direction of maximum highlight also changes per fragment
	vec3 halfVector = normalize(lightDirection + uEyeDirection);
	float diffuse = max(0.0, dot(normal, lightDirection));
	float specular = max(0.0, dot(normal, halfVector));
	if (diffuse == 0.0)
		specular = 0.0;
	else
		specular = pow(specular, uShininess) * uStrength;
	vec3 scatteredLight = lightColor * diffuse * attenuation;
	vec3 reflectedLight = lightColor * specular * attenuation;
	vec3 rgb = min(scatteredLight + reflectedLight, vec3(1.0));
	
	return rgb;
}

void main()
{
	vec3 N = normalize(cross(dFdx(vVertexIn.mvPos), dFdy(vVertexIn.mvPos)));	// for flat shading
	N = mix( N, normalize(vVertexIn.normal), 0.25 );
	
	vec3 pos = vVertexIn.pos;
	float diffuse = max(0.0, dot(N, uLightDirection));
	vec3 scatteredLight = uAmbient + uLightColor * diffuse * 0.5;
	float dist = length(vVertexIn.pos);
	
	vec3 halfVector = vec3( 0.75, 0.75, 1.0 );
	float specular = max(0.0, dot(N, halfVector));
	

	// surfaces facing away from the light (negative dot products)
	// won’t be lit by the directional light
	if (diffuse == 0.0)
		specular = 0.0;
	else
		specular = pow(specular, uShininess); // sharpen the highlight
	vec3 lightColor = vec3(0.55, 1.0, 0.78);
	vec3 reflectedLight = lightColor * specular * uStrength;
//	vec3 reflectedLight = lightColor * 0.0;
	

	
	
	// determine frag distance to closest edge
	float fNearest = min( min( vVertexIn.distance[0], vVertexIn.distance[1] ), vVertexIn.distance[2] );
	float fEdgeIntensity = exp2( -1.0 * fNearest * fNearest );
//	float fEdgeIntensity = exp2( -2.0 * fNearest );
	float glowIntensity = exp2( -0.25 * fNearest ) * max( 0.1, smoothstep(1.0, 1.02, dist) );	// based on position distance
	
	// blend between edge color and face color
	vec3 vFaceColor = min( scatteredLight + reflectedLight, vec3(1.0) );
	vec3 ptColor1 = pointLight( vVertexIn.mvPos, vVertexIn.normal, vVertexIn.light1pos, uPointLight1Color );
	vFaceColor += ptColor1;
	
	vec3 vEdgeColor = min(vec3( 0.22, 0.65, 0.87 ) + reflectedLight, vec3( 1.0 ) );
	vEdgeColor += uLightColor * diffuse;
	vEdgeColor += ptColor1;
	
	vec3 rgb = mix( vFaceColor, vEdgeColor, (fEdgeIntensity * 0.2) + (glowIntensity * 0.5) );

	
	oFragColor = vec4(rgb, 0.9);
}
