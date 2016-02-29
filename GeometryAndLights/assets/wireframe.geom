#version 150

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

uniform vec2 uViewportSize;

in VertexData {
	vec3 normal;
	vec2 texcoord;
	vec3 mvPos;
	vec3 pos;
	vec3 light1pos;
} vVertexIn[];


out VertexData {
	noperspective vec3 distance;
	vec3 normal;
	vec2 texcoord;
	vec3 mvPos;
	vec3 pos;
	vec3 light1pos;
} vVertexOut;

void main()
{
//	float MEW = 100.0; // max edge width

	// taken from 'Single-Pass Wireframe Rendering'
	vec2 p0 = uViewportSize * gl_in[0].gl_Position.xy / gl_in[0].gl_Position.w;
	vec2 p1 = uViewportSize * gl_in[1].gl_Position.xy / gl_in[1].gl_Position.w;
	vec2 p2 = uViewportSize * gl_in[2].gl_Position.xy / gl_in[2].gl_Position.w;

	vec2 v0 = p2-p1;
	vec2 v1 = p2-p0;
	vec2 v2 = p1-p0;
	float fArea = abs( v1.x * v2.y - v1.y * v2.x );

	vVertexOut.distance = vec3( fArea / length( v0 ), 0, 0 );
	vVertexOut.normal = vVertexIn[0].normal;
	vVertexOut.texcoord = vVertexIn[0].texcoord;
	vVertexOut.mvPos = vVertexIn[0].mvPos;
	vVertexOut.pos = vVertexIn[0].pos;
	vVertexOut.light1pos = vVertexIn[0].light1pos;
	gl_Position = gl_in[0].gl_Position;
	EmitVertex();

	vVertexOut.distance = vec3( 0, fArea / length( v1 ), 0 );
	vVertexOut.normal = vVertexIn[1].normal;
	vVertexOut.texcoord = vVertexIn[1].texcoord;
	vVertexOut.mvPos = vVertexIn[1].mvPos;
	vVertexOut.pos = vVertexIn[1].pos;
	vVertexOut.light1pos = vVertexIn[1].light1pos;
	gl_Position = gl_in[1].gl_Position;
	EmitVertex();

	vVertexOut.distance = vec3( 0, 0, fArea / length( v2 ) );
	vVertexOut.normal = vVertexIn[2].normal;
	vVertexOut.texcoord = vVertexIn[2].texcoord;
	vVertexOut.mvPos = vVertexIn[2].mvPos;
	vVertexOut.pos = vVertexIn[2].pos;
	vVertexOut.light1pos = vVertexIn[2].light1pos;
	gl_Position = gl_in[2].gl_Position;
	EmitVertex();

	EndPrimitive();
}
