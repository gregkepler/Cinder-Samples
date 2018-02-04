#version 150

uniform vec3	uAmbient;
//uniform vec3	uLightColor;
//uniform vec3	uLightDirection;
uniform float	uMaterialShininess = 1.0;
uniform vec3	uMaterialSpecularColor = vec3( 1.0, 1.0, 1.0 );
//uniform vec3	uSurfaceColor = vec3( 1.0 );


in vec4		ciPosition;
in vec3		Color;
in vec3		Normal;
in vec2		TexCoord;
in vec3		ec_pos;

out vec4 	oColor;

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
	
	//diffuse
	float diffuseCoefficient = max(0.0, dot(normal, surfaceToLight));
	vec3 diffuse = diffuseCoefficient * surfaceColor.rgb * lightIntensities;
	
	//specular
	vec3 reflection = normalize(reflect(-surfaceToLight, normal));
	float specularCoefficient = 0.0;
	if(diffuseCoefficient > 0.0)
		specularCoefficient = pow(max(0.0, dot(surfaceToCamera, reflection)), uMaterialShininess);
	vec3 specular = specularCoefficient * uMaterialSpecularColor * lightIntensities;
	
	
	//linear color (color before gamma correction)
	return ambient + (attenuation * (diffuse + specular));
}

void main( void )
{
/*
	vec3 N = normalize( cross( dFdx(ec_pos), dFdy(ec_pos) ) );	// for flat shading
	float diffuse = max(0.0, dot(N, uLightDirection));
	vec3 scatteredLight = uAmbient + uLightColor * diffuse;
	vec3 rgb = min( scatteredLight, vec3(1.0));
	
	vec3 color = vec3( 0 );
	
	
	oColor = vec4(rgb, 1.0);*/
	
	// --------------------
	
//	vec3 normal  = oNormal;
	vec3 normal = normalize( cross( dFdx(ec_pos), dFdy(ec_pos) ) );	// for flat shading
	
	vec3 surfaceModelPosition = ec_pos;
	vec3 surfaceToCamera = normalize( -surfaceModelPosition );
	
	
	vec3 linearColor = vec3(0);
	for(int i = 0; i < uNumLights; ++i){
		linearColor += ApplyLight( uLights[i], uAmbient, normal, surfaceModelPosition, surfaceToCamera);
	}
	
	vec3 gamma = vec3(1.0/2.2);
	oColor = vec4(pow(linearColor, gamma), oColor.a);
}
