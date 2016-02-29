#version 150

uniform mat4 ciModelView;
uniform mat4 ciProjectionMatrix;

uniform float uTime;

in vec4 ciPosition;
in vec2 ciTexCoord0;
in vec4 ciColor;

in mat4 vInstanceTransform;
in vec4 vInstanceData;

// out vec2 vertTexCoord0;
// out vec2 vertTexCoord1;
out vec4 vertColor;
out vec4 vertPosition;

void main(void)
{
	// Calculate instanced position.
	vec4 position = vInstanceTransform * ciPosition;

	// Slowly rotate and scale icon.
	// float c = cos( uTime );
	// float s = sin( uTime );
	// mat2  m = mat2( c, -s, s, c ); // rotate

	// float n = 0.45 + 0.075 * sin( uTime * 15.0 ); // scale

	// vertTexCoord1 = m * ( ciTexCoord0 * 2.0 - 1.0 ) * n + 0.5;
	// vertTexCoord1 = vertTexCoord1 * vec2( 0.25, 0.5 ) + vInstanceData.xy;

    //
	// vertTexCoord0 = ciTexCoord0;
    vertColor = vInstanceData;
    vertColor.a *= ciColor.a;
    vertPosition = position;

    gl_Position = ciProjectionMatrix * ciModelView * position;
}
