#version 150

//uniform sampler2D uTex0;

in vec4 vertColor;
in vec4 vertPosition;

out vec4 fragColor;

void main(void)
{
	// const float kOuterRadius = 1.0;
	// const float kInnerRadius = 0.925;
    //
	// vec2 uv = vertTexCoord0 * 2.0 - 1.0;
	// float d = length( uv );
	// float w = fwidth( d );
    //
	// // Circle.
	// float alpha = smoothstep( kOuterRadius + w, kOuterRadius - w, d );
	// alpha -= smoothstep( kInnerRadius + w, kInnerRadius - w, d );
    //
	// // Only draw the front part.
	// alpha *= 0.5 + 0.02 * vertPosition.z;
    //
	// // Apply motion blur factor.
	float alpha = vertColor.a;

	fragColor.rgb = vertColor.rgb * alpha;
	fragColor.a = vertColor.a * alpha;
    // fragColor.rgb = vertColor.rgb;
    // fragColor.rgb = vec3(1, 1, 0);
    // fragColor.a = 0.0;
}
