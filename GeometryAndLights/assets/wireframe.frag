#version 150

uniform mat4	ciModelView;
uniform vec3	uAmbient;
uniform vec3 uLightColor;
uniform vec3 uLightDirection;
//uniform float uBrightness;
uniform float uShininess = 20.0;
uniform float uStrength = 0.001; // extra factor to adjust shininess

uniform vec3 uPointLight1;
uniform vec3 uPointLight1Color		= vec3( 1.0, 0.0, 0.0 );
uniform float ConstantAttenuation	= 1.0; // attenuation coefficients
uniform float LinearAttenuation		= 1.0; // attenuation coefficients
uniform float QuadraticAttenuation	= 0.1; // attenuation coefficients
uniform vec3 uEyeDirection;
uniform float	uMaterialShininess = 1.0;
uniform vec3	uMaterialSpecularColor = vec3( 1.0, 1.0, 1.0 );

in VertexData {
	noperspective vec3 distance;
	vec3 normal;
	vec2 texcoord;
	vec3 mvPos;
	vec3 pos;
	vec3 light1pos;
} vVertexIn;


// array of lights
#define MAX_LIGHTS 10
uniform int uNumLights;
struct Light
{
	vec4 position;
	vec4 intensities; //a.k.a the color of the light
	float attenuation;
	float ambientCoefficient;
};

layout (std140) uniform Lights
{
	Light uLights[ MAX_LIGHTS ];
};


out vec4 oFragColor;




//vec3 pointLight( vec3 pos, vec3 normal, vec3 lightPos, vec3 lightColor )
vec3 pointLight( vec3 pos, vec3 normal, Light light )
{
	// point light
//	vec3 lightPosition = mat3(ciModelView) * light.position.xyz;
	vec3 lightPosition = light.position.xyz;
	vec3 lightDirection = lightPosition - pos;
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
	vec3 scatteredLight = light.intensities.rgb * diffuse * attenuation;
	vec3 reflectedLight = light.intensities.rgb * specular * attenuation;
	vec3 rgb = min(scatteredLight + reflectedLight, vec3(1.0));
	
	return rgb;
}


vec3 ApplyLight(Light light, vec3 surfaceColor, vec3 normal, vec3 surfaceModelPos, vec3 surfaceToCamera)
{
	
	// lighting properties
	vec3 lightPosition = light.position.xyz;
	vec3 lightIntensities = light.intensities.rgb;
	
	//point light
	vec3 surfaceToLight = normalize( lightPosition - surfaceModelPos );
	float distanceToLight = length( lightPosition - surfaceModelPos );
	float attenuation = 1.0 / (1.0 + light.attenuation * pow(distanceToLight, 2));
	
	//ambient
	vec3 ambient = light.ambientCoefficient * surfaceColor.rgb * lightIntensities;
//	vec3 ambient = 1.0 * surfaceColor.rgb * lightIntensities;
	
	//diffuse
	float diffuseCoefficient = max(0.0, dot(normal, surfaceToLight));
	vec3 diffuse = diffuseCoefficient * surfaceColor.rgb * lightIntensities;
	
	//specular
	vec3 reflection = normalize(reflect(-surfaceToLight, normal));
	float specularCoefficient = 0.0;
	if(diffuseCoefficient > 0.0)
		specularCoefficient = pow(max(0.0, dot(surfaceToCamera, reflection)), uStrength);
	vec3 specular = specularCoefficient * uMaterialSpecularColor * lightIntensities;
	
	
	//linear color (color before gamma correction)
//	return ambient + (attenuation * (diffuse + specular));
	return ambient + (diffuse);
	
	/*
	
	
	////
	// point light
	vec3 lightPosition = mat3(ciModelView) * light.position.xyz;
	vec3 lightIntensities = light.intensities.rgb;
	vec3 lightDirection = lightPosition - surfaceModelPos;
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
	vec3 scatteredLight = lightIntensities * diffuse * attenuation;
	vec3 reflectedLight = lightIntensities * specular * attenuation;
	vec3 rgb = min(scatteredLight + reflectedLight, vec3(1.0));
	
	return rgb;*/
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
	
	// ----
//	vec3 normal = normalize( cross( dFdx(pos), dFdy(pos) ) );	// for flat shading
	vec3 surfaceModelPosition = vVertexIn.mvPos;
	vec3 surfaceToCamera = normalize( -surfaceModelPosition );
/*
	vec3 linearColor = vec3(0);
	for(int i = 0; i < uNumLights; ++i){
		linearColor += ApplyLight( uLights[i], uAmbient, N, surfaceModelPosition, surfaceToCamera);
	}
	
	vec3 gamma = vec3(1.0/2.2);
	linearColor = pow(linearColor, gamma);*/
	// ----

	
	
	// determine frag distance to closest edge
	float fNearest = min( min( vVertexIn.distance[0], vVertexIn.distance[1] ), vVertexIn.distance[2] );
	float fEdgeIntensity = exp2( -1.0 * fNearest * fNearest );
//	float fEdgeIntensity = exp2( -2.0 * fNearest );
	float glowIntensity = exp2( -0.25 * fNearest ) * max( 0.1, smoothstep(1.0, 1.02, dist) );	// based on position distance
	
	// blend between edge color and face color
	vec3 vFaceColor = min( scatteredLight + reflectedLight, vec3(1.0) );
//	vec3 vFaceColor = min( linearColor, vec3(1.0) );
	
	vec3 ptColor1 = pointLight( vVertexIn.mvPos, vVertexIn.normal, uLights[0] );
//	vec3 ptColor1 = ApplyLight( uLights[0], uAmbient, N, vVertexIn.mvPos, surfaceToCamera );
//	vec3 ptColor1 = ApplyLight( uLights[0], scatteredLight, vVertexIn.normal, vVertexIn.mvPos, surfaceToCamera );
	vFaceColor += ptColor1;
//	vFaceColor += linearColor;
	
	vec3 vEdgeColor = min(vec3( 0.22, 0.65, 0.87 ) + reflectedLight, vec3( 1.0 ) );
	vEdgeColor += uLightColor * diffuse;
	vEdgeColor += ptColor1;
	
	vec3 rgb = mix( vFaceColor, vEdgeColor, (fEdgeIntensity * 0.2) + (glowIntensity * 0.5) );

	
	oFragColor = vec4(rgb, 0.9);
//	oFragColor = vec4(ptColor1, 0.9);
//	oFragColor = vec4(scatteredLight, 0.9);
//	oFragColor = vec4(linearColor, 0.9);
}
